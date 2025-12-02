//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

typedef struct _USB_WRITE_REQ_CONTEXT
{
    WDFMEMORY UrbMemory;
    PURB Urb;
    PMDL Mdl;
    WDFMEMORY LookasideBuffer;
    PBUS_OBJECT BusObject;
    MBB_BUS_SEND_DATA_COMPLETION_CALLBACK Callback;
    MBB_REQUEST_HANDLE RequestHandle;

} USB_WRITE_REQ_CONTEXT, *PUSB_WRITE_REQ_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_WRITE_REQ_CONTEXT, GetWriteRequestContext)

EVT_WDF_USB_READER_COMPLETION_ROUTINE BulkInReadComplete;

EVT_WDF_USB_READERS_FAILED BulkInReadError;

NTSTATUS
GetWriteRequests(WDFUSBDEVICE UsbDevice, WDFREQUEST* ReturnedWriteRequest);

VOID FreeWriteRequest(WDFUSBDEVICE UsbDevice, WDFREQUEST WriteRequest);

NTSTATUS
MbbBusSelectDataAltSetting(__in MBB_BUS_HANDLE BusHandle, __in UCHAR AltSetting)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS SettingParams;
    WDFUSBINTERFACE UsbInterface = NULL;
    WDF_USB_PIPE_INFORMATION pipeInfo;
    WDFUSBPIPE pipe;
    UCHAR ConfiguredPipes = 0;
    UCHAR index = 0;
    NTSTATUS Status;
    NTSTATUS TempStatus;
    WDF_USB_CONTINUOUS_READER_CONFIG ReaderConfig;

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    WdfWaitLockAcquire(usbDeviceContext->PipeStateLock, NULL);

    UsbInterface = WdfUsbTargetDeviceGetInterface(BusObject->WdfUsbDevice, usbDeviceContext->WdfDataInterfaceIndex);

    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&SettingParams, AltSetting == 0 ? 0 : usbDeviceContext->UsbDataInterfaceSetting);

    Status = WdfUsbInterfaceSelectSetting(UsbInterface, NULL, &SettingParams);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(UsbInterface);

    if (((AltSetting == ALT_DATA_SETTING_0) && (ConfiguredPipes != ALT_DATA_SETTING_0_PIPES)) ||
        ((AltSetting == ALT_DATA_SETTING_1) && (ConfiguredPipes != ALT_DATA_SETTING_1_PIPES)))
    {
        Status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    if (AltSetting == ALT_DATA_SETTING_1)
    {
        for (index = 0; index < ConfiguredPipes; index++)
        {

            WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);

            pipe = WdfUsbInterfaceGetConfiguredPipe(
                UsbInterface,
                index, // PipeIndex,
                &pipeInfo);

            if (WdfUsbPipeTypeBulk == pipeInfo.PipeType)
            {

                if (WdfUsbTargetPipeIsInEndpoint(pipe))
                {
                    usbDeviceContext->BulkInputPipeConfigured = TRUE;
                    usbDeviceContext->BulkInputPipe = pipe;
                    usbDeviceContext->BulkInputPipeMaxPacket = pipeInfo.MaximumPacketSize;
                }
                else
                {
                    usbDeviceContext->BulkOutputPipeConfigured = TRUE;
                    usbDeviceContext->BulkOutputPipe = pipe;
                    usbDeviceContext->BulkOutputPipeMaxPacket = pipeInfo.MaximumPacketSize;
                }
            }
            else
            {

                Status = STATUS_UNSUCCESSFUL;

                goto Cleanup;
            }
        }

        if (!(usbDeviceContext->BulkInputPipeConfigured && usbDeviceContext->BulkOutputPipeConfigured))
        {
            Status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }

        //
        //  configure the continous reader on the bulk pipe now, since this can only be done once on a given pipe
        //  unless it is unselected
        //
        WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&ReaderConfig, BulkInReadComplete, BusObject, BusObject->MaxBulkInTransfer);

        if (BusObject->UsbCapDeviceInfo.DeviceInfoHeader.DeviceType == USB_CAP_DEVICE_TYPE_UDE_MBIM)
        {
            ReaderConfig.NumPendingReads = PENDING_BULK_IN_READS_FOR_UDE_MBIM;
        }
        else
        {
            ReaderConfig.NumPendingReads = PENDING_BULK_IN_READS;
        }
        ReaderConfig.HeaderLength = BusObject->BulkInHeaderSize;
        ReaderConfig.EvtUsbTargetPipeReadersFailed = BulkInReadError;

        Status = WdfUsbTargetPipeConfigContinuousReader(usbDeviceContext->BulkInputPipe, &ReaderConfig);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        //  call this now that the pipe is configured
        //
        PreAllocateWriteRequests(BusObject->WdfUsbDevice);
    }
    else
    {
        //
        //  selecting the setting causes the frame work to delete the pipe
        //
        WDFREQUEST WriteRequest = NULL;

        usbDeviceContext->BulkInputPipeConfigured = FALSE;
        usbDeviceContext->BulkInputPipe = NULL;

        usbDeviceContext->BulkOutputPipeConfigured = FALSE;
        usbDeviceContext->BulkOutputPipe = NULL;

        WdfSpinLockAcquire(usbDeviceContext->WriteCollectionLock);

        WriteRequest = (WDFREQUEST)WdfCollectionGetLastItem(usbDeviceContext->WriteRequestCollection);

        //
        //  delete the write request because they are associated with a pipe object
        //
        while (WriteRequest != NULL)
        {
            WdfCollectionRemove(usbDeviceContext->WriteRequestCollection, WriteRequest);

            WdfObjectDelete(WriteRequest);
            WriteRequest = (WDFREQUEST)WdfCollectionGetLastItem(usbDeviceContext->WriteRequestCollection);
        }

        WdfSpinLockRelease(usbDeviceContext->WriteCollectionLock);
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        //  failed to configure the pipes, set back to alt setting 0
        //
        WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&SettingParams, 0);

        TempStatus = WdfUsbInterfaceSelectSetting(UsbInterface, NULL, &SettingParams);

        usbDeviceContext->BulkInputPipeConfigured = FALSE;
        usbDeviceContext->BulkInputPipe = NULL;

        usbDeviceContext->BulkOutputPipeConfigured = FALSE;
        usbDeviceContext->BulkOutputPipe = NULL;
    }

    WdfWaitLockRelease(usbDeviceContext->PipeStateLock);

    return Status;
}

