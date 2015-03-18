/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WdfMultiComp.c

Abstract:
    This module implements a KMDF sample driver for a multi-component device.
    The driver uses the power framework to manage the power state of the
    components of its device.

    The device used in this sample is a root-enumerated device whose components
    are simulated entirely in software. The simulation of the components is 
    implemented in HwSim.h and HwSim.c.

    The driver's interaction with the power framework is encapsulated in a 
    separate library named WdfPoFx.lib. The driver statically links to this 
    library.

Environment:

    Kernel mode

--*/

#include "WdfMultiComp.h"
#include "HwSim.h"
#include "WdfMultiComp.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, MCompEvtDriverCleanup)
#pragma alloc_text (PAGE, MCompEvtDeviceAdd)
#pragma alloc_text (PAGE, MCompEvtDeviceD0Exit)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Driver initialization entry point. This entry point is called directly by 
    the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path to the 
                   driver-specific key in the registry.

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    WDF_DRIVER_CONFIG   config;
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS            status;
    WDFDRIVER           hDriver;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Create a framework driver object to represent our driver.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = MCompEvtDriverCleanup;
    WDF_DRIVER_CONFIG_INIT(
        &config,
        MCompEvtDeviceAdd
        );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &hDriver);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDriverCreate failed with %!status!", 
              status);
        WPP_CLEANUP(DriverObject);
    }
    
    return status;
}

VOID
MCompEvtDriverCleanup(
    _In_ WDFOBJECT Driver
    )
/*++

Routine Description:

    This routine is invoked when the framework driver object that was created in
    DriverEntry is about to be deleted.

Arguments:

    Driver - Handle to the framework driver object created in DriverEntry

Return Value:

    None
    
--*/
{
    PAGED_CODE();

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER) Driver));
}

NTSTATUS
InitializePowerFrameworkSettings(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    This routine initializes the power framework helper.

Arguments:

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    PO_FX_DEVICE_MCOMP_EXT poFxDevice;
    PPO_FX_DEVICE poFxDevicePtr;
    PO_FX_COMPONENT_IDLE_STATE idleState[2]; // F0 and F1 only
    ULONG i;
    
    RtlZeroMemory(&poFxDevice, sizeof(poFxDevice));
    RtlZeroMemory(idleState, sizeof(idleState));
    
    //
    // Specify callbacks and context
    //
    poFxDevicePtr = (PPO_FX_DEVICE) &poFxDevice;
    poFxDevicePtr->Version = PO_FX_VERSION_V1;
    poFxDevicePtr->ComponentIdleStateCallback =
                        MCompComponentIdleStateCallback;
    poFxDevicePtr->ComponentActiveConditionCallback = NULL;
    poFxDevicePtr->ComponentIdleConditionCallback = NULL;
    poFxDevicePtr->DevicePowerRequiredCallback = NULL;
    poFxDevicePtr->DevicePowerNotRequiredCallback = NULL;
    poFxDevicePtr->DeviceContext = Device;
    poFxDevicePtr->ComponentCount = COMPONENT_COUNT;

    //
    // Initialize F-state-related information
    // NOTE: We're making the F1 specification be the same for all components, 
    // but it does not have to be that way. 
    //
    
    //
    // Transition latency should be 0 for F0
    //
    idleState[0].TransitionLatency = 0;
    
    //
    // Residency requirement should be 0 for F0
    //
    idleState[0].ResidencyRequirement = 0;

    //
    // Nominal power for F0
    //
    idleState[0].NominalPower = PO_FX_UNKNOWN_POWER;

    //
    // Transition latency for F1
    //
    idleState[1].TransitionLatency = TRANSITION_LATENCY_F1;
    
    //
    // Residency requirement for F1
    //
    idleState[1].ResidencyRequirement = RESIDENCY_REQUIREMENT_F1;

    //
    // Nominal power for F1
    //
    idleState[1].NominalPower = PO_FX_UNKNOWN_POWER;
    
    for (i=0; i < COMPONENT_COUNT; i++) {
        //
        // We're declaring that each of components support only F0 and F1
        //
        poFxDevicePtr->Components[i].IdleStateCount = 2;
        
        //
        // Specify the F-state information
        //
        poFxDevicePtr->Components[i].IdleStates = idleState;
    }

    //
    // Provide the power framework settings to the power framework helper
    //
    status = PfhInitializePowerFrameworkSettings(Device, poFxDevicePtr);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    status = STATUS_SUCCESS;

exit:
    return status;
}

