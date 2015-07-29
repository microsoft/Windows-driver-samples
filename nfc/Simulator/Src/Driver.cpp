/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Driver.cpp

Abstract:

    This module contains the implementation of the UMDF driver callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "Driver.tmh"

DECLARE_TRACING_TLS;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG Config;

    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TracingTlsInitialize();

    FunctionEntry("...");

    WDF_DRIVER_CONFIG_INIT(&Config, CDevice::OnDeviceAdd);
    Config.EvtDriverUnload = OnDriverUnload;

    Status = WdfDriverCreate(
                    DriverObject,
                    RegistryPath,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &Config,
                    WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        TraceInfo("WdfDriverCreate failed with Status %!STATUS!", Status);
        goto Exit;
    }
    
Exit:
    if (!NT_SUCCESS(Status)) {
        TracingTlsFree();
        WPP_CLEANUP(DriverObject);
    }
    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

VOID
OnDriverUnload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);

    TracingTlsFree();
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}
