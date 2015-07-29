/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Toaster.c

Abstract:

    This is a featured version of the toaster function driver. This version
    shows how to register for PNP and Power events, handle create & close
    file requests.

Environment:

    Kernel mode

--*/

#include "toaster.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, ToasterEvtDeviceAdd)
#pragma alloc_text (PAGE, ToasterEvtDeviceFileCreate)
#pragma alloc_text (PAGE, ToasterEvtFileClose)
#pragma alloc_text (PAGE, ToasterEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, ToasterEvtDeviceReleaseHardware)
#pragma alloc_text (PAGE, ToasterEvtDeviceContextCleanup)
#pragma alloc_text (PAGE, ToasterEvtIoDeviceControl)
#pragma alloc_text (PAGE, ToasterEvtIoRead)
#pragma alloc_text (PAGE, ToasterEvtIoWrite)
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
    points in the function driver, such as ToasterAddDevice and ToasterUnload.

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

    KdPrint(("WDF Toaster Function Driver Sample - Featured version\n"));

    //
    // Initiialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG_INIT(
                            &config,
                            ToasterEvtDeviceAdd
                            );

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(
                            DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
                            &config,          // Driver Config Info
                            WDF_NO_HANDLE
                            );

    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;
}


NTSTATUS
ToasterEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    ToasterEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of toaster device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                              status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS          pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES                 fdoAttributes;
    WDFDEVICE                             device;
    WDF_FILEOBJECT_CONFIG                 fileConfig;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    WDF_POWER_POLICY_EVENT_CALLBACKS      powerPolicyCallbacks;
    WDF_IO_QUEUE_CONFIG                   queueConfig;
    //PFDO_DATA                             fdoData;
    WDFQUEUE                              queue;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint(("ToasterEvtDeviceAdd called\n"));

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Register PNP callbacks.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = ToasterEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = ToasterEvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = ToasterEvtDeviceSelfManagedIoInit;

    //
    // Register Power callbacks.
    //
    pnpPowerCallbacks.EvtDeviceD0Entry = ToasterEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = ToasterEvtDeviceD0Exit;


    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Register power policy event callbacks so that we would know when to
    // arm/disarm the hardware to handle wait-wake and when the wake event
    // is triggered by the hardware.
    //
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);

    //
    // This group of three callbacks allows this sample driver to manage
    // arming the device for wake from the S0 or Sx state.  We don't really
    // differentiate between S0 and Sx state..
    //
    powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = ToasterEvtDeviceArmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = ToasterEvtDeviceDisarmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceWakeFromS0Triggered = ToasterEvtDeviceWakeFromS0Triggered;
    powerPolicyCallbacks.EvtDeviceArmWakeFromSx = ToasterEvtDeviceArmWakeFromSx;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromSx = ToasterEvtDeviceDisarmWakeFromSx;
    powerPolicyCallbacks.EvtDeviceWakeFromSxTriggered = ToasterEvtDeviceWakeFromSxTriggered;

    //
    // Register the power policy callbacks.
    //
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handling Create, Close and
    // Cleanup requests that gets genereate when an application or another
    // kernel component opens an handle to the device. If you don't register,
    // the framework default behaviour would be complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    WDF_FILEOBJECT_CONFIG_INIT(
                            &fileConfig,
                            ToasterEvtDeviceFileCreate,
                            ToasterEvtFileClose,
                            WDF_NO_EVENT_CALLBACK // not interested in Cleanup
                            );

    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                       &fileConfig,
                                       WDF_NO_OBJECT_ATTRIBUTES);

    //
    // Now specify the size of device extension where we track per device
    // context. Along with setting the context type as shown below, you should also
    // specify the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME in header to specify the
    // accessor function name.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DATA);

    //
    // Set a context cleanup routine to cleanup any resources that are not
    // parent to this device. This cleanup will be called in the context of
    // pnp remove-device when the framework deletes the device object.
    //
    fdoAttributes.EvtCleanupCallback = ToasterEvtDeviceContextCleanup;

    //
    // DeviceInit is completely initialized. So call the framework to create the
    // device and attach it to the lower stack.
    //
    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDeviceCreate failed with Status code 0x%x\n", status));
        return status;
    }

    //
    // Get the device context by using accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for FDO_DATA.
    //
    //fdoData = ToasterFdoGetData(device);

    //
    // Tell the Framework that this device will need an interface so that
    // application can find our device and talk to it.
    //
    status = WdfDeviceCreateDeviceInterface(
                 device,
                 (LPGUID) &GUID_DEVINTERFACE_TOASTER,
                 NULL
             );

    if (!NT_SUCCESS (status)) {
        KdPrint( ("WdfDeviceCreateDeviceInterface failed 0x%x\n", status));
        return status;
    }

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
    // A default queue gets all the requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                             WdfIoQueueDispatchParallel); // EvtIoCancel

    queueConfig.EvtIoRead = ToasterEvtIoRead;
    queueConfig.EvtIoWrite = ToasterEvtIoWrite;
    queueConfig.EvtIoDeviceControl = ToasterEvtIoDeviceControl;

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

    if (!NT_SUCCESS (status)) {

        KdPrint( ("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    //
    // Set the idle power policy to put the device to Dx if the device is not used
    // for the specified IdleTimeout time. Since this is a virtual device we
    // tell the framework that we cannot wake ourself if we sleep in S0. Only
    // way the device can be brought to D0 is if the device recieves an I/O from
    // the system.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 60000; // 60 secs idle timeout
    status = WdfDeviceAssignS0IdleSettings(device, &idleSettings);
    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDeviceAssignS0IdleSettings failed 0x%x\n", status));
        return status;
    }

    //
    // Set the wait-wake policy.
    //

    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
    status = WdfDeviceAssignSxWakeSettings(device, &wakeSettings);
    if (!NT_SUCCESS(status)) {
        //
        // We are probably enumerated on a bus that doesn't support Sx-wake.
        // Let us not fail the device add just because we aren't able to support
        // wait-wake. I will let the user of this sample decide how important it's
        // to support wait-wake for their hardware and return appropriate status.
        //
        KdPrint( ("WdfDeviceAssignSxWakeSettings failed 0x%x\n", status));
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
ToasterEvtDevicePrepareHardware(
    WDFDEVICE      Device,
    WDFCMRESLIST   ResourcesRaw,
    WDFCMRESLIST   ResourcesTranslated
    )
/*++

Routine Description:

    EvtDevicePrepareHardware event callback performs operations that are
    necessary to make the driver's device operational. The framework calls the
    driver's EvtDevicePrepareHardware callback when the PnP manager sends an
    IRP_MN_START_DEVICE request to the driver stack.

    Specifically, most drivers will use this callback to map resources.  USB
    drivers may use it to get device descriptors, config descriptors and to
    select configs.

    Some drivers may choose to download firmware to a device in this callback,
    but that is usually only a good choice if the device firmware won't be
    destroyed by a D0 to D3 transition.  If firmware will be gone after D3,
    then firmware downloads should be done in EvtDeviceD0Entry, not here.

Arguments:

    Device - Handle to a framework device object.

    ResourcesRaw - Handle to a collection of framework resource objects.
                This collection identifies the raw (bus-relative) hardware
                resources that have been assigned to the device.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    WDF status code

--*/
{
    //PFDO_DATA   fdoData;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

    //fdoData = ToasterFdoGetData(Device);

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);

    KdPrint(("ToasterEvtDevicePrepareHardware called\n"));

    PAGED_CODE();
    //
    // Get the number item that are currently in Resources collection and
    // iterate thru as many times to get more information about the each items
    //
    for (i=0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch(descriptor->Type) {

        case CmResourceTypePort:

            KdPrint(("I/O Port: (%x) Length: (%d)\n",
                    descriptor->u.Port.Start.LowPart,
                    descriptor->u.Port.Length));
            break;

        case CmResourceTypeMemory:

            KdPrint(("Memory: (%x) Length: (%d)\n",
                    descriptor->u.Memory.Start.LowPart,
                    descriptor->u.Memory.Length));
            break;
        case CmResourceTypeInterrupt:

            KdPrint(("Interrupt level: 0x%0x, Vector: 0x%0x, Affinity: 0x%0Ix\n",
                descriptor->u.Interrupt.Level,
                descriptor->u.Interrupt.Vector,
                descriptor->u.Interrupt.Affinity));
            break;

        default:
            break;
        }

    }

    return status;

}

NTSTATUS
ToasterEvtDeviceReleaseHardware(
    IN  WDFDEVICE    Device,
    IN  WDFCMRESLIST ResourcesTranslated
    )
/*++

Routine Description:

    EvtDeviceReleaseHardware is called by the framework whenever the PnP manager
    is revoking ownership of our resources.  This may be in response to either
    IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE.  The callback is made before
    passing down the IRP to the lower driver.

    In this callback, do anything necessary to free those resources.

Arguments:

    Device - Handle to a framework device object.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    NTSTATUS - Failures will be logged, but not acted on.

--*/
{
    //PFDO_DATA   fdoData;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    KdPrint(("ToasterEvtDeviceReleaseHardware called\n"));

    PAGED_CODE();

    //fdoData = ToasterFdoGetData(Device);
    //
    // Unmap any I/O ports, registers that you mapped in PrepareHardware.
    // Disconnecting from the interrupt will be done automatically by the framework.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ToasterEvtDeviceSelfManagedIoInit(
    IN  WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceSelfManagedIoInit is called it once for each device,
    after the framework has called the driver's EvtDeviceD0Entry
    callback function for the first time. The framework does not
    call the EvtDeviceSelfManagedIoInit callback function again for
    that device, unless the device is removed and reconnected, or
    the drivers are reloaded.

    The EvtDeviceSelfManagedIoInit callback function must initialize
    the self-managed I/O operations that the driver will handle
    for the device.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    NTSTATUS - Failures will result in the device stack being torn down.

--*/
{
    UNREFERENCED_PARAMETER(Device);
    
    KdPrint(("ToasterEvtDeviceSelfManagedIoInit called\n"));

    return STATUS_SUCCESS;
}


VOID
ToasterEvtDeviceContextCleanup(
    IN WDFOBJECT Device
    )
/*++

Routine Description:

   EvtDeviceContextCleanup event callback must perform any operations that are
   necessary before the specified device is removed. The framework calls
   the driver's EvtDeviceContextCleanup callback when the device is deleted in response
   to IRP_MN_REMOVE_DEVICE request.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    None

--*/
{
    //PFDO_DATA   fdoData;
    UNREFERENCED_PARAMETER(Device);
    KdPrint( ("ToasterEvtDeviceContextCleanup called\n"));

    PAGED_CODE();

    //fdoData = ToasterFdoGetData((WDFDEVICE)Device);

    return;
}

VOID
ToasterEvtDeviceFileCreate (
    IN WDFDEVICE Device,
    IN WDFREQUEST Request,
    IN WDFFILEOBJECT FileObject
    )
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing to a device.
    This callback is called in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters for create

Return Value:

    None

--*/
{
    //PFDO_DATA   fdoData;

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Device);

    KdPrint( ("ToasterEvtDeviceFileCreate %p\n", Device));

    PAGED_CODE ();

    //
    // Get the device context given the device handle.
    //
    //fdoData = ToasterFdoGetData(Device);

    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}


VOID
ToasterEvtFileClose (
    IN WDFFILEOBJECT    FileObject
    )

/*++

Routine Description:

   EvtFileClose is called when all the handles represented by the FileObject
   is closed and all the references to FileObject is removed. This callback
   may get called in an arbitrary thread context instead of the thread that
   called CloseHandle. If you want to delete any per FileObject context that
   must be done in the context of the user thread that made the Create call,
   you should do that in the EvtDeviceCleanp callback.

Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

    None
    
--*/
{
    //PFDO_DATA    fdoData;
    UNREFERENCED_PARAMETER(FileObject);
    PAGED_CODE ();

    //fdoData = ToasterFdoGetData(WdfFileObjectGetDevice(FileObject));

    KdPrint( ("ToasterEvtFileClose\n"));

    return;
}



VOID
ToasterEvtIoRead (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t        Length
    )
/*++

Routine Description:

    Performs read to the toaster device. This event is called when the
    framework receives IRP_MJ_READ requests.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Lenght - Length of the data buffer associated with the request.
             The default property of the queue is to not dispatch
             zero lenght read & write requests to the driver and
             complete is with status success. So we will never get
             a zero length request.

Return Value:

    None

--*/
{
    NTSTATUS    status;
    ULONG_PTR bytesCopied =0;
    WDFMEMORY memory;

    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Queue);

    PAGED_CODE();

    KdPrint(("ToasterEvtIoRead: Request: 0x%p, Queue: 0x%p\n",
             Request, Queue));

    //
    // Get the request memory and perform read operation here
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if(NT_SUCCESS(status) ) {
        //
        // Copy data into the memory buffer using WdfMemoryCopyFromBuffer
        //
    }

    WdfRequestCompleteWithInformation(Request, status, bytesCopied);

}

VOID
ToasterEvtIoWrite (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t        Length
    )
/*++

Routine Description:

    Performs write to the toaster device. This event is called when the
    framework receives IRP_MJ_WRITE requests.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

    None
   
--*/

{
    NTSTATUS    status;
    WDFMEMORY memory;

    UNREFERENCED_PARAMETER(Queue);

    KdPrint(("ToasterEvtIoWrite. Request: 0x%p, Queue: 0x%p\n",
                                Request, Queue));
    PAGED_CODE();

    //
    // Get the request buffer and perform write operation here
    //
    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if(NT_SUCCESS(status) ) {
        //
        // 1) Use WdfMemoryCopyToBuffer to copy data from the request
        // to driver buffer.
        // 2) Or get the buffer pointer from the request by calling
        // WdfRequestRetrieveInputBuffer to  transfer data to the hw
        // 3) Or you can get the buffer pointer from the memory handle
        // by calling WdfMemoryGetBuffer to  transfer data to the hw.
        //
    }

    WdfRequestCompleteWithInformation(Request, status, Length);

}


VOID
ToasterEvtIoDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

    None

--*/
{
    NTSTATUS             status= STATUS_SUCCESS;
    WDF_DEVICE_STATE     deviceState;
    WDFDEVICE            hDevice = WdfIoQueueGetDevice(Queue);


    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    KdPrint(("ToasterEvtIoDeviceControl called\n"));

    PAGED_CODE();

    switch (IoControlCode) {

    case IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE:
        //
        // This is just an example on how to hide your device in the
        // device manager. Please remove this code when you adapt
        // this sample for your hardware.
        //
        WDF_DEVICE_STATE_INIT(&deviceState);
        deviceState.DontDisplayInUI = WdfTrue;
        WdfDeviceSetDeviceState(
            hDevice,
            &deviceState
            );
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Complete the Request.
    //
    WdfRequestCompleteWithInformation(Request, status, (ULONG_PTR) 0);

}


