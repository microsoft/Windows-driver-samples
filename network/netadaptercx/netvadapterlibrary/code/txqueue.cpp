// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"
#include "netvadapter.h"
#include "txqueue.h"

NetvTxQueue::NetvTxQueue(
    NETPACKETQUEUE Handle,
    NetvAdapter & Adapter
) noexcept
    : NetvQueue{Handle, Adapter, NetTxQueueGetRingCollection(Handle)}
{
    NET_EXTENSION_QUERY extension;
    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_NAME,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_VERSION_1,
        NetExtensionTypeFragment);

    NetTxQueueGetExtension(m_handle, &extension, &VirtualAddressExtension);

    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_PACKET_EXTENSION_GSO_NAME,
        NET_PACKET_EXTENSION_GSO_VERSION_1,
        NetExtensionTypePacket);

    NetTxQueueGetExtension(m_handle, &extension, &UsoExtension);

    EnlQueueHandle = EnlCreateQueue(Handle, TX);
}

_Use_decl_annotations_
void
NetvTxQueue::Destroy(
    void
)
{
    EnlDestroyQueue(EnlQueueHandle, TX);
}

void
NetvTxQueue::Start(
    void
)
{
    auto link = NetvEnlMLink[m_adapter.EnlIndex].LinkHandle[0];
    auto port = &link->Ports[m_adapter.EnlPortIndex];
    auto queue = &port->TxQueue[0];

    NT_ASSERT(queue->State == Stopped);

    queue->QueueNext = queue->QueueEnd = 0U;

    EnlIndicateQueueState(EnlQueueHandle, Started);
}

void
NetvTxQueue::Stop(
    void
)
{
    EnlIndicateQueueState(EnlQueueHandle, Stopped);
}

_Use_decl_annotations_
void
NetvTxQueue::Advance(
    void
)
{
    auto pr = GetPacketRing();
    auto pi = NetRingGetAllPackets(m_rings);

    // drain Tx packets
    for (; NetPacketIteratorHasAny(&pi); NetPacketIteratorAdvance(&pi))
    {
        auto packet = NetPacketIteratorGetPacket(&pi);
        if (! packet->Scratch)
        {
            break;
        }

        auto fi = NetPacketIteratorGetFragments(&pi);
        for (; NetFragmentIteratorHasAny(&fi); NetFragmentIteratorAdvance(&fi))
        {
            continue;
        }

        m_rings->Rings[NetRingTypeFragment]->BeginIndex =
            NetFragmentIteratorGetIndex(&fi);
    }

    NetPacketIteratorSet(&pi);

    // post Tx packets
    EnlRingDoorBell(EnlQueueHandle, pr->EndIndex);
}

_Use_decl_annotations_
void
NetvTxQueue::Cancel(
    void
)
{
    auto ringBuffer = GetPacketRing();

    EnlRingDoorBell(EnlQueueHandle, ringBuffer->EndIndex);
}

_Use_decl_annotations_
void
NetvTxQueue::SetNotify(
    bool NotificationEnabled
)
{
    EnlArmInterrupt(EnlQueueHandle, NotificationEnabled);
}
