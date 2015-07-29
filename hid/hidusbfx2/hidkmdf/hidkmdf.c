/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    HidKmdf.C

Abstract:

    Hid miniport to be used as an upper layer for supporting
    KMDF based driver for HID devices.

Author:


Environment:

    Kernel mode only
--*/

#include <wdm.h>

#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>

#pragma warning(default:4201)  // suppress nameless struct/union warning
#pragma warning(default:4214)  // suppress bit field types other than int warning

#define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)


//
// This type of function declaration is for Prefast for drivers. 
// Because this declaration specifies the function type, PREfast for Drivers
// does not need to infer the type or to report an inference. The declaration
// also prevents PREfast for Drivers from misinterpreting the function type 
// and applying inappropriate rules to the function. For example, PREfast for
// Drivers would not apply rules for completion routines to functions of type
// DRIVER_CANCEL. The preferred way to avoid Warning 28101 is to declare the
// function type explicitly. In the following example, the DriverEntry function
// is declared to be of type DRIVER_INITIALIZE.
//
DRIVER_INITIALIZE     DriverEntry;
DRIVER_ADD_DEVICE     HidKmdfAddDevice;
_Dispatch_type_(IRP_MJ_OTHER)
DRIVER_DISPATCH       HidKmdfPassThrough;
_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH       HidKmdfPowerPassThrough;
DRIVER_UNLOAD         HidKmdfUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, HidKmdfAddDevice )
#pragma alloc_text( PAGE, HidKmdfUnload )
#endif

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    HID_MINIDRIVER_REGISTRATION hidMinidriverRegistration;
    NTSTATUS status;
    ULONG i;

    KdPrint(("Enter DriverEntry()\n"));

    //
    // Initialize the dispatch table to pass through all the IRPs.
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = HidKmdfPassThrough;
    }

    //
    // Special case power irps so that we call PoCallDriver instead of IoCallDriver
    // when sending the IRP down the stack.
    //
    DriverObject->MajorFunction[IRP_MJ_POWER] = HidKmdfPowerPassThrough;


    DriverObject->DriverExtension->AddDevice = HidKmdfAddDevice;
    DriverObject->DriverUnload = HidKmdfUnload;

    RtlZeroMemory(&hidMinidriverRegistration,
                  sizeof(hidMinidriverRegistration));

    //
    // Revision must be set to HID_REVISION by the minidriver
    //
    hidMinidriverRegistration.Revision            = HID_REVISION;
    hidMinidriverRegistration.DriverObject        = DriverObject;
    hidMinidriverRegistration.RegistryPath        = RegistryPath;
    hidMinidriverRegistration.DeviceExtensionSize = 0;

    //
    // if "DevicesArePolled" is False then the hidclass driver does not do
    // polling and instead reuses a few Irps (ping-pong) if the device has
    // an Input item. Otherwise, it will do polling at regular interval. USB
    // HID devices do not need polling by the HID classs driver. Some leagcy
    // devices may need polling.
    //
    hidMinidriverRegistration.DevicesArePolled = FALSE;

    //
    // Register with hidclass
    //
    status = HidRegisterMinidriver(&hidMinidriverRegistration);
    if (!NT_SUCCESS(status) ){
        KdPrint(("HidRegisterMinidriver FAILED, returnCode=%x\n", status));
    }

    return status;
}


NTSTATUS
HidKmdfAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT FunctionalDeviceObject
    )
/*++

Routine Description:

    HidClass Driver calls our AddDevice routine after creating an FDO for us.
    We do not need to create a device object or attach it to the PDO.
    Hidclass driver will do it for us.

Arguments:

    DriverObject - pointer to the driver object.

    FunctionalDeviceObject -  pointer to the FDO created by the
                            Hidclass driver for us.

Return Value:

    NT status code.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);


    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
HidKmdfPassThrough(
    _In_    PDEVICE_OBJECT  DeviceObject,
    _Inout_ PIRP            Irp
    )
/*++

Routine Description:

    Pass through routine for all the IRPs except power.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    //
    // Copy current stack to next instead of skipping. We do this to preserve 
    // current stack information provided by hidclass driver to the minidriver
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}


NTSTATUS
HidKmdfPowerPassThrough(
    _In_    PDEVICE_OBJECT  DeviceObject,
    _Inout_ PIRP            Irp
    )
/*++

Routine Description:

    Pass through routine for power IRPs .

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    //
    // Must start the next power irp before skipping to the next stack location
    //
    PoStartNextPowerIrp(Irp);

    //
    // Copy current stack to next instead of skipping. We do this to preserve 
    // current stack information provided by hidclass driver to the minidriver
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return PoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}


VOID
HidKmdfUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

    return;
}

