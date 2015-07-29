/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.c

Abstract:

    Bulk USB device driver for Intel 82930 USB test board
    Plug and Play module. This file contains routines to handle pnp requests.

Environment:

    Kernel mode

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, UsbSamp_EvtDeviceAdd)
#pragma alloc_text(PAGE, UsbSamp_EvtDevicePrepareHardware)
#pragma alloc_text(PAGE, UsbSamp_EvtDeviceContextCleanup)
#pragma alloc_text(PAGE, ReadAndSelectDescriptors)
#pragma alloc_text(PAGE, ConfigureDevice)
#pragma alloc_text(PAGE, SelectInterfaces)
#pragma alloc_text(PAGE, SetPowerPolicy)
#pragma alloc_text(PAGE, AbortPipes)
#pragma alloc_text(PAGE, ReadFdoRegistryKeyValue)
#pragma alloc_text(PAGE, RetrieveDeviceInformation)
#pragma alloc_text(PAGE, UsbSamp_ValidateConfigurationDescriptor)
#if (NTDDI_VERSION >= NTDDI_WIN8)
#pragma alloc_text(PAGE, UsbSamp_EvtPipeContextCleanup)
#pragma alloc_text(PAGE, InitializePipeContextForSuperSpeedDevice)
#pragma alloc_text(PAGE, GetEndpointDescriptorForEndpointAddress)
#pragma alloc_text(PAGE, InitializePipeContextForSuperSpeedIsochPipe)
#endif
#pragma alloc_text(PAGE, InitializePipeContextForHighSpeedDevice)
#pragma alloc_text(PAGE, InitializePipeContextForFullSpeedDevice)
#endif

NTSTATUS
UsbSamp_EvtDeviceAdd(
    WDFDRIVER        Driver,
    PWDFDEVICE_INIT  DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_FILEOBJECT_CONFIG     fileConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES               attributes;
    NTSTATUS                            status;
    WDFDEVICE                           device;
    WDF_DEVICE_PNP_CAPABILITIES         pnpCaps;
    WDF_IO_QUEUE_CONFIG                 ioQueueConfig;
    PDEVICE_CONTEXT                     pDevContext;
    WDFQUEUE                            queue;
    ULONG                               maximumTransferSize;

    UNREFERENCED_PARAMETER(Driver);

    UsbSamp_DbgPrint (3, ("UsbSamp_EvtDeviceAdd routine\n"));

    PAGED_CODE();

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDevicePrepareHardware = UsbSamp_EvtDevicePrepareHardware;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Initialize the request attributes to specify the context size and type
    // for every request created by framework for this device.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);

    WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handle Create, Close and
    // Cleanup requests that gets genereate when an application or another
    // kernel component opens an handle to the device. If you don't register
    // the framework default behaviour would be complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    WDF_FILEOBJECT_CONFIG_INIT(
        &fileConfig,
        UsbSamp_EvtDeviceFileCreate,
        WDF_NO_EVENT_CALLBACK,
        WDF_NO_EVENT_CALLBACK
        );

    //
    // Specify a context for FileObject. If you register FILE_EVENT callbacks,
    // the framework by default creates a framework FILEOBJECT corresponding
    // to the WDM fileobject. If you want to track any per handle context,
    // use the context for FileObject. Driver that typically use FsContext
    // field should instead use Framework FileObject context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FILE_CONTEXT);

    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                       &fileConfig,
                                       &attributes);

#if !defined(BUFFERED_READ_WRITE)
    //
    // I/O type is Buffered by default. We want to do direct I/O for Reads
    // and Writes so set it explicitly. Please note that this sample
    // can do isoch transfer only if the io type is directio.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

