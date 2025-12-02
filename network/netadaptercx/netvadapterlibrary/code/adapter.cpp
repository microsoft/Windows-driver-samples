// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"

#include <new.h>
#ifdef _KERNEL_MODE
#include <xfilter.h>
#else
#include "net/umxfilter.h" // Copied from Km XFilter.h, NETCX please move this xfilter.h into shared location
#endif
#include "adapter.h"
#include "rxqueue.h"
#include "txqueue.h"
#include "configuration.h"
#include "trace.h"
#include "memory.h"

#include "adapter.tmh"

UCHAR NetvMacAddressBase[MAC_ADDR_LEN] = { 0x22, 0x22, 0x22, 0x22, 0x00, 0x00 };
const ULONG GSO_MAX_OFFLOAD_SIZE = 0xffff;
const ULONG GSO_MIN_SEGMENT_COUNT = 2;

/*
 * increasing beyond 1Gbps results in intermittent failure of
 * netvadapter start due to buffer allocation failures in
 * netadaptercx when running in nebula.
 * tracked as bug 50671552 (if it doesn't get archived)
 */
static auto constexpr MAX_LINK_SPEED{1'000'000'000ull};

void
NetvEnlInterruptRoutine(
    _Inout_ PVOID PortContext,
    _In_ bool Tx
)
{
    NETPACKETQUEUE queue = (NETPACKETQUEUE)PortContext;

    if (Tx) // Tx
    {
        NetTxQueueNotifyMoreCompletedPacketsAvailable(queue);
    }
    else // Rx
    {
        NetRxQueueNotifyMoreReceivedPacketsAvailable(queue);
    }
}

static
EVT_PACKET_QUEUE_START
    EvtTxQueueStart;

static
EVT_PACKET_QUEUE_STOP
    EvtTxQueueStop;

static
EVT_PACKET_QUEUE_ADVANCE
    EvtTxQueueAdvance;

static
EVT_PACKET_QUEUE_CANCEL
    EvtTxQueueCancel;

static
EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED
    EvtTxQueueSetNotify;

static
EVT_PACKET_QUEUE_START
    EvtRxQueueStart;

static
EVT_PACKET_QUEUE_STOP
    EvtRxQueueStop;

static
EVT_PACKET_QUEUE_ADVANCE
    EvtRxQueueAdvance;

static
EVT_PACKET_QUEUE_CANCEL
    EvtRxQueueCancel;

static
EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED
    EvtRxQueueSetNotify;

NetvAdapter::NetvAdapter(
    NETADAPTER Handle,
    WDFDEVICE Device
) noexcept
    : m_handle(Handle)
    , m_device(Device)
{
}

NTSTATUS
NetvAdapter::Initialize(
    void
)
{
    RETURN_IF_NOT_STATUS_SUCCESS(
        NetvAdapterReadConfiguration(this, m_device));

    RETURN_IF_NOT_STATUS_SUCCESS(NetvAdapterReadAddress());

    SetLinkState();

    NTSTATUS status = STATUS_SUCCESS;

    // Create ENL
    EnlPortCreated = FALSE;

    if (NetvEnlMLink[EnlIndex].LinkCount == 0)
    {
        RETURN_IF_NOT_STATUS_SUCCESS(
            EnlMCreateLink(
                LinkCount,
                LinkProcIndex,
                LinkPoll,
                &NetvEnlMLink[EnlIndex]));
    }

    RETURN_NTSTATUS_IF(STATUS_INVALID_ADDRESS,
        EnlMIsPortActive(&NetvEnlMLink[EnlIndex], EnlPortIndex));

    status = EnlMActivateLinkPort(
        &NetvEnlMLink[EnlIndex],
        EnlPortIndex,
        NetvEnlInterruptRoutine,
        this);

    // what is the virtue of doing this?
    if (NT_SUCCESS(status))
    {
        EnlPortCreated = TRUE;
    }

    RETURN_STATUS_SUCCESS();
}

void
NetvAdapter::Destroy(
    void
)
{
    if (EnlPortCreated)
    {
        EnlMDeactivateLinkPort(&NetvEnlMLink[EnlIndex], EnlPortIndex);
        EnlPortCreated = FALSE;
    }

    if (EnlIndexValid && !EnlIsLinkActive(NetvEnlMLink[EnlIndex].LinkHandle[0]))
    {
        EnlMCloseLink(&NetvEnlMLink[EnlIndex]);
    }
}

_Use_decl_annotations_
NTSTATUS
NetvAdapter::CreateRxQueue(
    NETRXQUEUE_INIT * NetRxQueueInit
    )
{
    WDF_OBJECT_ATTRIBUTES rxAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&rxAttributes, NetvRxQueue);
    rxAttributes.EvtDestroyCallback = [](WDFOBJECT Handle) {
        NetvRxQueueGetContext(static_cast<NETPACKETQUEUE>(Handle))->Destroy();
    };

    NET_PACKET_QUEUE_CONFIG rxConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(
        &rxConfig,
        EvtRxQueueAdvance,
        EvtRxQueueSetNotify,
        EvtRxQueueCancel);
    rxConfig.EvtStart = EvtRxQueueStart;
    rxConfig.EvtStop = EvtRxQueueStop;

    NETPACKETQUEUE rxQueue;
    RETURN_IF_NOT_STATUS_SUCCESS(NetRxQueueCreate(
        NetRxQueueInit,
        &rxAttributes,
        &rxConfig,
        &rxQueue));

    new (NetvRxQueueGetContext(rxQueue)) NetvRxQueue(rxQueue, *this);

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
NTSTATUS
NetvAdapter::CreateTxQueue(
    NETTXQUEUE_INIT * NetTxQueueInit
    )
{
    WDF_OBJECT_ATTRIBUTES txAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&txAttributes, NetvTxQueue);
    txAttributes.EvtDestroyCallback = [](WDFOBJECT Handle) {
        NetvTxQueueGetContext(static_cast<NETPACKETQUEUE>(Handle))->Destroy();
    };

    NET_PACKET_QUEUE_CONFIG txConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(
        &txConfig,
        EvtTxQueueAdvance,
        EvtTxQueueSetNotify,
        EvtTxQueueCancel);
    txConfig.EvtStart = EvtTxQueueStart;
    txConfig.EvtStop = EvtTxQueueStop;

    NETPACKETQUEUE txQueue;
    RETURN_IF_NOT_STATUS_SUCCESS(NetTxQueueCreate(
        NetTxQueueInit,
        &txAttributes,
        &txConfig,
        &txQueue));

    new (NetvTxQueueGetContext(txQueue)) NetvTxQueue(txQueue, *this);

    RETURN_STATUS_SUCCESS();
}