NTSTATUS
MbbUsbDeviceStartDataPipes(__in PUSB_DEVICE_CONTEXT usbDeviceContext)
{
    NTSTATUS Status;

    WdfWaitLockAcquire(usbDeviceContext->PipeStateLock, NULL);

    if (!(usbDeviceContext->BulkInputPipeConfigured && usbDeviceContext->BulkOutputPipeConfigured))
    {
        Status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkInputPipe));

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    usbDeviceContext->BulkInputPipeStarted = TRUE;

    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe));

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    usbDeviceContext->BulkOutputPipeStarted = TRUE;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        //  failed to configure the pipes, set back to alt setting 0
        //
        if (usbDeviceContext->BulkInputPipeStarted)
        {
            WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkInputPipe), WdfIoTargetCancelSentIo);
        }

        if (usbDeviceContext->BulkOutputPipeStarted)
        {
            WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe), WdfIoTargetCancelSentIo);
        }

        usbDeviceContext->BulkInputPipeStarted = FALSE;
        usbDeviceContext->BulkOutputPipeStarted = FALSE;
    }

    WdfWaitLockRelease(usbDeviceContext->PipeStateLock);

    return Status;
}

NTSTATUS
MbbBusStartDataPipes(__in MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    return MbbUsbDeviceStartDataPipes(GetUsbDeviceContext(BusObject->WdfUsbDevice));
}

VOID MbbUsbDeviceStopDataPipes(__in PUSB_DEVICE_CONTEXT usbDeviceContext)
{
    WDFUSBINTERFACE UsbInterface = NULL;

    WdfWaitLockAcquire(usbDeviceContext->PipeStateLock, NULL);

    if (usbDeviceContext->BulkInputPipeConfigured)
    {
        WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkInputPipe), WdfIoTargetCancelSentIo);
    }
    if (usbDeviceContext->BulkOutputPipeConfigured)
    {
        WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe), WdfIoTargetCancelSentIo);
    }

    usbDeviceContext->BulkInputPipeStarted = FALSE;

    usbDeviceContext->BulkOutputPipeStarted = FALSE;

    WdfWaitLockRelease(usbDeviceContext->PipeStateLock);

    return;
}

