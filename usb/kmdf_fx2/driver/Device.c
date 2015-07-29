/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.c

Abstract:

    USB device driver for OSR USB-FX2 Learning Kit

Environment:

    Kernel mode only


--*/

#include <osrusbfx2.h>
#include <devpkey.h>

#if defined(EVENT_TRACING)
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OsrFxEvtDeviceAdd)
#pragma alloc_text(PAGE, OsrFxEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, OsrFxEvtDeviceD0Exit)
#pragma alloc_text(PAGE, SelectInterfaces)
#pragma alloc_text(PAGE, OsrFxSetPowerPolicy)
#pragma alloc_text(PAGE, OsrFxReadFdoRegistryKeyValue)
#pragma alloc_text(PAGE, GetDeviceEventLoggingNames)
#pragma alloc_text(PAGE, OsrFxValidateConfigurationDescriptor)
#endif


NTSTATUS
OsrFxEvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
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
    WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES               attributes;
    NTSTATUS                            status;
    WDFDEVICE                           device;
    WDF_DEVICE_PNP_CAPABILITIES         pnpCaps;
    WDF_IO_QUEUE_CONFIG                 ioQueueConfig;
    PDEVICE_CONTEXT                     pDevContext;
    WDFQUEUE                            queue;
    GUID                                activity;
    UNICODE_STRING                      symbolicLinkName;
    WDFSTRING                           symbolicLinkString;
    DEVPROP_BOOLEAN                     isRestricted;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,"--> OsrFxEvtDeviceAdd routine\n");

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    //
    // For usb devices, PrepareHardware callback is the to place select the
    // interface and configure the device.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = OsrFxEvtDevicePrepareHardware;

    //
    // These two callbacks start and stop the wdfusb pipe continuous reader
    // as we go in and out of the D0-working state.
    //

    pnpPowerCallbacks.EvtDeviceD0Entry = OsrFxEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = OsrFxEvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoFlush = OsrFxEvtDeviceSelfManagedIoFlush;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Now specify the size of device extension where we track per device
    // context.DeviceInit is completely initialized. So call the framework
    // to create the device and attach it to the lower stack.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreate failed with Status code %!STATUS!\n", status);
        return status;
    }

    //
    // Setup the activity ID so that we can log events using it.
    //

    activity = DeviceToActivityId(device);

    //
    // Get the DeviceObject context by using accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for DEVICE_CONTEXT.
    //
    pDevContext = GetDeviceContext(device);

    //
    // Get the device's friendly name and location so that we can use it in
    // error logging.  If this fails then it will setup dummy strings.
    //

    GetDeviceEventLoggingNames(device);

    //
    // Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
    // that you don't get the popup in usermode when you surprise remove the device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.SurpriseRemovalOK = WdfTrue;

    WdfDeviceSetPnpCapabilities(device, &pnpCaps);

    //
    // Create a parallel default queue and register an event callback to
    // receive ioctl requests. We will create separate queues for
    // handling read and write requests. All other requests will be
    // completed with error status automatically by the framework.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                             WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoDeviceControl    = OsrFxEvtIoDeviceControl;
    
    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests for 
    // long time or forward them to other drivers. 
    // If the EvtIoStop callback is not implemented, the framework waits for
    // all driver-owned requests to be done before moving in the Dx/sleep 
    // states or before removing the device, which is the correct behavior 
    // for this type of driver. If the requests were taking an indeterminate
    // amount of time to complete, or if the driver forwarded the requests
    // to a lower driver/another stack, the queue should have an 
    // EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                             &ioQueueConfig,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &queue);// pointer to default queue
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                            "WdfIoQueueCreate failed  %!STATUS!\n", status);
        goto Error;
    }

    //
    // We will create a separate sequential queue and configure it
    // to receive read requests.  We also need to register a EvtIoStop
    // handler so that we can acknowledge requests that are pending
    // at the target driver.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoRead = OsrFxEvtIoRead;
    ioQueueConfig.EvtIoStop = OsrFxEvtIoStop;

    status = WdfIoQueueCreate(
                 device,
                 &ioQueueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue // queue handle
             );

    if (!NT_SUCCESS (status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed 0x%x\n", status);
        goto Error;
    }

    status = WdfDeviceConfigureRequestDispatching(
                    device,
                    queue,
                    WdfRequestTypeRead);

    if(!NT_SUCCESS (status)){
        NT_ASSERT(NT_SUCCESS(status));
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceConfigureRequestDispatching failed 0x%x\n", status);
        goto Error;
    }


    //
    // We will create another sequential queue and configure it
    // to receive write requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoWrite = OsrFxEvtIoWrite;
    ioQueueConfig.EvtIoStop  = OsrFxEvtIoStop;

    status = WdfIoQueueCreate(
                 device,
                 &ioQueueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue // queue handle
             );

    if (!NT_SUCCESS (status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed 0x%x\n", status);
        goto Error;
    }

     status = WdfDeviceConfigureRequestDispatching(
                    device,
                    queue,
                    WdfRequestTypeWrite);

    if(!NT_SUCCESS (status)){
        NT_ASSERT(NT_SUCCESS(status));
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceConfigureRequestDispatching failed 0x%x\n", status);
        goto Error;
    }

    //
    // Register a manual I/O queue for handling Interrupt Message Read Requests.
    // This queue will be used for storing Requests that need to wait for an
    // interrupt to occur before they can be completed.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

    //
    // This queue is used for requests that dont directly access the device. The
    // requests in this queue are serviced only when the device is in a fully
    // powered state and sends an interrupt. So we can use a non-power managed
    // queue to park the requests since we dont care whether the device is idle
    // or fully powered up.
    //
    ioQueueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &pDevContext->InterruptMsgQueue
                              );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed 0x%x\n", status);
        goto Error;
    }

    //
    // Register a device interface so that app can find our device and talk to it.
    //
    status = WdfDeviceCreateDeviceInterface(device,
                                            (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                            NULL); // Reference String

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                 "WdfDeviceCreateDeviceInterface failed  %!STATUS!\n", status);
        goto Error;
    }

    // 
    // Create the lock that we use to serialize calls to ResetDevice(). As an 
    // alternative to using a WDFWAITLOCK to serialize the calls, a sequential 
    // WDFQUEUE can be created and reset IOCTLs would be forwarded to it.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfWaitLockCreate(&attributes, &pDevContext->ResetDeviceWaitLock);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                 "WdfWaitLockCreate failed  %!STATUS!\n", status);
        goto Error;
    }

    //
    // Get the string for the device interface and set the restricted
    // property on it to allow applications bound with device metadata
    // to access the interface.
    //
    if (g_pIoSetDeviceInterfacePropertyData != NULL) {

        status = WdfStringCreate(NULL,
                                 WDF_NO_OBJECT_ATTRIBUTES,
                                 &symbolicLinkString);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                     "WdfStringCreate failed  %!STATUS!\n", status);
            goto Error;
        }

        status = WdfDeviceRetrieveDeviceInterfaceString(device,
                                                        (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                                        NULL,
                                                        symbolicLinkString);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                     "WdfDeviceRetrieveDeviceInterfaceString failed  %!STATUS!\n", status);
            goto Error;
        }

        WdfStringGetUnicodeString(symbolicLinkString, &symbolicLinkName);

        isRestricted = DEVPROP_TRUE;

        status = g_pIoSetDeviceInterfacePropertyData(&symbolicLinkName,
                                                     &DEVPKEY_DeviceInterface_Restricted,
                                                     0,
                                                     0,
                                                     DEVPROP_TYPE_BOOLEAN,
                                                     sizeof(isRestricted),
                                                     &isRestricted );

        WdfObjectDelete(symbolicLinkString);

        if (!NT_SUCCESS(status)) {
           TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                     "IoSetDeviceInterfacePropertyData failed  %!STATUS!\n", status);
            goto Error;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- OsrFxEvtDeviceAdd\n");

    return status;

