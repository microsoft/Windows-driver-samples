//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "RequestManager.tmh"
#include "WMBClassTelemetry.h"

////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MBB_REQUEST_TIMEOUT_TOLERANCE_MS    5000    // 5 sec in milliseconds
#define MBB_REQUEST_TIMEOUT_MS              210000  //  210secs in milliseconds
#define MBB_REQUEST_TIMEOUT_100NS           ((LONGLONG)MBB_REQUEST_TIMEOUT_MS * 10000)
#define MBB_FRAGMENT_TIMEOUT_TOLERANCE_MS   100
#define MBB_FRAGMENT_TIMEOUT_MS             1250    // based on MBIM spec
#define MBB_FRAGMENT_TIMEOUT_100NS          ((LONGLONG)MBB_FRAGMENT_TIMEOUT_MS * 10000)


////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////
typedef
VOID
(*MBB_REQUEST_STATE_TRANSITION)(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );


////////////////////////////////////////////////////////////////////////////////
//
//  DECLARATIONS
//
////////////////////////////////////////////////////////////////////////////////

__callback
VOID
MbbReqMgrDrainCompleteCallback(
    __in PVOID Context
    );

//
// ASYNC CONTEXT MANAGEMENT
//

VOID
MbbReqMgrTimerCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    );

VOID
MbbReqMgrTimerRequestTimeoutCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    );

VOID
MbbReqMgrTimerFragmentTimeoutCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    );

VOID
MbbReqMgrCleanupRequest(
    __in __drv_freesMem(Mem) PMBB_REQUEST_CONTEXT Request
    );

VOID
MbbReqMgrReleaseRequestEvent(
    __in PMBB_EVENT_ENTRY       EventEntry
    );

NDIS_OID
MbbNdisGetOid(
    __in PNDIS_OID_REQUEST OidRequest
    );

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByKey(
    __in     PMBB_REQUEST_MANAGER    RequestManager,
    __in     ULONG                   ValidKeys,
    __in_opt ULONG                   RequestId,
    __in_opt ULONG                   TransactionId,
    __in_opt PNDIS_OID_REQUEST       OidRequest
    );

//
// REQUEST FSM
//

VOID
MbbReqMgrInitializeFsmTransitionTable( );

