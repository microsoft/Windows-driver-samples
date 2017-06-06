/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    bulkrwr.c

Abstract:

    This file has routines to perform reads and writes.
    The read and writes are targeted bulk to endpoints.

Environment:

    User mode

--*/

#include "osrusbfx2.h"


#if defined(EVENT_TRACING)
#include "bulkrwr.tmh"
#endif

#pragma warning(disable:4267)


/*++

Routine Description:

    Called by the framework when it receives Read or Write requests.

Arguments:

    Queue   - Default queue handle

    Request - Handle to the read/write request

    Length  - Length of the data buffer associated with the request.
              The default property of the queue is to not dispatch
              zero lenght read & write requests to the driver and
              complete is with status success. So we will never get
              a zero length request

Return Value:

    VOID

--*/
VOID
OsrFxEvtIoRead(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     Length
    )
{
    WDFUSBPIPE Pipe;
    NTSTATUS Status;
    WDFMEMORY RequiredMemory;
    PDEVICE_CONTEXT DeviceContext;

    UNREFERENCED_PARAMETER(Queue);

    // 
    // Log read start event, using IRP activity ID if available or request
    // handle otherwise.
    //
    EventWriteReadStart(WdfIoQueueGetDevice(Queue), (ULONG)Length);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ, "-->OsrFxEvtIoRead\n");

    //
    // First validate input parameters.
    //
    if (Length > TEST_BOARD_TRANSFER_BUFFER_SIZE) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "Transfer exceeds %d\n",
                            TEST_BOARD_TRANSFER_BUFFER_SIZE);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    DeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    Pipe = DeviceContext->BulkReadPipe;

    Status = WdfRequestRetrieveOutputMemory(Request, &RequiredMemory);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "WdfRequestRetrieveOutputMemory failed %!STATUS!\n", Status);
        goto Exit;
    }

    //
    // The format call validates to make sure that you are reading or
    // writing to the right Pipe type, sets the appropriate transfer flags,
    // creates an URB and initializes the request.
    //
    Status = WdfUsbTargetPipeFormatRequestForRead(Pipe,
                                                  Request,
                                                  RequiredMemory,
                                                  NULL // Offsets
                                                  );

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "WdfUsbTargetPipeFormatRequestForRead failed 0x%x\n", Status);
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request,
                                   EvtRequestReadCompletionRoutine,
                                   Pipe);
    //
    // Send the request asynchronously.
    //
    if (WdfRequestSend(Request,
                       WdfUsbTargetPipeGetIoTarget(Pipe),
                       WDF_NO_SEND_OPTIONS) == FALSE)
    {
        //
        // Framework couldn't send the request for some reason.
        //
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "WdfRequestSend failed\n");
        Status = WdfRequestGetStatus(Request);
        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(Status))
    {
        //
        // Log event read failed.
        //
        EventWriteReadFail(WdfIoQueueGetDevice(Queue), Status);
        WdfRequestCompleteWithInformation(Request, Status, 0);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ, "<-- OsrFxEvtIoRead\n");

    return;
}


/*++

Routine Description:

    This is the completion routine for reads
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context

    Device  - Device handle

    Request - Request handle

    Params  - request completion params

Return Value:

    VOID

--*/
VOID
EvtRequestReadCompletionRoutine(
    _In_ WDFREQUEST                     Request,
    _In_ WDFIOTARGET                    Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT                     Context
    )
{
    NTSTATUS Status;
    size_t BytesRead = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS UsbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    Status = CompletionParams->IoStatus.Status;

    UsbCompletionParams = CompletionParams->Parameters.Usb.Completion;

    BytesRead =  UsbCompletionParams->Parameters.PipeRead.Length;

    if (NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                    "Number of bytes read: %I64d\n", (INT64)BytesRead);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "Read failed - request status 0x%x UsbdStatus 0x%x\n",
                    Status, UsbCompletionParams->UsbdStatus);

    }

    //
    // Log read stop event, using IRP activity ID if available or request
    // handle otherwise.
    //
    EventWriteReadStop(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)),
                       BytesRead, 
                       Status, 
                       UsbCompletionParams->UsbdStatus);

    WdfRequestCompleteWithInformation(Request, Status, BytesRead);

    return;
}


