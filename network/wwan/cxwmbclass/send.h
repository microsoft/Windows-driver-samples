//
//    Copyright (C) Microsoft.  All rights reserved.
//
NDIS_STATUS
MbbSendQInitialize(
    __in  PMBB_SEND_QUEUE           SendQueue,
    __in  ULONG                     MaxConcurrentSends,
    __in  MBB_DRAIN_COMPLETE        DrainCompleteCallback,
    __in  PVOID                     DrainCompleteContext,
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  MBB_BUS_HANDLE            BusHandle,
    __in  NDIS_HANDLE               MiniportHandle
    );

VOID
MbbSendQCleanup(
    __in  PMBB_SEND_QUEUE       SendQueue
    );

VOID
MbbSendQCancel(
    __in  PMBB_SEND_QUEUE           SendQueue,
    __in  NDIS_STATUS               Status,
    __in  BOOLEAN                   WaitForCompletion
    );


NDIS_STATUS
MbbSendDeviceServiceSessionData(
    __in    PMINIPORT_ADAPTER_CONTEXT           Adapter,
    __in    MBB_REQUEST_HANDLE                  RequestHandle,
    __in    ULONG                               RequestId,
    __in    ULONG                               SessionId,
    __in    ULONG                               DataSize,
    __in    PVOID                               Data
    );

VOID
MbbCleanupDssNbl(
    _In_    PMBB_SEND_QUEUE         SendQueue,
    _In_    PNET_BUFFER_LIST        NetBufferList
);
