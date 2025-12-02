// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <netadaptercx.h>

#define MAX_RX_BUFFER_SIZE 65535
#define MAX_RX_QUEUES 1
#define MAX_TX_QUEUES 1
#define MTU_SIZE 1500
#define MAX_MULTICAST_LIST_SIZE 32
#define MAC_ADDR_LEN 6
#define NETV_NUMBER_OF_QUEUES 1

// supported filters
#define NETV_SUPPORTED_FILTERS ( \
    NetPacketFilterFlagDirected | \
    NetPacketFilterFlagMulticast | \
    NetPacketFilterFlagBroadcast | \
    NetPacketFilterFlagPromiscuous | \
    NetPacketFilterFlagAllMulticast)

NTSTATUS
ConfigureAndStartAdapter(
    _In_ NETADAPTER Adapter
    );

EVT_NET_ADAPTER_CREATE_TXQUEUE
    CreateTxQueue;
EVT_NET_ADAPTER_CREATE_RXQUEUE
    CreateRxQueue;

typedef enum _NETV_FLOW_CONTROL
{
    NetvFlowControlDisabled = 0,
    NetvFlowControlTxEnabled = 1,
    NetvFlowControlRxEnabled = 2,
    NetvFlowControlTxRxEnabled = 3,
} NETV_FLOW_CONTROL;

typedef NTSTATUS(EVT_PDO_WAKE_SIGNAL)(_In_ void* Context);

class NetvAdapter
{

public:

    NetvAdapter(
        NETADAPTER Handle,
        WDFDEVICE Device
    ) noexcept;

    void
    Destroy(
        void
    );

    NTSTATUS
    Initialize(
        void
    );

    NTSTATUS
    CreateRxQueue(
        NETRXQUEUE_INIT * NetRxQueueInit
    );

    NTSTATUS
    CreateTxQueue(
        NETTXQUEUE_INIT * NetTxQueueInit
    );

    NTSTATUS ConfigureDataCapabilities();

    void SetPdoWakeSignalCallback(_In_ EVT_PDO_WAKE_SIGNAL* evtPdoWakeSignal, _In_ void* context);

    void ArmWakeFromS0(void);

    void DisarmWakeFromS0(void);

    NETADAPTER m_handle = WDF_NO_HANDLE;

    WDFDEVICE m_device = WDF_NO_HANDLE;

    NETMEMORYCOLLECTION
        m_preallocatedRxBuffers = WDF_NO_HANDLE;

    // configuration
    NET_ADAPTER_LINK_LAYER_ADDRESS PermanentAddress;
    NET_ADAPTER_LINK_LAYER_ADDRESS CurrentAddress;
    ULONG MACLastByte;
    BOOLEAN S0Idle;
    BOOLEAN EnableUsoUro;
    BOOLEAN PreallocatedRxBuffers;

    // Packet Filter and look ahead size.
    NET_PACKET_FILTER_FLAGS PacketFilter;

    bool LinkAutoNeg{false};
    NETV_FLOW_CONTROL FlowControl;

    ULONG MtuSize;
    ULONG CurrentPacketFilter;
    ULONG NumMulticastAddresses;
    NET_ADAPTER_LINK_LAYER_ADDRESS MulticastAddressList[MAX_MULTICAST_LIST_SIZE];

    //ENL
    LIST_ENTRY AdapterListLink;
    ULONG LinkCount{1};
    ULONG LinkProcIndex;
    ULONG EnlIndex;
    ULONG EnlPortIndex;
    BOOLEAN EnlIndexValid;
    BOOLEAN EnlPortCreated;
    BOOLEAN LinkPoll;
    ULONG64 EnlTxDrops;

    // Offloads
    bool UsoEnabled;
    bool UroEnabled;

private:

    _IRQL_requires_(PASSIVE_LEVEL)
    void
    SetLinkState(
        void
    ) const;

    virtual NTSTATUS NetvAdapterReadAddress();
};

extern NetvAdapter* NetvAdapterGetContextFromWDFObject(NETADAPTER netAdapter);

typedef struct _GLOBAL_CONTEXT
{
} GLOBAL_CONTEXT;

extern GLOBAL_CONTEXT NetvGlobalContext;


