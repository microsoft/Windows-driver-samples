// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//
//  This module contains the implementation of entry and exit point of combo driver.
//
// Environment:
//
//  Windows User-Mode Driver Framework (WUDF)

#include "Clients.h"
#include "Driver.h"

#include "Driver.tmh"

//------------------------------------------------------------------------------
// Function: DriverEntry
//
// This routine is the driver initialization entry point.
//
// Arguments:
//      DriverObject: IN: Pointer to the driver object created by the I/O manager
//      RegistryPath: IN: Pointer to the driver specific registry key
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
DriverEntry(
    _In_  PDRIVER_OBJECT  DriverObject,
    _In_  PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, NULL);

    SENSOR_FunctionEnter();

    DriverConfig.DriverPoolTag = SENSORV2_POOL_TAG_COMBO;

    //
    // Initialize the driver configuration structure.
    //
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, OnDeviceAdd);
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
        TraceError("COMBO %!FUNC! WdfDriverCreate failed: %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}



//------------------------------------------------------------------------------
// Function: OnDriverUnload
//
// This routine is called when the driver unloads.
//
// Arguments:
//      Driver: IN: driver object
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
VOID
OnDriverUnload(
    _In_ WDFDRIVER Driver
    )
{
    SENSOR_FunctionEnter();

    UNREFERENCED_PARAMETER(Driver);

    SENSOR_FunctionExit(STATUS_SUCCESS);

    WPP_CLEANUP(Driver);

    return;
}