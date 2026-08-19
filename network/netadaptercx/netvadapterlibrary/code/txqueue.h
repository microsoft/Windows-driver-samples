// Copyright (c) Microsoft Corporation. All rights reserved

#include "queue.h"

class NetvTxQueue final
    : public NetvQueue
{

public:

    NetvTxQueue(
        NETPACKETQUEUE Handle,
        NetvAdapter & Adapter
    ) noexcept;

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
    NET_EXTENSION UsoExtension;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NetvTxQueue, NetvTxQueueGetContext);
