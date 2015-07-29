/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Queue.c

Abstract:

    This file contains dispatch routines for create,
    close, device-control, read & write.

Environment:

    Kernel mode

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UsbSamp_EvtDeviceFileCreate)
#pragma alloc_text(PAGE, UsbSamp_EvtIoDeviceControl)
#pragma alloc_text(PAGE, UsbSamp_EvtIoRead)
#pragma alloc_text(PAGE, UsbSamp_EvtIoWrite)
#pragma alloc_text(PAGE, GetPipeFromName)
#pragma alloc_text(PAGE, ResetPipe)
#pragma alloc_text(PAGE, ResetDevice)
#endif


VOID
UsbSamp_EvtDeviceFileCreate(
    _In_ WDFDEVICE            Device,
    _In_ WDFREQUEST           Request,
    _In_ WDFFILEOBJECT        FileObject
    )
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing a file.
    This callback is called synchronously, in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - copy of the create IO_STACK_LOCATION

Return Value:

   NT status code

--*/
{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PUNICODE_STRING             fileName;
    PFILE_CONTEXT               pFileContext;
    PDEVICE_CONTEXT             pDevContext;
    WDFUSBPIPE                  pipe;

    UsbSamp_DbgPrint(3, ("EvtDeviceFileCreate - begins\n"));

    PAGED_CODE();

    //
    // initialize variables
    //
    pDevContext = GetDeviceContext(Device);
    pFileContext = GetFileContext(FileObject);


    fileName = WdfFileObjectGetFileName(FileObject);

    if (0 == fileName->Length) {
        //
        // opening a device as opposed to pipe.
        //
        status = STATUS_SUCCESS;
    }
    else {
        pipe = GetPipeFromName(pDevContext, fileName);

        if (pipe != NULL) {
            //
            // found a match
            //
            pFileContext->Pipe = pipe;

            WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

            status = STATUS_SUCCESS;
        } 
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    WdfRequestComplete(Request, status);

    UsbSamp_DbgPrint(3, ("EvtDeviceFileCreate - ends\n"));

    return;
}

VOID
UsbSamp_EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
Return Value:

    VOID

--*/
{
    WDFDEVICE          device;
    PVOID              ioBuffer;
    size_t             bufLength;
    NTSTATUS           status;
    PDEVICE_CONTEXT    pDevContext;
    PFILE_CONTEXT      pFileContext;
    ULONG              length = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    UsbSamp_DbgPrint(3, ("Entered UsbSamp_DispatchDevCtrl\n"));

    PAGED_CODE();

    //
    // initialize variables
    //
    device = WdfIoQueueGetDevice(Queue);
    pDevContext = GetDeviceContext(device);

    switch(IoControlCode) {

    case IOCTL_USBSAMP_RESET_PIPE:

        pFileContext = GetFileContext(WdfRequestGetFileObject(Request));

        if (pFileContext->Pipe == NULL) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = ResetPipe(pFileContext->Pipe);
        }

        break;

    case IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR:


        if (pDevContext->UsbConfigurationDescriptor) {

            length = pDevContext->UsbConfigurationDescriptor->wTotalLength;

            status = WdfRequestRetrieveOutputBuffer(Request, length, &ioBuffer, &bufLength);
            if (!NT_SUCCESS(status)){
                UsbSamp_DbgPrint(1, ("WdfRequestRetrieveInputBuffer failed\n"));
                break;
            }

            RtlCopyMemory(ioBuffer,
                          pDevContext->UsbConfigurationDescriptor,
                          length);

            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INVALID_DEVICE_STATE;
        }

        break;

    case IOCTL_USBSAMP_RESET_DEVICE:

        status = ResetDevice(device);
        break;

    default :
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, length);

    UsbSamp_DbgPrint(3, ("Exit UsbSamp_DispatchDevCtrl\n"));

    return;
}

VOID
UsbSamp_EvtIoRead(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ size_t           Length
    )
