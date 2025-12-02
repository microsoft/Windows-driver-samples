/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:
    This module implements a KMDF sample driver for a single-component device.
    The driver uses the power framework to manage the power state of the
    component that represents device.

    The device used in this sample is a root-enumerated device whose components
    are simulated entirely in software. The simulation of the components is 
    implemented in HwSim.h and HwSim.c.

    This driver works only on Win8 and above.

Environment:

    Kernel mode

--*/

#include "include.h"
#include "hwsim.h"

#include <initguid.h>
#include "AppInterface.h"

#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SingleCompEvtDriverCleanup)
#pragma alloc_text (PAGE, SingleCompEvtDeviceAdd)
#pragma alloc_text (PAGE, SingleCompEvtDeviceD0Exit)
#pragma alloc_text (PAGE, AssignS0IdleSettings)
#pragma alloc_text (PAGE, AssignPowerFrameworkSettings)
#pragma alloc_text (PAGE, SingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice)
#pragma alloc_text (PAGE, SingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice)
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
    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;
    WDF_OBJECT_ATTRIBUTES attributes;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Initialize driver config to control the attributes that are global to
    // the driver. Note that framework by default provides a driver unload 
    // routine. If DriverEntry creates any resources that require clean-up in
    // driver unload, you can manually override the default by supplying a 
    // pointer to the EvtDriverUnload callback in the config structure. In 
    // general xxx_CONFIG_INIT macros are provided to initialize most commonly
    // used members.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = SingleCompEvtDriverCleanup;
    WDF_DRIVER_CONFIG_INIT(
        &config,
        SingleCompEvtDeviceAdd
        );

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes, // Driver Attributes
                             &config,     // Driver Config Info
                             WDF_NO_HANDLE
                             );

    if (FALSE == NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
        WPP_CLEANUP(DriverObject);
    }

    return status;
}

VOID
SingleCompEvtDriverCleanup(
    _In_ WDFOBJECT Driver
    )
{
    PAGED_CODE();
    
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER) Driver));
}

NTSTATUS
SingleCompEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the KMDF in response to AddDevice call from 
    the PnP manager. 

Arguments:

    Driver - Handle to the KMDF driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG     queueConfig;
    FDO_DATA               *fdoContext = NULL;
    ULONG                   queueIndex = 0;
    WDF_OBJECT_ATTRIBUTES   objectAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, FDO_DATA);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDeviceD0Entry = SingleCompEvtDeviceD0Entry;
    pnpCallbacks.EvtDeviceD0Exit = SingleCompEvtDeviceD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
    
    status = WdfDeviceCreate(&DeviceInit, &objectAttributes, &device);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceCreate failed with %!status!.", 
              status);
        goto exit;
    }

    fdoContext = FdoGetContext(device);

    //
    // Our initial state is active
    //
    fdoContext->IsActive = TRUE;

    //
    // Create three power-managed queues, one each for read, write and IOCTL 
    // requests. The handles to these power-managed queues are stored in an 
    // array in the device object context space. When the component becomes idle
    // we need to stop our power-managed queues. When the component becomes 
    // active we need to start them. In those situations, we go through this 
    // array of power-managed queues and stop or start each queue as 
    // appropriate. Handles to non-power-managed queues should not be stored in
    // this array.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, 
                                           WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = SingleCompEvtIoDeviceControl;

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
                              &(fdoContext->Queues[queueIndex]));
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfIoQueueCreate for IoDeviceControl failed with %!status!.", 
              status);
        goto exit;
    }

    status = WdfDeviceConfigureRequestDispatching(device,
                                                  fdoContext->Queues[queueIndex],
                                                  WdfRequestTypeDeviceControl);
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceConfigureRequestDispatching for "
              "WdfRequestTypeDeviceControl failed with %!status!.", 
              status);
        goto exit;
    }

    ++queueIndex;
    
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, 
                                           WdfIoQueueDispatchParallel);
    queueConfig.EvtIoRead = SingleCompEvtIoRead;

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
                              &(fdoContext->Queues[queueIndex]));
    __analysis_assume(queueConfig.EvtIoStop == 0);
    
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfIoQueueCreate for IoRead failed with %!status!.", 
              status);
        goto exit;
    }

    status = WdfDeviceConfigureRequestDispatching(device,
                                                  fdoContext->Queues[queueIndex],
                                                  WdfRequestTypeRead);
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceConfigureRequestDispatching for "
              "WdfRequestTypeRead failed with %!status!.", 
              status);
        goto exit;
    }

    ++queueIndex;

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, 
                                           WdfIoQueueDispatchParallel);
    queueConfig.EvtIoWrite = SingleCompEvtIoWrite;

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
                              &(fdoContext->Queues[queueIndex]));
    __analysis_assume(queueConfig.EvtIoStop == 0);
    
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfIoQueueCreate for IoWrite failed with %!status!.", 
              status);
        goto exit;
    }

    status = WdfDeviceConfigureRequestDispatching(device,
                                                  fdoContext->Queues[queueIndex],
                                                  WdfRequestTypeWrite);
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceConfigureRequestDispatching for "
              "WdfRequestTypeWrite failed with %!status!.", 
              status);
        goto exit;
    }

    ++queueIndex;

    ASSERT(queueIndex == QUEUE_COUNT);

    status = AssignS0IdleSettings(device);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // If you need to talk to hardware to figure out what F-states are 
    // applicable this can be done in EvtSelfManagedIoInit 
    // (but no later than that). EvtSelfManagedIoInit gets invoked after
    // EvtPrepareHardware so you'd have chance to initialize your hardware.
    //
    status = AssignPowerFrameworkSettings(device);
    if (!NT_SUCCESS(status)) {
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
exit:
    return status;
}

