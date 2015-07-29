#include <pch.h>

#include "File.h"
#include "PosEvents.h"
#include "Ioctl.h"
#include "IoRead.h"

/*
** Driver TODO: Complete the implementation of EvtDriverDeviceAdd for your specific device.
**
** WDF calls this callback when a device instance is added to the driver.  Good drivers will do a lot of
** work here to set up everything necessary, such as adding callbacks for PNP power state changes.
** This function defines an IO queue for handling DeviceIoControl and file read requests, both of which are
** important to the POS magnetic stripe reader model.
**
** Note that this is not a complete device add implementation, as the PNP power callbacks are not handled.
** Additionally, driver writers may wish to set up additional queues to serialize device property requests
** (see Ioctl.cpp for more info).
*/
NTSTATUS EvtDriverDeviceAdd(_In_ WDFDRIVER /* UnusedDriver */, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_FILEOBJECT_CONFIG fileConfig;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES fileAttributes;
    WDFDEVICE device;

    // Handle file events
    WDF_FILEOBJECT_CONFIG_INIT(
        &fileConfig,
        EvtDeviceFileCreate,
        EvtFileClose,
        WDF_NO_EVENT_CALLBACK
        );

    WDF_OBJECT_ATTRIBUTES_INIT(&fileAttributes);
    WdfDeviceInitSetFileObjectConfig(
        DeviceInit,
        &fileConfig,
        &fileAttributes
        );

    // Create Device
    WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    status = WdfDeviceCreate(
        &DeviceInit,
        &deviceAttributes,
        &device
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Create a device interface for POS Magnetic Stripe Reader so that the device can be enumerated
    status = WdfDeviceCreateDeviceInterface(
        device,
        &GUID_DEVINTERFACE_POS_MSR,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Initialize the POS library
    POS_CX_ATTRIBUTES posCxAttributes;
    POS_CX_ATTRIBUTES_INIT(&posCxAttributes);
    posCxAttributes.EvtDeviceOwnershipChange = EvtDeviceOwnershipChange;

    status = PosCxInit(device, &posCxAttributes);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Set up an IO queue to handle DeviceIoControl and ReadFile
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFQUEUE queue;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    queueConfig.EvtIoRead = EvtIoRead;

    // Call us in PASSIVE_LEVEL
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        &attributes,
        &queue
        );

    return status;
}