VOID
MbbReqFsmReady(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmDispatching(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmSendPending(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmSendComplete(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmResponseReceived(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmCancelled(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );

VOID
MbbReqFsmInvalid(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    );




////////////////////////////////////////////////////////////////////////////////
//
//  GLOBALS
//
////////////////////////////////////////////////////////////////////////////////
MBB_REQUEST_STATE MbbReqFsmTransitionTable[MbbRequestStateMaximum][MbbRequestEventMaximum];

MBB_REQUEST_STATE_TRANSITION MbbReqFsmStateHandlerTable[MbbRequestStateMaximum] = 
{
    MbbReqFsmReady,
    MbbReqFsmDispatching,
    MbbReqFsmSendPending,
    MbbReqFsmSendComplete,
    MbbReqFsmResponseReceived,
    MbbReqFsmResponseReceived,
    MbbReqFsmCancelled,
    MbbReqFsmInvalid
};

MBB_TIMER_CONTEXT GlobalTimerContexts[MbbTimerTypeMaximum] =
{
    {
        MbbTimerTypeRequest,
        0,
        NULL,
        MBB_REQUEST_TIMEOUT_MS,
        0,
        MBB_REQUEST_TIMEOUT_TOLERANCE_MS,
        MbbReqMgrTimerRequestTimeoutCallback,
        NULL
    },
    {
        MbbTimerTypeFragment,
        0,
        NULL,
        MBB_FRAGMENT_TIMEOUT_MS,
        0,
        MBB_FRAGMENT_TIMEOUT_TOLERANCE_MS,
        MbbReqMgrTimerFragmentTimeoutCallback,
        NULL
    }
};




////////////////////////////////////////////////////////////////////////////////
//
//  ASYNC REQUEST MANAGER
//
////////////////////////////////////////////////////////////////////////////////
#define KEY_REQUESTID       1
#define KEY_TRANSACTIONID   2
#define KEY_OIDID           4

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByKey(
    __in     PMBB_REQUEST_MANAGER    RequestManager,
    __in     ULONG                   ValidKeys,
    __in_opt ULONG                   RequestId,
    __in_opt ULONG                   TransactionId,
    __in_opt PNDIS_OID_REQUEST       OidRequest
    )
{
    PLIST_ENTRY          ListEntry;
    PMBB_REQUEST_CONTEXT Request = NULL;

    MbbReqMgrLockManager( RequestManager );
    for( ListEntry  = RequestManager->AllocatedRequestList.Flink;
         ListEntry != &RequestManager->AllocatedRequestList;
         ListEntry  = ListEntry->Flink )
    {
        Request = CONTAINING_RECORD(
                    ListEntry,
                    MBB_REQUEST_CONTEXT,
                    ManagerLink
                    );
        if( ((ValidKeys & KEY_REQUESTID) && (Request->RequestId == RequestId)) ||
            ((ValidKeys & KEY_OIDID) && (OidRequest != NULL) && (Request->OidContext.OidRequest == OidRequest)) ||
            ((ValidKeys & KEY_TRANSACTIONID) && (Request->TransactionId == TransactionId)) )
        {
            MbbReqMgrRefRequest( Request );
            break;
        }
        else
        {
            Request = NULL;
        }
    }
    MbbReqMgrUnlockManager( RequestManager );

    return Request;
}

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestById(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    ULONG                   RequestId
    )
{
    return MbbReqMgrGetRequestByKey(
                RequestManager,
                KEY_REQUESTID,
                RequestId,
                0,
                NULL
                );
}

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByTransactionId(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    ULONG                   TransactionId
    )
{
    return MbbReqMgrGetRequestByKey(
                RequestManager,
                KEY_TRANSACTIONID,
                0,
                TransactionId,
                NULL
                );
}

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByOidRequestId(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    PNDIS_OID_REQUEST       OidRequestId
    )
{
    return MbbReqMgrGetRequestByKey(
                RequestManager,
                KEY_OIDID,
                0,
                0,
                OidRequestId
                );
}

MBB_PROTOCOL_HANDLE
MbbReqMgrGetAdapterHandle(
    __in    PMBB_REQUEST_CONTEXT    Request
    )
{
    return (MBB_PROTOCOL_HANDLE)(Request->RequestManager->AdapterContext);
}

BOOLEAN
MbbReqMgrIsInternalRequest(
    __in    PMBB_REQUEST_CONTEXT    Request
    )
{
    if( Request->OidContext.OidRequest == NULL &&
        MbbReqMgrIsUnsolicitedIndication( Request ) == FALSE )
        return TRUE;
    else
        return FALSE;
}

VOID
MbbReqMgrSetUnsolicitedIndication(
    __in    PMBB_REQUEST_CONTEXT    Request
    )
{
    Request->HandlerContext.IsIndication = TRUE;
}

BOOLEAN
MbbReqMgrIsUnsolicitedIndication(
    __in    PMBB_REQUEST_CONTEXT    Request
    )
{
    return Request->HandlerContext.IsIndication;
}

_Requires_lock_not_held_(&(Request->RequestManager->Spinlock))
VOID
MbbReqMgrGetState(
    __in    PMBB_REQUEST_CONTEXT    Request,
    __inout MBB_REQUEST_STATE      *pCurrentState,
    __inout MBB_REQUEST_STATE      *pLastState
    )
{

    PMBB_REQUEST_MANAGER    RequestManager = NULL;

    RequestManager  = Request->RequestManager;

    if(RequestManager)
    {
        MbbReqMgrLockManager( RequestManager );

        if(pCurrentState)
        {
            *pCurrentState = Request->State;
        }

        if(pLastState)
        {
            *pLastState = Request->LastState;
        }
        
        MbbReqMgrUnlockManager( RequestManager );
    }
}

BOOLEAN
MbbReqMgrIsSetOid(
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    BOOLEAN IsOidRequest = ! MbbReqMgrIsInternalRequest( Request );

    return ( IsOidRequest && Request->OidContext.IsSetOid );
}

MBB_NDIS_OID_STATE
MbbReqMgrGetSetOidState(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_NDIS_OID_STATE     OidState
    )
/*++
Description
    Sets the new oid state and returns the old state.
    Oid state is a enum so a interlocked exchange as a long is fine.

Return
    Old Oid state.
--*/
{
    LONG Long;

    Long = InterlockedExchange(
            (volatile LONG*)(&Request->OidContext.OidState),
            OidState
            );

    return (MBB_NDIS_OID_STATE)Long;
}

_Acquires_lock_( RequestManager->Spinlock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, RequestManager )
VOID
MbbReqMgrLockManager(
    __in PMBB_REQUEST_MANAGER RequestManager
    )
{
    NdisAcquireSpinLock( &RequestManager->Spinlock );
}

_Releases_lock_( RequestManager->Spinlock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, RequestManager )
VOID
MbbReqMgrUnlockManager(
    __in PMBB_REQUEST_MANAGER RequestManager
    )
{
    NdisReleaseSpinLock( &RequestManager->Spinlock );
}

VOID
MbbReqMgrIntProcessNextRequestWorker(
    __in PVOID  Context1,
    __in PVOID  Context2,
    __in PVOID  Context3,
    __in PVOID  Context4
    )
{
    PMBB_REQUEST_CONTEXT    NextRequest = NULL;
    PMBB_REQUEST_MANAGER    RequestManager = (PMBB_REQUEST_MANAGER)Context1;

    MbbReqMgrLockManager( RequestManager );
    if( ! IsListEmpty( &(RequestManager->PendingRequestQueue) ) )
    {
        NextRequest = CONTAINING_RECORD(
                            RemoveHeadList( &(RequestManager->PendingRequestQueue) ),
                            MBB_REQUEST_CONTEXT,
                            ReqMgrContext.QueueLink
                            );
        InitializeListHead( &NextRequest->ReqMgrContext.QueueLink );
        NextRequest->ReqMgrContext.IsQueued = FALSE;
    }
    RequestManager->CurrentRequest = NextRequest;
    MbbReqMgrUnlockManager( RequestManager );

    if( NextRequest != NULL )
    {
        MbbReqMgrQueueEvent(
            RequestManager,
            NextRequest,
            MbbRequestEventStart,
            NULL,
            0
            );
        //
        // Ref taken when queuing the request in the PendingRequestQueue
        //
        MbbReqMgrDerefRequest( NextRequest );
    }
}

VOID
MbbReqMgrIntProcessNextRequest(
    __in    PMBB_REQUEST_MANAGER    RequestManager
    )
/*++
Description
    If a request is pending to be dispatched
    this routine will queue a work item to start
    the request on another thread. If there are
    no more requests it returns enabling the
    request manager serializer to process the
    next request.
--*/
{
    NDIS_STATUS NdisStatus;

    MbbReqMgrLockManager( RequestManager );
    if( IsListEmpty( &(RequestManager->PendingRequestQueue) ) )
    {
        RequestManager->CurrentRequest = NULL;

        TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr] No more requests, idling queue. QueueStatus[Current=0x%p %s]",
                    RequestManager->CurrentRequest,
                    IsListEmpty( &RequestManager->PendingRequestQueue )? "EMPTY": "NON-EMPTY"
                    );
    }
    else
    {
        if( (NdisStatus = MbbWorkMgrQueueWorkItem(
                                RequestManager->WorkItemManagerHandle,
                                RequestManager,
                                NULL,
                                NULL,
                                NULL,
                                MbbReqMgrIntProcessNextRequestWorker
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to queue work item to process request with status=%!status!. Requests may hang", NdisStatus );
        }
    }
    MbbReqMgrUnlockManager( RequestManager );
}

VOID
MbbReqMgrTransition(
    __in PMBB_EVENT_ENTRY   EventEntry
    )
{
    PMBB_REQUEST_CONTEXT    Request;
    PMBB_REQUEST_MANAGER    RequestManager;
    MBB_REQUEST_STATE       OldState = {0};
    MBB_REQUEST_STATE       NewState = MbbRequestStateMaximum;

    Request         = EventEntry->Request;
    RequestManager  = Request->RequestManager;

    MbbReqMgrLockManager( RequestManager );
    if( (Request->State < MbbRequestStateMaximum && Request->State >= 0) &&
        (EventEntry->Event < MbbRequestEventMaximum && EventEntry->Event >= 0) )
    {
        NewState = MbbReqFsmTransitionTable[Request->State][EventEntry->Event];
        TraceInfo(  WMBCLASS_REQUEST_FSM, "[ReqFsm][ReqId=0x%04x] Transition: %!MbbRequestState! -> %!MbbRequestState! event=%!MbbRequestEvent!",
                    Request->RequestId, Request->State, NewState, EventEntry->Event
                    );
        OldState = Request->State;
        Request->State     = NewState;
        Request->LastState = OldState;
        Request->LastEvent = EventEntry->Event;
    }
    MbbReqMgrUnlockManager( RequestManager );

    if( NewState < MbbRequestStateMaximum )
    {
        MbbReqFsmStateHandlerTable[NewState](
            Request,
            OldState,
            EventEntry
            );
    }
}

FORCEINLINE
BOOLEAN
MbbReqMgrRef(
    __in PMBB_REQUEST_MANAGER RequestManager
    )
{
    return DrainAddRef( &RequestManager->DrainObject );
}

FORCEINLINE
VOID
MbbReqMgrDeref(
    __in PMBB_REQUEST_MANAGER RequestManager
    )
{
    DrainRelease( &RequestManager->DrainObject );
}

NDIS_STATUS
MbbReqMgrInitialize(
    __in PMBB_REQUEST_MANAGER       RequestManager,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    ULONG                       BufferLength;
    NTSTATUS                    NtStatus;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_RESOURCES;
    MBB_ALLOC_FLAGS             Flags = { 0 };
    NDIS_TIMER_CHARACTERISTICS  TimerCharacteristics;

    do
    {
        InitializeListHead( &(RequestManager->AllocatedRequestList) );

        InitializeListHead( &(RequestManager->PendingRequestQueue) );

        NdisAllocateSpinLock( &(RequestManager->Spinlock) );

        KeInitializeEvent(
            &RequestManager->DrainCompleteEvent,
            NotificationEvent,
            FALSE
            );

        InitDrainObject(
            &RequestManager->DrainObject,
            MbbReqMgrDrainCompleteCallback,
            &RequestManager->DrainCompleteEvent
             );

        DrainComplete( &RequestManager->DrainObject );

        MbbReqMgrInitializeFsmTransitionTable( );

        KeInitializeEvent(
            &(RequestManager->NoAllocatedRequestEvent),
            NotificationEvent,
            TRUE
            );
        RequestManager->AdapterContext  = Adapter;
        //
        // Work item manager
        //
        if( (RequestManager->WorkItemManagerHandle = MbbWorkMgrInitialize( MBB_PREALLOC_WORKITEM_COUNT )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to initialize work item manager" );
            break;
        }
        //
        // Allocation manager for requests.
        //
        if( (RequestManager->RequestAllocatorHandle = MbbAllocMgrInitialize(
                                                        sizeof(MBB_REQUEST_CONTEXT),
                                                        MBB_PREALLOC_REQUEST_COUNT,
                                                        MbbPoolTagRequest,
                                                        Flags
                                                        )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to initialize request allocator" );
            break;
        }
        //
        // Allocate fragment buffers
        //
        RequestManager->Fragmentation.ControlFragmentLength = Adapter->BusParams.FragmentSize;
        RequestManager->Fragmentation.BulkFragmentLength    = MBB_FRAGMENATION_BULK_BUFFER_LENGTH;

        BufferLength = MAX(
                            RequestManager->Fragmentation.ControlFragmentLength,
                            RequestManager->Fragmentation.BulkFragmentLength
                            );
        if( (RequestManager->Fragmentation.BufferManager = MbbBufMgrInitialize(
                                                                MBB_PREALLOC_FRAGMENATION_BUFFER_COUNT,
                                                                BufferLength
                                                                )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate %d fragment buffers of size=%dBytes",
                        MBB_PREALLOC_FRAGMENATION_BUFFER_COUNT,
                        BufferLength
                        );
            break;
        }
        //
        // Allocate default response buffer.
        //
        RequestManager->Fragmentation.Reassemble.FragmentBufferLength = Adapter->BusParams.FragmentSize;
        if( (RequestManager->Fragmentation.Reassemble.FragmentBuffer = ALLOCATE_NONPAGED_POOL( RequestManager->Fragmentation.Reassemble.FragmentBufferLength )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate default fragment buffer of size=%dBytes", RequestManager->Fragmentation.Reassemble.FragmentBufferLength );
            break;
        }
        //
        // Allocate default reassemble buffer.
        //
        RequestManager->Fragmentation.Reassemble.BufferLength = MBB_REASSEMBLE_BUFFER_LENGTH;
        if( (RequestManager->Fragmentation.Reassemble.Buffer = ALLOCATE_NONPAGED_POOL( RequestManager->Fragmentation.Reassemble.BufferLength )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate default fragment buffer of size=%dBytes", RequestManager->Fragmentation.Reassemble.BufferLength );
            break;
        }
        //
        // Allocate default timer.
        //
        if( (NdisStatus = MbbReqMgrTimerInitialize(
                            RequestManager,
                            MbbTimerTypeRequest
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate %!MbbTimer! with status=%!status!", MbbTimerTypeRequest, NdisStatus );
            break;
        }

        if( (NdisStatus = MbbReqMgrTimerInitialize(
                            RequestManager,
                            MbbTimerTypeFragment
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate %!MbbTimer! with status=%!status!", MbbTimerTypeFragment, NdisStatus );
            break;
        }

        RequestManager->IsClosed = FALSE;

        NdisStatus = NDIS_STATUS_SUCCESS;

    } while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        MbbReqMgrTimerCleanup( RequestManager, MbbTimerTypeFragment );
        MbbReqMgrTimerCleanup( RequestManager, MbbTimerTypeRequest  );

        if( RequestManager->Fragmentation.Reassemble.Buffer != NULL )
        {
            FREE_POOL( RequestManager->Fragmentation.Reassemble.Buffer );
        }
        if( RequestManager->Fragmentation.Reassemble.FragmentBuffer != NULL )
        {
            FREE_POOL( RequestManager->Fragmentation.Reassemble.FragmentBuffer );
        }
        if( RequestManager->Fragmentation.BufferManager != NULL )
        {
            MbbBufMgrCleanup( RequestManager->Fragmentation.BufferManager );
            RequestManager->Fragmentation.BufferManager = NULL;
        }
        if( RequestManager->RequestAllocatorHandle != NULL )
        {
            MbbAllocMgrCleanup( RequestManager->RequestAllocatorHandle );
            RequestManager->RequestAllocatorHandle = NULL;
        }
        if( RequestManager->WorkItemManagerHandle != NULL )
        {
            MbbWorkMgrCleanup( RequestManager->WorkItemManagerHandle );
            RequestManager->WorkItemManagerHandle = NULL;
        }
    }
    else
    {
        TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] Initialization complete" );
    }
    return NdisStatus;
}

__callback
VOID
MbbReqMgrDrainCompleteCallback(
    __in PVOID Context
    )
{
    PKEVENT DrainCompleteEvent = (PKEVENT)Context;

    KeSetEvent( DrainCompleteEvent, IO_NO_INCREMENT, FALSE );
}

VOID
MbbReqMgrCleanup(
    __in PMBB_REQUEST_MANAGER RequestManager
    )
{
    NTSTATUS                NtStatus;
    BOOLEAN                 SignalThread = FALSE;
    BOOLEAN                 WaitForTimers = FALSE;
    PVOID                   EventThreadObject;

    //
    // Synchronize draining with new request creation.
    // Once draining is started inside the lock
    // it is guaranteed that no more requests would be created.
    //
    MbbReqMgrLockManager( RequestManager );
    KeResetEvent( &RequestManager->DrainCompleteEvent );
    StartDrain( &RequestManager->DrainObject );
    MbbReqMgrUnlockManager( RequestManager );
    //
    // Cancel all pending requests by queuing cancel event.
    //
    MbbReqMgrCancelRequests( RequestManager, FALSE);

    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Cleanup] Waiting for pending requests to complete" );
    //
    // Wait for pending operations to complete.
    //
    KeWaitForSingleObject(
        &(RequestManager->NoAllocatedRequestEvent),
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Cleanup] Waiting for references to go down" );
    //
    // Drop the reference that was taken to pass in the request manager.
    //
    MbbReqMgrDeref( RequestManager );
    //
    // Wait for all references on request manager to go down.
    // This guarantees that no thread is accessign the request
    // manager while its wiped out here.
    //
    KeWaitForSingleObject(
        &(RequestManager->DrainCompleteEvent),
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
    //
    // Wait for the lock before cleaning up so that the 
    // thread that set the event has a change to release the lock.
    //
    MbbReqMgrLockManager( RequestManager );
    //
    // Ensure all lists are empty
    //
    ASSERT( IsListEmpty( &(RequestManager->AllocatedRequestList) ) );
    ASSERT( IsListEmpty( &(RequestManager->PendingRequestQueue) ) );

    MbbReqMgrUnlockManager( RequestManager );
    //
    // Cancel timers
    //
    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Cleanup] Cancelling timers" );
    MbbReqMgrTimerCleanup( RequestManager, MbbTimerTypeFragment );
    MbbReqMgrTimerCleanup( RequestManager, MbbTimerTypeRequest  );
    //
    // Free resources, if allocated.
    //
    if( RequestManager->Fragmentation.Reassemble.Buffer != NULL )
    {
        FREE_POOL( RequestManager->Fragmentation.Reassemble.Buffer );
    }
    if( RequestManager->Fragmentation.Reassemble.FragmentBuffer != NULL )
    {
        FREE_POOL( RequestManager->Fragmentation.Reassemble.FragmentBuffer );
    }
    if( RequestManager->Fragmentation.BufferManager != NULL )
    {
        MbbBufMgrCleanup( RequestManager->Fragmentation.BufferManager );
        RequestManager->Fragmentation.BufferManager = NULL;
    }
    if( RequestManager->RequestAllocatorHandle != NULL )
    {
        MbbAllocMgrCleanup( RequestManager->RequestAllocatorHandle );
        RequestManager->RequestAllocatorHandle = NULL;
    }
    if( RequestManager->WorkItemManagerHandle != NULL )
    {
        MbbWorkMgrCleanup( RequestManager->WorkItemManagerHandle );
        RequestManager->WorkItemManagerHandle = NULL;
    }
    //
    // Always allocated
    //
    NdisFreeSpinLock( &(RequestManager->Spinlock) );
    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Cleanup] Cleanup complete" );
}

/// <summary>
/// Function to Cancel all the Requests in the Request Manager
/// </summary>
/// <param name="RequestManager">Pointer to the Request Manager</param>
/// <param name="IsClosed">Flag to indicate that the Request Manager is Closed. Set only in the Surprise Removal Path</param>
VOID
MbbReqMgrCancelRequests(
    __in PMBB_REQUEST_MANAGER    RequestManager,
    __in BOOLEAN                 IsClosed 
    )
{
    LIST_ENTRY              TempList;
    PLIST_ENTRY             ListEntry;
    PMBB_REQUEST_CONTEXT    Request;

    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] Cancelling pending requests" );

    InitializeListHead( &TempList );

    MbbReqMgrLockManager( RequestManager );
    RequestManager->IsClosed = IsClosed;
    for( ListEntry  =  RequestManager->AllocatedRequestList.Flink;
         ListEntry != &RequestManager->AllocatedRequestList;
         ListEntry  =  ListEntry->Flink )
    {
        Request = CONTAINING_RECORD( ListEntry, MBB_REQUEST_CONTEXT, ManagerLink );
        MbbReqMgrRefRequest( Request );
        InsertTailList( &TempList, &Request->ReqMgrContext.CancelLink );
    }
    MbbReqMgrUnlockManager( RequestManager );

    while (!IsListEmpty(&TempList))
    {
        // Get the Entry of the list
        ListEntry = RemoveHeadList(&TempList);
        Request = CONTAINING_RECORD(ListEntry, MBB_REQUEST_CONTEXT, ReqMgrContext.CancelLink);

        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventCancel,
            (PVOID)(NDIS_STATUS_REQUEST_ABORTED),
            0
            );
        MbbReqMgrDerefRequest(Request);
    }
}

/// <summary>
/// Function to Create the Request for the Request Manager off the OID Request that came through
/// </summary>
/// <param name="RequestManager">Request Manager</param>
/// <param name="OidRequest">The oid request.</param>
/// <param name="ResponseBufferLength">Length of the response buffer.</param>
/// <param name="NdisStatus">out parameter that has the NDIS status filled in</param>
/// <returns>newly created request otherwise null</returns>
__checkReturn
__drv_aliasesMem
__drv_allocatesMem(Mem)
PMBB_REQUEST_CONTEXT
MbbReqMgrCreateRequest(
    __in     PMBB_REQUEST_MANAGER    RequestManager,
    __in_opt PNDIS_OID_REQUEST       OidRequest,
    __in     ULONG                   ResponseBufferLength,
    __out    PNDIS_STATUS            NdisStatus
    )
{
    PMBB_REQUEST_CONTEXT        Request = NULL;
    NDIS_TIMER_CHARACTERISTICS  TimerCharacteristics;

    do
    {
        *NdisStatus = NDIS_STATUS_RESOURCES;

        if( (Request = (PMBB_REQUEST_CONTEXT) MbbAllocMgrAllocate( RequestManager->RequestAllocatorHandle )) == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to allocate request context" );
            break;
        }
        RtlZeroMemory( Request, sizeof(MBB_REQUEST_CONTEXT) );
        Request->OidContext.OidRequest = OidRequest;
        if (OidRequest != NULL)
        {
            Request->OidContext.OidRequestId       = OidRequest->RequestId;
            Request->OidContext.OidRequestHandle   = OidRequest->RequestHandle;
        }
        InitializeListHead( &(Request->ManagerLink) );
        InitializeListHead( &(Request->ReqMgrContext.QueueLink) );
        InitializeListHead( &(Request->ReqMgrContext.CancelLink) );
        InitializeListHead( &(Request->ReqMgrContext.TimeoutLink) );
        //
        // Insert the request in allocated list for tracking.
        // Take the reference on the request manager since a pointer
        // is stored in the request. A successful reference also
        // guarantees that the request manager is not draining.
        //
        MbbReqMgrLockManager( RequestManager );

        // If the Request Manager is Closed, then no point in inserting the requests in to the Queue
        // Currently this code path will get hit for Surprise Removal
        if (RequestManager->IsClosed)
        {
            TraceError(WMBCLASS_REQUEST_MANAGER, "[ReqMgr] Not Queuing up the Requests as the Request Manager is Closed");
            *NdisStatus = NDIS_STATUS_NOT_ACCEPTED;
            MbbReqMgrUnlockManager(RequestManager);
            break;
        }

        if( MbbReqMgrRef( RequestManager ) )
        {
            Request->RequestId = (RequestManager->RequestIdCounter)++;
            Request->RequestManager = RequestManager;
            KeResetEvent( &(RequestManager->NoAllocatedRequestEvent) );
            InsertTailList(
                &RequestManager->AllocatedRequestList,
                &Request->ManagerLink
                );
        }

        else
        {
            TraceError(WMBCLASS_REQUEST_MANAGER, "[ReqMgr] FAILED to reference the RequestManager for creating a RequestContext");
            *NdisStatus = NDIS_STATUS_RESOURCES;
            MbbReqMgrUnlockManager(RequestManager);
            break;
        }

        KeInitializeEvent(
            &Request->WaitEvent,
            NotificationEvent,
            FALSE
            );

        if (OidRequest != NULL)
        {
            if (OidRequest->RequestType == NdisRequestSetInformation)
            {
                Request->OidContext.IsSetOid = TRUE;
            }
            TraceInfo(WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] Request created for %s [RequestContext=0x%p OidRequest=0x%p] SET=%!BOOLEAN!",
                Request->RequestId, GetOidName(MbbNdisGetOid(OidRequest)), Request, OidRequest, MbbReqMgrIsSetOid(Request)
                );
        }
        else
        {
            TraceInfo(WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] Internal Request created [RequestContext=0x%p]",
                Request->RequestId, Request
                );
        }
        Request->ReferenceCount = 1;

        MbbReqMgrUnlockManager( RequestManager );

        *NdisStatus = NDIS_STATUS_SUCCESS;

    } while( FALSE );

    if( Request != NULL )
    {
        if( *NdisStatus != NDIS_STATUS_SUCCESS)
        {
            if(Request->RequestManager)
            {
                MbbReqMgrDeref(Request->RequestManager);
            }
            MbbAllocMgrFree( Request );

            Request = NULL;
        }
    }
    
    return Request;
}

#pragma warning(push)
#pragma warning(disable:__WARNING_MEMORY_LEAK)
VOID
MbbReqMgrCleanupRequest(
    __in __drv_freesMem(Mem) PMBB_REQUEST_CONTEXT Request
    )
{
    PMBB_REQUEST_MANAGER RequestManager = Request->RequestManager;

    if(!RequestManager)
    {
        TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] MbbReqMgrCleanupRequest: RequestManager is  NULL.", Request->RequestId );
        ASSERT(FALSE);
        return;
    }

    ASSERT( Request->ReferenceCount == 0 );

    //
    // If the request allocation failed directly free the request memory.
    //

    MbbReqMgrLockManager( RequestManager );
    
    if( IsListEmpty( &(Request->ManagerLink) ) )
    {       
        TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] Request double free attempt.", Request->RequestId );
        ASSERT(FALSE);
        MbbReqMgrUnlockManager( RequestManager );
        return;
    }
    else
    {
        TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] Destroying request.", Request->RequestId );
        
        RemoveEntryList( &(Request->ManagerLink) );
        
        // Clear the manager link flink and blink so that if IsListEmpty
        // check is called again on the same link, it returns empty.
        InitializeListHead( &(Request->ManagerLink) );

        if( IsListEmpty( &(RequestManager->AllocatedRequestList) ) )
        {
            //
            // The event can trigger the cleanup code which
            // could cleanup the lock making it invalid. But
            // the cleanup code waits for the lock before cleaning
            // the request manager. So the lock is guaranteed to
            // be valid until we release it.
            //
            KeSetEvent(
                &(RequestManager->NoAllocatedRequestEvent),
                IO_NO_INCREMENT,
                FALSE
                );
            //
            // Disarm the request timeout timer if its still active.
            //
            MbbReqMgrTimerDisarm(
                RequestManager,
                MbbTimerTypeRequest,
                MBB_LOCK_TAKEN
                );
        }
        
        MbbReqMgrUnlockManager( RequestManager );
        MbbAllocMgrFree( Request );
        
        //
        // Do not touch the request manager beyond this point.
        //       

        if( RequestManager )
            MbbReqMgrDeref( RequestManager );
    }
}
#pragma warning(pop)

VOID
MbbNdisIndicateStatus(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

VOID
MbbReqMgrDestroyRequest(
    __in PMBB_REQUEST_MANAGER RequestManager,
    __in PMBB_REQUEST_CONTEXT Request
    )
{
    //
    // Drop the initial reference taken during initialization.
    //
    MbbReqMgrDerefRequest( Request );
}

VOID
MbbReqMgrRefRequest(
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    LONG ref = InterlockedIncrement( &Request->ReferenceCount );

    // When the request is created its initial ref count is 1. Hence whenever
    // the request is going to be ref'd the ref count should be > 1. 
    // This assert will catch the ref count to be going from 0 to 1, meaning
    // that some thread is trying to use a freed request.
    
    ASSERT(ref > 1);
}

VOID
MbbReqMgrDerefRequest(
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    LONG ref = InterlockedDecrement( &Request->ReferenceCount );

    // Catches negative dereferences
    ASSERT(ref >= 0);
    
    if( ref == 0 )
    {
        MbbReqMgrCleanupRequest( Request );
    }
}

NDIS_STATUS
MbbReqMgrDispatchRequest(
    __in        PMBB_REQUEST_CONTEXT            Request,
    __in        BOOLEAN                         IsSerialized,
    __in        MBB_REQUEST_DISPATCH_ROUTINE    DispatchRoutine,
    __in        MBB_REQUEST_COMPLETION_CALLBACK CompletionCallback,
    __in_opt    MBB_REQUEST_RESPONSE_HANDLER    ResponseHandler
    )
/*++
    Return Value
        NDIS_STATUS_PENDING
            Request was queued for later dispatch.
        NDIS_STATUS_SUCCESS
            Request was dispatched synchronously and successfully complete.
        Ndis failure codes
            Request was either not dispatched or completed with failure.
--*/
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    LARGE_INTEGER   DueTime;

    do
    {
        if( Request == NULL || DispatchRoutine == NULL )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] Invalid parameters, Request=0x%p DispatchRoutine=0x%p", Request, DispatchRoutine );
            NdisStatus = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        Request->ReqMgrContext.IsSerialized       = IsSerialized;
        Request->ReqMgrContext.DispatchRoutine    = DispatchRoutine;
        Request->ReqMgrContext.CompletionCallback = CompletionCallback;
        Request->ReqMgrContext.ResponseHandler    = ResponseHandler;
        Request->ReqMgrContext.IsQueued           = FALSE;

        NdisGetSystemUpTimeEx( (PLARGE_INTEGER)(&Request->DispatchTime) );       

        MbbReqMgrQueueEvent(
            Request->RequestManager,
            Request,
            MbbRequestEventDispatch,
            NULL,
            0
            );
        NdisStatus = NDIS_STATUS_PENDING;

    } while( FALSE );

    return NdisStatus;
}

VOID
MbbReqMgrInitializeFsmTransitionTable( )
{
    ULONG StateIndex;
    ULONG EventIndex;

    for( StateIndex = 0; StateIndex < MbbRequestStateMaximum; StateIndex++ )
    {
        for( EventIndex = 0; EventIndex < MbbRequestEventMaximum; EventIndex++ )
        {
            MbbReqFsmTransitionTable[StateIndex][EventIndex] = MbbRequestStateInvalid;
        }
    }
    //
    // Valid State transitions.
    //
    MbbReqFsmTransitionTable[MbbRequestStateReady][MbbRequestEventDispatch]                                     = MbbRequestStateDispatching;
    MbbReqFsmTransitionTable[MbbRequestStateReady][MbbRequestEventCancel]                                       = MbbRequestStateCancelled;

    MbbReqFsmTransitionTable[MbbRequestStateDispatching][MbbRequestEventStart]                                  = MbbRequestStateSendPending;
    MbbReqFsmTransitionTable[MbbRequestStateDispatching][MbbRequestEventCancel]                                 = MbbRequestStateCancelled;

    MbbReqFsmTransitionTable[MbbRequestStateSendPending][MbbRequestEventCancel]                                 = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateSendPending][MbbRequestEventSendComplete]                           = MbbRequestStateSendComplete;
    MbbReqFsmTransitionTable[MbbRequestStateSendPending][MbbRequestEventResponseReceived]                       = MbbRequestStateResponseReceived;
    MbbReqFsmTransitionTable[MbbRequestStateSendPending][MbbRequestEventResponseReceivedMoreData]               = MbbRequestStateResponseReceivedMoreData;

    MbbReqFsmTransitionTable[MbbRequestStateSendComplete][MbbRequestEventCancel]                                = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateSendComplete][MbbRequestEventResponseReceived]                      = MbbRequestStateResponseReceived;
    MbbReqFsmTransitionTable[MbbRequestStateSendComplete][MbbRequestEventResponseReceivedMoreData]              = MbbRequestStateResponseReceivedMoreData;

    MbbReqFsmTransitionTable[MbbRequestStateResponseReceived][MbbRequestEventCancel]                            = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateResponseReceived][MbbRequestEventSendComplete]                      = MbbRequestStateResponseReceived;

    MbbReqFsmTransitionTable[MbbRequestStateResponseReceivedMoreData][MbbRequestEventCancel]                    = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateResponseReceivedMoreData][MbbRequestEventSendComplete]              = MbbRequestStateResponseReceivedMoreData;
    MbbReqFsmTransitionTable[MbbRequestStateResponseReceivedMoreData][MbbRequestEventResponseReceived]          = MbbRequestStateResponseReceived;
    MbbReqFsmTransitionTable[MbbRequestStateResponseReceivedMoreData][MbbRequestEventResponseReceivedMoreData]  = MbbRequestStateResponseReceivedMoreData;

    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventDispatch]                                 = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventStart]                                    = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventCancel]                                   = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventSendComplete]                             = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventResponseReceived]                         = MbbRequestStateCancelled;
    MbbReqFsmTransitionTable[MbbRequestStateCancelled][MbbRequestEventResponseReceivedMoreData]                 = MbbRequestStateCancelled;
}