void NetvAdapter::SetPdoWakeSignalCallback(_In_ EVT_PDO_WAKE_SIGNAL* evtPdoWakeSignal, _In_ void* context)
{
    EnlSetPdoWakeSignalCallback(NetvEnlMLink[EnlIndex].LinkHandle[0], evtPdoWakeSignal, context);
}

_Use_decl_annotations_
_IRQL_requires_max_(PASSIVE_LEVEL)
void NetvAdapter::ArmWakeFromS0(void)
{
    if (EnlPortCreated)
    {
        ENLP_LINK* enLinkHandle = NetvEnlMLink[EnlIndex].LinkHandle[0];
        EnlArmWake(enLinkHandle);
    }
}

_Use_decl_annotations_
_IRQL_requires_max_(PASSIVE_LEVEL)
void NetvAdapter::DisarmWakeFromS0(void)
{
    if (EnlPortCreated)
    {
        ENLP_LINK* enLinkHandle = NetvEnlMLink[EnlIndex].LinkHandle[0];
        EnlDisarmWake(enLinkHandle);
    }
}

_Use_decl_annotations_
void
NetvAdapter::SetLinkState(
    void
) const
{
    NET_ADAPTER_AUTO_NEGOTIATION_FLAGS autoNegotiationFlags{NetAdapterAutoNegotiationFlagNone};
    if (LinkAutoNeg)
    {
        autoNegotiationFlags |=
            NetAdapterAutoNegotiationFlagXmitLinkSpeedAutoNegotiated |
            NetAdapterAutoNegotiationFlagRcvLinkSpeedautoNegotiated |
            NetAdapterAutoNegotiationFlagDuplexAutoNegotiated;
    }
    if (FlowControl != NetvFlowControlDisabled)
    {
        autoNegotiationFlags |=
            NetAdapterAutoNegotiationFlagPauseFunctionsAutoNegotiated;
    }

    NET_ADAPTER_PAUSE_FUNCTION_TYPE pauseFunctions{NetAdapterPauseFunctionTypeUnknown};
    switch (FlowControl)
    {
    case NetvFlowControlDisabled:
        pauseFunctions = NetAdapterPauseFunctionTypeUnsupported;
        break;
    case NetvFlowControlRxEnabled:
        pauseFunctions = NetAdapterPauseFunctionTypeReceiveOnly;
        break;
    case NetvFlowControlTxEnabled:
        pauseFunctions = NetAdapterPauseFunctionTypeSendOnly;
        break;
    case NetvFlowControlTxRxEnabled:
        pauseFunctions = NetAdapterPauseFunctionTypeSendAndReceive;
        break;
    }

    NET_ADAPTER_LINK_STATE linkState;
    NET_ADAPTER_LINK_STATE_INIT(
        &linkState,
        MAX_LINK_SPEED,
        MediaConnectStateConnected,
        MediaDuplexStateFull,
        pauseFunctions,
        autoNegotiationFlags);
    NetAdapterSetLinkState(m_handle, &linkState);
}