Error:

    //
    // Log fail to add device to the event log
    //
    EventWriteFailAddDevice(&activity,
                            pDevContext->DeviceName,
                            pDevContext->Location,
                            status);

    return status;
}

NTSTATUS
OsrFxEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourceList,
    WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

Arguments:

    Device - handle to a device

    ResourceList - handle to a resource-list object that identifies the
                   raw hardware resources that the PnP manager assigned
                   to the device

    ResourceListTranslated - handle to a resource-list object that
                             identifies the translated hardware resources
                             that the PnP manager assigned to the device

Return Value:

    NT status value

--*/
{
    NTSTATUS                            status;
    PDEVICE_CONTEXT                     pDeviceContext;
    WDF_USB_DEVICE_INFORMATION          deviceInfo;
    ULONG                               waitWakeEnable;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);
    waitWakeEnable = FALSE;
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "--> EvtDevicePrepareHardware\n");

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
    if (pDeviceContext->UsbDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;

        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config,
                                   USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                               &config,
                                               WDF_NO_OBJECT_ATTRIBUTES,
                                               &pDeviceContext->UsbDevice);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                 "WdfUsbTargetDeviceCreateWithParameters failed with Status code %!STATUS!\n", status);
            return status;
        }
 
        //
        // TODO: If you are fetching configuration descriptor from device for
        // selecting a configuration or to parse other descriptors, call OsrFxValidateConfigurationDescriptor
        // to do basic validation on the descriptors before you access them .
        //
    }

    //
    // Retrieve USBD version information, port driver capabilites and device
    // capabilites such as speed, power, etc.
    //
    WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);

    status = WdfUsbTargetDeviceRetrieveInformation(
                                pDeviceContext->UsbDevice,
                                &deviceInfo);
    if (NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "IsDeviceHighSpeed: %s\n",
            (deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) ? "TRUE" : "FALSE");
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                    "IsDeviceSelfPowered: %s\n",
            (deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED) ? "TRUE" : "FALSE");

        waitWakeEnable = deviceInfo.Traits &
                            WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                            "IsDeviceRemoteWakeable: %s\n",
                            waitWakeEnable ? "TRUE" : "FALSE");
        //
        // Save these for use later.
        //
        pDeviceContext->UsbDeviceTraits = deviceInfo.Traits;
    }
    else  {
        pDeviceContext->UsbDeviceTraits = 0;
    }

    status = SelectInterfaces(Device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "SelectInterfaces failed 0x%x\n", status);
        return status;
    }

    //
    // Enable wait-wake and idle timeout if the device supports it
    //
    if (waitWakeEnable) {
        status = OsrFxSetPowerPolicy(Device);
        if (!NT_SUCCESS (status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                                "OsrFxSetPowerPolicy failed  %!STATUS!\n", status);
            return status;
        }
    }

    status = OsrFxConfigContReaderForInterruptEndPoint(pDeviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- EvtDevicePrepareHardware\n");

    return status;
}


