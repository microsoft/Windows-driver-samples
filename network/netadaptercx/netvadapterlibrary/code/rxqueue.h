// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "queue.h"

class NetvRxQueue final
    : public NetvQueue
{

public:

    NetvRxQueue(
        NETPACKETQUEUE Handle,
        NetvAdapter & Adapter
    );

    void
    Destroy(
        void
    );

    void
    Start(
        void
    );

    void
    Stop(
        void
    );

    void
    Advance(
        void
    );

    void
    Cancel(
        void
    );

    void
    SetNotify(
        bool Enable
    );

    ENLP_QUEUE * EnlQueueHandle;

    NET_EXTENSION VirtualAddressExtension;
    NET_EXTENSION UdpRscExtension;
    NET_EXTENSION RxXSumExtension;
    NET_EXTENSION NetMemoryExtension;
    NET_EXTENSION NetMemoryReturnContextExtensionIn;

    bool CheckedWakeFrame = false;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NetvRxQueue, NetvRxQueueGetContext);
