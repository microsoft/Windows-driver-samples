/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    driver.c

Abstract:

    This module implements DriverEntry for WDF driver.


Environment:

    Kernel Mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"

#if defined(EVENT_TRACING)
#include "driver.tmh"
#endif

//
//------------------------------------------------------------------- Globals
//

WDFDEVICE PepGlobalWdfDevice;

//
//------------------------------------------------------------------ Prototypes
//

DRIVER_INITIALIZE DriverEntry;

//
//--------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PepEvtDeviceAdd)

//
//------------------------------------------------------------------- Functions
//

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded.

Parameters Description:

    DriverObject - Supplies a pointer to the driver object.

    RegistryPath - Supplies a pointer to a unicode string representing the
        path to the driver-specific key in the registry.

Return Value:

    NTSTATUS.

--*/

{

    WDFDEVICE Device;
    PWDFDEVICE_INIT DeviceInit;
    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    NTSTATUS Status;
    BOOLEAN WdfDriverCreated;

    UNREFERENCED_PARAMETER(DriverObject);

    //
    // Initialize WPP tracing.
    //

    Device = NULL;
    DeviceInit = NULL;
    Driver = NULL;
    WdfDriverCreated = FALSE;
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Initialize the configuration object with necessary callbacks.
    //

    PepGlobalWdfDevice = NULL;
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, PepEvtDeviceAdd);
    DriverConfig.DriverPoolTag = PEP_POOL_TAG;
    DriverConfig.EvtDriverUnload = PepEvtDriverUnload;

    //
    // Create the WDF driver object
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             &Driver);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: WdfDriverCreate() failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto DriverEntryEnd;
    }

    WdfDriverCreated = TRUE;

    //
    // Secure the device
    //

    DeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (DeviceInit == NULL) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: WdfControlDeviceInitAllocate() failed!\n",
                    __FUNCTION__);

        Status = STATUS_UNSUCCESSFUL;
        goto DriverEntryEnd;
    }

    //
    // Set various attributes for this device.

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Initialize FDO attributes with the device extension.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);
    FdoAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Call the framework to create the device.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: WdfDeviceCreate() failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto DriverEntryEnd;
    }

    PepGlobalWdfDevice = Device;
    Status = PepInitialize(DriverObject, Driver, RegistryPath);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: PepInitialize() failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto DriverEntryEnd;
    }

    //
    // As a non-PNP device we need to explicitly tell WDF to complete
    // initialization
    //

    WdfControlFinishInitializing(Device);

    //
    // If the driver object was created, then clean up will be done in the
    // unload routine. Otherwise, things need to be cleaned up here.
    //

DriverEntryEnd:
    if ((!NT_SUCCESS(Status)) && (WdfDriverCreated == FALSE)) {
        if (Device != NULL) {
            WdfObjectDelete(Device);
            Device = NULL;

        } else if (DeviceInit != NULL) {
            WdfDeviceInitFree(DeviceInit);
            DeviceInit = NULL;
        }

        WPP_CLEANUP(DriverObject);
    }

    return Status;
}

NTSTATUS
PepEvtDeviceAdd (
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine is the AddDevice entry point for the sample Platform Extension
    driver. This can be called if there is a device corresponding to the
    Platform Extension in ACPI namespace or a root-enumerated instance was
    created during driver install.

Arguments:

    Driver - Supplies a handle to the driver object created in DriverEntry.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS code.

--*/

{

    WDFDEVICE Device;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    //
    // Set various attributes for this device.

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Initialize FDO attributes with the device extension.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);
    FdoAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Call the framework to create the device.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: WdfDeviceCreate() failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto DeviceAddEnd;
    }

    //
    // Mark the PEP device as not stoppable or removable.
    //

    WdfDeviceSetStaticStopRemove(Device, FALSE);

DeviceAddEnd:
    return Status;
}

VOID
PepEvtDriverUnload (
    __in WDFDRIVER Driver
    )

/*++

Routine Description:

    This routine is called by WDF to allow final cleanup prior to unloading
    the Resource Hub. This routine performs resource hub cleanup and stops
    tracing.

Arguments:

    Driver - Supplies a handle to a framework driver object.

Return Value:

    None.

--*/

{

    PDRIVER_OBJECT DriverObject;

    //
    // Platform extension should never be unloaded after it is registered.
    //

    if (PepRegistered != FALSE) {
#pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "KeBugCheckEx must be used for all Framework rules violations")

        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: Fatal error - Driver is unloaded after it is registered!\n",
                    __FUNCTION__);

        KeBugCheckEx(DRIVER_VIOLATION, 0, 0, 0, 0);
    }

    TraceEvents(VERBOSE,
                DBG_INIT,
                "%s: Driver unloaded!\n",
                __FUNCTION__);

    DriverObject = WdfDriverWdmGetDriverObject(Driver);
    WPP_CLEANUP(DriverObject);
    return;
}

