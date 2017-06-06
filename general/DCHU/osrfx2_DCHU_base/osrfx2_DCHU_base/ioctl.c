/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Ioctl.c

Abstract:

    USB device driver for OSR USB-FX2 Learning Kit

Environment:

    User mode only

--*/

#include "osrusbfx2.h"

#if defined(EVENT_TRACING)
#include "ioctl.tmh"
#endif

#pragma alloc_text(PAGE, OsrFxEvtIoDeviceControl)
#pragma alloc_text(PAGE, ResetPipe)
#pragma alloc_text(PAGE, ResetDevice)
#pragma alloc_text(PAGE, ReenumerateDevice)
#pragma alloc_text(PAGE, GetBarGraphState)
#pragma alloc_text(PAGE, SetBarGraphState)
#pragma alloc_text(PAGE, GetSevenSegmentState)
#pragma alloc_text(PAGE, SetSevenSegmentState)
#pragma alloc_text(PAGE, GetSwitchState)


/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue              - Handle to the framework queue object that is
                         associated with the I/O request

    Request            - Handle to a framework request object

    OutputBufferLength - Length of the request's output buffer, if an output
                         buffer is available

    InputBufferLength  - Length of the request's input buffer, if an input
                         buffer is available

    IoControlCode      - The driver-defined or system-defined I/O control code
                         (IOCTL) that is associated with the request

Return Value:

    VOID

--*/
VOID
OsrFxEvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode    
    )
{
    WDFDEVICE Device;
    PDEVICE_CONTEXT DeviceContext;
    size_t BytesReturned = 0;
    PBAR_GRAPH_STATE BarGraphState = NULL;
    PSWITCH_STATE SwitchState = NULL;
    PUCHAR SevenSegment = NULL;
    BOOLEAN RequestPending = FALSE;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                "--> OsrFxEvtIoDeviceControl\n");

    //
    // Initialize variables.
    //
    Device = WdfIoQueueGetDevice(Queue);
    DeviceContext = GetDeviceContext(Device);

    switch(IoControlCode)
    {

    case IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR:
    {
        PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor = NULL;
        USHORT RequiredSize = 0;

        //
        // First get the size of the config descriptor.
        //
        Status = WdfUsbTargetDeviceRetrieveConfigDescriptor(DeviceContext->UsbDevice,
                                                            NULL,
                                                            &RequiredSize);

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x\n",
                        Status);
            break;
        }

        //
        // Get the buffer - make sure the buffer is big enough.
        //
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                (size_t)RequiredSize,
                                                &ConfigurationDescriptor,
                                                NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "WdfRequestRetrieveOutputBuffer failed 0x%x\n",
                        Status);
            break;
        }

        Status = WdfUsbTargetDeviceRetrieveConfigDescriptor(DeviceContext->UsbDevice,
                                                            ConfigurationDescriptor,
                                                            &RequiredSize);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x\n",
                        Status);
            break;
        }

        BytesReturned = RequiredSize;
    }

        break;

    case IOCTL_OSRUSBFX2_RESET_DEVICE:

        Status = ResetDevice(Device);
        break;

    case IOCTL_OSRUSBFX2_REENUMERATE_DEVICE:
        //
        // Otherwise, call our function to re-enumerate the device.
        //
        Status = ReenumerateDevice(DeviceContext);

        BytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY:
        //
        // Make sure the caller's output buffer is large enough
        // to hold the state of the bar graph.
        //
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(BAR_GRAPH_STATE),
                                                &BarGraphState,
                                                NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "User's output buffer is too small for this IOCTL, expecting a BAR_GRAPH_STATE\n");
            break;
        }

        //
        // Call our function to get the bar graph state.
        //
        Status = GetBarGraphState(DeviceContext, BarGraphState);

        //
        // Return the user their data.
        //
        if (NT_SUCCESS(Status))
        {
            BytesReturned = sizeof(BAR_GRAPH_STATE);
        }
        else
        {
            BytesReturned = 0;
        }

        break;

    case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:
        Status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(BAR_GRAPH_STATE),
                                               &BarGraphState,
                                               NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "User's input buffer is too small for this IOCTL, expecting a BAR_GRAPH_STATE\n");
            break;
        }

        //
        // Call our routine to set the bar graph state.
        //
        Status = SetBarGraphState(DeviceContext, BarGraphState);

        //
        // There's no data returned for this call.
        //
        BytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY:
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(UCHAR),
                                                &SevenSegment,
                                                NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "User's output buffer is too small for this IOCTL, expecting an UCHAR\n");
            break;
        }

        //
        // Call our function to get the 7 segment state.
        //
        Status = GetSevenSegmentState(DeviceContext, SevenSegment);

        //
        // If we succeeded return the user their data.
        //
        if (NT_SUCCESS(Status))
        {
            BytesReturned = sizeof(UCHAR);
        }
        else
        {
            BytesReturned = 0;
        }

        break;

    case IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY:
        Status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(UCHAR),
                                               &SevenSegment,
                                               NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "User's input buffer is too small for this IOCTL, expecting an UCHAR\n");

            BytesReturned = sizeof(UCHAR);
            break;
        }

        //
        // Call our routine to set the 7 segment state.
        //
        Status = SetSevenSegmentState(DeviceContext, SevenSegment);

        //
        // There's no data returned for this call.
        //
        BytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_READ_SWITCHES:
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(SWITCH_STATE),
                                                &SwitchState,
                                                NULL);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "User's output buffer is too small for this IOCTL, expecting a SWITCH_STATE\n");

            BytesReturned = sizeof(SWITCH_STATE);
            break;
        }

        //
        // Call our routine to get the state of the switches.
        //
        Status = GetSwitchState(DeviceContext, SwitchState);

        //
        // If successful, return the user their data.
        //
        if (NT_SUCCESS(Status))
        {
            BytesReturned = sizeof(SWITCH_STATE);
        }
        else
        {
            //
            // Don't return any data.
            //
            BytesReturned = 0;
        }

        break;

    case IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE:
        //
        // Forward the request to an interrupt message queue and dont complete
        // the request until an interrupt from the USB Device occurs.
        //
        Status = WdfRequestForwardToIoQueue(Request,
                                            DeviceContext->InterruptMsgQueue);

        if (NT_SUCCESS(Status))
        {
            RequestPending = TRUE;
        }

        break;

    default :
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (RequestPending == FALSE)
    {
        WdfRequestCompleteWithInformation(Request, Status, BytesReturned);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                "<-- OsrFxEvtIoDeviceControl\n");

    return;
}