#endif

    //
    // Now specify the size of device extension where we track per device
    // context.DeviceInit is completely initialized. So call the framework
    // to create the device and attach it to the lower stack.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = UsbSamp_EvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfDeviceCreate failed with Status code 0x%x\n", status));
        return status;
    }

    //
    // Get the DeviceObject context by using accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for DEVICE_CONTEXT.
    //

    pDevContext = GetDeviceContext(device);

    //
    //Get MaximumTransferSize from registry
    //
    maximumTransferSize = 0;

    ReadFdoRegistryKeyValue(Driver,
                              L"MaximumTransferSize",
                              &maximumTransferSize);

    if (maximumTransferSize){
        pDevContext->MaximumTransferSize = maximumTransferSize;
    }
    else {
        pDevContext->MaximumTransferSize = DEFAULT_REGISTRY_TRANSFER_SIZE;
    }

    //
    // Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
    // that you don't get the popup in usermode (on Win2K) when you surprise
    // remove the device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.SurpriseRemovalOK = WdfTrue;

    WdfDeviceSetPnpCapabilities(device, &pnpCaps);

    //
    // Register I/O callbacks to tell the framework that you are interested
    // in handling WdfRequestTypeRead, WdfRequestTypeWrite, and 
    // IRP_MJ_DEVICE_CONTROL requests.
    // WdfIoQueueDispatchParallel means that we are capable of handling
    // all the I/O request simultaneously and we are responsible for protecting
    // data that could be accessed by these callbacks simultaneously.
    // This queue will be,  by default,  automanaged by the framework with
    // respect to PNP and Power events. That is, framework will take care
    // of queuing, failing, dispatching incoming requests based on the current
    // pnp/power state of the device.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                           WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoRead = UsbSamp_EvtIoRead;
    ioQueueConfig.EvtIoWrite = UsbSamp_EvtIoWrite;
    ioQueueConfig.EvtIoDeviceControl = UsbSamp_EvtIoDeviceControl;
    ioQueueConfig.EvtIoStop = UsbSamp_EvtIoStop;

    status = WdfIoQueueCreate(device,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue);// pointer to default queue
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfIoQueueCreate failed  for Default Queue 0x%x\n", status));
        return status;
    }

    //
    // Create a synchronized manual queue so we can retrieve one read request at a
    // time and dispatch it to the lower driver with the right StartFrame number.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig,
                              WdfIoQueueDispatchManual);
    
     WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
     attributes.SynchronizationScope=WdfSynchronizationScopeQueue;
     
     ioQueueConfig.EvtIoStop = UsbSamp_EvtIoStop;
    
     status = WdfIoQueueCreate(device,
                               &ioQueueConfig,
                               &attributes,
                               &pDevContext->IsochReadQueue);
     if (!NT_SUCCESS(status)) {
         UsbSamp_DbgPrint(1, ("WdfIoQueueCreate failed  for isoch 0x%x\n", status));
         return status;
     }

    //
    // Register a ready notification routine so we get notified whenever the queue transitions
    // from empty to non-empty state.
    //
    status = WdfIoQueueReadyNotify(pDevContext->IsochReadQueue,
                                   UsbSamp_EvtIoQueueReadyNotification,
                                   (WDFCONTEXT)pDevContext);
    
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfIoQueueReadyNotify failed  for isoch 0x%x\n", status));
        return status;
    }

    //
    // Create a synchronized manual queue so we can retrieve one write request at a
    // time and dispatch it to the lower driver with the right StartFrame number.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig,
                              WdfIoQueueDispatchManual);
    
     WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
     attributes.SynchronizationScope=WdfSynchronizationScopeQueue;
     
     ioQueueConfig.EvtIoStop = UsbSamp_EvtIoStop;
    
     status = WdfIoQueueCreate(device,
                               &ioQueueConfig,
                               &attributes,
                               &pDevContext->IsochWriteQueue);
     if (!NT_SUCCESS(status)) {
         UsbSamp_DbgPrint(1, ("WdfIoQueueCreate failed  for isoch 0x%x\n", status));
         return status;
     }

    //
    // Register a ready notification routine so we get notified whenever the queue transitions
    // from empty to non-empty state.
    //
    status = WdfIoQueueReadyNotify(pDevContext->IsochWriteQueue,
                                   UsbSamp_EvtIoQueueReadyNotification,
                                   (WDFCONTEXT)pDevContext);
    
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfIoQueueReadyNotify failed  for isoch 0x%x\n", status));
        return status;
    }     
     
    //
    // Register a device interface so that app can find our device and talk to it.
    //
    status = WdfDeviceCreateDeviceInterface(device,
                        (LPGUID) &GUID_CLASS_USBSAMP_USB,
                        NULL);// Reference String
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("WdfDeviceCreateDeviceInterface failed  0x%x\n", status));
        return status;
    }

    status = USBD_CreateHandle(WdfDeviceWdmGetDeviceObject(device),    
                               WdfDeviceWdmGetAttachedDevice(device),   
                               USBD_CLIENT_CONTRACT_VERSION_602,   
                               POOL_TAG,   
                               &pDevContext->UsbdHandle);   
    if(!NT_SUCCESS(status)){
        UsbSamp_DbgPrint(1, ("USBD_CreateHandle failed 0x%x", status));
        return status;
    }

    UsbSamp_DbgPrint(3, ("EvtDriverDeviceAdd - ends\n"));

    return status;
}

NTSTATUS
UsbSamp_EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

    //TODO:

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS                    status;
    PDEVICE_CONTEXT             pDeviceContext;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    UsbSamp_DbgPrint(3, ("EvtDevicePrepareHardware - begins\n"));

    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);

    //
    // Read the device descriptor, configuration descriptor
    // and select the interface descriptors
    //
    status = ReadAndSelectDescriptors(Device);

    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("ReadandSelectDescriptors failed\n"));
        return status;
    }

    //
    // Enable wait-wake and idle timeout if the device supports it
    //
    if (pDeviceContext->WaitWakeEnable){
        status = SetPowerPolicy(Device);
        if (!NT_SUCCESS (status)) {
            UsbSamp_DbgPrint(3, ("UsbSampSetPowerPolicy failed\n"));
            return status;
        }
    }

    UsbSamp_DbgPrint(3, ("EvtDevicePrepareHardware - ends\n"));

    return status;
}

NTSTATUS
SetPowerPolicy(
    _In_ WDFDEVICE Device
    )
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IDLE_CAPS_TYPE);
    idleSettings.IdleTimeout = 10000; // 10-sec

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
    if ( !NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(3, ("WdfDeviceSetPowerPolicyS0IdlePolicy failed  0x%x\n", status));
        return status;
    }

    //
    // Init wait-wake policy structure.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

    status = WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(3, ("WdfDeviceAssignSxWakeSettings failed  0x%x\n", status));
        return status;
    }

    return status;
}


NTSTATUS
ReadAndSelectDescriptors(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine configures the USB device.
    In this routines we get the device descriptor,
    the configuration descriptor and select the
    configuration.

Arguments:

    Device - Handle to a framework device

Return Value:

    NTSTATUS - NT status value.

--*/
{
    NTSTATUS               status;
    PDEVICE_CONTEXT        pDeviceContext;

    PAGED_CODE();

    //
    // initialize variables
    //
    pDeviceContext = GetDeviceContext(Device);

    //
    // Create a USB device handle so that we can communicate with the
    // underlying USB stack. The WDFUSBDEVICE handle is used to query,
    // configure, and manage all aspects of the USB device.
    // These aspects include device properties, bus properties,
    // and I/O creation and synchronization. We only create device the first
    // the PrepareHardware is called. If the device is restarted by pnp manager
    // for resource rebalance, we will use the same device handle but then select
    // the interfaces again because the USB stack could reconfigure the device on
    // restart.
    //
    if (pDeviceContext->WdfUsbTargetDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;
        
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config,
                                   USBD_CLIENT_CONTRACT_VERSION_602);
        
        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                              &config,
                                              WDF_NO_OBJECT_ATTRIBUTES,
                                              &pDeviceContext->WdfUsbTargetDevice);
        if (!NT_SUCCESS(status)) {
            UsbSamp_DbgPrint(1, ("WdfUsbTargetDeviceCreateWithParameters failed with Status code %x\n", status));
            return status;
        }
    }
    
    WdfUsbTargetDeviceGetDeviceDescriptor(pDeviceContext->WdfUsbTargetDevice,
                                    &pDeviceContext->UsbDeviceDescriptor);

    NT_ASSERT(pDeviceContext->UsbDeviceDescriptor.bNumConfigurations);

    status = ConfigureDevice(Device);
    
    return status;
}

