// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.hpp"

#include <wil/resource.h>

#include "netvadapter.h"
#include "trace.h"
#include "driver.tmh"
#include "device.h"
#include "power.h"

GLOBAL_CONTEXT NetvGlobalContext;

EXTERN_C
DRIVER_INITIALIZE
    DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDriverDeviceAdd);
    config.EvtDriverUnload = EvtDriverUnload;

    NTSTATUS status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        NULL);

    if (!NT_SUCCESS(status))
    {
        LogError(FLAG_DRIVER, "%!STATUS! WdfDriverCreate", status);
        WPP_CLEANUP(DriverObject);

        return status;
    }

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
NTSTATUS
EvtDriverDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )
{
    UNREFERENCED_PARAMETER(Driver);

    RETURN_IF_NOT_STATUS_SUCCESS(
        NetDeviceInitConfig(DeviceInit));

    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);
    powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = EvtDeviceArmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = EvtDeviceDisarmWakeFromS0;
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, NetvDevice);

    WDFDEVICE wdfDevice;
    RETURN_IF_NOT_STATUS_SUCCESS(
        WdfDeviceCreate(&DeviceInit, &deviceAttributes, &wdfDevice));

    auto device = new (NetvDeviceGetContext(wdfDevice)) NetvDevice(wdfDevice);

    RETURN_IF_NOT_STATUS_SUCCESS(
        device->Initialize());

    RETURN_STATUS_SUCCESS();
}

_Use_decl_annotations_
VOID
EvtDriverUnload(
    WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}