/*++

Routine Description:

    This routine resets the pipe.

Arguments:

    Pipe - framework pipe handle

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
ResetPipe(
    _In_ WDFUSBPIPE Pipe
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // This routine synchronously submits a URB_FUNCTION_RESET_PIPE
    // request down the stack.
    //
    Status = WdfUsbTargetPipeResetSynchronously(Pipe,
                                                WDF_NO_HANDLE,
                                                NULL);

    if (NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "ResetPipe - success\n");

        Status = STATUS_SUCCESS;
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "ResetPipe - failed\n");
    }

    return Status;
}


/*++

Routine Description:

    This routine stops all of the device's pipes.

Arguments:

    DeviceContext - The device context with the pipe information

Return Value:

    VOID

--*/
VOID
StopAllPipes(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe),
                    WdfIoTargetCancelSentIo);
                           
    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkReadPipe),
                    WdfIoTargetCancelSentIo);

    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkWritePipe),
                    WdfIoTargetCancelSentIo);
}


/*++

Routine Description:

    This routine starts all of the device's pipes.

Arguments:

    DeviceContext - The device context with the pipe information

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
NTSTATUS
StartAllPipes(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS Status;

    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkReadPipe));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkWritePipe));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return Status;
}


/*++

Routine Description:

    This routine calls WdfUsbTargetDeviceResetPortSynchronously to reset the
    device if it's still connected.

Arguments:

    Device - Handle to a framework device object

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
ResetDevice(
    _In_ WDFDEVICE Device
    )
{
    PDEVICE_CONTEXT DeviceContext;
    NTSTATUS Status;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                "--> ResetDevice\n");

    DeviceContext = GetDeviceContext(Device);

    //
    // A NULL timeout indicates an infinite wake.
    //
    Status = WdfWaitLockAcquire(DeviceContext->ResetDeviceWaitLock, NULL);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "ResetDevice - could not acquire lock\n");

        return Status;
    }

    StopAllPipes(DeviceContext);
    
    Status = WdfUsbTargetDeviceResetPortSynchronously(DeviceContext->UsbDevice);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "ResetDevice failed - 0x%x\n",
                    Status);
    }

    Status = StartAllPipes(DeviceContext);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "Failed to start all pipes - 0x%x\n",
                    Status);
    }
    
    WdfWaitLockRelease(DeviceContext->ResetDeviceWaitLock);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                "<-- ResetDevice\n");

    return Status;
}


/*++

Routine Description

    This routine re-enumerates the USB device.

Arguments:

    DeviceContext - One of our device extensions

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
ReenumerateDevice(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    GUID Activity;
    
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "--> ReenumerateDevice\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);
              
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestHostToDevice,
                                             BmRequestToDevice,
                                             USBFX2LK_REENUMERATE,
                                             0,
                                             0);


    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                WDF_NO_HANDLE,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                NULL,
                                                                NULL);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "ReenumerateDevice: Failed to Reenumerate - 0x%x \n",
                    Status);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "<-- ReenumerateDevice\n");

    //
    // Send event to the event log.
    //
    Activity = DeviceToActivityId(WdfObjectContextGetObject(DeviceContext));

    EventWriteDeviceReenumerated(DeviceContext->DeviceName,
                                 DeviceContext->Location,
                                 Status);

    return Status;

}


/*++

Routine Description

    This routine gets the state of the bar graph on the board.

Arguments:

    DeviceContext - One of our device extensions

    BarGraphState - Struct that receives the bar graph's state

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
GetBarGraphState(
    _In_  PDEVICE_CONTEXT  DeviceContext, 
    _Out_ PBAR_GRAPH_STATE BarGraphState
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG BytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "--> GetBarGraphState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestDeviceToHost,
                                             BmRequestToDevice,
                                             USBFX2LK_READ_BARGRAPH_DISPLAY,
                                             0,
                                             0);

    //
    // Set the buffer to 0, the board will OR in everything that is set.
    //
    BarGraphState->BarsAsUChar = 0;


    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor,
                                      BarGraphState,
                                      sizeof(BAR_GRAPH_STATE));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                WDF_NO_HANDLE,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                &MemoryDescriptor,
                                                                &BytesTransferred);

    if(!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "GetBarGraphState: Failed to GetBarGraphState - 0x%x \n",
                    Status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                    "GetBarGraphState: LED mask is 0x%x\n",
                    BarGraphState->BarsAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "<-- GetBarGraphState\n");

    return Status;

}


/*++

Routine Description

    This routine sets the state of the bar graph on the board.

Arguments:

    DeviceContext - One of our device extensions

    BarGraphState - Struct that describes the bar graph's desired state

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
SetBarGraphState(
    _In_ PDEVICE_CONTEXT  DeviceContext, 
    _In_ PBAR_GRAPH_STATE BarGraphState
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG BytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "--> SetBarGraphState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestHostToDevice,
                                             BmRequestToDevice,
                                             USBFX2LK_SET_BARGRAPH_DISPLAY,
                                             0,
                                             0);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor,
                                      BarGraphState,
                                      sizeof(BAR_GRAPH_STATE));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                NULL,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                &MemoryDescriptor,
                                                                &BytesTransferred);

    if(!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "SetBarGraphState: Failed - 0x%x \n",
                    Status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                    "SetBarGraphState: LED mask is 0x%x\n",
                    BarGraphState->BarsAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "<-- SetBarGraphState\n");

    return Status;
}


/*++

Routine Description

    This routine gets the state of the 7 segment display on the board
    by sending a synchronous control command.

    NOTE: It's not good practice to send a synchronous request in the
          context of the user thread because if the transfer takes a long
          time to complete, you end up holding the user thread.

          I'm choosing to do synchronous transfer because a) I know this one
          completes immediately and b) for demonstration.

Arguments:

    DeviceContext - One of our device extensions

    SevenSegment  - Receives the state of the 7 segment display

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
GetSevenSegmentState(
    _In_  PDEVICE_CONTEXT DeviceContext, 
    _Out_ PUCHAR          SevenSegment
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG BytesTransferred;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "GetSetSevenSegmentState: Enter\n");

    PAGED_CODE();

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestDeviceToHost,
                                             BmRequestToDevice,
                                             USBFX2LK_READ_7SEGMENT_DISPLAY,
                                             0,
                                             0);

    //
    // Set the buffer to 0, the board will OR in everything that is set.
    //
    *SevenSegment = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor,
                                      SevenSegment,
                                      sizeof(UCHAR));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                NULL,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                &MemoryDescriptor,
                                                                &BytesTransferred);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "GetSevenSegmentState: Failed to get 7 Segment state - 0x%x \n",
                    Status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                    "GetSevenSegmentState: 7 Segment mask is 0x%x\n",
                    *SevenSegment);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "GetSetSevenSegmentState: Exit\n");

    return Status;
}


/*++

Routine Description

    This routine sets the state of the 7 segment display on the board.

Arguments:

    DeviceContext - One of our device extensions

    SevenSegment  - Desired state of the 7 segment display

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
SetSevenSegmentState(
    _In_ PDEVICE_CONTEXT DeviceContext, 
    _In_ PUCHAR SevenSegment
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG BytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "--> SetSevenSegmentState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestHostToDevice,
                                             BmRequestToDevice,
                                             USBFX2LK_SET_7SEGMENT_DISPLAY,
                                             0,
                                             0);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor,
                                    SevenSegment,
                                    sizeof(UCHAR));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                NULL,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                &MemoryDescriptor,
                                                                &BytesTransferred);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "SetSevenSegmentState: Failed to set 7 Segment state - 0x%x \n",
                    Status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                    "SetSevenSegmentState: 7 Segment mask is 0x%x\n",
                    *SevenSegment);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "<-- SetSevenSegmentState\n");

    return Status;

}


/*++

Routine Description

    This routine gets the state of the switches on the board.

Arguments:

    DeviceContext - One of our device extensions

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
GetSwitchState(
    _In_ PDEVICE_CONTEXT DeviceContext, 
    _In_ PSWITCH_STATE SwitchState
    )
{
    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET ControlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG BytesTransferred;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "--> GetSwitchState\n");

    PAGED_CODE();

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&ControlSetupPacket,
                                             BmRequestDeviceToHost,
                                             BmRequestToDevice,
                                             USBFX2LK_READ_SWITCHES,
                                             0,
                                             0);

    SwitchState->SwitchesAsUChar = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor,
                                      SwitchState,
                                      sizeof(SWITCH_STATE));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(DeviceContext->UsbDevice,
                                                                NULL,
                                                                &SendOptions,
                                                                &ControlSetupPacket,
                                                                &MemoryDescriptor,
                                                                &BytesTransferred);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "GetSwitchState: Failed to get switches - 0x%x \n",
                    Status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                    "GetSwitchState: Switch mask is 0x%x\n",
                    SwitchState->SwitchesAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
                "<-- GetSwitchState\n");

    return Status;
}


/*++

Routine Description

    This method handles the completion of the pending request for the IOCTL
    IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE.

Arguments:

    Device - Handle to a framework device object

Return Value:

    VOID

--*/
VOID
OsrUsbIoctlGetInterruptMessage(
    _In_ WDFDEVICE Device,
    _In_ NTSTATUS  ReaderStatus
    )
{
    NTSTATUS Status;
    WDFREQUEST Request;
    PDEVICE_CONTEXT DeviceContext;
    size_t BytesReturned = 0;
    PSWITCH_STATE SwitchState = NULL;

    DeviceContext = GetDeviceContext(Device);

    do
    {
        //
        // Check if there are any pending requests in the Interrupt Message Queue.
        // If a request is found then complete the pending request.
        //
        Status = WdfIoQueueRetrieveNextRequest(DeviceContext->InterruptMsgQueue,
                                               &Request);

        if (NT_SUCCESS(Status))
        {
            Status = WdfRequestRetrieveOutputBuffer(Request,
                                                    sizeof(SWITCH_STATE),
                                                    &SwitchState,
                                                    NULL);

            if (!NT_SUCCESS(Status))
            {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                            "User's output buffer is too small for this IOCTL, expecting a SWITCH_STATE\n");

                BytesReturned = sizeof(SWITCH_STATE);
            }
            else
            {
                //
                // Copy the state information saved by the continuous reader.
                //
                if (NT_SUCCESS(ReaderStatus))
                {
                    SwitchState->SwitchesAsUChar = DeviceContext->CurrentSwitchState;
                    BytesReturned = sizeof(SWITCH_STATE);
                }
                else
                {
                    BytesReturned = 0;
                }
            }

            //
            // Complete the request. If we failed to get the output buffer then 
            // complete with that status. Otherwise complete with the Status from the reader.
            //
            WdfRequestCompleteWithInformation(Request, 
                                              NT_SUCCESS(Status) ? ReaderStatus :
                                                                   Status, 
                                              BytesReturned);

            Status = STATUS_SUCCESS;
        }
        else if (Status != STATUS_NO_MORE_ENTRIES)
        {
            KdPrint(("WdfIoQueueRetrieveNextRequest Status %08x\n", Status));
        }

        Request = NULL;

    } while (Status == STATUS_SUCCESS);
}


