/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Driver.cpp

Abstract:

    Driver-global types and functions.

Environment:

    Kernel-mode.

--*/

#include "Pch.h"
#include "Driver.tmh"


INIT_CODE_SEG
NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    return UcmUcsiAcpiClient::Driver::CreateAndInitialize(DriverObject, RegistryPath);
}


namespace UcmUcsiAcpiClient
{

INIT_CODE_SEG
NTSTATUS
Driver::CreateAndInitialize (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);

    NTSTATUS status;
    WDF_DRIVER_CONFIG config;
    WDFDRIVER wdfDriver;

    WDF_DRIVER_CONFIG_INIT(&config, &Driver::EvtDriverDeviceAddThunk);
    config.EvtDriverUnload = &Driver::EvtDriverUnloadThunk;
    config.DriverPoolTag = Driver::c_PoolTag;

    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        &wdfDriver
        );

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_DRIVER, "[PDRIVER_OBJECT: 0x%p] WdfDriverCreate failed - %!STATUS!",
            DriverObject, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_DRIVER, "[WDFDRIVER: 0x%p] Driver entry completed", wdfDriver);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);

    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}


PAGED_CODE_SEG
void
Driver::EvtDriverUnloadThunk (
    WDFDRIVER WdfDriver
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);
    PAGED_CODE();

    PDRIVER_OBJECT driverObject;

    driverObject = WdfDriverWdmGetDriverObject(WdfDriver);

    TRACE_INFO(TRACE_FLAG_DRIVER, "[WDFDRIVER: 0x%p] Driver unloading", WdfDriver);

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);
    WPP_CLEANUP(driverObject);
}


PAGED_CODE_SEG
NTSTATUS
Driver::EvtDriverDeviceAddThunk (
    WDFDRIVER /* WdfDriver */,
    PWDFDEVICE_INIT DeviceInit
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);
    PAGED_CODE();

    NTSTATUS status;

    status = Fdo::CreateAndInitialize(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);
    return status;
}

}
