// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"
#include "adapter.h"
#include "rxqueue.h"
#include "memory.h"

static
void
CheckForWakeFrame(
    NetvRxQueue * rx
)
{
    NET_RING_FRAGMENT_ITERATOR fi = NetRingGetAllFragments(rx->m_rings);

    if (! NetFragmentIteratorHasAny(&fi))
    {
        return;
    }

    auto *fragment = NetFragmentIteratorGetFragment(&fi);
    auto *rxVirtualAddress = NetExtensionGetFragmentVirtualAddress(
        &rx->VirtualAddressExtension,
        NetFragmentIteratorGetIndex(&fi));

    auto *fragmentBuffer = reinterpret_cast<unsigned char *>(rxVirtualAddress->VirtualAddress) + fragment->Offset;

    fragment->ValidLength = EnlCopyWakeFrame(
        NetvEnlMLink[rx->m_adapter.EnlIndex].LinkHandle[0],
        fragmentBuffer,
        fragment->Capacity);

    // If there was a pending wake frame mark this fragment as complete, the normal advance code will get to it
    fragment->Scratch = fragment->ValidLength > 0 ? 1 : 0;

    rx->CheckedWakeFrame = true;
}


NetvRxQueue::NetvRxQueue(
    NETPACKETQUEUE Handle,
    NetvAdapter & Adapter
)
    : NetvQueue{Handle, Adapter, NetRxQueueGetRingCollection(Handle)}
{
    NET_EXTENSION_QUERY extension;

    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_NAME,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_VERSION_1,
        NetExtensionTypeFragment);

    NetRxQueueGetExtension(m_handle, &extension, &VirtualAddressExtension);

    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_PACKET_EXTENSION_RSC_NAME,
        NET_PACKET_EXTENSION_RSC_VERSION_2,
        NetExtensionTypePacket);

    NetRxQueueGetExtension(m_handle, &extension, &UdpRscExtension);

    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_PACKET_EXTENSION_CHECKSUM_NAME,
        NET_PACKET_EXTENSION_CHECKSUM_VERSION_1,
        NetExtensionTypePacket);

    NetRxQueueGetExtension(m_handle, &extension, &RxXSumExtension);

    if (Adapter.PreallocatedRxBuffers)
    {
        NET_EXTENSION_QUERY_INIT(
            &extension,
            NET_FRAGMENT_EXTENSION_NET_MEMORY_NAME,
            NET_FRAGMENT_EXTENSION_NET_MEMORY_VERSION_1,
            NetExtensionTypeFragment);

        NetRxQueueGetExtension(m_handle, &extension, &NetMemoryExtension);

        NET_EXTENSION_QUERY_INIT(
            &extension,
            NET_FRAGMENT_EXTENSION_RETURN_CONTEXT_NAME,
            NET_FRAGMENT_EXTENSION_RETURN_CONTEXT_VERSION_1,
            NetExtensionTypeFragment);

        NetRxQueueGetExtension(m_handle, &extension, &NetMemoryReturnContextExtensionIn);
    }

    EnlQueueHandle = EnlCreateQueue(Handle, RX);
}

_Use_decl_annotations_
void
NetvRxQueue::Destroy(
    void
)
{
    EnlDestroyQueue(EnlQueueHandle, RX);
}

void
NetvRxQueue::Start(
    void
)
{
    auto link = NetvEnlMLink[m_adapter.EnlIndex].LinkHandle[0];
    auto port = &link->Ports[m_adapter.EnlPortIndex];
    auto queue = &port->RxQueue[0];

    WDFVERIFY(queue->State == Stopped);

    queue->QueueNext = queue->QueueEnd = 0U;

    EnlIndicateQueueState(EnlQueueHandle, Started);
}

void
NetvRxQueue::Stop(
    void
)
{
    EnlIndicateQueueState(EnlQueueHandle, Stopped);
}

_Use_decl_annotations_
void
NetvRxQueue::Advance(
    void
)
{
    auto fr = GetFragmentRing();
    NET_RING_PACKET_ITERATOR pi = NetRingGetAllPackets(m_rings);
    NET_RING_FRAGMENT_ITERATOR fi = NetRingGetAllFragments(m_rings);

    // Ideally this would run in EvtQueueStart, but at that point the receive buffers are not
    // attached to the fragment yet
    if (! CheckedWakeFrame)
    {
        CheckForWakeFrame(this);
    }

    // Move begin index forward for all fragments with Scratch == 1, thus returning them to the OS since we're done processing them.
    for (; NetFragmentIteratorHasAny(&fi) && NetPacketIteratorHasAny(&pi); NetPacketIteratorAdvance(&pi), NetFragmentIteratorAdvance(&fi))
    {
        NET_FRAGMENT const * fragment = NetFragmentIteratorGetFragment(&fi);
        if (! fragment->Scratch)
        {
            break;
        }
    }

    if (m_adapter.PreallocatedRxBuffers)
    {
        NET_RING* dataBufferRing = GetNetMemoryReturnRing();
        while (dataBufferRing->BeginIndex != dataBufferRing->EndIndex)
        {
            NET_FRAGMENT_RETURN_CONTEXT* netMemoryReturnContextOut =
                NetRingGetFragmentReturnContextAtIndex(
                    dataBufferRing,
                    dataBufferRing->BeginIndex);

            MemoryBuffer* memoryBuffer = reinterpret_cast<MemoryBuffer*>(netMemoryReturnContextOut->Handle);
            GetMemoryFromHandle(m_adapter.m_preallocatedRxBuffers)->ReturnBuffer(memoryBuffer);
            dataBufferRing->BeginIndex = NetRingIncrementIndex(dataBufferRing, dataBufferRing->BeginIndex);
        }
    }

    NetFragmentIteratorSet(&fi);
    NetPacketIteratorSet(&pi);
    EnlRingDoorBell(EnlQueueHandle, fr->EndIndex);
}

_Use_decl_annotations_
void
NetvRxQueue::Cancel(
    void
)
{
    CancelRxPackets(m_rings);
}

_Use_decl_annotations_
void
NetvRxQueue::SetNotify(
    bool NotificationEnabled
)
{
    EnlArmInterrupt(EnlQueueHandle, NotificationEnabled);
}
