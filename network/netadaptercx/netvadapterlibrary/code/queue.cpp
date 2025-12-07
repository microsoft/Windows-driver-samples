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

#if ((NETADAPTER_VERSION_MAJOR == 2) && (NETADAPTER_VERSION_MINOR >= 6))
NET_RING *
NetvQueue::GetNetMemoryReturnRing(
    void
)
{
    return NetRingCollectionGetFragmentReturnContextRing(m_rings);
}
#endif // NETCX 2.6 only