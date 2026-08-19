// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"

#include <net/ringcollection.h>

#include "queue.h"

NetvQueue::NetvQueue(
    NETPACKETQUEUE Handle,
    NetvAdapter & Adapter,
    NET_RING_COLLECTION const * Rings
)
    : m_handle{Handle}
    , m_adapter{Adapter}
    , m_rings{Rings}
{
}

NET_RING *
NetvQueue::GetPacketRing(
    void
)
{
    return NetRingCollectionGetPacketRing(m_rings);
}

NET_RING *
NetvQueue::GetFragmentRing(
    void
)
{
    return NetRingCollectionGetFragmentRing(m_rings);
}