VOID
MbbReqMgrReleaseRequestEvent(
    __in PMBB_EVENT_ENTRY EventEntry
    )
{
    PMBB_REQUEST_CONTEXT    Request = EventEntry->Request;
    PMBB_REQUEST_MANAGER    RequestManager = Request->RequestManager;

    //
    // If this is a pre-allocated event hold the manager
    // lock to synchronize access. If allocated from pool
    // no one else can access it.
    //
    if( EventEntry->EventFlags.Allocated == TRUE )
    {
        FREE_POOL( EventEntry );
    }
    else
    {
        MbbReqMgrLockManager( RequestManager );
        EventEntry->EventFlags.InUse = FALSE;
        MbbReqMgrUnlockManager( RequestManager );
    }
    MbbReqMgrDerefRequest( Request );
}

_Requires_lock_not_held_( RequestManager->Spinlock )
VOID
MbbReqMgrQueueEvent(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_EVENT      Event,
    __in_opt PVOID              EventData,
    __in ULONG                  EventDataLength
    )
{
    BOOLEAN             AllocateEvent = FALSE;
    PMBB_EVENT_ENTRY    EventEntry;

    //
    // Try to use the pre-allocated events for forward progress.
    // Some events can occur multiple times at the same instance like ResponseReceived.
    // For these events try to allocate a new entry.
    //
    MbbReqMgrLockManager( RequestManager );
    EventEntry = &(Request->EventEntries[Event]);
    if( EventEntry->EventFlags.InUse == TRUE )
        AllocateEvent = TRUE;
    else
        EventEntry->EventFlags.InUse = TRUE;
    MbbReqMgrUnlockManager( RequestManager );

    if( AllocateEvent )
    {
#pragma prefast(suppress: __WARNING_MEMORY_LEAK, "Released by MbbReqMgrReleaseRequestEvent")
        if( (EventEntry = ALLOCATE_NONPAGED_POOL( sizeof(MBB_EVENT_ENTRY) )) == NULL )
            TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqID=0x%04x] FAILED to queue event=%!MbbRequestEvent!, unable to allocate entry", Request->RequestId, Event );
        else
            EventEntry->EventFlags.Allocated = TRUE;
    }

    if( EventEntry != NULL )
    {
        EventEntry->Event           = Event;
        EventEntry->EventData       = EventData;
        EventEntry->EventDataLength = EventDataLength;
        EventEntry->Request         = Request;
        //
        // Reference the request since the event stores a pointer to it.
        //
        MbbReqMgrRefRequest( Request );
    }

    MbbReqMgrTransition( EventEntry );

    MbbReqMgrReleaseRequestEvent( EventEntry );
}

