 /*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    stream.c

Abstract:

    This files contain routines for Streams.

Environment:

    Kernel mode only

Notes:


--*/

#include "private.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetStackCapability)
#if (NTDDI_VERSION >= NTDDI_WIN8)
#pragma alloc_text(PAGE, InitializePipeContextForSuperSpeedBulkPipe)
#endif
#endif

NTSTATUS
GetStackCapability(
    _In_  WDFDEVICE   Device,
    _In_  const GUID* CapabilityType,
    _In_ ULONG       OutputBufferLength,
    _When_(OutputBufferLength == 0, _Pre_null_)
    _When_(OutputBufferLength != 0, _Out_writes_bytes_(OutputBufferLength))
         PUCHAR      OutputBuffer
    )
/*++

Routine Description:

    The helper routine gets stack's capability.

Arguments:

    Device - WDF Device Object

    CapabilityType - Pointer to capability type GUID

    OutputBufferLength - Length of output buffer

    OutPutBuffer - Output buffer

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_CONTEXT  pDevContext;
    NTSTATUS status;

    PAGED_CODE();

    pDevContext = GetDeviceContext(Device);

    //
    // Note: All super speed bulk stream I/O transfers use USBD Handle obtained in
    //       UsbSamp_EvtDeviceAdd. If you call WdfUsbTargetDeviceQueryUsbCapability
    //       method instead of USBD_QueryUsbCapability here, it will not set stream 
    //       capabilites for USBD Handle used by stream transfer in which case 
    //       the open streams request will fail in this example.
    //
    status = USBD_QueryUsbCapability(pDevContext->UsbdHandle,
                                      CapabilityType,
                                      OutputBufferLength,
                                      OutputBuffer,
                                      NULL);
    if (NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("USBD_QueryUsbCapability %x\n", status));
    }

    return status;    
}

#if (NTDDI_VERSION >= NTDDI_WIN8)
                     
NTSTATUS
InitializePipeContextForSuperSpeedBulkPipe(
    _In_ PDEVICE_CONTEXT            DeviceContext,
    _In_ UCHAR                      InterfaceNumber,
    _In_ WDFUSBPIPE                 Pipe
    )
/*++

Routine Description:

    This routine initialize pipe's streams context.

Arguments:

    DeviceContext - pointer to device context

    InterfaceNumber - InterfaceNumber of selected interface

    Pipe - Bullk Pipe

Return Value:

    NTSTATUS

--*/

