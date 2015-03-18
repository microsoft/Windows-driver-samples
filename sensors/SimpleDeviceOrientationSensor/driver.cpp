//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of entry and exit point of sample simple device orientation sensor driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "Driver.h"

#include "Driver.tmh"

// This routine is the driver initialization entry point.
// Returns an NTSTATUS code
NTSTATUS DriverEntry(
    _In_  PDRIVER_OBJECT  DriverObject,     // Pointer to the driver object created by the I/O manager
    _In_  PUNICODE_STRING RegistryPath)     // Pointer to the driver specific registry key
{
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, NULL);

    SENSOR_FunctionEnter();

    DriverConfig.DriverPoolTag = SENSORV2_POOL_TAG_SDO;

    //
    // Initialize the driver configuration structure.
    //
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SdoDevice::OnDeviceAdd);
    DriverConfig.EvtDriverUnload = OnDriverUnload;

    //
    // Create a framework driver object to represent our driver.
    //
    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status))
    {
        TraceError("SDOS %!FUNC! WdfDriverCreate failed: %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    
    return Status;
}

// This routine is called when the driver unloads.
VOID OnDriverUnload(
    _In_ WDFDRIVER Driver) // driver object
{
    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(STATUS_SUCCESS);

    // WPP_CLEANUP doesn't actually use the Driver parameter
    // So we need to set it as unreferenced.
    UNREFERENCED_PARAMETER(Driver);
    WPP_CLEANUP(Driver);

    return;
}