NTSTATUS
CreateDevice(
    _In_ WDFOBJECT PfhInitializer,
    _In_ PWDFDEVICE_INIT  DeviceInit,
    _Out_ WDFDEVICE * Device
    )
/*++
Routine Description:

    This routine creates the KMDF device object and also initializes the power
    framework helper's settings for this device object.

Arguments:

    PfhInitializer - Handle to the initializer object for initializing the power
        framework helper.

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

    Device - Handle to the KMDF device object created by this routine.

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDFDEVICE device = NULL;    
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS s0IdleSettings;

    //
    // Register the PNP/power callbacks
    //
    // Note: This is the function driver for the device, so the framework 
    // automatically enables power policy ownership.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDeviceD0Entry = MCompEvtDeviceD0Entry;
    pnpCallbacks.EvtDeviceD0Exit = MCompEvtDeviceD0Exit;

    //
    // Get the power framework helper to override our PNP/power callbacks
    //
    PfhInterceptWdfPnpPowerEventCallbacks(PfhInitializer, &pnpCallbacks);
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    //
    // Give the power framework helper an opportunity to register WDM IRP 
    // preprocess callbacks for power IRPs
    //
    status = PfhAssignWdmPowerIrpPreProcessCallback(
                                        PfhInitializer,
                                        DeviceInit,
                                        NULL, // EvtDeviceWdmPowerIrpPreprocess
                                        NULL, // MinorFunctions
                                        0 // NumMinorFunctions
                                        );
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Specify the context for the device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, 
                                            DEVICE_EXTENSION);

    //
    // Create a framework device object
    //
    status = WdfDeviceCreate(&DeviceInit, &objectAttributes, &device);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceCreate failed with %!status!", 
              status);
        goto exit;
    }

    //
    // Specify the POHANDLE availability callbacks
    //
    PfhSetPoHandleAvailabilityCallbacks(PfhInitializer,
                                        MCompPoHandleAvailable,
                                        MCompPoHandleUnavailable);

    //
    // Enable S0-idle power management
    //
    // Note:
    //   * We set IdleTimeoutType = DriverManagedIdleTimeout to make sure KMDF 
    //     doesn't register with the power framework on our behalf, because we  
    //     will be directly registering with the power framework ourselves.
    //   * We set PowerUpIdleDeviceOnSystemWake = TRUE because when we register
    //     with power framework, we'd like to return to D0 immediately after the
    //     system returns to S0. It is a power framework requirement that once
    //     the system returns to S0, then device must return to D0 and the 
    //     driver must invoke PoFxReportDevicePoweredOn (the power framework 
    //     helper library does this on our behalf).
    //   * We pick a really small idle timeout value so that KMDF can power down
    //     the device almost immediately after the power framework allows it to
    //     do so.
    //
    // If S0-idle power management support is not required, then the following
    // modifications should be made:
    //   1. Recompile the power framework helper library with the value of the
    //      WDFPOFX_S0IDLE_SUPPORTED compiler switch set to 0. This omits the 
    //      code that is specific to S0-idle power management, thereby reducing
    //      the size of the library.
    //   2. Remove the calls to PfhSetS0IdleConfiguration and 
    //      WdfDeviceAssignS0IdleSettings below.
    //
    PfhSetS0IdleConfiguration(PfhInitializer, PfhS0IdleSupportedPowerPageable);

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&s0IdleSettings, 
                                               IdleCannotWakeFromS0);
    s0IdleSettings.IdleTimeoutType = DriverManagedIdleTimeout; 
    s0IdleSettings.PowerUpIdleDeviceOnSystemWake = WdfTrue;
    s0IdleSettings.IdleTimeout = 1; // 1 millisecond
    status = WdfDeviceAssignS0IdleSettings(device, &s0IdleSettings);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceAssignS0IdleSettings failed with %!status!", 
              status);
        goto exit;
    }

    //
    // Get the power framework helper to initialize its settings for this device
    //
    status = PfhInitializeDeviceSettings(device, PfhInitializer);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    *Device = device;
    status = STATUS_SUCCESS;
    
exit:
    return status;
}

NTSTATUS
CreateComponentQueues(
    _In_ WDFOBJECT PfhInitializer,
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    This routine creates the component queues and also initializes the power
    framework helper's settings for each of them.

Arguments:

    PfhInitializer - Handle to the initializer object for initializing the power
        framework helper.

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION devExt;
    ULONG i;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;

    //
    // Get the device extension
    //
    devExt = DeviceGetData(Device);

    for (i=0; i < COMPONENT_COUNT; i++) {
        
        //
        // Create the component-specific queues
        //
        WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchParallel);
        ioQueueConfig.EvtIoDeviceControl = MCompEvtIoDeviceControlSecondary;

        //
        // Get the power framework helper to override the configuration for our
        // component-specific queue 
        //
        PfhInterceptComponentQueueConfig(PfhInitializer, &ioQueueConfig);
        
        //
        // Associate the queue with a component
        //
        PfhSetComponentForComponentQueue(PfhInitializer, i);
        
        //
        // Create the queue
        //

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
        __analysis_assume(ioQueueConfig.EvtIoStop != 0);
        status = WdfIoQueueCreate(Device,
                                  &ioQueueConfig,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  &(devExt->Queue[i]));
        __analysis_assume(ioQueueConfig.EvtIoStop == 0);
        
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - WdfIoQueueCreate failed with %!status! when "
                  "creating queue for component %d", 
                  status,
                  i);
            goto exit;
        }

        //
        // Get the power framework helper to initialize its settings for this
        // component-specific queue
        //
        status = PfhInitializeComponentQueueSettings(devExt->Queue[i], 
                                                     PfhInitializer);
        if (FALSE == NT_SUCCESS(status)) {
            goto exit;
        }
    }

    status = STATUS_SUCCESS;

exit:
    return status;
}

NTSTATUS
MCompEvtDeviceAdd(
    _In_ WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice call from 
    the PnP manager. 

Arguments:

    Driver - Handle to the framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    WDFOBJECT pfhInitializer = NULL;
    WDFDEVICE device = NULL;    
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDFQUEUE sourceQueue = NULL;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    //
    // Create an initializer object for the power framework helper
    //
    status = PfhInitializerCreate(&pfhInitializer);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Create the device object
    //
    status = CreateDevice(pfhInitializer, DeviceInit, &device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Create the source queue.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                           WdfIoQueueDispatchParallel);
    ioQueueConfig.EvtIoDeviceControl = MCompEvtIoDeviceControlPrimary;

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
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &sourceQueue);
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfIoQueueCreate failed with %!status! when creating "
              "source queue.", 
              status);
        goto exit;
    }

    //
    // Initialize power framework settings
    //
    status = InitializePowerFrameworkSettings(device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Create the component queues
    //
    status = CreateComponentQueues(pfhInitializer, device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }
    
    //
    // Create a device interface so that applications can open a handle to this
    // device.
    //
    status = WdfDeviceCreateDeviceInterface(device, 
                                            &GUID_DEVINTERFACE_POWERFX,
                                            NULL /* ReferenceString */);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceCreateDeviceInterface failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // Initialize the hardware simulator
    //
    status = HwSimInitialize(device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    status = STATUS_SUCCESS;
    
exit:
    if (NULL != pfhInitializer) {
        WdfObjectDelete(pfhInitializer);
    }
    return status;
}

