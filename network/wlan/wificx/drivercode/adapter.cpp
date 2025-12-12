// Copyright (c) Microsoft Corporation.  All rights reserved.
#include "precomp.h"
#include "adapter.h"
#include "adapter.tmh"

extern UCHAR NetvMacAddressBase[MAC_ADDR_LEN];

NetvAdapter* NetvAdapterGetContextFromWDFObject(NETADAPTER netAdapter)
{
    WifiNetvAdapter* wifiNetvAdapter = WifiNetvAdapterGetContext(netAdapter);
    NetvAdapter* netvAdapter{ wifiNetvAdapter };
    return netvAdapter;
}

WifiNetvAdapter::WifiNetvAdapter(NETADAPTER Handle, WDFDEVICE Device) : NetvAdapter(Handle, Device)
{
}

NTSTATUS WifiNetvAdapter::Initialize()
{
    if (WifiGetIhvDeviceContext(m_device)->netAdapters[WifiAdapterGetPortId(m_handle)] != WDF_NO_HANDLE)
    {
        return STATUS_SUCCESS;
    }
    return NetvAdapter::Initialize();
}

NTSTATUS WifiNetvAdapter::AdapterStart()
{
    TraceEntry();

    NTSTATUS status = STATUS_SUCCESS;

    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES wakeMediaChangeCapabilities;
    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES_INIT(&wakeMediaChangeCapabilities);

    wakeMediaChangeCapabilities.MediaConnect = TRUE;
    wakeMediaChangeCapabilities.MediaDisconnect = TRUE;

    NetAdapterWakeSetMediaChangeCapabilities(m_handle, &wakeMediaChangeCapabilities);

    WIFI_ADAPTER_WAKE_CAPABILITIES wakeCap{};
    WIFI_ADAPTER_WAKE_CAPABILITIES_INIT(&wakeCap);
    if (WIFI_IS_FIELD_AVAILABLE(WIFI_ADAPTER_WAKE_CAPABILITIES, ClientDriverDiagnostic))
    {
        wakeCap.ClientDriverDiagnostic = true;
    }
    WifiAdapterSetWakeCapabilities(m_handle, &wakeCap);

    status = NetvAdapter::ConfigureDataCapabilities();
    if (!NT_SUCCESS(status))
    {
        WFCError("%!FUNC!: NetvAdapter::ConfigureDataCapabilities failed with %!STATUS!", status);
        return status;
    }

    status = NetAdapterStart(m_handle);
    if (!NT_SUCCESS(status))
    {
        WFCError("%!FUNC!: NetAdapterStart failed with %!STATUS!", status);
        return status;
    }

    ASSERT(STATUS_SUCCESS == status);
    TraceExit(status);

    return status;
}

NTSTATUS WifiNetvAdapter::CreateRxQueue(NETRXQUEUE_INIT* NetRxQueueInit)
{
    return NetvAdapter::CreateRxQueue(NetRxQueueInit);
}

NTSTATUS WifiNetvAdapter::CreateTxQueue(NETTXQUEUE_INIT* NetTxQueueInit)
{
    return NetvAdapter::CreateTxQueue(NetTxQueueInit);
}

void WifiNetvAdapter::Destroy(void)
{
    return NetvAdapter::Destroy();
}