//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MBB_PREALLOC_REQUEST_COUNT                  10
#define MBB_PREALLOC_WORKITEM_COUNT                 4
#define MBB_PREALLOC_FRAGMENATION_BUFFER_COUNT      3
#define MBB_FRAGMENATION_BULK_BUFFER_LENGTH         (64*1024)
#define MBB_REASSEMBLE_BUFFER_LENGTH                (64*1024)


////////////////////////////////////////////////////////////////////////////////
//
//  DECLARATIONS
//
////////////////////////////////////////////////////////////////////////////////
FORCEINLINE
BOOLEAN
MbbReqMgrRef(
    __in PMBB_REQUEST_MANAGER RequestManager
    );

FORCEINLINE
VOID
MbbReqMgrDeref(
    __in PMBB_REQUEST_MANAGER RequestManager
    );

NDIS_STATUS
MbbReqMgrInitialize(
    __in PMBB_REQUEST_MANAGER       RequestManager,
    __in PMINIPORT_ADAPTER_CONTEXT  AdapterContext
    );

VOID
MbbReqMgrCleanup(
    __in PMBB_REQUEST_MANAGER RequestManager
    );

VOID
MbbReqMgrCancelRequests(
    __in PMBB_REQUEST_MANAGER    RequestManager,
    __in BOOLEAN                 IsClosed
    );

MBB_PROTOCOL_HANDLE
MbbReqMgrGetAdapterHandle(
    __in    PMBB_REQUEST_CONTEXT    Request
    );

BOOLEAN
MbbReqMgrIsInternalRequest(
    __in    PMBB_REQUEST_CONTEXT    Request
    );

VOID
MbbReqMgrSetUnsolicitedIndication(
    __in    PMBB_REQUEST_CONTEXT    Request
    );

BOOLEAN
MbbReqMgrIsUnsolicitedIndication(
    __in    PMBB_REQUEST_CONTEXT    Request
    );

_Requires_lock_not_held_(&(Request->RequestManager->Spinlock))
VOID
MbbReqMgrGetState(
    __in    PMBB_REQUEST_CONTEXT    Request,
    __inout MBB_REQUEST_STATE      *pCurrentState,
    __inout MBB_REQUEST_STATE      *pLastState
    );

BOOLEAN
MbbReqMgrIsSetOid(
    __in PMBB_REQUEST_CONTEXT   Request
    );

MBB_NDIS_OID_STATE
MbbReqMgrGetSetOidState(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_NDIS_OID_STATE     OidState
    );

__checkReturn
__drv_aliasesMem
__drv_allocatesMem(Mem)
PMBB_REQUEST_CONTEXT
MbbReqMgrCreateRequest(
    __in     PMBB_REQUEST_MANAGER    RequestManager,
    __in_opt PNDIS_OID_REQUEST       OidRequest,
    __in     ULONG                   ResponseBufferLength,
    __out    PNDIS_STATUS            NdisStatus
    );

VOID
MbbReqMgrDestroyRequest(
    __in PMBB_REQUEST_MANAGER RequestManager,
    __in PMBB_REQUEST_CONTEXT Request
    );

NDIS_STATUS
MbbReqMgrDispatchRequest(
    __in        PMBB_REQUEST_CONTEXT            Request,
    __in        BOOLEAN                         IsSerialized,
    __in        MBB_REQUEST_DISPATCH_ROUTINE    DispatchRoutine,
    __in        MBB_REQUEST_COMPLETION_CALLBACK CompletionCallback,
    __in_opt    MBB_REQUEST_RESPONSE_HANDLER    ResponseHandler
    );

_Acquires_lock_( RequestManager->Spinlock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, RequestManager )
VOID
MbbReqMgrLockManager(
    __in PMBB_REQUEST_MANAGER RequestManager
    );

_Releases_lock_( RequestManager->Spinlock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, RequestManager )
VOID
MbbReqMgrUnlockManager(
    __in PMBB_REQUEST_MANAGER RequestManager
    );

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByOidRequestId(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    PNDIS_OID_REQUEST       RequestId
    );

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestById(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    ULONG                   RequestId
    );

PMBB_REQUEST_CONTEXT
MbbReqMgrGetRequestByTransactionId(
    __in    PMBB_REQUEST_MANAGER    RequestManager,
    __in    ULONG                   TransactionId
    );

_Requires_lock_held_( RequestManager->Spinlock )
VOID
MbbReqMgrQueueEventLocked(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_EVENT      Event,
    __in_opt PVOID              EventData,
    __in ULONG                  EventDataLength,
    __in BOOLEAN                SignalThread
    );

_Requires_lock_not_held_( RequestManager->Spinlock )
VOID
MbbReqMgrQueueEvent(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in PMBB_REQUEST_CONTEXT   Request,
    __in MBB_REQUEST_EVENT      Event,
    __in_opt PVOID              EventData,
    __in ULONG                  EventDataLength
    );

VOID
MbbReqMgrRefRequest(
    __in PMBB_REQUEST_CONTEXT   Request
    );

VOID
MbbReqMgrDerefRequest(
    __in PMBB_REQUEST_CONTEXT   Request
    );

// Reassemble context

_Requires_lock_not_held_( RequestManager->Spinlock )
PMBB_REASSEMBLE_CONTEXT
MbbReqMgrAcquireSharedReassembleContext(
    __in PMBB_REQUEST_MANAGER                               RequestManager,
    __in_bcount(MessageLength) PMBB_COMMAND_FRAGMENT_HEADER MessageFragmentHeader,
    __in ULONG                                              MessageLength,
    __in GUID                                               ActivityId
    );

_Requires_lock_not_held_( RequestManager->Spinlock )
VOID
MbbReqMgrReleaseSharedReassembleContext(
    __in PMBB_REQUEST_MANAGER   RequestManager
    );

// Timer

NDIS_STATUS
MbbReqMgrTimerInitialize(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType
    );

BOOLEAN
MbbReqMgrTimerCleanup(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType
    );

_When_( Locked==FALSE, _Requires_lock_not_held_( RequestManager->Spinlock ) )
VOID
MbbReqMgrTimerArm(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType,
    __in_opt ULONG              DelayInMS,
    __in_opt ULONG              PeriodInMS,
    __in_opt ULONG              ToleranceInMS,
    __in BOOLEAN                Locked
    );

_When_( Locked==FALSE, _Requires_lock_not_held_( RequestManager->Spinlock ) )
BOOLEAN
MbbReqMgrTimerDisarm(
    __in PMBB_REQUEST_MANAGER   RequestManager,
    __in MBB_TIMER_TYPE         TimerType,
    __in BOOLEAN                Locked
    );
