//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "device.h"
#include "power.h"

MINIPORT_DRIVER_CONTEXT GlobalControl = {0};

EXTERN_C __declspec(code_seg("INIT")) DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

#define MBB_DRIVER_DEFAULT_POOL_TAG 'DBMW'

__declspec(code_seg("INIT")) NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDF_DRIVER_CONFIG driverConfig;
    WDF_DRIVER_CONFIG_INIT(&driverConfig, EvtDriverDeviceAdd);
    driverConfig.DriverPoolTag = MBB_DRIVER_DEFAULT_POOL_TAG;

    driverConfig.EvtDriverUnload = EvtDriverUnload;

    WDFDRIVER driver;
    status = WdfDriverCreate(driverObject, registryPath, WDF_NO_OBJECT_ATTRIBUTES, &driverConfig, &driver);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
EvtDriverDeviceAdd(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT deviceInit)
{
    UNREFERENCED_PARAMETER((driver));

    NTSTATUS status = STATUS_SUCCESS;

    status = NetDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = MbbDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    // Register with the NetAdapter framework that we want to do the Device Reset
    //NET_DEVICE_RESET_CONFIG resetConfig;
    //NET_DEVICE_RESET_CONFIG_INIT(&resetConfig, EvtMbbDeviceReset);
    //NetDeviceInitSetResetConfig(deviceInit, &resetConfig);

    NET_DEVICE_POWER_POLICY_EVENT_CALLBACKS netPowerPolicyCallbacks;
    NET_DEVICE_POWER_POLICY_EVENT_CALLBACKS_INIT(&netPowerPolicyCallbacks);
    netPowerPolicyCallbacks.EvtDevicePreviewBitmapPattern = EvtDevicePreviewBitmapPattern;

    NetDeviceInitSetPowerPolicyEventCallbacks(deviceInit, &netPowerPolicyCallbacks);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, WMBCLASS_DEVICE_CONTEXT);

    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSurpriseRemoval = EvtDeviceSurpriseRemoval;
    pnpPowerCallbacks.EvtDeviceD0Entry = EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = EvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &pnpPowerCallbacks);

    WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);
    powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = EvtDeviceArmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = EvtDeviceDisarmWakeFromS0;
    WdfDeviceInitSetPowerPolicyEventCallbacks(deviceInit, &powerPolicyCallbacks);

    WDFDEVICE wdfDevice;
    status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &wdfDevice);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    // Set the device to be not ejectable
    WDF_DEVICE_PNP_CAPABILITIES pnpCapabilities;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
    pnpCapabilities.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(wdfDevice, &pnpCapabilities);

    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(wdfDevice);
    deviceContext->WdfDevice = wdfDevice;

    MBB_DEVICE_CONFIG mbbDeviceConfig;
    MBB_DEVICE_CONFIG_INIT(
        &mbbDeviceConfig, EvtMbbDeviceSendMbimFragment, EvtMbbDeviceReceiveMbimFragment, EvtMbbDeviceSendDeviceServiceSessionData, EvtMbbDeviceCreateAdapter);

    status = MbbDeviceInitialize(wdfDevice, &mbbDeviceConfig);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    return status;
}

VOID EvtDriverUnload(_In_ WDFDRIVER driver)
{
    UNREFERENCED_PARAMETER(driver);
}
