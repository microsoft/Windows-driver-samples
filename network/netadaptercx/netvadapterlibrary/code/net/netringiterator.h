// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <net/ringcollection.h>

typedef struct _NET_RING_ITERATOR
{

    NET_RING_COLLECTION const*
        Rings;

    UINT32* const
        IndexToSet;

    UINT32
        Index;

    UINT32 const
        End;

} NET_RING_ITERATOR;

typedef struct _NET_RING_PACKET_ITERATOR
{

    NET_RING_ITERATOR
        Iterator;

} NET_RING_PACKET_ITERATOR;

typedef struct _NET_RING_FRAGMENT_ITERATOR
{

    NET_RING_ITERATOR
        Iterator;

} NET_RING_FRAGMENT_ITERATOR;


inline
NET_RING_PACKET_ITERATOR
NetRingGetPostPackets(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypePacket];
    NET_RING_PACKET_ITERATOR iterator = {
        Rings, &ring->NextIndex, ring->NextIndex, ring->EndIndex,
    };

    return iterator;
}

inline
NET_RING_PACKET_ITERATOR
NetRingGetDrainPackets(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypePacket];
    NET_RING_PACKET_ITERATOR iterator = {
        Rings, &ring->BeginIndex, ring->BeginIndex, ring->NextIndex,
    };

    return iterator;
}

inline
NET_RING_PACKET_ITERATOR
NetRingGetAllPackets(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypePacket];
    NET_RING_PACKET_ITERATOR iterator = {
        Rings, &ring->BeginIndex, ring->BeginIndex, ring->EndIndex,
    };

    return iterator;
}

inline
NET_PACKET*
NetPacketIteratorGetPacket(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    return NetRingGetPacketAtIndex(
        Iterator->Iterator.Rings->Rings[NetRingTypePacket],
        Iterator->Iterator.Index);
}

inline
UINT32
NetPacketIteratorGetIndex(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    return Iterator->Iterator.Index;
}

inline
BOOLEAN
NetPacketIteratorHasAny(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    return Iterator->Iterator.Index != Iterator->Iterator.End;
}

inline
UINT32
NetPacketIteratorGetCount(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    NET_RING const* ring = Iterator->Iterator.Rings->Rings[NetRingTypePacket];

    return (Iterator->Iterator.End - Iterator->Iterator.Index) & ring->ElementIndexMask;
}

inline
void
NetPacketIteratorAdvance(
    _In_ NET_RING_PACKET_ITERATOR* Iterator
)
{
    Iterator->Iterator.Index = NetRingIncrementIndex(
        Iterator->Iterator.Rings->Rings[NetRingTypePacket],
        Iterator->Iterator.Index);
}

inline
void
NetPacketIteratorAdvanceToTheEnd(
    _In_ NET_RING_PACKET_ITERATOR* Iterator
)
{
    Iterator->Iterator.Index = Iterator->Iterator.End;
}

inline
void
NetPacketIteratorSet(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    *Iterator->Iterator.IndexToSet
        = Iterator->Iterator.Index;
}


inline
NET_RING_FRAGMENT_ITERATOR
NetPacketIteratorGetFragments(
    _In_ NET_RING_PACKET_ITERATOR const* Iterator
)
{
    NET_RING const* ring = Iterator->Iterator.Rings->Rings[NetRingTypeFragment];
    NET_PACKET const* packet = NetPacketIteratorGetPacket(Iterator);
    UINT32 const end = NetRingIncrementIndex(ring,
        packet->FragmentIndex + packet->FragmentCount - 1);
    NET_RING_FRAGMENT_ITERATOR iterator = {
        Iterator->Iterator.Rings, NULL, packet->FragmentIndex, end,
    };

    return iterator;
}

inline
NET_RING_FRAGMENT_ITERATOR
NetRingGetPostFragments(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypeFragment];
    NET_RING_FRAGMENT_ITERATOR iterator = {
        Rings, &ring->NextIndex, ring->NextIndex, ring->EndIndex,
    };

    return iterator;
}

inline
NET_RING_FRAGMENT_ITERATOR
NetRingGetDrainFragments(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypeFragment];
    NET_RING_FRAGMENT_ITERATOR iterator = {
        Rings, &ring->BeginIndex, ring->BeginIndex, ring->NextIndex,
    };

    return iterator;
}

inline
NET_RING_FRAGMENT_ITERATOR
NetRingGetAllFragments(
    _In_ NET_RING_COLLECTION const* Rings
)
{
    NET_RING* ring = Rings->Rings[NetRingTypeFragment];
    NET_RING_FRAGMENT_ITERATOR iterator = {
        Rings, &ring->BeginIndex, ring->BeginIndex, ring->EndIndex,
    };

    return iterator;
}

inline
NET_FRAGMENT*
NetFragmentIteratorGetFragment(
    _In_ NET_RING_FRAGMENT_ITERATOR const* Iterator
)
{
    return NetRingGetFragmentAtIndex(
        Iterator->Iterator.Rings->Rings[NetRingTypeFragment],
        Iterator->Iterator.Index);
}

inline
UINT32
NetFragmentIteratorGetIndex(
    _In_ NET_RING_FRAGMENT_ITERATOR const* Iterator
)
{
    return Iterator->Iterator.Index;
}

inline
BOOLEAN
NetFragmentIteratorHasAny(
    _In_ NET_RING_FRAGMENT_ITERATOR const* Iterator
)
{
    return Iterator->Iterator.Index != Iterator->Iterator.End;
}

inline
UINT32
NetFragmentIteratorGetCount(
    _In_ NET_RING_FRAGMENT_ITERATOR const* Iterator
)
{
    NET_RING const* ring = Iterator->Iterator.Rings->Rings[NetRingTypeFragment];

    return (Iterator->Iterator.End - Iterator->Iterator.Index) & ring->ElementIndexMask;
}

inline
void
NetFragmentIteratorAdvance(
    _In_ NET_RING_FRAGMENT_ITERATOR* Iterator
)
{
    Iterator->Iterator.Index = NetRingIncrementIndex(
        Iterator->Iterator.Rings->Rings[NetRingTypeFragment],
        Iterator->Iterator.Index);
}

inline
void
NetFragmentIteratorAdvanceToTheEnd(
    _In_ NET_RING_FRAGMENT_ITERATOR* Iterator
)
{
    Iterator->Iterator.Index = Iterator->Iterator.End;
}

inline
void
NetFragmentIteratorSet(
    _In_ NET_RING_FRAGMENT_ITERATOR const* Iterator
)
{
    *(Iterator->Iterator.IndexToSet)
        = Iterator->Iterator.Index;
}
