//
//    Copyright (C) Microsoft.  All rights reserved.
//
NDIS_STATUS
MbbRecvQInitialize(
    __in    PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    MBB_DRAIN_COMPLETE  DrainCompleteCallback,
    __in    PVOID               DrainCompleteContext,
    __in    NDIS_HANDLE         MiniportHandle
    );

VOID
MbbRecvQCleanup(
    __in    PMBB_RECEIVE_QUEUE  RecvQueue
    );

VOID
MbbRecvQCancel(
    __in  PMBB_RECEIVE_QUEUE        RecvQueue,
    __in  NDIS_STATUS               Status,
    __in  BOOLEAN                   WaitForCompletion
    );