VOID MbbBusStopDataPipes(__in MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    PUSB_DEVICE_CONTEXT usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    WdfWorkItemFlush(usbDeviceContext->BulkPipeResetWorkitem);

    MbbUsbDeviceStopDataPipes(GetUsbDeviceContext(BusObject->WdfUsbDevice));
}

NTSTATUS
MbbUsbDeviceResetBulkPipe(__in PUSB_DEVICE_CONTEXT usbDeviceContext, __in BOOLEAN Out)

{
    WDFUSBPIPE Pipe = NULL;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    NTSTATUS StatusToReturn = STATUS_SUCCESS;
    NTSTATUS Status;

    WdfWaitLockAcquire(usbDeviceContext->PipeStateLock, NULL);

    if (Out ? usbDeviceContext->BulkOutputPipeConfigured : usbDeviceContext->BulkInputPipeConfigured)
    {

        Pipe = Out ? usbDeviceContext->BulkOutputPipe : usbDeviceContext->BulkInputPipe;

        WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(30));

        Status = WdfUsbTargetPipeAbortSynchronously(Pipe, NULL, &SendOptions);

        if (!NT_SUCCESS(Status))
        {
            if (StatusToReturn == STATUS_SUCCESS)
            {
                StatusToReturn = Status;
            }
        }

        WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(30));

        Status = WdfUsbTargetPipeResetSynchronously(Pipe, NULL, &SendOptions);

        if (!NT_SUCCESS(Status))
        {
            if (StatusToReturn == STATUS_SUCCESS)
            {
                StatusToReturn = Status;
            }
        }
    }
    WdfWaitLockRelease(usbDeviceContext->PipeStateLock);

    return StatusToReturn;
}

NTSTATUS
MbbBusResetBulkPipe(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN Out)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    return MbbUsbDeviceResetBulkPipe(GetUsbDeviceContext(BusObject->WdfUsbDevice), Out);
}

EVT_WDF_REQUEST_COMPLETION_ROUTINE WriteCompletionRoutine;

void WriteCompletionRoutine(__in WDFREQUEST Request, __in WDFIOTARGET Target, __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, __in WDFCONTEXT Context)
/*++

Routine Description

    Request - Handle to the WDFREQUEST which was used to send data to the USB target.
    Target - Handle to the Iotarget to which teh Request was sent. COnceptually this
              is the BULK USB __out pipe(on of  Data or beacon)
    CompletionParams - In case of USB this contains the USB status and amount of bytes transferred
                       

    Context - This is the COntext we set in WdfRequestSend


Arguments:

    

Return Value:

    

--*/

{
    PUSB_WRITE_REQ_CONTEXT WriteContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Target);
    WriteContext = (PUSB_WRITE_REQ_CONTEXT)Context;

    Status = CompletionParams->IoStatus.Status;

    //
    // For usb devices, we should look at the Usb.Completion param.
    //

    if (WriteContext->Callback != NULL)
    {
        (*WriteContext->Callback)(WriteContext->BusObject->ProtocolHandle, WriteContext->RequestHandle, Status, WriteContext->Mdl);
    }

    if (WriteContext->LookasideBuffer != NULL)
    {

        WdfObjectDelete(WriteContext->LookasideBuffer);
    }

    if (WriteContext->UrbMemory != NULL)
    {
        WdfObjectDelete(WriteContext->UrbMemory);
    }

    FreeWriteRequest(WriteContext->BusObject->WdfUsbDevice, Request);
}

