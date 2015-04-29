/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Echo.c

Abstract:

    Contains echo related functionality

Environment:

    Kernel mode only

--*/

#include "clisrv.h"
#include "device.h"
#include "server.h"
#include "echo.h"

#if defined(EVENT_TRACING)
#include "echo.tmh"
#endif

void
BthEchoSrvWriteCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++
Description:

    Completion routine for echo L2Ca transfer

Arguments:

    Request - Request that completed
    Target - Target to which request was sent
    Params - Request completion parameters
    Context - We receive connection as the context

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    status = Params->IoStatus.Status;
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Write completion, status: %!STATUS!", status);        

    WdfObjectDelete(Request);    

    //
    // We don't attempt to disconnect in case of failure
    // here because we expect the failure only be due to
    // device/channel disconnect, in which case disconnect
    // would happen anyway.
    //
    // Without taking a reference on the connection object
    // while submitting echo write
    // we cannot touch connection object here as connection could get
    // disconnected and connection object could get freed.
    //
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvSendEcho(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ PVOID SrcBuffer,
    _In_ size_t SrcBufferLength
    )
/*++
Routine Description:

    Performs L2Cap transfer to client to do the echo.
    
    This routine is invoked by continuous reader read completion callback
    (BthEchoSrvConnectionObjectContReaderReadCompletedCallback).

Arguments:

    DevCtxHdr - Device context
    Connection - Connection whose continous reader had read completion
    SrcBuffer - Source buffer for the echo
    SrcBufferLength - Length of the source buffer

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFREQUEST request                  = NULL;
    WDFMEMORY memory                    = NULL;
    PVOID dataBuffer                    = NULL;
    struct _BRB_L2CA_ACL_TRANSFER *brb  = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = DevCtxHdr->Device;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BRB);

    status = WdfRequestCreate(
        &attributes,
        DevCtxHdr->IoTarget,
        &request
        );                    

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Creating request for echo failed, Status code %!STATUS!\n",
            status
            );

        goto exit;
    }

    if (SrcBufferLength <= 0) 
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER,
            "SrcBufferLength has an invalid value: %I64d\n",
            SrcBufferLength
            );

        status = STATUS_INVALID_PARAMETER;

        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    status = WdfMemoryCreate(
        &attributes,
        NonPagedPoolNx,
        POOLTAG_BTHECHOSAMPLE,
        SrcBufferLength,
        &memory,
        &dataBuffer
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Creating memory for pending read failed, Status code %!STATUS!\n",
            status
            );

        goto exit;
    }

    memcpy(dataBuffer, SrcBuffer, SrcBufferLength);

    brb = (struct _BRB_L2CA_ACL_TRANSFER *)GetEchoRequestContext(request);

    status = BthEchoConnectionObjectFormatRequestForL2CaTransfer(
        Connection,
        request,
        &brb,
        memory,
        ACL_TRANSFER_DIRECTION_OUT
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    //
    // Set a CompletionRoutine callback function.
    //
    
    WdfRequestSetCompletionRoutine(
        request,
        BthEchoSrvWriteCompletion,
        Connection
        );

    if (FALSE == WdfRequestSend(
        request,
        DevCtxHdr->IoTarget,
        NULL
        ))
    {
        status = WdfRequestGetStatus(request);

        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Request send failed for request 0x%p, Brb 0x%p, Status code %!STATUS!\n", 
            request,
            brb,
            status
            );

        goto exit;
    }    

exit:
    if (!NT_SUCCESS(status))
    {
        if (NULL != request)
        {
            WdfObjectDelete(request);
        }

        //
        // If we failed disconnect
        //
        BthEchoSrvDisconnectConnection(Connection);        
    }    
}


