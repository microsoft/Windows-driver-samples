/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    driver.C

Abstract:

    This module contains routines to handle the function driver
    aspect of the bus driver. 

Environment:

    kernel mode only

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, DriverCleanup)
#pragma alloc_text (PAGE, DriverSetDeviceCallbackEvents)
#pragma alloc_text (PAGE, DriverDeviceAdd)
#pragma alloc_text (INIT, DriverEntry)
#endif

VOID 
DriverCleanup(
    _In_  WDFOBJECT _Object
    )
/*++

Routine Description:

    This callback function performs operations that must take place before the 
    driver is unloaded. Free all the resources allocated in DriverEntry.

Arguments:

    _Object - handle to a WDF Driver object.

Return Value:

    None.
    
--*/     
{
    PAGED_CODE();  

    DoTrace(LEVEL_INFO, TFLAG_PNP,("+DriverCleanup"));      

    WPP_CLEANUP( WdfDriverWdmGetDriverObject( _Object ));
}


VOID
DriverSetDeviceCallbackEvents(
    _In_  PWDFDEVICE_INIT  _DeviceInit
    )
// Initialize device callback events    
{
    WDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicyCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;    

    PAGED_CODE();  

    DoTrace(LEVEL_INFO, TFLAG_PNP,("+DriverSetDeviceCallbackEvents"));    
    
    //
    // Set event callbacks
    //   1. Pnp & Power events
    //   2. Power Policy events
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    
    //
    // Register PnP callback
    //
    PnpPowerCallbacks.EvtDevicePrepareHardware = FdoDevPrepareHardware;
    PnpPowerCallbacks.EvtDeviceReleaseHardware = FdoDevReleaseHardware;    

    //
    // Register Power callback
    //
    PnpPowerCallbacks.EvtDeviceD0Entry = FdoDevD0Entry;
    PnpPowerCallbacks.EvtDeviceD0Exit  = FdoDevD0Exit;    
    PnpPowerCallbacks.EvtDeviceSelfManagedIoInit    = FdoDevSelfManagedIoInit;
    PnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = FdoDevSelfManagedIoCleanup;     
    
    WdfDeviceInitSetPnpPowerEventCallbacks(_DeviceInit, 
                                           &PnpPowerCallbacks);  

    //
    // This driver can manage arm and disarm wake signal to support 
    // idle while S0/Sx.
    //
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&PowerPolicyCallbacks);

    //
    // Register power policy callback.  This is device specific.  The ArmWake 
    // callback function can enable/disable external event that triggers a 
    // wake signal. 
    // These functions are invoked only if Idle capability is also set; that is,
    //     IdleSettings.IdleCaps == IdleCanWakeFromS0
    //
    PowerPolicyCallbacks.EvtDeviceArmWakeFromS0    = FdoEvtDeviceArmWake;
    PowerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = FdoEvtDeviceDisarmWake;
    
    WdfDeviceInitSetPowerPolicyEventCallbacks(_DeviceInit, 
                                              &PowerPolicyCallbacks);          
}


NTSTATUS
DriverDeviceAdd(
    IN  WDFDRIVER        _Driver,
    IN  PWDFDEVICE_INIT  _DeviceInit
    )
