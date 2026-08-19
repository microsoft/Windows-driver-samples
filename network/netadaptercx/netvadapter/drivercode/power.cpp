// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"
#include "netvadapter.h"
#include "device.h"
#include "trace.h"
#include "power.h"
#include "power.tmh"

void
NetvDeviceInitializePowerManagement(
    _In_ NetvDevice * Device
)
{
#ifdef _KERNEL_MODE
    WDFDEVICE wdfDevice = Device->m_handle;
    NetvAdapter * adapter = Device->m_adapter;

    if (!adapter->S0Idle)
    {
        return;
    }

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
        &idleSettings,
        IdleCanWakeFromS0);

    idleSettings.IdleTimeout = 3000;
    idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

    NTSTATUS ntStatus = WdfDeviceAssignS0IdleSettings(
        wdfDevice,
        &idleSettings);

    if (ntStatus != STATUS_SUCCESS)
    {
        // MSDN says we should ignore error codes from assign S0 idle settings, do that
        return;
    }

    // Now tell NetAdapterCx we support wake on packet filter match
    NET_ADAPTER_WAKE_PACKET_FILTER_CAPABILITIES wakeOnPacketFilterMatch;
    NET_ADAPTER_WAKE_PACKET_FILTER_CAPABILITIES_INIT(&wakeOnPacketFilterMatch);
    wakeOnPacketFilterMatch.PacketFilterMatch = TRUE;

    NetAdapterWakeSetPacketFilterCapabilities(adapter->m_handle, &wakeOnPacketFilterMatch);
#else
    UNREFERENCED_PARAMETER(Device);
#endif
}

_Use_decl_annotations_
NTSTATUS
EvtDeviceArmWakeFromS0(
    WDFDEVICE Device
)
{
    auto adapter = NetvDeviceGetContext(Device)->m_adapter;
    adapter->ArmWakeFromS0();

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void
EvtDeviceDisarmWakeFromS0(
    WDFDEVICE Device
)
{
    auto adapter = NetvDeviceGetContext(Device)->m_adapter;
    adapter->DisarmWakeFromS0();
}
