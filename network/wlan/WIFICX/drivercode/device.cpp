//-------------------------------------------------------------------------------
// Net Adapter source file
//
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
    UNREFERENCED_PARAMETER(device);
    NTSTATUS status = STATUS_SUCCESS;

    //status = WifiCxTestSetDeviceCapabilities(device);

    if (!NT_SUCCESS(status))
    {
        WFCError("%!FUNC!: WifiCxTestSetDeviceCapabilities failed with %!STATUS!", status);
        return status;
    }

    WFCInfo("Device=0x%p", device);
    return status;
}

_Use_decl_annotations_
NTSTATUS EvtDeviceReleaseHardware(WDFDEVICE device, WDFCMRESLIST resourcesTranslated)
{
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(resourcesTranslated);

    WFCInfo("Device=0x%p", device);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void EvtDeviceSurpriseRemoval(WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(device);
    WFCInfo("Device=0x%p is surprise removed", device);
}

static NTSTATUS WifiInitAdapterContext(_In_ WDFDEVICE Device, _In_ NETADAPTER NetAdapter)
{
    PWIFI_DEVICE_CONTEXT deviceContext = WifiGetDeviceContext(Device);
    PWIFI_NETADAPTER_CONTEXT netAdapterContext = WifiGetNetAdapterContext(NetAdapter);
    NTSTATUS status = STATUS_SUCCESS;

    deviceContext->LastConnectEntryId = 0; // Disconnected State
    deviceContext->LastConnectTransactionId = 0;
    deviceContext->CurrentRadioState = TRUE;
    if (deviceContext->primaryStaAdapter == WDF_NO_HANDLE)
    {
        deviceContext->primaryStaAdapter = NetAdapter;
    }

    InitializeListHead(&netAdapterContext->ReceiveList);
    netAdapterContext->WifiDeviceContext = deviceContext;
    netAdapterContext->NetAdapter = NetAdapter;
    return status;
}

_Use_decl_annotations_
NTSTATUS EvtWifiDeviceCreateAdapter(WDFDEVICE Device, NETADAPTER_INIT* AdapterInit)
{

    //NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    //NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks, EvtAdapterCreateTxQueue, EvtAdapterCreateRxQueue);

    //NetAdapterInitSetDatapathCallbacks(AdapterInit, &datapathCallbacks);

    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, WIFI_NETADAPTER_CONTEXT);
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

    ntStatus = WifiInitAdapterContext(Device, netAdapter);
    ASSERT(NT_SUCCESS(ntStatus));
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: WifiInitAdapterContext failed with %!STATUS!", ntStatus);
        return ntStatus;
    }

    ntStatus = WifiCxTestAdapterStart(netAdapter);
    ASSERT(NT_SUCCESS(ntStatus));
    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: WifiCxTestAdapterStart failed with %!STATUS!", ntStatus);
        return ntStatus;
    }

    return ntStatus;
}

_Use_decl_annotations_
void EvtAdapterCleanup(_In_ WDFOBJECT NetAdapter)
{
    UNREFERENCED_PARAMETER(NetAdapter);
    TraceEntry();
    TraceExit(STATUS_SUCCESS);
}