NTSTATUS
MCompPoHandleAvailable(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    )
{
    PDEVICE_EXTENSION devExt = NULL;
    ULONG i;
    
    //
    // Get the device extension
    //
    devExt = DeviceGetData(Device);

    //
    // Save the POHANDLE
    //
    devExt->PoHandle = PoHandle;

    //
    // Provide latency and residency hints to enable the power framework to 
    // select lower-powered F-states when we are idle.
    //
    for (i=0; i < COMPONENT_COUNT; i++) {
        PoFxSetComponentLatency(
            PoHandle,
            i,
            (TRANSITION_LATENCY_F1 + 1)
            );
        PoFxSetComponentResidency(
            PoHandle,
            i,
            (RESIDENCY_REQUIREMENT_F1 + 1)
            );
    }
    
    return STATUS_SUCCESS;
}

VOID
MCompPoHandleUnavailable(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    )
{
    PDEVICE_EXTENSION devExt = NULL;
    
    //
    // Get the device extension
    //
    devExt = DeviceGetData(Device);

    //
    // Clear the POHANDLE. It is not guaranteed to remain valid after we return 
    // from this callback.
    //
    devExt->PoHandle = PoHandle;

    return;
}

NTSTATUS
MCompEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++
Routine Description:

    In this routine the driver enters D0