// Reassemble context

_Requires_lock_not_held_( RequestManager->Spinlock )
PMBB_REASSEMBLE_CONTEXT
MbbReqMgrAcquireSharedReassembleContext(
    __in PMBB_REQUEST_MANAGER                               RequestManager,
    __in_bcount(MessageLength) PMBB_COMMAND_FRAGMENT_HEADER MessageFragmentHeader,
    __in ULONG                                              MessageLength,
    __in GUID                                               ActivityId
    )
{
    MBB_COMMAND                 Command = { 0 };
    PMBB_COMMAND_DONE_HEADER    CommandDone;
    PMBB_INDICATE_STATUS_HEADER IndicationStatus;
    PMBB_REASSEMBLE_CONTEXT     ReassembleContext = &RequestManager->Fragmentation.Reassemble;

    //
    // Get the command for debugging purposes
    //
    if( MessageFragmentHeader->FragmentHeader.CurrentFragment == 0 )
    {
        if( MessageFragmentHeader->MessageHeader.MessageType == MBB_MESSAGE_TYPE_COMMAND_DONE &&
            MessageFragmentHeader->MessageHeader.MessageLength >= sizeof(MBB_COMMAND_DONE_HEADER) &&
            MessageLength >= sizeof(MBB_COMMAND_DONE_HEADER) )
        {
            CommandDone = (PMBB_COMMAND_DONE_HEADER)MessageFragmentHeader;
            Command.CommandId = CommandDone->Command.CommandId;
            MBB_UUID_TO_HOST(
                &Command.ServiceId,
                &CommandDone->Command.ServiceId
                );
        }
        else
        if( MessageFragmentHeader->MessageHeader.MessageType == MBB_MESSAGE_TYPE_INDICATE_STATUS &&
            MessageFragmentHeader->MessageHeader.MessageLength >= sizeof(MBB_INDICATE_STATUS_HEADER) &&
            MessageLength >= sizeof(MBB_INDICATE_STATUS_HEADER) )
        {
            IndicationStatus = (PMBB_INDICATE_STATUS_HEADER)MessageFragmentHeader;
            Command.CommandId = IndicationStatus->Command.CommandId;
            MBB_UUID_TO_HOST(
                &Command.ServiceId,
                &IndicationStatus->Command.ServiceId
                );
        }
    }

    MbbReqMgrLockManager( RequestManager );

    if( RequestManager->Fragmentation.ReassembleInUse == TRUE )
    {
        if( ReassembleContext->TransactionId == MessageFragmentHeader->MessageHeader.MessageTransactionId &&
            ReassembleContext->FragmentCount == MessageFragmentHeader->FragmentHeader.TotalFragments &&
            ReassembleContext->NextFragment  == MessageFragmentHeader->FragmentHeader.CurrentFragment )
        {
            TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][TID=0x%08x] Using owned reassemble context", MessageFragmentHeader->MessageHeader.MessageTransactionId );
        }
        else
        {
            TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][TID=0x%08x] FAILED to acquire reassemble context, owned by TID=0x%08x, Command=%s",
                        MessageFragmentHeader->MessageHeader.MessageTransactionId,
                        ReassembleContext->TransactionId,
                        MbbUtilGetCommandString( &Command )
                        );
            ReassembleContext = NULL;
        }
    }
    else
    {
        if( MessageFragmentHeader->FragmentHeader.CurrentFragment == 0 )
        {
            TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][TID=0x%08x] Acquired reassemble context", MessageFragmentHeader->MessageHeader.MessageTransactionId );

            ReassembleContext->Command       = Command;
            ReassembleContext->TransactionId = MessageFragmentHeader->MessageHeader.MessageTransactionId;
            ReassembleContext->FragmentCount = MessageFragmentHeader->FragmentHeader.TotalFragments;

            RequestManager->Fragmentation.ReassembleInUse = TRUE;
        }
        else
        {
            TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][TID=0x%08x] FAILED to acquire reassemble context for out of order fragment, Fragement=%d/%d",
                        MessageFragmentHeader->MessageHeader.MessageTransactionId,
                        MessageFragmentHeader->FragmentHeader.CurrentFragment,
                        MessageFragmentHeader->FragmentHeader.TotalFragments
                        );
            ReassembleContext = NULL;
        }
    }
    //
    // If the context was acquired disable the fragment timer.
    // If the timer cannot be disabled then do not proceed.
    //
    // Else send an error message to the device.
    //
    if( ReassembleContext != NULL )
    {
        ReassembleContext->NextFragment = MessageFragmentHeader->FragmentHeader.CurrentFragment + 1;

        if( MessageFragmentHeader->FragmentHeader.CurrentFragment != 0 )
        {
            if( MbbReqMgrTimerDisarm(
                    RequestManager,
                    MbbTimerTypeFragment,
                    MBB_LOCK_TAKEN
                    ) == FALSE )
            {
                ReassembleContext = NULL;
            }
        }
    }
    else
    {
        MbbUtilSendMbimError(
            MessageFragmentHeader->MessageHeader.MessageTransactionId,
            MBB_ERROR_FRAGMENT_OUT_OF_SEQUENCE,
            RequestManager->AdapterContext,
            ActivityId,
            &Command
            );
    }

    MbbReqMgrUnlockManager( RequestManager );

    return ReassembleContext;
}