NTSTATUS
OsrFxEvtDeviceD0Entry(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

    EvtDeviceD0Entry event callback must perform any operations that are
    necessary before the specified device is used.  It will be called every
    time the hardware needs to be (re-)initialized.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

    This function runs at PASSIVE_LEVEL, even though it is not paged.  A
    driver can optionally make this function pageable if DO_POWER_PAGABLE
    is set.  Even if DO_POWER_PAGABLE isn't set, this function still runs
    at PASSIVE_LEVEL.  In this case, though, the function absolutely must
    not do anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    PreviousState - Device power state which the device was in most recently.
        If the device is being newly started, this will be
        PowerDeviceUnspecified.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_CONTEXT         pDeviceContext;
    NTSTATUS                status;
    BOOLEAN                 isTargetStarted;

    pDeviceContext = GetDeviceContext(Device);
    isTargetStarted = FALSE;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
                "-->OsrFxEvtEvtDeviceD0Entry - coming from %s\n",
                DbgDevicePowerString(PreviousState));

    //
    // Since continuous reader is configured for this interrupt-pipe, we must explicitly start
    // the I/O target to get the framework to post read requests.
    //
    status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pDeviceContext->InterruptPipe));
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_POWER, "Failed to start interrupt pipe %!STATUS!\n", status);
        goto End;
    }

    isTargetStarted = TRUE;

End:

    if (!NT_SUCCESS(status)) {
        //
        // Failure in D0Entry will lead to device being removed. So let us stop the continuous
        // reader in preparation for the ensuing remove.
        //
        if (isTargetStarted) {
            WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pDeviceContext->InterruptPipe), WdfIoTargetCancelSentIo);
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "<--OsrFxEvtEvtDeviceD0Entry\n");

    return status;
}