Arguments:

    Device - Handle to the framework device object

    PreviousState - Previous device power state

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    UNREFERENCED_PARAMETER(PreviousState);
    
    HwSimD0Entry(Device);
    
    return STATUS_SUCCESS;
}

NTSTATUS
MCompEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++
Routine Description:

    In this routine the driver leaves D0

Arguments:

    Device - Handle to the framework device object

    TargetState - Device power state that the device is about to enter

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(TargetState);

    HwSimD0Exit(Device);

    return STATUS_SUCCESS;
}

VOID
MCompComponentIdleStateCallback(
    _In_ PVOID Context,
    _In_ ULONG Component,
    _In_ ULONG State
    )
/*++
Routine Description:

    The power framework helper invokes this routine to change the F-state of one
    of our components.

Arguments:

    Context - Context that we passed in to our power framework helper module
      when we asked it to register with the power framework helper on our behalf
      
    Component - Index of component for which the F-state change is to be made
    
    State - The new F-state to transition the component to

Return Value:

    None
    
--*/
{
    PDEVICE_EXTENSION devExt = NULL;
    WDFDEVICE device = NULL;

    //
    // Get the device object handle
    //
    device = (WDFDEVICE) Context;
    
    //
    // Get the device extension
    //
    devExt = DeviceGetData(device);
    
    //
    // Change the F-state of the component
    // This includes hardware-specific operations such as:
    //   * disabling/enabling DMA capabilities associated with the component
    //   * disabling/enabling interrupts associated with the component 
    //   * saving/restoring component state
    //
    HwSimFStateChange(device, Component, State);
    
    //
    // F-state transition complete
    //
    PoFxCompleteIdleState(devExt->PoHandle, Component);
    return;
}

VOID
MCompEvtIoDeviceControlPrimary(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++
Routine Description:

    This routine is the IOCTL handler for the device's primary queue

Arguments:

    Queue - Handle to the framework queue object for the primary queue

    Request - Handle to the framework request object for IOCTL being dispatched

    OutputBufferLength - Output buffer length for IOCTL

    InputBufferLength - Input buffer length for IOCTL

    IoControlCode - IOCTL code

Return Value:

    None

--*/
{
    NTSTATUS status;
    PPOWERFX_READ_COMPONENT_INPUT inputBuffer = NULL;
    WDFDEVICE device = NULL;
    PDEVICE_EXTENSION devExt = NULL;
    WDFQUEUE componentQueue = NULL;
    ULONG component;
    
    UNREFERENCED_PARAMETER(OutputBufferLength);

    switch (IoControlCode) {
        case IOCTL_POWERFX_READ_COMPONENT:
        {
            //
            // Validate input buffer length
            //
            if (InputBufferLength != sizeof(*inputBuffer)) {
                status = STATUS_INVALID_PARAMETER;
                Trace(TRACE_LEVEL_ERROR, 
                      "%!FUNC! -Invalid input buffer size. Expected: %d. "
                      "Actual: %I64u. %!status!.", 
                      sizeof(*inputBuffer),
                      InputBufferLength,
                      status);
                goto exit;
            }
            
            //
            // Identify the component that we need to read in order to satisfy 
            // this request
            //
            status = WdfRequestRetrieveInputBuffer(Request,
                                                   sizeof(*inputBuffer),
                                                   (PVOID*) &inputBuffer,
                                                   NULL // Length
                                                   );
            if (FALSE == NT_SUCCESS(status)) {
                Trace(TRACE_LEVEL_ERROR, 
                      "%!FUNC! - WdfRequestRetrieveInputBuffer failed with "
                      "%!status!.", 
                      status);
                goto exit;
            }
            component = inputBuffer->ComponentNumber;
            
            //
            // Identify the queue corresponding to the above component
            //
            device = WdfIoQueueGetDevice(Queue);
            devExt = DeviceGetData(device);
            componentQueue = devExt->Queue[component];
        }
        break;

        default:
        {
            status = STATUS_INVALID_PARAMETER;
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unexpected IOCTL code %d. %!status!.", 
                  IoControlCode,
                  status);
            goto exit;
        }
    }

    //
    // Forward the request to the appropriate queue. We do this via the power
    // framework helper so that it can take a power reference on behalf of the
    // request.
    //
    PfhForwardRequestToQueue(Request, componentQueue);

    status = STATUS_SUCCESS;
    
exit:
    if (FALSE == NT_SUCCESS(status)) {
        //
        // Complete the request in case of failure. In case of success, we would
        // have dispatched the request to the component-specific queue, so we
        // don't need to complete it here.
        //
        PfhCompleteRequest(Request, status, 0 /* Information */);
    }
    
    return;
}

