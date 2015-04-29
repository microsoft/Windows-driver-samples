/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    connection.c

Abstract:

    Implementation of connection object which represents an L2CA connection

    We create a WDFOBJECT for connection in BthEchoConnectionObjectCreate. 
    PBTHECHO_CONNECTION is maintained as context of this object.

    We use passive dispose for connection WDFOBJECT. This allows us to wait for
    continuous readers to stop and disconnect to complete in the cleanup
    callback (BthEchoEvtConnectionObjectCleanup) of connection object.

    Connection object also includes continuous readers which need 
    to be explicitly initialied and submitted.

    Both client and server utlize connection object although continous reader
    functionality is utilized only by server.

Environment:

    Kernel mode

--*/

#include "clisrv.h"

#if defined(EVENT_TRACING)
#include "connection.tmh"
#endif

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoRepeatReaderSubmit(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtx,
    _In_ PBTHECHO_REPEAT_READER RepeatReader
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoRepeatReaderWaitForStop(    
    _In_ PBTHECHO_REPEAT_READER RepeatReader
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoConnectionObjectWaitForAndUninitializeContinuousReader(
    _In_ PBTHECHO_CONNECTION Connection
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, BthEchoRepeatReaderWaitForStop)
#pragma alloc_text (PAGE, BthEchoConnectionObjectWaitForAndUninitializeContinuousReader)
#pragma alloc_text (PAGE, BthEchoEvtConnectionObjectCleanup)
#pragma alloc_text (PAGE, BthEchoConnectionObjectRemoteDisconnectSynchronously)
#endif


KDEFERRED_ROUTINE
BthEchoRepeatReaderResubmitReadDpc;

VOID
BthEchoRepeatReaderResubmitReadDpc(
    _In_ struct _KDPC  *Dpc,
    _In_opt_ PVOID  DeferredContext,
    _In_opt_ PVOID  SystemArgument1,
    _In_opt_ PVOID  SystemArgument2
)
/*++

Description:

    Dpc for resubmitting pending read

Arguments:

    Dpc - Dpc
    DeferredContext - We receive device context header as DeferredContext
    SystemArgument1 - We receive repeat reader as SystemArgument1
    SystemArgument2 - Unused

--*/
{
    PBTHECHO_REPEAT_READER repeatReader;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);    

    repeatReader =
        (PBTHECHO_REPEAT_READER) SystemArgument1;

    //
    // BthEchoRepeatReaderSubmit will invoke contreader
    // failure callback in case of failure
    // so we don't check for failure here
    //
    if ((NULL != DeferredContext) && (NULL != repeatReader))
    {
        (void) BthEchoRepeatReaderSubmit(
            (PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER) DeferredContext,
            repeatReader
            ); 
    }
}

EVT_WDF_REQUEST_COMPLETION_ROUTINE
BthEchoRepeatReaderPendingReadCompletion;

void
BthEchoRepeatReaderPendingReadCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++

Description:

    Completion routine for pending read request

    In this routine we invoke the read completion callback
    in case of success and contreader failure callback
    in case of failure.

    These are implemented by server.

Arguments:

    Request - Request completed
    Target - Target to which request was sent
    Params - Completion parameters
    Context - We receive repeat reader as the context

--*/
{
    PBTHECHO_REPEAT_READER repeatReader;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request);

    repeatReader = (PBTHECHO_REPEAT_READER) Context;
       
    NT_ASSERT((repeatReader != NULL));

    status = Params->IoStatus.Status;

    if (NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, 
            "Pending read completion, RepeatReader: 0x%p, status: %!STATUS!, Buffer: 0x%p, BufferSize: %d", 
            repeatReader,
            Params->IoStatus.Status,
            repeatReader->TransferBrb.Buffer,
            repeatReader->TransferBrb.BufferSize            
            );
           
        repeatReader->Connection->ContinuousReader.BthEchoConnectionObjectContReaderReadCompleteCallback(
            repeatReader->Connection->DevCtxHdr,
            repeatReader->Connection,
            repeatReader->TransferBrb.Buffer,
            repeatReader->TransferBrb.BufferSize
            );

    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Pending read completed with failure, RepeatReader: 0x%p, status: %!STATUS!", 
            repeatReader,
            Params->IoStatus.Status          
            );        
    }

    if (!NT_SUCCESS(status))
    {
        //
        // Invoke reader failed callback only if status is something other
        // than cancelled. If the status is STATUS_CANCELLED this is because
        // we have cancelled repeat reader.
        //
        if (STATUS_CANCELLED != status)
        {       
            //
            // Invoke reader failed callback before setting the stop event
            // to ensure that connection object is alive during this callback
            //
            repeatReader->Connection->ContinuousReader.BthEchoConnectionObjectContReaderFailedCallback(
                repeatReader->Connection->DevCtxHdr, 
                repeatReader->Connection
                );
        }
            
        //
        // Set stop event because we are not resubmitting the reader
        //
        KeSetEvent(&repeatReader->StopEvent, 0, FALSE);        
    }
    else
    {
        //
        // Resubmit pending read
        // We use a Dpc to avoid recursing in this completion routine
        //

        BOOLEAN ret = KeInsertQueueDpc(&repeatReader->ResubmitDpc, repeatReader, NULL);
        NT_ASSERT (TRUE == ret); //we only expect one outstanding dpc
        UNREFERENCED_PARAMETER(ret); //ret remains unused in fre build
    }
}

