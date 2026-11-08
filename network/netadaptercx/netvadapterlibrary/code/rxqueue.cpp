// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"
#include "netvadapter.h"
#include "rxqueue.h"
#include "memory.h"

#if defined(_KERNEL_MODE) && defined(NETV_DATAPATH_DEBUG)
// Rx-indication logging budget.  Non-static so enl.cpp can reset it when the
// link goes idle (see g_enlFwdDbg).
LONG g_rxAdvDbg = 0;
#endif

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
#if defined(_KERNEL_MODE) && defined(NETV_DATAPATH_DEBUG)
    bool const ranWake = ! CheckedWakeFrame;
#endif
    if (! CheckedWakeFrame)
    {
        CheckForWakeFrame(this);
    }

#if defined(_KERNEL_MODE) && defined(NETV_DATAPATH_DEBUG)
    ULONG const dbgFBegin = fr->BeginIndex;
    ULONG const dbgFEnd   = fr->EndIndex;
    int   const dbgHaveFrag = NetFragmentIteratorHasAny(&fi) ? 1 : 0;
    int   const dbgHavePkt  = NetPacketIteratorHasAny(&pi)  ? 1 : 0;
    int         dbgFirstScratch = -1;
    if (dbgHaveFrag)
        dbgFirstScratch = (int)NetFragmentIteratorGetFragment(&fi)->Scratch;
    ULONG dbgDrained = 0;
#endif

    // Move begin index forward for all fragments with Scratch == 1, thus returning them to the OS since we're done processing them.
    for (; NetFragmentIteratorHasAny(&fi) && NetPacketIteratorHasAny(&pi); NetPacketIteratorAdvance(&pi), NetFragmentIteratorAdvance(&fi))
    {
        NET_FRAGMENT const * fragment = NetFragmentIteratorGetFragment(&fi);
        if (! fragment->Scratch)
        {
            break;
        }
#if defined(_KERNEL_MODE) && defined(NETV_DATAPATH_DEBUG)
        dbgDrained++;
#endif
    }

    NetFragmentIteratorSet(&fi);
    NetPacketIteratorSet(&pi);
    EnlRingDoorBell(EnlQueueHandle, fr->EndIndex);

#if defined(_KERNEL_MODE) && defined(NETV_DATAPATH_DEBUG)
    if ((dbgDrained > 0 || ranWake || dbgFirstScratch == 1) &&
        InterlockedIncrement(&g_rxAdvDbg) <= 40)
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RX adv port=%u fBegin=%lu fEnd=%lu haveFrag=%d havePkt=%d firstScr=%d drained=%lu wake=%d\n",
            (ULONG)m_adapter.EnlPortIndex, dbgFBegin, dbgFEnd,
            dbgHaveFrag, dbgHavePkt, dbgFirstScratch, dbgDrained, ranWake ? 1 : 0);
#endif
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
