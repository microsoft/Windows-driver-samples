#pragma once

// FASTIO_SEND_NET_BUFFER_LISTS_COMPLETE FastIOSendNetBufferListsComplete;
// FASTIO_INDICATE_RECEIVE_NET_BUFFER_LISTS FastIOIndicateReceiveNetBufferLists;

_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C VOID MbbBusSendNetBufferLists(_In_ MBB_BUS_HANDLE BusHandle, _In_ PNET_BUFFER_LIST NetBufferList, _In_ ULONG SessionId, _In_ ULONG SendFlags);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C VOID MbbBusReturnNetBufferLists(_In_ MBB_BUS_HANDLE BusHandle, _In_ PNET_BUFFER_LIST NetBufferList, _In_ ULONG ReturnFlags);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXTERN_C VOID MbbBusCancelSendHandler(_In_ MBB_BUS_HANDLE BusHandle, _In_ PVOID CancelId);

_IRQL_requires_(PASSIVE_LEVEL)
EXTERN_C
VOID MbbBusHalt(_In_ MBB_BUS_HANDLE BusHandle, _In_ NDIS_HALT_ACTION HaltAction);

_IRQL_requires_(PASSIVE_LEVEL)
EXTERN_C
NDIS_STATUS
MbbBusPause(_In_ MBB_BUS_HANDLE BusHandle);

_When_(ShutdownAction == NdisShutdownPowerOff, _IRQL_requires_(PASSIVE_LEVEL))
    _When_(ShutdownAction == NdisShutdownBugCheck, _IRQL_requires_(HIGH_LEVEL))
    EXTERN_C VOID MbbBusShutdown(_In_ MBB_BUS_HANDLE BusHandle, _In_ NDIS_SHUTDOWN_ACTION ShutdownAction);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    EXTERN_C NDIS_STATUS MbbBusReset(_In_ MBB_BUS_HANDLE BusHandle);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    EXTERN_C NDIS_STATUS MbbBusRestart(_In_ MBB_BUS_HANDLE BusHandle);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    EXTERN_C BOOLEAN MbbBusIsFastIO(_In_ MBB_BUS_HANDLE BusHandle);