VOID
BthEchoRepeatReaderUninitialize(    
    _In_ PBTHECHO_REPEAT_READER RepeatReader
    )
{
    if (NULL != RepeatReader->RequestPendingRead)
    {
        RepeatReader->RequestPendingRead = NULL;

        RepeatReader->Stopping = 0;        
    }    
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoRepeatReaderInitialize(
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ PBTHECHO_REPEAT_READER RepeatReader,
    _In_ size_t BufferSize    
    )

/*++

Description:

    This routine initializes repeat reader.

Arguments:

    Connection - Connection with which this repeat reader is associated
    RepeatReader - Repeat reader
    BufferSize - Buffer size for read

Return Value:

    NTSTATUS Status code.

--*/

{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Create request object for pending read
    // Set connection object as the parent for the request
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = WdfObjectContextGetObject(Connection);

    status = WdfRequestCreate(
        &attributes,
        Connection->DevCtxHdr->IoTarget,
        &RepeatReader->RequestPendingRead
        );                    

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Creating request for pending read failed, "
            "Status code %!STATUS!\n",
            status
            );

        goto exit;
    }

    if (BufferSize <= 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER,
            "BufferSize has an invalid value: %I64d\n",
            BufferSize
            );

        status = STATUS_INVALID_PARAMETER;

        goto exit;
    }

    //
    // Create memory object for the pending read
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = RepeatReader->RequestPendingRead;

    status = WdfMemoryCreate(
        &attributes,
        NonPagedPoolNx,
        POOLTAG_BTHECHOSAMPLE,
        BufferSize,
        &RepeatReader->MemoryPendingRead,
        NULL //buffer
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Creating memory for pending read failed, "
            "Status code %!STATUS!\n",
            status
            );

        goto exit;
    }

    //
    // Initialize Dpc that we will use for resubmitting pending read
    //
    KeInitializeDpc(
        &RepeatReader->ResubmitDpc, 
        BthEchoRepeatReaderResubmitReadDpc, 
        Connection->DevCtxHdr
        );

    //
    // Initialize event used to wait for stop.
    // This even is created as signalled. It gets cleared when
    // request is submitted.
    //
    KeInitializeEvent(&RepeatReader->StopEvent, NotificationEvent, TRUE);

    RepeatReader->Connection = Connection;
    
exit:
    if (!NT_SUCCESS(status))
    {
        BthEchoRepeatReaderUninitialize(RepeatReader);
    }
    
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoRepeatReaderSubmit(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_REPEAT_READER RepeatReader
    )