NTSTATUS
SingleCompEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++
Routine Description:

    KMDF calls this routine when the device has entered D0.

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
SingleCompEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++
Routine Description:

    KMDF calls this routine when the device is about to leave D0.

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

NTSTATUS
SingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    )
/*++
Routine Description:

    KMDF calls this routine after it has registered with the Power Framework
    and supplies the registration handle that driver can use directly.

Arguments:

    Device - Handle to the framework device object

    PoHandle - Handle of registration with Power Framework.

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    FDO_DATA *fdoContext = NULL;

    PAGED_CODE();
    
    //
    // Get the device context
    //
    fdoContext = FdoGetContext(Device);

    //
    // Save the POHANDLE
    //
    fdoContext->PoHandle = PoHandle;

    //
    // Set latency and residency hints so that the power framework chooses lower
    // powered F-states when we are idle.
    // The values used here are for illustration purposes only. The driver 
    // should use values that are appropriate for its device.
    //
    PoFxSetComponentLatency(
        PoHandle,
        0, // Component
        (WDF_ABS_TIMEOUT_IN_MS(DEEPEST_FSTATE_LATENCY_IN_MS) + 1)
        );
    PoFxSetComponentResidency(
        PoHandle,
        0, // Component
        (WDF_ABS_TIMEOUT_IN_SEC(DEEPEST_FSTATE_RESIDENCY_IN_SEC) + 1)
        );

    return STATUS_SUCCESS;
}

VOID
SingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    )
/*++
Routine Description:

    KMDF calls this routine when it is about to unregister with the Power
    Framework. After returning from this routine driver must not use the
    supplied registration handle anymore.

Arguments:

    Device - Handle to the framework device object

    PoHandle - Handle of registration with Power Framework.

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    FDO_DATA *fdoContext = NULL;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(PoHandle);

    //
    // Get the device context
    //
    fdoContext = FdoGetContext(Device);

    //
    // Reset the POHANDLE
    //
    fdoContext->PoHandle = NULL;

    return;
}

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
AssignS0IdleSettings(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    Helper function to assign S0 idle settings for the device

Arguments:

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    NTSTATUS status;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS powerPolicy;

    PAGED_CODE();

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&powerPolicy, 
                                               IdleCannotWakeFromS0);
    powerPolicy.IdleTimeoutType = SystemManagedIdleTimeout;

    status = WdfDeviceAssignS0IdleSettings(Device, &powerPolicy);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceAssignS0IdleSettings failed with %!status!.", 
              status);
    }
    return status;
}

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
AssignPowerFrameworkSettings(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    Helper function to assign Power Framework related settings for the device

Arguments:

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    NTSTATUS status;
    WDF_POWER_FRAMEWORK_SETTINGS poFxSettings;
    PO_FX_COMPONENT component;
    PO_FX_COMPONENT_IDLE_STATE idleStates[FSTATE_COUNT];
    
    //
    // Note that we initialize the 'idleStates' array below based on the
    // assumption that MAX_FSTATE_COUNT is 4.
    // If we increase the value of MAX_FSTATE_COUNT, we need to initialize those
    // additional F-states below. If we decrease the value of MAX_FSTATE_COUNT,
    // we need to remove the corresponding initializations below.
    //
    C_ASSERT(FSTATE_COUNT == 4);
    
    PAGED_CODE();
    
    //
    // Initialization
    //
    RtlZeroMemory(&component, sizeof(component));
    RtlZeroMemory(idleStates, sizeof(idleStates));

    //
    // The transition latency and residency requirement values used here are for
    // illustration purposes only. The driver should use values that are 
    // appropriate for its device.
    //

    //
    // F0
    //
    idleStates[0].TransitionLatency = 0;    
    idleStates[0].ResidencyRequirement = 0;    
    idleStates[0].NominalPower = 0;    

    //
    // F1
    //
    idleStates[1].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(200);
    idleStates[1].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(3);   
    idleStates[1].NominalPower = 0;    

    //
    // F2
    //
    idleStates[2].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(400);    
    idleStates[2].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(6);    
    idleStates[2].NominalPower = 0;    

    //
    // F3
    //
    idleStates[3].TransitionLatency = 
        WDF_ABS_TIMEOUT_IN_MS(DEEPEST_FSTATE_LATENCY_IN_MS); 
    idleStates[3].ResidencyRequirement = 
        WDF_ABS_TIMEOUT_IN_SEC(DEEPEST_FSTATE_RESIDENCY_IN_SEC);
    idleStates[3].NominalPower = 0;

    //
    // Component 0 (the only component)
    //
    component.IdleStateCount = FSTATE_COUNT;
    component.IdleStates = idleStates;

    WDF_POWER_FRAMEWORK_SETTINGS_INIT(&poFxSettings);

    poFxSettings.EvtDeviceWdmPostPoFxRegisterDevice = 
                        SingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice;
    poFxSettings.EvtDeviceWdmPrePoFxUnregisterDevice =
                        SingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice;

    poFxSettings.Component = &component;
    poFxSettings.ComponentActiveConditionCallback = 
                        SingleCompWdmActiveConditionCallback;
    poFxSettings.ComponentIdleConditionCallback = 
                        SingleCompWdmIdleConditionCallback;
    poFxSettings.ComponentIdleStateCallback = 
                        SingleCompWdmIdleStateCallback;
    poFxSettings.PoFxDeviceContext = (PVOID) Device;
    
    status = WdfDeviceWdmAssignPowerFrameworkSettings(Device, &poFxSettings);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceWdmAssignPowerFrameworkSettings failed with "
              "%!status!.", 
              status);
    }
    return status;
}

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
F0Entry(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    Helper function invoked when component (representing the whole device) is
    requested to enter F0.

Arguments:

    Device - Handle to the framework device object

Return Value:

    BOOLEAN indicating whether F0 transition has completed.
    
--*/
{
    //
    // Change the F-state of the component
    // This includes hardware-specific operations such as:
    //   * enabling DMA capabilities associated with the component
    //   * enabling interrupts associated with the component 
    //   * restoring component state
    //
    //
    HwSimFStateChange(Device, 0);
    return TRUE;
}

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
F0Exit(
    _In_ WDFDEVICE Device,
    _In_ ULONG State
    )