NTSTATUS
OsrFxEvtDeviceD0Exit(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    This routine undoes anything done in EvtDeviceD0Entry.  It is called
    whenever the device leaves the D0 state, which happens when the device is
    stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.


    EvtDeviceD0Exit event callback must perform any operations that are
    necessary before the specified device is moved out of the D0 state.  If the
    driver needs to save hardware state before the device is powered down, then
    that should be done here.

    This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
    driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

    Even if DO_POWER_PAGABLE isn't set, this function still runs at
    PASSIVE_LEVEL.  In this case, though, the function absolutely must not do
    anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    TargetState - Device power state which the device will be put in once this
        callback is complete.

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
{
    PDEVICE_CONTEXT         pDeviceContext;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
          "-->OsrFxEvtDeviceD0Exit - moving to %s\n",
          DbgDevicePowerString(TargetState));

    pDeviceContext = GetDeviceContext(Device);

    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pDeviceContext->InterruptPipe),   WdfIoTargetCancelSentIo);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "<--OsrFxEvtDeviceD0Exit\n");

    return STATUS_SUCCESS;
}

VOID
OsrFxEvtDeviceSelfManagedIoFlush(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine handles flush activity for the device's
    self-managed I/O operations.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    None

--*/
{
    // Service the interrupt message queue to drain any outstanding
    // requests
    OsrUsbIoctlGetInterruptMessage(Device, STATUS_DEVICE_REMOVED);
}

_IRQL_requires_(PASSIVE_LEVEL)
USBD_STATUS
OsrFxValidateConfigurationDescriptor(
    _In_reads_bytes_(BufferLength) PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    _In_ ULONG BufferLength,
    _Inout_ PUCHAR *Offset
    )
/*++

Routine Description:

    Validates a USB Configuration Descriptor

Parameters:

    ConfigDesc: Pointer to the entire USB Configuration descriptor returned by the device

    BufferLength: Known size of buffer pointed to by ConfigDesc (Not wTotalLength)

    Offset: if the USBD_STATUS returned is not USBD_STATUS_SUCCESS, offet will
        be set to the address within the ConfigDesc buffer where the failure occured.

Return Value:

    USBD_STATUS
    Success implies the configuration descriptor is valid.

--*/
{


    USBD_STATUS status = USBD_STATUS_SUCCESS;
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
    // Check that all such descriptors have size >= sizeof(the descriptor they point to) 
    // Check for any association between them if required 
    // 
   
    return status;
}


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
OsrFxSetPowerPolicy(
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
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
    idleSettings.IdleTimeout = 10000; // 10-sec

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
    if ( !NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                "WdfDeviceSetPowerPolicyS0IdlePolicy failed %x\n", status);
        return status;
    }

    //
    // Init wait-wake policy structure.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

    status = WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceAssignSxWakeSettings failed %x\n", status);
        return status;
    }

    return status;
}


