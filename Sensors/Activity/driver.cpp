// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//     This module contains the implementation of entry and exit point of activity sample driver.
//
// Environment:
//     Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "Driver.h"
#include "Driver.tmh"

NTSTATUS DriverEntry(
    _In_  PDRIVER_OBJECT driverObject,  // Pointer to the driver object created by the I/O manager
    _In_  PUNICODE_STRING registryPath) // Pointer to the driver specific registry key
{
    WDF_DRIVER_CONFIG driverConfig;
    NTSTATUS status = STATUS_SUCCESS;

    // Initialize WPP Tracing
    WPP_INIT_TRACING(driverObject, NULL);

    SENSOR_FunctionEnter();

    driverConfig.DriverPoolTag = SENSOR_POOL_TAG_ACTIVITY;

    // Initialize the driver configuration structure.
    WDF_DRIVER_CONFIG_INIT(&driverConfig, ActivityDevice::OnDeviceAdd);
    driverConfig.EvtDriverUnload = OnDriverUnload;

    // Create a framework driver object to represent our driver.
    status = WdfDriverCreate(driverObject, registryPath, WDF_NO_OBJECT_ATTRIBUTES, &driverConfig, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! WdfDriverCreate failed: %!STATUS!", status);
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine is called when the driver unloads.
VOID OnDriverUnload(_In_ WDFDRIVER driver)
{
    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(STATUS_SUCCESS);

    // WPP_CLEANUP doesn't actually use the Driver parameter
    // So we need to set it as unreferenced.
    UNREFERENCED_PARAMETER(driver);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(driver));

    return;
}