{
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    PPIPE_CONTEXT               pipeContext;
    PUSB_ENDPOINT_DESCRIPTOR    pEndpointDescriptor;
    PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR pEndpointCompanionDescriptor;

    UCHAR               endpointAddress;
    ULONG               maxStreams;
    ULONG               supportedStreams;
    PUSBSAMP_STREAM_INFO pStreamInfo;
    NTSTATUS            status;
    PURB                pUrb = NULL;
    ULONG               i;

    PAGED_CODE();

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
    pipeContext = GetPipeContext(Pipe);
    pStreamInfo = &pipeContext->StreamInfo;
    
    pStreamInfo->NumberOfStreams = 0;
    pStreamInfo->StreamList = NULL;
 
    pipeContext->StreamConfigured = FALSE;
    
    //
    // Validate that the endpoint/pipe is of type BULK.
    // Streams are only allowed on a SS BULK endpoint.
    // Also ensure that the bus actually supports streams
    // before proceeding
    //
    if (pipeInfo.PipeType != WdfUsbPipeTypeBulk ||
        DeviceContext->NumberOfStreamsSupportedByController == 0) {
        status = STATUS_SUCCESS;
        goto End;
    }

    endpointAddress = pipeInfo.EndpointAddress;

    pEndpointDescriptor = GetEndpointDescriptorForEndpointAddress(
                                DeviceContext,
                                InterfaceNumber,
                                endpointAddress,
                                &pEndpointCompanionDescriptor);

    if (pEndpointDescriptor != NULL &&
        pEndpointCompanionDescriptor != NULL) {
        
        maxStreams = pEndpointCompanionDescriptor->bmAttributes.Bulk.MaxStreams;

        if (maxStreams == 0) {

            supportedStreams = 0;
        
        } else {
        
            supportedStreams = 1 << maxStreams;
        }

    } else {
    
        UsbSamp_DbgPrint(1, ("Endpoint Descriptor or Endpoint Companion Descriptor is NULL.\n"));
        status = STATUS_INVALID_PARAMETER;
        goto End;
    
    }

    if (supportedStreams == 0) {
        status = STATUS_SUCCESS;
        goto End;
    }

    if (supportedStreams > DeviceContext->NumberOfStreamsSupportedByController) {
        supportedStreams = DeviceContext->NumberOfStreamsSupportedByController;
    }

    pStreamInfo->NumberOfStreams = supportedStreams;

    pStreamInfo->StreamList = ExAllocatePoolWithTag(
                                    NonPagedPool,
                                    supportedStreams * sizeof(USBD_STREAM_INFORMATION),
                                    POOL_TAG);

    if (pStreamInfo->StreamList == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    for(i = 0; i < supportedStreams; i++)
    {
        pStreamInfo->StreamList[i].StreamID = i + 1;
    }

    status = USBD_UrbAllocate(DeviceContext->UsbdHandle, &pUrb);

    if (!NT_SUCCESS(status)){
        pUrb = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    pUrb->UrbOpenStaticStreams.Hdr.Length = sizeof(struct _URB_OPEN_STATIC_STREAMS);
    pUrb->UrbOpenStaticStreams.Hdr.Function = URB_FUNCTION_OPEN_STATIC_STREAMS;
    pUrb->UrbOpenStaticStreams.PipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(Pipe);
    pUrb->UrbOpenStaticStreams.NumberOfStreams = pStreamInfo->NumberOfStreams;
    pUrb->UrbOpenStaticStreams.StreamInfoSize = sizeof(USBD_STREAM_INFORMATION);
    pUrb->UrbOpenStaticStreams.Streams = pStreamInfo->StreamList;

    // Send the URB down the stack
    status = WdfUsbTargetPipeSendUrbSynchronously(
                                                  Pipe,
                                                  NULL,
                                                  NULL,
                                                  pUrb                           
                                                  );

    if (NT_SUCCESS(status)) {

         pipeContext->StreamConfigured = TRUE;
  
    }
End:
    if (!NT_SUCCESS(status)) {

        pipeContext->StreamConfigured = FALSE;
        pStreamInfo->NumberOfStreams = 0;

        if (pStreamInfo->StreamList != NULL) {
            ExFreePool(pStreamInfo->StreamList);
            pStreamInfo->StreamList = NULL;
        }

    }

    if(pUrb != NULL) {
        USBD_UrbFree(DeviceContext->UsbdHandle, pUrb);
    }

    return status;
}


USBD_PIPE_HANDLE
GetStreamPipeHandleFromBulkPipe(
    _In_ WDFUSBPIPE                 Pipe
    )
/*++

Routine Description:

    This routine gets a stream USBD_PIPE_HANDLE from a super speed bulk pipe

Arguments:

    Pipe - Bullk Pipe

Return Value:

    A stream's USBD_PIPE_HANDLE

--*/
{
    PPIPE_CONTEXT               pipeContext;

    PUSBSAMP_STREAM_INFO        pStreamInfo;
    USBD_PIPE_HANDLE            streamPipeHandle;
    ULONG                       index;

    pipeContext = GetPipeContext(Pipe);

    if (pipeContext->StreamConfigured == FALSE) 
    {
        streamPipeHandle = NULL;
        goto End;
    }

    pStreamInfo = &pipeContext->StreamInfo;

    if (pStreamInfo->NumberOfStreams == 0 ||
        pStreamInfo->StreamList == NULL)
    {
         streamPipeHandle = NULL;
         goto End;
    }

    //
    // Specify one associate stream's PipeHandle as the super speed bulk endpoint's PipeHandle
    //
    index = 1;

    streamPipeHandle = pStreamInfo->StreamList[index].PipeHandle;

End:
    return streamPipeHandle;

}

VOID
ConfigureStreamPipeHandleForRequest(
    _In_ WDFREQUEST       Request,
    _In_ WDFUSBPIPE       Pipe
    )
/*++

Routine Description:

    The framework has formated request for super speed bulk pipe. 
    For stream transfer, use the associated stream's PipeHandle for transfer.

Arguments:
    
    Request - Read/Write Request.

    Pipe - Bullk Pipe

Return Value:

    NULL

--*/
{
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    PURB                urb;

    //
    // Get the IRP that is associated with the framework request object.
    //
    irp = WdfRequestWdmGetIrp(Request);

    //
    // Obtain the next-lower driver's I/O stack location in the IRP
    //
    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // The framework uses pipe's Pipehandle for data transfer by default.
    // For stream transfer, we should use the associated stream's PipeHandle of 
    // the super speed bulk pipe for transfer. Replace the PipeHandle with 
    // its associated stream's PipeHandle .
    //
    urb = irpSp->Parameters.Others.Argument1;
    urb->UrbBulkOrInterruptTransfer.PipeHandle = GetStreamPipeHandleFromBulkPipe(Pipe);

}


#endif

ULONG
GetMaxTransferSize(
    _In_ WDFUSBPIPE         Pipe,
    _In_ PDEVICE_CONTEXT    DeviceContext
)
/*++

Routine Description:

    This routine returns maximum packet size of a bulk pipe

Arguments:
    
    Pipe - Bullk Pipe

Return Value:

    Maximum Packet Size

--*/
{
    ULONG           maxTransferSize;
    PPIPE_CONTEXT   pipeContext;
    
    pipeContext = GetPipeContext(Pipe);

    if (pipeContext->StreamConfigured == TRUE) {

        //
        // For super speed bulk stream pipe, the max transfer size is
        // MAX_STREAM_VALID_PACKET_SIZE which depends on the implementation
        // of super speed bulk stream endpoint
        //
        maxTransferSize = MAX_STREAM_VALID_PACKET_SIZE;

    }
    else{
        if (DeviceContext->IsDeviceSuperSpeed == TRUE)
        {
            maxTransferSize = MAX_SUPER_SPEED_TRANSFER_SIZE;
        } 
        else if (DeviceContext->IsDeviceHighSpeed == TRUE)
        {
            maxTransferSize = MAX_HIGH_SPEED_TRANSFER_SIZE;
        }
        else
        {         
            maxTransferSize = MAX_FULL_SPEED_TRANSFER_SIZE;
        }
            
    }

    return maxTransferSize;
}




  


