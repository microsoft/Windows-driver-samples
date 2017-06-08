/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Fdo.cpp

Abstract:

    FDO callbacks, functions, and types.

Environment:

    Kernel-mode only.

--*/

#include "Pch.h"
#include "Fdo.tmh"

EXTERN_C_START

EVT_WDF_DEVICE_PREPARE_HARDWARE Fdo_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE Fdo_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY Fdo_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT Fdo_EvtDeviceD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT Fdo_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_RESTART Fdo_EvtDeviceSelfManagedIoRestart;
EVT_WDF_WORKITEM Fdo_ConnectorAndNotificationWorkItem;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Fdo_Initialize (
    _In_ PFDO_CONTEXT FdoCtx
    );

EXTERN_C_END

#pragma alloc_text(PAGE, Fdo_Create)
#pragma alloc_text(PAGE, Fdo_Initialize)
#pragma alloc_text(PAGE, Fdo_EvtDevicePrepareHardware)
#pragma alloc_text(PAGE, Fdo_EvtDeviceReleaseHardware)
#pragma alloc_text(PAGE, Fdo_EvtDeviceD0Entry)
#pragma alloc_text(PAGE, Fdo_EvtDeviceD0Exit)
#pragma alloc_text(PAGE, Fdo_EvtDeviceSelfManagedIoInit)
#pragma alloc_text(PAGE, Fdo_EvtDeviceSelfManagedIoRestart)


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Fdo_Create (
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    PFDO_CONTEXT fdoCtx;
    WDFDEVICE wdfDevice;
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = Fdo_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = Fdo_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = Fdo_EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = Fdo_EvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = Fdo_EvtDeviceSelfManagedIoInit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = Fdo_EvtDeviceSelfManagedIoRestart;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WdfDeviceInitSetPowerPageable(DeviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FDO_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[DeviceInit: 0x%p] WdfDeviceCreate failed - %!STATUS!", DeviceInit, status);
        goto Exit;
    }

    fdoCtx = Fdo_GetContext(wdfDevice);
    fdoCtx->WdfDevice = wdfDevice;

    status = Fdo_Initialize(fdoCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = Ppm_Initialize(&fdoCtx->PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Device created", wdfDevice);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Fdo_Initialize (
    _In_ PFDO_CONTEXT FdoCtx
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_DEVICE_STATE deviceState;
    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    device = FdoCtx->WdfDevice;

    //
    // ACPI-enumerated devices are not disableable by default. Override that,
    // since there is no need for that restriction.
    //

    WDF_DEVICE_STATE_INIT(&deviceState);
    deviceState.NotDisableable = WdfFalse;
    WdfDeviceSetDeviceState(device, &deviceState);

	//
	// Create a workitem that will create connectors and enable notifications 
	// so that we don't block D0 Entry and thereby boot sequence of the system. 
	//
	
    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, Fdo_ConnectorAndNotificationWorkItem);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfWorkItemCreate(&workItemConfig, &attributes, &FdoCtx->ConnectorAndNotificationWorkItem);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] WdfWorkItemCreate for ConnectorAndNotificationWorkItem failed - %!STATUS!", device, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] FDO initialized", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return STATUS_SUCCESS;
}


NTSTATUS
Fdo_EvtDevicePrepareHardware (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;
    ULONG index;
    ULONG resourceCount;
    BOOLEAN allResourcesFound;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR res;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR rawRes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    fdoCtx = Fdo_GetContext(Device);
    resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    allResourcesFound = FALSE;

    for (index = 0; !allResourcesFound && (index < resourceCount); ++index)
    {
        res = WdfCmResourceListGetDescriptor(ResourcesTranslated, index);
        if (res->Type != CmResourceTypeMemory)
        {
            continue;
        }

        rawRes = WdfCmResourceListGetDescriptor(ResourcesRaw, index);

        //
        // Verify if address is below 4GB and not straddling the 4GB boundary.
        //

        if ((rawRes->u.Memory.Start.HighPart != 0) ||
            ((rawRes->u.Memory.Start.LowPart + rawRes->u.Memory.Length) <
                rawRes->u.Memory.Start.LowPart))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] Memory resource address (%I64x) not below 4GB", Device, rawRes->u.Memory.Start.QuadPart);
            goto Exit;
        }

        status = Ppm_PrepareHardware(&fdoCtx->PpmCtx, res->u.Memory.Start, res->u.Memory.Length);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        allResourcesFound = TRUE;
    }

    if (allResourcesFound == FALSE)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] Could not find required resources", Device);
        goto Exit;
    }

    status = Acpi_PrepareHardware(&fdoCtx->AcpiCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Prepare hardware completed", Device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return status;
}


NTSTATUS
Fdo_EvtDeviceReleaseHardware (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PFDO_CONTEXT fdoCtx;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    fdoCtx = Fdo_GetContext(Device);

    Acpi_ReleaseHardware(&fdoCtx->AcpiCtx);

    Ppm_ReleaseHardware(&fdoCtx->PpmCtx);

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Release hardware completed", Device);

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return STATUS_SUCCESS;
}


NTSTATUS
Fdo_EvtDeviceD0Entry (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    fdoCtx = Fdo_GetContext(Device);

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Entering D0 from %!WDF_POWER_DEVICE_STATE!", Device, PreviousState);

    status = Ppm_PowerOn(&fdoCtx->PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return status;
}


NTSTATUS
Fdo_EvtDeviceD0Exit (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PFDO_CONTEXT fdoCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    fdoCtx = Fdo_GetContext(Device);

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Exiting D0 to %!WDF_POWER_DEVICE_STATE!", Device, TargetState);

    Ppm_PowerOff(&fdoCtx->PpmCtx);

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return STATUS_SUCCESS;
}


NTSTATUS
Fdo_EvtDeviceSelfManagedIoInit (
    _In_ WDFDEVICE Device
    )
{
    PFDO_CONTEXT fdoCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    fdoCtx = Fdo_GetContext(Device);

    WdfWorkItemEnqueue(fdoCtx->ConnectorAndNotificationWorkItem);

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return STATUS_SUCCESS;
}


NTSTATUS
Fdo_EvtDeviceSelfManagedIoRestart (
    _In_ WDFDEVICE Device
    )
{
    NTSTATUS status;
    PPPM_CONTEXT ppmCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    ppmCtx = &Fdo_GetContext(Device)->PpmCtx;

    status = Ppm_EnableNotifications(ppmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return status;
}

VOID
Fdo_ConnectorAndNotificationWorkItem(
    _In_ WDFWORKITEM WorkItem
)
{
    NTSTATUS status;
    PPPM_CONTEXT ppmCtx;
    PFDO_CONTEXT fdoCtx;
    WDFDEVICE device;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    device = (WDFDEVICE)WdfWorkItemGetParentObject(WorkItem);

    fdoCtx = Fdo_GetContext(device);
    ppmCtx = &fdoCtx->PpmCtx;
    status = Ucm_CreateConnectors(ppmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Connector objects are ready. Now we can enable all notifications.
    //
    status = Ppm_EnableNotifications(ppmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    //
    // Failing to create the connectors or enable notifications are both
    // unrecoverable failures. Attempt to have WDF reload the driver.
    //
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] Failed to initialize PPM connectors - attempting to restart device.", device);
        WdfDeviceSetFailed(fdoCtx->WdfDevice, WdfDeviceFailedAttemptRestart);
    }

    TRACE_INFO(TRACE_FLAG_FDO, "[Device: 0x%p] Work item complete.", device);

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
}
