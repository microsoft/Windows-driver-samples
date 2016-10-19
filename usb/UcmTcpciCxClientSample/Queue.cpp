/*++

Module Name:

    Queue.c

Abstract:

    This file contains the I/O queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HardwareRequestQueueInitialize)
#endif

NTSTATUS
HardwareRequestQueueInitialize(
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single I/O Queue is configured for sequential request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Returns:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_QUEUE);

    PAGED_CODE();

    NTSTATUS status;
    WDFQUEUE hardwareRequestQueue;
    WDF_IO_QUEUE_CONFIG queueConfig;
    PDEVICE_CONTEXT deviceContext;

    deviceContext = DeviceGetContext(Device);

    WDF_IO_QUEUE_CONFIG_INIT(
        &queueConfig,
        WdfIoQueueDispatchSequential);

    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    queueConfig.EvtIoStop = EvtIoStop;

    // Create the hardware request queue.
    status = WdfIoQueueCreate(Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hardwareRequestQueue);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        goto Exit;
    }

    // Set this queue as the one to which UcmTcpciCx will forward its hardware requests.
    UcmTcpciPortControllerSetHardwareRequestQueue(deviceContext->PortController, hardwareRequestQueue);

Exit:
    TRACE_FUNC_EXIT(TRACE_QUEUE);
    return status;
}

VOID
EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_QUEUE);

    WDFDEVICE device;
    PDEVICE_CONTEXT deviceContext;

    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_QUEUE,
        "Queue 0x%p, Request 0x%p OutputBufferLength %Iu InputBufferLength %Iu IoControlCode %!UCMTCPCI_PORT_CONTROLLER_IOCTL!",
        Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    // Save the WDFREQUEST so we can complete it later.
    deviceContext->IncomingRequest = Request;

    // Check if we recognize the IOCTL.
    // The helper functions perform the requested hardware read or write and complete the WDFREQUEST.

    // Note: The driver would need to support some of the other IOCTLs from UcmTcpciCx if the
    // capabilities indicate that they are supported.
    switch (IoControlCode)
    {
    case IOCTL_UCMTCPCI_PORT_CONTROLLER_GET_STATUS:
        PostponeToWorkitem(Request, device, deviceContext->I2CWorkItemGetStatus);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_GET_CONTROL:
        PostponeToWorkitem(Request, device, deviceContext->I2CWorkItemGetControl);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_CONTROL:
        EvtSetControl(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT:
        EvtSetTransmit(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT_BUFFER:
        EvtSetTransmitBuffer(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_RECEIVE_DETECT:
        EvtSetReceiveDetect(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_CONFIG_STANDARD_OUTPUT:
        EvtSetConfigStandardOutput(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_COMMAND:
        EvtSetCommand(Request, device);
        break;

    case IOCTL_UCMTCPCI_PORT_CONTROLLER_SET_MESSAGE_HEADER_INFO:
        EvtSetMessageHeaderInfo(Request, device);
        break;

    default:
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
            "Received unexpected IoControlCode %lu", IoControlCode);
        deviceContext->IncomingRequest = WDF_NO_HANDLE;
        // If we don't recognize the IOCTL, we must complete the WDFREQUEST here.
        WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
    }

    TRACE_FUNC_EXIT(TRACE_QUEUE);
}

VOID
EvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_QUEUE);

    UNREFERENCED_PARAMETER(ActionFlags);
    UNREFERENCED_PARAMETER(Request);

    WDFDEVICE device;
    PDEVICE_CONTEXT deviceContext;

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    // Attempt to cancel the I2C WDFREQUESTs.
    if (deviceContext->I2CAsyncRequest != WDF_NO_HANDLE)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
            "[WDFREQUEST: 0x%p] Attempting to cancel.", deviceContext->I2CAsyncRequest);
        WdfRequestCancelSentRequest(deviceContext->I2CAsyncRequest);
    }

    TRACE_FUNC_EXIT(TRACE_QUEUE);
}