NTSTATUS
ConfigureDevice(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This helper routine reads the configuration descriptor
    for the device in couple of steps.

Arguments:

    Device - Handle to a framework device

Return Value:

    NTSTATUS - NT status value

--*/
{
    USHORT                        size = 0;
    NTSTATUS                      status;
    PDEVICE_CONTEXT               pDeviceContext;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY   memory;
    PUCHAR Offset = NULL;

    PAGED_CODE();

    //
    // initialize the variables
    //
    configurationDescriptor = NULL;
    pDeviceContext = GetDeviceContext(Device);

    //
    // Read the first configuration descriptor
    // This requires two steps:
    // 1. Ask the WDFUSBDEVICE how big it is
    // 2. Allocate it and get it from the WDFUSBDEVICE
    //
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(pDeviceContext->WdfUsbTargetDevice,
                                               NULL,
                                               &size);

    if (status != STATUS_BUFFER_TOO_SMALL || size == 0) {
        return status;
    }

    //
    // Create a memory object and specify usbdevice as the parent so that
    // it will be freed automatically.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    attributes.ParentObject = pDeviceContext->WdfUsbTargetDevice;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPool,
                             POOL_TAG,
                             size,
                             &memory,
                             &configurationDescriptor);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(pDeviceContext->WdfUsbTargetDevice,
                                               configurationDescriptor,
                                               &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Check if the descriptors are valid
    // 
    status = UsbSamp_ValidateConfigurationDescriptor(configurationDescriptor, size , &Offset);
    
    if (!NT_SUCCESS(status)) {
        UsbSamp_DbgPrint(1, ("Descriptor validation failed with Status code %x and at the offset %p\n", status , Offset ));
        return status;
    }

    pDeviceContext->UsbConfigurationDescriptor = configurationDescriptor;

    status = SelectInterfaces(Device);

    return status;
}

NTSTATUS
SelectInterfaces(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This helper routine selects the configuration, interface and
    creates a context for every pipe (end point) in that interface.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS                            status;
    PDEVICE_CONTEXT                     pDeviceContext;
    UCHAR                               i;
    WDF_OBJECT_ATTRIBUTES               pipeAttributes;
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS selectSettingParams;
    UCHAR                               numberAlternateSettings = 0;
    UCHAR                               numberConfiguredPipes;

    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);

    //
    // The device has only one interface and the interface may have multiple
    // alternate settings. It will try to use alternate setting zero if it has
    // non-zero endpoints, otherwise it will try to search an alternate 
    // setting with non-zero endpoints.
    // 

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pipeAttributes, PIPE_CONTEXT);

#if (NTDDI_VERSION >= NTDDI_WIN8)
    pipeAttributes.EvtCleanupCallback = UsbSamp_EvtPipeContextCleanup;
#endif

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->WdfUsbTargetDevice,
                                        &pipeAttributes,
                                        &configParams);


    if (NT_SUCCESS(status) &&
        WdfUsbTargetDeviceGetNumInterfaces(pDeviceContext->WdfUsbTargetDevice) > 0) {

        status = RetrieveDeviceInformation(Device);
        if (!NT_SUCCESS(status)) {
            UsbSamp_DbgPrint(1, ("RetrieveDeviceInformation failed %x\n", status));
            return status;
        }

        pDeviceContext->UsbInterface =
            configParams.Types.SingleInterface.ConfiguredUsbInterface;

        //
        // This is written to work with Intel 82930 board, OSRUSBFX2, FX2 MUTT and FX3 MUTT
        // devices. The alternate setting zero of MUTT devices don't have any endpoints. So
        // in the code below, we will walk through the list of alternate settings until we
        // find one that has non-zero endpoints.
        //

        numberAlternateSettings = WdfUsbInterfaceGetNumSettings(pDeviceContext->UsbInterface);

        NT_ASSERT(numberAlternateSettings > 0);

        numberConfiguredPipes = 0;

        for (i = 0; i < numberAlternateSettings && numberConfiguredPipes == 0; i++) {
 
            WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&selectSettingParams, i);

            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pipeAttributes, PIPE_CONTEXT);

#if (NTDDI_VERSION >= NTDDI_WIN8)
            pipeAttributes.EvtCleanupCallback = UsbSamp_EvtPipeContextCleanup;
#endif

            status = WdfUsbInterfaceSelectSetting(pDeviceContext->UsbInterface,
                                                  &pipeAttributes,
                                                  &selectSettingParams
                                                  );
 
            if (NT_SUCCESS(status)) {
             
                numberConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(pDeviceContext->UsbInterface);

                if (numberConfiguredPipes > 0){
                
                    pDeviceContext->SelectedAlternateSetting = i;
                
                }
            
            }
         
        }

        pDeviceContext->NumberConfiguredPipes = numberConfiguredPipes;

        for (i = 0; i < pDeviceContext->NumberConfiguredPipes; i++) {
            WDFUSBPIPE pipe;

            pipe =  WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface,
                                                    i, //PipeIndex,
                                                    NULL
                                                    );
