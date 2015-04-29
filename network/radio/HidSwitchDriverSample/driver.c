#include <RadioSwitchHidUsbFx2.h>
#pragma warning(disable:28252)
#pragma warning(disable:28253)
#include "driver.tmh"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, HidFx2EvtDriverContextCleanup)
    #pragma alloc_text(PAGE, HidFx2EvtDeviceAdd)
#endif



//Installable driver initialization entry point. This entry point is called directly by the I/O system.
//
NTSTATUS DriverEntry (_In_ PDRIVER_OBJECT pDriverObject, _In_ PUNICODE_STRING pszRegistryPath)
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attributes;


    // Initialize WPP Tracing
    WPP_INIT_TRACING(pDriverObject, pszRegistryPath);

    TraceInfo(DBG_INIT, "(%!FUNC!) Enter -Sample Built %s %s\n", __DATE__, __TIME__);

    WDF_DRIVER_CONFIG_INIT(&config, HidFx2EvtDeviceAdd);

    // Register a cleanup callback so that we can call WPP_CLEANUP when  the framework driver object is deleted during driver unload.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = HidFx2EvtDriverContextCleanup;

    // Create a framework driver object to represent our driver.
    status = WdfDriverCreate(pDriverObject,
                             pszRegistryPath,
                             &attributes,      // Driver Attributes
                             &config,          // Driver Config Info
                             WDF_NO_HANDLE
                            );
    if (!NT_SUCCESS(status)) 
    {
        TraceErr(DBG_INIT, "(%!FUNC!)WdfDriverCreate failed with status %!STATUS!\n", status);
        WPP_CLEANUP(pDriverObject);
    }

    TraceVerbose(DBG_INIT, "(%!FUNC!) Exit\n");
    return status;
}



//Free resources allocated in DriverEntry that are not automatically cleaned up framework.
//
void HidFx2EvtDriverContextCleanup(_In_ WDFOBJECT hDriver)
{
    PAGED_CODE ();

    TraceVerbose(DBG_INIT, "(%!FUNC!) Enter/Exit\n");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(hDriver));
}



//HidFx2EvtDeviceAdd is called by the framework in response to AddDevicecall from the PnP manager. 
//We create and initialize a WDF device object to represent a new instance of  device.
//
NTSTATUS HidFx2EvtDeviceAdd(_In_ WDFDRIVER hDriver, _Inout_ PWDFDEVICE_INIT pDeviceInit)
{
    NTSTATUS                      status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG           queueConfig;
    WDF_OBJECT_ATTRIBUTES         attributes;
    WDFDEVICE                     hDevice;
    PDEVICE_EXTENSION             pDevContext = NULL;
    WDFQUEUE                      hQueue;
    WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;
    WDF_TIMER_CONFIG              timerConfig;
    WDFTIMER                      hTimer;

    UNREFERENCED_PARAMETER(hDriver);

    PAGED_CODE();

    TraceVerbose(DBG_PNP, "(%!FUNC!) Enter\n");

    // Tell framework this is a filter driver.
    WdfFdoInitSetFilter(pDeviceInit);

    // Initialize pnp-power callbacks, attributes and a context area for the device object.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    // For usb devices, PrepareHardware callback is the to place select the interface and configure the device.
    pnpPowerCallbacks.EvtDevicePrepareHardware = HidFx2EvtDevicePrepareHardware;

    // These two callbacks start and stop the wdfusb pipe continuous reader as we go in and out of the D0-working state.
    pnpPowerCallbacks.EvtDeviceD0Entry = HidFx2EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = HidFx2EvtDeviceD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);

    // Create a framework device object.This call will in turn create a WDM device object, attach to the lower stack.
    status = WdfDeviceCreate(&pDeviceInit, &attributes, &hDevice);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfDeviceCreate failed with status code %!STATUS!\n", status);
        return status;
    }

    pDevContext = GetDeviceContext(hDevice);
    
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoInternalDeviceControl = HidFx2EvtInternalDeviceControl;

    status = WdfIoQueueCreate(hDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &hQueue);
    if (!NT_SUCCESS (status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfIoQueueCreate failed 0x%x\n", status);
        return status;
    }

    // Register a manual I/O queue for handling Interrupt Message Read Requests.
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    // This queue is used for requests that dont directly access the device.
    // The requests in this queue are serviced only when the device is in a fully powered state and sends an interrupt.
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(hDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->hInterruptMsgQueue);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfIoQueueCreate failed %!STATUS!\n", status);
        return status;
    }

    // Create a timer to handle debouncing of switchpack 
    WDF_TIMER_CONFIG_INIT(&timerConfig, HidFx2EvtTimerFunction);
    timerConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = hDevice;
    status = WdfTimerCreate(&timerConfig, &attributes, &hTimer);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfTimerCreate failed status:%!STATUS!\n", status);
        return status;
    }

    pDevContext->hDebounceTimer = hTimer;

    TraceVerbose(DBG_PNP, "(%!FUNC!) Exit\n");
    return status;
}