/*++
Routine Description:

    Helper function invoked when component (representing the whole device) is
    requested to exit F0.

Arguments:

    Device - Handle to the framework device object
    State - The new F-state to be entered

Return Value:

    BOOLEAN indicating whether Fx transition has completed.
    
--*/
{
    //
    // Change the F-state of the component
    // This includes hardware-specific operations such as:
    //   * disabling DMA capabilities associated with the component
    //   * disabling interrupts associated with the component 
    //   * saving component state
    //
    HwSimFStateChange(Device, State);
    return TRUE;
}

VOID
SingleCompWdmIdleStateCallback(
    _In_ PVOID Context,
    _In_ ULONG Component,
    _In_ ULONG State
    )
/*++
Routine Description:

    This callback is invoked by Power Framework to notify driver about any
    F-state transition.

Arguments:

    Context - Context supplied to Power Framework. KMDF supplies WDFDEVICE as
              the context while registering with Power Framework. Hence Context
              contains the KMDF device object.
              
    Component - Component for which F state transition is requested. Since we
                have only one component this value is always 0.
                
Return Value:

    BOOLEAN indicating whether Fx transition has completed.
    
--*/
{
    WDFDEVICE device = NULL;
    FDO_DATA *fdoContext = NULL;
    BOOLEAN transitionComplete = TRUE;

    //
    // We have only component 0
    //
    if (0 != Component) {
        Trace(TRACE_LEVEL_ERROR,"%!FUNC! - Unexpected component %d",Component);
        ASSERT(FALSE);
    }

    //
    // Get the device
    //
    device = (WDFDEVICE) Context;
    
    //
    // Get the device context
    //
    fdoContext = FdoGetContext(device);

    //
    // Note the new F-state
    //
    switch (State) {
        case 0: {
            transitionComplete = F0Entry(device);
        }
        break;

        //
        // PEP may make us go to any of the F-states directly, hence we execute
        // F0Exit code for all of the Fx states.
        //
        // Transition to any Fx state happens from F0 (and not another
        // Fx state)
        //
        default: {
            ASSERT(State < FSTATE_COUNT);

            transitionComplete = F0Exit(device, State);
        }
        break;        
    }

    if (transitionComplete) {
        PoFxCompleteIdleState(fdoContext->PoHandle, 0 /* Component */);
    }
}

