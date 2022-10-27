#pragma once
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FastIONdisDeviceServiceSessionReceive(
    _In_ PMINIPORT_ADAPTER_CONTEXT          AdapterContext,
    _In_ PNET_BUFFER_LIST                   NetBufferList,
    _In_ ULONG                              SessionId,
    _In_ ULONG                    ReceiveFlags
);

FASTIO_SEND_NET_BUFFER_LISTS_COMPLETE FastIOSendNetBufferListsComplete;
FASTIO_INDICATE_RECEIVE_NET_BUFFER_LISTS FastIOIndicateReceiveNetBufferLists;