#if (NTDDI_VERSION >= NTDDI_WIN8)
            if (pDeviceContext->IsDeviceSuperSpeed) {
                status = InitializePipeContextForSuperSpeedDevice(pDeviceContext,
                                                                  pipe);
            }
            else if (pDeviceContext->IsDeviceHighSpeed) {
                status = InitializePipeContextForHighSpeedDevice(pipe);
            } 
            else {
                status = InitializePipeContextForFullSpeedDevice(pipe);
            }
#else
            if (pDeviceContext->IsDeviceHighSpeed) {
                status = InitializePipeContextForHighSpeedDevice(pipe);
            } 
            else {
                status = InitializePipeContextForFullSpeedDevice(pipe);
            }
#endif
            if (!NT_SUCCESS(status)) {
                UsbSamp_DbgPrint(1, ("InitializePipeContext failed %x\n", status));
                break;        
            }
        }

    }

    return status;
}


NTSTATUS
AbortPipes(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description

    sends an abort pipe request on all open pipes.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
{
    UCHAR              i;
    ULONG              count;
    NTSTATUS           status;
    PDEVICE_CONTEXT    pDevContext;

    PAGED_CODE();

    //
    // initialize variables
    //
    pDevContext = GetDeviceContext(Device);

    UsbSamp_DbgPrint(3, ("AbortPipes - begins\n"));

    count = pDevContext->NumberConfiguredPipes;

    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;
        pipe = WdfUsbInterfaceGetConfiguredPipe(pDevContext->UsbInterface,
                                                i, //PipeIndex,
                                                NULL
                                                );

        UsbSamp_DbgPrint(3, ("Aborting open pipe %d\n", i));

        status = WdfUsbTargetPipeAbortSynchronously(pipe,
                                    WDF_NO_HANDLE, // WDFREQUEST
                                    NULL);//PWDF_REQUEST_SEND_OPTIONS

        if (!NT_SUCCESS(status)) {
            UsbSamp_DbgPrint(1, ("WdfUsbTargetPipeAbortSynchronously failed %x\n", status));
            break;
        }
    }

    UsbSamp_DbgPrint(3, ("AbortPipes - ends\n"));

    return STATUS_SUCCESS;
}


#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSTATUS
InitializePipeContextForSuperSpeedDevice(
    _In_ PDEVICE_CONTEXT            DeviceContext,
    _In_ WDFUSBPIPE                 Pipe
    )
/*++

Routine Description

    This function initialize pipe context for super speed isoch and
    bulk endpoints.

Return Value:

    NT status value

--*/
{
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    NTSTATUS                    status = STATUS_SUCCESS;

    PAGED_CODE();

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
    
    //
    // We only use pipe context for super speed isoch and bulk speed bulk endpoints.
    //
    if ((WdfUsbPipeTypeIsochronous == pipeInfo.PipeType)) {

        status = InitializePipeContextForSuperSpeedIsochPipe(DeviceContext,
                    WdfUsbInterfaceGetInterfaceNumber(DeviceContext->UsbInterface),
                    Pipe);

    } else if (WdfUsbPipeTypeBulk == pipeInfo.PipeType) {

        status = InitializePipeContextForSuperSpeedBulkPipe(DeviceContext,
                   WdfUsbInterfaceGetInterfaceNumber(DeviceContext->UsbInterface),
                   Pipe);

    }
    
    return status;
     
}


PUSB_ENDPOINT_DESCRIPTOR
GetEndpointDescriptorForEndpointAddress(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ UCHAR            InterfaceNumber,
    _In_ UCHAR            EndpointAddress,
    _Out_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR *ppEndpointCompanionDescriptor
    )