NTSTATUS
MbbBusWriteData(__in MBB_BUS_HANDLE BusHandle, __in MBB_REQUEST_HANDLE RequestHandle, __in PMDL Mdl, __in MBB_BUS_SEND_DATA_COMPLETION_CALLBACK Callback)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    PMDL TempMdl = NULL;
    ULONGLONG TotalTransferLength = 0;
    NTSTATUS Status;
    WDFMEMORY urbMemory = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttribs;
    PURB urbBuffer = NULL;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDFREQUEST WriteRequest = NULL;
    USBD_PIPE_HANDLE usbdPipeHandle = NULL;
    PUSB_WRITE_REQ_CONTEXT writeContext = NULL;
    BOOLEAN SentToDevice = FALSE;
    WDFMEMORY BufferMemoryObject = NULL;
    WDFREQUEST WriteRequestZLP = NULL;
    PUSB_WRITE_REQ_CONTEXT writeContextZLP = NULL;
    NTSTATUS ZlpStatus;

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    if (!ExAcquireRundownProtection(&usbDeviceContext->BulkPipeResetRundown))
    {
        return STATUS_NDIS_ADAPTER_NOT_READY;
    }

    //
    //  make sure the length is not too big
    //
    TempMdl = Mdl;

    while (TempMdl != NULL)
    {
        TotalTransferLength += MmGetMdlByteCount(TempMdl);

        TempMdl = TempMdl->Next;
    }

    if (TotalTransferLength > BusObject->NtbParam.dwNtbOutMaxSize)
    {
        //
        //  too big
        //
        Status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    if ((TotalTransferLength < BusObject->NtbParam.dwNtbOutMaxSize) && (TotalTransferLength % usbDeviceContext->BulkOutputPipeMaxPacket == 0))
    {
        //
        //  The transfer length is less than the max out transfer size, and transfer length is a multiple of MaxPacket size.
        //  Need to send a ZLP to the device stack to terminated the transfer.

        Status = GetWriteRequests(BusObject->WdfUsbDevice, &WriteRequestZLP);

        if (!NT_SUCCESS(Status))
        {

            goto Cleanup;
        }

        writeContextZLP = GetWriteRequestContext(WriteRequestZLP);

        writeContextZLP->BusObject = BusObject;
        writeContextZLP->Callback = NULL;
    }

    Status = GetWriteRequests(BusObject->WdfUsbDevice, &WriteRequest);

    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    writeContext = GetWriteRequestContext(WriteRequest);

    if (usbDeviceContext->LookasideList == NULL)
    {

        //
        //  allocate the URB, request is the parent
        //
        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
        objectAttribs.ParentObject = WriteRequest;

        Status = WdfUsbTargetDeviceCreateUrb(BusObject->WdfUsbDevice, &objectAttribs, &urbMemory, &urbBuffer);

        if (!NT_SUCCESS(Status))
        {

            goto Cleanup;
        }

        //
        //  get the USBD pipe handle, to build the URB
        //
        usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(usbDeviceContext->BulkOutputPipe);

        //
        // NOTE : call    UsbBuildInterruptOrBulkTransferRequest otherwise
        //    WdfUsbTargetPipeFormatRequestForUrb    will assert
        //  with *** Assertion failed: Urb->UrbHeader.Length >= sizeof(_URB_HEADER)
        //
        UsbBuildInterruptOrBulkTransferRequest(
            urbBuffer,
            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
            usbdPipeHandle,
            NULL,
            Mdl,
            (ULONG)TotalTransferLength,
            USBD_TRANSFER_DIRECTION_OUT | USBD_SHORT_TRANSFER_OK,
            NULL);

        //
        // By calling  WdfUsbTargetPipeFormatRequestForUrb the frameworks allocate a lot of resources
        // like the underlying IRP for the request and hence it is better to do it at initilization
        // to prevent an avoidable  failure later.
        //
        Status = WdfUsbTargetPipeFormatRequestForUrb(usbDeviceContext->BulkOutputPipe, WriteRequest, urbMemory, NULL);

        if (!NT_SUCCESS(Status))
        {

            goto Cleanup;
        }

        writeContext->UrbMemory = urbMemory;
        writeContext->Urb = (PURB)urbBuffer;
    }
    else
    {
        //
        //  usb stack does not support chained mdl's, double buffer the transfer
        //
        PUCHAR DestBuffer = NULL;
        PUCHAR SourceAddress = NULL;
        size_t BufferSize = 0;
        ULONG SourceLength = 0;
        WDFMEMORY_OFFSET MemoryOffset;

        Status = WdfMemoryCreateFromLookaside(usbDeviceContext->LookasideList, &BufferMemoryObject);

        if (!NT_SUCCESS(Status))
        {

            goto Cleanup;
        }

        DestBuffer = (PUCHAR)WdfMemoryGetBuffer(BufferMemoryObject, &BufferSize);

        TempMdl = Mdl;
        TotalTransferLength = 0;
        while (TempMdl != NULL)
        {
            SourceLength = MmGetMdlByteCount(TempMdl);

            SourceAddress = (PUCHAR)MmGetSystemAddressForMdlSafe(TempMdl, NormalPagePriority | MdlMappingNoExecute);

            if (SourceAddress == NULL)
            {

                Status = STATUS_INSUFFICIENT_RESOURCES;

                goto Cleanup;
            }

            RtlCopyMemory(DestBuffer, SourceAddress, SourceLength);

            DestBuffer += SourceLength;

            TotalTransferLength += SourceLength;

            ASSERT(TotalTransferLength <= BufferSize);

            TempMdl = TempMdl->Next;
        }

        MemoryOffset.BufferOffset = 0;
        MemoryOffset.BufferLength = (ULONG)TotalTransferLength;

        Status = WdfUsbTargetPipeFormatRequestForWrite(usbDeviceContext->BulkOutputPipe, WriteRequest, BufferMemoryObject, &MemoryOffset);
        if (!NT_SUCCESS(Status))
        {

            goto Cleanup;
        }

        writeContext->LookasideBuffer = BufferMemoryObject;
        BufferMemoryObject = NULL;
    }

    //
    // set REQUEST_CONTEXT  parameters.
    //

    writeContext->Mdl = Mdl;
    writeContext->BusObject = BusObject;
    writeContext->Callback = Callback;
    writeContext->RequestHandle = RequestHandle;

    WdfRequestSetCompletionRoutine(WriteRequest, WriteCompletionRoutine, writeContext);

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(30));

    SentToDevice = WdfRequestSend(WriteRequest, WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe), &SendOptions);

    Status = STATUS_PENDING;

    if (!SentToDevice)
    {
        Status = WdfRequestGetStatus(WriteRequest);
    }
    else
    {
        WriteRequest = NULL;
    }

    if (NT_SUCCESS(Status))
    {

        //
        //  actual data write worked, see if we need to send the zlp
        //

        if (WriteRequestZLP != NULL)
        {

            

            Status = WdfUsbTargetPipeFormatRequestForWrite(usbDeviceContext->BulkOutputPipe, WriteRequestZLP, NULL, NULL);
            if (!NT_SUCCESS(Status))
            {

                goto Cleanup;
            }

            WdfRequestSetCompletionRoutine(WriteRequestZLP, WriteCompletionRoutine, writeContextZLP);

            WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(30));

            SentToDevice = WdfRequestSend(WriteRequestZLP, WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe), &SendOptions);

            ZlpStatus = STATUS_PENDING;

            if (!SentToDevice)
            {

                ZlpStatus = WdfRequestGetStatus(WriteRequestZLP);
            }
            else
            {
                WriteRequestZLP = NULL;
            }
        }
    }