VOID
MCompEvtIoDeviceControlSecondary(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++
Routine Description:

    This routine is the IOCTL handler for the device's secondary queue (i.e.
    component-specific queue).

Arguments:

    Queue - Handle to the framework queue object for the secondary queue

    Request - Handle to the framework request object for IOCTL being dispatched

    OutputBufferLength - Output buffer length for IOCTL

    InputBufferLength - Input buffer length for IOCTL

    IoControlCode - IOCTL code

Return Value:

    None

--*/
{
    NTSTATUS status;
    PPOWERFX_READ_COMPONENT_INPUT inputBuffer = NULL;
    PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer = NULL;
    WDFDEVICE device = NULL;
    ULONG component;
    ULONG componentData;
    ULONG_PTR information = 0;

    //
    // When we complete the request, make sure we don't get the I/O manager to 
    // copy any more data to the client address space than what we write to the
    // output buffer. The only data that we write to the output buffer is the 
    // component data and the C_ASSERT below ensures that the output buffer does
    // not have room to contain anything other than that.
    //
    C_ASSERT(sizeof(componentData) == sizeof(*outputBuffer));
    
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);
    
    //
    // The following ASSERTMSG checks should have already been made by the IOCTL
    // dispatch routine for the primary queue. That routine should have 
    // completed the request with failure if the checks had failed. Therefore we
    // only use ASSERTMSG here.
    //
    ASSERTMSG("Unexpected IOCTL code\n", 
              IOCTL_POWERFX_READ_COMPONENT == IoControlCode);
    ASSERTMSG("Invalid input buffer size", 
              InputBufferLength == sizeof(*inputBuffer));

    //
    // Validate output buffer length
    //
    if (OutputBufferLength != sizeof(*outputBuffer)) {
        status = STATUS_INVALID_PARAMETER;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! -Invalid input buffer size. Expected: %d. Actual: %I64u."
              " %!status!.", 
              sizeof(*outputBuffer),
              OutputBufferLength,
              status);
        goto exit;
    }
    
    //
    // Identify the component that we need to read in order to satisfy 
    // this request.
    //
    status = WdfRequestRetrieveInputBuffer(Request,
                                           sizeof(*inputBuffer),
                                           (PVOID*) &inputBuffer,
                                           NULL // Length
                                           );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfRequestRetrieveInputBuffer failed with %!status!.",
              status);
        goto exit;
    }
    component = inputBuffer->ComponentNumber;

    //
    // Get the output buffer
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(*outputBuffer),
                                            (PVOID*) &outputBuffer,
                                            NULL // Length
                                            );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfRequestRetrieveOutputBuffer failed with %!status!.",
              status);
        goto exit;
    }
    
    //
    // Read the data from the component
    //
    device = WdfIoQueueGetDevice(Queue);
    componentData = HwSimReadComponent(device, component);
    outputBuffer->ComponentData = componentData;
    information = sizeof(*outputBuffer);
    
    status = STATUS_SUCCESS;
    
exit:
    //
    // Complete the request
    //
    PfhCompleteRequest(Request, status, information);
    return;
}
