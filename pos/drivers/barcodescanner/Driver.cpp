#include <pch.h>

#include "Device.h"

// Forward declaration
VOID EvtDriverCleanup(_In_ WDFOBJECT DriverObject);

/*
** Driver TODO:
**
** This is the main entry point of the driver.  POS APIs require that the driver sets up additional data in device add.
**
** Note that your driver may have additional configuration to do in this function, and it should not be assumed that this sample is complete.
*/
NTSTATUS DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtDriverCleanup;

    WDF_DRIVER_CONFIG_INIT(
        &config,
        EvtDriverDeviceAdd
        );

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        WDF_NO_HANDLE
        );

    return status;
}

/*
** Driver TODO:
**
** This is the cleanup callback for the driver (as set above in DriverEntry).
** PosCx requires no cleanup at this point.
*/
_Use_decl_annotations_
VOID EvtDriverCleanup(WDFOBJECT /* UnusedDriverObject */)
{
    // Do any cleanup needed here
    return;
}
