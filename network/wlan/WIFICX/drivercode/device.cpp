// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "precomp.h"

#include "adapter.h"
#include "device.h"
#include "device.tmh"

_Use_decl_annotations_
NTSTATUS EvtDevicePrepareHardware(WDFDEVICE device, WDFCMRESLIST resourcesRaw, WDFCMRESLIST resourcesTranslated)
{
    UNREFERENCED_PARAMETER(resourcesRaw);
    UNREFERENCED_PARAMETER(resourcesTranslated);

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiHAL::_Create(device),
        "WifiHAL::_Create failed");

    WFCInfo("Device=0x%p", device);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS EvtDeviceReleaseHardware(WDFDEVICE device, WDFCMRESLIST resourcesTranslated)
{
    UNREFERENCED_PARAMETER(resourcesTranslated);

    WFCInfo("Device=0x%p", device);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS EvtWifiDeviceCreateAdapter(WDFDEVICE Device, NETADAPTER_INIT* AdapterInit)
{
    if (WifiAdapterInitGetType(AdapterInit) != WIFI_ADAPTER_EXTENSIBLE_STATION)
    {
        WFCError("%!FUNC!: Unsupported adapter type = 0x%x != 0x%x", WifiAdapterInitGetType(AdapterInit), WIFI_ADAPTER_EXTENSIBLE_STATION);
        return STATUS_NOT_SUPPORTED;
    }

    NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks, EvtAdapterCreateTxQueue, EvtAdapterCreateRxQueue);

    NetAdapterInitSetDatapathCallbacks(AdapterInit, &datapathCallbacks);

    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, WifiNetvAdapter);
    adapterAttributes.EvtCleanupCallback = EvtAdapterCleanup;

    NETADAPTER netAdapter;
    NTSTATUS ntStatus = NetAdapterCreate(AdapterInit, &adapterAttributes, &netAdapter);
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: NetAdapterCreate failed, status=0x%x", ntStatus);
        return ntStatus;
    }

    ntStatus = WifiAdapterInitialize(netAdapter);
    ASSERT(NT_SUCCESS(ntStatus));
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: WifiAdapterInitialize failed with %!STATUS!", ntStatus);
        return ntStatus;
    }
    auto wifiNetvAdapter = new (reinterpret_cast<void*>(WifiNetvAdapterGetContext(netAdapter))) WifiNetvAdapter(netAdapter, Device);
    ntStatus = wifiNetvAdapter->Initialize();
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: WifiNetvAdapter::Initialize failed with %!STATUS!", ntStatus);
        return ntStatus;
    }

    ntStatus = wifiNetvAdapter->AdapterStart();
    ASSERT(NT_SUCCESS(ntStatus));
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: WifiNetvAdapter::AdapterStart failed with %!STATUS!", ntStatus);
        return ntStatus;
    }

    WFCInfo("%!FUNC!: Success!");
    return ntStatus;
}

_Use_decl_annotations_
void EvtAdapterCleanup(_In_ WDFOBJECT NetAdapter)
{
    TraceEntry();
    auto wifiNetvAdapter = WifiNetvAdapterGetContext(NetAdapter);
    wifiNetvAdapter->Destroy();
    TraceExit(STATUS_SUCCESS);
}


_Use_decl_annotations_
NTSTATUS EvtWifiDeviceCreateWifiDirectDevice(WDFDEVICE, WIFIDIRECT_DEVICE_INIT*)
{
    NTSTATUS status = STATUS_SUCCESS;
    TraceEntry();
    TraceExit(status);
    return status;
}


_Use_decl_annotations_
NTSTATUS
EvtAdapterCreateTxQueue(NETADAPTER Adapter, NETTXQUEUE_INIT* Init)
{
    TraceEntry();
    return WifiNetvAdapterGetContext(Adapter)->CreateTxQueue(Init);
}

_Use_decl_annotations_
NTSTATUS
EvtAdapterCreateRxQueue(NETADAPTER Adapter, NETRXQUEUE_INIT* Init)
{
    TraceEntry();
    return WifiNetvAdapterGetContext(Adapter)->CreateRxQueue(Init);
}
