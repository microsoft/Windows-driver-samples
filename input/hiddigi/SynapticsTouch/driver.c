/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        driver.c

    Abstract:

        Contains driver-specific WDF functions

    Environment:

        Kernel mode

    Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "driver.h"
#include "device.h"
#include "hid.h"
#include "queue.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, OnDeviceAdd)
  #pragma alloc_text(PAGE, OnContextCleanup)
#endif

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
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

    STATUS_SUCCESS if successful, error code otherwise.

--*/
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    //
    // Initialize tracing via WPP
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Create a framework driver object
    //
    WDF_DRIVER_CONFIG_INIT(&config, OnDeviceAdd);
    config.DriverPoolTag = TOUCH_POOL_TAG;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OnContextCleanup;
   
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        WDF_NO_HANDLE
        );

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "Error creating WDF driver object - %!STATUS!",
            status);

        WPP_CLEANUP(DriverObject);

        goto exit;
    }

exit:

    return status;
}

NTSTATUS
OnDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    OnDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager when a device is found. We create and 
    initialize a WDF device object to represent the new instance of 
    an touch device. Per-device objects are also instantiated.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry
    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS indicating success or failure

--*/
{
    WDF_OBJECT_ATTRIBUTES attributes;
    PDEVICE_EXTENSION devContext;
    WDFDEVICE fxDevice;
    WDF_INTERRUPT_CONFIG interruptConfig;  
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_IO_QUEUE_CONFIG queueConfig;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();
    
    //
    // Relinquish power policy ownership because HIDCLASS acts a power
    // policy owner for ther HID stack.
    //
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, FALSE);

    //
    // This driver handles D0 Entry and Exit to power on and off the
    // controller. It also handles prepare/release hardware so that it 
    // can acquire and free SPB resources needed to communicate with the  
    // touch controller.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDeviceD0Entry = OnD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = OnD0Exit;
    pnpPowerCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);
    
    //
    // Create a framework device object. This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &fxDevice);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "WdfDeviceCreate failed - %!STATUS!",
            status);

        goto exit;
    }

    devContext = GetDeviceContext(fxDevice);
    devContext->FxDevice = fxDevice;
    devContext->InputMode = MODE_MULTI_TOUCH;
  
    //
    // Create a parallel dispatch queue to handle requests from HID Class
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig, 
        WdfIoQueueDispatchParallel);

    queueConfig.EvtIoInternalDeviceControl = OnInternalDeviceControl;
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(
        fxDevice,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->DefaultQueue);

    if (!NT_SUCCESS (status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "Error creating WDF default queue - %!STATUS!",
            status);

        goto exit;
    }

    //
    // Register a manual I/O queue for Read Requests. This queue will be used 
    // for storing HID read requests until touch data is available to 
    // complete them.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(
        fxDevice,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->PingPongQueue);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "Error creating WDF read request queue - %!STATUS!",
            status);

        goto exit;
    }

    //
    // Register one last manual I/O queue for parking HIDClass's idle power
    // requests. This queue stores idle requests until they're cancelled,
    // and could be used to complete a wait/wake request.
    //

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(
        fxDevice,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->IdleQueue
        );

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "Error creating WDF idle request queue - %!STATUS!", 
            status);

        goto exit;
    }

    //
    // Create an interrupt object for hardware notifications
    //
    WDF_INTERRUPT_CONFIG_INIT(
        &interruptConfig,
        OnInterruptIsr,
        NULL);
    interruptConfig.PassiveHandling = TRUE;

    status = WdfInterruptCreate(
        fxDevice,
        &interruptConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->InterruptObject);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_INIT,
            "Error creating WDF interrupt object - %!STATUS!",
            status);

        goto exit;
    }

exit:

    return status;
}

VOID
OnContextCleanup(
    IN WDFOBJECT Driver
    )
/*++
Routine Description:

    Free resources allocated in DriverEntry that are not automatically
    cleaned up framework.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE();
    
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));    
}
       