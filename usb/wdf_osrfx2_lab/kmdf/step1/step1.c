/*++

Step1: This step shows how to create a simplest functional driver. 
       It only registers DriverEntry and EvtDeviceAdd callback.
       Framework provides default behaviour for everything else.
       This allows you to install and uninstall this driver.
--*/

#include "ntddk.h"
#include "wdf.h"
#include "prototypes.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;

    KdPrint(("DriverEntry of Step1\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        WDF_NO_OBJECT_ATTRIBUTES, 
                        &config,     
                        WDF_NO_HANDLE 
                        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed 0x%x\n", status));
    }

    return status;
}

NTSTATUS
EvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
{
    NTSTATUS                            status;
    WDFDEVICE                           device;
    
    UNREFERENCED_PARAMETER(Driver);

    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    return status;
}
