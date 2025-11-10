// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

#include "device.h"

// packet and header sizes
#define WIFI_MAX_PACKET_SIZE (1514)

// maximum link speed for send and recv in bps
#define WIFI_MEDIA_MAX_SPEED 1'000'000'000

//
// NetAdapter functions
//

EVT_NET_ADAPTER_CREATE_TXQUEUE EvtAdapterCreateTxQueue;
EVT_NET_ADAPTER_CREATE_RXQUEUE EvtAdapterCreateRxQueue;

EVT_WDF_DEVICE_CONTEXT_DESTROY MbbDestroyAdapterContext;

NTSTATUS WifiCxTestAdapterStart(_In_ NETADAPTER netAdapter);
