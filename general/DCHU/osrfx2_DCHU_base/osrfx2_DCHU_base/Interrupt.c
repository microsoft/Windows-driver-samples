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

    User mode

--*/

#include "osrusbfx2.h"

#if defined(EVENT_TRACING)
#include "interrupt.tmh"
#endif


/*++

Routine Description:

    This routine configures a continuous reader on the
    interrupt endpoint. It's called from the PrepareHarware event.

Arguments:

    DeviceContext - The device context to use for configuration

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
OsrFxConfigContReaderForInterruptEndPoint(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_USB_CONTINUOUS_READER_CONFIG ReaderConfig;
    NTSTATUS Status;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&ReaderConfig,
                                          OsrFxEvtUsbInterruptPipeReadComplete,
                                          DeviceContext,
                                          sizeof(UCHAR));

    ReaderConfig.EvtUsbTargetPipeReadersFailed = OsrFxEvtUsbInterruptReadersFailed;

    //
    // Reader requests are not posted to the target automatically.
    // The driver must explictly call WdfIoTargetStart to kick start the
    // reader. In this sample, it's done in D0Entry.
    // By defaut, framework queues two requests to the target
    // endpoint. Driver can configure up to 10 requests with CONFIG macro.
    //
    Status = WdfUsbTargetPipeConfigContinuousReader(DeviceContext->InterruptPipe,
                                                    &ReaderConfig);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "OsrFxConfigContReaderForInterruptEndPoint failed %x\n",
                    Status);
        return Status;
    }

    return Status;
}


/*++

Routine Description:

    This is the completion routine of the continuous reader. This can be called
    concurrently on a multiprocessor system if there are multiple readers
    configured, so make sure to protect access to global resources.

Arguments:

    Buffer  - This buffer is freed when this call returns.
              If the driver wants to delay processing of the buffer, it
              can take an additional referrence

    Context - Provided in the WDF_USB_CONTINUOUS_READER_CONFIG_INIT macro

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
VOID
OsrFxEvtUsbInterruptPipeReadComplete(
    _In_ WDFUSBPIPE Pipe,
    _In_ WDFMEMORY  Buffer,
    _In_ size_t     NumBytesTransferred,
    _In_ WDFCONTEXT Context
    )
{
    PUCHAR          SwitchState = NULL;
    WDFDEVICE       Device;
    PDEVICE_CONTEXT DeviceContext = Context;

    UNREFERENCED_PARAMETER(Pipe);

    Device = WdfObjectContextGetObject(DeviceContext);

    //
    // Make sure that there is data in the read packet.  Depending on the device
    // specification, it is possible for it to return a 0 length read in
    // certain conditions.
    //
    if (NumBytesTransferred == 0)
    {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_INIT,
                    "OsrFxEvtUsbInterruptPipeReadComplete Zero length read "
                    "occured on the Interrupt Pipe's Continuous Reader\n");
        return;
    }

    assert(NumBytesTransferred == sizeof(UCHAR));

    SwitchState = WdfMemoryGetBuffer(Buffer, NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "OsrFxEvtUsbInterruptPipeReadComplete SwitchState %x\n",
                *SwitchState);

    DeviceContext->CurrentSwitchState = *SwitchState;

    //
    // Handle any pending Interrupt Message IOCTLs. Note that the OSR USB device
    // will generate an interrupt message when the the device resumes from a low
    // power state. So if the Interrupt Message IOCTL was sent after the device
    // has gone to a low power state, the pending Interrupt Message IOCTL will
    // get completed in the function call below, before the user twiddles the
    // dip switches on the OSR USB device. If this is not the desired behavior
    // for your driver, then you could handle this condition by maintaining a
    // state variable on D0Entry to track interrupt messages caused by power up.
    //
    OsrUsbIoctlGetInterruptMessage(Device, STATUS_SUCCESS);

}


/*++

Routine Description:

    This the failure routine of the continous reader.

Arguments:

    Pipe       - The pipe that failed creation

    Status     - The failure NTSTATUS

    UsbdStatus - A USB specific error code

Return Value:

    TRUE

--*/
BOOLEAN
OsrFxEvtUsbInterruptReadersFailed(
    _In_ WDFUSBPIPE  Pipe,
    _In_ NTSTATUS    Status,
    _In_ USBD_STATUS UsbdStatus
    )
{
    WDFDEVICE Device = WdfIoTargetGetDevice(WdfUsbTargetPipeGetIoTarget(Pipe));
    PDEVICE_CONTEXT DeviceContext = GetDeviceContext(Device);

    UNREFERENCED_PARAMETER(UsbdStatus);

    //
    // Clear the current switch state.
    //
    DeviceContext->CurrentSwitchState = 0;

    //
    // Service the pending interrupt switch change request
    //
    OsrUsbIoctlGetInterruptMessage(Device, Status);

    return TRUE;
}

