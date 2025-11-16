// Copyright (c) Microsoft Corporation.  All rights reserved.
#include "precomp.h"

#include "device.h"

#include "adapter.h"
#include "adapter.tmh"

_Use_decl_annotations_
NTSTATUS WifiIhvInitAdapterContext(_In_ WDFDEVICE Device, _In_ NETADAPTER NetAdapter)
{
    PWIFI_IHV_DEVICE_CONTEXT deviceContext = WifiGetIhvDeviceContext(Device);
    PWIFI_IHV_NETADAPTER_CONTEXT netAdapterContext = WifiGetIhvNetAdapterContext(NetAdapter);

    if (deviceContext->primaryStaAdapter == WDF_NO_HANDLE)
    {
        deviceContext->primaryStaAdapter = NetAdapter;
    }

    netAdapterContext->WifiDeviceContext = deviceContext;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiIhvAdapterStart(NETADAPTER netAdapter)
{
    TraceEntry();

    ULONG64 maxXmitLinkSpeed = WIFI_MEDIA_MAX_SPEED;
    ULONG64 maxRcvLinkSpeed = WIFI_MEDIA_MAX_SPEED;

    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(&linkLayerCapabilities, maxXmitLinkSpeed, maxRcvLinkSpeed);

    NetAdapterSetLinkLayerCapabilities(netAdapter, &linkLayerCapabilities);

    NetAdapterSetLinkLayerMtuSize(netAdapter, WIFI_MAX_PACKET_SIZE);

    static WDI_MAC_ADDRESS STAAddress = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    NET_ADAPTER_LINK_LAYER_ADDRESS permanentLinkLayerAddress;
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(&permanentLinkLayerAddress, sizeof(WDI_MAC_ADDRESS), STAAddress.Address);
    NetAdapterSetPermanentLinkLayerAddress(netAdapter, &permanentLinkLayerAddress);

    WIFI_ADAPTER_WAKE_CAPABILITIES wakeCap{};
    WIFI_ADAPTER_WAKE_CAPABILITIES_INIT(&wakeCap);
    if (WIFI_IS_FIELD_AVAILABLE(WIFI_ADAPTER_WAKE_CAPABILITIES, ClientDriverDiagnostic))
    {
        wakeCap.ClientDriverDiagnostic = true;
    }
    WifiAdapterSetWakeCapabilities(netAdapter, &wakeCap);

    NTSTATUS status = NetAdapterStart(netAdapter);
    ASSERT(STATUS_SUCCESS == status);

    TraceExit(status);

    return status;
}
