/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simemi.c

Abstract:

    This module contains the driver initialization routines.

--*/

#include <initguid.h>
#include "simemi.h"
#include <devguid.h>

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

#pragma alloc_text(INIT, DriverEntry)

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the entry point for the driver.

Arguments:

    DriverObject - Supplies a pointer to the driver object instance.

    RegistryPath - Supplies a pointer to the driver's registry path.

Return Value:

    NTSTATUS.

--*/

{

    NTSTATUS Status;
    WDF_DRIVER_CONFIG WdfConfig;

    //
    // Initialize WDF.
    //

    WDF_DRIVER_CONFIG_INIT(&WdfConfig, &SimEmiFdoCreateDevice);
    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &WdfConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDriverCreate failed with status 0x%08X\n", Status));
        goto DriverEntryEnd;
    }

    KdPrint(("Successfully created SimEmi Driver.\n"));

DriverEntryEnd:
    return Status;
}