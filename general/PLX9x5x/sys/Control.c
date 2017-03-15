/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Control.c

Abstract:

    This module implements the driver's IOCTL handler.

Environment:

    Kernel mode

--*/

#include "precomp.h"

#include "Control.tmh"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID
PLxEvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
    )
/*++

Routine Description:

    Called by the framework as soon as it receives a Device I/O request.

    Note that this callback may run concurrently to the read and write
    queues' callbacks. This is acceptable in the context of this sample,
    because the companion application runs Read/Write requests and IOCTL
    requests sequentially, but your project might require some form of
    synchronization.

Arguments:

    Queue              - Handle to the IOCTL Queue
    Request            - Handle to the IOCTL request
    OutputBufferLength - Length of request's output buffer
    InputBufferLength  - Length of request's input buffer
    IoControlCode      - The IOCTL associated with the request

Return Value:

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    PDEVICE_EXTENSION devExt;
    PUCHAR outBuffer;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    device = WdfIoQueueGetDevice(Queue);
    devExt = PLxGetDeviceContext(device);

    switch (IoControlCode) {
        case IOCTL_PLX9X5X_TOGGLE_SINGLE_TRANSFER:
            status = WdfRequestRetrieveOutputBuffer(Request,
                                                    sizeof(*outBuffer),
                                                    &outBuffer,
                                                    NULL);
            if (!NT_SUCCESS(status)) {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                            "WdfRequestRetrieveOutputBuffer failed %!STATUS!",
                            status);
                break;
            }

            devExt->RequireSingleTransfer = !devExt->RequireSingleTransfer;
            *outBuffer = (UCHAR)devExt->RequireSingleTransfer;

            WdfRequestSetInformation(Request, (ULONG_PTR)sizeof(*outBuffer));
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "Unknown IOCTL 0x%x %!STATUS!",
                        IoControlCode,
                        status);
            break;
    }

    WdfRequestComplete(Request, status);
}

