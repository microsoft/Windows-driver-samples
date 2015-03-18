//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of entry and exit point
//    of sample ADXL345 accelerometer driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "Driver.h"

#include "Driver.tmh"

// This routine is the driver initialization entry point.
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,  // Pointer to the driver object created by the I/O manager.
                                        // DriverEntry must initialize members of DriverObject before it
                                        // returns to the caller. DriverObject is allocated by the system 
                                        // before the driver is loaded, and it is released by the system 
                                        // after the system unloads the function driver from memory.
    _In_ PUNICODE_STRING RegistryPath)  // Pointer to the driver specific registry key. The function
                                        // driver can use the path to store driver related data between
                                        // reboots. The path does not store hardware instance specific data.
{
    // Initialize WPP Tracing
    WPP_INIT_TRACING(DriverObject, NULL);

    SENSOR_FunctionEnter();

    WDF_DRIVER_CONFIG DriverConfig;
    DriverConfig.DriverPoolTag = SENSORV2_POOL_TAG_ACCELEROMETER;

    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, ADXL345AccDevice::OnDeviceAdd);
    DriverConfig.EvtDriverUnload = OnDriverUnload;

    NTSTATUS Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &DriverConfig, WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status))
    {
        TraceError("WdfDriverCreate failed %!STATUS!", Status);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is called when the driver unloads.
VOID OnDriverUnload(
    _In_ WDFDRIVER Driver)      // Driver object
{
    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(STATUS_SUCCESS);

    // WPP_CLEANUP doesn't actually use the Driver parameter so we need to set it as unreferenced.
    UNREFERENCED_PARAMETER(Driver);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));

    return;
}

