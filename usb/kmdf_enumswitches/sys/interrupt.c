/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Interrupt.c

Abstract:

    This modules has routines configure a continuous reader on an
    interrupt pipe to asynchronously read toggle switch states.

Environment:

    Kernel mode

--*/

#include <osrusbfx2.h>

#include "interrupt.tmh"


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
OsrFxConfigContReaderForInterruptEndPoint(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
/*++

Routine Description:

    This routine configures a continuous reader on the
    interrupt endpoint. It's called from the PrepareHarware event.

Arguments:


Return Value:

    NT status value

--*/
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;
    NTSTATUS                         status;
    WDFUSBPIPE                       pipe;

    pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                            INTERRUPT_IN_ENDPOINT_INDEX, // PipeIndex,
                                            NULL);                       // pipeInfo
    //
    // Tell the framework that it's okay to read less than
    // MaximumPacketSize
    //
    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
                                          OsrFxEvtUsbInterruptPipeReadComplete,
                                          DeviceContext,    // Context
                                          sizeof(UCHAR));   // TransferLength

    contReaderConfig.EvtUsbTargetPipeReadersFailed =
                                    OsrFxEvtUsbInterruptReadersFailed;

    //
    // Reader requests are not posted to the target automatically.
    // Driver must explictly call WdfIoTargetStart to kick start the
    // reader.  In this sample, it's done in D0Entry.
    // By defaut, framework queues two requests to the target
    // endpoint. Driver can configure up to 10 requests with CONFIG macro.
    //
    status = WdfUsbTargetPipeConfigContinuousReader(pipe, &contReaderConfig);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "OsrFxConfigContReaderForInterruptEndPoint failed %x\n",
                    status);
        return status;
    }

    return status;
}

VOID
OsrFxEvtUsbInterruptPipeReadComplete(
    WDFUSBPIPE  Pipe,
    WDFMEMORY   Buffer,
    size_t      NumBytesTransferred,
    WDFCONTEXT  Context
    )
/*++

Routine Description:

    This the completion routine of the continour reader. This can
    called concurrently on multiprocessor system if there are
    more than one readers configured. So make sure to protect
    access to global resources.

Arguments:

    Buffer - This buffer is freed when this call returns.
             If the driver wants to delay processing of the buffer, it
             can take an additional referrence.

    Context - Provided in the WDF_USB_CONTINUOUS_READER_CONFIG_INIT macro

Return Value:

    NT status value

--*/
{
    PUCHAR          switchState = NULL;
    WDFDEVICE       device;
    PDEVICE_CONTEXT pDeviceContext = Context;

    UNREFERENCED_PARAMETER(Pipe);

    device = WdfObjectContextGetObject(pDeviceContext);

    //
    // Make sure that there is data in the read packet.  Depending on the device
    // specification, it is possible for it to return a 0 length read in
    // certain conditions.
    //

    if (NumBytesTransferred == 0) {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_INIT,
                    "OsrFxEvtUsbInterruptPipeReadComplete Zero length read "
                    "occured on the Interrupt Pipe's Continuous Reader\n"
                    );
        return;
    }

    NT_ASSERT(NumBytesTransferred == sizeof(UCHAR));

    switchState = WdfMemoryGetBuffer(Buffer, NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "OsrFxEvtUsbInterruptPipeReadComplete SwitchState %x\n",
                *switchState);

    pDeviceContext->CurrentSwitchState = *switchState;

    OsrFxEnumerateChildren(device);
}

BOOLEAN
OsrFxEvtUsbInterruptReadersFailed(
    WDFUSBPIPE      Pipe,
    NTSTATUS        Status,
    USBD_STATUS     UsbdStatus
    )
/*++

Routine Description:

    EvtUsbTargetPipeReadersFailed is called to inform the driver that a
    continuous reader has reported an error while processing a read request.

Arguments:

    Pipe - handle to a framework pipe object.
    Status - NTSTATUS value that the pipe's I/O target returned.
    UsbdStatus - USBD_STATUS-typed status value that the pipe's I/O target returned.

Return Value:

    If TRUE, causes the framework to reset the USB pipe and then
        restart the continuous reader.
    If FASLE, the framework does not reset the device or restart
        the continuous reader.

    If this event is not registered, framework default action is to reset
    the pipe and restart the reader.

--*/
{
    UNREFERENCED_PARAMETER(Pipe);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "OsrFxEvtUsbInterruptReadersFailed NTSTATUS 0x%x, UsbdStatus 0x%x\n",
                Status, UsbdStatus);

    return TRUE;
}