/*++
Routine Description:

    DriverDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of toaster bus.

Arguments:

    _Driver - Handle to a framework driver object created in DriverEntry

    _DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_IO_QUEUE_CONFIG        QueueConfig;
    WDF_OBJECT_ATTRIBUTES      Attributes;
    NTSTATUS                   Status;
    WDFDEVICE                  Device;
    PFDO_EXTENSION             FdoExtension;
    WDFQUEUE                   Queue;
    PNP_BUS_INFORMATION        BusInfo;
    WDF_DEVICE_STATE           DeviceState;  
#ifdef DYNAMIC_ENUM
    WDF_CHILD_LIST_CONFIG      Config;
#endif


    PAGED_CODE();

    DoTrace(LEVEL_INFO, TFLAG_PNP, ("+DriverDeviceAdd: 0x%p", _Driver));

    //
    // Get device specific parameters, such as baudrate
    //
    DeviceQueryDeviceParameters(_Driver);

    //
    // Set PnP, Power and Power Policy event callback
    //    
    DriverSetDeviceCallbackEvents(_DeviceInit);  
   
    //
    // Initialize all the properties specific to the device.
    // Framework has default values for the one that are not
    // set explicitly here. So please read the doc and make sure
    // you are okay with the defaults.
    //
    WdfDeviceInitSetDeviceType(_DeviceInit, FILE_DEVICE_BUS_EXTENDER);

#ifdef DYNAMIC_ENUM

    //
    // WDF_ DEVICE_LIST_CONFIG describes how the framework should handle
    // dynamic child enumeration on behalf of the driver writer.
    // Since we are a bus driver, we need to specify identification description
    // for our child devices. This description will serve as the identity of our
    // child device. Since the description is opaque to the framework, we
    // have to provide bunch of callbacks to compare, copy, or free
    // any other resources associated with the description.
    //
    WDF_CHILD_LIST_CONFIG_INIT(&Config,
                                sizeof(PDO_IDENTIFICATION_DESCRIPTION),
                                FdoEvtDeviceListCreatePdo // callback to create a child device.
                                );

    // Do not register function pointers and use default option unless customization is 
    // required.  Consult MSDN or other WDK documentation for their usage.

    //
    // Tell the framework to use the built-in childlist to track the state
    // of the device based on the configuration we just created.
    //
    WdfFdoInitSetDefaultChildListConfig(_DeviceInit,
                                         &Config,
                                         WDF_NO_OBJECT_ATTRIBUTES);
#endif

    //
    // Initialize Attributes structure to specify size and accessor function
    // for storing device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, FDO_EXTENSION);

    //
    // Create a framework device object to represent FDO of this bus driver. In response 
    // to this call, framework creates a WDM deviceobject.
    // Can no longer access the WDFDEVICE_INIT structure after this call.
    //
    Status = WdfDeviceCreate(&_DeviceInit, 
                             &Attributes, 
                             &Device);
    if (!NT_SUCCESS(Status)) {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, (" WdfDeveiceCreate failed %!STATUS!", Status));
        return Status;
    }   


    //
    // Allow serial bus driver to be disabled
    //
    WDF_DEVICE_STATE_INIT(&DeviceState);        
    DeviceState.NotDisableable = WdfFalse;        
    WdfDeviceSetDeviceState(Device, &DeviceState);
    

    //
    // Get the device context.
    //
    FdoExtension = FdoGetExtension(Device);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device;
    
    //
    // Purpose of this lock is documented in FdoCreateOneChildDevice routine.
    //
    Status = WdfWaitLockCreate(&Attributes, &FdoExtension->ChildLock);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Create a power-managed IO Queue
    //
    // Configure a default queue so that requests that are not
    // configure-forwarded using WdfDeviceConfigureRequestDispatching to go to
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchParallel);
    QueueConfig.PowerManaged = WdfTrue;
    // Queue's callback event    
    QueueConfig.EvtIoDeviceControl = FdoIoQuDeviceControl;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. 
    //
    // No need to handle EvtIoStop/Resume:
    //
    //   Condition: When there is a device state change from D0 to Dx, it is processed as a 
    //   device stop event, and the caller (BthMini) will cancel all pending IOs.  
    //
    //   1. Write command/data Requests are marked cancellable by serial bus driver, and 
    //   the cancellation routine will handle the cancellation.
    // 
    //   2. Read event/data Requests have their separate queues (with manual dispatch); 
    //   when a request is in the queue, WDF owns the requests and can cancel the request 
    //   in response to a cancel reqeust (IoCancelIrp) from the caller (BthMini).
    //

    __analysis_assume(QueueConfig.EvtIoStop != 0);
    Status = WdfIoQueueCreate(Device,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);
    __analysis_assume(QueueConfig.EvtIoStop == 0);
    if (!NT_SUCCESS(Status)) {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, (" WdfIoQueueCreate failed %!STATUS!", Status));
        return Status;
    }

    //
    // Create device interface for this device. The interface will be
    // enabled by the framework when we return from StartDevice successfully.
    // Use this interface to support Bluetooth Radio on/off scenario
    //
    Status = WdfDeviceCreateDeviceInterface(Device,
                                            &GUID_DEVINTERFACE_BLUETOOTH_RADIO_ONOFF_VENDOR_SPECIFIC,
                                            NULL /* No Reference String */  );
    if (!NT_SUCCESS(Status)) {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, (" WdfDeviceCreateDeviceInterface failed %!STATUS!", Status));
        return Status;
    }

    //
    // This value is used in responding to the IRP_MN_QUERY_BUS_INFORMATION
    // for the child devices. This is an optional information provided to
    // uniquely idenitfy the bus the device is connected.
    //
    BusInfo.BusTypeGuid = GUID_SERENUM_BUS_ENUMERATOR;
    BusInfo.LegacyBusType = PNPBus;
    BusInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(Device, &BusInfo);

    //
    // Note: Do static PDO enumeration in FdoDevPrepareHardware PnP callback
    //

    DoTrace(LEVEL_INFO, TFLAG_PNP, ("-DriverDeviceAdd: exit %!STATUS!", Status));

    return Status;
}


NTSTATUS
DriverEntry(
    _In_  PDRIVER_OBJECT  _DriverObject,
    _In_  PUNICODE_STRING _RegistryPath
    )
/*++
Routine Description:

    Initialize the call backs structure of Driver Framework.

Arguments:

    _DriverObject - pointer to the driver object

    _RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

  NT Status Code

--*/
{   
    WDF_DRIVER_CONFIG   Config;
    NTSTATUS            Status;
    WDF_OBJECT_ATTRIBUTES Attributes;
    

    WDF_DRIVER_CONFIG_INIT(&Config, DriverDeviceAdd);
    Config.DriverPoolTag = POOLTAG_UARTHCIBUSSAMPLE;    
    
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);   
    Attributes.EvtCleanupCallback = DriverCleanup;  
    
    //
    // Create a framework driver object to represent our driver.
    //       
    Status = WdfDriverCreate(_DriverObject,
                             _RegistryPath,
                             &Attributes,
                             &Config,
                             WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status)) 
    {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, ("WdfDriverCreate failed %!STATUS!", Status));    
        return Status;
    }

    WPP_INIT_TRACING(_DriverObject, _RegistryPath);

    return Status;

}