VOID
SingleCompEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a Device Io Control request.

Arguments:

    Queue - Device I/O control queue
              
    Request - Device I/O control request

    OutputBufferLength - Output buffer length for the I/O control

    InputBufferLength - Input buffer length for the I/O control

    IoControlCode - I/O control code
                
--*/
{
    NTSTATUS status;
    PPOWERFX_READ_COMPONENT_INPUT inputBuffer = NULL;
    PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer = NULL;
    WDFDEVICE device = NULL;
    ULONG componentData;
    ULONG_PTR information = 0;
    FDO_DATA *fdoContext = NULL;
    
    //
    // When we complete the request, make sure we don't get the I/O manager to 
    // copy any more data to the client address space than what we write to the
    // output buffer. The only data that we write to the output buffer is the 
    // component data and the C_ASSERT below ensures that the output buffer does
    // not have room to contain anything other than that.
    //
    C_ASSERT(sizeof(componentData) == sizeof(*outputBuffer));

    //
    // This is a power-managed queue. So our queue stop/start logic should have 
    // ensured that we are in the active condition when a request is dispatched
    // from this queue.
    //
    device = WdfIoQueueGetDevice(Queue);
    fdoContext = FdoGetContext(device);
    if (FALSE == fdoContext->IsActive) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - IOCTL %d was dispatched from WDFQUEUE %p when the "
              "component was not in an active condition.",
              IOCTL_POWERFX_READ_COMPONENT,
              Queue);
        WdfVerifierDbgBreakPoint();
    }
    
    //
    // Validate Ioctl code
    //
    if (IOCTL_POWERFX_READ_COMPONENT != IoControlCode) {
        status = STATUS_NOT_SUPPORTED;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! -Unsupported IoControlCode. Expected: %d. Actual: %d."
              " %!status!.", 
              IOCTL_POWERFX_READ_COMPONENT,
              IoControlCode,
              status);
        goto exit;
    }

    //
    // Validate input buffer length
    //
    if (InputBufferLength != sizeof(*inputBuffer)) {
        status = STATUS_INVALID_PARAMETER;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! -Invalid output buffer size. Expected: %d. Actual: %I64u."
              " %!status!.", 
              sizeof(*inputBuffer),
              InputBufferLength,
              status);
        goto exit;
    }
    
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
    componentData = HwSimReadComponent(device);
    outputBuffer->ComponentData = componentData;
    information = sizeof(*outputBuffer);
    
    status = STATUS_SUCCESS;
    
exit:
    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, information);
    return;
}

//
// Read and write queues are only for illustration purposes - on how to stop
// multiple queues. Currently app doesn't send Read/Write to the driver.
//
VOID
SingleCompEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a read request.

Arguments:

    Queue - Read queue
              
    Request - Read request

    OutputBufferLength - Length of read
                
--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    status = STATUS_NOT_SUPPORTED;
        
    Trace(TRACE_LEVEL_ERROR, 
          "%!FUNC! -Reads are currently not supported: %!status!.",
          status);
    
    WdfRequestComplete(Request, status);
}

VOID
SingleCompEvtIoWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t InputBufferLength
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a write request.

Arguments:

    Queue - Write queue
              
    Request - Write request

    InputBufferLength - Length of write
                
--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
        
    status = STATUS_NOT_SUPPORTED;
        
    Trace(TRACE_LEVEL_ERROR, 
          "%!FUNC! -Writes are currently not supported: %!status!.",
          status);
    
    WdfRequestComplete(Request, status);
}

VOID
SingleCompWdmActiveConditionCallback(
    _In_ PVOID Context,
    _In_ ULONG Component
    )