_IRQL_requires_(PASSIVE_LEVEL)
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
    NTSTATUS                            status = STATUS_SUCCESS;
    PDEVICE_CONTEXT                     pDeviceContext;
    WDFUSBPIPE                          pipe;
    WDF_USB_PIPE_INFORMATION            pipeInfo;
    UCHAR                               index;
    UCHAR                               numberConfiguredPipes;

    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &configParams);
    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                        "WdfUsbTargetDeviceSelectConfig failed %!STATUS! \n",
                        status);

        //
        // Since the Osr USB fx2 device is capable of working at high speed, the only reason
        // the device would not be working at high speed is if the port doesn't
        // support it. If the port doesn't support high speed it is a 1.1 port
        //
        if ((pDeviceContext->UsbDeviceTraits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) == 0) {
            GUID activity = DeviceToActivityId(Device);

            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                            " On a 1.1 USB port on Windows Vista"
                            " this is expected as the OSR USB Fx2 board's Interrupt EndPoint descriptor"
                            " doesn't conform to the USB specification. Windows Vista detects this and"
                            " returns an error. \n"
                            );
            EventWriteSelectConfigFailure(
                &activity,
                pDeviceContext->DeviceName,
                pDeviceContext->Location,
                status
                );
        }

        return status;
    }

    pDeviceContext->UsbInterface =
                configParams.Types.SingleInterface.ConfiguredUsbInterface;

    numberConfiguredPipes = configParams.Types.SingleInterface.NumberConfiguredPipes;

    //
    // Get pipe handles
    //
    for(index=0; index < numberConfiguredPipes; index++) {

        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);

        pipe = WdfUsbInterfaceGetConfiguredPipe(
            pDeviceContext->UsbInterface,
            index, //PipeIndex,
            &pipeInfo
            );
        //
        // Tell the framework that it's okay to read less than
        // MaximumPacketSize
        //
        WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

        if(WdfUsbPipeTypeInterrupt == pipeInfo.PipeType) {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "Interrupt Pipe is 0x%p\n", pipe);
            pDeviceContext->InterruptPipe = pipe;
        }

        if(WdfUsbPipeTypeBulk == pipeInfo.PipeType &&
                WdfUsbTargetPipeIsInEndpoint(pipe)) {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "BulkInput Pipe is 0x%p\n", pipe);
            pDeviceContext->BulkReadPipe = pipe;
        }

        if(WdfUsbPipeTypeBulk == pipeInfo.PipeType &&
                WdfUsbTargetPipeIsOutEndpoint(pipe)) {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "BulkOutput Pipe is 0x%p\n", pipe);
            pDeviceContext->BulkWritePipe = pipe;
        }

    }

    //
    // If we didn't find all the 3 pipes, fail the start.
    //
    if(!(pDeviceContext->BulkWritePipe
            && pDeviceContext->BulkReadPipe && pDeviceContext->InterruptPipe)) {
        status = STATUS_INVALID_DEVICE_STATE;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                            "Device is not configured properly %!STATUS!\n",
                            status);

        return status;
    }

    return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
GetDeviceEventLoggingNames(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    Retrieve the friendly name and the location string into WDFMEMORY objects
    and store them in the device context.

Arguments:

Return Value:

    None

--*/
{
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);

    WDF_OBJECT_ATTRIBUTES objectAttributes;

    WDFMEMORY deviceNameMemory = NULL;
    WDFMEMORY locationMemory = NULL;

    NTSTATUS status;

    PAGED_CODE();

    //
    // We want both memory objects to be children of the device so they will
    // be deleted automatically when the device is removed.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = Device;

    //
    // First get the length of the string. If the FriendlyName
    // is not there then get the lenght of device description.
    //

    status = WdfDeviceAllocAndQueryProperty(Device,
                                            DevicePropertyFriendlyName,
                                            NonPagedPool,
                                            &objectAttributes,
                                            &deviceNameMemory);

    if (!NT_SUCCESS(status))
    {
        status = WdfDeviceAllocAndQueryProperty(Device,
                                                DevicePropertyDeviceDescription,
                                                NonPagedPool,
                                                &objectAttributes,
                                                &deviceNameMemory);
    }

    if (NT_SUCCESS(status))
    {
        pDevContext->DeviceNameMemory = deviceNameMemory;
        pDevContext->DeviceName = WdfMemoryGetBuffer(deviceNameMemory, NULL);
    }
    else
    {
        pDevContext->DeviceNameMemory = NULL;
        pDevContext->DeviceName = L"(error retrieving name)";
    }

    //
    // Retrieve the device location string.
    //

    status = WdfDeviceAllocAndQueryProperty(Device,
                                            DevicePropertyLocationInformation,
                                            NonPagedPool,
                                            WDF_NO_OBJECT_ATTRIBUTES,
                                            &locationMemory);

    if (NT_SUCCESS(status))
    {
        pDevContext->LocationMemory = locationMemory;
        pDevContext->Location = WdfMemoryGetBuffer(locationMemory, NULL);
    }
    else
    {
        pDevContext->LocationMemory = NULL;
        pDevContext->Location = L"(error retrieving location)";
    }

    return;
}

_IRQL_requires_(PASSIVE_LEVEL)
PCHAR
DbgDevicePowerString(
    _In_ WDF_POWER_DEVICE_STATE Type
    )
{
    switch (Type)
    {
    case WdfPowerDeviceInvalid:
        return "WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return "WdfPowerDeviceD0";
    case WdfPowerDeviceD1:
        return "WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return "WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return "WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return "WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return "WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return "WdfPowerDeviceMaximum";
    default:
        return "UnKnown Device Power State";
    }
}



