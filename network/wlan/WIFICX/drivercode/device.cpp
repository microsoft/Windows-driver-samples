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

    //NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    //NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks, EvtAdapterCreateTxQueue, EvtAdapterCreateRxQueue);

    //NetAdapterInitSetDatapathCallbacks(AdapterInit, &datapathCallbacks);

    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, WIFI_IHV_NETADAPTER_CONTEXT);
    adapterAttributes.EvtCleanupCallback = EvtAdapterCleanup;

    NETADAPTER netAdapter{};
    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        NetAdapterCreate(AdapterInit, &adapterAttributes, &netAdapter), "Failed to create NetAdapter");

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiAdapterInitialize(netAdapter), "Failed to initialize WifiAdapter");

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiIhvInitAdapterContext(Device, netAdapter), "Failed to initialize WifiAdapterContext");
    
    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiIhvAdapterStart(netAdapter), "Failed to start WifiIhvAdapter");

    return STATUS_SUCCESS;
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
void EvtAdapterCleanup(_In_ WDFOBJECT NetAdapter)
{
    UNREFERENCED_PARAMETER(NetAdapter);
    TraceEntry();
    TraceExit(STATUS_SUCCESS);
}