/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, EvtDeviceAdd)
#pragma alloc_text (PAGE, EvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    NTSTATUS

--*/
{
    // Initialize WPP Tracing
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TRACE_FUNC_ENTRY(TRACE_DRIVER);

    NTSTATUS status;
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attributes;

    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
            "[PDRIVER_OBJECT: 0x%p] WdfDriverCreate failed - %!STATUS!", DriverObject, status);

        TRACE_FUNC_EXIT(TRACE_DRIVER);

        // Cleanup tracing here because EvtDriverContextCleanup will not be called
        // as we have failed to create WDFDRIVER object itself.
        //
        // Please note that if your return failure from DriverEntry after the
        // WDFDRIVER object is created successfully, you don't have to
        // call WPP cleanup because in those cases DriverContextCleanup
        // will be executed when the framework deletes the DriverObject.
        WPP_CLEANUP(DriverObject);

        return status;
    }

    TRACE_FUNC_EXIT(TRACE_DRIVER);

    return status;
}

NTSTATUS
EvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_DRIVER);

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    NTSTATUS status;

    status = EvtCreateDevice(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_DRIVER);
    return status;
}

VOID
EvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

--*/
{
    //
    // EvtCleanupCallback for WDFDRIVER is always called at PASSIVE_LEVEL
    //
    _IRQL_limited_to_(PASSIVE_LEVEL);

    TRACE_FUNC_ENTRY(TRACE_DRIVER);

    PAGED_CODE();

    TRACE_FUNC_EXIT(TRACE_DRIVER);

    // Stop WPP Tracing
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}
