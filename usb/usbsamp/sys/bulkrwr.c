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
    The read and writes are for bulk transfers.

Environment:

    Kernel mode

--*/

#include "private.h"

#if !defined(BUFFERED_READ_WRITE) // if doing DIRECT_IO

VOID
ReadWriteBulkEndPoints(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            Length,
    _In_ WDF_REQUEST_TYPE RequestType
    )
/*++

Routine Description:

    This callback is invoked when the framework received  WdfRequestTypeRead or
    WdfRequestTypeWrite request. This read/write is performed in stages of
    maximum transfer size. Once a stage of transfer is complete, then the
    request is circulated again, until the requested length of transfer is
    performed.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the WdfRequestTypeRead/WdfRequestTypeWrite IRP received by the framework.

    Length - Length of the input/output buffer.

Return Value:

   VOID

--*/
{
    PMDL                    newMdl=NULL, requestMdl = NULL;
    PURB                    urb = NULL;
    WDFMEMORY               urbMemory;
    ULONG                   totalLength = Length;
    ULONG                   stageLength = 0;
    ULONG                   urbFlags = 0;
    NTSTATUS                status;
    ULONG_PTR               virtualAddress = 0;
    PREQUEST_CONTEXT        rwContext = NULL;
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;
    WDF_OBJECT_ATTRIBUTES   objectAttribs;
    USBD_PIPE_HANDLE        usbdPipeHandle;
    PDEVICE_CONTEXT         deviceContext;
    ULONG                   maxTransferSize;
    PPIPE_CONTEXT           pipeContext;

    UsbSamp_DbgPrint(3, ("UsbSamp_DispatchReadWrite - begins\n"));

    //
    // First validate input parameters.
    //
    deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    if (totalLength > deviceContext->MaximumTransferSize) {
        UsbSamp_DbgPrint(1, ("Transfer length > circular buffer\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((RequestType != WdfRequestTypeRead) &&
        (RequestType != WdfRequestTypeWrite)) {
        UsbSamp_DbgPrint(1, ("RequestType has to be either Read or Write\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    pipeContext = GetPipeContext(pipe);
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if ((WdfUsbPipeTypeBulk != pipeInfo.PipeType) &&
            (WdfUsbPipeTypeInterrupt != pipeInfo.PipeType)) {
        UsbSamp_DbgPrint(1, ("Usbd pipe type is not bulk or interrupt\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit;

    }

    rwContext = GetRequestContext(Request);

    if (RequestType == WdfRequestTypeRead) {

        status = WdfRequestRetrieveOutputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveOutputWdmMdl failed %x\n", status));
            goto Exit;
        }

        urbFlags |= USBD_TRANSFER_DIRECTION_IN;
        rwContext->Read = TRUE;
        UsbSamp_DbgPrint(3, ("Read operation\n"));

    } 
    else {
        status = WdfRequestRetrieveInputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveInputWdmMdl failed %x\n", status));
            goto Exit;
        }

        urbFlags |= USBD_TRANSFER_DIRECTION_OUT;
        rwContext->Read = FALSE;
        UsbSamp_DbgPrint(3, ("Write operation\n"));
    }

    urbFlags |= USBD_SHORT_TRANSFER_OK;
    virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(requestMdl);

    //
    // The transfer request is for totalLength.
    // We can perform a max of maxTransfersize in each stage.
    //
    maxTransferSize = GetMaxTransferSize(pipe, deviceContext);

    if (totalLength > maxTransferSize) {
        stageLength = maxTransferSize;
    }
    else {
        stageLength = totalLength;
    }

    newMdl = IoAllocateMdl((PVOID) virtualAddress,
                           totalLength,
                           FALSE,
                           FALSE,
                           NULL);

    if (newMdl == NULL) {
        UsbSamp_DbgPrint(1, ("Failed to alloc mem for mdl\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // map the portion of user-buffer described by an mdl to another mdl
    //
    IoBuildPartialMdl(requestMdl,
                      newMdl,
                      (PVOID) virtualAddress,
                      stageLength);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Request;

    status = WdfUsbTargetDeviceCreateUrb(deviceContext->WdfUsbTargetDevice,
		                     &objectAttribs,
                             &urbMemory,
                             &urb);

    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfUsbTargetDeviceCreateUrb failed %x\n", status));
        goto Exit;
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    if(WdfUsbPipeTypeBulk == pipeInfo.PipeType &&
        pipeContext->StreamConfigured == TRUE) {
        //
        // For super speed bulk pipe with streams, we specify one of its associated 
        // usbd pipe handles to format an URB for sending or receiving data. 
        // The usbd pipe handle is returned by the HCD via sucessful open-streams request
        //
        usbdPipeHandle = GetStreamPipeHandleFromBulkPipe(pipe);
    }
    else {
        usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(pipe);
    }
#else
    usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(pipe);
#endif

    UsbBuildInterruptOrBulkTransferRequest(urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           usbdPipeHandle,
                                           NULL,
                                           newMdl,
                                           stageLength,
                                           urbFlags,
                                           NULL);

    status = WdfUsbTargetPipeFormatRequestForUrb(pipe, Request, urbMemory, NULL  );
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("Failed to format requset for urb\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request, UsbSamp_EvtReadWriteCompletion, deviceContext);

    //
    // set REQUEST_CONTEXT  parameters.
    //
    rwContext->UrbMemory       = urbMemory;
    rwContext->Mdl             = newMdl;
    rwContext->Length          = totalLength - stageLength;
    rwContext->Numxfer         = 0;
    rwContext->VirtualAddress  = virtualAddress + stageLength;

    if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS)) {
        status = WdfRequestGetStatus(Request);
        NT_ASSERT(!NT_SUCCESS(status));
    }

Exit:
    if (!NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, 0);

        if (newMdl != NULL) {
            IoFreeMdl(newMdl);
        }
    }

    UsbSamp_DbgPrint(3, ("UsbSamp_DispatchReadWrite - ends\n"));

    return;
}


VOID
UsbSamp_EvtReadWriteCompletion(
    _In_ WDFREQUEST                  Request,
    _In_ WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT                  Context
    )
/*++

Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context
    Device - Device handle
    Request - Request handle
    Params - request completion params

Return Value:
    None

--*/
{
    PMDL                    requestMdl;
    WDFUSBPIPE              pipe;
    ULONG                   stageLength;
    NTSTATUS               status;
    PREQUEST_CONTEXT        rwContext;
    PURB                    urb;
    PCHAR                   operation;
    ULONG                   bytesReadWritten;
    ULONG                   maxTransferSize;
    PDEVICE_CONTEXT         deviceContext;

    rwContext = GetRequestContext(Request);
    deviceContext = Context;

    if (rwContext->Read) {
        operation = "Read";
    } 
    else {
        operation = "Write";
    }

    pipe = (WDFUSBPIPE) Target   ;
    status = CompletionParams->IoStatus.Status;

    if (!NT_SUCCESS(status)){
        //
        // Queue a workitem to reset the pipe because the completion could be
        // running at DISPATCH_LEVEL.
        //
        QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
        goto End;
    }

    urb = (PURB) WdfMemoryGetBuffer(rwContext->UrbMemory, NULL);
    bytesReadWritten = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    rwContext->Numxfer += bytesReadWritten;

    //
    // If there is anything left to transfer.
    //
    if (rwContext->Length == 0) {
        //
        // this is the last transfer
        //
        WdfRequestSetInformation(Request, rwContext->Numxfer);
        goto End;
    }

    //
    // Start another transfer
    //
    UsbSamp_DbgPrint(3, ("Stage next %s transfer...\n", operation));

    //
    // The transfer request is for totalLength. 
    // We can perform a max of maxTransfersize in each stage.
    //
    maxTransferSize = GetMaxTransferSize(pipe, deviceContext);

    if (rwContext->Length > maxTransferSize) {
        stageLength = maxTransferSize;
    }
    else {
        stageLength = rwContext->Length;
    }

    //
    // Following call is required to free any mapping made on the partial MDL
    // and reset internal MDL state.
    //
    MmPrepareMdlForReuse(rwContext->Mdl);

    if (rwContext->Read) {
        status = WdfRequestRetrieveOutputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveOutputWdmMdl for Read failed %x\n", status));
            goto End;
        }
    } 
    else {
        status = WdfRequestRetrieveInputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveInputWdmMdl for Write failed %x\n", status));
            goto End;
        }
    }

    IoBuildPartialMdl(requestMdl,
                      rwContext->Mdl,
                      (PVOID) rwContext->VirtualAddress,
                      stageLength);

    //
    // reinitialize the urb
    //
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength = stageLength;

    rwContext->VirtualAddress += stageLength;
    rwContext->Length -= stageLength;

    //
    // Format the request to send a URB to a USB pipe.
    //
    status = WdfUsbTargetPipeFormatRequestForUrb(pipe,
                                Request,
                                rwContext->UrbMemory,
                                NULL);
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("Failed to format requset for urb\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    WdfRequestSetCompletionRoutine(Request, UsbSamp_EvtReadWriteCompletion, deviceContext);

    //
    // Send the request asynchronously.
    //
    if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS)) {
        UsbSamp_DbgPrint(1, ("WdfRequestSend for %s failed\n", operation));
        status = WdfRequestGetStatus(Request);
        goto End;
    }

    //
    // Else when the request completes, this completion routine will be
    // called again.
    //
    return;

End:
    //
    // We are here because the request failed or some other call failed.
    // Dump the request context, complete the request and return.
    //
    DbgPrintRWContext(rwContext);

    IoFreeMdl(rwContext->Mdl);

    UsbSamp_DbgPrint(3, ("%s request completed with status 0x%x\n",
                                                    operation, status));

    WdfRequestComplete(Request, status);

    return;
}

#else

VOID
ReadWriteBulkEndPoints(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            Length,
    _In_ WDF_REQUEST_TYPE RequestType
    )
/*++

Routine Description:

    This callback is invoked when the framework received  WdfRequestTypeRead or
    RP_MJ_WRITE request. This read/write is performed in stages of
    maximum transfer size. Once a stage of transfer is complete, then the
    request is circulated again, until the requested length of transfer is
    performed.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the WdfRequestTypeRead/WdfRequestTypeWrite IRP received by the framework.

    Length - Length of the input/output buffer.

Return Value:

   VOID

--*/
{
    size_t                totalLength = Length;
    size_t                stageLength = 0;
    NTSTATUS              status;
    PVOID                 virtualAddress = 0;
    PREQUEST_CONTEXT      rwContext = NULL;
    PFILE_CONTEXT         fileContext = NULL;
    WDFUSBPIPE            pipe;
    WDF_USB_PIPE_INFORMATION pipeInfo;
    WDFMEMORY             reqMemory;
    WDFMEMORY_OFFSET      offset;
    WDF_OBJECT_ATTRIBUTES objectAttribs;
    PDEVICE_CONTEXT       deviceContext;
    PPIPE_CONTEXT         pipeContext;

    ULONG                 maxTransferSize;
 
    UsbSamp_DbgPrint(3, ("UsbSamp_DispatchReadWrite - begins\n"));

    //
    // First validate input parameters.
    //
    deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    if (totalLength > deviceContext->MaximumTransferSize) {
        UsbSamp_DbgPrint(1, ("Transfer length > circular buffer\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((RequestType != WdfRequestTypeRead) &&
        (RequestType != WdfRequestTypeWrite)) {
        UsbSamp_DbgPrint(1, ("RequestType has to be either Read or Write\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    pipeContext = GetPipeContext(pipe);
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    rwContext = GetRequestContext(Request);

    if (RequestType == WdfRequestTypeRead) {
        status = WdfRequestRetrieveOutputBuffer(Request, Length, &virtualAddress, &totalLength);
        rwContext->Read = TRUE;

    } 
    else { //Write

        status = WdfRequestRetrieveInputBuffer(Request, Length, &virtualAddress, &totalLength);
        rwContext->Read = FALSE;
    }

    if (!NT_SUCCESS(status)){
        UsbSamp_DbgPrint(1, ("WdfRequestRetrieveInputBuffer failed\n"));
        goto Exit;
    }

    //
    // The transfer request is for totalLength.
    // We can perform a max of maxTransfersize in each stage.
    //
    maxTransferSize = GetMaxTransferSize(pipe, deviceContext);

    if (totalLength > maxTransferSize) {
        stageLength = maxTransferSize;
    }
    else {
        stageLength = totalLength;   
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Request;
    status = WdfMemoryCreatePreallocated(&objectAttribs,
                                         virtualAddress,
                                         totalLength,
                                         &reqMemory);
    if (!NT_SUCCESS(status)){
        UsbSamp_DbgPrint(1, ("WdfMemoryCreatePreallocated failed\n"));
        goto Exit;
    }

    offset.BufferOffset = 0;
    offset.BufferLength = stageLength;

    //
    // The framework format call validates to make sure that you are reading or
    // writing to the right pipe type, sets the appropriate transfer flags,
    // creates an URB and initializes the request.
    //
    if (RequestType == WdfRequestTypeRead) {

        UsbSamp_DbgPrint(3, ("Read operation\n"));
        status = WdfUsbTargetPipeFormatRequestForRead(pipe,
                                                      Request,
                                                      reqMemory,
                                                      &offset);
    } 
    else {

        UsbSamp_DbgPrint(3, ("Write operation\n"));
        status = WdfUsbTargetPipeFormatRequestForWrite(pipe,
                                                       Request,
                                                       reqMemory,
                                                       &offset);
    }

    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfUsbTargetPipeFormatRequest failed 0x%x\n", status));
        goto Exit;
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    //
    // If the request is for a super speed bulk pipe with streams,
    // configure its urb's PipeHandle with its associated stream's PipeHandle
    //
    if(WdfUsbPipeTypeBulk == pipeInfo.PipeType &&
        pipeContext->StreamConfigured == TRUE) {
        ConfigureStreamPipeHandleForRequest(Request, pipe);
    }
#endif

    WdfRequestSetCompletionRoutine(
                            Request,
                            UsbSamp_EvtReadWriteCompletion,
                            deviceContext);
    //
    // set REQUEST_CONTEXT parameters.
    //
    rwContext->Length  = (ULONG)totalLength - (ULONG)stageLength;
    rwContext->Numxfer = 0;

    //
    // Send the request asynchronously.
    //
    if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS) == FALSE) {
        UsbSamp_DbgPrint(1, ("WdfRequestSend failed\n"));
        status = WdfRequestGetStatus(Request);
        goto Exit;
    }


Exit:
    if (!NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, 0);
    }

    UsbSamp_DbgPrint(3, ("UsbSamp_DispatchReadWrite - ends\n"));

    return;
}

// TODO: Use mouser sample trick to avoid stack recursion

VOID
UsbSamp_EvtReadWriteCompletion(
    _In_ WDFREQUEST                  Request,
    _In_ WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT                  Context
    )
/*++

Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context
    Device - Device handle
    Request - Request handle
    Params - request completion params

Return Value:
    None

--*/
{
    WDFUSBPIPE              pipe;
    ULONG                   stageLength = 0;
    NTSTATUS               status;
    PREQUEST_CONTEXT        rwContext;
    ULONG                   bytesReadWritten;
    WDFMEMORY_OFFSET       offset;
    PCHAR                   operation;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
    PPIPE_CONTEXT           pipeContext;
    WDF_USB_PIPE_INFORMATION   pipeInfo;
    ULONG                   maxTransferSize;
    PDEVICE_CONTEXT         deviceContext;

    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
    rwContext = GetRequestContext(Request);
    deviceContext = Context;

    if (rwContext->Read) {
        operation = "Read";
        bytesReadWritten =  (ULONG)usbCompletionParams->Parameters.PipeRead.Length;
    } 
    else {
        operation = "Write";
        bytesReadWritten =  (ULONG)usbCompletionParams->Parameters.PipeWrite.Length;
    }

    pipe = (WDFUSBPIPE) Target;
    pipeContext = GetPipeContext(pipe);
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe,&pipeInfo);
    status = CompletionParams->IoStatus.Status;

    if (!NT_SUCCESS(status)){
        //
        // Queue a workitem to reset the pipe because the completion could be
        // running at DISPATCH_LEVEL.
        // TODO: preallocate per pipe workitem to avoid allocation failure.
        QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
        goto End;
    }

    rwContext->Numxfer += bytesReadWritten;

    //
    // If there is anything left to transfer.
    //
    if (rwContext->Length == 0) {
        //
        // this is the last transfer
        //
        WdfRequestSetInformation(Request, rwContext->Numxfer);
        goto End;
    }

    //
    // Start another transfer
    //
    UsbSamp_DbgPrint(3, ("Stage next %s transfer...\n", operation));

    //
    // The transfer request is for totalLength. 
    // We can perform a max of maxTransfersize in each stage.
    //
    maxTransferSize = GetMaxTransferSize(pipe, deviceContext);

    if (rwContext->Length > maxTransferSize) {
       stageLength = maxTransferSize;    
    }
    else
    {
       stageLength = rwContext->Length;
    }

    offset.BufferOffset = rwContext->Numxfer;
    offset.BufferLength = stageLength;

    rwContext->Length -= stageLength;

    if (rwContext->Read) {

        status = WdfUsbTargetPipeFormatRequestForRead(
            pipe,
            Request,
            usbCompletionParams->Parameters.PipeRead.Buffer,
            &offset);

    } 
    else {

        status = WdfUsbTargetPipeFormatRequestForWrite(
            pipe,
            Request,
            usbCompletionParams->Parameters.PipeWrite.Buffer,
            &offset);

    }

    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfUsbTargetPipeFormat%sRequest failed 0x%x\n",
                                    operation, status));
        goto End;
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    //
    // If the request is for a super speed bulk pipe with streams,
    // configure its urb's PipeHandle with its associated stream's PipeHandle
    //
    if(WdfUsbPipeTypeBulk == pipeInfo.PipeType &&
        pipeContext->StreamConfigured == TRUE) {
        ConfigureStreamPipeHandleForRequest(Request, pipe);
    }
#endif

    WdfRequestSetCompletionRoutine(
                            Request,
                            UsbSamp_EvtReadWriteCompletion,
                            deviceContext);

    //
    // Send the request asynchronously.
    //
    if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS)) {
        UsbSamp_DbgPrint(1, ("WdfRequestSend for %s failed\n", operation));
        status = WdfRequestGetStatus(Request);
        goto End;
    }

    //
    // Else when the request completes, this completion routine will be
    // called again.
    //
    return;

End:
    //
    // We are here because the request failed or some other call failed.
    // Dump the request context, complete the request and return.
    //
    DbgPrintRWContext(rwContext);

    UsbSamp_DbgPrint(3, ("%s request completed with status 0x%x\n",
                                                    operation, status));

    WdfRequestComplete(Request, status);

    return;
}

#endif

VOID
UsbSamp_EvtReadWriteWorkItem(
    _In_ WDFWORKITEM  WorkItem
    )
{
    PWORKITEM_CONTEXT pItemContext;
    NTSTATUS status;

    UsbSamp_DbgPrint(3, ("ReadWriteWorkItem called\n"));

    pItemContext = GetWorkItemContext(WorkItem);

    status = ResetPipe(pItemContext->Pipe);
    if (!NT_SUCCESS(status)) {

        UsbSamp_DbgPrint(1, ("ResetPipe failed 0x%x\n", status));

        status = ResetDevice(pItemContext->Device);
        if (!NT_SUCCESS(status)){

            UsbSamp_DbgPrint(1, ("ResetDevice failed 0x%x\n", status));
        }
    }

    WdfObjectDelete(WorkItem);

    return;
}

NTSTATUS
QueuePassiveLevelCallback(
    _In_ WDFDEVICE    Device,
    _In_ WDFUSBPIPE   Pipe
    )
/*++

Routine Description:

    This routine is used to queue workitems so that the callback
    functions can be executed at PASSIVE_LEVEL in the conext of
    a system thread.

Arguments:


Return Value:

--*/
{
    NTSTATUS                       status = STATUS_SUCCESS;
    PWORKITEM_CONTEXT               context;
    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_WORKITEM_CONFIG             workitemConfig;
    WDFWORKITEM                     hWorkItem;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, WORKITEM_CONTEXT);
    attributes.ParentObject = Device;

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, UsbSamp_EvtReadWriteWorkItem);

    status = WdfWorkItemCreate( &workitemConfig,
                                &attributes,
                                &hWorkItem);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    context = GetWorkItemContext(hWorkItem);

    context->Device = Device;
    context->Pipe = Pipe;

    //
    // Execute this work item.
    //
    WdfWorkItemEnqueue(hWorkItem);

    return STATUS_SUCCESS;
}

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT rwContext
    )
{
    UNREFERENCED_PARAMETER(rwContext);

    UsbSamp_DbgPrint(3, ("rwContext->UrbMemory       = %p\n",
                         rwContext->UrbMemory));
    UsbSamp_DbgPrint(3, ("rwContext->Mdl             = %p\n",
                         rwContext->Mdl));
    UsbSamp_DbgPrint(3, ("rwContext->Length          = %d\n",
                         rwContext->Length));
    UsbSamp_DbgPrint(3, ("rwContext->Numxfer         = %d\n",
                         rwContext->Numxfer));
    UsbSamp_DbgPrint(3, ("rwContext->VirtualAddress  = %p\n",
                         (PVOID)rwContext->VirtualAddress));
    return;
}

