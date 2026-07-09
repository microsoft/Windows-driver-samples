// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <net/virtualaddress.h>

//
// Following are some helper APIs for common ring manipulations.
// They are all implemented using iterator
//

inline
SIZE_T
GetTxPacketDataLength(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    SIZE_T length = 0;

    for (NET_RING_FRAGMENT_ITERATOR fi = NetPacketIteratorGetFragments(Iterator);
        NetFragmentIteratorHasAny(&fi);
        NetFragmentIteratorAdvance(&fi))
    {
        NET_FRAGMENT* fragment = NetFragmentIteratorGetFragment(&fi);
        length += (SIZE_T)fragment->ValidLength;
    }

    return length;
}

inline
VOID
CompleteTxPacketsBatch(
    _In_ NET_RING_COLLECTION const* Rings,
    _In_ UINT32 BatchSize
)
{
    UINT32 packetCount = 0;

    NET_RING_PACKET_ITERATOR pi = NetRingGetDrainPackets(Rings);

    while (NetPacketIteratorHasAny(&pi))
    {
        NET_PACKET* packet = NetPacketIteratorGetPacket(&pi);

        // this function uses Scratch field as the bit for testing completion
        if (!packet->Scratch)
        {
            break;
        }

        packetCount++;

        NET_RING_FRAGMENT_ITERATOR fi = NetPacketIteratorGetFragments(&pi);
        NetFragmentIteratorAdvanceToTheEnd(&fi);

        NetPacketIteratorAdvance(&pi);

        if (packetCount >= BatchSize)
        {
            NetPacketIteratorSet(&pi);
            Rings->Rings[NetRingTypeFragment]->BeginIndex = NetFragmentIteratorGetIndex(&fi);
        }
    }
}

inline
void
CancelRxPackets(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING_PACKET_ITERATOR pi = NetRingGetAllPackets(Rings);

    for (; NetPacketIteratorHasAny(&pi); NetPacketIteratorAdvance(&pi))
    {
        NetPacketIteratorGetPacket(&pi)->Ignore = 1;
    }

    NetPacketIteratorSet(&pi);

    NET_RING_FRAGMENT_ITERATOR fi = NetRingGetAllFragments(Rings);
    NetFragmentIteratorAdvanceToTheEnd(&fi);
    NetFragmentIteratorSet(&fi);
}
