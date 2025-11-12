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


// Context for each "Wdi Port"[NetAdapter] instance.
// Each NetAdapter instance corresponds to an IP interface
typedef struct _WIFI_IHV_NETADAPTER_CONTEXT
{
    PWIFI_IHV_DEVICE_CONTEXT WifiDeviceContext; // Wdf Ihv device context
    NETADAPTER NetAdapter;                  // NetAdapter object
} WIFI_IHV_NETADAPTER_CONTEXT, * PWIFI_IHV_NETADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_IHV_NETADAPTER_CONTEXT, WifiGetIhvNetAdapterContext);
