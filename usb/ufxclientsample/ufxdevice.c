/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ufxdevice.c

Abstract:

    Implements the UFXDEVICE functionality and callbacks.

Environment:

    Kernel mode

--*/


#include "device.h"
#include "ufxdevice.h"
#include "ufxendpoint.h"
#include "transfer.h"
#include "interrupt.h"
#include <usbspec.h>

#include "ufxdevice.tmh"

#define DISCONNECT_TIMEOUT WDF_REL_TIMEOUT_IN_MS(2000)

//
// UFXDEVICE callbacks
//
EVT_UFX_DEVICE_HOST_CONNECT UfxDevice_EvtDeviceHostConnect;
EVT_UFX_DEVICE_HOST_DISCONNECT UfxDevice_EvtDeviceHostDisconnect;
EVT_UFX_DEVICE_ADDRESSED UfxDevice_EvtDeviceAddressed;
EVT_UFX_DEVICE_ENDPOINT_ADD UfxDevice_EvtDeviceEndpointAdd;
EVT_UFX_DEVICE_DEFAULT_ENDPOINT_ADD UfxDevice_EvtDeviceDefaultEndpointAdd;
EVT_UFX_DEVICE_USB_STATE_CHANGE UfxDevice_EvtDeviceUsbStateChange;
EVT_UFX_DEVICE_PORT_CHANGE UfxDevice_EvtDevicePortChange;
EVT_UFX_DEVICE_PORT_DETECT UfxDevice_EvtDevicePortDetect;
EVT_UFX_DEVICE_REMOTE_WAKEUP_SIGNAL UfxDevice_EvtDeviceRemoteWakeupSignal;
EVT_UFX_DEVICE_TEST_MODE_SET UfxDevice_EvtDeviceTestModeSet;
EVT_UFX_DEVICE_SUPER_SPEED_POWER_FEATURE UfxDevice_EvtDeviceSuperSpeedPowerFeature;

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxDeviceStopOrResumeIdle (
    _In_ UFXDEVICE Device,
    _In_ USBFN_DEVICE_STATE UsbState,
    _In_ USBFN_PORT_TYPE UsbPort
    );

//
// WDFOBJECT callbacks
//
EVT_WDF_OBJECT_CONTEXT_CLEANUP UfxDevice_EvtCleanupCallback;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UfxDevice_DeviceCreate)
#pragma alloc_text (PAGE, UfxDevice_EvtDeviceUsbStateChange)
#pragma alloc_text (PAGE, UfxDevice_EvtDevicePortChange)
#pragma alloc_text (PAGE, UfxDeviceStopOrResumeIdle)
#pragma alloc_text (PAGE, UfxDevice_EvtDeviceDefaultEndpointAdd)
#pragma alloc_text (PAGE, UfxDevice_EvtDeviceRemoteWakeupSignal)
#endif

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxDevice_DeviceCreate (
    _In_ WDFDEVICE WdfDevice
    )
