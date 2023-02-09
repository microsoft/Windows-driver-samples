//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "power.h"
#include "device.h"
#include "adapter.h"

_Use_decl_annotations_ NTSTATUS EvtDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE previousPowerState)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);
    UNREFERENCED_PARAMETER(previousPowerState);
    do
    {
        if (MbbBusIsStoped(deviceContext->BusHandle))
        {
            //
            //  device was in d3, open it again
            //
            status = MbbBusStart(deviceContext->BusHandle);

            if (!NT_SUCCESS(status))
            {
                WdfDeviceSetFailed(deviceContext->WdfDevice, WdfDeviceFailedAttemptRestart);
                break;
            }
        }
        else
        {
            MbbBusSetNotificationState(deviceContext->BusHandle, TRUE);
        }
        // Allow sessions to receive data from this point since RxQueueCreate happens later after D0Entry. We need cache NDPs receive before RxQueueCreate
        for (int i = 0; i < MBB_MAX_NUMBER_OF_SESSIONS; i++)
        {
            if (deviceContext->Sessions[i].NetAdapterContext == NULL)
            {
                continue;
            }
            deviceContext->Sessions[i].NetAdapterContext->AllowRxTraffic = TRUE;
        }
        MbbBusStartDataPipes(deviceContext->BusHandle);
    } while (FALSE);

    return status;
}

_Use_decl_annotations_ NTSTATUS EvtDeviceD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetPowerState)
{
    UNREFERENCED_PARAMETER(TargetPowerState);
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);

    MbbBusStopDataPipes(deviceContext->BusHandle);
    //
    // If the device is not armed for wake, close has been sent by MbbCx. All response available notifications from close in MbbCx to EvtDeviceD0Exit will be ignored.
    //
    // If the device is armed for wake, No response available notification after EvtDeviceArmWakeFromS0 where we stop the notification.
    if (!deviceContext->m_armedWake)
    {
        MbbBusStop(deviceContext->BusHandle);
    }
    return STATUS_SUCCESS;
}

_Use_decl_annotations_ NTSTATUS EvtDevicePreviewBitmapPattern(WDFDEVICE Device, NETWAKESOURCE BitmapPattern)
{
    // We don't need to look at the pattern itself, only how many we currently have
    UNREFERENCED_PARAMETER(BitmapPattern);

    NET_WAKE_SOURCE_LIST wakeSourceList;
    NET_WAKE_SOURCE_LIST_INIT(&wakeSourceList);
    NetDeviceGetWakeSourceList(Device, &wakeSourceList);

    auto const wakeSourceCount = NetWakeSourceListGetCount(&wakeSourceList);
    size_t numberOfPowerFilters = 0;

    // Count how many power filters are currently stored for this device
    for (size_t i = 0; i < wakeSourceCount; i++)
    {
        auto wakeSource = NetWakeSourceListGetElement(&wakeSourceList, i);
        auto const wakeSourceType = NetWakeSourceGetType(wakeSource);

        if (wakeSourceType == NetWakeSourceTypeBitmapPattern)
        {
            numberOfPowerFilters++;
        }
    }

    auto const deviceContext = WmbClassGetDeviceContext(Device);
    auto const maxPowerFilters = deviceContext->BusParams.PowerFiltersSupported;

    NT_ASSERT(numberOfPowerFilters <= maxPowerFilters);

    if (numberOfPowerFilters == maxPowerFilters)
    {
        return STATUS_NDIS_PM_WOL_PATTERN_LIST_FULL;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_ NTSTATUS EvtDeviceArmWakeFromS0(WDFDEVICE Device)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);
    deviceContext->m_armedWake = true;
    MbbBusSetNotificationState(deviceContext->BusHandle, FALSE);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_ VOID EvtDeviceDisarmWakeFromS0(WDFDEVICE Device)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);
    deviceContext->m_armedWake = false;
}
