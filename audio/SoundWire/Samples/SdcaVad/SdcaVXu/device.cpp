/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.cpp

Abstract:

    Plug and Play module. This file contains routines to handle pnp requests.

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>

#include "streamengine.h"
#include "device.h"
#include "CircuitDevice.h"
#include "ModuleCircuit.h"

#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "device.tmh"
#endif

UNICODE_STRING g_RegistryPath = {0};      // This is used to store the registry settings path for the driver

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS
CopyRegistrySettingsPath(
    _In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

Copies the following registry path to a global variable.

\REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters

Arguments:

RegistryPath - Registry path passed to DriverEntry

Returns:

NTSTATUS - SUCCESS if able to configure the framework

--*/

{
    PAGED_CODE();

    // Initializing the unicode string, so that if it is not allocated it will not be deallocated too.
    RtlInitUnicodeString(&g_RegistryPath, NULL);

    g_RegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);

    g_RegistryPath.Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_PAGED, g_RegistryPath.MaximumLength, DRIVER_TAG);

    if (g_RegistryPath.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // ExAllocatePool2 zeros memory.

    RtlAppendUnicodeToString(&g_RegistryPath, RegistryPath->Buffer);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetHwConfig
(
    _In_        PVOID                           Context,
    _In_        SDCAXU_HW_CONFIG_TYPE           HwConfigType,
    _In_opt_    PVOID                           HwConfigData,
    _In_        ULONG                           HwConfigDataSize
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    PSDCAXU_DEVICE_CONTEXT devCtx;
    devCtx = GetSdcaXuDeviceContext((WDFDEVICE)Context);

    if (HwConfigType == SdcaXuHwConfigTypeAcpiBlob &&
        NULL != HwConfigData &&
        sizeof(devCtx->SDCADeviceData.HwData) <= HwConfigDataSize)
    {
        RtlCopyMemory(&devCtx->SDCADeviceData.HwData, HwConfigData, sizeof(devCtx->SDCADeviceData.HwData));
    }
    else if (HwConfigType == SdcaXuHwConfigTypeAcpiBlob &&
             NULL != HwConfigData &&
             sizeof(SdcaXuAcpiBlob) <= HwConfigDataSize)
    {
        devCtx->SDCADeviceData.NumEndpoints = ((PSdcaXuAcpiBlob)HwConfigData)->NumEndpoints;
    }

    RETURN_NTSTATUS_IF_FAILED(SdcaXu_SetXUEntities((WDFDEVICE)Context));

    RETURN_NTSTATUS_IF_FAILED(SdcaXu_RegisterForInterrupts((WDFDEVICE)Context));

    RETURN_NTSTATUS_IF_FAILED(SdcaXu_SetJackOverride((WDFDEVICE)Context));

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetXUEntities(_In_ WDFDEVICE Device)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    PSDCAXU_DEVICE_CONTEXT devCtx;
    devCtx = GetSdcaXuDeviceContext(Device);

    if (devCtx->SDCADeviceData.bSDCAInterface)
    {
        ULONG xuEntities[ARRAYSIZE(devCtx->SDCADeviceData.HwData.AvailableXUEntities)] = { 0 };
        ULONG xuCount = 0;

        // A real driver would select the XU Entites to turn on
        //for (ULONG i = 0; i < ARRAYSIZE(devCtx->SDCADeviceData.HwData.AvailableXUEntities); ++i)
        //{
        //    if (devCtx->SDCADeviceData.HwData.AvailableXUEntities[i] != 0)
        //    {
        //        xuEntities[xuCount] = devCtx->SDCADeviceData.HwData.AvailableXUEntities[i];
        //        ++xuCount;
        //    }
        //}
        
        RETURN_NTSTATUS_IF_FAILED(devCtx->SDCADeviceData.SDCAInterface.EvtSetXUEntities(devCtx->SDCADeviceData.SDCAContext, xuCount, xuEntities));
        DrvLogInfo(g_SDCAVXuLog, FLAG_INIT, "SdcaXu_SetXUEntities - Configured %d XU Entities", xuCount);
    }
    else
    {
        DrvLogInfo(g_SDCAVXuLog, FLAG_INIT, "SdcaXu_SetXUEntities - No SDCA Interface available");
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_RegisterForInterrupts(_In_ WDFDEVICE Device)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    PSDCAXU_DEVICE_CONTEXT devCtx;
    devCtx = GetSdcaXuDeviceContext(Device);

    if (devCtx->SDCADeviceData.bSDCAInterface)
    {

        SDCAXU_INTERRUPT_INFO interruptInfo{ 0 };

        interruptInfo.Size = sizeof(SDCAXU_INTERRUPT_INFO);
        SDCA_INTERRUPT_DEFINE_MASK(interruptInfo.SDCAInterruptMask, 0);
        //SDCA_INTERRUPT_DEFINE_MASK(interruptInfo.SCPInterruptMask, 1, 2);
        //SDCA_INTERRUPT_DEFINE_MASK(interruptInfo.DataPortInterrupts[0], 1, 2);
        //SDCA_INTERRUPT_DEFINE_MASK(interruptInfo.DataPortInterrupts[1], 1, 5);

        RETURN_NTSTATUS_IF_FAILED(devCtx->SDCADeviceData.SDCAInterface.EvtRegisterForInterrupts(
            devCtx->SDCADeviceData.SDCAContext,
            &interruptInfo));

        DrvLogInfo(g_SDCAVXuLog, FLAG_INIT, "SdcaXu_RegisterForInterrupts - Registered for Interrupts");
    }
    else
    {
        RETURN_NTSTATUS(STATUS_NOINTERFACE);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetJackOverride(_In_ WDFDEVICE Device)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    PSDCAXU_DEVICE_CONTEXT devCtx;
    devCtx = GetSdcaXuDeviceContext(Device);

    if (devCtx->SDCADeviceData.bSDCAInterface)
    {
        //RETURN_NTSTATUS_IF_FAILED(devCtx->SDCADeviceData.SDCAInterface.EvtSetJackOverride(
        //    devCtx->SDCADeviceData.SDCAContext,
        //    TRUE));

        DrvLogInfo(g_SDCAVXuLog, FLAG_INIT, "SdcaXu_SetJackOverride - Skipping enabling Jack Override");
    }
    else
    {
        RETURN_NTSTATUS(STATUS_NOINTERFACE);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetEndpointConfig
(
    _In_        PVOID                           Context,
    _In_        SDCAXU_ENDPOINT_CONFIG_TYPE     EndpointConfigType,
    _In_opt_    PVOID                           EndpointConfigData,
    _In_        ULONG                           EndpointConfigDataSize
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    RETURN_NTSTATUS_IF_TRUE(NULL == EndpointConfigData, STATUS_INVALID_PARAMETER);

    WDFDEVICE Device = (WDFDEVICE)Context;

    switch (EndpointConfigType)
    {
    case SdcaXuEndpointConfigTypeAcxCircuitConfig:
    {
        RETURN_NTSTATUS_IF_TRUE_MSG((sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) > EndpointConfigDataSize ||
            sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) > ((PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData)->cbSize),
            STATUS_INVALID_PARAMETER_1, L"%d %d", EndpointConfigDataSize, ((PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData)->cbSize);

        PSDCAXU_ACX_CIRCUIT_CONFIG circuitConfig = (PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData;
        //
        // Add Xu circuit. 
        //
        if (AcxCircuitTypeRender == circuitConfig->CircuitType)
        {
            RETURN_NTSTATUS_IF_FAILED_MSG(SdcaXu_AddRenders(Device, circuitConfig), L"Device %p", Device);
        }
        else if (AcxCircuitTypeCapture == circuitConfig->CircuitType)
        {
            RETURN_NTSTATUS_IF_FAILED_MSG(SdcaXu_AddCaptures(Device, circuitConfig), L"Device %p", Device);
        }
    }
    break;

    default:
        RETURN_NTSTATUS_MSG(STATUS_INVALID_PARAMETER_2, L"%d", EndpointConfigType);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_RemoveEndpointConfig
(
    _In_        PVOID                           Context,
    _In_        SDCAXU_ENDPOINT_CONFIG_TYPE     EndpointConfigType,
    _In_opt_    PVOID                           EndpointConfigData,
    _In_        ULONG                           EndpointConfigDataSize
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVXuLog);

    RETURN_NTSTATUS_IF_TRUE(NULL == EndpointConfigData, STATUS_INVALID_PARAMETER);

    WDFDEVICE Device = (WDFDEVICE)Context;

    switch (EndpointConfigType)
    {
    case SdcaXuEndpointConfigTypeAcxCircuitConfig:
    {
        RETURN_NTSTATUS_IF_TRUE_MSG((sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) > EndpointConfigDataSize ||
            sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) > ((PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData)->cbSize),
            STATUS_INVALID_PARAMETER_1, L"%d %d", EndpointConfigDataSize, ((PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData)->cbSize);

        PSDCAXU_ACX_CIRCUIT_CONFIG circuitConfig = (PSDCAXU_ACX_CIRCUIT_CONFIG)EndpointConfigData;

        PSDCAXU_DEVICE_CONTEXT devCtx = GetSdcaXuDeviceContext(Device);
        for (ULONG i = 0; i < ARRAYSIZE(devCtx->EndpointDevices); ++i)
        {
            if (circuitConfig->ComponentUri.Length > 0)
            {
                if (devCtx->EndpointDevices[i].CircuitUri.Buffer &&
                    RtlEqualUnicodeString(&circuitConfig->ComponentUri, &devCtx->EndpointDevices[i].CircuitUri, TRUE /* case insensitive */))
                {
                    DrvLogInfo(g_SDCAVXuLog, FLAG_DDI, L"Xu Device %p removing circuit device %p with component URI %ls",
                        Device, devCtx->EndpointDevices[i].CircuitDevice, circuitConfig->ComponentUri.Buffer);
                    AcxDeviceRemoveCircuitDevice(Device, devCtx->EndpointDevices[i].CircuitDevice);
                    ExFreePool(devCtx->EndpointDevices[i].CircuitUri.Buffer);
                    RtlZeroMemory(&devCtx->EndpointDevices[i], sizeof(ENDPOINT_DEVICE_PAIR));
                    break;
                }
            }
            else if (IsEqualGUID(devCtx->EndpointDevices[i].CircuitId, circuitConfig->ComponentID))
            {
                DrvLogInfo(g_SDCAVXuLog, FLAG_DDI, L"Xu Device %p removing circuit device %p with component ID %!GUID!",
                    Device, devCtx->EndpointDevices[i].CircuitDevice, &circuitConfig->ComponentID);
                AcxDeviceRemoveCircuitDevice(Device, devCtx->EndpointDevices[i].CircuitDevice);
                if (devCtx->EndpointDevices[i].CircuitUri.Buffer)
                {
                    ExFreePool(devCtx->EndpointDevices[i].CircuitUri.Buffer);
                }
                RtlZeroMemory(&devCtx->EndpointDevices[i], sizeof(ENDPOINT_DEVICE_PAIR));
                break;
            }
        }
    }
    break;

    default:
        RETURN_NTSTATUS_MSG(STATUS_INVALID_PARAMETER_2, L"%d", EndpointConfigType);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_InterruptHandler
(
    _In_ PVOID Context,
    _Inout_ PSDCAXU_INTERRUPT_INFO Interrupt
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Interrupt SCP %08x "
        L"DP %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
        L"SDCA %08x",
        Device,
        Interrupt->SCPInterruptMask,
        Interrupt->DataPortInterrupts[0],
        Interrupt->DataPortInterrupts[1],
        Interrupt->DataPortInterrupts[2],
        Interrupt->DataPortInterrupts[3],
        Interrupt->DataPortInterrupts[4],
        Interrupt->DataPortInterrupts[5],
        Interrupt->DataPortInterrupts[6],
        Interrupt->DataPortInterrupts[7],
        Interrupt->DataPortInterrupts[8],
        Interrupt->DataPortInterrupts[9],
        Interrupt->DataPortInterrupts[10],
        Interrupt->DataPortInterrupts[11],
        Interrupt->DataPortInterrupts[12],
        Interrupt->DataPortInterrupts[13],
        Interrupt->DataPortInterrupts[14],
        Interrupt->SDCAInterruptMask);
    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuPowerStateChangeHandlerPre(_In_ PVOID Context, _In_ ULONG PDE_EntityId, _In_ SDCAXU_POWER_STATE OldState, _In_ SDCAXU_POWER_STATE NewState)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Power State Change Pre for Entity 0x%x, Current State %d, New State %d",
        Device,
        PDE_EntityId,
        OldState,
        NewState);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuPowerStateChangeHandlerPost(_In_ PVOID Context, _In_ ULONG PDE_EntityId, _In_ SDCAXU_POWER_STATE OldState, _In_ SDCAXU_POWER_STATE NewState)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Power State Change Post for Entity 0x%x, Previous State %d, New State %d",
        Device,
        PDE_EntityId,
        OldState,
        NewState);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuJackStateChangeHandler(_In_ PVOID Context, _In_ ULONG GroupEntityId, _In_ ULONG DetectedMode, _In_ SDCAXU_JACK_EVENT JackEvent)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    PSDCAXU_DEVICE_CONTEXT devCtx = GetSdcaXuDeviceContext(Device);

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Jack State Change for Entity 0x%x, Detected Mode %d, Event %d, will use %d as Selected Mode",
        Device,
        GroupEntityId,
        DetectedMode,
        JackEvent,
        DetectedMode);

    // A real XU driver would probably do more with this.
    status = devCtx->SDCADeviceData.SDCAInterface.EvtSetJackSelectedMode(
        devCtx->SDCADeviceData.SDCAContext,
        GroupEntityId,
        DetectedMode);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuFunctionHasBeenResetHandler(_In_ PVOID Context)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Device %p Handling Function_Has_Been_Reset", Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuFunctionNeedsInitializationHandler(_In_ PVOID Context)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Device %p Handling Function_Needs_Initialization", Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuFunctionFaultHandler(_In_ PVOID Context)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Device %p Handling Function_Fault", Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuUMPSequenceFaultHandler(_In_ PVOID Context)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Device %p Handling Function_UMP_Sequence_Fault", Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuStreamingStoppedAbnormallyHandler(_In_ PVOID Context)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Device %p Handling Streaming_Stopped_Abnormally", Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuCommitGroupHandler(_In_ PVOID Context, _In_ PSDCAXU_NOTIFICATION_COMMIT_GROUP CommitGroup)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Commit Group notification with Commit Group %#x",
        Device,
        CommitGroup->CommitGroupHandle);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuPostureHandler(_In_ PVOID Context, _In_ PSDCAXU_NOTIFICATION_POSTURE Posture)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling Posture notification with Posture %d",
        Device,
        Posture->Posture);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuFdlBeginHandler(_In_ PVOID Context, _In_ PSDCAXU_NOTIFICATION_FDL_BEGIN FdlBegin)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling FDL Begin notification for entity %#x",
        Device,
        FdlBegin->FdlEntityId);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXuFdlEndHandler(_In_ PVOID Context, _In_ PSDCAXU_NOTIFICATION_FDL_END FdlEnd)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;

    WDFDEVICE Device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling FDL End notification for entity %#x with status %!STATUS!",
        Device,
        FdlEnd->FdlEntityId,
        FdlEnd->FdlStatus);

    return status;
}

PAGED_CODE_SEG
NTSTATUS SdcaXu_ChangeNotification(
    _In_        PVOID                       Context,
    _In_        SDCAXU_NOTIFICATION_TYPE    NotificationType,
    _In_opt_    PVOID                       NotificationData,
    _In_        ULONG                       NotificationDataSize
)
{
    PAGED_CODE();

    switch (NotificationType)
    {
    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeJackDetect:
        if (sizeof(SDCAXU_NOTIFICATION_JACK_DETECT) <= NotificationDataSize)
        {
            PSDCAXU_NOTIFICATION_JACK_DETECT pJackNotification = PSDCAXU_NOTIFICATION_JACK_DETECT(NotificationData);
            return SdcaXuJackStateChangeHandler(Context, pJackNotification->GroupEntityId, pJackNotification->DetectedMode, pJackNotification->JackEvent);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypePowerPre:
        if (sizeof(SDCAXU_NOTIFICATION_POWER) <= NotificationDataSize)
        {
            PSDCAXU_NOTIFICATION_POWER pPowerNotification = PSDCAXU_NOTIFICATION_POWER(NotificationData);
            return SdcaXuPowerStateChangeHandlerPre(Context, pPowerNotification->PowerDomainEntityId, pPowerNotification->OldState, pPowerNotification->NewState);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypePowerPost:
        if (sizeof(SDCAXU_NOTIFICATION_POWER) <= NotificationDataSize)
        {
            PSDCAXU_NOTIFICATION_POWER pPowerNotification = PSDCAXU_NOTIFICATION_POWER(NotificationData);
            return SdcaXuPowerStateChangeHandlerPost(Context, pPowerNotification->PowerDomainEntityId, pPowerNotification->OldState, pPowerNotification->NewState);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeHardwareReset:
        if (0 == NotificationDataSize)
        {
            return SdcaXuFunctionHasBeenResetHandler(Context);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeFunctionNeedsInitialization:
        if (0 == NotificationDataSize)
        {
            return SdcaXuFunctionNeedsInitializationHandler(Context);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeFunctionFault:
        if (0 == NotificationDataSize)
        {
            return SdcaXuFunctionFaultHandler(Context);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeUMPSequenceFault:
        if (0 == NotificationDataSize)
        {
            return SdcaXuUMPSequenceFaultHandler(Context);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeStreamingStoppedAbnormally:
        if (0 == NotificationDataSize)
        {
            return SdcaXuStreamingStoppedAbnormallyHandler(Context);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeCommitGroup:
        if (sizeof(SDCAXU_NOTIFICATION_COMMIT_GROUP) <= NotificationDataSize)
        {
            return SdcaXuCommitGroupHandler(Context, (PSDCAXU_NOTIFICATION_COMMIT_GROUP)NotificationData);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypePosture:
        if (sizeof(SDCAXU_NOTIFICATION_POSTURE) <= NotificationDataSize)
        {
            return SdcaXuPostureHandler(Context, (PSDCAXU_NOTIFICATION_POSTURE)NotificationData);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;
    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeFdlBegin:
        if (sizeof(SDCAXU_NOTIFICATION_FDL_BEGIN) <= NotificationDataSize)
        {
            return SdcaXuFdlBeginHandler(Context, (PSDCAXU_NOTIFICATION_FDL_BEGIN)NotificationData);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;
    case SDCAXU_NOTIFICATION_TYPE::SdcaXuNotificationTypeFdlEnd:
        if (sizeof(SDCAXU_NOTIFICATION_FDL_END) <= NotificationDataSize)
        {
            return SdcaXuFdlEndHandler(Context, (PSDCAXU_NOTIFICATION_FDL_END)NotificationData);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_RetrieveSwftFileOverride(
    _In_        PVOID   Context,
    _In_        USHORT  VendorID,
    _In_        ULONG   FileID,
    _In_        USHORT  SwftFileVersion,
    _In_        ULONG   SwftFileLength,
    _Out_       PUSHORT NewFileVersion,
    _Out_       PULONG  NewFileLength,
    _In_        ULONG   NewFileBufferLength,
    _Out_writes_bytes_opt_(NewFileBufferLength)
                PVOID   NewFileBuffer
)
{
    PAGED_CODE();
    const ULONG replaceFiles[][2] = {
        // Using the Microsoft Vendor ID for testing and demonstration purposes - XU developer
        // must use a different appropriate vendor ID
        { 0x02cb, 0x00000001}
    };
    const USHORT newFileVersion = 0x1010;

    BYTE newFileData[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };

    WDFDEVICE device = (WDFDEVICE)Context;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
        L"Device %p Handling SWFT File Override for VendorID %#x FileID %#x Swft Version %#x Length %d",
        device,
        VendorID,
        FileID,
        SwftFileVersion,
        SwftFileLength);

    if (NewFileBufferLength > 0 &&
        (!NewFileVersion || !NewFileLength || !NewFileBuffer))
    {
        RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }
    else if (NewFileBufferLength == 0 && !NewFileLength)
    {
        RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    // An XU developer may choose to override a SWFT file only if the version matches certain criteria
    // or they may choose to override based on other criteria or always.
    for (ULONG file = 0; file < ARRAYSIZE(replaceFiles); ++file)
    {
        if (replaceFiles[file][0] == VendorID && replaceFiles[file][1] == FileID && SwftFileVersion < newFileVersion)
        {
            if (NewFileBufferLength >= sizeof(newFileData))
            {
                DrvLogInfo(g_SDCAVXuLog, FLAG_INFO,
                    L"Device %p Overriding SWFT file for VendorID %#x FileID %#x, new Version %#x Length %d",
                    device,
                    VendorID,
                    FileID,
                    newFileVersion,
                    sizeof(newFileData));

                RtlCopyMemory(NewFileBuffer, newFileData, sizeof(newFileData));
                *NewFileVersion = newFileVersion;
                *NewFileLength = sizeof(newFileData);
                return STATUS_SUCCESS;
            }
            else
            {
                *NewFileLength = sizeof(newFileData);
                RETURN_NTSTATUS(STATUS_BUFFER_OVERFLOW);
            }
        }
    }

    return STATUS_NOT_FOUND;
}

PAGED_CODE_SEG
_Use_decl_annotations_
NTSTATUS
EvtSDCAVXuProcessQueryInterfaceRequest(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData
)
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVXuLog);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUID(*InterfaceType, SDCAXU_INTERFACE) &&
        ExposedInterface->Size >= sizeof(SDCAXU_INTERFACE_V0102)&&
        ExposedInterface->Version == SDCAXU_INTERFACE_VERSION_0102)
    {
        PSDCAXU_INTERFACE_V0102 pSdcaXuInterface = PSDCAXU_INTERFACE_V0102(ExposedInterface);

        pSdcaXuInterface->InterfaceHeader.Context = Device;
        pSdcaXuInterface->InterfaceHeader.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        pSdcaXuInterface->InterfaceHeader.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        pSdcaXuInterface->EvtSetHwConfig = SdcaXu_SetHwConfig;
        pSdcaXuInterface->EvtSetEndpointConfig = SdcaXu_SetEndpointConfig;
        pSdcaXuInterface->EvtRemoveEndpointConfig = SdcaXu_RemoveEndpointConfig;
        pSdcaXuInterface->EvtInterruptHandler = SdcaXu_InterruptHandler;
        pSdcaXuInterface->EvtChangeNotification = SdcaXu_ChangeNotification;
        pSdcaXuInterface->EvtRetrieveSwftFileOverride = SdcaXu_RetrieveSwftFileOverride;

        //
        // Check if SDCA has provided its own functions for 2-way communication
        //
        if (pSdcaXuInterface->EvtSetXUEntities &&
            pSdcaXuInterface->EvtRegisterForInterrupts &&
            pSdcaXuInterface->EvtSetJackOverride &&
            pSdcaXuInterface->EvtSetJackSelectedMode &&
            pSdcaXuInterface->EvtPDEPowerReferenceAcquire &&
            pSdcaXuInterface->EvtPDEPowerReferenceRelease &&
            pSdcaXuInterface->EvtReadDeferredAudioControls &&
            pSdcaXuInterface->EvtWriteDeferredAudioControls)
        {
            PSDCAXU_DEVICE_CONTEXT devCtx;
            devCtx = GetSdcaXuDeviceContext(Device);

            RtlZeroMemory(&devCtx->SDCADeviceData.SDCAInterface, sizeof(SDCAXU_INTERFACE_V0102));
            devCtx->SDCADeviceData.SDCAInterface.EvtSetXUEntities = pSdcaXuInterface->EvtSetXUEntities;
            devCtx->SDCADeviceData.SDCAInterface.EvtRegisterForInterrupts = pSdcaXuInterface->EvtRegisterForInterrupts;
            devCtx->SDCADeviceData.SDCAInterface.EvtSetJackOverride = pSdcaXuInterface->EvtSetJackOverride;
            devCtx->SDCADeviceData.SDCAInterface.EvtSetJackSelectedMode = pSdcaXuInterface->EvtSetJackSelectedMode;
            devCtx->SDCADeviceData.SDCAInterface.EvtPDEPowerReferenceAcquire = pSdcaXuInterface->EvtPDEPowerReferenceAcquire;
            devCtx->SDCADeviceData.SDCAInterface.EvtPDEPowerReferenceRelease = pSdcaXuInterface->EvtPDEPowerReferenceRelease;
            devCtx->SDCADeviceData.SDCAInterface.EvtReadDeferredAudioControls = pSdcaXuInterface->EvtReadDeferredAudioControls;
            devCtx->SDCADeviceData.SDCAInterface.EvtWriteDeferredAudioControls = pSdcaXuInterface->EvtWriteDeferredAudioControls;

            devCtx->SDCADeviceData.SDCAContext = ExposedInterfaceSpecificData;
            devCtx->SDCADeviceData.bSDCAInterface = WdfTrue;
        }
    }
    else if (IsEqualGUID(*InterfaceType, SDCAXU_INTERFACE) &&
        ExposedInterface->Size >= sizeof(SDCAXU_INTERFACE_V0101)&&
        ExposedInterface->Version == SDCAXU_INTERFACE_VERSION_0101)
    {
        PSDCAXU_INTERFACE_V0101 pSdcaXuInterface = PSDCAXU_INTERFACE_V0101(ExposedInterface);

        pSdcaXuInterface->InterfaceHeader.Context = Device;
        pSdcaXuInterface->InterfaceHeader.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        pSdcaXuInterface->InterfaceHeader.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        pSdcaXuInterface->EvtSetHwConfig = SdcaXu_SetHwConfig;
        pSdcaXuInterface->EvtSetEndpointConfig = SdcaXu_SetEndpointConfig;
        pSdcaXuInterface->EvtRemoveEndpointConfig = SdcaXu_RemoveEndpointConfig;
        pSdcaXuInterface->EvtInterruptHandler = SdcaXu_InterruptHandler;
        pSdcaXuInterface->EvtChangeNotification = SdcaXu_ChangeNotification;

        //
        // Check if SDCA has provided its own functions for 2-way communication
        //
        if (pSdcaXuInterface->EvtSetXUEntities &&
            pSdcaXuInterface->EvtRegisterForInterrupts &&
            pSdcaXuInterface->EvtSetJackOverride &&
            pSdcaXuInterface->EvtSetJackSelectedMode &&
            pSdcaXuInterface->EvtPDEPowerReferenceAcquire &&
            pSdcaXuInterface->EvtPDEPowerReferenceRelease &&
            pSdcaXuInterface->EvtReadDeferredAudioControls &&
            pSdcaXuInterface->EvtWriteDeferredAudioControls)
        {
            PSDCAXU_DEVICE_CONTEXT devCtx;
            devCtx = GetSdcaXuDeviceContext(Device);

            RtlZeroMemory(&devCtx->SDCADeviceData.SDCAInterface, sizeof(SDCAXU_INTERFACE_V0101));
            devCtx->SDCADeviceData.SDCAInterface.EvtSetXUEntities = pSdcaXuInterface->EvtSetXUEntities;
            devCtx->SDCADeviceData.SDCAInterface.EvtRegisterForInterrupts = pSdcaXuInterface->EvtRegisterForInterrupts;
            devCtx->SDCADeviceData.SDCAInterface.EvtSetJackOverride = pSdcaXuInterface->EvtSetJackOverride;
            devCtx->SDCADeviceData.SDCAInterface.EvtSetJackSelectedMode = pSdcaXuInterface->EvtSetJackSelectedMode;
            devCtx->SDCADeviceData.SDCAInterface.EvtPDEPowerReferenceAcquire = pSdcaXuInterface->EvtPDEPowerReferenceAcquire;
            devCtx->SDCADeviceData.SDCAInterface.EvtPDEPowerReferenceRelease = pSdcaXuInterface->EvtPDEPowerReferenceRelease;
            devCtx->SDCADeviceData.SDCAInterface.EvtReadDeferredAudioControls = pSdcaXuInterface->EvtReadDeferredAudioControls;
            devCtx->SDCADeviceData.SDCAInterface.EvtWriteDeferredAudioControls = pSdcaXuInterface->EvtWriteDeferredAudioControls;

            devCtx->SDCADeviceData.SDCAContext = ExposedInterfaceSpecificData;
            devCtx->SDCADeviceData.bSDCAInterface = WdfTrue;
        }
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
    }

    RETURN_NTSTATUS_IF_FAILED(status);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SDCAVXuAddDDI(
    _In_ WDFDEVICE device
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;
    WDF_QUERY_INTERFACE_CONFIG qiConfig;

    //
    // Initialize the qiConfig structure
    //
    WDF_QUERY_INTERFACE_CONFIG_INIT(
        &qiConfig,
        NULL,
        &SDCAXU_INTERFACE,
        EvtSDCAVXuProcessQueryInterfaceRequest
    );

    qiConfig.ImportInterface = WdfTrue;

    //
    // Create the interface
    //
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceAddQueryInterface(device, &qiConfig));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtBusDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = SdcaXu_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = SdcaXu_EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Specify the type of context needed. 
    // Use default locking, i.e., none.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXu_EvtDeviceContextCleanup;
    
    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG devInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&devInitCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(DeviceInit, &devInitCfg));

    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, FALSE);

    WdfFdoInitSetFilter(DeviceInit);
    
    //
    // Create the device.
    //
    WDFDEVICE device = NULL;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&DeviceInit, &attributes, &device));

    //
    // Init SdcaXu's device context.
    //
    PSDCAXU_DEVICE_CONTEXT devCtx;
    devCtx = GetSdcaXuDeviceContext(device);
    ASSERT(devCtx != NULL);
    devCtx->Render = NULL;

    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(device, &devCfg));

    //
    // Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
    // that you don't get the popup in usermode (on Win2K) when you surprise
    // remove the device.
    //
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(device, &pnpCaps);

    //
    // Add QI based Direct Device Interface
    //
    RETURN_NTSTATUS_IF_FAILED(SDCAVXuAddDDI(device));

    //
    // Create Raw PDO for ACX circuits
    //
    RETURN_NTSTATUS_IF_FAILED(SDCAVXu_CreateCircuitDevice(device));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtDevicePrepareHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceList,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtDeviceReleaseHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver releases the h/w resources allocated in the 
    prepare h/w callback.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS                        status;
    PSDCAXU_DEVICE_CONTEXT          devCtx;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    devCtx = GetSdcaXuDeviceContext(Device);
    ASSERT(devCtx != NULL);


    status = STATUS_SUCCESS;
    
    return status;
}

#pragma code_seg()
VOID
SdcaXu_EvtDeviceContextCleanup(
    _In_ WDFOBJECT      WdfDevice
)
/*++

Routine Description:

    In this callback, it cleans up device context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    PSDCAXU_DEVICE_CONTEXT  devCtx;

    devCtx = GetSdcaXuDeviceContext(WdfDevice);
    ASSERT(devCtx != NULL);

    if (devCtx)
    {
        for (ULONG i = 0; i < ARRAYSIZE(devCtx->EndpointDevices); ++i)
        {
            if (devCtx->EndpointDevices[i].CircuitUri.Buffer)
            {
                // Since the Buffer item is part of the actual context memory block, it's guaranteed to be 0-valued until we use it
#pragma prefast(suppress: 6001 , "C6001	Using uninitialized memor:	Using uninitialized memory '*devCtx.EndpointDevices.CircuitUri.Buffer" )
                ExFreePool(devCtx->EndpointDevices[i].CircuitUri.Buffer);
                RtlZeroMemory(&devCtx->EndpointDevices[i], sizeof(devCtx->EndpointDevices[i]));
            }
        }
    }
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_ReplicateFormats(
    _In_ ACXPIN             Pin,
    _In_ ACXTARGETCIRCUIT   TargetCircuit,
    _In_ ULONG              TargetPinId
)
{
    PAGED_CODE();

    // Using the opposite Pin Id to find the upstream pin only works here because
    // we only register for EvtAcxPinConnected on the downstream pin and the XU circuit
    // has only two pins (upstream and downstream). If the XU driver exposes more than
    // two pins, the following logic would need to be updated to find correct upstream pin.

    ACXPIN upstreamPin;
    if (AcxPinGetId(Pin) == 0)
    {
        upstreamPin = AcxCircuitGetPinById(AcxPinGetCircuit(Pin), 1);
    }
    else
    {
        upstreamPin = AcxCircuitGetPinById(AcxPinGetCircuit(Pin), 0);
    }

    if (!upstreamPin)
    {
        RETURN_NTSTATUS(STATUS_UNSUCCESSFUL);
    }

    ACXTARGETPIN targetPin = AcxTargetCircuitGetTargetPin(TargetCircuit, TargetPinId);
    if (!targetPin)
    {
        RETURN_NTSTATUS(STATUS_UNSUCCESSFUL);
    }

    // Don't delete the target pin - it will be cleaned up when the target circuit is cleaned up by ACX

    GUID targetModes[] =
    {
        AUDIO_SIGNALPROCESSINGMODE_RAW,
        AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        AUDIO_SIGNALPROCESSINGMODE_SPEECH
    };

    ULONG totalFormats = 0;
    for (ULONG modeIdx = 0; modeIdx < ARRAYSIZE(targetModes); ++modeIdx)
    {
        ACXDATAFORMATLIST targetFormatList;
        ACXDATAFORMATLIST localFormatList = nullptr;
        NTSTATUS status = AcxTargetPinRetrieveModeDataFormatList(targetPin, targetModes+modeIdx, &targetFormatList);
        if (!NT_SUCCESS(status))
        {
            // If the downstream pin doesn't support any formats for this mode, make sure we clear out our host pin
            // formats for this mode as well.
            if (modeIdx == 0)
            {
                localFormatList = AcxPinGetRawDataFormatList(upstreamPin);
            }
            else
            {
                // Ignore the status
                AcxPinRetrieveModeDataFormatList(upstreamPin, targetModes + modeIdx, &localFormatList);
            }
            if (localFormatList)
            {
                RETURN_NTSTATUS_IF_FAILED(SdcaVad_ClearDataFormatList(localFormatList));
            }
            continue;
        }

        RETURN_NTSTATUS_IF_FAILED(SdcaVad_RetrieveOrCreateDataFormatList(upstreamPin, targetModes + modeIdx, &localFormatList));

        RETURN_NTSTATUS_IF_FAILED(SdcaVad_ClearDataFormatList(localFormatList));

        ULONG formatCount = 0;
        RETURN_NTSTATUS_IF_FAILED(SdcaVad_CopyFormats(targetFormatList, localFormatList, &formatCount));

        totalFormats += formatCount;
    }

    if (totalFormats == 0)
    {
        RETURN_NTSTATUS(STATUS_NO_MATCH);
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
VOID
SdcaXu_EvtPinConnected(
    _In_ ACXPIN Pin,
    _In_ ACXTARGETCIRCUIT TargetCircuit,
    _In_ ULONG TargetPinId
)
{
    NTSTATUS status;

    PAGED_CODE();

    // Call the worker so we can trace the return of failures
    status = SdcaXu_ReplicateFormats(Pin, TargetCircuit, TargetPinId);

    // If we found no formats, there's not much we can do about it.
    if (!NT_SUCCESS(status))
    {
        DrvLogError(g_SDCAVXuLog, FLAG_INIT, L"Failed to replicate downstream formats to upstream pin, %!STATUS!", status);
    }
}

#pragma code_seg()
VOID
SdcaXu_EvtStreamDestroy(
    _In_ WDFOBJECT Object
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    NTSTATUS status = STATUS_SUCCESS;
    auto exit = scope_exit([&status]() {
        if (!NT_SUCCESS(status))
        {
            DrvLogError(g_SDCAVXuLog, FLAG_INIT, L"SdcaXu_EvtStreamDestroy - failed, %!STATUS!", status);
        }
    });

    ctx = GetSdcaXuStreamContext((ACXSTREAM)Object);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;
    ctx->StreamEngine = NULL;
    if (streamEngine)
    {
        delete streamEngine;
    }
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamGetHwLatency(
    _In_ ACXSTREAM Stream,
    _Out_ ULONG * FifoSize,
    _Out_ ULONG * Delay
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->GetHWLatency(FifoSize, Delay);
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamPrepareHardware(
    _In_ ACXSTREAM Stream
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->PrepareHardware();
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamReleaseHardware(
    _In_ ACXSTREAM Stream
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->ReleaseHardware();
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamRun(
    _In_ ACXSTREAM Stream
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->Run();
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamPause(
    _In_ ACXSTREAM Stream
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->Pause();
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtStreamAssignDrmContentId(
    _In_ ACXSTREAM      Stream,
    _In_ ULONG          DrmContentId,
    _In_ PACXDRMRIGHTS  DrmRights
)
{
    PSDCAXU_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetSdcaXuStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->AssignDrmContentId(DrmContentId, DrmRights);
}


