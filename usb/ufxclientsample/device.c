/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    device.c - WDFDEVICE creation and callback handling for the sample 
    controller device.

Abstract:

   This file contains the device entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "initguid.h"
#include "device.h"
#include "ufxdevice.h"
#include "registers.h"
#include "ufxendpoint.h"
#include "interrupt.h"
#include "defaultqueue.h"
#include "usbfnioctl.h"
#include "device.tmh"

//
// Minimum amount of time to wait before exiting D0. The purpose of waiting
// is to ensure that the current state / port are stabilized. For example,
// on IDCP, we may be connected to a suspended PC or disconnected hub, and
// in these cases we will get a suspend interrupt after ~3ms.
//
#define IDLE_TIMEOUT (100)

EVT_WDF_DEVICE_PREPARE_HARDWARE OnEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE OnEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY OnEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT OnEvtDeviceD0Exit;

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
static
NTSTATUS
DeviceInitialize(
    _In_ WDFDEVICE Device
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UfxClientDeviceCreate)
#pragma alloc_text (PAGE, OnEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, OnEvtDeviceReleaseHardware)
#endif

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
UfxClientDeviceCreate(
    _In_ WDFDRIVER Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    Driver - WDF driver object

    DeviceInit - Pointer to an opaque init structure. Memory for this
                 structure will be freed by the framework when the WdfDeviceCreate
                 succeeds. So don't access the structure after that point.

Return Value:

    Appropriate NTSTATUS value

--*/
{
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDFDEVICE WdfDevice;
    NTSTATUS Status;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpCallbacks;
    WDF_DMA_ENABLER_CONFIG DmaConfig;
    PCONTROLLER_CONTEXT ControllerContext;

    PAGED_CODE();

    TraceEntry();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, CONTROLLER_CONTEXT);

    //
    // Do UFX-specific initialization
    //
    Status = UfxFdoInit(Driver, DeviceInit, &DeviceAttributes);
    CHK_NT_MSG(Status, "Failed UFX initialization");

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpCallbacks);
    PnpCallbacks.EvtDevicePrepareHardware = OnEvtDevicePrepareHardware;
    PnpCallbacks.EvtDeviceReleaseHardware = OnEvtDeviceReleaseHardware;
    PnpCallbacks.EvtDeviceD0Entry = OnEvtDeviceD0Entry;
    PnpCallbacks.EvtDeviceD0Exit = OnEvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpCallbacks);

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &WdfDevice);
    CHK_NT_MSG(Status, "Failed to create wdf device");

    ControllerContext = DeviceGetControllerContext(WdfDevice);

    KeInitializeEvent(&ControllerContext->DetachEvent,
                      NotificationEvent,
                      FALSE);

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&ControllerContext->IdleSettings, IdleCanWakeFromS0);
    ControllerContext->IdleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;
    ControllerContext->IdleSettings.IdleTimeout = IDLE_TIMEOUT;
    ControllerContext->IdleSettings.DxState = PowerDeviceD3;

    Status = WdfDeviceAssignS0IdleSettings(WdfDevice, &ControllerContext->IdleSettings);
    LOG_NT_MSG(Status, "Failed to set S0 Idle Settings");

    //
    // Create and initialize device's default queue
    //
    Status = DefaultQueueCreate(WdfDevice);
    CHK_NT_MSG(Status, "Failed to intialize default queue");

    //
    // Set alignment required by controller
    //
    WdfDeviceSetAlignmentRequirement(WdfDevice, UFX_CLIENT_ALIGNMENT);

    //
    // Create and Initialize DMA Enabler object for the device.
    //
    WDF_DMA_ENABLER_CONFIG_INIT(
                        &DmaConfig,
                        WdfDmaProfileScatterGatherDuplex,
                        MAX_DMA_LENGTH);
    //
    // Version 3 is required to perform multiple
    // simultaneous transfers.
    //
    DmaConfig.WdmDmaVersionOverride = 3;

    Status = WdfDmaEnablerCreate(
                            WdfDevice,
                            &DmaConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &ControllerContext->DmaEnabler);
    CHK_NT_MSG(Status, "Failed to create DMA enabler object");

    //
    // Create UFXDEVICE object
    //
    Status = UfxDevice_DeviceCreate(WdfDevice);
    CHK_NT_MSG(Status, "Failed to create UFX Device object");

    //
    // Create DPC Lock
    //
    Status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &ControllerContext->DpcLock);
    CHK_NT_MSG(Status, "Failed to create DPC lock");

    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &ControllerContext->InitializeDefaultEndpointLock);
    CHK_NT_MSG(Status, "Failed to create Ep0 init lock");

End:
    TraceExit();
    return Status;
}



