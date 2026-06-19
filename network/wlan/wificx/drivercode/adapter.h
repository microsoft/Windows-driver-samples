// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "device.h"
#include "netvadapter.h"

// for STA TX DEMUX
#define MaxNumOfPeers 1

// Context for each "Wdi Port"[NetAdapter] instance.
// Each NetAdapter instance corresponds to an IP interface
typedef struct _WIFI_IHV_NETADAPTER_CONTEXT
{
    PWIFI_IHV_DEVICE_CONTEXT WifiDeviceContext; // Wdf Ihv device context
    NETADAPTER NetAdapter;                  // NetAdapter object
} WIFI_IHV_NETADAPTER_CONTEXT, * PWIFI_IHV_NETADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_IHV_NETADAPTER_CONTEXT, WifiGetIhvNetAdapterContext);

class WifiNetvAdapter : public NetvAdapter
{
public:
    WifiNetvAdapter(NETADAPTER Handle, WDFDEVICE Device);

    NTSTATUS
        Initialize();

    NTSTATUS
        AdapterStart();

    NTSTATUS
        CreateRxQueue(NETRXQUEUE_INIT* NetRxQueueInit);

    NTSTATUS
        CreateTxQueue(NETTXQUEUE_INIT* NetTxQueueInit);

    void Destroy(void);

    bool CanReportWifiWakeSourceTypeClientDriverDiagnostic;

};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WifiNetvAdapter, WifiNetvAdapterGetContext);