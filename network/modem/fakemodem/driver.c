/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:

    This is a simple form of function driver for fakemodem device. The driver
    doesn't handle any PnP and Power events because the framework provides
    default behaviour for those events. This driver has enough support to
    allow an user application (toast/notify.exe) to open the device
    interface registered by the driver and send read, write or ioctl requests.

Environment:

    Kernel mode

--*/

#include "fakemodem.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (INIT, DriverEntry)
    #pragma alloc_text (PAGE, FmEvtDeviceAdd)
    #pragma alloc_text (PAGE, FmCreateDosDevicesSymbolicLink)
    #pragma alloc_text (PAGE, FmDeviceCleanup)
#endif


NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT  DriverObject,
           IN PUNICODE_STRING RegistryPath
           )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as FmAddDevice and FmUnload.

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
    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;

    KdPrint(("Fakemode Function Driver Sample - Driver Framework Edition.\n"));
    KdPrint(("Built %s %s\n", __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT( &config, FmEvtDeviceAdd );

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(
                            DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,          // Driver Config Info
                            WDF_NO_HANDLE
                            );

    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;
}


NTSTATUS
FmEvtDeviceAdd(
              IN WDFDRIVER       Driver,
              IN PWDFDEVICE_INIT DeviceInit
              )
/*++
Routine Description:

    FmEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of Fm device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PFM_DEVICE_DATA             fmDeviceData;
    WDF_IO_QUEUE_CONFIG         queueConfig;
    WDF_OBJECT_ATTRIBUTES       fdoAttributes;
    WDFDEVICE                   hDevice;
    WDFQUEUE                    defQueue;

    UNREFERENCED_PARAMETER(Driver);

    KdPrint( ("FmEvtDeviceAdd routine \n"));

    PAGED_CODE();

    //
    // Modem type is serial port.
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_SERIAL_PORT);

    //
    // Use Buffered IO.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Specify the size of device extension where we track per device
    // context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FM_DEVICE_DATA);
    //
    // Register a cleanup callback on the device to free up some resources at the
    // time the device is deleted.
    //
    fdoAttributes.EvtCleanupCallback = FmDeviceCleanup;
    //
    // By opting for SynchronizationScopeDevice, we tell the framework to
    // synchronize callbacks events of all the objects directly associated
    // with the device. In this driver, we will associate queues.
    // By doing that we don't have to worrry about synchronizing
    // access to device-context by various io Events.
    // Framework will serialize them by using an internal device-lock.
    //
    fdoAttributes.SynchronizationScope = WdfSynchronizationScopeDevice;
    //
    // Create a framework device object.This call will inturn create
    // a WDM deviceobject, attach to the lower stack and set the
    // appropriate flags and attributes.
    //

    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &hDevice);

    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDeviceCreate failed with Status code 0x%x\n", status));
        return status;
    }

    //
    // Get the DeviceExtension and initialize it.
    //
    fmDeviceData = FmDeviceDataGet(hDevice);

    //
    // Tell the Framework that this device will need an interface
    //
    status = WdfDeviceCreateDeviceInterface(
                                           hDevice,
                                           (LPGUID) &GUID_DEVINTERFACE_MODEM,
                                           NULL
                                           );

    if (!NT_SUCCESS (status)) {
        KdPrint( ("WdfDeviceCreateDeviceInterface failed 0x%x\n", status));
        return status;
    }

    fmDeviceData->Flags = 0;
    status = FmCreateDosDevicesSymbolicLink(hDevice, fmDeviceData);

    if (!NT_SUCCESS(status)) {
        KdPrint( ("FmCreateDosDevicesSymbolicLink failed with Status code 0x%x\n", status));
        return status;
    }

    //
    // Initialize the context
    //
    fmDeviceData->BaudRate=1200;
    fmDeviceData->LineControl = SERIAL_7_DATA |  SERIAL_EVEN_PARITY | SERIAL_NONE_PARITY;

    //
    // Register I/O callbacks to tell the framework that you are interested
    // in handling IRP_MJ_READ, IRP_MJ_WRITE, and IRP_MJ_DEVICE_CONTROL requests.
    // In case a specific handler is not specified for one of these,
    // the request will be dispatched to the EvtIoDefault handler, if any.
    // If there is no EvtIoDefault handler, the request will be failed with
    // STATUS_INVALID_DEVICE_REQUEST.
    // WdfIoQueueDispatchParallel means that we are capable of handling
    // all the I/O request simultaneously and we are responsible for protecting
    // data that could be accessed by these callbacks simultaneously.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                           WdfIoQueueDispatchParallel);

    queueConfig.EvtIoRead = FmEvtIoRead;
    queueConfig.EvtIoWrite = FmEvtIoWrite;
    queueConfig.EvtIoDeviceControl = FmEvtIoDeviceControl;

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(
                             hDevice,
                             &queueConfig,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &defQueue // pointer to default queue
                             );
    __analysis_assume(queueConfig.EvtIoStop == 0);
	
    if (!NT_SUCCESS (status)) {

        //
        // We don't need to cleanup symbolic link here. The destroy callback for
        // the device object will do it.
        //
        return status;
    }

    //
    // Create a manual queue to hold pending read requests. By keeping
    // them in the queue, framework takes care of cancelling them if the app exits
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchManual);

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(hDevice,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &fmDeviceData->FmReadQueue
                             );
    __analysis_assume(queueConfig.EvtIoStop == 0);
	
    if (!NT_SUCCESS (status)) {
        KdPrint( ("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    //
    // Create a manual queue to hold pending ioctl wait mask requests. By keeping
    // them in the queue, framework takes care of cancelling them if the app exits
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchManual);

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(hDevice,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &fmDeviceData->FmMaskWaitQueue
                             );
    __analysis_assume(queueConfig.EvtIoStop == 0);
	
    if (!NT_SUCCESS (status)) {
        KdPrint( ("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    return status;
}


VOID
FmDeviceCleanup(
               WDFOBJECT Device
               )
/*++
Routine Description:

    This event is called when the device object is destroyed.
    Cleanup any associated data.

Arguments:

Return Value:

   VOID

--*/
{
    PFM_DEVICE_DATA   fmData;

    PAGED_CODE();

    fmData = FmDeviceDataGet((WDFDEVICE)Device);

    if (fmData->Flags & REG_VALUE_CREATED_FLAG) {
        RtlDeleteRegistryValue(
                              RTL_REGISTRY_DEVICEMAP,
                              L"SERIALCOMM",
                              fmData->PdoName.Buffer
                              );
    }
}

NTSTATUS
FmCreateDosDevicesSymbolicLink(
                              WDFDEVICE       Device,
                              PFM_DEVICE_DATA FmDeviceData
                              )
{
    NTSTATUS        status;
    UNICODE_STRING  comPort;
    UNICODE_STRING  pdoName;
    UNICODE_STRING  symbolicLink;
    WDFKEY          hKey = NULL;
    DECLARE_CONST_UNICODE_STRING(valueName, L"PortName");
    WDFSTRING       string = NULL;
    WDFMEMORY       memory;
    WDF_OBJECT_ATTRIBUTES  memoryAttributes;
    size_t          bufferLength;


    PAGED_CODE();

    symbolicLink.Buffer = NULL;

    //
    // Open the device registry and read the "PortName" value written by the
    // class installer.
    //
    status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      NULL, // PWDF_OBJECT_ATTRIBUTES
                                      &hKey);

    if (!NT_SUCCESS (status)) {
        goto Error;
    }
    status = WdfStringCreate(
                            NULL,
                            WDF_NO_OBJECT_ATTRIBUTES ,
                            &string
                            );

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Retrieve the value of ValueName from registry
    //
    status = WdfRegistryQueryString(
                                   hKey,
                                   &valueName,
                                   string
                                   );


    if (!NT_SUCCESS (status)) {
        goto Error;
    }

    //
    // Retrieve the UNICODE_STRING from string object
    //
    WdfStringGetUnicodeString(
                             string,
                             &comPort
                             );

    WdfRegistryClose(hKey);
    hKey = NULL;

    symbolicLink.Length=0;
    symbolicLink.MaximumLength = sizeof(OBJECT_DIRECTORY) + comPort.MaximumLength;

    symbolicLink.Buffer = ExAllocatePool2(POOL_FLAG_PAGED,
                                                symbolicLink.MaximumLength + sizeof(WCHAR),
                                                'wkaF');

    if (symbolicLink.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    RtlAppendUnicodeToString(&symbolicLink, OBJECT_DIRECTORY);
    RtlAppendUnicodeStringToString(&symbolicLink, &comPort);
    //
    // This DDI will get the underlying PDO name and create a symbolic to that
    // because our FDO doesn't have a name.
    //
    status = WdfDeviceCreateSymbolicLink(Device,
                                         &symbolicLink);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
    memoryAttributes.ParentObject = Device;

    status = WdfDeviceAllocAndQueryProperty(Device,
                                            DevicePropertyPhysicalDeviceObjectName,
                                            PagedPool,
                                            &memoryAttributes,
                                            &memory);
    if (!NT_SUCCESS(status)) {
        //
        // We expect a zero length buffer. Anything else is fatal.
        //
        goto Error;
    }

    pdoName.Buffer = WdfMemoryGetBuffer(memory, &bufferLength);

    if (pdoName.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;

    }
    pdoName.MaximumLength = (USHORT) bufferLength;
    pdoName.Length = (USHORT) bufferLength - sizeof(UNICODE_NULL);

    status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"SERIALCOMM",
                                   pdoName.Buffer,
                                   REG_SZ,
                                   comPort.Buffer,
                                   comPort.Length);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }
    FmDeviceData->Flags |= REG_VALUE_CREATED_FLAG;
    //
    // Store it so it can be deleted later.
    //

    FmDeviceData->PdoName = pdoName;

    Error:

    if (symbolicLink.Buffer != NULL) {
        ExFreePool(symbolicLink.Buffer);
    }

    if (hKey != NULL) {
        WdfRegistryClose(hKey);
    }
    if (string != NULL) {
        WdfObjectDelete(string);
    }

    return status;
}