static
void
EvtSetReceiveFilter(
    _In_ NETADAPTER NetAdapter,
    _In_ NETRECEIVEFILTER Handle
    )
{
    NetvAdapter* adapter = NetvAdapterGetContextFromWDFObject(NetAdapter);

    adapter->PacketFilter = NetReceiveFilterGetPacketFilter(Handle);

    adapter->NumMulticastAddresses = (ULONG)NetReceiveFilterGetMulticastAddressCount(Handle);

    RtlZeroMemory(adapter->MulticastAddressList,
        sizeof(NET_ADAPTER_LINK_LAYER_ADDRESS) * MAX_MULTICAST_LIST_SIZE);

    if (adapter->NumMulticastAddresses != 0U)
    {
        NET_ADAPTER_LINK_LAYER_ADDRESS const * MulticastAddressList = NetReceiveFilterGetMulticastAddressList(Handle);
        RtlCopyMemory(adapter->MulticastAddressList,
            MulticastAddressList,
            sizeof(NET_ADAPTER_LINK_LAYER_ADDRESS) * adapter->NumMulticastAddresses);
    }
}

static
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
NTAPI
EvtNetAdapterOffloadSetRxXSum(
    _In_ NETADAPTER Adapter,
    _In_ NETOFFLOAD Offload
    )
{
    UNREFERENCED_PARAMETER((Adapter, Offload));
    ASSERT(NetOffloadIsRxChecksumIPv4Enabled(Offload));
    ASSERT(NetOffloadIsRxChecksumUdpEnabled(Offload));
}

static
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
NTAPI
EvtNetAdapterOffloadSetGso(
    _In_ NETADAPTER Adapter,
    _In_ NETOFFLOAD Offload
    )
{
    auto adapter = NetvAdapterGetContextFromWDFObject(Adapter);
    if (adapter->UsoEnabled)
    {
        //
        // Since netvadapter only converts USO sends into URO receives,
        // both must be enabled together, or disabled together.
        // The order of callbacks is nondeterministic, so this can't assert
        // that URO is enabled, since it might not have been enabled yet.
        // This assert is so anyone using netvadapter knows that USO has
        // been disabled after being enabled.
        //
        NT_FRE_ASSERTMSG("USO can't be disabled after being enabled", NetOffloadIsUsoIPv4Enabled(Offload));
        NT_FRE_ASSERTMSG("USO can't be disabled after being enabled", NetOffloadIsUsoIPv6Enabled(Offload));
    }
    else
    {
        adapter->UsoEnabled = NetOffloadIsUsoIPv4Enabled(Offload) && NetOffloadIsUsoIPv6Enabled(Offload);
    }
}

static
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
NTAPI
EvtNetAdapterOffloadSetRsc(
    _In_ NETADAPTER Adapter,
    _In_ NETOFFLOAD Offload
    )
{
    auto adapter = NetvAdapterGetContextFromWDFObject(Adapter);
    if (adapter->UroEnabled)
    {
        //
        // Since netvadapter only converts USO sends into URO receives,
        // both must be enabled together, or disabled together.
        // The order of callbacks is nondeterministic, so this can't assert
        // that USO is enabled, since it might not have been enabled yet.
        // This assert is so anyone using netvadapter knows that URO has
        // been disabled after being enabled.
        // This assert is important because if URO is disabled, netvadapter
        // will forward the USO send without fixing it up or segmenting it,
        // and the stack may behave badly.
        //
        NT_FRE_ASSERTMSG("URO can't be disabled after being enabled", NetOffloadIsUdpRscEnabled(Offload));
    }
    else
    {
        adapter->UroEnabled = NetOffloadIsUdpRscEnabled(Offload);
    }
}