/*++

Description:

    This routine submits the repeat reader.

    In case of failure it invoked contreader failed callback

Arguments:

    DevCtxHdr - Device context header
    RepeatReader - Repeat reader to submit

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    struct _BRB_L2CA_ACL_TRANSFER *brb = &RepeatReader->TransferBrb;    

    DevCtxHdr->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_L2CA_ACL_TRANSFER);

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_UNSUCCESSFUL);
    statusReuse = WdfRequestReuse(RepeatReader->RequestPendingRead, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    //
    // Check if we are stopping, if yes set StopEvent and exit.
    //
    // After this point request is eligible for cancellation, so if this
    // flag gets set after we check it, request will be cancelled for stopping
    // the repeat reader and next time around we will stop when we are invoked
    // again upon completion of cancelled request.
    //
    if (RepeatReader->Stopping)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, 
            "Continuos reader 0x%p stopping", RepeatReader);        
        
        KeSetEvent(&RepeatReader->StopEvent, 0, FALSE);

        status = STATUS_SUCCESS;

        goto exit;
    }

    //
    // Format request for L2CA IN transfer
    //
    status = BthEchoConnectionObjectFormatRequestForL2CaTransfer(
        RepeatReader->Connection,
        RepeatReader->RequestPendingRead,
        &brb,
        RepeatReader->MemoryPendingRead,
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
        RepeatReader->RequestPendingRead,
        BthEchoRepeatReaderPendingReadCompletion,
        RepeatReader
        );

    //
    // Clear the stop event before sending the request
    // This is relevant only on start of the repeat reader 
    // (i.e. the first submission)
    // this event eventually gets set only when repeat reader stops
    // and not on every resubmission.
    //
    KeClearEvent(&RepeatReader->StopEvent);

    if (FALSE == WdfRequestSend(
        RepeatReader->RequestPendingRead,
        DevCtxHdr->IoTarget,
        NULL
        ))
    {
        status = WdfRequestGetStatus(RepeatReader->RequestPendingRead);

        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, 
            "Request send failed for request 0x%p, Brb 0x%p, Status code %!STATUS!\n", 
            RepeatReader->RequestPendingRead,
            brb,
            status
            );

        goto exit;
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, 
            "Resubmited pending read with request 0x%p, Brb 0x%p", 
            RepeatReader->RequestPendingRead,
            brb
            );        
    }

exit:
    if (!NT_SUCCESS(status))
    {
        //
        // Invoke the reader failed callback before setting the event
        // to ensure that the connection object is alive during this callback
        //
        RepeatReader->Connection->ContinuousReader.BthEchoConnectionObjectContReaderFailedCallback(
            RepeatReader->Connection->DevCtxHdr, 
            RepeatReader->Connection
            );

        //
        // If we failed to send pending read, set the event since
        // we will not get completion callback
        //
            
        KeSetEvent(&RepeatReader->StopEvent, 0, FALSE);
    }
    
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoRepeatReaderCancel(
    _In_ PBTHECHO_REPEAT_READER RepeatReader    
    )
/*++

Description:

    This routine cancels repeat reader
    but it does not wait for repeat reader to stop.

    This wait must be performed separately.

Arguments:

    RepeatReader - Repeat reader to be cancelled

--*/
{
    //
    // Signal that we are transitioning to stopped state
    // so that we don't resubmit pending read
    //
    
    InterlockedIncrement(&RepeatReader->Stopping);

    //
    // Cancel the pending read.
    //
    // If BthEchoRepeatReaderSubmit misses the setting of Stopping
    // flag, cancel would cause the request to complete quickly and
    // repeat reader will subsequently be stopped.
    //
    WdfRequestCancelSentRequest(RepeatReader->RequestPendingRead);    
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoRepeatReaderWaitForStop(    
    _In_ PBTHECHO_REPEAT_READER RepeatReader
    )
/*++

Description:

    This routine waits for repeat reader to stop

Arguments:

    RepeatReader - Repeat reader to be waited on

--*/
{
    PAGED_CODE();

    //
    // Wait for pending read to complete
    //
    
    KeWaitForSingleObject(
        &RepeatReader->StopEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );    
}

//====================================================================
//Connection object functions
//====================================================================

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoConnectionObjectInitializeContinuousReader(
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ PFN_BTHECHO_CONNECTION_OBJECT_CONTREADER_READ_COMPLETE 
        BthEchoConnectionObjectContReaderReadCompleteCallback,
    _In_ PFN_BTHECHO_CONNECTION_OBJECT_CONTREADER_FAILED 
        BthEchoConnectionObjectContReaderFailedCallback,
    _In_ size_t BufferSize
    )
/*++
Description:

    This routine initializes continuous reader for connection

Arguments:

    Connection - Connection for which to initialize continuous readers
    BthEchoConnectionObjectContReaderReadCompleteCallback - 
        Pending read completion callback
    BthEchoConnectionObjectContReaderFailedCallback - 
        Repeat reader failed callback
    BufferSize - Buffer size for the pending read

Return Value:

    NTSTATUS Status code.
--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    UINT i;

    Connection->ContinuousReader.BthEchoConnectionObjectContReaderReadCompleteCallback = 
        BthEchoConnectionObjectContReaderReadCompleteCallback;
    Connection->ContinuousReader.BthEchoConnectionObjectContReaderFailedCallback = 
        BthEchoConnectionObjectContReaderFailedCallback;
        
    for (i = 0; i < BTHECHOSAMPLE_NUM_CONTINUOUS_READERS; i++)
    {
        status = BthEchoRepeatReaderInitialize(
            Connection,
            &Connection->ContinuousReader.RepeatReaders[i],
            BufferSize
            );
        
        if (!NT_SUCCESS(status))
        {
            break;
        }
        else
        {
            ++Connection->ContinuousReader.InitializedReadersCount;
        }
    }

    //
    // In case of error (as well as on success) BthEchoEvtConnectionObjectCleanup 
    // will uninitialize readers
    //
    
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoConnectionObjectContinuousReaderSubmitReaders(
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++

Description:

    This routine submits all the initialized repeat readers for the 
    connection

Arguments:

    Connection - Connection whose repeat readers are to be submitted

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT i;

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount
        <= BTHECHOSAMPLE_NUM_CONTINUOUS_READERS);
    
    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++)
    {
        status = BthEchoRepeatReaderSubmit(
            Connection->DevCtxHdr, 
            &Connection->ContinuousReader.RepeatReaders[i]
            );
        
        if (!NT_SUCCESS(status))
        {
            goto exit;
        }
    }

exit:
    if (!NT_SUCCESS(status))
    {
        //
        // Cancel any submitted continuous readers
        // We make sure that it is safe to call cancel on unsubmitted readers
        //
        
        BthEchoConnectionObjectContinuousReaderCancelReaders(Connection);
    }
    
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoConnectionObjectContinuousReaderCancelReaders(
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++

Description:

    This routine cancels all the initialized repeat readers for the
    connection

Arguments:

    Connection - Connection whose repeat readers are to be camcelled

--*/
{
    UINT i;

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount
        <= BTHECHOSAMPLE_NUM_CONTINUOUS_READERS);

    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++)
    {
        BthEchoRepeatReaderCancel(&Connection->ContinuousReader.RepeatReaders[i]);
    }
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoConnectionObjectWaitForAndUninitializeContinuousReader(
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++

Description:

    This routine waits for and uninitializes all the initialized 
    continuous readers for the connection.

Arguments:

    Connection - Connection whose continuous reader is to be uninitialized

--*/
{
    UINT i;

    PAGED_CODE();

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount
        <= BTHECHOSAMPLE_NUM_CONTINUOUS_READERS);
    
    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++)
    {
        PBTHECHO_REPEAT_READER repeatReader = &Connection->ContinuousReader.RepeatReaders[i];

        BthEchoRepeatReaderWaitForStop(repeatReader); //It is OK to wait on unsubmitted readers
        BthEchoRepeatReaderUninitialize(repeatReader);
    }    
}

