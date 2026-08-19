// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"

#include <wil/resource.h>

#include "netvadapter.h"
#include "device.h"
#include "trace.h"
#include "device.tmh"
#include "power.h"

static
EVT_NET_ADAPTER_CREATE_TXQUEUE
    EvtAdapterCreateTxQueue;

static
EVT_NET_ADAPTER_CREATE_RXQUEUE
    EvtAdapterCreateRxQueue;

NetvDevice::NetvDevice(
    WDFDEVICE Handle
)
    : m_triage(WdfGetTriageInfo())
    , m_handle(Handle)
{
}

_Use_decl_annotations_
NTSTATUS
NetvDevice::Initialize(
    void
)
{
    using unique_adapterinit = wil::unique_any<NETADAPTER_INIT*,
        decltype(&::NetAdapterInitFree),
        ::NetAdapterInitFree>;

    unique_adapterinit adapterInit{NetAdapterInitAllocate(m_handle)};
    RETURN_NTSTATUS_IF(
        STATUS_INSUFFICIENT_RESOURCES,
        ! adapterInit);

    NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    NET_ADAPTER_DATAPATH_CALLBACKS_INIT(
        &datapathCallbacks,
        EvtAdapterCreateTxQueue,
        EvtAdapterCreateRxQueue);

    NetAdapterInitSetDatapathCallbacks(
        adapterInit.get(),
        &datapathCallbacks);

    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, NetvAdapter);
    adapterAttributes.EvtDestroyCallback = [](WDFOBJECT Handle) {
        NetvAdapterGetContext(static_cast<NETADAPTER>(Handle))->Destroy();
    };

    NETADAPTER netAdapter;
    RETURN_IF_NOT_STATUS_SUCCESS(
        NetAdapterCreate(adapterInit.get(), &adapterAttributes, &netAdapter));

    m_adapter = new (NetvAdapterGetContext(netAdapter)) NetvAdapter(netAdapter, m_handle);

    RETURN_IF_NOT_STATUS_SUCCESS(
        m_adapter->Initialize());

    RETURN_STATUS_SUCCESS();
}

NTSTATUS
NetvDevice::PrepareHardware(
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    NetvDeviceInitializePowerManagement(this);

    RETURN_IF_NOT_STATUS_SUCCESS(
        m_adapter->ConfigureDataCapabilities());

    RETURN_IF_NOT_STATUS_SUCCESS(
        NetAdapterStart(m_adapter->m_handle));

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
NTSTATUS
EvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
    )
{
    return NetvDeviceGetContext(Device)->PrepareHardware(ResourcesRaw, ResourcesTranslated);
}

_Use_decl_annotations_
NTSTATUS
EvtAdapterCreateTxQueue(
    NETADAPTER Adapter,
    NETTXQUEUE_INIT * Init
)
{
    return NetvAdapterGetContext(Adapter)->CreateTxQueue(Init);
}

_Use_decl_annotations_
NTSTATUS
EvtAdapterCreateRxQueue(
    NETADAPTER Adapter,
    NETRXQUEUE_INIT * Init
)
{
    return NetvAdapterGetContext(Adapter)->CreateRxQueue(Init);
}

NetvAdapter* NetvAdapterGetContextFromWDFObject(NETADAPTER netAdapter)
{
    return NetvAdapterGetContext(netAdapter);
}