Cleanup:

    ExReleaseRundownProtection(&usbDeviceContext->BulkPipeResetRundown);

    if (WriteRequest != NULL)
    {
        if (NULL != writeContext)
        {
            if (NULL != writeContext->LookasideBuffer)
            {
                WdfObjectDelete(writeContext->LookasideBuffer);
            }
            if (NULL != writeContext->UrbMemory)
            {
                WdfObjectDelete(writeContext->UrbMemory);
            }
        }

        FreeWriteRequest(BusObject->WdfUsbDevice, WriteRequest);
        WriteRequest = NULL;
    }

    if (WriteRequestZLP != NULL)
    {
        Status = ZlpStatus;
        FreeWriteRequest(BusObject->WdfUsbDevice, WriteRequestZLP);
        WriteRequestZLP = NULL;
    }

    return Status;
}

VOID BulkInReadComplete(__in WDFUSBPIPE Pipe, __in WDFMEMORY Memory, __in size_t NumBytesTransferred, __in WDFCONTEXT Context)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)Context;
    PUCHAR Buffer = NULL;
    PMDL Mdl = NULL;
    PUCHAR DataBuffer = NULL;

    if (NumBytesTransferred > 0)
    {
        //
        //  actaully got some data
        //
        Buffer = (PUCHAR)WdfMemoryGetBuffer(Memory, NULL);

        //
        //  the header at the front is where we will put the MDL
        //
        Mdl = (PMDL)Buffer;

        //
        //  the actual payload starts after the header/mdl
        //
        DataBuffer = Buffer + BusObject->BulkInHeaderSize;

        ASSERT(((ULONG_PTR)DataBuffer & 0x7) == 0);

        //
        //  bytes transfered does not include the header
        //
        MmInitializeMdl(Mdl, DataBuffer, NumBytesTransferred);

        MmBuildMdlForNonPagedPool(Mdl);

        ASSERT(BusObject->BulkInHeaderSize >= MmSizeOfMdl(Mdl, NumBytesTransferred));

        WdfObjectReference(Memory);

        (*BusObject->ReceiveDataCallback)(BusObject->ProtocolHandle, Memory, Mdl);
    }

    return;
}