_Requires_lock_not_held_( RequestManager->Spinlock )
VOID
MbbReqMgrReleaseSharedReassembleContext(
    __in PMBB_REQUEST_MANAGER   RequestManager
    )
{
    PMBB_REASSEMBLE_CONTEXT ReassembleContext = &RequestManager->Fragmentation.Reassemble;

    MbbReqMgrLockManager( RequestManager );

    if( RequestManager->Fragmentation.ReassembleInUse == TRUE )
    {
        TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][TID=0x%08x] Released reassemble context. Command=%s",
                    ReassembleContext->TransactionId,
                    MbbUtilGetCommandString( &ReassembleContext->Command )
                    );

        ReassembleContext->FragmentLength = 0;
        ReassembleContext->BufferOffset   = 0;
        ReassembleContext->DataLength     = 0;
        ReassembleContext->TransactionId  = 0;
        ReassembleContext->FragmentCount  = 0;
        ReassembleContext->NextFragment   = 0;
        ReassembleContext->NdisStatus     = 0;
        ReassembleContext->MbbStatus      = 0;

        RtlZeroMemory( &ReassembleContext->Command, sizeof(MBB_COMMAND) );

        RequestManager->Fragmentation.ReassembleInUse = FALSE;
    }
    else
    {
        ReassembleContext =  NULL;
    }

    MbbReqMgrUnlockManager( RequestManager );

    if( ReassembleContext == NULL )
    {
        TraceError( WMBCLASS_REQUEST_MANAGER, "[ReqMgr] INVALID release of reassemble context." );
        ASSERT( FALSE );
    }
}

