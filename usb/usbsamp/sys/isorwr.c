/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    isorwr.c

Abstract:

    This file has dispatch routines for read and write.

Environment:

    Kernel mode

Notes:

--*/

#include "private.h"
       
VOID
ReadWriteIsochEndPoints(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            Length,
    _In_ WDF_REQUEST_TYPE RequestType
    )
/*++

Routine Description:

    This routine does some validation and invokes appropriate function to perform Isoch transfer

--*/
{
    NTSTATUS                    status;
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    PFILE_CONTEXT               fileContext;
    WDFUSBPIPE                  pipe;
    PDEVICE_CONTEXT             deviceContext;
    PREQUEST_CONTEXT            rwContext;

    UNREFERENCED_PARAMETER(Length);

    UsbSamp_DbgPrint(3, ("ReadWriteIsochEndPoints - begins\n"));

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if ((WdfUsbPipeTypeIsochronous != pipeInfo.PipeType)) {
        UsbSamp_DbgPrint(1, ("Pipe type is not Isochronous\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit;

    }

    if (RequestType == WdfRequestTypeRead && WdfUsbTargetPipeIsInEndpoint(pipe) == FALSE) {
        UsbSamp_DbgPrint(1, ("Invalid pipe - not an input pipe\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;    
    }

    if (RequestType == WdfRequestTypeWrite && WdfUsbTargetPipeIsOutEndpoint(pipe) == FALSE) {
        UsbSamp_DbgPrint(1, ("Invalid pipe - not an output pipe\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;    
    }

    deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
    rwContext = GetRequestContext(Request);

    if (RequestType == WdfRequestTypeRead) {       
        rwContext->Read = TRUE;
        status = WdfRequestForwardToIoQueue(Request, deviceContext->IsochReadQueue);
    } 
    else {
        rwContext->Read = FALSE;
        status = WdfRequestForwardToIoQueue(Request, deviceContext->IsochWriteQueue);
    }

    if (!NT_SUCCESS(status)){
       UsbSamp_DbgPrint(1, ("WdfRequestForwardToIoQueue failed with status 0x%x\n", status));
       goto Exit;
    }

    return;

Exit:
    WdfRequestCompleteWithInformation(Request, status, 0);
    return;
}

VOID
UsbSamp_EvtIoQueueReadyNotification(
    WDFQUEUE    Queue,
    WDFCONTEXT  Context
    )
/*++

Routine Description:

    This function is called when the WDF queue transitions from 0 to 1 requests in the
    queue. Because we are using queue level synchronization, the framework will not
    call this routine concurrently if another request is received while this routine is
    handling the previously retrieved request. That way we can compute the StartFrame
    and dispatch requests without being concerned about two requests racing through
    this routine and using StartFrame numbers that overlap each other.

    This is common routine for both read and write requests.

--*/
{
    NTSTATUS                status;
    WDF_REQUEST_PARAMETERS  requestParams;
    WDFREQUEST              request;
    PREQUEST_CONTEXT        rwContext;

    do {

        status = WdfIoQueueRetrieveNextRequest(Queue, &request);
        
        if (!NT_SUCCESS(status)) {
            return;
        }

        rwContext = GetRequestContext(request);

        WDF_REQUEST_PARAMETERS_INIT(&requestParams);
        WdfRequestGetParameters(request, &requestParams);

        if (rwContext->Read) {
            PerformIsochTransfer((PDEVICE_CONTEXT)Context,
                                  request,
                                  (ULONG)requestParams.Parameters.Read.Length);
        } 
        else {
            PerformIsochTransfer((PDEVICE_CONTEXT)Context,
                                  request,
                                  (ULONG)requestParams.Parameters.Write.Length);

        }

    } while (status == STATUS_SUCCESS);

    return;
}


VOID
PerformIsochTransfer(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            TotalLength
    )
/*++

Routine Description:

    Common routine to perform isoch transfer to fullspeed and highspeed device.

Arguments:

    Device - Device handle
    Queue - Queue the request is delivered from
    Request - Read/Write Request received from the user app.
    TotalLength - Length of the user buffer.
    Request - Read or Write request

Return Value:

    VOID
--*/
{
    ULONG                       numberOfPackets;
    NTSTATUS                    status;
    PREQUEST_CONTEXT            rwContext;
    WDFUSBPIPE                  pipe;
    PFILE_CONTEXT               fileContext;
    WDF_OBJECT_ATTRIBUTES       attributes;
    ULONG                       j;
    USBD_PIPE_HANDLE            usbdPipeHandle;
    PMDL                        requestMdl;
    WDFMEMORY                   urbMemory;
    PURB                        urb;
    size_t                      urbSize;
    ULONG                       offset;
    ULONG                       frameNumber, numberOfFrames;
    PPIPE_CONTEXT               pipeContext;

    rwContext = GetRequestContext(Request);

    UsbSamp_DbgPrint(3, ("PerformIsochTransfer %s for Length %d - begins\n",
                                rwContext->Read ? "Read":"Write", TotalLength));
    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    pipeContext = GetPipeContext(pipe);

    if ((TotalLength % pipeContext->TransferSizePerFrame) != 0) {
        UsbSamp_DbgPrint(1, ("The transfer must evenly start and end on whole frame boundaries.\n"));
        UsbSamp_DbgPrint(1, ("Transfer length should be multiples of %d\n", pipeContext->TransferSizePerFrame));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (DeviceContext->IsDeviceSuperSpeed) {
        
        numberOfFrames  = TotalLength / pipeContext->TransferSizePerFrame;
        numberOfPackets = TotalLength / pipeContext->TransferSizePerMicroframe;
        
        //
        // Then make sure the buffer doesn't exceed maximum allowed packets per transfer 
        //

        if (numberOfPackets > MAX_SUPPORTED_PACKETS_FOR_SUPER_SPEED) {
            UsbSamp_DbgPrint(1, ("NumberOfPackets %d required to transfer exceeds the limit %d\n", 
                               numberOfPackets, MAX_SUPPORTED_PACKETS_FOR_SUPER_SPEED));
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        UsbSamp_DbgPrint(3, ("Will send %d packets of %d bytes in %d frames\n",
                        numberOfPackets, pipeContext->TransferSizePerMicroframe, numberOfFrames));

    } else if (DeviceContext->IsDeviceHighSpeed) {

        
        numberOfFrames  = TotalLength / pipeContext->TransferSizePerFrame;
        numberOfPackets = TotalLength / pipeContext->TransferSizePerMicroframe;
        
        //
        // Then make sure the buffer doesn't exceed maximum allowed packets per transfer 
        //

        if (numberOfPackets > MAX_SUPPORTED_PACKETS_FOR_HIGH_SPEED) {
            UsbSamp_DbgPrint(1, ("NumberOfPackets %d required to transfer exceeds the limit %d\n", 
                               numberOfPackets, MAX_SUPPORTED_PACKETS_FOR_HIGH_SPEED));
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        UsbSamp_DbgPrint(3, ("Will send %d packets of %d bytes in %d frames\n",
                        numberOfPackets, pipeContext->TransferSizePerMicroframe, numberOfFrames));

    }
    else {

        numberOfPackets = TotalLength / pipeContext->TransferSizePerFrame;
        numberOfFrames = numberOfPackets;

        //
        // Then make sure the buffer doesn't exceed maximum allowed packets per transfer 
        //

        if (numberOfPackets > MAX_SUPPORTED_PACKETS_FOR_FULL_SPEED) {
            UsbSamp_DbgPrint(1, ("NumberOfPackets %d required to transfer exceeds the limit %d\n", 
                               numberOfPackets, MAX_SUPPORTED_PACKETS_FOR_FULL_SPEED));
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        UsbSamp_DbgPrint(3, ("Will send %d packets of %d bytes in %d frames\n",
                    numberOfPackets, pipeContext->TransferSizePerFrame, numberOfFrames));
    }

    if (rwContext->Read == TRUE) {
        status = WdfRequestRetrieveOutputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveOutputWdmMdl failed %x\n", status));
            goto Exit;
        }
    } 
    else {
        status = WdfRequestRetrieveInputWdmMdl(Request, &requestMdl);
        if (!NT_SUCCESS(status)){
            UsbSamp_DbgPrint(1, ("WdfRequestRetrieveInputWdmMdl failed %x\n", status));
            goto Exit;
        }
    }

    urbSize = GET_ISO_URB_SIZE(numberOfPackets);

    //
    // Allocate memory for URB.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Request;

    status = WdfUsbTargetDeviceCreateIsochUrb(DeviceContext->WdfUsbTargetDevice,
                        &attributes,
                        numberOfPackets,
                        &urbMemory,
                        NULL);

    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfUsbTargetDeviceCreateIsochUrb failed 0x%x\n", status));
        goto Exit;
    }

    urb = WdfMemoryGetBuffer(urbMemory, NULL);

    usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(pipe);
    urb->UrbIsochronousTransfer.Hdr.Length = (USHORT) urbSize;
    urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    urb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;

    if (rwContext->Read) {
        urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN;
    }
    else {
        urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    }

    urb->UrbIsochronousTransfer.TransferBufferLength = TotalLength;
    urb->UrbIsochronousTransfer.TransferBufferMDL = requestMdl;
    urb->UrbIsochronousTransfer.NumberOfPackets = numberOfPackets;
    urb->UrbIsochronousTransfer.UrbLink = NULL;   
        
    //
    // Set the offsets for every packet for reads/writes
    //
    offset = 0;

    for (j = 0; j < numberOfPackets; j++) {

        if (DeviceContext->IsDeviceHighSpeed ||
            DeviceContext->IsDeviceSuperSpeed) {
            urb->UrbIsochronousTransfer.IsoPacket[j].Offset = j * pipeContext->TransferSizePerMicroframe;
        } 
        else {
            urb->UrbIsochronousTransfer.IsoPacket[j].Offset = j * pipeContext->TransferSizePerFrame;
        }

        //
        // Length is a return value on Isoch IN.  It is ignored on Isoch OUT.
        //
        urb->UrbIsochronousTransfer.IsoPacket[j].Length = 0;
        urb->UrbIsochronousTransfer.IsoPacket[j].Status = 0;

        UsbSamp_DbgPrint(3, ("IsoPacket[%d].Offset = %X IsoPacket[%d].Length = %X\n",
                            j, urb->UrbIsochronousTransfer.IsoPacket[j].Offset,
                            j, urb->UrbIsochronousTransfer.IsoPacket[j].Length));
    }

    //
    // Calculate the StartFrame number:
    // When the client driver sets the ASAP flag, it basically guarantees that 
    // it will make data available to the host controller (HC) and that the  
    // HC should transfer it in the next transfer frame for the endpoint.
    // (The HC maintains a next transfer frame  state variable for each endpoint). 
    // If the data does not get to the HC  fast enough, the USBD_ISO_PACKET_DESCRIPTOR - 
    // Status is USBD_STATUS_BAD_START_FRAME on uhci. On ohci it is 0xC000000E.
    //

    //urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;

    //
    // Instead of using ASAP, we will explicitly set start frame since we cannot control the
    // response time of application that's sending request to us.
    //
    status = WdfUsbTargetDeviceRetrieveCurrentFrameNumber(DeviceContext->WdfUsbTargetDevice, 
                                                &frameNumber);
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("Failed to get frame number urb\n"));
        goto Exit;
    }

    if (frameNumber < pipeContext->NextFrameNumber) {
        //
        // Controller hasn't finished sending perivously scheduled request. So we will use
        // the NextFrameNumber that we calculated for this one.
        //
        urb->UrbIsochronousTransfer.StartFrame = pipeContext->NextFrameNumber;
    } 
    else {
        //
        // Controller frame number has advanced beyond the NextFrameNumber we calculated.
        //
        urb->UrbIsochronousTransfer.StartFrame = frameNumber;
    }

    //
    // Let us add a small latency to account for the time delay in reaching the controller 
    // from here.
    //
    urb->UrbIsochronousTransfer.StartFrame += DISPATCH_LATENCY_IN_MS;

    //
    // Calculate the NextFrameNumber. 
    //
    pipeContext->NextFrameNumber = urb->UrbIsochronousTransfer.StartFrame + numberOfFrames; 

    //
    // Associate the URB with the request.
    //
    status = WdfUsbTargetPipeFormatRequestForUrb(pipe,
                                      Request,
                                      urbMemory,
                                      NULL );
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("Failed to format requset for urb\n"));
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request,
                                   UsbSamp_EvtIsoRequestCompletionRoutine,
                                   rwContext);
 
    rwContext->UrbMemory       = urbMemory;
    rwContext->Mdl             = requestMdl;
    rwContext->Length          = TotalLength;
    rwContext->Numxfer         = 0;
    rwContext->VirtualAddress  = (ULONG_PTR)MmGetMdlVirtualAddress(requestMdl);        

    if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(Request);
        UsbSamp_DbgPrint(1, ("WdfRequestSend failed with status code 0x%x\n", status));
        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, 0);
    }

    UsbSamp_DbgPrint(3, ("PerformHighSpeedIsochTransfer -- ends status 0x%x\n", status));
    return;
}


VOID
UsbSamp_EvtIsoRequestCompletionRoutine(
    _In_ WDFREQUEST                  Request,
    _In_ WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT                  Context
    )
/*++

Routine Description:

    Completion Routine

Arguments:

    Context - Driver supplied context
    Target - Target handle
    Request - Request handle
    Params - request completion params


Return Value:

    VOID

--*/
{
    PURB                    urb;
    NTSTATUS                status;
    ULONG                   length, i, totalPacketLenght;
    PREQUEST_CONTEXT        rwContext;

    UNREFERENCED_PARAMETER(Target);

    UsbSamp_DbgPrint(3, ("UsbSampEvtIsoRequestCompletionRoutine - begins\n"));

    rwContext = (PREQUEST_CONTEXT)Context;

    urb = (PURB) WdfMemoryGetBuffer(rwContext->UrbMemory, NULL);

    length = urb->UrbIsochronousTransfer.TransferBufferLength;

    status = CompletionParams->IoStatus.Status;

    totalPacketLenght = 0;

    for (i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

        UsbSamp_DbgPrint(3, ("IsoPacket[%d].Length = %X IsoPacket[%d].Status = %X\n",
                            i,
                            urb->UrbIsochronousTransfer.IsoPacket[i].Length,
                            i,
                            urb->UrbIsochronousTransfer.IsoPacket[i].Status));

        totalPacketLenght += urb->UrbIsochronousTransfer.IsoPacket[i].Length;
    }

    if (NT_SUCCESS(status) && USBD_SUCCESS(urb->UrbHeader.Status)) {
        //
        // For iosch out, IsoPacket[].Length field is not updated by the USB stack.
        //
        if (rwContext->Read == TRUE) {
            NT_ASSERT(totalPacketLenght == length);
        }

        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, length);

        UsbSamp_DbgPrint(3, ("Request completed with success, TransferBufferLength = %d, TotalPacketLength = %d\n", 
                                length, totalPacketLenght));
    }
    else {

        UsbSamp_DbgPrint(1, ("Read or write irp failed with NTSTATUS 0x%x, USBD_STATUS 0x%x\n", 
                           status, urb->UrbHeader.Status));

        WdfRequestCompleteWithInformation(Request, status, 0); 
    }

    UsbSamp_DbgPrint(3, ("UsbSampEvtIsoRequestCompletionRoutine - ends\n"));

    return;
}

VOID
UsbSamp_EvtIoStop(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            ActionFlags
    )
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

Return Value:
    None

--*/
{
    UNREFERENCED_PARAMETER(Queue);

    if (ActionFlags & WdfRequestStopActionSuspend ) {
        WdfRequestStopAcknowledge(Request, FALSE); // Don't requeue
    } 
    else if (ActionFlags & WdfRequestStopActionPurge) {
        WdfRequestCancelSentRequest(Request);
    }

    return;
}