static
void
NetvAdapterSetUsoUroOffloadCapabilities(
    _In_ NETADAPTER Adapter
    )
{
    NET_ADAPTER_OFFLOAD_GSO_CAPABILITIES gsoCapabilities;
    NET_ADAPTER_OFFLOAD_GSO_CAPABILITIES_INIT(
        &gsoCapabilities,
        NetAdapterOffloadLayer3FlagIPv4NoOptions | NetAdapterOffloadLayer3FlagIPv6NoExtensions,
        NetAdapterOffloadLayer4FlagUdp,
        GSO_MAX_OFFLOAD_SIZE,
        GSO_MIN_SEGMENT_COUNT,
        EvtNetAdapterOffloadSetGso);

    NET_ADAPTER_OFFLOAD_RSC_CAPABILITIES rscCapabilities;
    NET_ADAPTER_OFFLOAD_RSC_CAPABILITIES_INIT(
        &rscCapabilities,
        NetAdapterOffloadLayer3FlagIPv4NoOptions | NetAdapterOffloadLayer3FlagIPv6NoExtensions,
        NetAdapterOffloadLayer4FlagUdp,
        EvtNetAdapterOffloadSetRsc);
    rscCapabilities.TcpTimestampOption = FALSE;

    NetAdapterOffloadSetGsoCapabilities(Adapter, &gsoCapabilities);
    NetAdapterOffloadSetRscCapabilities(Adapter, &rscCapabilities);
}

_Use_decl_annotations_
NTSTATUS NetvAdapter::ConfigureDataCapabilities()
{
    if (PreallocatedRxBuffers)
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, Memory);
        attributes.EvtDestroyCallback = [](WDFOBJECT Object)
        {
            Memory* memory = GetMemoryFromHandle(Object);
            memory->~Memory();
        };

        attributes.EvtCleanupCallback = [](WDFOBJECT Object)
        {
            Memory* memory = GetMemoryFromHandle(Object);
            memory->ReleaseAllMappings();
        };

        NET_MEMORY_COLLECTION_CONFIG collectionConfig;
        NET_MEMORY_COLLECTION_CONFIG_INIT(
            &collectionConfig,
            PREALLOCATED_BUFFERS_COUNT);

        RETURN_IF_NOT_STATUS_SUCCESS(
            NetMemoryCollectionCreate(
                m_device,
                &attributes,
                &collectionConfig,
                &m_preallocatedRxBuffers));

        Memory* memory = new (GetMemoryFromHandle(m_preallocatedRxBuffers)) Memory();
        RETURN_IF_NOT_STATUS_SUCCESS(
            memory->Initialize(
                m_preallocatedRxBuffers,
                MAX_RX_BUFFER_SIZE
            ));
    }

    NET_ADAPTER_TX_CAPABILITIES txCapabilities;
    NET_ADAPTER_TX_CAPABILITIES_INIT(&txCapabilities, MAX_TX_QUEUES);

    NET_ADAPTER_RX_CAPABILITIES rxCapabilities;
    NET_ADAPTER_RX_CAPABILITIES_INIT_SYSTEM_MANAGED(&rxCapabilities, MAX_RX_BUFFER_SIZE, MAX_RX_QUEUES);

    if (PreallocatedRxBuffers)
    {
        rxCapabilities.AllocationMode = NetRxFragmentBufferAllocationModeDriverV2;
        rxCapabilities.AttachmentMode = NetRxFragmentBufferAttachmentModeDriver;
        rxCapabilities.MemoryCollection = m_preallocatedRxBuffers;
    }

    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(&linkLayerCapabilities, MAX_LINK_SPEED, MAX_LINK_SPEED);

    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES receiveFilterCapabilities;
    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES_INIT(&receiveFilterCapabilities, EvtSetReceiveFilter);
    receiveFilterCapabilities.SupportedPacketFilters = NETV_SUPPORTED_FILTERS;
    receiveFilterCapabilities.MaximumMulticastAddresses = MAX_MULTICAST_LIST_SIZE;

    NET_ADAPTER_OFFLOAD_RX_CHECKSUM_CAPABILITIES xsumCapabilities;
    NET_ADAPTER_OFFLOAD_RX_CHECKSUM_CAPABILITIES_INIT(&xsumCapabilities, EvtNetAdapterOffloadSetRxXSum);

    NetAdapterSetLinkLayerCapabilities(m_handle, &linkLayerCapabilities);
    NetAdapterSetLinkLayerMtuSize(m_handle, MTU_SIZE);
    NetAdapterSetDataPathCapabilities(m_handle, &txCapabilities, &rxCapabilities);
    NetAdapterSetReceiveFilterCapabilities(m_handle, &receiveFilterCapabilities);
    NetAdapterOffloadSetRxChecksumCapabilities(m_handle, &xsumCapabilities);

    NET_ADAPTER_LINK_LAYER_ADDRESS netvLinkAddress;
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(&netvLinkAddress, MAC_ADDR_LEN, (CONST UCHAR*)&(PermanentAddress.Address));

    NetAdapterSetPermanentLinkLayerAddress(m_handle, &netvLinkAddress);
    NetAdapterSetCurrentLinkLayerAddress(m_handle, &netvLinkAddress);

    if (EnableUsoUro)
    {
        NetvAdapterSetUsoUroOffloadCapabilities(m_handle);
    }

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
NTSTATUS
ConfigureAndStartAdapter(
    NETADAPTER Adapter
    )
{
    auto adapter = NetvAdapterGetContextFromWDFObject(Adapter);
    RETURN_IF_NOT_STATUS_SUCCESS(
        adapter->ConfigureDataCapabilities());

    RETURN_IF_NOT_STATUS_SUCCESS(
        NetAdapterStart(Adapter));

    RETURN_STATUS_SUCCESS();
}