// Timer

NDIS_STATUS
MbbReqMgrTimerInitialize(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType
    )
{
    NDIS_STATUS                 NdisStatus;
    NDIS_TIMER_CHARACTERISTICS  TimerCharacteristics;

    RtlCopyMemory(
        &RequestManager->TimerContexts[TimerType],
        &GlobalTimerContexts[TimerType],
        sizeof(MBB_TIMER_CONTEXT)
        );
    RequestManager->TimerContexts[TimerType].RequestManager = RequestManager;

    TimerCharacteristics.Header.Type        = NDIS_OBJECT_TYPE_TIMER_CHARACTERISTICS;
    TimerCharacteristics.Header.Revision    = NDIS_TIMER_CHARACTERISTICS_REVISION_1;
    TimerCharacteristics.Header.Size        = NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1;
    TimerCharacteristics.AllocationTag      = MbbPoolTagTimer;
    TimerCharacteristics.TimerFunction      = MbbReqMgrTimerCallback;
    TimerCharacteristics.FunctionContext    = &RequestManager->TimerContexts[TimerType];

    NdisStatus = NdisAllocateTimerObject(
                    RequestManager->AdapterContext->MiniportAdapterHandle,
                    &TimerCharacteristics,
                    &RequestManager->TimerContexts[TimerType].TimerHandle
                    );
    return NdisStatus;
}

BOOLEAN
MbbReqMgrTimerCleanup(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType
    )
/*++
    Return Value
        TRUE
            Timer is successfully cancelled.
        FALSE
            Timer is running, caller needs to wait.
--*/
{
    BOOLEAN WaitForTimer = FALSE;

    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] Cleaning %!MbbTimer!", TimerType );

    if( RequestManager->TimerContexts[TimerType].TimerHandle != NULL )
    {
        MbbReqMgrLockManager( RequestManager );
        if( RequestManager->TimerContexts[TimerType].TimerArmTime != 0 )
        {
            if( NdisCancelTimerObject( RequestManager->TimerContexts[TimerType].TimerHandle ) == FALSE )
            {
                WaitForTimer = TRUE;
            }
            RequestManager->TimerContexts[TimerType].TimerArmTime = 0;
        }
        MbbReqMgrUnlockManager( RequestManager );

        if( WaitForTimer )
        {
            TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] Waiting for %!MbbTimer! thread to exit", TimerType );
            KeFlushQueuedDpcs( );
        }
        NdisFreeTimerObject( RequestManager->TimerContexts[TimerType].TimerHandle );
        RequestManager->TimerContexts[TimerType].TimerHandle = NULL;
    }

    return (!WaitForTimer);
}

