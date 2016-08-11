/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Driver.cpp

Abstract:

    Driver object callbacks, functions, and types.

Environment:

    Kernel-mode only.

--*/

#include "Pch.h"
#include "Driver.tmh"


EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD Driver_EvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD Driver_EvtDriverUnload;

EXTERN_C_END

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Driver_EvtDriverDeviceAdd)
#pragma alloc_text(PAGE, Driver_EvtDriverUnload)


NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDFDRIVER wdfDriver;

    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);

    WDF_DRIVER_CONFIG_INIT(&config, Driver_EvtDriverDeviceAdd);
    config.EvtDriverUnload = Driver_EvtDriverUnload;
    config.DriverPoolTag = TAG_UCSI;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             &wdfDriver
                             );

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_DRIVER, "[WdfDriver: 0x%p] WdfDriverCreate failed - %!STATUS!", DriverObject, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_DRIVER, "[WdfDriver: 0x%p] Driver entry", wdfDriver);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);

    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}


NTSTATUS
Driver_EvtDriverDeviceAdd (
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);

    status = Fdo_Create(DeviceInit);

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);

    return status;
}


VOID
Driver_EvtDriverUnload (
    _In_ WDFDRIVER Driver
    )
{
    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_DRIVER);

    TRACE_INFO(TRACE_FLAG_DRIVER, "[WdfDriver: 0x%p] Driver unloading", Driver);

    TRACE_FUNC_EXIT(TRACE_FLAG_DRIVER);

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}
