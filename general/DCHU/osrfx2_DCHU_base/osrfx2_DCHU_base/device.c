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

    User mode only

--*/

#include "osrusbfx2.h"
#include <devpkey.h>

#if defined(EVENT_TRACING)
#include "Device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OsrFxEvtDeviceAdd)
#pragma alloc_text(PAGE, OsrFxEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, OsrFxEvtDeviceD0Exit)
#pragma alloc_text(PAGE, SelectInterfaces)
#pragma alloc_text(PAGE, OsrFxSetPowerPolicy)
#pragma alloc_text(PAGE, OsrFxReadFdoRegistryKeyValue)
#pragma alloc_text(PAGE, GetDeviceEventLoggingNames)
#endif


/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a Device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver     - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
NTSTATUS
OsrFxEvtDeviceAdd(
    _In_ WDFDRIVER       Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    WDFDEVICE Device;
    WDF_DEVICE_PNP_CAPABILITIES PnpCaps;
    WDF_IO_QUEUE_CONFIG IoQueueConfig;
    PDEVICE_CONTEXT DeviceContext;
    WDFQUEUE Queue;
    GUID Activity;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,"--> OsrFxEvtDeviceAdd routine\n");

    //
    // Initialize the PnpPowerCallbacks structure. Callback events for PNP
    // and Power are specified here. If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);

    //
    // For usb Devices, PrepareHardware callback is the to place select the
    // interface and configure the device.
    //
    PnpPowerCallbacks.EvtDevicePrepareHardware = OsrFxEvtDevicePrepareHardware;

    //
    // These two callbacks start and stop the wdfusb pipe continuous reader
    // as we go in and out of the D0-working state.
    //
    PnpPowerCallbacks.EvtDeviceD0Entry = OsrFxEvtDeviceD0Entry;
    PnpPowerCallbacks.EvtDeviceD0Exit = OsrFxEvtDeviceD0Exit;
    PnpPowerCallbacks.EvtDeviceSelfManagedIoFlush = OsrFxEvtDeviceSelfManagedIoFlush;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Now specify the size of the device extension where we track per-device
    // context. DeviceInit is completely initialized. So, call the framework
    // to create the device and attach it to the lower stack.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, DEVICE_CONTEXT);

    Status = WdfDeviceCreate(&DeviceInit, &Attributes, &Device);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceCreate failed with Status code %!STATUS!\n", Status);
        return Status;
    }

    //
    // Setup the Activity ID so that we can log events using it.
    //
    Activity = DeviceToActivityId(Device);

    //
    // Get the DeviceObject context by using accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for DEVICE_CONTEXT.
    //
    DeviceContext = GetDeviceContext(Device);

    //
    // Get the device's friendly name and location so that we can use it in
    // error logging. If this fails then it will setup dummy strings.
    //
    GetDeviceEventLoggingNames(Device);

    //
    // Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
    // that you don't get the popup in user-mode when you surprise remove the device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&PnpCaps);
    PnpCaps.SurpriseRemovalOK = WdfTrue;

    WdfDeviceSetPnpCapabilities(Device, &PnpCaps);

    //
    // Create a parallel default queue and register an event callback to
    // receive ioctl requests. We will create separate Queues for
    // handling read and write requests. All other requests will be
    // completed with error Status automatically by the framework.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IoQueueConfig,
                                           WdfIoQueueDispatchParallel);

    IoQueueConfig.EvtIoDeviceControl = OsrFxEvtIoDeviceControl;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it
    // doesn't find the EvtIoStop callback on a power-managed queue.
    // The 'assume' below causes SDV to suppress this warning. If the driver
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver. Normally
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests for a
    // long time or forward them to other drivers.
    //
    // If the EvtIoStop callback is not implemented, the framework waits for
    // all driver-owned requests to be done before moving into the Dx/sleep
    // states or before removing the device, which is the correct behavior
    // for this type of driver. If the requests were taking an indeterminate
    // amount of time to complete, or if the driver forwarded the requests
    // to a lower driver/another stack, the queue should have an
    // EvtIoStop/EvtIoResume.
    //
    __analysis_assume(IoQueueConfig.EvtIoStop != 0);

    Status = WdfIoQueueCreate(Device,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    __analysis_assume(IoQueueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfIoQueueCreate failed  %!STATUS!\n", Status);
        goto Error;
    }

    //
    // We will create a separate sequential queue and configure it
    // to receive read requests. We also need to register a EvtIoStop
    // handler so that we can acknowledge requests that are pending
    // at the target driver.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&IoQueueConfig, WdfIoQueueDispatchSequential);

    IoQueueConfig.EvtIoRead = OsrFxEvtIoRead;
    IoQueueConfig.EvtIoStop = OsrFxEvtIoStop;

    Status = WdfIoQueueCreate(Device,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS (Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfIoQueueCreate failed 0x%x\n", Status);
        goto Error;
    }

    Status = WdfDeviceConfigureRequestDispatching(Device,
                                                  Queue,
                                                  WdfRequestTypeRead);

    if (!NT_SUCCESS (Status))
    {
        assert(NT_SUCCESS(Status));

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceConfigureRequestDispatching failed 0x%x\n", Status);
        goto Error;
    }


    //
    // We will create another sequential queue and configure it
    // to receive write requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&IoQueueConfig, WdfIoQueueDispatchSequential);

    IoQueueConfig.EvtIoWrite = OsrFxEvtIoWrite;
    IoQueueConfig.EvtIoStop  = OsrFxEvtIoStop;

    Status = WdfIoQueueCreate(Device,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS (Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfIoQueueCreate failed 0x%x\n", Status);
        goto Error;
    }

    Status = WdfDeviceConfigureRequestDispatching(Device,
                                                  Queue,
                                                  WdfRequestTypeWrite);

    if (!NT_SUCCESS (Status))
    {
        assert(NT_SUCCESS(Status));

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceConfigureRequestDispatching failed 0x%x\n", Status);
        goto Error;
    }

    //
    // Register a manual I/O queue for handling Interrupt Message Read Requests.
    // This queue will be used for storing requests that need to wait for an
    // interrupt to occur before they can be completed.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&IoQueueConfig, WdfIoQueueDispatchManual);

    //
    // This queue is used for requests that don't directly access the device. The
    // requests in this queue are serviced only when the device is in a fully
    // powered state and sends an interrupt. So we can use a non-power managed
    // queue to park the requests since we dont care whether the device is idle
    // or fully powered up.
    //
    IoQueueConfig.PowerManaged = WdfFalse;

    Status = WdfIoQueueCreate(Device,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &DeviceContext->InterruptMsgQueue);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfIoQueueCreate failed 0x%x\n", Status);
        goto Error;
    }

    //
    // Register a device interface so that app can find our device and talk to it.
    //
    Status = WdfDeviceCreateDeviceInterface(Device,
                                            (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                            NULL // Reference string
                                            );

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceCreateDeviceInterface failed  %!STATUS!\n", Status);
        goto Error;
    }

#if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
    //
    // Adding Custom Capability:
    //
    // Adds a custom capability to the device interface instance that allows a Windows
    // Store device app to access this interface using Windows.Devices.Custom namespace.
    // This capability can be defined either in the INF or here as shown below. In order
    // to define it from the INF, uncomment the section "OsrUsb Interface installation"
    // from the INF and then remove the block of code below.
    //
    WDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData = { 0 };
    static const wchar_t customCapabilities[] = L"microsoft.hsaTestCustomCapability_q536wpkpf5cy2\0";

    WDF_DEVICE_INTERFACE_PROPERTY_DATA_INIT(&PropertyData,
                                            &GUID_DEVINTERFACE_OSRUSBFX2,
                                            &DEVPKEY_DeviceInterface_UnrestrictedAppCapabilities);

    Status = WdfDeviceAssignInterfaceProperty(Device,
                                              &PropertyData,
                                              DEVPROP_TYPE_STRING_LIST,
                                              sizeof(customCapabilities),
                                              (PVOID)customCapabilities);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceAssignInterfaceProperty failed  %!STATUS!\n", Status);
        goto Error;
    }
#endif

    //
    // Create the lock that we use to serialize calls to ResetDevice(). As an
    // alternative to using a WDFWAITLOCK to serialize the calls, a sequential
    // WDFQUEUE can be created and reset IOCTLs would be forwarded to it.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device;

    Status = WdfWaitLockCreate(&Attributes, &DeviceContext->ResetDeviceWaitLock);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfWaitLockCreate failed  %!STATUS!\n", Status);
        goto Error;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- OsrFxEvtDeviceAdd\n");

    return Status;

Error:

    //
    // Log failure to add the device.
    //
    EventWriteFailAddDevice(DeviceContext->DeviceName,
                            DeviceContext->Location,
                            Status);

    return Status;
}


