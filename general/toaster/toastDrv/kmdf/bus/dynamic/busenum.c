/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    BUSENUM.C

Abstract:

    This module contains routies to handle the function driver
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
    WDFDRIVER driver;

    KdPrint(("WDF Toaster Bus Driver Sample Dynamic Version.\n"));

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
                             &driver);

    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
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
    WDF_CHILD_LIST_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES      fdoAttributes;
    NTSTATUS                   status;
    WDFDEVICE                  device;
    WDF_IO_QUEUE_CONFIG        queueConfig;
    PNP_BUS_INFORMATION        busInfo;
    //PFDO_DEVICE_DATA           deviceData;
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
    // Since this is pure software bus enumerator, we don't have to register for
    // any PNP/Power callbacks. Framework will take the default action for
    // all the PNP and Power IRPs.
    //


    //
    // WDF_ DEVICE_LIST_CONFIG describes how the framework should handle
    // dynamic child enumeration on behalf of the driver writer.
    // Since we are a bus driver, we need to specify identification description
    // for our child devices. This description will serve as the identity of our
    // child device. Since the description is opaque to the framework, we
    // have to provide bunch of callbacks to compare, copy, or free
    // any other resources associated with the description.
    //
    WDF_CHILD_LIST_CONFIG_INIT(&config,
                                sizeof(PDO_IDENTIFICATION_DESCRIPTION),
                                Bus_EvtDeviceListCreatePdo // callback to create a child device.
                                );
    //
    // This function pointer will be called when the framework needs to copy a
    // identification description from one location to another.  An implementation
    // of this function is only necessary if the description contains description
    // relative pointer values (like  LIST_ENTRY for instance) .
    // If set to NULL, the framework will use RtlCopyMemory to copy an identification .
    // description. In this sample, it's not required to provide these callbacks.
    // they are added just for illustration.
    //
    config.EvtChildListIdentificationDescriptionDuplicate =
                                Bus_EvtChildListIdentificationDescriptionDuplicate;

    //
    // This function pointer will be called when the framework needs to compare
    // two identificaiton descriptions.  If left NULL a call to RtlCompareMemory
    // will be used to compare two identificaiton descriptions.
    //
    config.EvtChildListIdentificationDescriptionCompare =
                                Bus_EvtChildListIdentificationDescriptionCompare;
    //
    // This function pointer will be called when the framework needs to free a
    // identification description.  An implementation of this function is only
    // necessary if the description contains dynamically allocated memory
    // (by the driver writer) that needs to be freed. The actual identification
    // description pointer itself will be freed by the framework.
    //
    config.EvtChildListIdentificationDescriptionCleanup =
                                Bus_EvtChildListIdentificationDescriptionCleanup;

    //
    // Tell the framework to use the built-in childlist to track the state
    // of the device based on the configuration we just created.
    //
    WdfFdoInitSetDefaultChildListConfig(DeviceInit,
                                         &config,
                                         WDF_NO_OBJECT_ATTRIBUTES);

    //
    // Initialize attributes structure to specify size and accessor function
    // for storing device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DEVICE_DATA);

    //
    // Create a framework device object. In response to this call, framework
    // creates a WDM deviceobject and attach to the PDO.
    //
    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Error creating device 0x%x\n", status));
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
    status = WdfIoQueueCreate( device,
                               &queueConfig,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &queue );
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed status 0x%x\n", status));
        return status;
    }

    //
    // Get the device context.
    //
    //deviceData = FdoGetData(device);

    //
    // Create device interface for this device. The interface will be
    // enabled by the framework when we return from StartDevice successfully.
    // Clients of this driver will open this interface and send ioctls.
    //
    status = WdfDeviceCreateDeviceInterface(
        device,
        &GUID_DEVINTERFACE_BUSENUM_TOASTER,
        NULL // No Reference String. If you provide one it will appended to the
        );   // symbolic link. Some drivers register multiple interfaces for the same device
             // and use the reference string to distinguish between them
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

    //
    // Check the registry to see if we need to enumerate child devices during
    // start.
    //
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


    UNREFERENCED_PARAMETER(OutputBufferLength);

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(Queue);

    KdPrint(("Bus_EvtIoDeviceControl: 0x%p\n", hDevice));

    switch (IoControlCode) {
    case IOCTL_BUSENUM_PLUGIN_HARDWARE:

        status = WdfRequestRetrieveInputBuffer (Request,
                                    sizeof (BUSENUM_PLUGIN_HARDWARE) +
                                    (sizeof(UNICODE_NULL) * 2), // 2 for double NULL termination (MULTI_SZ)
                                    &plugIn, &length);
        if( !NT_SUCCESS(status) ) {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        ASSERT(length == InputBufferLength);

        if (sizeof (BUSENUM_PLUGIN_HARDWARE) == plugIn->Size)
        {

            length = (InputBufferLength - sizeof (BUSENUM_PLUGIN_HARDWARE))/sizeof(WCHAR);
            //
            // Make sure the IDs is two NULL terminated.
            //
            if ((UNICODE_NULL != plugIn->HardwareIDs[length - 1]) ||
                (UNICODE_NULL != plugIn->HardwareIDs[length - 2])) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_PlugInDevice( hDevice,
                                       plugIn->HardwareIDs,
                                       length,
                                       plugIn->SerialNo );
        }

        break;

    case IOCTL_BUSENUM_UNPLUG_HARDWARE:

        status = WdfRequestRetrieveInputBuffer( Request,
                                                sizeof(BUSENUM_UNPLUG_HARDWARE),
                                                &unPlug,
                                                &length );
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
    _In_ WDFDEVICE       Device,
    _In_ PWCHAR          HardwareIds,
    _In_ size_t          CchHardwareIds,
    _In_ ULONG           SerialNo
    )

/*++

Routine Description:

    The user application has told us that a new device on the bus has arrived.

    We therefore create a description structure in stack, fill in information about
    the child device and call WdfChildListAddOrUpdateChildDescriptionAsPresent
    to add the device.

--*/

{
    PDO_IDENTIFICATION_DESCRIPTION description;
    NTSTATUS         status;

    PAGED_CODE ();

    //
    // Initialize the description with the information about the newly
    // plugged in device.
    //
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
        &description.Header,
        sizeof(description)
        );

    description.SerialNo = SerialNo;
    description.CchHardwareIds = CchHardwareIds;
    description.HardwareIds = HardwareIds;

    //
    // Call the framework to add this child to the childlist. This call
    // will internaly call our DescriptionCompare callback to check
    // whether this device is a new device or existing device. If
    // it's a new device, the framework will call DescriptionDuplicate to create
    // a copy of this description in nonpaged pool.
    // The actual creation of the child device will happen when the framework
    // receives QUERY_DEVICE_RELATION request from the PNP manager in
    // response to InvalidateDeviceRelations call made as part of adding
    // a new child.
    //
    status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
                    WdfFdoGetDefaultChildList(Device), &description.Header,
                    NULL); // AddressDescription

    if (status == STATUS_OBJECT_NAME_EXISTS) {
        //
        // The description is already present in the list, the serial number is
        // not unique, return error.
        //
        status = STATUS_INVALID_PARAMETER;
    }

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

    We therefore need to flag the PDO as no longer present.

