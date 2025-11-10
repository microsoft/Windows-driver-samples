//-------------------------------------------------------------------------------
// Net Adapter source file
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "precomp.h"

#include "device.h"
#include "driver.tmh"

EXTERN_C __declspec(code_seg("INIT")) DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;



#if __cplusplus >= 201703L
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif

__declspec(code_seg("INIT")) NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
    NTSTATUS status = STATUS_SUCCESS;

    WPP_INIT_TRACING(driverObject, registryPath);
    TraceEntry();

    WDF_DRIVER_CONFIG driverConfig;
    WDF_DRIVER_CONFIG_INIT(&driverConfig, EvtDriverDeviceAdd);
    driverConfig.DriverPoolTag = WIFI_DRIVER_DEFAULT_POOL_TAG;

    driverConfig.EvtDriverUnload = EvtDriverUnload;

    WDFDRIVER driver;
    status = WdfDriverCreate(driverObject, registryPath, WDF_NO_OBJECT_ATTRIBUTES, &driverConfig, &driver);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDriverCreate failed 0x%x", status);
        goto Exit;
    }

Exit:
    TraceExit(status);

    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(driverObject);
    }

    return status;
}

NTSTATUS EvtDriverDeviceAdd(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT deviceInit)
{
    UNREFERENCED_PARAMETER(driver);

    TraceEntry();

    NTSTATUS status = STATUS_SUCCESS;

    status = NetDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        WFCError("NetDeviceInitConfig failed, status=0x%x", status);
        goto Exit;
    }

    status = WifiDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        WFCError("WifiDeviceInitConfig failed, status=0x%x", status);
        goto Exit;
    }

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, WIFI_DEVICE_CONTEXT);

    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSurpriseRemoval = EvtDeviceSurpriseRemoval;

    WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &pnpPowerCallbacks);

    WDFDEVICE wdfDevice;
    status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &wdfDevice);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDeviceCreate failed, status=0x%x", status);
        goto Exit;
    }

    status = WdfDeviceCreateDeviceInterface(wdfDevice, &GUID_WIFICX_SAMPLE_CLIENT_INTERFACE, nullptr);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDeviceCreateDeviceInterface failed with status=0x%x", status);
        goto Exit;
    }

    // Set the device to be not ejectable
    WDF_DEVICE_PNP_CAPABILITIES pnpCapabilities;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
    pnpCapabilities.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(wdfDevice, &pnpCapabilities);

    WIFI_DEVICE_CONFIG wifiDeviceConfig;
    WIFI_DEVICE_CONFIG_INIT(
        &wifiDeviceConfig, WDI_VERSION_LATEST, EvtWifiDeviceSendCommand, EvtWifiDeviceCreateAdapter, nullptr);

    status = WifiDeviceInitialize(wdfDevice, &wifiDeviceConfig);
    if (!NT_SUCCESS(status))
    {
        WFCError("WifiDeviceInitialize failed, status=0x%x", status);
        goto Exit;
    }

    PWIFI_DEVICE_CONTEXT deviceContext = WifiGetDeviceContext(wdfDevice);
    deviceContext->WdfDevice = wdfDevice;
    deviceContext->WdfTriageInfoPtr = WdfGetTriageInfo();

    deviceContext->TlvContext.AllocationContext = 0;
    deviceContext->TlvContext.PeerVersion = WifiDeviceGetOsWdiVersion(wdfDevice);

Exit:
    TraceExit(status);
    return status;
}

void EvtDriverUnload(MAYBE_UNUSED _In_ WDFDRIVER driver)
{
    TraceEntry();
    UNREFERENCED_PARAMETER(driver);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(driver));
}
