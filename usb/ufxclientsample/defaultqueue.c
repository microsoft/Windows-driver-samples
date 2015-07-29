/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    defaultqueue.c

Abstract:

    Implements the device's default queue callbacks.

Environment:

    Kernel mode

--*/

#include "device.h"
#include "defaultqueue.h"
#include "defaultqueue.tmh"

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DefaultQueue_EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL DefaultQueue_EvtIoInternalDeviceControl;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, DefaultQueueCreate)
#endif

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
DefaultQueueCreate(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     Creates and configures the default IO Queue for the device.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    Appropriate NTSTATUS message

--*/
{
    NTSTATUS Status;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    
    TraceEntry();

    PAGED_CODE();
    
    //
    // Create the default queue
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
                                    &QueueConfig,
                                    WdfIoQueueDispatchParallel
                                    );

    QueueConfig.PowerManaged = WdfTrue;
    QueueConfig.EvtIoDeviceControl = DefaultQueue_EvtIoDeviceControl;
    QueueConfig.EvtIoInternalDeviceControl = DefaultQueue_EvtIoInternalDeviceControl;

    Status = WdfIoQueueCreate(
                             Device,
                             &QueueConfig,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             NULL);

    CHK_NT_MSG(Status, "WdfIoQueueCreate failed");

End:

    TraceExit();
    return Status;
}

VOID
DefaultQueue_EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    EvtIoDeviceControl handler for the default Queue

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

--*/
{
    WDFDEVICE WdfDevice;
    PCONTROLLER_CONTEXT ControllerContext;
    BOOLEAN HandledbyUfx;   

    TraceEntry();

    TraceVerbose("Queue 0x%p, Request 0x%p, OutputBufferLength %d, "
                  "InputBufferLength %d, IoControlCode %d",
                  Queue, Request, (int) OutputBufferLength, 
                  (int) InputBufferLength, IoControlCode);
    
    WdfDevice = WdfIoQueueGetDevice(Queue);
    ControllerContext = DeviceGetControllerContext(WdfDevice);

    HandledbyUfx = UfxDeviceIoControl(
                        ControllerContext->UfxDevice,
                        Request,
                        OutputBufferLength,
                        InputBufferLength,
                        IoControlCode);

    if (!HandledbyUfx) {
        TraceError("Recieved an unsupported IOCTL");
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
    }

    TraceExit();
}

VOID
DefaultQueue_EvtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    EvtIoInternalDeviceControl handler for the default Queue

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

--*/
{
    WDFDEVICE WdfDevice;
    PCONTROLLER_CONTEXT ControllerContext;
    BOOLEAN HandledbyUfx;   

    TraceEntry();

    TraceVerbose("InternalQueue 0x%p, Request 0x%p, OutputBufferLength %d, "
                  "InputBufferLength %d, IoControlCode %d",
                  Queue, Request, (int) OutputBufferLength, 
                  (int) InputBufferLength, IoControlCode);
    
    WdfDevice = WdfIoQueueGetDevice(Queue);
    ControllerContext = DeviceGetControllerContext(WdfDevice);

    HandledbyUfx = UfxDeviceIoInternalControl(
                        ControllerContext->UfxDevice,
                        Request,
                        OutputBufferLength,
                        InputBufferLength,
                        IoControlCode);

    if (!HandledbyUfx) {
        TraceError("Recieved an un supported IOCTL");
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
    }

    TraceExit();
}

