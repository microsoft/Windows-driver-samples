// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

class NetvAdapter;
struct ENLP_QUEUE;

class NetvQueue
{

public:

    NetvQueue(
        NETPACKETQUEUE Handle,
        NetvAdapter & Adapter,
        NET_RING_COLLECTION const * Rings
    );

    NET_RING *
    GetPacketRing(
        void
    );

    NET_RING *
    GetFragmentRing(
        void
    );

    NET_RING *
    GetNetMemoryReturnRing(
        void
    );

    NETPACKETQUEUE const
        m_handle{WDF_NO_HANDLE};

    NetvAdapter &
        m_adapter;

    NET_RING_COLLECTION const *
        m_rings;

    ENLP_QUEUE *
        EnlQueueHandle;

};
