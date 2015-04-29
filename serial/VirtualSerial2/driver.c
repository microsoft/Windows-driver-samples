/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Driver.c

Abstract:

    This module contains the implementation of the VirtualSerial Sample's
    core driver callback object.

Environment:

    Windows Driver Framework

--*/

#include <initguid.h>
#include "internal.h"

NTSTATUS
DriverEntry(
    _In_  PDRIVER_OBJECT    DriverObject,
    _In_  PUNICODE_STRING   RegistryPath
    )
{
    NTSTATUS                status;
    WDF_DRIVER_CONFIG       driverConfig;

    WDF_DRIVER_CONFIG_INIT(&driverConfig,
                            EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &driverConfig,
                            WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfDriverCreate failed 0x%x", status);
        return status;
    }

    return status;
}

NTSTATUS
EvtDeviceAdd(
    _In_  WDFDRIVER         Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS                status;
    PDEVICE_CONTEXT         deviceContext;

    status = DeviceCreate(Driver,
                            DeviceInit,
                            &deviceContext);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = DeviceConfigure(deviceContext);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

