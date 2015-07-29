/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    events.c

Abstract:

    Event handler 

Environment:

    Kernel mode

--*/

#include "device.h"
#include "ufxdevice.h"
#include "ufxendpoint.h"
#include "transfer.h"
#include <usbspec.h>
#include "event.h"
#include "event.tmh"


_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleUsbReset (
    WDFDEVICE WdfDevice
    )
/*++

Routine Description:

    Handles a reset event notification from the controller.

Arguments:

    WdfDevice - WDFDEVICE object representing the controller.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);

    UfxDevice_Reset(ControllerContext->UfxDevice);
            
    ControllerContext->RemoteWakeupRequested = FALSE;

    //
    // Update the controller to set device address to 0
    //

    //
    // #### TODO: Add code to set the USB device address to 0 on the controller ####
    //

    TraceExit();
}

_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleUsbConnect (
    WDFDEVICE WdfDevice
    )
/*++

Routine Description:

    Handles a connect event from the controller.

Arguments:

    WDfDevice - WDFDEVICE object representing the controller.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;
    USB_DEVICE_SPEED DeviceSpeed;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);

    //
    // Read the device speed.
    //

    //
    // #### TODO: Add code to read device speed from the controller ####
    //
    
    // Sample will assume SuperSpeed operation for illustration purposes
    DeviceSpeed = UsbSuperSpeed;
    
    //
    // #### TODO: Add any code needed to configure the controller after connect has occurred ####
    //


    ControllerContext->Speed = DeviceSpeed;
    TraceInformation("Connected Speed is %d!", DeviceSpeed);

    //
    // Notify UFX about reset, which will take care of updating 
    // Max Packet Size for EP0 by calling descriptor update.
    //
    UfxDeviceNotifyReset(ControllerContext->UfxDevice, DeviceSpeed);

    ControllerContext->Connect = TRUE;

    TraceExit();
}


_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleUsbDisconnect (
    WDFDEVICE WdfDevice
    )
/*++

Routine Description:

    Handles a disconnect event from the controller.

Arguments:

    WdfDevice - WDFDEVICE object representing the controller.

--*/
{
    TraceEntry();

    UNREFERENCED_PARAMETER(WdfDevice);

    //
    // #### TODO: Add any code needed to configure controller after disconnect event ####
    //

    TraceExit();
}


_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleUSBLinkStateChange (
    WDFDEVICE WdfDevice
    )
/*++

Routine Description:

    Handles a link change event from the controller.

Arguments:

    WdfDevice - WDFDEVICE object representing the controller.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;
    ULONG UsbLinkEvent;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);
    
    //
    // #### TODO: Add code to read link state from controller ####
    // 
    // For sample purposes use U0.
    UsbLinkEvent = USB_LINK_STATE_U0;

    if (UsbLinkEvent == USB_LINK_STATE_U0) { 

        if(ControllerContext->Suspended) {
            ControllerContext->Suspended = FALSE;
            UfxDeviceNotifyResume(ControllerContext->UfxDevice);    
        }

        ControllerContext->RemoteWakeupRequested = FALSE;

    } else {
        TraceVerbose("Ignoring link state change event: 0X%X", UsbLinkEvent);
    }

    TraceExit();
}


_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleDeviceEvent (
    WDFDEVICE WdfDevice,
    DEVICE_EVENT DeviceEvent
    )
/*++

Routine Description:

    Function to dispatch device events from the controller.

Arguments:

    WdfDevice - Wdf device object corresponding to the FDO

    DeviceEvent -  Device specific event.

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);

    switch (DeviceEvent) {

    case DeviceEventDisconnect:
        ControllerContext->Suspended = FALSE;
        HandleUsbDisconnect(WdfDevice);
        break;

    case DeviceEventUSBReset:
        ControllerContext->Suspended = FALSE;
        HandleUsbReset(WdfDevice);
        break;

    case DeviceEventConnect:
        HandleUsbConnect(WdfDevice);
        break;

    case DeviceEventUSBLinkStateChange:
        HandleUSBLinkStateChange(WdfDevice);
        break;

    case DeviceEventWakeUp:
        if (ControllerContext->Suspended) {
            ControllerContext->Suspended = FALSE;
            UfxDeviceNotifyResume(ControllerContext->UfxDevice);
        }
        break;

    case DeviceEventSuspend:
        if (!ControllerContext->Suspended) {
            ControllerContext->Suspended = TRUE;
            UfxDeviceNotifySuspend(ControllerContext->UfxDevice);
        }
        break;

    default:
        TraceError("Unknown device event raised by controller");
        NT_ASSERT(FALSE);
        break;
    }    

    TraceExit();
}




_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleEndpointEvent (
    WDFDEVICE WdfDevice,
    ENDPOINT_EVENT EndpointEvent
    )
/*++

Routine Description:

    Function to dispatch endpoint events.

Arguments:

    WdfDevice - Wdf device object corresponding to the FDO

    EndpointEvent -  Endpoint specific event.

--*/
{
    UFXENDPOINT Endpoint;
    PCONTROLLER_CONTEXT ControllerContext;
    PUFXDEVICE_CONTEXT DeviceContext;

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfDevice);
    DeviceContext = UfxDeviceGetContext(ControllerContext->UfxDevice);

    //
    // #### TODO: Insert code to extract endpoint event and endpoint number ####
    // 
    
    // Sample will assume a transfer complete event on endpoint 1 for illustration purposes
    EndpointEvent = EndpointEventTransferComplete;
    
    Endpoint = DeviceContext->PhysicalEndpointToUfxEndpoint[1];

    switch (EndpointEvent) {

    case EndpointEventTransferComplete:
        TraceInformation("ENDPOINT EVENT: TransferComplete");
        TransferComplete(Endpoint);
        break;

    case EndpointEventStartTransferComplete:
        TraceInformation("ENDPOINT EVENT: Complete Start Transfer");
        TransferCommandStartComplete(Endpoint);
        break;

    case EndpointEventEndTransferComplete:
        TraceInformation("ENDPOINT EVENT: Complete End Transfer");
        TransferCommandEndComplete(Endpoint);
        break;
        
    case EndpointEventSetStall:
        TraceInformation("ENDPOINT EVENT: Command Complete Stall Set");
        TransferStallSetComplete(Endpoint);
        break;
           
    case EndpointEventClearStall:
        TraceInformation("ENDPOINT EVENT: Command Complete Stall Clear");
        TransferStallClearComplete(Endpoint);
        break;

    default:
        TraceError("Unexpected endpoint event!");
        NT_ASSERT(FALSE);
    }

    TraceExit();
}