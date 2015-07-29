/*++

Copyright (c) 2003  Microsoft Corporation All Rights Reserved

Module Name:

    BUSENUM.C

Abstract:

    This module contains routines to handle the function driver
    aspect of the bus driver.

Environment:

    kernel mode only

--*/

#include "busenum.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Bus_EvtDeviceAdd)
#pragma alloc_text (PAGE, Bus_EvtIoDeviceControl)
#pragma alloc_text (PAGE, Bus_PlugInDevice)
#pragma alloc_text (PAGE, Bus_UnPlugDevice)
#pragma alloc_text (PAGE, Bus_EjectDevice)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    Initialize the call backs structure of Driver Framework.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

  NT Status Code

--*/
{
    WDF_DRIVER_CONFIG   config;
    NTSTATUS            status;

    KdPrint(("Toaster Static Bus Driver Sample - Driver Framework Edition.\n"));

    //
    // Initiialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by specifing one in the Config structure.
    //

    WDF_DRIVER_CONFIG_INIT(
        &config,
        Bus_EvtDeviceAdd
        );

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;

}


NTSTATUS
Bus_EvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
/*++
Routine Description:

    Bus_EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of toaster bus.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_IO_QUEUE_CONFIG        queueConfig;
    WDF_OBJECT_ATTRIBUTES      attributes;
    NTSTATUS                   status;
    WDFDEVICE                  device;
    PFDO_DEVICE_DATA           deviceData;
    PNP_BUS_INFORMATION        busInfo;
    WDFQUEUE                   queue;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE ();

    KdPrint(("Bus_EvtDeviceAdd: 0x%p\n", Driver));

    //
    // Initialize all the properties specific to the device.
    // Framework has default values for the one that are not
    // set explicitly here. So please read the doc and make sure
    // you are okay with the defaults.
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    //
    // Initialize attributes structure to specify size and accessor function
    // for storing device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FDO_DEVICE_DATA);

    //
    // Create a framework device object. In response to this call, framework
    // creates a WDM deviceobject.
    //
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the device context.
    //
    deviceData = FdoGetData(device);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;
    //
    // Purpose of this lock is documented in Bus_PlugInDevice routine below.
    //
    status = WdfWaitLockCreate(&attributes, &deviceData->ChildLock);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
    );

    queueConfig.EvtIoDeviceControl = Bus_EvtIoDeviceControl;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests or 
    // forward them to other drivers. This driver completes the requests 
    // directly in the queue's handlers. If the EvtIoStop callback is not 
    // implemented, the framework waits for all driver-owned requests to be
    // done before moving in the Dx/sleep states or before removing the 
    // device, which is the correct behavior for this type of driver.
    // If the requests were taking an indeterminate amount of time to complete,
    // or if the driver forwarded the requests to a lower driver/another stack,
    // the queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                            &queueConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &queue
                            );
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed with status 0x%x\n", status));
        return status;
    }

    //
    // Create device interface for this device. The interface will be
    // enabled by the framework when we return from StartDevice successfully.
    //
    status = WdfDeviceCreateDeviceInterface(
                                            device,
                                            &GUID_DEVINTERFACE_BUSENUM_TOASTER,
                                            NULL // No Reference String
                                            );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // This value is used in responding to the IRP_MN_QUERY_BUS_INFORMATION
    // for the child devices. This is an optional information provided to
    // uniquely idenitfy the bus the device is connected.
    //
    busInfo.BusTypeGuid = GUID_DEVCLASS_TOASTER;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(device, &busInfo);

    status = Bus_WmiRegistration(device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = Bus_DoStaticEnumeration(device);

    return status;
}

VOID
Bus_EvtIoDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
    )

/*++
Routine Description:

  Handle user mode PlugIn, UnPlug and device Eject requests.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the IRP_MJ_DEVICE_CONTROL IRP received by the framework.

    OutputBufferLength - Length, in bytes, of the request's output buffer,
                        if an output buffer is available.

    InputBufferLength - Length, in bytes, of the request's input buffer,
                        if an input buffer is available.
    IoControlCode - Driver-defined or system-defined I/O control code (IOCTL)
                    that is associated with the request.

Return Value:

   VOID

--*/
{
    NTSTATUS                 status = STATUS_INVALID_PARAMETER;
    WDFDEVICE                hDevice;
    size_t                   length = 0;
    PBUSENUM_PLUGIN_HARDWARE plugIn = NULL;
    PBUSENUM_UNPLUG_HARDWARE unPlug = NULL;
    PBUSENUM_EJECT_HARDWARE  eject  = NULL;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(OutputBufferLength);

    hDevice = WdfIoQueueGetDevice(Queue);

    KdPrint(("Bus_EvtIoDeviceControl: 0x%p\n", hDevice));

    switch (IoControlCode) {
    case IOCTL_BUSENUM_PLUGIN_HARDWARE:

        status = WdfRequestRetrieveInputBuffer (Request,
                                    sizeof (BUSENUM_PLUGIN_HARDWARE) +
                                        (sizeof(UNICODE_NULL) * 2), // 2 for double NULL termination (MULTI_SZ)
                                    &plugIn,
                                    &length);
        if( !NT_SUCCESS(status) ) {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if (sizeof (BUSENUM_PLUGIN_HARDWARE) == plugIn->Size)
         {

            length = (InputBufferLength - sizeof (BUSENUM_PLUGIN_HARDWARE))/sizeof(WCHAR);
            //
            // Make sure the IDs is double NULL terminated. TODO:
            //
            if ((UNICODE_NULL != plugIn->HardwareIDs[length - 1]) ||
                (UNICODE_NULL != plugIn->HardwareIDs[length - 2])) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_PlugInDevice(hDevice,
                                      plugIn->HardwareIDs,
                                      plugIn->SerialNo);

        }
        break;

    case IOCTL_BUSENUM_UNPLUG_HARDWARE:

        status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof (BUSENUM_UNPLUG_HARDWARE),
                                                    &unPlug, &length);
        if( !NT_SUCCESS(status) ) {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if (unPlug->Size == InputBufferLength)
        {

            status= Bus_UnPlugDevice(hDevice, unPlug->SerialNo );

        }

        break;

    case IOCTL_BUSENUM_EJECT_HARDWARE:

        status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof (BUSENUM_EJECT_HARDWARE),
                                                    &eject, &length);
        if( !NT_SUCCESS(status) ) {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if (eject->Size == InputBufferLength)
        {
            status= Bus_EjectDevice(hDevice, eject->SerialNo);

        }
        break;

    default:
        break; // default status is STATUS_INVALID_PARAMETER
    }

    WdfRequestCompleteWithInformation(Request, status, length);
}

NTSTATUS
Bus_PlugInDevice(
    _In_ WDFDEVICE  Device,
    _In_ PWSTR      HardwareIds,
    _In_ ULONG      SerialNo
    )

/*++

Routine Description:

    The user application has told us that a new device on the bus has arrived.

    We therefore need to create a new PDO, initialize it, add it to the list
    of PDOs for this FDO bus, and then tell Plug and Play that all of this
    happened so that it will start sending prodding IRPs.

--*/

{
    NTSTATUS         status = STATUS_SUCCESS;
    BOOLEAN          unique = TRUE;
    WDFDEVICE        hChild;
    PPDO_DEVICE_DATA pdoData;
    PFDO_DEVICE_DATA deviceData;

    PAGED_CODE ();

    //
    // First make sure that we don't already have another device with the
    // same serial number.
    // Framework creates a collection of all the child devices we have
    // created so far. So acquire the handle to the collection and lock
    // it before walking the item.
    //
    deviceData = FdoGetData(Device);
    hChild = NULL;

    //
    // We need an additional lock to synchronize addition because
    // WdfFdoLockStaticChildListForIteration locks against anyone immediately
    // updating the static child list (the changes are put on a queue until the
    // list has been unlocked).  This type of lock does not enforce our concept
    // of unique IDs on the bus (ie SerialNo).
    //
    // Without our additional lock, 2 threads could execute this function, both
    // find that the requested SerialNo is not in the list and attempt to add
    // it.  If that were to occur, 2 PDOs would have the same unique SerialNo,
    // which is incorrect.
    //
    // We must use a passive level lock because you can only call WdfDeviceCreate
    // at PASSIVE_LEVEL.
    //
    WdfWaitLockAcquire(deviceData->ChildLock, NULL);
    WdfFdoLockStaticChildListForIteration(Device);

    while ((hChild = WdfFdoRetrieveNextStaticChild(Device,
                            hChild, WdfRetrieveAddedChildren)) != NULL) {
        //
        // WdfFdoRetrieveNextStaticChild returns reported and to be reported
        // children (ie children who have been added but not yet reported to PNP).
        //
        // A surprise removed child will not be returned in this list.
        //
        pdoData = PdoGetData(hChild);

        //
        // It's okay to plug in another device with the same serial number
        // as long as the previous one is in a surprise-removed state. The
        // previous one would be in that state after the device has been
        // physically removed, if somebody has an handle open to it.
        //
        if (SerialNo == pdoData->SerialNo) {
            unique = FALSE;
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (unique) {
        //
        // Create a new child device.  It is OK to create and add a child while
        // the list locked for enumeration.  The enumeration lock applies only
        // to enumeration, not addition or removal.
        //
        status = Bus_CreatePdo(Device, HardwareIds, SerialNo);
    }

    WdfFdoUnlockStaticChildListFromIteration(Device);
    WdfWaitLockRelease(deviceData->ChildLock);

    return status;
}

NTSTATUS
Bus_UnPlugDevice(
    WDFDEVICE   Device,
    ULONG       SerialNo
    )
/*++

Routine Description:

    The application has told us a device has departed from the bus.

    We therefore need to flag the PDO as no longer present
    and then tell Plug and Play about it.

Arguments:


Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful

--*/

{
    PPDO_DEVICE_DATA pdoData;
    BOOLEAN          found = FALSE;
    BOOLEAN          plugOutAll;
    WDFDEVICE        hChild;
    NTSTATUS         status = STATUS_INVALID_PARAMETER;

    PAGED_CODE ();

    plugOutAll = (0 == SerialNo) ? TRUE : FALSE;

    hChild = NULL;

    WdfFdoLockStaticChildListForIteration(Device);

    while ((hChild = WdfFdoRetrieveNextStaticChild(Device,
                            hChild, WdfRetrieveAddedChildren)) != NULL) {

        if (plugOutAll) {

            status = WdfPdoMarkMissing(hChild);
            if(!NT_SUCCESS(status)) {
                KdPrint(("WdfPdoMarkMissing failed 0x%x\n", status));
                break;
            }

            found = TRUE;
        }
        else {
            pdoData = PdoGetData(hChild);

            if (SerialNo == pdoData->SerialNo) {

                status = WdfPdoMarkMissing(hChild);
                if(!NT_SUCCESS(status)) {
                    KdPrint(("WdfPdoMarkMissing failed 0x%x\n", status));
                    break;
                }

                found = TRUE;
                break;
            }
        }
    }

    WdfFdoUnlockStaticChildListFromIteration(Device);

    if (found) {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
Bus_EjectDevice(
    WDFDEVICE   Device,
    ULONG       SerialNo
    )

/*++

Routine Description:

    The user application has told us to eject the device from the bus.
    In a real situation the driver gets notified by an interrupt when the
    user presses the Eject button on the device.

Arguments:


Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful

--*/

{
    PPDO_DEVICE_DATA pdoData;
    BOOLEAN          ejectAll;
    WDFDEVICE        hChild;
    NTSTATUS         status = STATUS_INVALID_PARAMETER;

    PAGED_CODE ();

    //
    // Scan the list to find matching PDOs
    //
    ejectAll = (0 == SerialNo) ? TRUE : FALSE;
    hChild = NULL;

    WdfFdoLockStaticChildListForIteration(Device);

    while ((hChild = WdfFdoRetrieveNextStaticChild(Device,
                                       hChild,
                                       WdfRetrieveAddedChildren)) != NULL) {

        pdoData = PdoGetData(hChild);

        if (ejectAll || SerialNo == pdoData->SerialNo) {

            status = STATUS_SUCCESS;
            WdfPdoRequestEject(hChild);
            if (!ejectAll) {
                break;
            }
        }
    }

    WdfFdoUnlockStaticChildListFromIteration(Device);

    return status;
}

NTSTATUS
Bus_DoStaticEnumeration(
    IN WDFDEVICE Device
    )
/*++
Routine Description:

    The routine enables you to statically enumerate child devices
    during start instead of running the enum.exe/notify.exe to
    enumerate toaster devices.

    In order to statically enumerate, user must specify the number
    of toasters in the Toaster Bus driver's device registry. The
    default value is zero.

    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\Root\SYSTEM\0002\
                    Device Parameters
                        NumberOfToasters:REG_DWORD:2

    You can also configure this value in the Toaster Bus Inf file.

--*/

{
    WDFKEY      hKey = NULL;
    NTSTATUS    status;
    ULONG       value, i;
    DECLARE_CONST_UNICODE_STRING(valueName, L"NumberOfToasters");

    //
    // If the registry value doesn't exist, we will use the
    // hardcoded default number.
    //
    value = DEF_STATICALLY_ENUMERATED_TOASTERS;

    //
    // Open the device registry and read the "NumberOfToasters" value.
    //
    status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      NULL, // PWDF_OBJECT_ATTRIBUTES
                                      &hKey);

    if (NT_SUCCESS (status)) {

        status = WdfRegistryQueryULong(hKey,
                                  &valueName,
                                  &value);

        WdfRegistryClose(hKey);
        hKey = NULL; // Set hKey to NULL to catch any accidental subsequent use.

        if (NT_SUCCESS (status)) {
            //
            // Make sure it doesn't exceed the max. This is required to prevent
            // denial of service by enumerating large number of child devices.
            //
            value = min(value, MAX_STATICALLY_ENUMERATED_TOASTERS);
        }else {
            return STATUS_SUCCESS; // This is an optional property.
        }
    }

    KdPrint(("Enumerating %d toaster devices\n", value));

    for(i=1; i<= value; i++) {
        //
        // Value of i is used as serial number.
        //
        status = Bus_PlugInDevice(Device,
                         BUS_HARDWARE_IDS,
                         i );
    }

    return status;
}