/*++

Routine Description:

    The helper routine gets the Endpoint Descriptor matched with EndpointAddress and return
    its Endpoint Companion Descriptor if it has.

    UsbSamp_ValidateConfigurationDescriptor already validates that descriptors lie within
    allocated buffer.

Arguments:

    DeviceContext - pointer to the device context which includes configuration descriptor

    InterfaceNumber - InterfaceNumber of selected interface

    EndpointAddress - EndpointAddress of the Pipe

    ppEndpointCompanionDescriptor - pointer to the Endpoint Companioin Descroptor pointer

Return Value:

    Pointer to Endpoint Descriptor

--*/
{

    PUSB_COMMON_DESCRIPTOR                          pCommonDescriptorHeader      = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR                   pConfigurationDescriptor     = NULL;
    PUSB_INTERFACE_DESCRIPTOR                       pInterfaceDescriptor         = NULL;
    PUSB_ENDPOINT_DESCRIPTOR                        pEndpointDescriptor          = NULL;
    PUCHAR                                          startingPosition;
    ULONG                                           index;
    BOOLEAN                                         found                        = FALSE;

    PAGED_CODE();

    pConfigurationDescriptor = DeviceContext->UsbConfigurationDescriptor;

    *ppEndpointCompanionDescriptor = NULL;

    // 
    // Parse the ConfigurationDescriptor (including all Interface and
    // Endpoint Descriptors) and locate a Interface Descriptor which
    // matches the InterfaceNumber, AlternateSetting, InterfaceClass,
    // InterfaceSubClass, and InterfaceProtocol parameters.  
    //
    pInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                                pConfigurationDescriptor,
                                pConfigurationDescriptor,
                                InterfaceNumber,
                                DeviceContext->SelectedAlternateSetting,
                                -1, // InterfaceClass, don't care
                                -1, // InterfaceSubClass, don't care
                                -1  // InterfaceProtocol, don't care
                                );

    if (pInterfaceDescriptor == NULL ) {

        UsbSamp_DbgPrint(1, ("USBD_ParseConfigurationDescriptorEx failed to retrieve Interface Descriptor.\n"));
        goto End;

    }

    startingPosition = (PUCHAR) pInterfaceDescriptor;

    for(index = 0; index < pInterfaceDescriptor->bNumEndpoints; index++) {
    
        pCommonDescriptorHeader = USBD_ParseDescriptors(pConfigurationDescriptor,
                                                        pConfigurationDescriptor->wTotalLength,
                                                        startingPosition,
                                                        USB_ENDPOINT_DESCRIPTOR_TYPE);

        if (pCommonDescriptorHeader == NULL) {

            UsbSamp_DbgPrint(1, ("USBD_ParseDescriptors failed to retrieve SuperSpeed Endpoint Descriptor unexpectedly\n"));
            goto End;
        
        }

        //
        // UsbSamp_ValidateConfigurationDescriptor validates all descriptors.
        // This means that the descriptor pointed to by pCommonDescriptorHeader( received above ) is completely 
        // contained within the buffer representing ConfigurationDescriptor and
        // it also verifies that pCommonDescriptorHeader->bLength is equal to sizeof(USB_ENDPOINT_DESCRIPTOR).
        //

        pEndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) pCommonDescriptorHeader;

        //
        // Search an Endpoint Descriptor that matches the EndpointAddress
        //
        if (pEndpointDescriptor->bEndpointAddress == EndpointAddress){
        
            found = TRUE;

            break;
        
        }

        //
        // Skip the current Endpoint Descriptor and search for the next.
        //
        startingPosition = (PUCHAR)pCommonDescriptorHeader + pCommonDescriptorHeader->bLength;
    
    }

    if (found) {
        //
        // Locate the SuperSpeed Endpoint Companion Descriptor associated with the endpoint descriptor
        //
        pCommonDescriptorHeader = USBD_ParseDescriptors(pConfigurationDescriptor,
                                                        pConfigurationDescriptor->wTotalLength,
                                                        pEndpointDescriptor,
                                                        USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE);

        if (pCommonDescriptorHeader != NULL) {
            
            //
            // UsbSamp_ValidateConfigurationDescriptor validates all descriptors.
            // This means that the descriptor pointed to by pCommonDescriptorHeader( received above ) is completely 
            // contained within the buffer representing ConfigurationDescriptor and
            // it also verifies that pCommonDescriptorHeader->bLength is >= sizeof(USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR)
            //
                    
            *ppEndpointCompanionDescriptor = 
                (PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR) pCommonDescriptorHeader;
                
        } else {
        
             UsbSamp_DbgPrint(3, ("USBD_ParseDescriptors failed to retrieve SuperSpeed Endpoint Companion Descriptor unexpectedly\n"));
        
        }

    } 

 
End:
    return pEndpointDescriptor;
}


NTSTATUS
InitializePipeContextForSuperSpeedIsochPipe(
    _In_ PDEVICE_CONTEXT    DeviceContext,
    _In_ UCHAR              InterfaceNumber,
    _In_ WDFUSBPIPE         Pipe
    )