Arguments:


Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful

--*/

{
    NTSTATUS       status;
    WDFCHILDLIST   list;

    PAGED_CODE ();

    list = WdfFdoGetDefaultChildList(Device);

    if (0 == SerialNo) {
        //
        // Unplug everybody.  We do this by starting a scan and then not reporting
        // any children upon its completion
        //
        status = STATUS_SUCCESS;

        WdfChildListBeginScan(list);
        //
        // A call to WdfChildListBeginScan indicates to the framework that the
        // driver is about to scan for dynamic children. After this call has
        // returned, all previously reported children associated with this will be
        // marked as potentially missing.  A call to either
        // WdfChildListUpdateChildDescriptionAsPresent  or
        // WdfChildListMarkAllChildDescriptionsPresent will mark all previuosly
        // reported missing children as present.  If any children currently
        // present are not reported present by calling
        // WdfChildListUpdateChildDescriptionAsPresent at the time of
        // WdfChildListEndScan, they will be reported as missing to the PnP subsystem
        // After WdfChildListEndScan call has returned, the framework will
        // invalidate the device relations for the FDO associated with the list
        // and report the changes
        //
        WdfChildListEndScan(list);

    }
    else {
        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
            &description.Header,
            sizeof(description)
            );

        description.SerialNo = SerialNo;
        //
        // WdfFdoUpdateChildDescriptionAsMissing indicates to the framework that a
        // child device that was previuosly detected is no longe present on the bus.
        // This API can be called by itself or after a call to WdfChildListBeginScan.
        // After this call has returned, the framework will invalidate the device
        // relations for the FDO associated with the list and report the changes.
        //
        status = WdfChildListUpdateChildDescriptionAsMissing(list,
                                                              &description.Header);
        if (status == STATUS_NO_SUCH_DEVICE) {
            //
            // serial number didn't exist. Remap it to a status that user
            // application can understand when it gets translated to win32
            // error code.
            //
            status = STATUS_INVALID_PARAMETER;
        }
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
    STATUS_INVALID_PARAMETER if the ejection was unsuccessful

--*/

{
    WDFDEVICE        hChild;
    NTSTATUS         status = STATUS_INVALID_PARAMETER;
    WDFCHILDLIST     list;

    PAGED_CODE ();

    list = WdfFdoGetDefaultChildList(Device);

    //
    // A zero serial number means eject all children
    //
    if (0 == SerialNo) {
        WDF_CHILD_LIST_ITERATOR     iterator;

        WDF_CHILD_LIST_ITERATOR_INIT( &iterator,
                                      WdfRetrievePresentChildren );

        WdfChildListBeginIteration(list, &iterator);

        for ( ; ; ) {
            WDF_CHILD_RETRIEVE_INFO         childInfo;
            PDO_IDENTIFICATION_DESCRIPTION  description;
            BOOLEAN                         ret;

            //
            // Init the structures.
            //
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));
            WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);

            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
                                    &description.Header,
                                    sizeof(description)
                                    );
            //
            // Get the device identification description
            //
            status = WdfChildListRetrieveNextDevice(list,
                                                &iterator,
                                                &hChild,
                                                &childInfo);

            if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES) {
                break;
            }

            ASSERT(childInfo.Status == WdfChildListRetrieveDeviceSuccess);

            //
            // Use that description to request an eject.
            //
            ret = WdfChildListRequestChildEject(list, &description.Header);
            if(!ret) {
                WDFVERIFY(ret);
            }

        }

        WdfChildListEndIteration(list, &iterator);

        if (status == STATUS_NO_MORE_ENTRIES) {
            status = STATUS_SUCCESS;
        }

    }
    else {

        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
            &description.Header,
            sizeof(description)
            );

        description.SerialNo = SerialNo;

        if (WdfChildListRequestChildEject(list, &description.Header)) {
            status = STATUS_SUCCESS;
        }
    }

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
                         BUS_HARDWARE_IDS_LENGTH / sizeof(WCHAR),
                         i );
    }

    return status;
}