#pragma warning(push)
#pragma warning(disable:28118) // this callback will run at IRQL=PASSIVE_LEVEL
_Use_decl_annotations_
VOID
BthEchoEvtConnectionObjectCleanup(
    WDFOBJECT  ConnectionObject
    )
/*++

Description:

    This routine is invoked by the Framework when connection object
    gets deleted (either explicitly or implicitly because of parent
    deletion).

    Since we mark ExecutionLevel as passive for connection object we get this
    callback at passive level and can wait for stopping of continuous
    readers and for disconnect to complete.

Arguments:

    ConnectionObject - The Connection Object

Return Value:

    None

--*/
{
    PBTHECHO_CONNECTION connection = GetConnectionObjectContext(ConnectionObject);

    PAGED_CODE();
        
    BthEchoConnectionObjectWaitForAndUninitializeContinuousReader(connection);

    KeWaitForSingleObject(&connection->DisconnectEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL);

    WdfObjectDelete(connection->ConnectDisconnectRequest);
}
#pragma warning(pop) // enable 28118 again

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoConnectionObjectInit(
    _In_ WDFOBJECT ConnectionObject,
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr
    )
/*++

Description:

    This routine initializes connection object.
    It is invoked by BthEchoConnectionObjectCreate.

Arguments:

    ConnectionObject - Object to initialize
    DevCtxHdr - Device context header

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    PBTHECHO_CONNECTION connection = GetConnectionObjectContext(ConnectionObject);

    
    connection->DevCtxHdr = DevCtxHdr;

    connection->ConnectionState = ConnectionStateInitialized;

    //
    // Initialize spinlock
    //
    
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = ConnectionObject;

    status = WdfSpinLockCreate(
                               &attributes,
                               &connection->ConnectionLock
                               );
    if (!NT_SUCCESS(status))
    {
        goto exit;                
    }

    //
    // Create connect/disconnect request
    //
    
    status = WdfRequestCreate(
        &attributes,
        DevCtxHdr->IoTarget,
        &connection->ConnectDisconnectRequest
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Initialize event
    //
    
    KeInitializeEvent(&connection->DisconnectEvent, NotificationEvent, TRUE);

    //
    // Initalize list entry
    //

    InitializeListHead(&connection->ConnectionListEntry);

    connection->ConnectionState = ConnectionStateInitialized;

exit:
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoConnectionObjectCreate(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ WDFOBJECT ParentObject,
    _Out_ WDFOBJECT*  ConnectionObject
    )
/*++

Description:

    This routine creates a connection object.
    This is called by client and server when a remote connection
    is made.

Arguments:

    DevCtxHdr - Device context header
    ParentObject - Parent object for connection object
    ConnectionObject - receives the created connection object

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFOBJECT connectionObject = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BTHECHO_CONNECTION);
    attributes.ParentObject = ParentObject;    
    attributes.EvtCleanupCallback = BthEchoEvtConnectionObjectCleanup;

    //
    // We set execution level to passive so that we get cleanup at passive
    // level where we can wait for continuous readers to run down
    // and for completion of disconnect
    //
    attributes.ExecutionLevel = WdfExecutionLevelPassive; 

    status = WdfObjectCreate(
        &attributes,
        &connectionObject
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "WdfObjectCreate for connection object failed, Status code %!STATUS!\n", status);
        
        goto exit;
    }

    status = BthEchoConnectionObjectInit(connectionObject, DevCtxHdr);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "Context initialize for connection object failed, ConnectionObject 0x%p, Status code %!STATUS!\n",
            connectionObject,
            status
            );

        goto exit;
    }

    *ConnectionObject = connectionObject;
    
exit:
    if(!NT_SUCCESS(status) && connectionObject)
    {
        WdfObjectDelete(connectionObject);
    }
    
    return status;
}

EVT_WDF_REQUEST_COMPLETION_ROUTINE
BthEchoConnectionObjectDisconnectCompletion;

void
BthEchoConnectionObjectDisconnectCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++

Description:

    Completion routine for disconnect BRB completion

    In this routine we set the connection to disconnect
    and set the DisconnectEvent.

Arguments:

    Request - Request completed
    Target - Target to which request was sent
    Params - Completion parameters
    Context - We receive connection as the context

--*/
{
    PBTHECHO_CONNECTION connection = (PBTHECHO_CONNECTION) Context;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    WdfSpinLockAcquire(connection->ConnectionLock);
    connection->ConnectionState = ConnectionStateDisconnected;
    WdfSpinLockRelease(connection->ConnectionLock);

    //
    // Disconnect complete, set the event
    //

    KeSetEvent(
        &connection->DisconnectEvent,
        0,
        FALSE
        );    
}