/*++

Routine Description

    This function validates all the isoch related fields in the endpoint descriptor
    to make sure it's in comformance with the spec and Microsoft core stack
    implementation and intializes the pipe context.

    The TransferSizePerMicroframe and TransferSizePerFrame values will be
    used in the I/O path to do read and write transfers.

Return Value:

    NT status value

-*/
{
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    PPIPE_CONTEXT               pipeContext;    
    UCHAR                       endpointAddress;
    PUSB_ENDPOINT_DESCRIPTOR    pEndpointDescriptor;
    PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR pEndpointCompanionDescriptor;
    USHORT                      wMaxPacketSize;
    UCHAR                       bMaxBurst;
    UCHAR                       bMult;
    USHORT                      wBytesPerInterval;


    PAGED_CODE();

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
    
    //
    // We use the pipe context only for isoch endpoints.
    //
    if ((WdfUsbPipeTypeIsochronous != pipeInfo.PipeType)) {

        return STATUS_SUCCESS;

    }

    pipeContext = GetPipeContext(Pipe);

    endpointAddress = pipeInfo.EndpointAddress;

    pEndpointDescriptor = GetEndpointDescriptorForEndpointAddress(
                                DeviceContext,
                                InterfaceNumber,
                                endpointAddress,
                                &pEndpointCompanionDescriptor);

    if (pEndpointDescriptor == NULL || pEndpointCompanionDescriptor == NULL ){
        
        UsbSamp_DbgPrint(1, ("pEndpointDescriptor or pEndpointCompanionDescriptor is invalid (NULL)\n"));
        return STATUS_INVALID_PARAMETER;
    
    }

    //
    // For SuperSpeed isoch endpoint, it uses wBytesPerInterval from its
    // endpoint companion descriptor. If bMaxBurst field in its endpoint
    // companion descriptor is greater than zero, wMaxPacketSize must be
    // 1024. If the value in the bMaxBurst field is set to zero then 
    // wMaxPacketSize can have any value from 0 to 1024.
    //
    wBytesPerInterval = pEndpointCompanionDescriptor->wBytesPerInterval;
    wMaxPacketSize = pEndpointDescriptor->wMaxPacketSize;
    bMaxBurst = pEndpointCompanionDescriptor->bMaxBurst;
    bMult = pEndpointCompanionDescriptor->bmAttributes.Isochronous.Mult;

    if (wBytesPerInterval > (wMaxPacketSize * (bMaxBurst + 1) * (bMult + 1))){
                        
        UsbSamp_DbgPrint(1, ("SuperSpeed isochronouse endpoints's wBytesPerInterval value (%d) is greater than wMaxPacketSize * (bMaxBurst+1) * (Mult +1) (%d) \n",
                                wBytesPerInterval, (wMaxPacketSize * (bMaxBurst + 1) * (bMult + 1))));
        return STATUS_INVALID_PARAMETER;

    }

    if (bMaxBurst > 0){

        if (wMaxPacketSize != USB_ENDPOINT_SUPERSPEED_ISO_MAX_PACKET_SIZE){              
                    
            UsbSamp_DbgPrint(1, ("SuperSpeed isochronouse endpoints must have wMaxPacketSize value of %d bytes when bMaxpBurst is %d \n",
                                    USB_ENDPOINT_SUPERSPEED_ISO_MAX_PACKET_SIZE, bMaxBurst));
            return STATUS_INVALID_PARAMETER;

        } 

    } else {

        if (wMaxPacketSize > USB_ENDPOINT_SUPERSPEED_ISO_MAX_PACKET_SIZE){

            UsbSamp_DbgPrint(1, ("SuperSpeed isochronouse endpoints must have wMaxPacketSize value no more than %d bytes when bMaxpBurst is %d \n",
                                    USB_ENDPOINT_SUPERSPEED_ISO_MAX_PACKET_SIZE, bMaxBurst));
            return STATUS_INVALID_PARAMETER;

        } 

    }

 
    //
    // This sample demos how to use wBytesPerInterval from its Endpoint 
    // Companion Descriptor. Actaully, for Superspeed isochronous endpoints,
    // MaximumPacketSize in WDF_USB_PIPE_INFORMATION and USBD_PIPE_INFORMATION
    // is returned with the value of wBytesPerInterval in the endpoint
    // companion descriptor. This is different than the true MaxPacketSize of
    // the endpoint descriptor.
    //
    NT_ASSERT(pipeInfo.MaximumPacketSize == wBytesPerInterval); 
    pipeContext->TransferSizePerMicroframe = wBytesPerInterval;
         
    //
    // Microsoft USB 3.0 stack only supports bInterval value of 1, 2, 3 and 4 
    // (or polling period of 1, 2, 4 and 8).
    // For super-speed isochronous endpoints, the bInterval value is used as
    // the exponent for a 2^(bInterval-1) value expressed in microframes;
    // e.g., a bInterval of 4 means a period of 8 (2^(4-1)) microframes.
    //
    if (pipeInfo.Interval == 0 || pipeInfo.Interval > 4) {

        UsbSamp_DbgPrint(1, ("bInterval value in pipeInfo is invalid (0 or > 4)\n"));
        return STATUS_INVALID_PARAMETER;

    }

    switch (pipeInfo.Interval) {
    case 1:
        //
        // Transfer period is every microframe (8 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 8;
        break;

    case 2:
        //
        // Transfer period is every 2 microframes (4 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 4;
        break;

    case 3:
        //
        // Transfer period is every 4 microframes (2 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 2;
        break;

    case 4:
        //
        // Transfer period is every 8 microframes (1 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe;
        break;
    }

    UsbSamp_DbgPrint(1, ("MaxPacketSize = %d, bInterval = %d\n", 
                       pipeInfo.MaximumPacketSize,
                       pipeInfo.Interval));

    UsbSamp_DbgPrint(1, ("TransferSizePerFrame = %d, TransferSizePerMicroframe = %d\n", 
                       pipeContext->TransferSizePerFrame, 
                       pipeContext->TransferSizePerMicroframe));        
    
    return STATUS_SUCCESS;
}

#endif

NTSTATUS
InitializePipeContextForHighSpeedDevice(
    _In_ WDFUSBPIPE Pipe
    )