_When_( Locked==FALSE, _Requires_lock_not_held_( RequestManager->Spinlock ) )
VOID
MbbReqMgrTimerArm(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType,
    __in_opt ULONG              DelayInMS,
    __in_opt ULONG              PeriodInMS,
    __in_opt ULONG              ToleranceInMS,
    __in BOOLEAN                Locked
    )
{
    LARGE_INTEGER   DueTime;

    if( ! Locked )
        MbbReqMgrLockManager( RequestManager );

    if( RequestManager->TimerContexts[TimerType].TimerArmTime == 0 )
    {
        NdisGetSystemUpTimeEx( (PLARGE_INTEGER)(&RequestManager->TimerContexts[TimerType].TimerArmTime) );

        if( DelayInMS == 0 )
            DelayInMS = RequestManager->TimerContexts[TimerType].TimerDelayInMS;

        if( PeriodInMS == 0 )
            PeriodInMS = RequestManager->TimerContexts[TimerType].TimerPeriodInMS;

        if( ToleranceInMS == 0 )
            ToleranceInMS = RequestManager->TimerContexts[TimerType].TimerToleranceInMS;

        DueTime.QuadPart  = DelayInMS;
        DueTime.QuadPart *= 10000 * -1;

        NdisSetCoalescableTimerObject(
            RequestManager->TimerContexts[TimerType].TimerHandle,
            DueTime,
            PeriodInMS,
            NULL,
            ToleranceInMS
            );

        TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] %!MbbTimer! armed at %I64x for %I64x ms",
                    TimerType,
                    RequestManager->TimerContexts[TimerType].TimerArmTime,
                    DelayInMS
                    );
    }
    else
    {
        TraceWarn(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] %!MbbTimer! already armed at %I64x, not re-arming",
                    TimerType,
                    RequestManager->TimerContexts[TimerType].TimerArmTime
                    );
    }

    if( ! Locked )
        MbbReqMgrUnlockManager( RequestManager );
}

_When_( Locked==FALSE, _Requires_lock_not_held_( RequestManager->Spinlock ) )
BOOLEAN
MbbReqMgrTimerDisarm(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType,
    __in BOOLEAN                Locked
    )
/*++
    Return Value
        TRUE
            Timer is successfully cancelled.
        FALSE
            Timer could not be cancelled.
--*/
{
    BOOLEAN IsTimerCancelled = TRUE;

    if( ! Locked )
        MbbReqMgrLockManager( RequestManager );

    if( RequestManager->TimerContexts[TimerType].TimerArmTime != 0 )
    {
        TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] %!MbbTimer! disarmed", TimerType );
        IsTimerCancelled = NdisCancelTimerObject( RequestManager->TimerContexts[TimerType].TimerHandle );
        RequestManager->TimerContexts[TimerType].TimerArmTime = 0;
    }
    else
    {
        TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] %!MbbTimer! not armed, cant disarmed", TimerType );
    }

    if( ! Locked )
        MbbReqMgrUnlockManager( RequestManager );

    return IsTimerCancelled;
}

VOID
MbbReqMgrTimerCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    )
{
    PMBB_TIMER_CONTEXT      TimerContext = (PMBB_TIMER_CONTEXT)FunctionContext;
    PMBB_REQUEST_MANAGER    RequestManager = TimerContext->RequestManager;

    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer] %!MbbTimer! fired", TimerContext->TimerType );

    MbbReqMgrLockManager( RequestManager );
    TimerContext->TimerArmTime = 0;
    MbbReqMgrUnlockManager( RequestManager );

    TimerContext->TimerFunction(
                    SystemSpecific1,
                    RequestManager,
                    SystemSpecific2,
                    SystemSpecific3
                    );
}

// Inter-Fragment timer

VOID
MbbReqMgrTimerFragmentTimeoutCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    )
{
    PMBB_REQUEST_MANAGER    RequestManager = (PMBB_REQUEST_MANAGER)FunctionContext;
    PMBB_REASSEMBLE_CONTEXT ReassembleContext = &RequestManager->Fragmentation.Reassemble;
    PMBB_REQUEST_CONTEXT    Request;
    GUID ActivityId = {0};

    TraceInfo( WMBCLASS_REQUEST_MANAGER, "[ReqMgr][Timer][TID=0x%08x] Fragment timeout", ReassembleContext->TransactionId );

    Request = MbbReqMgrGetRequestByTransactionId( RequestManager, ReassembleContext->TransactionId );

    if(Request)
    {
        ActivityId = Request->ActivityId;
    }

    MbbUtilSendMbimError(
        ReassembleContext->TransactionId,
        MBB_ERROR_TIMEOUT_FRAGMENT,
        RequestManager->AdapterContext,
        ActivityId,
        &ReassembleContext->Command
        );

    MbbReqMgrReleaseSharedReassembleContext( RequestManager );

    if( Request )
    {
        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventCancel,
            (PVOID)(STATUS_TIMEOUT),
            0
            );
        MbbReqMgrDerefRequest( Request );
    }
}

// Request timeout timer

VOID
MbbReqMgrTimerRequestTimeoutCallback(
    __in PVOID  SystemSpecific1,
    __in PVOID  FunctionContext,
    __in PVOID  SystemSpecific2,
    __in PVOID  SystemSpecific3
    )
{
    PMBB_REQUEST_MANAGER    RequestManager = (PMBB_REQUEST_MANAGER)FunctionContext;
    PMBB_REQUEST_CONTEXT    Request;
    PLIST_ENTRY             ListEntry;
    LIST_ENTRY              TempList;
    ULONGLONG               TimeoutDelta;
    ULONGLONG               NextTimeout = ULLONG_MAX;
    ULONGLONG               CurrentTime;

    InitializeListHead( &TempList );

    MbbReqMgrLockManager( RequestManager );
    NdisGetSystemUpTimeEx( (PLARGE_INTEGER)(&CurrentTime) );
    for( ListEntry  =  RequestManager->AllocatedRequestList.Flink;
         ListEntry != &RequestManager->AllocatedRequestList;
         ListEntry  =  ListEntry->Flink )
    {
        Request = CONTAINING_RECORD( ListEntry, MBB_REQUEST_CONTEXT, ManagerLink );

        TimeoutDelta = CurrentTime - Request->DispatchTime;
        if( TimeoutDelta > (MBB_REQUEST_TIMEOUT_MS-MBB_REQUEST_TIMEOUT_TOLERANCE_MS) )
        {
            MbbReqMgrRefRequest( Request );
            InsertTailList( &TempList, &Request->ReqMgrContext.TimeoutLink );
        }
        else
        if( TimeoutDelta < NextTimeout )
        {
            NextTimeout = TimeoutDelta;
        }
    }
    //
    // Re-arm the timer if required.
    // Note that the tolerance is smaller than MBB_REQUEST_TIMEOUT_TOLERANCE_MS.
    // This is because its not known how large NextTimeout is.
    //
    if( NextTimeout != ULLONG_MAX )
    {
        MbbReqMgrTimerArm(
            RequestManager,
            MbbTimerTypeRequest,
            (ULONG)NextTimeout,
            0,
            100,
            MBB_LOCK_TAKEN
            );
    }

    MbbReqMgrUnlockManager( RequestManager );

    while (!IsListEmpty(&TempList))
    {
        // Get the Entry of the list
        ListEntry = RemoveHeadList(&TempList);
        Request = CONTAINING_RECORD(ListEntry, MBB_REQUEST_CONTEXT, ReqMgrContext.TimeoutLink);

        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventCancel,
            (PVOID)(NDIS_STATUS_REQUEST_ABORTED),
            0
            );

        PMBB_SEND_QUEUE SendQueue = &((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request))->SendQueue;
        if (Request->OidHandler != NULL)
        {
            switch (Request->OidHandler->Oid)
            {
            case OID_WWAN_CONNECT:
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ConnectResponseTimeout",
                    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
                //case OID_WWAN_PIN_EX:
                TryQueueStallState(SendQueue);
                break;
            }
        }
        MbbReqMgrDerefRequest(Request);
    }
}