_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
BthEchoConnectionObjectRemoteDisconnect(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++

Description:

    This routine sends a disconnect BRB for the connection

Arguments:

    DevCtxHdr - Device context header
    Connection - Connection which is to be disconnected

Return Value:

    TRUE is this call initiates the disconnect.
    FALSE if the connection was already disconnected.

--*/
{
    struct _BRB_L2CA_CLOSE_CHANNEL * disconnectBrb;

    //
    // Cancel continuous readers for the connection
    //
    BthEchoConnectionObjectContinuousReaderCancelReaders(Connection);
    
    WdfSpinLockAcquire(Connection->ConnectionLock);
    
    if (Connection->ConnectionState == ConnectionStateConnecting) 
    {
        //
        // If the connection is not completed yet set the state 
        // to disconnecting.
        // In such case we should send CLOSE_CHANNEL Brb down after 
        // we receive connect completion.
        //
        
        Connection->ConnectionState = ConnectionStateDisconnecting;

        //
        // Clear event to indicate that we are in disconnecting
        // state. It will be set when disconnect is completed
        //
        KeClearEvent(&Connection->DisconnectEvent);

        WdfSpinLockRelease(Connection->ConnectionLock);
        return TRUE;

    } 
    else if (Connection->ConnectionState != ConnectionStateConnected)
    {
        //
        // Do nothing if we are not connected
        //

        WdfSpinLockRelease(Connection->ConnectionLock);
        return FALSE;
    }

    Connection->ConnectionState = ConnectionStateDisconnecting;
    WdfSpinLockRelease(Connection->ConnectionLock);

    //
    // We are now sending the disconnect, so clear the event.
    //

    KeClearEvent(&Connection->DisconnectEvent);

    DevCtxHdr->ProfileDrvInterface.BthReuseBrb(&Connection->ConnectDisconnectBrb, BRB_L2CA_CLOSE_CHANNEL);

    disconnectBrb = (struct _BRB_L2CA_CLOSE_CHANNEL *) &(Connection->ConnectDisconnectBrb);
    
    disconnectBrb->BtAddress = Connection->RemoteAddress;
    disconnectBrb->ChannelHandle = Connection->ChannelHandle;

    //
    // The BRB can fail with STATUS_DEVICE_DISCONNECT if the device is already
    // disconnected, hence we don't assert for success
    //

    (void) BthEchoSharedSendBrbAsync(
        DevCtxHdr->IoTarget, 
        Connection->ConnectDisconnectRequest, 
        (PBRB) disconnectBrb,
        sizeof(*disconnectBrb),
        BthEchoConnectionObjectDisconnectCompletion,
        Connection
        );    

    return TRUE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoConnectionObjectRemoteDisconnectSynchronously(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++

Description:

    This routine disconnects the connection synchronously

Arguments:

    DevCtxHdr - Device context header
    Connection - Connection which is to be disconnected

--*/
{
    PAGED_CODE();

    BthEchoConnectionObjectRemoteDisconnect(DevCtxHdr, Connection);

    KeWaitForSingleObject(&Connection->DisconnectEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );    
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FormatRequestWithBrb(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ size_t BrbSize
    )
/*++

Description:

    This routine formats are WDFREQUEST with the passed in BRB

Arguments:

    IoTarget - Target to which request will be sent
    Request - Request to be formattted
    Brb - BRB to format the request with
    BrbSize - size of the BRB

--*/
{
    NTSTATUS status         = BTH_ERROR_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memoryArg1    = NULL;

    if (BrbSize <= 0) 
    {        
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, 
            "BrbSize has an invalid value: %I64d\n",
            BrbSize
            );

        status = STATUS_INVALID_PARAMETER;

        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Request;

    status = WdfMemoryCreatePreallocated(
        &attributes,
        Brb,
        BrbSize,
        &memoryArg1
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, 
            "Creating preallocted memory for Brb 0x%p failed, Request to be formatted 0x%p, "
            "Status code %!STATUS!\n",
            Brb,
            Request,
            status
            );

        goto exit;
    }

    status = WdfIoTargetFormatRequestForInternalIoctlOthers(
        IoTarget,
        Request,
        IOCTL_INTERNAL_BTH_SUBMIT_BRB,
        memoryArg1,
        NULL, //OtherArg1Offset
        NULL, //OtherArg2
        NULL, //OtherArg2Offset
        NULL, //OtherArg4
        NULL  //OtherArg4Offset
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, 
            "Formatting request 0x%p with Brb 0x%p failed, Status code %!STATUS!\n",
            Request,
            Brb,
            status
            );

        goto exit;
    }
exit:
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(return==0, _At_(*Brb, __drv_allocatesMem(Mem)))
NTSTATUS
BthEchoConnectionObjectFormatRequestForL2CaTransfer(
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ WDFREQUEST Request,
    _Inout_ struct _BRB_L2CA_ACL_TRANSFER ** Brb,
    _In_ WDFMEMORY Memory,
    _In_ ULONG TransferFlags //flags include direction of transfer
    )
/*++

Description:

    Formats a request for L2Ca transfer

Arguments:

    Connection - Connection on which L2Ca transfer will be made
    Request - Request to be formatted
    Brb - If a Brb is passed in, it will be used, otherwise
          this routine will allocate the Brb and return in this parameter
    Memory - Memory object which has the buffer for transfer
    TransferFlags - Transfer flags which include direction of the transfer

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    struct _BRB_L2CA_ACL_TRANSFER *brb = NULL;
    size_t bufferSize;
    BOOLEAN brbAllocatedLocally = FALSE; //whether this function allocated the BRB

    WdfSpinLockAcquire(Connection->ConnectionLock);

    if(Connection->ConnectionState != ConnectionStateConnected)
    {
        status = STATUS_CONNECTION_DISCONNECTED;
        WdfSpinLockRelease(Connection->ConnectionLock);
        goto exit;
    }

    WdfSpinLockRelease(Connection->ConnectionLock);

    if (NULL == *Brb)
    {
        brb= (struct _BRB_L2CA_ACL_TRANSFER *)
            Connection->DevCtxHdr->ProfileDrvInterface.BthAllocateBrb(
                BRB_L2CA_ACL_TRANSFER, 
                POOLTAG_BTHECHOSAMPLE
                );
        if(!brb)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;

            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, 
                "Failed to allocate BRB_L2CA_ACL_TRANSFER, returning status code %!STATUS!\n", status);

            goto exit;
        }
        else
        {
            brbAllocatedLocally = TRUE;
        }
    }
    else
    {
        brb = *Brb;
        Connection->DevCtxHdr->ProfileDrvInterface.BthReuseBrb(
            (PBRB)brb, BRB_L2CA_ACL_TRANSFER
            );
    }

    brb->BtAddress = Connection->RemoteAddress;
    brb->BufferMDL = NULL;
    brb->Buffer = WdfMemoryGetBuffer(Memory, &bufferSize);

    __analysis_assume(bufferSize <= (ULONG)(-1));
    if (bufferSize > (ULONG)(-1))
    {
        status = STATUS_BUFFER_OVERFLOW;

        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, 
            "Buffer passed in longer than max ULONG, returning status code %!STATUS!\n", status);

        goto exit;        
    }
    
    brb->BufferSize = (ULONG) bufferSize;
    brb->ChannelHandle = Connection->ChannelHandle;
    brb->TransferFlags = TransferFlags;

    status = FormatRequestWithBrb(
        Connection->DevCtxHdr->IoTarget,
        Request,
        (PBRB) brb,
        sizeof(*brb)
        );
    
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    if (NULL == *Brb)
    {
        *Brb = brb;
    }
exit:
    if (!NT_SUCCESS(status) && brb && brbAllocatedLocally)
    {
        Connection->DevCtxHdr->ProfileDrvInterface.BthFreeBrb((PBRB)brb);
    }
    
    return status;
}