/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

Arguments:

    Device                 - Handle to the device

    ResourceList           - Handle to a resource-list object that identifies the
                             raw hardware resources that the PnP manager assigned
                             to the Device

    ResourceListTranslated - Handle to a resource-list object that
                             identifies the translated hardware resources
                             that the PnP manager assigned to the device

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
NTSTATUS
OsrFxEvtDevicePrepareHardware(
    _In_ WDFDEVICE    Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
{
    NTSTATUS Status;
    PDEVICE_CONTEXT DeviceContext;
    WDF_USB_DEVICE_INFORMATION DeviceInfo;
    ULONG WaitWakeEnable;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    WaitWakeEnable = FALSE;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                "--> EvtDevicePrepareHardware\n");

    DeviceContext = GetDeviceContext(Device);

    //
    // Create a USB device handle so that we can communicate with the
    // underlying USB stack. The WDFUSBDEVICE handle is used to query,
    // configure, and manage all aspects of the USB device.
    // These aspects include device properties, bus properties,
    // and I/O creation and synchronization. We only create the device the
    // first time that PrepareHardware is called. If the device is restarted by
    // PnP Manager to rebalance resources, we will use the same device handle
    // but then select the interfaces again because the USB stack could
    // reconfigure the device on restart.
    //
    if (DeviceContext->UsbDevice == NULL)
    {
        Status = WdfUsbTargetDeviceCreate(Device,
                                          WDF_NO_OBJECT_ATTRIBUTES,
                                          &DeviceContext->UsbDevice);

        if (!NT_SUCCESS(Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                        "WdfUsbTargetDeviceCreate failed with status code %!STATUS!\n", Status);
            return Status;
        }
    }

    //
    // Retrieve USBD version information, port driver capabilites and device
    // capabilites such as speed, power, etc.
    //
    WDF_USB_DEVICE_INFORMATION_INIT(&DeviceInfo);

    Status = WdfUsbTargetDeviceRetrieveInformation(DeviceContext->UsbDevice,
                                                   &DeviceInfo);
    if (NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                    "IsDeviceHighSpeed: %s\n",
                    (DeviceInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) ? "TRUE" :
                                                                               "FALSE");

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                    "IsDeviceSelfPowered: %s\n",
                    (DeviceInfo.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED) ? "TRUE" :
                                                                              "FALSE");

        WaitWakeEnable = DeviceInfo.Traits &
                         WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                    "IsDeviceRemoteWakeable: %s\n",
                    WaitWakeEnable ? "TRUE" :
                                     "FALSE");
        //
        // Save these for use later.
        //
        DeviceContext->UsbDeviceTraits = DeviceInfo.Traits;
    }
    else
    {
        DeviceContext->UsbDeviceTraits = 0;
    }

    Status = SelectInterfaces(Device);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "SelectInterfaces failed 0x%x\n", Status);

        return Status;
    }

    //
    // Enable wait-wake and idle timeout if the device supports it.
    //
    if (WaitWakeEnable)
    {
        Status = OsrFxSetPowerPolicy(Device);

        if (!NT_SUCCESS (Status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                        "OsrFxSetPowerPolicy failed  %!STATUS!\n", Status);
            return Status;
        }
    }

    Status = OsrFxConfigContReaderForInterruptEndPoint(DeviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- EvtDevicePrepareHardware\n");

    return Status;
}