NTSTATUS
OnEvtDevicePrepareHardware (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
/*++

Routine Description:

    EvtDevicePrepareHardware callback handler for the controller FDO.
    It expects Register memory and two interrupts - Device and Wake.

Arguments:

    Device - WDFDEVICE object representing the controller.

    ResourcesRaw - Raw resources relative to the parent bus driver.

    ResourcesTranslated - Translated resources available for the controller.

Return Value:

    Appropriate NTSTATUS value

--*/
{
    NTSTATUS Status;
    ULONG ResCount;
    ULONG ResIndex;
    PCONTROLLER_CONTEXT ControllerContext;
    BOOLEAN MemoryResourceMapped;

    PAGED_CODE();

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(Device);
    MemoryResourceMapped = FALSE;

    ResCount = WdfCmResourceListGetCount(ResourcesRaw);

    Status = STATUS_SUCCESS;

    for (ResIndex = 0; ResIndex < ResCount; ResIndex++) {

        PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptorRaw;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptorTranslated;

        ResourceDescriptorRaw = WdfCmResourceListGetDescriptor(
                                                        ResourcesRaw,
                                                        ResIndex);
        switch (ResourceDescriptorRaw->Type) {

        case CmResourceTypeMemory:
            if (MemoryResourceMapped == FALSE) {
                MemoryResourceMapped = TRUE;
                Status = RegistersCreate(Device, ResourceDescriptorRaw);
                CHK_NT_MSG(Status, "Failed to read the HW registers");
            }
            break;

        case CmResourceTypeInterrupt:
            ResourceDescriptorTranslated = WdfCmResourceListGetDescriptor(
                                                            ResourcesTranslated,
                                                            ResIndex);
            Status = InterruptCreate(
                                Device,
                                ResourceDescriptorRaw,
                                ResourceDescriptorTranslated);
            CHK_NT_MSG(Status, "Failed to create WDFINTERRUPT object");
            break;

        default:
            break;
        }
    }

    NT_ASSERT(ControllerContext->DeviceInterrupt != NULL);

End:
    TraceExit();
    return STATUS_SUCCESS;
}


NTSTATUS
OnEvtDeviceReleaseHardware (
  _In_ WDFDEVICE Device,
  _In_ WDFCMRESLIST ResourcesTranslated
    )
/*++

Routine Description:

    EvtDeviceReleaseHardware callback handler for the controller FDO.
    It expects Register memory and two interrupts - Device and Wake.

Arguments:

    Device - WDFDEVICE object representing the controller.

    ResourcesTranslated - Translated resources available for the controller.

Return Value:

    Appropriate NTSTATUS value

--*/
{
    PREGISTERS_CONTEXT RegistersContext;
    PCONTROLLER_CONTEXT ControllerContext;

    TraceEntry();

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    ControllerContext = DeviceGetControllerContext(Device);
    ControllerContext->DeviceInterrupt = NULL;
    ControllerContext->AttachDetachInterrupt = NULL;
   
    RegistersContext = DeviceGetRegistersContext(Device);
    if ((RegistersContext == NULL) || (RegistersContext->RegisterBase == NULL)) {
        goto End;
    }

    MmUnmapIoSpace(RegistersContext->RegisterBase, RegistersContext->RegistersLength);
    RtlZeroMemory(RegistersContext, sizeof(*RegistersContext));

End:
    TraceExit();
    return STATUS_SUCCESS;
}

NTSTATUS
OnEvtDeviceD0Entry (
  _In_ WDFDEVICE Device,
  _In_ WDF_POWER_DEVICE_STATE PreviousState
)
/*++

Routine Description:

    Called by the framework after entering D0 state.

Arguments:

    Device - WDFDEVICE framework handle to the bus FDO.

    PreviousState - The WDF_POWER_DEVICE_STATE from which the stack is
        making this transition.

Return Value:

    Returns STATUS_SUCCESS or an appropriate NTSTATUS code otherwise.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(Device);

    if (PreviousState > WdfPowerDeviceD1) { 
        DevicePerformSoftReset(Device);

        WdfWaitLockAcquire(ControllerContext->InitializeDefaultEndpointLock, NULL);
        ControllerContext->InitializeDefaultEndpoint = TRUE;
        WdfWaitLockRelease(ControllerContext->InitializeDefaultEndpointLock);
    }

    if (PreviousState == WdfPowerDeviceD3Final) {
        //
        // Notify UFX that HW is now ready
        //
        UfxDeviceNotifyHardwareReady(ControllerContext->UfxDevice);
    }

    TraceExit();
    return STATUS_SUCCESS;
}

NTSTATUS
OnEvtDeviceD0Exit (
  _In_ WDFDEVICE Device,
  _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    Called by the framework when leaving our operational state (D0).

Arguments:

    Device - WDFDEVICE framework handle to the bus FDO.

    TargetState - The WDF_POWER_DEVICE_STATE to which the stack is
        making its transition.


Return Value:

    Returns STATUS_SUCCESS.
    Any failed status code will leave the device stack in failed state and
    device will not get any D0Entry callbacks after that.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(Device);

    WdfWaitLockAcquire(ControllerContext->InitializeDefaultEndpointLock, NULL);
    ControllerContext->InitializeDefaultEndpoint = FALSE;
    WdfWaitLockRelease(ControllerContext->InitializeDefaultEndpointLock);

    if (TargetState != WdfPowerDeviceD3Final) {
        goto End;
    }

    //
    // If the stack is being removed while we are attached to a host, simulate a detach. At this
    // point all attach/detach notification mechanisms are disabled, so no other code will report a
    // detach, nor will "WasAttached" change from underneath us.
    //
    if (ControllerContext->WasAttached) {
        KeClearEvent(&ControllerContext->DetachEvent);
        UfxDeviceNotifyDetach(ControllerContext->UfxDevice);
        ControllerContext->WasAttached = FALSE;
        KeWaitForSingleObject(&ControllerContext->DetachEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

End:
    TraceExit();
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
DevicePerformSoftReset (
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This functions performs soft reset of the controller and completes
    any initialization that needs to be done afterwards.

Arguments:

    Device - Wdf FDO device object representing the controller

--*/
{

    TraceEntry();

    UNREFERENCED_PARAMETER(Device);

    //
    // #### TODO: Insert code that performs a controller soft reset ####
    //

    TraceExit();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceInitializeDefaultEndpoint (
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This function initializes the default control endpoint

Arguments:

    Device - Wdf FDO device object representing the controller

--*/
{
    UFXENDPOINT Endpoint;
    PUFXDEVICE_CONTEXT DeviceContext;
    PCONTROLLER_CONTEXT ControllerContext;
    NTSTATUS Status;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(Device);
    DeviceContext = UfxDeviceGetContext(ControllerContext->UfxDevice);

    if (DeviceContext->PhysicalEndpointToUfxEndpoint[0] != NULL) {
        Endpoint = DeviceContext->PhysicalEndpointToUfxEndpoint[0];

        //
        // Re-initialize endpoint 0
        //
        TransferDestroy(Endpoint);
        
        Status = TransferInitialize(Endpoint);
        LOG_NT_MSG(Status, "Failed to initialize default endpoint");

        UfxEndpointConfigureHardware(Endpoint, FALSE);
        TransferStart(Endpoint);

    } else {
        NT_ASSERT(FALSE);
    }

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
DeviceHardwareFailure (
    _In_ WDFDEVICE Device,
    _In_ ULONG ExceptionCode
    )
/*++

Routine Description:

    The client driver may call this function if the controller has encountered a non-recoverable error, to 
    allow UFX to attempt to reset the controller if possible.

Arguments:

    Device - Wdf FDO device object representing the controller

    ExceptionCode - Exception code indicating the specific failure encountered by the controller.  This
                    value is specific to the type of controller.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;
    PUFXDEVICE_CONTEXT DeviceContext;
    PHARDWARE_FAILURE_CONTEXT HardwareFailureContext;
    TraceEntry();

    NT_ASSERT(ExceptionCode < ExceptionMaximum);

    ControllerContext = DeviceGetControllerContext(Device);
    DeviceContext = UfxDeviceGetContext(ControllerContext->UfxDevice);

#pragma prefast(suppress:6014, "Memory allocation is expected")
    HardwareFailureContext = ExAllocatePoolWithTag(
                                 NonPagedPool,
                                 sizeof(HARDWARE_FAILURE_CONTEXT),
                                 UFX_CLIENT_TAG);

    if (HardwareFailureContext) {
        HardwareFailureContext->Size = sizeof(*HardwareFailureContext);
        HardwareFailureContext->ExceptionCode = ExceptionCode;

        RtlCopyMemory(
            &HardwareFailureContext->ControllerContext,
            ControllerContext,
            sizeof(CONTROLLER_CONTEXT));

        RtlCopyMemory(
            &HardwareFailureContext->DeviceContext,
            DeviceContext,
            sizeof(UFXDEVICE_CONTEXT));

    } else {
        TraceError("Failed to allocate hardware failure context!");
    }

    UfxDeviceNotifyHardwareFailure(
        ControllerContext->UfxDevice,
        (PUFX_HARDWARE_FAILURE_CONTEXT) HardwareFailureContext);

    TraceExit();
}