/*++

Routine Description:

    Called by the framework when it receives Read requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;

    PAGED_CODE();

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    if (pipe == NULL) {
        UsbSamp_DbgPrint(1, ("pipe handle is NULL\n"));
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if ((WdfUsbPipeTypeBulk == pipeInfo.PipeType) ||
       (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)) {

        ReadWriteBulkEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeRead);
        return;

    } 
    else if (WdfUsbPipeTypeIsochronous == pipeInfo.PipeType){

#if !defined(BUFFERED_READ_WRITE) // if doing DIRECT_IO
        ReadWriteIsochEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeRead);
        return;
#endif

    }

    UsbSamp_DbgPrint(1, ("ISO transfer is not supported for buffered I/O transfer\n"));
    WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, 0);

    return;
}

VOID
UsbSamp_EvtIoWrite(
    _In_ WDFQUEUE         Queue,
    _In_ WDFREQUEST       Request,
    _In_ size_t           Length
    )
/*++

Routine Description:

    Called by the framework when it receives Write requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;

    PAGED_CODE();

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    if (pipe == NULL) {
        UsbSamp_DbgPrint(1, ("pipe handle is NULL\n"));
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if ((WdfUsbPipeTypeBulk == pipeInfo.PipeType) ||
       (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)) {

        ReadWriteBulkEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeWrite);
        return;

    } 
    else if (WdfUsbPipeTypeIsochronous == pipeInfo.PipeType){

#if !defined(BUFFERED_READ_WRITE) // if doing DIRECT_IO
        ReadWriteIsochEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeWrite);
        return;
#endif

    }

    UsbSamp_DbgPrint(1, ("ISO transfer is not supported for buffered I/O transfer\n"));
    WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, 0);

    return;
}

WDFUSBPIPE
GetPipeFromName(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ PUNICODE_STRING FileName
    )
/*++

Routine Description:

    This routine will pass the string pipe name and
    fetch the pipe handle.

Arguments:

    DeviceContext - pointer to Device Context

    FileName - string pipe name

Return Value:

    The device extension maintains a pipe context for
    the pipes on 82930 board.

--*/
{
    LONG                  ix;
    ULONG                 uval;
    ULONG                 nameLength;
    ULONG                 umultiplier;
    WDFUSBPIPE            pipe = NULL;

    PAGED_CODE();

    //
    // typedef WCHAR *PWSTR;
    //
    nameLength = (FileName->Length / sizeof(WCHAR));

    UsbSamp_DbgPrint(3, ("UsbSamp_PipeWithName - begins\n"));

    if (nameLength != 0) {

        UsbSamp_DbgPrint(3, ("Filename = %wZ nameLength = %d\n", FileName, nameLength));

        //
        // Parse the pipe#
        //
        ix = nameLength - 1;

        // if last char isn't digit, decrement it.
        while((ix > -1) &&
              ((FileName->Buffer[ix] < (WCHAR) '0')  ||
               (FileName->Buffer[ix] > (WCHAR) '9')))             {

            ix--;
        }

        if (ix > -1) {

            uval = 0;
            umultiplier = 1;

            // traversing least to most significant digits.

            while((ix > -1) &&
                  (FileName->Buffer[ix] >= (WCHAR) '0') &&
                  (FileName->Buffer[ix] <= (WCHAR) '9'))          {

                uval += (umultiplier *
                         (ULONG) (FileName->Buffer[ix] - (WCHAR) '0'));

                ix--;
                umultiplier *= 10;
            }
            pipe = WdfUsbInterfaceGetConfiguredPipe(
                DeviceContext->UsbInterface,
                (UCHAR)uval, //PipeIndex,
                NULL
                );

        }
    }

    UsbSamp_DbgPrint(3, ("UsbSamp_PipeWithName - ends\n"));

    return pipe;
}

NTSTATUS
ResetPipe(
    _In_ WDFUSBPIPE Pipe
    )
/*++

Routine Description:

    This routine resets the pipe.

Arguments:

    Pipe - framework pipe handle

Return Value:

    NT status value

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    //  This routine synchronously submits a URB_FUNCTION_RESET_PIPE
    // request down the stack.
    //
    status = WdfUsbTargetPipeResetSynchronously(Pipe,
                                WDF_NO_HANDLE, // WDFREQUEST
                                NULL  // PWDF_REQUEST_SEND_OPTIONS
                                );

    if (NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(3, ("ResetPipe - success\n"));
        status = STATUS_SUCCESS;
    }
    else {
        UsbSamp_DbgPrint(1, ("ResetPipe - failed\n"));
    }

    return status;
}

VOID
StopAllPipes(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    UCHAR count,i;

    count = DeviceContext->NumberConfiguredPipes;
    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;
        pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                                i, //PipeIndex,
                                                NULL
                                                );
        WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pipe),
                        WdfIoTargetCancelSentIo);
    }
}


VOID
StartAllPipes(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;
    UCHAR count,i;

    count = DeviceContext->NumberConfiguredPipes;
    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;
        pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                                i, //PipeIndex,
                                                NULL
                                                );
        status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pipe));
        if (!NT_SUCCESS(status)) {
            UsbSamp_DbgPrint(1, ("StartAllPipes - failed pipe #%d\n", i));
        }
    }
}

NTSTATUS
ResetDevice(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine calls WdfUsbTargetDeviceResetPortSynchronously to reset the device if it's still
    connected.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
{
    PDEVICE_CONTEXT pDeviceContext;
    NTSTATUS status;

    UsbSamp_DbgPrint(3, ("ResetDevice - begins\n"));

    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);
    
    //
    // A reset-device
    // request will be stuck in the USB until the pending transactions
    // have been canceled. Similarly, if there are pending tranasfers on the BULK
    // _In_/OUT pipe cancel them.
    // To work around this issue, the driver should stop the continuous reader
    // (by calling WdfIoTargetStop) before resetting the device, and restart the
    // continuous reader (by calling WdfIoTargetStart) after the request completes.
    //
    StopAllPipes(pDeviceContext);
    
    //
    // It may not be necessary to check whether device is connected before
    // resetting the port.
    //
    status = WdfUsbTargetDeviceIsConnectedSynchronous(pDeviceContext->WdfUsbTargetDevice);

    if (NT_SUCCESS(status)) {
        status = WdfUsbTargetDeviceResetPortSynchronously(pDeviceContext->WdfUsbTargetDevice);
    }

    StartAllPipes(pDeviceContext);
    
    UsbSamp_DbgPrint(3, ("ResetDevice - ends\n"));

    return status;
}