/*++

Routine Description

    This function validates all the isoch related fields in the endpoint descriptor
    to make sure it's in comformance with the spec and Microsoft core stack
    implementation and intializes the pipe context.

    The TransferSizePerMicroframe and TransferSizePerFrame values will be
    used in the I/O path to do read and write transfers.

Return Value:

    NT status value

--*/
{
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    PPIPE_CONTEXT               pipeContext;

    PAGED_CODE();

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
    
    //
    // We use the pipe context only for isoch endpoints.
    //
    if ((WdfUsbPipeTypeIsochronous != pipeInfo.PipeType)) {
        return STATUS_SUCCESS;
    }
    
    pipeContext = GetPipeContext(Pipe);
   
    if (pipeInfo.MaximumPacketSize == 0) {
        UsbSamp_DbgPrint(1, ("MaximumPacketSize in the pipeInfo is invalid (zero)\n"));
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Universal Serial Bus Specification Revision 2.0 5.6.3 Isochronous Transfer 
    // Packet Size Constraints: High-speed endpoints are allowed up to 1024-byte data
    // payloads per microframe and allowed up to a maximum of 3 transactions per microframe.
    //
    // For highspeed isoch endpoints, bits 12-11 of wMaxPacketSize in the endpoint descriptor
    // specify the number of additional transactions oppurtunities per microframe.
    // 00 - None (1 transaction per microframe)
    // 01 - 1 additional (2 per microframe)
    // 10 - 2 additional (3 per microframe)
    // 11 - Reserved.
    //
    // Note: MaximumPacketSize of WDF_USB_PIPE_INFORMATION is already adjusted to include 
    // additional transactions if it is a high bandwidth pipe.
    //

    if (pipeInfo.MaximumPacketSize > 1024 * 3) {
        UsbSamp_DbgPrint(1, ("MaximumPacketSize in the endpoint descriptor is invalid (>1024*3)\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Microsoft USB stack only supports bInterval value of 1, 2, 3 and 4 (or polling period of 1, 2, 4 and 8).
    //    
    if (pipeInfo.Interval == 0 || pipeInfo.Interval > 4) {
        UsbSamp_DbgPrint(1, ("bInterval value in pipeInfo is invalid (0 or > 4)\n"));
        return STATUS_INVALID_PARAMETER;
    }

    pipeContext->TransferSizePerMicroframe = pipeInfo.MaximumPacketSize;

    //
    // For high-speed isochronous endpoints, the bInterval value is used
    // as the exponent for a 2^(bInterval-1) value expressed in
    // microframes; e.g., a bInterval of 4 means a period of 8 (2^(4-1))
    // microframes. The bInterval value must be from 1 to 16.  NOTE: The
    // USBPORT.SYS driver only supports high-speed isochronous bInterval
    // values of {1, 2, 3, 4}.
    //
    switch (pipeInfo.Interval) {
    case 1:
        //
        // Transfer period is every microframe (8 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 8;
        break;

    case 2:
        //
        // Transfer period is every 2 microframes (4 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 4;
        break;

    case 3:
        //
        // Transfer period is every 4 microframes (2 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe * 2;
        break;

    case 4:
        //
        // Transfer period is every 8 microframes (1 times a frame).
        //
        pipeContext->TransferSizePerFrame = pipeContext->TransferSizePerMicroframe;
        break;
    }

    UsbSamp_DbgPrint(1, ("MaxPacketSize = %d, bInterval = %d\n", 
                       pipeInfo.MaximumPacketSize,
                       pipeInfo.Interval));

    UsbSamp_DbgPrint(1, ("TransferSizePerFrame = %d, TransferSizePerMicroframe = %d\n", 
                       pipeContext->TransferSizePerFrame, 
                       pipeContext->TransferSizePerMicroframe));        
    
    return STATUS_SUCCESS;
}

NTSTATUS
InitializePipeContextForFullSpeedDevice(
    _In_ WDFUSBPIPE Pipe
    )
/*++

Routine Description

    This function validates all the isoch related fields in the endpoint descriptor
    to make sure it's in comformance with the spec and Microsoft core stack
    implementation and intializes the pipe context.

    The TransferSizePerMicroframe and TransferSizePerFrame values will be
    used in the I/O path to do read and write transfers.

Return Value:

    NT status value

--*/
{
    WDF_USB_PIPE_INFORMATION    pipeInfo;
    PPIPE_CONTEXT               pipeContext;

    PAGED_CODE();

    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
    
    //
    // We use the pipe context only for isoch endpoints.
    //
    if ((WdfUsbPipeTypeIsochronous != pipeInfo.PipeType)) {
        return STATUS_SUCCESS;
    }
    
    pipeContext = GetPipeContext(Pipe);

    if (pipeInfo.MaximumPacketSize == 0) {
        UsbSamp_DbgPrint(1, ("MaximumPacketSize in the endpoint descriptor is invalid\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Universal Serial Bus Specification Revision 2.0
    // 5.6.3 Isochronous Transfer Packet Size Constraints
    //
    // The USB limits the maximum data payload size to 1,023 bytes
    // for each full-speed isochronous endpoint.
    //
    if (pipeInfo.MaximumPacketSize > 1023) {
        UsbSamp_DbgPrint(1, ("MaximumPacketSize in the endpoint descriptor is invalid\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Microsoft USB stack only supports bInterval value of 1 for
    // full-speed isochronous endpoints.
    //    
    if (pipeInfo.Interval != 1) {
        UsbSamp_DbgPrint(1, ("bInterval value in endpoint descriptor is invalid\n"));
        return STATUS_INVALID_PARAMETER;
    }

    pipeContext->TransferSizePerFrame = pipeInfo.MaximumPacketSize;
    pipeContext->TransferSizePerMicroframe = 0;

    UsbSamp_DbgPrint(1, ("TransferSizePerFrame = %d\n", pipeContext->TransferSizePerFrame));        
    
    return STATUS_SUCCESS;
}

NTSTATUS
RetrieveDeviceInformation(
    _In_ WDFDEVICE Device
    )
{
    PDEVICE_CONTEXT             pDeviceContext;
    WDF_USB_DEVICE_INFORMATION  info;
    NTSTATUS                    status;
    USHORT                      numberOfStreams = 0;
    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);

    WDF_USB_DEVICE_INFORMATION_INIT(&info);

    //
    // Retrieve USBD version information, port driver capabilites and device
    // capabilites such as speed, power, etc.
    //
    status = WdfUsbTargetDeviceRetrieveInformation(pDeviceContext->WdfUsbTargetDevice,
                                                   &info);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pDeviceContext->IsDeviceHighSpeed =
        (info.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) ? TRUE : FALSE;

    UsbSamp_DbgPrint(3, ("DeviceIsHighSpeed: %s\n",
                 pDeviceContext->IsDeviceHighSpeed ? "TRUE" : "FALSE"));

    UsbSamp_DbgPrint(3, ("IsDeviceSelfPowered: %s\n",
        (info.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED) ? "TRUE" : "FALSE"));

    pDeviceContext->WaitWakeEnable =
                        info.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;

    UsbSamp_DbgPrint(3, ("IsDeviceRemoteWakeable: %s\n",
        (info.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE) ? "TRUE" : "FALSE"));

    status = GetStackCapability(Device,
                                &GUID_USB_CAPABILITY_DEVICE_CONNECTION_SUPER_SPEED_COMPATIBLE,
                                0,
                                NULL);
    if (NT_SUCCESS(status)) {
        pDeviceContext->IsDeviceSuperSpeed = TRUE;
    }

    UsbSamp_DbgPrint(3, ("DeviceIsSuperSpeed: %s\n",
                 pDeviceContext->IsDeviceSuperSpeed ? "TRUE" : "FALSE"));

    if (pDeviceContext->IsDeviceSuperSpeed == TRUE) {
        status = GetStackCapability(Device,
                                    &GUID_USB_CAPABILITY_STATIC_STREAMS,
                                    sizeof(numberOfStreams),
                                    (PUCHAR)&numberOfStreams);
        if (NT_SUCCESS(status)) {
            pDeviceContext->IsStaticStreamsSupported = TRUE;
            pDeviceContext->NumberOfStreamsSupportedByController = numberOfStreams;
        }

        if (pDeviceContext->IsStaticStreamsSupported) {
             UsbSamp_DbgPrint(3, ("Number of Streams supported by the controller:  %d\n", numberOfStreams));
        } 
    }

    return STATUS_SUCCESS;
}

    
NTSTATUS
ReadFdoRegistryKeyValue(
    _In_  WDFDRIVER   Driver,
    _In_ LPWSTR      Name,
    _Out_ PULONG      Value
    )
/*++

Routine Description:

    Can be used to read any REG_DWORD registry value stored
    under Device Parameter.

Arguments:

    Driver - pointer to the device object
    Name - Name of the registry value
    Value -


Return Value:

    NTSTATUS 

--*/
{
    WDFKEY      hKey = NULL;
    NTSTATUS    status;
    UNICODE_STRING  valueName;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    *Value = 0;

    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(),
                                                KEY_READ,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &hKey);

    if (NT_SUCCESS (status)) {

        RtlInitUnicodeString(&valueName,Name);

        status = WdfRegistryQueryULong (hKey,
                                  &valueName,
                                  Value);

        WdfRegistryClose(hKey);
    }

    return status;
}

VOID
UsbSamp_EvtDeviceContextCleanup(
    IN WDFOBJECT WdfDevice
   )
/*++

Routine Description:

    In this callback, it cleans up device context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    WDFDEVICE device;
    PDEVICE_CONTEXT	pDevContext;

    PAGED_CODE();

    device = (WDFDEVICE)WdfDevice;

    pDevContext = GetDeviceContext(device);

    if (pDevContext->UsbdHandle != NULL) {
        USBD_CloseHandle(pDevContext->UsbdHandle);
    }

}

USBD_STATUS
UsbSamp_ValidateConfigurationDescriptor(  
    _In_reads_bytes_(BufferLength) PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    _In_ ULONG BufferLength,
    _Inout_ PUCHAR *Offset
    )
/*++

Routine Description:

    Validates a USB Configuration Descriptor

Parameters:

    ConfigDesc: Pointer to the entire USB Configuration descriptor returned by 
        the device

    BufferLength: Known size of buffer pointed to by ConfigDesc (Not wTotalLength)

    Offset: if the USBD_STATUS returned is not USBD_STATUS_SUCCESS, offet will
        be set to the address within the ConfigDesc buffer where the failure 
        occured.

Return Value:

    USBD_STATUS
    Success implies the configuration descriptor is valid.

--*/
{
    USBD_STATUS status = USBD_STATUS_SUCCESS;
    PUCHAR offset, end;
    PUCHAR index = NULL;
    USHORT ValidationLevel = 3;

    PAGED_CODE();

    //
    // Call USBD_ValidateConfigurationDescriptor to validate the descriptors which are present in this supplied configuration descriptor.
    // USBD_ValidateConfigurationDescriptor validates that all descriptors are completely contained within the configuration descriptor buffer.
    // It also checks for interface numbers, number of endpoints in an interface etc.
    // Please refer to msdn documentation for this function for more information.
    //
    
    status = USBD_ValidateConfigurationDescriptor( ConfigDesc, BufferLength , ValidationLevel , Offset , POOL_TAG );
    if (!(NT_SUCCESS (status)) ){
        return status;
    }

    //
    // TODO: You should validate the correctness of other descriptors which are not taken care by USBD_ValidateConfigurationDescriptor
    // The below code loops through the configuration descriptor and verifies that all instances of 
    // USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR have a size of >= USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR
    // You should put in more validations as per requirement
    // 
  
   
    offset = ((PUCHAR)ConfigDesc) + ConfigDesc->bLength;
    end = ((PUCHAR)ConfigDesc) + ConfigDesc->wTotalLength;

    while(offset < end){
        PUSB_COMMON_DESCRIPTOR commonDesc;
        //
        // Ensure the descriptor header is in bounds.  We always
        // need to ensure we have enough data to look at the descriptor
        // header fields.  Sometimes descriptors are zeroed at the end,         
        // this is OK.
        //
        commonDesc = (PUSB_COMMON_DESCRIPTOR)offset;

        
        switch(commonDesc->bDescriptorType){

            case USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE:

                //
                // basic validation.
                //
                if(commonDesc->bLength < sizeof(USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR)) {
                    status = USBD_STATUS_BAD_DESCRIPTOR;
                    index = (PUCHAR)commonDesc;
                    goto ValidateConfigurationDescriptor_Done;
                }
                break;

            //
            // TODO: Add validation for any other descriptor your device will return.
            //
            default:
                break;
        }
        offset += commonDesc->bLength;

    }

ValidateConfigurationDescriptor_Done:
    
    if(!USBD_SUCCESS(status)){
        NT_ASSERT(index);
        *Offset = index;
    }
    else{
        *Offset = NULL;
    }
    return status;
}

#if (NTDDI_VERSION >= NTDDI_WIN8)
VOID
UsbSamp_EvtPipeContextCleanup(
    IN WDFOBJECT WdfObject
   )
/*++

Routine Description:

    In this callback, it cleans up pipe context.

Arguments:

    WdfObject - WDFUSBPIPE object

Return Value:

    NULL

--*/
{
    WDFUSBPIPE pipe;
    PPIPE_CONTEXT	pPipeContext = NULL;
    PUSBSAMP_STREAM_INFO  pStreamInfo = NULL;

    PAGED_CODE();

    pipe = (WDFUSBPIPE)WdfObject;
    pPipeContext = GetPipeContext(pipe);

    pPipeContext->StreamConfigured = FALSE;
    pStreamInfo = &pPipeContext->StreamInfo;
    pStreamInfo->NumberOfStreams = 0;
    if(pStreamInfo->StreamList != NULL){
        ExFreePool(pStreamInfo->StreamList);
        pStreamInfo->StreamList = NULL;
    }

}
#endif
