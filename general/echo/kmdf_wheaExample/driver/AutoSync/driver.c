/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This driver demonstrates use of a default I/O Queue, its
    request start events, cancellation event, and a synchronized DPC.

    To demonstrate asynchronous operation, the I/O requests are not completed
    immediately, but stored in the drivers private data structure, and a timer
    DPC will complete it next time the DPC runs.

    During the time the request is waiting for the DPC to run, it is
    made cancellable by the call WdfRequestMarkCancelable. This
    allows the test program to cancel the request and exit instantly.

    This rather complicated set of events is designed to demonstrate
    the driver frameworks synchronization of access to a device driver
    data structure, and a pointer which can be a proxy for device hardware
    registers or resources.

    This common data structure, or resource is accessed by new request
    events arriving, the DPC that completes it, and cancel processing.

    Notice the lack of specific lock/unlock operations.

    Even though this example utilizes a serial queue, a parallel queue
    would not need any additional explicit synchronization, just a
    strategy for managing multiple requests outstanding.

--*/

#include "driver.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, EchoPrintDriverVersion)
#pragma alloc_text (PAGE, EchoEvtDeviceAdd)
#endif

NTSTATUS InitWheaErrorSourceCallback
(
    PVOID Context,
    ULONG ErrorSourceId
)
{
    Context;
    GotErrorSourceId = ErrorSourceId;
    return STATUS_SUCCESS;
}

VOID UnInitWheaErrorSourceCallback
(
    PVOID Context
)
{
    Context;
    return;
}

WHEA_ERROR_SOURCE_CONFIGURATION_DEVICE_DRIVER ConfigureWheaErrorSource
(
)
{
    // I didn't make this creatorId specific to any GUID. I used the GUID creator to make a new one to satisfy
    // the WHEA_ERROR_SOURCE_CONFIGURATION_DEVICE_DRIVER object.
    GUID creatorId = { /* 12863047-1c7e-461c-a09e-a0a12f602eba */
    0x12863047,
    0x1c7e,
    0x461c,
    {0xa0, 0x9e, 0xa0, 0xa1, 0x2f, 0x60, 0x2e, 0xba}
    };

    WHEA_ERROR_SOURCE_CONFIGURATION_DEVICE_DRIVER  wheaErrorSourceConfigDeviceDriver
        = {
        // Version
        .Version = WHEA_DEVICE_DRIVER_CONFIG_V2,
        // GUID
        .SourceGuid = GUID_DEVINTERFACE_ECHO,
        // LogTag
        .LogTag = 1234u,
        // Reserved
        // Initialize
        .Initialize = &InitWheaErrorSourceCallback,
        // Uninitialize
        .Uninitialize = &UnInitWheaErrorSourceCallback,
        // MaxSectionDataLength
        .MaxSectionDataLength = 456700u,
        // MaxSectionsPerReport
        .MaxSectionsPerReport = 1u,
        // CreatorId
        .CreatorId = creatorId,
        // PartitionId
        .PartitionId = 0
    };

    return wheaErrorSourceConfigDeviceDriver;
}

NTSTATUS AddDummyWheaError
(
    PDRIVER_OBJECT  DriverObject
)
{
    NTSTATUS reportHwDeviceDriver;
    // call WHEA driver error

    GUID sectionTypeGuid = { /* 30f57004-7a78-4649-885e-7bc8c1155851 */
    0x30f57004,
    0x7a78,
    0x4649,
    {0x88, 0x5e, 0x7b, 0xc8, 0xc1, 0x15, 0x58, 0x51}
    };


    LPGUID pSectionTypeGuid = &sectionTypeGuid;

    reportHwDeviceDriver = WheaReportHwErrorDeviceDriver(GotErrorSourceId, DriverObject->DeviceObject, ((unsigned char*)"I had an accident!"), 19u, pSectionTypeGuid, WheaErrSevRecoverable, "Dummy WHEA Error");

    return reportHwDeviceDriver;
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
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

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    NTSTATUS reportHwDeviceDriver;

    // Reset the global error tracker so we can confirm that this value hasn't changed.
    GotErrorSourceId = 0u;
    NTSTATUS genericStatusContainer;

    WHEA_ERROR_SOURCE_CONFIGURATION_DEVICE_DRIVER  wheaErrorSourceConfigDeviceDriver = ConfigureWheaErrorSource();
    genericStatusContainer = WheaAddErrorSourceDeviceDriver(NULL, &wheaErrorSourceConfigDeviceDriver, 10u);

    WDF_DRIVER_CONFIG_INIT(&config,
                        EchoEvtDeviceAdd
                        );

    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,
                            WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error: WdfDriverCreate failed 0x%x\n", status));
        return status;
    }

#if DBG
    EchoPrintDriverVersion();
#endif

    if (GotErrorSourceId == 0u)
    {
        return STATUS_DRIVER_INTERNAL_ERROR;
    }
    else
    {

        reportHwDeviceDriver = AddDummyWheaError(DriverObject);
        return status;
    }
}

NTSTATUS
EchoEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
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
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint(("Enter  EchoEvtDeviceAdd\n"));

    status = EchoDeviceCreate(DeviceInit);

    return status;
}

NTSTATUS
EchoPrintDriverVersion(
    )
/*++
Routine Description:

   This routine shows how to retrieve framework version string and
   also how to find out to which version of framework library the
   client driver is bound to.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDFSTRING string;
    UNICODE_STRING us;
    WDF_DRIVER_VERSION_AVAILABLE_PARAMS ver;

    //
    // 1) Retreive version string and print that in the debugger.
    //
    status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &string);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error: WdfStringCreate failed 0x%x\n", status));
        return status;
    }

    status = WdfDriverRetrieveVersionString(WdfGetDriver(), string);
    if (!NT_SUCCESS(status)) {
        //
        // No need to worry about delete the string object because
        // by default it's parented to the driver and it will be
        // deleted when the driverobject is deleted when the DriverEntry
        // returns a failure status.
        //
        KdPrint(("Error: WdfDriverRetrieveVersionString failed 0x%x\n", status));
        return status;
    }

    WdfStringGetUnicodeString(string, &us);
    KdPrint(("Echo Sample %wZ\n", &us));

    WdfObjectDelete(string);
    string = NULL; // To avoid referencing a deleted object.

    //
    // 2) Find out to which version of framework this driver is bound to.
    //
    WDF_DRIVER_VERSION_AVAILABLE_PARAMS_INIT(&ver, 1, 0);
    if (WdfDriverIsVersionAvailable(WdfGetDriver(), &ver) == TRUE) {
        KdPrint(("Yes, framework version is 1.0\n"));
    }else {
        KdPrint(("No, framework verison is not 1.0\n"));
    }

    return STATUS_SUCCESS;
}