/*++

Routine Description:

    EvtDeviceD0Entry event callback must perform any operations that are
    necessary before the specified device is used.  It will be called every
    time the hardware needs to be (re-)initialized.

    This function is not marked pageable because this function is in the
    device power-up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

    This function runs at PASSIVE_LEVEL, even though it is not paged. A
    driver can optionally make this function pageable if DO_POWER_PAGABLE
    is set. Even if DO_POWER_PAGABLE isn't set, this function still runs
    at PASSIVE_LEVEL. In this case, though, the function absolutely must
    not do anything that will cause a page fault.

Arguments:

    Device        - Handle to a framework device object

    PreviousState - Device power state which the device was in most recently.
                    If the device is being newly started, this will be
                    PowerDeviceUnspecified.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
NTSTATUS
OsrFxEvtDeviceD0Entry(
    _In_ WDFDEVICE              Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    PDEVICE_CONTEXT DeviceContext;
    NTSTATUS Status;
    BOOLEAN IsTargetStarted;

    DeviceContext = GetDeviceContext(Device);
    IsTargetStarted = FALSE;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
                "-->OsrFxEvtEvtDeviceD0Entry - coming from %s\n",
                DbgDevicePowerString(PreviousState));

    //
    // Since continuous reader is configured for this interrupt-pipe, we must explicitly start
    // the I/O target to get the framework to post read requests.
    //
    Status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe));

    if (!NT_SUCCESS(Status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_POWER,
                    "Failed to start interrupt pipe %!STATUS!\n", Status);
        goto End;
    }

    IsTargetStarted = TRUE;

End:

    if (!NT_SUCCESS(Status))
    {
        //
        // Failure in D0Entry will lead to the device being removed. So stop
        // sthe continuous reader in preparation for the ensuing remove.
        //
        if (IsTargetStarted)
        {
            WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe),
                            WdfIoTargetCancelSentIo);
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "<--OsrFxEvtEvtDeviceD0Entry\n");

    return Status;
}


/*++

Routine Description:

    This routine undoes anything done in EvtDeviceD0Entry. It is called
    whenever the device leaves the D0 state, which happens when the device is
    stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.

    EvtDeviceD0Exit event callback must perform any operations that are
    necessary before the specified device is moved out of the D0 state. If the
    driver needs to save hardware state before the device is powered down, then
    that should be done here.

    This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
    driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

    Even if DO_POWER_PAGABLE isn't set, this function still runs at
    PASSIVE_LEVEL. In this case, though, the function absolutely must not do
    anything that will cause a page fault.

Arguments:

    Device      - Handle to a framework device object

    TargetState - Device power state which the device will be put in once this
                  callback is complete

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.
    
    If this function succeeds it implies that the device can be used.

--*/
NTSTATUS
OsrFxEvtDeviceD0Exit(
    _In_ WDFDEVICE              Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PDEVICE_CONTEXT DeviceContext;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
                "-->OsrFxEvtDeviceD0Exit - moving to %s\n",
                DbgDevicePowerString(TargetState));

    DeviceContext = GetDeviceContext(Device);

    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe), WdfIoTargetCancelSentIo);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
                "<--OsrFxEvtDeviceD0Exit\n");

    return STATUS_SUCCESS;
}