NTSTATUS
NetvAdapter::NetvAdapterReadAddress()
{
    PermanentAddress.Length = MAC_ADDR_LEN;

    RETURN_NTSTATUS_IF(STATUS_INVALID_ADDRESS,
        MACLastByte < 1 ||
        MACLastByte > MAX_ADAPTER_COUNT);

    ETH_COPY_NETWORK_ADDRESS(PermanentAddress.Address, NetvMacAddressBase);
    PermanentAddress.Address[MAC_ADDR_LEN - 1] = (UCHAR) MACLastByte;

    if (ETH_IS_MULTICAST(PermanentAddress.Address) ||
        ETH_IS_BROADCAST(PermanentAddress.Address))
    {
        RETURN_IF_NOT_STATUS_SUCCESS(STATUS_INVALID_ADDRESS);
    }

    RtlCopyMemory(
        &CurrentAddress,
        &PermanentAddress,
        sizeof(PermanentAddress)
        );

    EnlIndex = (MACLastByte - 1) >> 1;
    EnlPortIndex = (MACLastByte - 1) & 1;
    EnlIndexValid = TRUE;

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
void
EvtTxQueueStart(
    NETPACKETQUEUE Queue
)
{
    NetvTxQueueGetContext(Queue)->Start();
}

_Use_decl_annotations_
void
EvtTxQueueStop(
    NETPACKETQUEUE Queue
)
{
    NetvTxQueueGetContext(Queue)->Stop();
}

_Use_decl_annotations_
void
EvtTxQueueAdvance(
    NETPACKETQUEUE Queue
)
{
    NetvTxQueueGetContext(Queue)->Advance();
}

_Use_decl_annotations_
void
EvtTxQueueCancel(
    NETPACKETQUEUE Queue
)
{
    NetvTxQueueGetContext(Queue)->Cancel();
}

_Use_decl_annotations_
void
EvtTxQueueSetNotify(
    NETPACKETQUEUE Queue,
    BOOLEAN Enable
)
{
    NetvTxQueueGetContext(Queue)->SetNotify(Enable);
}

_Use_decl_annotations_
void
EvtRxQueueStart(
    NETPACKETQUEUE Queue
)
{
    NetvRxQueueGetContext(Queue)->Start();
}

_Use_decl_annotations_
void
EvtRxQueueStop(
    NETPACKETQUEUE Queue
)
{
    NetvRxQueueGetContext(Queue)->Stop();
}


_Use_decl_annotations_
void
EvtRxQueueAdvance(
    NETPACKETQUEUE Queue
)
{
    NetvRxQueueGetContext(Queue)->Advance();
}

_Use_decl_annotations_
void
EvtRxQueueCancel(
    NETPACKETQUEUE Queue
)
{
    NetvRxQueueGetContext(Queue)->Cancel();
}

_Use_decl_annotations_
void
EvtRxQueueSetNotify(
    NETPACKETQUEUE Queue,
    BOOLEAN Enable
)
{
    NetvRxQueueGetContext(Queue)->SetNotify(Enable);
}
