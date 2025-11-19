// Copyright (c) Microsoft Corporation.  All rights reserved.
#include "precomp.h"

#include "device.h"
#include "driver.h"
#include "driver.tmh"

extern "C"
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT driverObject, 
    _In_ PUNICODE_STRING registryPath)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG driverConfig{};
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(driverObject, registryPath);
    
    // Since WPP tracing is now initialized, we can use Trace functions
    TraceEntry();

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtWifiDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&driverConfig, EvtWifiDriverDeviceAdd);
    driverConfig.DriverPoolTag = WIFI_DRIVER_DEFAULT_POOL_TAG;

    status = WdfDriverCreate(driverObject, registryPath, &attributes, &driverConfig, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(driverObject);
        return status;
    }

    TraceExit(status);

    return status;
}

NTSTATUS EvtWifiDriverDeviceAdd(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT deviceInit)
/*++
Routine Description:

    EvtWifiDriverDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(driver);

    TraceEntry();

    NTSTATUS status = STATUS_SUCCESS;

    // Configure the device init for NetAdapterCx (Data Path)
    status = NetDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        WFCError("NetDeviceInitConfig failed, status=0x%x", status);
        goto Exit;
    }

    // Configure the device init for WifiCx (Control Path)
    status = WifiDeviceInitConfig(deviceInit);
    if (!NT_SUCCESS(status))
    {
        WFCError("WifiDeviceInitConfig failed, status=0x%x", status);
        goto Exit;
    }

    // Set PnP and Power Callbacks.
    // [Scope: Only PrepareHardware and ReleaseHardware are implemented in this sample.]
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &pnpPowerCallbacks);

    // [Scope: ArmWake and DisarmWake are not implemented in this sample.]
    //WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
    //WdfDeviceInitSetPowerPolicyEventCallbacks(deviceInit, &powerPolicyCallbacks);
    
    WDFDEVICE wdfIhvDevice{};
    // Create the device object context to store the device specific information
    WDF_OBJECT_ATTRIBUTES ihvDeviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ihvDeviceAttributes, WIFI_IHV_DEVICE_CONTEXT);

    status = WdfDeviceCreate(&deviceInit, &ihvDeviceAttributes, &wdfIhvDevice);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDeviceCreate failed, status=0x%x", status);
        goto Exit;
    }
    
    //
    // Create a device interface so that applications can find and talk
    // to us.
    //
    status = WdfDeviceCreateDeviceInterface(wdfIhvDevice, &GUID_WIFICX_SAMPLE_CLIENT_INTERFACE, nullptr);
    if (!NT_SUCCESS(status))
    {
        WFCError("WdfDeviceCreateDeviceInterface failed with status=0x%x", status);
        goto Exit;
    }

    // Initialize WifiCx device now that the WDFDEVICE has been created.
    // In short the WifICx is the "wdf managed WDI", so the Wdi core
    // concepts still applies.
    WIFI_DEVICE_CONFIG wifiDeviceConfig;
    WIFI_DEVICE_CONFIG_INIT(
        &wifiDeviceConfig,
        WDI_VERSION_LATEST,         // The "WDI" version supported by this Ihv driver
        EvtWifiDeviceSendCommand,   // The Wdi command and task, now called "wifirequest".
        EvtWifiDeviceCreateAdapter, // The Wdi "ports", now support by the netadapter instances.
        EvtWifiDeviceCreateWifiDirectDevice); // [Scope: No WiFi Direct support in this sample]

    // Initialize the WifiCx device with the configuration above to let OS side ready.
    status = WifiDeviceInitialize(wdfIhvDevice, &wifiDeviceConfig);
    if (!NT_SUCCESS(status))
    {
        WFCError("WifiDeviceInitialize failed, status=0x%x", status);
        goto Exit;
    }

    // Get a pointer to the device context structure that we just associated
    // with the device object. We define this structure in the device.h
    // header file. WifiGetIhvDeviceContext is an inline function generated by
    // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
    // This function will do the type checking and return the device context.
    // If you pass a wrong object handle it will return NULL and assert if
    // run under framework verifier mode.
    auto deviceIhvContext = WifiGetIhvDeviceContext(wdfIhvDevice);
    deviceIhvContext->WdfDevice = wdfIhvDevice;
    deviceIhvContext->WdfTriageInfoPtr = WdfGetTriageInfo();

    deviceIhvContext->TlvContext.AllocationContext = 0;
    deviceIhvContext->TlvContext.PeerVersion = WifiDeviceGetOsWdiVersion(wdfIhvDevice);

Exit:
    TraceExit(status);
    return status;
}

void EvtWifiDriverContextCleanup(_In_ WDFOBJECT DriverObject)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
#ifndef _KERNEL_MODE
    // follow https://github.com/MicrosoftDocs/windows-driver-docs/blob/staging/windows-driver-docs-pr/wdf/using-wpp-software-tracing-in-kmdf-and-umdf-2-drivers.md
    // because UMDF drivers use the kernel-mode signatures of these macros for initializing and cleaning up tracing, the calls look identical for KMDF and UMDF.
    UNREFERENCED_PARAMETER(DriverObject);
#endif // !_KERNEL_MODE

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(static_cast<WDFDRIVER>(DriverObject)));
}