BOOLEAN
BulkInReadError(__in WDFUSBPIPE Pipe, __in NTSTATUS Status, __in USBD_STATUS UsbdStatus)

{
    return TRUE;
}

VOID MbbBusReturnReceiveBuffer(__in MBB_BUS_HANDLE BusHandle, __in MBB_RECEIVE_CONTEXT ReceiveContext, __in PMDL Mdl)

{
    WDFMEMORY Memory = (WDFMEMORY)ReceiveContext;

    WdfObjectDereference(Memory);

    return;
}

NTSTATUS
CreateWriteRequest(WDFUSBDEVICE UsbDevice, WDFREQUEST* ReturnedWriteRequest)

{

    WDFREQUEST WriteRequest = NULL;
    PUSB_WRITE_REQ_CONTEXT writeContext = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttribs;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    *ReturnedWriteRequest = NULL;

    usbDeviceContext = GetUsbDeviceContext(UsbDevice);

    if (usbDeviceContext->BulkOutputPipe == NULL)
    {
        goto Cleanup;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttribs, USB_WRITE_REQ_CONTEXT);
    objectAttribs.ParentObject = UsbDevice;

    //
    //  create the request
    //
    Status = WdfRequestCreate(&objectAttribs, WdfUsbTargetPipeGetIoTarget(usbDeviceContext->BulkOutputPipe), &WriteRequest);

    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    writeContext = GetWriteRequestContext(WriteRequest);
    RtlZeroMemory(writeContext, sizeof(*writeContext));

    //
    // Preallocate the request timer to prevent the request from failing while trying to send it.
    //
    Status = WdfRequestAllocateTimer(WriteRequest);

    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    Status = WdfUsbTargetPipeFormatRequestForWrite(usbDeviceContext->BulkOutputPipe, WriteRequest, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    *ReturnedWriteRequest = WriteRequest;
    WriteRequest = NULL;

Cleanup:

    if (WriteRequest != NULL)
    {
        WdfObjectDelete(WriteRequest);
        WriteRequest = NULL;
    }

    return Status;
}

NTSTATUS
PreAllocateWriteRequests(WDFUSBDEVICE UsbDevice)

{
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    WDFREQUEST WriteRequest = NULL;
    NTSTATUS Status;
    ULONG i = 0;

    usbDeviceContext = GetUsbDeviceContext(UsbDevice);

    for (i = 0; i < MAX_PREALLOCATED_WRITE_REQUESTS; i++)
    {
        Status = CreateWriteRequest(UsbDevice, &WriteRequest);

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = WdfCollectionAdd(usbDeviceContext->WriteRequestCollection, WriteRequest);

        if (!NT_SUCCESS(Status))
        {
            WdfObjectDelete(WriteRequest);
            WriteRequest = NULL;
            break;
        }
    }

    return Status;
}

NTSTATUS
GetWriteRequests(WDFUSBDEVICE UsbDevice, WDFREQUEST* ReturnedWriteRequest)
{

    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    PUSB_WRITE_REQ_CONTEXT writeContext = NULL;

    usbDeviceContext = GetUsbDeviceContext(UsbDevice);

    *ReturnedWriteRequest = NULL;

    if (usbDeviceContext->BulkOutputPipe == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    WdfSpinLockAcquire(usbDeviceContext->WriteCollectionLock);

    *ReturnedWriteRequest = (WDFREQUEST)WdfCollectionGetLastItem(usbDeviceContext->WriteRequestCollection);

    if (*ReturnedWriteRequest == NULL)
    {
        //
        //  the collection is empty, try creating a new one
        //
        WdfSpinLockRelease(usbDeviceContext->WriteCollectionLock);

        Status = CreateWriteRequest(UsbDevice, ReturnedWriteRequest);
    }
    else
    {

        WdfCollectionRemove(usbDeviceContext->WriteRequestCollection, *ReturnedWriteRequest);

        WdfSpinLockRelease(usbDeviceContext->WriteCollectionLock);

        WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, 0, STATUS_SUCCESS);

        WdfRequestReuse(*ReturnedWriteRequest, &ReuseParams);
    }

    if (NT_SUCCESS(Status))
    {
        writeContext = GetWriteRequestContext(*ReturnedWriteRequest);
        RtlZeroMemory(writeContext, sizeof(*writeContext));
    }

    return Status;
}

VOID FreeWriteRequest(WDFUSBDEVICE UsbDevice, WDFREQUEST WriteRequest)

{
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RequestCount = 0;

    usbDeviceContext = GetUsbDeviceContext(UsbDevice);

    WdfSpinLockAcquire(usbDeviceContext->WriteCollectionLock);

    RequestCount = WdfCollectionGetCount(usbDeviceContext->WriteRequestCollection);

    if (RequestCount < MAX_PREALLOCATED_WRITE_REQUESTS * 2)
    {
        //
        //  put it back in the collection
        //
        Status = WdfCollectionAdd(usbDeviceContext->WriteRequestCollection, WriteRequest);

        if (NT_SUCCESS(Status))
        {
            WriteRequest = NULL;
        }
    }

    WdfSpinLockRelease(usbDeviceContext->WriteCollectionLock);

    if (WriteRequest != NULL)
    {
        WdfObjectDelete(WriteRequest);
        WriteRequest = NULL;
    }

    return;
}

VOID MbbBusResetDataPipes(_In_ MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    PUSB_DEVICE_CONTEXT usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    if (!InterlockedCompareExchange(&usbDeviceContext->BulkPipeResetFlag, TRUE, FALSE))
    {
        WdfWorkItemEnqueue(usbDeviceContext->BulkPipeResetWorkitem);
    }
    else
    {
    }
}

VOID MbbUsbDeviceCyclePort(_In_ PUSB_DEVICE_CONTEXT usbDeviceContext)
{
    NTSTATUS Status = WdfUsbTargetDeviceCyclePortSynchronously(usbDeviceContext->BusObject->WdfUsbDevice);
    if (!NT_SUCCESS(Status))
    {
    }
}

void ResetDataPipeWorkItem(_In_ WDFWORKITEM WorkItem)
{
    PUSB_DEVICE_CONTEXT usbDeviceContext = GetUsbDeviceContext(WdfWorkItemGetParentObject(WorkItem));

    ExWaitForRundownProtectionRelease(&usbDeviceContext->BulkPipeResetRundown);
    ExRundownCompleted(&usbDeviceContext->BulkPipeResetRundown);

    MbbUsbDeviceStopDataPipes(usbDeviceContext);

    MbbUsbDeviceResetBulkPipe(usbDeviceContext, TRUE);

    MbbUsbDeviceCyclePort(usbDeviceContext);

    MbbUsbDeviceStartDataPipes(usbDeviceContext);

    ExReInitializeRundownProtection(&usbDeviceContext->BulkPipeResetRundown);

    InterlockedExchange(&usbDeviceContext->BulkPipeResetFlag, FALSE);
}
