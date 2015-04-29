/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Queue.c

Abstract:

    Read/write functionality for bthecho client

Environment:

    Kernel mode only


--*/

#include "clisrv.h"
#include "device.h"
#include "queue.h"

#if defined(EVENT_TRACING)
#include "queue.tmh"
#endif

void
BthEchoCliReadWriteCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++
Description:

    Completion routine for read/write requests

    We receive l2ca transfer BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.
    
Arguments:

    Request - Request that got completed
    Target - Target to which request was sent
    Params - Completion parameters for the request
    Context - We receive BRB as the context

--*/
{
    struct _BRB_L2CA_ACL_TRANSFER *brb;    
    size_t information;

    UNREFERENCED_PARAMETER(Target);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "I/O completion, status: %!STATUS!", Params->IoStatus.Status);        

    brb = (struct _BRB_L2CA_ACL_TRANSFER *) Context;

    NT_ASSERT((brb != NULL));

    //
    // Bytes read/written are contained in Brb->BufferSize
    //
    information = brb->BufferSize;

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(
        Request,
        Params->IoStatus.Status,
        information
        );
}

VOID
BthEchoCliEvtQueueIoWrite(
    _In_ WDFQUEUE  Queue,
    _In_ WDFREQUEST  Request,
    _In_ size_t  Length
    )
/*++
Description:

    This routine is invoked by the framework to deliver a
    Write request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Write request
    Length - Length of write

--*/
{
    NTSTATUS status;
    PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx;
    WDFMEMORY memory;
    struct _BRB_L2CA_ACL_TRANSFER * brb = NULL;    
    PBTHECHO_CONNECTION connection;
    
    UNREFERENCED_PARAMETER(Length);

    connection =  GetFileContext(WdfRequestGetFileObject(Request))->Connection;

    DevCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));

    status = WdfRequestRetrieveInputMemory(
        Request,
        &memory
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, 
            "WdfRequestRetrieveInputMemory failed, request 0x%p, Status code %!STATUS!\n", 
            Request,
            status);

        goto exit;        
    }

    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_ACL_TRANSFER BRB
    //
    brb = (struct _BRB_L2CA_ACL_TRANSFER *)GetRequestContext(Request);
    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        (PBRB)brb, 
        BRB_L2CA_ACL_TRANSFER
        );


    //
    // Format the Write request for L2Ca OUT transfer
    //
    // This routine allocates a BRB which is returned to us
    // in brb parameter
    //
    // This BRB is freed by the completion routine is we send the
    // request successfully, else it is freed by this routine.
    //
    status = BthEchoConnectionObjectFormatRequestForL2CaTransfer(
        connection,
        Request,
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
        Request,
        BthEchoCliReadWriteCompletion,
        brb
        );

    //
    // Send the request down the stack
    //
    if (FALSE == WdfRequestSend(
        Request,
        DevCtx->Header.IoTarget,
        NULL
        ))
    {
        status = WdfRequestGetStatus(Request);

        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, 
            "Request send failed for request 0x%p, Brb 0x%p, Status code %!STATUS!\n", 
            Request,
            brb,
            status
            );

        goto exit;
    }    

exit:
    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(Request, status);
    }
}

VOID
BthEchoCliEvtQueueIoRead (
    _In_ WDFQUEUE  Queue,
    _In_ WDFREQUEST  Request,
    _In_ size_t  Length
    )
/*++
Description:

    This routine is invoked by the framework to deliver a
    Read request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Read request
    Length - Length of Read

--*/
{
    NTSTATUS status;
    PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx;
    WDFMEMORY memory;
    struct _BRB_L2CA_ACL_TRANSFER * brb = NULL;    
    PBTHECHO_CONNECTION connection;
    
    UNREFERENCED_PARAMETER(Length);

    connection =  GetFileContext(WdfRequestGetFileObject(Request))->Connection;
    
    DevCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));

    status = WdfRequestRetrieveOutputMemory(
        Request,
        &memory
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, 
            "WdfRequestRetrieveInputMemory failed, request 0x%p, Status code %!STATUS!\n", 
            Request,
            status);

        goto exit;        
    }

    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_ACL_TRANSFER BRB
    //
    brb = (struct _BRB_L2CA_ACL_TRANSFER *)GetRequestContext(Request);
    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        (PBRB)brb, 
        BRB_L2CA_ACL_TRANSFER
        );

    //
    // Format the Read request for L2Ca IN transfer
    //
    // This routine allocates a BRB which is returned to us
    // in brb parameter
    //
    // This BRB is freed by the completion routine is we send the
    // request successfully, else it is freed by this routine.
    //
    status = BthEchoConnectionObjectFormatRequestForL2CaTransfer(
        connection,
        Request,
        &brb,
        memory,
        ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    //
    // Set a CompletionRoutine callback function.
    //
    
    WdfRequestSetCompletionRoutine(
        Request,
        BthEchoCliReadWriteCompletion,
        brb
        );

    //
    // Send the request down the stack
    //
    if (FALSE == WdfRequestSend(
        Request,
        DevCtx->Header.IoTarget,
        NULL
        ))
    {
        status = WdfRequestGetStatus(Request);

        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, 
            "Request send failed for request 0x%p, Brb 0x%p, Status code %!STATUS!\n", 
            Request,
            brb,
            status
            );

        goto exit;
    }    

exit:
    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(Request, status);
    }
}

VOID
BthEchoCliEvtQueueIoStop(
    _In_ WDFQUEUE  Queue,
    _In_ WDFREQUEST  Request,
    _In_ ULONG  ActionFlags
    )
/*++
Description:

    This routine is invoked by Framework when Queue is being stopped

    We implement this routine to cancel any requests owned by our driver.
    
    Without this Queue stop would wait indefinitely for requests to complete
    during surprise remove.

Arguments:

    Queue - Framework queue being stopped
    Request - Request owned by the driver
    ActionFlags - Action flags

--*/
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);

    WdfRequestCancelSentRequest(Request);
}