/*++

Routine Description:

    This routine handles flush Activity for the device's
    self-managed I/O operations.

Arguments:

    Device - Handle to a framework device object

Return Value:

    VOID

--*/
VOID
OsrFxEvtDeviceSelfManagedIoFlush(
    _In_ WDFDEVICE Device
    )
{
    //
    // Service the interrupt message queue to drain any outstanding
    // requests
    //
    OsrUsbIoctlGetInterruptMessage(Device, STATUS_DEVICE_REMOVED);
}


/*++

Routine Description:

    This routine sets the power policy for the device.

Arguments:

    Device - Handle to a framework device object

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
OsrFxSetPowerPolicy(
    _In_ WDFDEVICE Device
    )
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);

    idleSettings.IdleTimeout = 10000; // 10 seconds

    Status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    if ( !NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceSetPowerPolicyS0IdlePolicy failed %x\n", Status);
        return Status;
    }

    //
    // Init wait-wake policy structure.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

    Status = WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);

    if (!NT_SUCCESS(Status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfDeviceAssignSxWakeSettings failed %x\n", Status);
        return Status;
    }

    return Status;
}


/*++

Routine Description:

    This helper routine selects the configuration, interface and
    creates a context for every pipe (end point) in that interface.

Arguments:

    Device - Handle to a framework device object

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SelectInterfaces(
    _In_ WDFDEVICE Device
    )
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_CONTEXT DeviceContext;
    WDFUSBPIPE Pipe;
    WDF_USB_PIPE_INFORMATION PipeInfo;
    UCHAR Index;
    UCHAR NumberConfiguredPipes;
    WDFUSBINTERFACE UsbInterface;

    PAGED_CODE();

    DeviceContext = GetDeviceContext(Device);

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    UsbInterface = WdfUsbTargetDeviceGetInterface(DeviceContext->UsbDevice, 0);

    if (NULL == UsbInterface)
    {
       Status = STATUS_UNSUCCESSFUL;

       TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                   "WdfUsbTargetDeviceGetInterface 0 failed %!STATUS! \n",
                   Status);
       return Status;
    }

    configParams.Types.SingleInterface.ConfiguredUsbInterface = UsbInterface;

    configParams.Types.SingleInterface.NumberConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(UsbInterface);

    DeviceContext->UsbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;

    NumberConfiguredPipes = configParams.Types.SingleInterface.NumberConfiguredPipes;

    //
    // Get pipe handles.
    //
    for (Index = 0; Index < NumberConfiguredPipes; Index++)
    {
        WDF_USB_PIPE_INFORMATION_INIT(&PipeInfo);

        Pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                                Index,
                                                &PipeInfo);

        //
        // Tell the framework that it's okay to read less than
        // MaximumPacketSize.
        //
        WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(Pipe);

        if (WdfUsbPipeTypeInterrupt == PipeInfo.PipeType)
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                        "Interrupt Pipe is 0x%p\n", Pipe);
            DeviceContext->InterruptPipe = Pipe;
        }

        if ((WdfUsbPipeTypeBulk == PipeInfo.PipeType) &&
            (WdfUsbTargetPipeIsInEndpoint(Pipe)))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                        "BulkInput Pipe is 0x%p\n", Pipe);
            DeviceContext->BulkReadPipe = Pipe;
        }

        if ((WdfUsbPipeTypeBulk == PipeInfo.PipeType) &&
            (WdfUsbTargetPipeIsOutEndpoint(Pipe)))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                        "BulkOutput Pipe is 0x%p\n", Pipe);
            DeviceContext->BulkWritePipe = Pipe;
        }

    }

    //
    // If we didn't find all 3 pipes, fail the start.
    //
    if (!((DeviceContext->BulkWritePipe) &&
          ((DeviceContext->BulkReadPipe) &&
           (DeviceContext->InterruptPipe))))
    {
        Status = STATUS_INVALID_DEVICE_STATE;

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "Device is not configured properly %!STATUS!\n",
                    Status);

        return Status;
    }

    return Status;
}


/*++

Routine Description:

    Retrieve the friendly name and the location string into WDFMEMORY objects
    and store them in the device context.

Arguments:

    Device - Handle to a device framework object

Return Value:

    VOID

--*/
_IRQL_requires_(PASSIVE_LEVEL)
VOID
GetDeviceEventLoggingNames(
    _In_ WDFDEVICE Device
    )
{
    PDEVICE_CONTEXT DeviceContext = GetDeviceContext(Device);
    WDF_OBJECT_ATTRIBUTES ObjectAttributes;
    WDFMEMORY DeviceNameMemory = NULL;
    WDFMEMORY LocationMemory = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // We want both memory objects to be children of the device so that they
    // will be deleted automatically when the device is removed.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
    ObjectAttributes.ParentObject = Device;

    //
    // First get the length of the string. If the FriendlyName
    // is not there then get the length of the device's description.
    //
    Status = WdfDeviceAllocAndQueryProperty(Device,
                                            DevicePropertyFriendlyName,
                                            NonPagedPoolNx,
                                            &ObjectAttributes,
                                            &DeviceNameMemory);

    if (!NT_SUCCESS(Status))
    {
        Status = WdfDeviceAllocAndQueryProperty(Device,
                                                DevicePropertyDeviceDescription,
                                                NonPagedPoolNx,
                                                &ObjectAttributes,
                                                &DeviceNameMemory);
    }

    if (NT_SUCCESS(Status))
    {
        DeviceContext->DeviceNameMemory = DeviceNameMemory;
        DeviceContext->DeviceName = WdfMemoryGetBuffer(DeviceNameMemory, NULL);
    }
    else
    {
        DeviceContext->DeviceNameMemory = NULL;
        DeviceContext->DeviceName = L"(error retrieving name)";
    }

    //
    // Retrieve the device location string.
    //
    Status = WdfDeviceAllocAndQueryProperty(Device,
                                            DevicePropertyLocationInformation,
                                            NonPagedPoolNx,
                                            WDF_NO_OBJECT_ATTRIBUTES,
                                            &LocationMemory);

    if (NT_SUCCESS(Status))
    {
        DeviceContext->LocationMemory = LocationMemory;
        DeviceContext->Location = WdfMemoryGetBuffer(LocationMemory, NULL);
    }
    else
    {
        DeviceContext->LocationMemory = NULL;
        DeviceContext->Location = L"(error retrieving location)";
    }
}


/*++

Routine Description:

    Retrieve the correct string for a given power device state.

Arguments:

    Type - The device power state to turn into a string

Return Value:

    The name that corresponds to the WDF_POWER_DEVICE_STATE given, or
    "Unknown Device Power State" if none was found.

--*/
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
        return "Unknown Device Power State";
    }
}