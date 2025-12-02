//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

// packet and header sizes
#define MBB_MAX_PACKET_SIZE (1514)

// maximum link speed for send and recv in bps
#define MBB_MEDIA_MAX_SPEED 1'000'000'000

// supported filters
#define WMBCLASS_MAX_WOL_PATTERN (256)
#define WMBCLASS_MAX_MBIM_WOL_PATTERN (192)

//
// Context for each NetAdapter instance.
// Each NetAdapter instance corresponds to an IP interface
//
typedef struct _WMBCLASS_NETADAPTER_CONTEXT
{
    PWMBCLASS_DEVICE_CONTEXT WmbDeviceContext; // Wdf device context
    NETADAPTER NetAdapter;                     // NetAdapter object

    ULONG ConnectionId;
    ULONG SessionId;
    MBB_CONNECTION_STATE ConnectionState;

    // RxQuere-related objects

    WDFLOOKASIDE NtbLookasideList;
    WDFLOOKASIDE ReceiveNdpLookasideList;

    LIST_ENTRY ReceiveNdpList;
    NETPACKETQUEUE TxQueue; // Tx Queue object
    NETPACKETQUEUE RxQueue; // Rx Queue object
    BOOLEAN AllowRxTraffic;
} WMBCLASS_NETADAPTER_CONTEXT, *PWMBCLASS_NETADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WMBCLASS_NETADAPTER_CONTEXT, WmbClassGetNetAdapterContext);

//
// NetAdapter functions
//

EVT_NET_ADAPTER_CREATE_TXQUEUE EvtAdapterCreateTxQueue;
EVT_NET_ADAPTER_CREATE_RXQUEUE EvtAdapterCreateRxQueue;

EVT_WDF_DEVICE_CONTEXT_DESTROY MbbDestroyAdapterContext;

NTSTATUS
WmbClassAdapterStart(_In_ NETADAPTER netAdapter);