/*++
Routine Description:

    This callback is invoked by Power Framework to notify driver that one of its
    components has become active.

Arguments:

    Context - Context that we supplied when calling 
              WdfDeviceWdmAssignPowerFrameworkSettings.
              
    Component - Component that have become active. Since we have only one 
                component this value is always 0.
    
Return Value:

    None
    
--*/
{
    WDFDEVICE  device;
    FDO_DATA *fdoContext = NULL;
    UCHAR i = 0;

    //
    // We have only component 0
    //
    if (0 != Component) {
        Trace(TRACE_LEVEL_ERROR,"%!FUNC! - Unexpected component %d",Component);
        ASSERT(FALSE);
    }

    //
    // Get the device
    //
    device = (WDFDEVICE) Context;
    
    //
    // Get the device context
    //
    fdoContext = FdoGetContext(device);

    //
    // Mark ourselves as active
    //
    fdoContext->IsActive = TRUE;

    //
    // Start power-managed queues
    //
    for (i = 0; i < QUEUE_COUNT; i++) {
        WdfIoQueueStart(fdoContext->Queues[i]);
    }

    return;
}
    
VOID
SingleCompWdmIdleConditionCallback(
   _In_ PVOID Context,
   _In_ ULONG Component
   )
/*++
Routine Description:
    This callback is invoked by Power Framework to notify driver that one of its
    components has become idle.

Arguments:

    Context - Context that we supplied when calling 
              WdfDeviceWdmAssignPowerFrameworkSettings.
              
    Component - Component that have become idle. Since we have only one 
                component this value is always 0.
    
Return Value:

    None
    
--*/
{
    WDFDEVICE  device;
    FDO_DATA *fdoContext = NULL;
    UCHAR i = 0;

    //
    // We have only component 0
    //
    if (0 != Component) {
        Trace(TRACE_LEVEL_ERROR,"%!FUNC! - Unexpected component %d",Component);
        ASSERT(FALSE);
    }

    //
    // Get the device
    //
    device = (WDFDEVICE) Context;
    
    //
    // Get the device context
    //
    fdoContext = FdoGetContext(device);

    //
    // Initialize the count of queues to be stopped.
    //
    // Until we complete this idle transition there cannot be other idle
    // transitions, which is only where we use this count. Thus nothing races
    // with it.
    //
    // If you have other code that stops/starts queues, it may be simpler
    // to queue a work-item from this callback which stops all the queues
    // synchronously. Synchronous queue stop cannot be done from here since
    // this callback may be called at DISPATCH_LEVEL.
    //
    fdoContext->QueueStopCount = QUEUE_COUNT;
    
    //
    // Stop power-managed queues
    //
    for (i = 0; i < QUEUE_COUNT; i++) {
#pragma warning(suppress: 6387) // passing NULL Context parameter is safe in this instance
        WdfIoQueueStop(fdoContext->Queues[i],
                       SingleCompEvtQueueStopComplete, 
                       NULL /* Context */);
    }
    
    //
    // The idle transition will complete asynchronously. We'll call 
    // PoFxCompleteIdleCondition to complete it when all the queues have been
    // stopped.
    //
    // IMPORTANT NOTE
    // ==============
    // Given that the idle transition does not complete until all the power-
    // managed queues have been stopped, it is extremely important for the 
    // driver to ensure that stopping of power-managed queues is reasonably 
    // quick. If the driver fails to ensure this, the power framework can remain
    // stuck in the idle transition for a long time, which could hamper its 
    // ability to put the component in a low-power F-state. This could 
    // negatively impact any power savings that can be gained via component 
    // power management.
    //
    // In order to ensure that idle transitions can complete quickly, the driver
    // should quickly process any requests that are dispatched to it via a 
    // power-managed queue. If the driver forwards a request (that was received 
    // via a power-managed queue) to an I/O target that might keep the request 
    // pending for a long time, then the driver should cancel the request when
    // the component idle condition callback is invoked. This is because the 
    // power-managed queue cannot be stopped while the request is pending in the
    // I/O target.
    //
    return;
}

VOID
SingleCompEvtQueueStopComplete(
    _In_ WDFQUEUE Queue,
    _In_ WDFCONTEXT Context
    )
/*++
Routine Description:

    Callback invoked by KMDF when stop transiton of the supplied queue has
    completed.

Arguments:

    Queue - Queue whose stop transition has completed
              
    Context - The context we supply while calling WdfIoQueueStop.

Return value:
    None
--*/
{
    PFDO_DATA fdoContext;
    WDFDEVICE device;

    UNREFERENCED_PARAMETER(Context);
    
    device = WdfIoQueueGetDevice(Queue);

    fdoContext = WdfObjectGetTypedContext(device, FDO_DATA);

    if (0 == InterlockedDecrement(&(fdoContext->QueueStopCount))) {
        //
        // All the queues have been stopped. Mark ourselves as idle and complete
        // the idle transition.
        //
        fdoContext->IsActive = FALSE;
        PoFxCompleteIdleCondition(fdoContext->PoHandle, 0 /* Component */);
    }
}