/*++

Routine Description:

    Routine that registers with UFX and creates the UFX device object.

Arguments:

    WdfDevice - Wdf object representing the device.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status;
    PCONTROLLER_CONTEXT ControllerContext;
    UFX_DEVICE_CALLBACKS UfxDeviceCallbacks;
    UFX_DEVICE_CAPABILITIES UfxDeviceCapabilities;
    PUFXDEVICE_CONTEXT UfxDeviceContext;
    UFXDEVICE UfxDevice;
    WDF_OBJECT_ATTRIBUTES Attributes;

    PAGED_CODE();

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);

    UFX_DEVICE_CAPABILITIES_INIT(&UfxDeviceCapabilities);
    UfxDeviceCapabilities.MaxSpeed = UsbSuperSpeed;
    UfxDeviceCapabilities.RemoteWakeSignalDelay = REMOTE_WAKEUP_TIMEOUT_INTERVAL_MS;
    
    //
    // Set bitmasks that define the IN and OUT endpoint numbers that are available on the controller
    //
    
    //
    // #### TODO: Set the IN endpoint mask here if not all endpoint addresses are supported ####
    //
    // For illustration purposes sample will set default control endpoint 0, 1-4, 8
    UfxDeviceCapabilities.InEndpointBitmap = 0x011F;
    
    //
    // #### TODO: Set the OUT endpoint mask here if not all endpoint addresses are supported ####
    //
    // For illustration purposes sample will set default control endpoint 0, 2-7
    //
    UfxDeviceCapabilities.OutEndpointBitmap = 0x00FD;

    //
    // Set the event callbacks for the ufxdevice
    //
    UFX_DEVICE_CALLBACKS_INIT(&UfxDeviceCallbacks);
    UfxDeviceCallbacks.EvtDeviceHostConnect = UfxDevice_EvtDeviceHostConnect;
    UfxDeviceCallbacks.EvtDeviceHostDisconnect = UfxDevice_EvtDeviceHostDisconnect;
    UfxDeviceCallbacks.EvtDeviceAddressed = UfxDevice_EvtDeviceAddressed;
    UfxDeviceCallbacks.EvtDeviceEndpointAdd = UfxDevice_EvtDeviceEndpointAdd;
    UfxDeviceCallbacks.EvtDeviceDefaultEndpointAdd = UfxDevice_EvtDeviceDefaultEndpointAdd;
    UfxDeviceCallbacks.EvtDeviceUsbStateChange = UfxDevice_EvtDeviceUsbStateChange;
    UfxDeviceCallbacks.EvtDevicePortChange = UfxDevice_EvtDevicePortChange;
    UfxDeviceCallbacks.EvtDevicePortDetect = UfxDevice_EvtDevicePortDetect;
    UfxDeviceCallbacks.EvtDeviceRemoteWakeupSignal = UfxDevice_EvtDeviceRemoteWakeupSignal;
    UfxDeviceCallbacks.EvtDeviceTestModeSet = UfxDevice_EvtDeviceTestModeSet;
    UfxDeviceCallbacks.EvtDeviceSuperSpeedPowerFeature = UfxDevice_EvtDeviceSuperSpeedPowerFeature;

    // Context associated with UFXDEVICE object
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, UFXDEVICE_CONTEXT);
    Attributes.EvtCleanupCallback = UfxDevice_EvtCleanupCallback;

    // Create the UFXDEVICE object
    Status = UfxDeviceCreate(WdfDevice,
                             &UfxDeviceCallbacks,
                             &UfxDeviceCapabilities,
                             &Attributes,
                             &UfxDevice);
    CHK_NT_MSG(Status, "Failed to create UFXDEVICE object");

    // Initialize the client context space in UFXDEVICE object
    UfxDeviceContext = UfxDeviceGetContext(UfxDevice);
    UfxDeviceContext->FdoWdfDevice = WdfDevice;
    UfxDeviceContext->UsbState = UsbfnDeviceStateDetached;
    UfxDeviceContext->UsbPort = UsbfnUnknownPort;
    UfxDeviceContext->IsIdle = TRUE;

    ControllerContext->UfxDevice = UfxDevice;

    //
    // Create endpoints collection
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = UfxDevice;
    Status = WdfCollectionCreate(&Attributes, &UfxDeviceContext->Endpoints);
    CHK_NT_MSG(Status, "Failed to create endpoint collection object");

End:
    TraceExit();
    return Status;
}


VOID
UfxDevice_EvtCleanupCallback (
    _In_ WDFOBJECT   UfxDevice
    )
/*++

Routine Description:

    EvtCleanupCallback handler for the UFXDEVICE object.

Arguments:

    UfxDevice - Wdf object representing the UFXDEVICE

--*/
{
    PUFXDEVICE_CONTEXT DeviceContext = NULL;
    PCONTROLLER_CONTEXT ControllerContext = NULL;

    TraceEntry();

    DeviceContext = UfxDeviceGetContext(UfxDevice);
    ControllerContext = DeviceGetControllerContext(DeviceContext->FdoWdfDevice);

    TraceExit();
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxDeviceSetRunStop (
    _In_ UFXDEVICE UfxDevice,
    _In_ BOOLEAN Set
    )
/*++

Routine Description:

    Function that sets or clears the run/stop state on the controller.  Setting the
    run state will initiate a connect to the host.

Arguments:

    UfxDevice - Wdf object representing the UFXDEVICE

    Set - TRUE if setting the run state, FALSE if clearing the run state.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;
    PUFXDEVICE_CONTEXT DeviceContext;
    BOOLEAN EventComplete;


    TraceEntry();

    DeviceContext = UfxDeviceGetContext(UfxDevice);
    ControllerContext = DeviceGetControllerContext(DeviceContext->FdoWdfDevice);

    EventComplete = TRUE;

    WdfSpinLockAcquire(ControllerContext->DpcLock);

    if (Set) {
        //
        // #### TODO: Insert code to set the run state on the controller ####
        //
    } else {
        //
        // #### TODO: Insert code to clear the run state on the controller ####
        //
    }

    WdfSpinLockRelease(ControllerContext->DpcLock);

    if (EventComplete) {
        UfxDeviceEventComplete(UfxDevice, STATUS_SUCCESS);
    }

    TraceExit();
}

VOID
UfxDevice_EvtDeviceHostConnect (
    _In_ UFXDEVICE UfxDevice
    )
/*++

Routine Description:

    EvtDeviceHostConnect callback handler for UFXDEVICE object.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

--*/
{
    TraceEntry();

    UfxDeviceSetRunStop(UfxDevice, TRUE);

    TraceExit();
}


VOID
UfxDevice_EvtDeviceHostDisconnect (
    _In_ UFXDEVICE UfxDevice
    )
/*++

Routine Description:

    EvtDeviceHostDisconnect callback handler for UFXDEVICE object.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

--*/
{
    TraceEntry();

    UfxDevice_Reset(UfxDevice);

    UfxDeviceSetRunStop(UfxDevice, FALSE);

    TraceExit();
}


VOID
UfxDevice_EvtDeviceAddressed (
    _In_ UFXDEVICE UfxDevice,
    _In_ USHORT DeviceAddress
    )
/*++

Routine Description:

    EvtDeviceAddressed handler for the UFXDEVICE object.
    Sets the Address indicated by 'DeviceAddress' on the controller.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    DeviceAddress - USB Device Address, as determined by the UFX.

--*/
{
    UNREFERENCED_PARAMETER(DeviceAddress);

    TraceEntry();

    //
    // Set the device address on the controller
    //

    //
    // #### Insert code to set the device address on controller ####
    //
    
    UfxDeviceEventComplete(UfxDevice, STATUS_SUCCESS);

    TraceExit();
}


NTSTATUS
UfxDevice_EvtDeviceEndpointAdd (
    _In_ UFXDEVICE UfxDevice,
    _In_ const PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    _Inout_ PUFXENDPOINT_INIT EndpointInit
    )
/*++

Routine Description:

    EvtDeviceEndpointAdd handler for the UFXDEVICE object.
    Creates UFXENDPOINT object corresponding to the newly reported endpoint.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    EndpointDescriptor - Cosntant Pointer to Endpoint descriptor for the
        newly reported endpoint.

    EndpointInit - Pointer to the Opaque UFXENDPOINT_INIT object

Return Value:

    STATUS_SUCCESS on success, or an appropirate NTSTATUS message on failure.

--*/
{
    NTSTATUS Status;

    TraceEntry();

    Status = UfxEndpointAdd(UfxDevice, EndpointDescriptor, EndpointInit);
    CHK_NT_MSG(Status, "Failed to create endpoint");

End:

    TraceExit();
    return Status;
}


VOID
UfxDevice_EvtDeviceDefaultEndpointAdd (
    _In_ UFXDEVICE UfxDevice,
    _In_ USHORT MaxPacketSize,
    _Inout_ PUFXENDPOINT_INIT EndpointInit
    )
/*++

Routine Description:

    EvtDeviceDefaultEndpointAdd handler for the UFXDEVICE object.
    Creates UFXENDPOINT object corresponding to the default endpoint of the
    device.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    MaxPacketSize - Max packet size of the device's default endpoint.

    EndpointInit - Pointer to the Opaque UFXENDPOINT_INIT object

--*/
{
    NTSTATUS Status;
    USB_ENDPOINT_DESCRIPTOR Descriptor;

    PAGED_CODE();

    TraceEntry();

    Descriptor.bDescriptorType = USB_ENDPOINT_DESCRIPTOR_TYPE;
    Descriptor.bEndpointAddress = 0;
    Descriptor.bInterval = 0;
    Descriptor.bLength = sizeof(USB_ENDPOINT_DESCRIPTOR);
    Descriptor.bmAttributes = USB_ENDPOINT_TYPE_CONTROL;
    Descriptor.wMaxPacketSize = MaxPacketSize;

    Status = UfxEndpointAdd(UfxDevice, &Descriptor, EndpointInit);
    CHK_NT_MSG(Status, "Failed to create default endpoint!");

End:
    UfxDeviceEventComplete(UfxDevice, Status);
    TraceExit();
}


VOID
UfxDevice_EvtDeviceUsbStateChange (
    _In_ UFXDEVICE UfxDevice,
    _In_ USBFN_DEVICE_STATE NewState
)
/*++

Routine Description:

    EvtDeviceUsbStateChange handler for the UFXDEVICE object.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    NewState - The new device state.

--*/
{
    NTSTATUS Status;
    PUFXDEVICE_CONTEXT Context;
    PCONTROLLER_CONTEXT ControllerContext;
    ULONG EpIndex;
    USBFN_DEVICE_STATE OldState;
 
    PAGED_CODE();

    TraceEntry();

    Context = UfxDeviceGetContext(UfxDevice);
    ControllerContext = DeviceGetControllerContext(Context->FdoWdfDevice);
    OldState = Context->UsbState;

    TraceInformation("New STATE: %d", NewState);

    Status = UfxDeviceStopOrResumeIdle(UfxDevice, NewState, Context->UsbPort);
    LOG_NT_MSG(Status, "Failed to stop or resume idle");

    WdfWaitLockAcquire(ControllerContext->InitializeDefaultEndpointLock, NULL);
    if (ControllerContext->InitializeDefaultEndpoint == TRUE) {
        //
        // Reset endpoint 0. This is the last part of soft reset, which was postponed
        // until now, since we need to make sure EP0 is created by UFX.
        //
        DeviceInitializeDefaultEndpoint(Context->FdoWdfDevice);
        ControllerContext->InitializeDefaultEndpoint = FALSE;
    }
    WdfWaitLockRelease(ControllerContext->InitializeDefaultEndpointLock);

    if (NewState == UsbfnDeviceStateConfigured && OldState != UsbfnDeviceStateSuspended) {

        for (EpIndex = 1; EpIndex < WdfCollectionGetCount(Context->Endpoints); EpIndex++) {
            UfxEndpointConfigure(WdfCollectionGetItem(Context->Endpoints, EpIndex));
        }

        // 
        // #### TODO: Insert code to allow the controller to accept U1/U2, if supported ####
        //
       
    }


    if (NewState == UsbfnDeviceStateDetached) {
        KeSetEvent(&ControllerContext->DetachEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }

    UfxDeviceEventComplete(UfxDevice, STATUS_SUCCESS);
    TraceExit();
}

_Function_class_(EVT_UFX_DEVICE_PORT_CHANGE)
_IRQL_requires_(PASSIVE_LEVEL)
VOID
UfxDevice_EvtDevicePortChange (
    _In_
        UFXDEVICE UfxDevice,
    _In_
        USBFN_PORT_TYPE NewPort
    )
/*++

Routine Description:

    EvtDevicePortChange handler for the UFXDEVICE object.
    Caches the new port type, and stops or resumes idle as needed.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    NewPort - New port type

--*/
{
    NTSTATUS Status;
    PUFXDEVICE_CONTEXT Context;

    PAGED_CODE();

    TraceEntry();

    Context = UfxDeviceGetContext(UfxDevice);

    TraceInformation("New PORT: %d", NewPort);

    Status = UfxDeviceStopOrResumeIdle(UfxDevice, Context->UsbState, NewPort);
    LOG_NT_MSG(Status, "Failed to stop or resume idle");

    UfxDeviceEventComplete(UfxDevice, STATUS_SUCCESS);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxDevice_Reset (
    _In_ UFXDEVICE Device
    )
/*++

Routine Description:

    Cancels all transfers and disables non-zero endpoints in
    response to USB reset.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

--*/
{
    PUFXDEVICE_CONTEXT DeviceContext;
    PREGISTERS_CONTEXT RegistersContext;
    ULONG EpIndex;

    TraceEntry();

    DeviceContext = UfxDeviceGetContext(Device);
    RegistersContext = DeviceGetRegistersContext(DeviceContext->FdoWdfDevice);

    //
    // Get EP0 back to setup stage.
    //
    TransferReset(WdfCollectionGetFirstItem(DeviceContext->Endpoints));

    //
    // Disable all non-default endpoints
    //

    // 
    // #### TODO: Insert code to disable non-default endpoints ####
    //

    //
    // End any transfers on non-default endpoints.
    //
    for (EpIndex = 1; EpIndex < WdfCollectionGetCount(DeviceContext->Endpoints); EpIndex++) {
        TransferReset(WdfCollectionGetItem(DeviceContext->Endpoints, EpIndex));
    }

    TraceExit();
}


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxDeviceStopOrResumeIdle (
    _In_ UFXDEVICE Device,
    _In_ USBFN_DEVICE_STATE UsbState,
    _In_ USBFN_PORT_TYPE UsbPort
    )
/*++

Routine Description:

    Examines the device USB state and port type and determines
    if it needs to stop or resume idle.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

Return Value:

    STATUS_SUCCESS on success, or an appropirate NTSTATUS message on failure.

--*/
{
    PUFXDEVICE_CONTEXT DeviceContext;
    PCONTROLLER_CONTEXT ControllerContext;
    NTSTATUS Status;
    BOOLEAN NeedPower;
    DEVICE_POWER_STATE DxState = PowerDeviceD3;

    PAGED_CODE();

    TraceEntry();

    DeviceContext = UfxDeviceGetContext(Device);
    ControllerContext = DeviceGetControllerContext(DeviceContext->FdoWdfDevice);

    switch (UsbState) {

        case UsbfnDeviceStateAttached:
            __fallthrough;
        case UsbfnDeviceStateDefault:

            switch (UsbPort) {

                case UsbfnStandardDownstreamPort:
                    __fallthrough;
                case UsbfnChargingDownstreamPort:
                    __fallthrough;
                case UsbfnUnknownPort:
                    NeedPower = TRUE;
                    break;

                case UsbfnDedicatedChargingPort:
                case UsbfnProprietaryDedicatedChargingPort:
                    NeedPower = FALSE;
                    DxState = PowerDeviceD2;
                    break;

                default:
                    NeedPower = FALSE;
            }
            break;

        case UsbfnDeviceStateSuspended:
            NeedPower = FALSE;
            DxState = PowerDeviceD2;
            break;

        case UsbfnDeviceStateAddressed:
            __fallthrough;

        case UsbfnDeviceStateConfigured:
            NeedPower = TRUE;
            break;

        default:
            NeedPower = FALSE;
    }

    //
    // Determine if our lowest idle state has changed, and set it if so
    //
    if (DxState != ControllerContext->IdleSettings.DxState)
    {
        ControllerContext->IdleSettings.DxState = DxState;
        Status = WdfDeviceAssignS0IdleSettings(
            DeviceContext->FdoWdfDevice,
            &ControllerContext->IdleSettings);
        CHK_NT_MSG(Status, "Failed to update device idle settings");
    }

    if (NeedPower && DeviceContext->IsIdle) {
        //
        // We don't want to update the USB state /port until we stop idle to
        // prevent D0 -> DX path from reading the wrong state.
        //
        TraceInformation("Stopping idle");
        Status = WdfDeviceStopIdle(DeviceContext->FdoWdfDevice, TRUE);
        CHK_NT_MSG(Status, "Failed to stop idle");
        DeviceContext->UsbState = UsbState;
        DeviceContext->UsbPort = UsbPort;
        DeviceContext->IsIdle = FALSE;

    } else if (!NeedPower && !DeviceContext->IsIdle) {
        //
        // We need to update USB state / port before resume idle to ensure
        // D0 -> DX path reads the correct state.
        //
        DeviceContext->UsbState = UsbState;
        DeviceContext->UsbPort = UsbPort;
        DeviceContext->IsIdle = TRUE;
        TraceInformation("Resuming idle");
        WdfDeviceResumeIdle(DeviceContext->FdoWdfDevice);

    } else {
        TraceInformation("No idle action");
        DeviceContext->UsbState = UsbState;
        DeviceContext->UsbPort = UsbPort;
    }

    Status = STATUS_SUCCESS;

End:
    TraceExit();
    return Status;
}

VOID
UfxDevice_EvtDevicePortDetect (
    _In_ UFXDEVICE UfxDevice
    )
/*++
Routine Description:

    Starts the port detection state machine

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

--*/
{
    PUFXDEVICE_CONTEXT DeviceContext;
    PCONTROLLER_CONTEXT ControllerContext;

    DeviceContext = UfxDeviceGetContext(UfxDevice);
    ControllerContext = DeviceGetControllerContext(DeviceContext->FdoWdfDevice);

    //
    // #### TODO: Insert code to determine port/charger type ####
    // 
    // In this example we will return an unknown port type.  This will allow UFX to connect to a host if
    // one is present.  UFX will timeout after 5 seconds if no host is present and transition to
    // an invalid charger type, which will allow the controller to exit D0.
    //
    UfxDevicePortDetectComplete(ControllerContext->UfxDevice, UsbfnUnknownPort);
}


VOID
UfxDevice_EvtDeviceRemoteWakeupSignal (
    _In_ UFXDEVICE UfxDevice
    )
/*++
Routine Description:

    Signals Remote Wakeup to the Host by issuing a link state change command.
    It acquires and releases the power reference to ensure a valid power state
    before accessing the device.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

--*/
{
    NTSTATUS Status;
    PUFXDEVICE_CONTEXT DeviceContext;

    PAGED_CODE();

    TraceEntry();

    DeviceContext = UfxDeviceGetContext(UfxDevice);

    //
    // Stop Idle to ensure the device is in working state
    //
    Status = WdfDeviceStopIdle(DeviceContext->FdoWdfDevice, TRUE);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to stop idle %!STATUS!", Status);
        goto End;
    }

    //
    // Issue a Link State Change Request.
    //

    //
    // #### TODO: Insert code to issue a link state change on the controller ####
    //

    WdfDeviceResumeIdle(DeviceContext->FdoWdfDevice);

End:
    UfxDeviceEventComplete(UfxDevice, Status);
    TraceExit();
}

VOID
UfxDevice_EvtDeviceTestModeSet (
    _In_ UFXDEVICE UfxDevice,
    _In_ ULONG TestMode
    )
/*++

Routine Description:

    EvtDeviceTestModeSet handler for the UFXDEVICE object.
    
    Handles a set test mode request from the host.  Places the controller into 
    the specified test mode.

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    TestMode - Test mode value.  See Section 7.1.20 of the USB 2.0 specification for definitions of 
               each test mode.

--*/
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(TestMode);

    TraceEntry();

    //
    // #### TODO: Insert code to put the controller into the specified test mode ####
    //

    Status = STATUS_SUCCESS;

    UfxDeviceEventComplete(UfxDevice, Status);
    TraceExit();
}

VOID
UfxDevice_EvtDeviceSuperSpeedPowerFeature (
    _In_ UFXDEVICE Device,
    _In_ USHORT Feature,
    _In_ BOOLEAN Set
    )
/*++

Routine Description:

    EvtDeviceSuperSpeedPowerFeature handler for the UFXDEVICE object.
    
    Handles a set or clear U1/U2 request from the host.  

Arguments:

    UfxDevice - UFXDEVICE object representing the device.

    Feature - Indicates the feature being set or cleared.  Either U1 or U2 enable.

    Set - Indicates if the feature should be set or cleared
    
--*/
{
    TraceEntry();

    if (Feature == USB_FEATURE_U1_ENABLE) {
        if (Set == TRUE) {
            //
            // #### TODO: Insert code to initiate U1  ####
            //
        } else {
            //
            // #### TODO: Insert code to exit U1 ####
            //
        }
    } else if (Feature == USB_FEATURE_U2_ENABLE) {
        if (Set == TRUE) {
            //
            // #### TODO: Insert code to initiate U2 ####
            //
        } else {
            //
            // #### TODO: Insert code to exit U2 ####
            //
        }
    } else {
        NT_ASSERT(FALSE);
    }

    UfxDeviceEventComplete(Device, STATUS_SUCCESS);
    TraceExit();
}