//
// FSM State Handlers
//

VOID
MbbReqFsmReady(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    //
    // No-op
    //
}

VOID
MbbReqFsmDispatching(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    BOOLEAN                     DispatchRequest;
    PMBB_REQUEST_MANAGER        RequestManager = Request->RequestManager;

    MbbReqMgrLockManager( RequestManager );

    if( Request->ReqMgrContext.IsSerialized == TRUE )
    {
        if( RequestManager->CurrentRequest == NULL )
        {
            RequestManager->CurrentRequest = Request;
            DispatchRequest = TRUE;
        }
        else
        {
            TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[ReqMgr][ReqId=0x%04x] Queuing request, QueueStatus[Current=0x%p %s]",
                        Request->RequestId,
                        RequestManager->CurrentRequest,
                        IsListEmpty( &RequestManager->PendingRequestQueue )? "EMPTY": "NON-EMPTY"
                        );
            //
            // Reference request while it is queued
            //
            MbbReqMgrRefRequest( Request );

            InsertTailList(
                &(RequestManager->PendingRequestQueue),
                &(Request->ReqMgrContext.QueueLink)
                );
            DispatchRequest = FALSE;
            Request->ReqMgrContext.IsQueued = TRUE;
        }
    }
    else
    {       
        DispatchRequest = TRUE;
    }
    //
    // Schedule the timeout timer, if not already scheduled.
    //
    MbbReqMgrTimerArm(
        RequestManager,
        MbbTimerTypeRequest,
        0,
        0,
        0,
        MBB_LOCK_TAKEN
        );

    MbbReqMgrUnlockManager( RequestManager );

    if( DispatchRequest )
    {   
        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventStart,
            NULL,
            0
            );
    }
}

VOID
MbbReqFsmSendPending(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    NDIS_STATUS         NdisStatus;
    PMBB_EVENT_ENTRY    NewEventEntry;

    //
    // Reference request while it is pending
    //
    MbbReqMgrRefRequest( Request );

    NdisStatus = Request->ReqMgrContext.DispatchRoutine(
                    MbbReqMgrGetAdapterHandle( Request ),
                    Request
                    );
    if( NdisStatus != NDIS_STATUS_PENDING )
    {
        MbbReqMgrQueueEvent(
            Request->RequestManager,
            Request,
            MbbRequestEventSendComplete,
            (PVOID)NdisStatus,
            0
            );
    }
}

VOID
MbbReqFsmSendComplete(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    BOOLEAN IsSerialized = Request->ReqMgrContext.IsSerialized;

    if ((NDIS_STATUS)(EventEntry->EventData) != STATUS_SUCCESS)
    {
        //
        //  the send failed. copy the command id from the send to the response area since
        //  no response will be received and this can cause some handlers to not complete in indication
        //
        Request->HandlerContext.Response.Command= Request->HandlerContext.Command.Command;
    }

    if( Request->ReqMgrContext.CompletionCallback != NULL )
    {
        Request->ReqMgrContext.CompletionCallback(
            MbbReqMgrGetAdapterHandle( Request ),
            Request,
            (NDIS_STATUS)(EventEntry->EventData)
            );
    }

    if( IsSerialized == TRUE )
    {
        MbbReqMgrIntProcessNextRequest( Request->RequestManager );
    }
    //
    // Request no longer pending
    //
    MbbReqMgrDerefRequest( Request );
}

VOID
MbbReqFsmResponseReceived(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    switch( EventEntry->Event )
    {
        case MbbRequestEventResponseReceived:
        {
            if( Request->ReqMgrContext.ResponseHandler != NULL )
            {
                Request->ReqMgrContext.ResponseHandler(
                    Request,
                    Request->HandlerContext.Response.NdisStatus,
                    Request->HandlerContext.Response.MbbStatus,
                    (PUCHAR) EventEntry->EventData,
                    EventEntry->EventDataLength
                    );
            }
        }
        break;

        case MbbRequestEventResponseReceivedMoreData:
        {
            if( Request->ReqMgrContext.ResponseHandler != NULL )
            {
                Request->ReqMgrContext.ResponseHandler(
                    Request,
                    Request->HandlerContext.Response.NdisStatus,
                    MBB_STATUS_SMS_MORE_DATA,
                    (PUCHAR) EventEntry->EventData,
                    EventEntry->EventDataLength
                    );
            }
        }
        break;

        case MbbRequestEventSendComplete:
        {
            MbbReqFsmSendComplete(
                Request,
                OldState,
                EventEntry
                );
        }
        break;
    }
}

VOID
MbbReqFsmCancelled(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    switch( EventEntry->Event )
    {
        case MbbRequestEventDispatch:
        {
            //
            // Request was cancelled after being created
            // but before being dispatched. The request
            // is not yet queued for dispatching. Fail the
            // request instead of dispatching it.
            //
            ASSERT( Request->ReqMgrContext.CompletionCallback != NULL );

            if( Request->ReqMgrContext.CompletionCallback != NULL )
            {
                Request->ReqMgrContext.CompletionCallback(
                    MbbReqMgrGetAdapterHandle( Request ),
                    Request,
                    (NDIS_STATUS)(NDIS_STATUS_REQUEST_ABORTED)
                    );
            }
        }
        break;

        case MbbRequestEventSendComplete:
        {

            EventEntry->EventData=(PVOID)NDIS_STATUS_REQUEST_ABORTED;

            MbbReqFsmSendComplete(
                Request,
                OldState,
                EventEntry
                );
        }
        break;

        case MbbRequestEventCancel:
        {
            switch( OldState )
            {
                case MbbRequestStateDispatching:
                {
                    //
                    // Request was dispatched and serialized.
                    // Dequeue the request and fail it.
                    //                    
                    MbbReqMgrLockManager( Request->RequestManager );
                    
                    RemoveEntryList( &Request->ReqMgrContext.QueueLink );
                    
                    if(Request->ReqMgrContext.IsQueued)
                    {
                        // this means that the request was actually queued during 
                        // dispatching and not processed directly. When the request
                        // was queued a reference was taken on the request which
                        // needs to be removed now.
                        
                        Request->ReqMgrContext.IsQueued = FALSE;
                        
                        MbbReqMgrUnlockManager( Request->RequestManager );

                        TraceInfo( WMBCLASS_REQUEST_FSM, "[ReqFsm][ReqId=0x%04x] Dequeuing queued request due to cancellation", Request->RequestId );
                        
                        MbbReqMgrDerefRequest( Request );
                    }
                    else
                    {
                        MbbReqMgrUnlockManager( Request->RequestManager );
                    }

                    if( Request->ReqMgrContext.CompletionCallback != NULL )
                    {
                        Request->ReqMgrContext.CompletionCallback(
                            MbbReqMgrGetAdapterHandle( Request ),
                            Request,
                            (NDIS_STATUS)(EventEntry->EventData)
                            );
                    }
                }
                break;

                case MbbRequestStateSendComplete:
                case MbbRequestStateResponseReceivedMoreData:
                {
                    if( Request->ReqMgrContext.ResponseHandler != NULL )
                    {
                        Request->ReqMgrContext.ResponseHandler(
                            Request,
                            (NDIS_STATUS)(EventEntry->EventData),
                            MBB_STATUS_FAILURE,
                            NULL,
                            0
                            );
                    }
                }
                break;

                case MbbRequestStateReady:
                case MbbRequestStateCancelled:
                case MbbRequestStateSendPending:
                case MbbRequestStateResponseReceived:
                default:
                {
                    //
                    // No-op
                    //
                }
                break;
            }
        }
        break;

        case MbbRequestEventStart:
        case MbbRequestEventResponseReceived:
        case MbbRequestEventResponseReceivedMoreData:
        default:
        {
            //
            // No-op
            //
        }
        break;
    }
}

VOID
MbbReqFsmInvalid(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_STATE      OldState,
    __in PMBB_EVENT_ENTRY       EventEntry
    )
{
    TraceInfo( WMBCLASS_REQUEST_FSM, "[ReqFsm][ReqId=0x%04x] INVALID Transition!!", Request->RequestId );
    ASSERT( FALSE );
}
