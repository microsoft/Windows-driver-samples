/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.h" 
#include "driver.tmh"


DRIVER_INITIALIZE DriverEntry;

//
// WDFDRIVER object Callbacks
//
EVT_WDF_DRIVER_DEVICE_ADD OnEvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD OnEvtDriverUnload;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, OnEvtDriverDeviceAdd)
#pragma alloc_text (PAGE, OnEvtDriverUnload)
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
    into memory. 

    RegistryPath - represents the driver specific path in the Registry.

Return Value:

    STATUS_SUCCESS if successful, appropriate NTSTATUS message otherwise.

--*/
{
    NTSTATUS Status;
    WDF_DRIVER_CONFIG DriverConfig;
    WDF_OBJECT_ATTRIBUTES DriverAttributes;
    
    //
    // Enable ETW tracing
    //
    EventRegisterUfxClientSample();
    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEntry();

    //
    // Register 'EvtDriverUnload' to de-register WPP and ETW.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&DriverAttributes);

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, OnEvtDriverDeviceAdd);
    DriverConfig.DriverPoolTag = UFX_CLIENT_TAG;
    DriverConfig.EvtDriverUnload = OnEvtDriverUnload;

    Status = WdfDriverCreate(
                    DriverObject,
                    RegistryPath,
                    &DriverAttributes,
                    &DriverConfig,
                    WDF_NO_HANDLE);
    CHK_NT_MSG(Status, "Failed to create WDFDRIVER object");

End:

    if (NT_SUCCESS(Status) != TRUE) {

        //
        // Stop WPP Tracing
        //
        WPP_CLEANUP(DriverObject);

        //
        // Unregister ETW tracing
        //
        EventUnregisterUfxClientSample();
        goto Return;
    }

    TraceExit();
Return:

    return Status;
}

NTSTATUS
OnEvtDriverDeviceAdd(
    WDFDRIVER       Driver,
    PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDriverDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    TraceEntry();

    Status = UfxClientDeviceCreate(Driver, DeviceInit);
    CHK_NT_MSG(Status, "Failed to create wdf device");

End:
    TraceExit();
    return Status;
}

VOID
OnEvtDriverUnload(
    _In_ WDFDRIVER WdfDriver
    )
/*++
Routine Description:

    Free-up all the driver wide common resources like WPP and ETW.

Arguments:

    WdfDriver - Driver object being unload.

--*/
{
    PAGED_CODE ();

    TraceEntry();

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(WdfDriver));
    //
    // Unregister ETW tracing
    //
    EventUnregisterUfxClientSample();
}