/*++

Routine Description:

    Called by the framework when it receives Read or Write requests.

Arguments:

    Queue   - Default queue handle

    Request - Handle to the read/write request

    Lenght  - Length of the data buffer associated with the request.
              The default property of the queue is to not dispatch
              zero lenght read & write requests to the driver and
              complete is with status success. So we will never get
              a zero length request

Return Value:

    VOID

--*/
VOID 
OsrFxEvtIoWrite(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     Length
    ) 
{
    NTSTATUS        Status;
    WDFUSBPIPE      Pipe;
    WDFMEMORY       RequiredMemory;
    PDEVICE_CONTEXT DeviceContext;

    UNREFERENCED_PARAMETER(Queue);


    // 
    // Log write start event, using IRP activity ID if available or request
    // handle otherwise.
    //
    EventWriteWriteStart(WdfIoQueueGetDevice(Queue), (ULONG)Length);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "-->OsrFxEvtIoWrite\n");

    //
    // First validate input parameters.
    //
    if (Length > TEST_BOARD_TRANSFER_BUFFER_SIZE)
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "Transfer exceeds %d\n",
                    TEST_BOARD_TRANSFER_BUFFER_SIZE);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    DeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    Pipe = DeviceContext->BulkWritePipe;

    Status = WdfRequestRetrieveInputMemory(Request, &RequiredMemory);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "WdfRequestRetrieveInputBuffer failed\n");
        goto Exit;
    }

    Status = WdfUsbTargetPipeFormatRequestForWrite(Pipe,
                                                   Request,
                                                   RequiredMemory,
                                                   NULL // Offset
                                                   );


    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "WdfUsbTargetPipeFormatRequestForWrite failed 0x%x\n", Status);
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request,
                                   EvtRequestWriteCompletionRoutine,
                                   Pipe);

    //
    // Send the request asynchronously.
    //
    if (WdfRequestSend(Request,
                       WdfUsbTargetPipeGetIoTarget(Pipe),
                       WDF_NO_SEND_OPTIONS) == FALSE)
    {
        //
        // Framework couldn't send the request for some reason.
        //
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "WdfRequestSend failed\n");
        Status = WdfRequestGetStatus(Request);
        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(Status))
    {
        //
        // log event write failed.
        //
        EventWriteWriteFail(WdfIoQueueGetDevice(Queue), Status);
        WdfRequestCompleteWithInformation(Request, Status, 0);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "<-- OsrFxEvtIoWrite\n");

    return;
}


/*++

Routine Description:

    This is the completion routine for writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context

    Device  - Device handle

    Request - Request handle

    Params  - request completion params

Return Value:

    VOID

--*/
VOID
EvtRequestWriteCompletionRoutine(
    _In_ WDFREQUEST                     Request,
    _In_ WDFIOTARGET                    Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT                     Context
    )
{
    NTSTATUS Status;
    size_t BytesWritten = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS UsbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    Status = CompletionParams->IoStatus.Status;

    //
    // For usb devices, we should look at the Usb.Completion param.
    //
    UsbCompletionParams = CompletionParams->Parameters.Usb.Completion;

    BytesWritten =  UsbCompletionParams->Parameters.PipeWrite.Length;

    if (NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                    "Number of bytes written: %I64d\n",
                    (INT64)BytesWritten);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "Write failed: request status 0x%x UsbdStatus 0x%x\n",
                    Status,
                    UsbCompletionParams->UsbdStatus);
    }

    //
    // Log write stop event, using IRP activtiy ID if available or request
    // handle otherwise.
    //
    EventWriteWriteStop(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)),
                        BytesWritten, 
                        Status, 
                        UsbCompletionParams->UsbdStatus);

    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);

    return;
}


/*++

Routine Description:

    This callback is invoked on every inflight request when the device
    is suspended or removed. Since our inflight read and write requests
    are actually pending in the target device, we will just acknowledge
    its presence. Until we acknowledge, complete, or requeue the requests
    framework will wait before allowing the device suspend or remove to
    proceeed. When the underlying USB stack gets the request to suspend or
    remove, it will fail all the pending requests.

Arguments:

    Queue       - Handle to queue object that is associated with the I/O request
    
    Request     - Handle to a request object
    
    ActionFlags - Bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS flags

Return Value:

    VOID

--*/
VOID
OsrFxEvtIoStop(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG      ActionFlags
    )
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);

    if (ActionFlags &  WdfRequestStopActionSuspend)
    {
        //
        // Don't requeue.
        //
        WdfRequestStopAcknowledge(Request, FALSE);
    }
    else if(ActionFlags & WdfRequestStopActionPurge)
    {
        WdfRequestCancelSentRequest(Request);
    }
    return;
}


