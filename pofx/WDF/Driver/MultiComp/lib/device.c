/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    device.c

Abstract:
    This module contains routines that implement power management for the device

Environment:

    Kernel mode

--*/

#include "WdfPoFxPriv.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, _PfhEvtSelfManagedIoInit)
#pragma alloc_text(PAGE, _PfhEvtSelfManagedIoFlush)
#endif

VOID
UnregisterWithPowerFrameworkWorker(
    _In_ WDFDEVICE Device
    )
/*++
Routine description:
    This routine unregisters a device with the power framework. This routine 
    can be called in EvtDeviceSelfManagedioRestart, which can not be made
    pageable, therefore this function should also not be pageable. 
    
Arguments:
    Device - Handle to the KMDF device object

Return value:
    None
--*/
{
    PPOFX_DEVICE_CONTEXT devCtx;
    PPFH_CALLBACK_POHANDLE_UNAVAILABLE pfhCallbackPoHandleUnavailable;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    if (NULL == devCtx->PoHandle) {
        //
        // We didn't successfully register with the power framework, so nothing
        // to do here.
        //
        return;
    }

    //
    // Notify the driver layer that the POHANDLE is about to be invalidated
    //
    pfhCallbackPoHandleUnavailable = 
        devCtx->DeviceInitSettings.PfhCallbackPoHandleUnavailable;
    if (NULL != pfhCallbackPoHandleUnavailable) {
        pfhCallbackPoHandleUnavailable(Device, devCtx->PoHandle);
    }

    //
    // Unregister with the power framework
    //
    PoFxUnregisterDevice(devCtx->PoHandle);
    devCtx->PoHandle = NULL;

    return;
}

NTSTATUS
RegisterWithPowerFrameworkWorker(
    _In_ WDFDEVICE Device
    )
/*++
Routine description:
    This routine registers a device with the power framework.

Arguments:
    Device - Handle to the KMDF device object

Return value:
    An NTSTATUS value representing success or failure of the function.
--*/
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx;
    PPFH_CALLBACK_POHANDLE_AVAILABLE pfhCallbackPoHandleAvailable;
    ULONG i;
    
    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    if (NULL != devCtx->PoHandle) {
        //
        // Already registered with power framework, nothing to do here.
        //
        status = STATUS_SUCCESS;
        goto exit;
    }

    for (i=0; i < (devCtx->PoFxDeviceInfo->ComponentCount); i++) {
        //
        // Initially, all components are active
        //
        devCtx->ComponentInfo[i].IsActive = TRUE;

        //
        // For PDOs, we unregister when the device is disabled and re-register
        // when the device is re-enabled. Before re-registering, we need to 
        // start the component queues. They would all be in the purged state
        // when the device was disabled.
        // NOTE: It is okay to start an already-started queue, so we can handle
        // first registration and re-registration in the same way.
        //
        if (NULL != devCtx->ComponentInfo[i].Queue) {
            WdfIoQueueStart(devCtx->ComponentInfo[i].Queue);
        }            
    }        

    //
    // Register with the power framework
    //
    status = PoFxRegisterDevice(
                WdfDeviceWdmGetPhysicalDevice(Device),
                devCtx->PoFxDeviceInfo,
                &(devCtx->PoHandle)
                );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PoFxRegisterDevice failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // Notify the driver layer that the POHANDLE is available
    //
    pfhCallbackPoHandleAvailable = 
        devCtx->DeviceInitSettings.PfhCallbackPoHandleAvailable;
    if (NULL != pfhCallbackPoHandleAvailable) {
        status = pfhCallbackPoHandleAvailable(Device, devCtx->PoHandle);
        if (FALSE == NT_SUCCESS(status)) {
            goto exit;
        }
    }

    //
    // Tell the power framework to start its power management
    //
    PoFxStartDevicePowerManagement(devCtx->PoHandle);

    status = STATUS_SUCCESS;
    
exit:
    if (FALSE == NT_SUCCESS(status)) {
        //
        // If an error occurred, we need to unregister with the power framework.
        // Note: the function below handles the case where we haven't registered
        // yet.
        //
        UnregisterWithPowerFrameworkWorker(Device);
    }
    return status;
}

NTSTATUS
_PfhEvtSelfManagedIoInit(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    In this routine, we initialize self-managed I/O operations.

Arguments:

    Device - Handle to the KMDF device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT evtDeviceSelfManagedIoInit;
    
    PAGED_CODE();

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    //
    // If the driver layer supplied a self-managed-IO-init callback, invoke it.
    // We need to do this before checking if power framework settings are 
    // available so that the driver has the opportunity to specify power 
    // framework settings in the self-managed-IO-init callback.
    //
    evtDeviceSelfManagedIoInit = 
        devCtx->DeviceInitSettings.EvtDeviceSelfManagedIoInit;
    if (NULL != evtDeviceSelfManagedIoInit) {
        status = evtDeviceSelfManagedIoInit(Device);
        if (FALSE == NT_SUCCESS(status)) {
            goto exit;
        }
    }

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // If S0-idle power management is supported, prevent the device from 
    // powering down to Dx due to S0-idle. We will allow the device to power 
    // down to Dx only after our device-power-not-required callback is invoked.
    //
    STOP_DEVICE_IDLE_ON_DEVICE_START(status, Device, devCtx, exit);
    
    //
    // Register with the power framework.
    //
    // The EvtDeviceSelfManagedIoInit callback is called after the first D0 
    // entry, but not after subsequent D0 entries. This means that it is not
    // called after D0 entries that occur due to S0-idle, system resuming from 
    // sleep or resource rebalance. Therefore, we register with the power 
    // framework here. We retain the registration until the device is removed.
    // 
    // Note: The function below handles the case where the driver layer already
    // registered with the power framework by calling 
    // PfhRegisterDeviceProactive.
    //
    status = RegisterWithPowerFrameworkWorker(Device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    status = STATUS_SUCCESS;

exit:
    return status;
}

VOID
_PfhEvtSelfManagedIoFlush(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    In this routine, we flush self-managed I/O operations.

Arguments:

    Device - Handle to the KMDF device object

Return Value:

    None

--*/
{
    PPOFX_DEVICE_CONTEXT devCtx;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH evtDeviceSelfManagedIoFlush;
    
    PAGED_CODE();

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p.",
              Device);
        WdfVerifierDbgBreakPoint();
    }
    
    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    if (devCtx->ShouldReportDevicePoweredOn) {
        //
        // We received an S0 IRP and we were supposed to call 
        // PoFxReportDevicePoweredOn after entering D0 in response to the S0 
        // IRP. But an error occurred that prevented us from entering D0 and 
        // KMDF has initiated a device teardown as a result of this error. 
        // Before we attempt to unregister with the power framework, we should
        // call PoFxReportDevicePoweredOn in order to bring the power framework 
        // to a consistent state.
        //
        devCtx->ShouldReportDevicePoweredOn = FALSE;
        PoFxReportDevicePoweredOn(devCtx->PoHandle);
    }

    //
    // The EvtDeviceSelfManagedIoFlush callback is called when the device is 
    // being removed. In this callback, we unregister with the power framework.
    // Note: the function below handles the case where we did not register 
    // successfully.
    //
    UnregisterWithPowerFrameworkWorker(Device);

    //
    // Perform tasks specific to S0-idle power management (if enabled)
    //
    SELF_MANAGED_IO_FLUSH_FOR_S0_IDLE(devCtx);

    //
    // If the driver layer supplied a self-managed-IO-flush callback, invoke it
    //
    evtDeviceSelfManagedIoFlush = 
        devCtx->DeviceInitSettings.EvtDeviceSelfManagedIoFlush;
    if (NULL != evtDeviceSelfManagedIoFlush) {
        evtDeviceSelfManagedIoFlush(Device);
    }

    return;
}

BOOLEAN
IsS0Irp(
    _In_ PIRP Irp,
    _In_ BOOLEAN StackLocationAdjusted
    )
/*++
Routine description:
    This routine determines whether the given IRP is an S0 IRP.

Arguments:
    Irp - Pointer to the IRP

    StackLocationAdjusted - A BOOLEAN value that indicates whether or not the
        IRP's stack location has already been adjusted for forwarding to the 
        next layer in the device stack. If TRUE, it means that the IRP stack 
        location has already been adjusted and this routine must use the next
        IRP stack location. If FALSE, it means that the IRP stack location has
        not yet been adjusted and this routine must use the previous IRP stack
        location.

Return value:
    TRUE if the IRP is an S0 IRP, FALSE otherwise
--*/
{
    PIO_STACK_LOCATION stackLoc;

    if (StackLocationAdjusted) {
        stackLoc = IoGetNextIrpStackLocation(Irp);
    } else {
        stackLoc = IoGetCurrentIrpStackLocation(Irp);
    }
 
    if ((IRP_MJ_POWER == stackLoc->MajorFunction) &&
        (IRP_MN_SET_POWER == stackLoc->MinorFunction) &&
        (SystemPowerState == stackLoc->Parameters.Power.Type) &&
        (PowerSystemWorking == stackLoc->Parameters.Power.State.SystemState)) {
        
        return TRUE;
        
    } else {
    
        return FALSE;
    }
}

NTSTATUS
PreprocessIrpAndDispatch(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN StackLocationAdjusted
    )
/*++
Routine description:
    This routine pre-processes a power IRP and dispatches it to KMDF.
    
Arguments:
    Device - Handle to the KMDF device object

    Irp - Pointer to the IRP

    StackLocationAdjusted - A BOOLEAN value that indicates whether or not the
        IRP's stack location has already been adjusted for forwarding to the 
        next layer in the device stack. If TRUE, it means that the IRP stack 
        location has already been adjusted and this routine must use the next
        IRP stack location. If FALSE, it means that the IRP stack location has
        not yet been adjusted and this routine must use the previous IRP stack
        location.

Return value:
    The NTSTATUS value that is returned from the 
    WdfDeviceWdmDispatchPreprocessedIrp call made by this routine to dispatch 
    the IRP to KMDF.
--*/
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;

    if (IsS0Irp(Irp, StackLocationAdjusted)) {
        //
        // We have received an S0 IRP. The power framework requires that we call
        // PoFxReportDevicePoweredOn after we have returned to D0 as a result of
        // receiving an S0 IRP. Therefore, we make a note in our device context
        // that we need to call PoFxReportDevicePoweredOn from our next 
        // EvtDeviceD0Entry callback.
        //
        devCtx = HelperGetDeviceContext(Device);
        devCtx->ShouldReportDevicePoweredOn = TRUE;
    }
    
    //
    // Forward the IRP for KMDF to handle
    //
    if (FALSE == StackLocationAdjusted) {
        IoSkipCurrentIrpStackLocation(Irp);
    }        
    status = WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);

    return status;
}

NTSTATUS
_PfhEvtWdmPowerIrpPreprocess(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp
    )
/*++
Routine description:
    This routine receives power IRPs before KMDF has processed them. It pre-
    processes the power IRP before eventually forwarding it to KMDF.

Arguments:
    Device - Handle to the KMDF device object

    Irp - Pointer to the IRP

Return value:
    If the driver layer is not interested in pre-processing the IRP, the return 
    value is the NTSTATUS value that is returned from the 
    WdfDeviceWdmDispatchPreprocessedIrp call made by this routine to dispatch 
    the IRP to KMDF.

    If the driver layer is interested in pre-processing the IRP, the return 
    value is the NTSTATUS value that is returned by the driver layer's WDM pre-
    process routine for this IRP.
--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLoc;
    UCHAR minorFunction;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    BOOLEAN invokeDriverLayerCallback = FALSE;
    ULONG i;
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS evtDeviceWdmPowerIrpPreprocess = NULL;

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p.",
              Device);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Get the minor function
    //
    stackLoc = IoGetCurrentIrpStackLocation(Irp);
    minorFunction = stackLoc->MinorFunction;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    //
    // Check if the driver layer is interested in pre-processing an IRP with 
    // this minor function code.
    //
    for (i=0; 
         i < devCtx->DriverLayerPowerIrpPreprocessInfo->NumMinorFunctions;
         i++) {

         if (minorFunction == 
             devCtx->DriverLayerPowerIrpPreprocessInfo->MinorFunctions[i]) {
            //
            // The driver layer is interested in this minor function code 
            //
            invokeDriverLayerCallback = TRUE;
            break;
        }
    }

    if (invokeDriverLayerCallback) {
        //
        // Invoke the driver layer's callback. After the driver layer has 
        // pre-processed the IRP, it will call 
        // PfhWdmDispatchPreprocessedPowerIrp. We will pre-process the IRP
        // at that time.
        //        
        evtDeviceWdmPowerIrpPreprocess = 
            devCtx->DeviceInitSettings.EvtDeviceWdmPowerIrpPreprocess;
        ASSERT(NULL != evtDeviceWdmPowerIrpPreprocess);
        
        status = evtDeviceWdmPowerIrpPreprocess(Device, Irp);

    } else {
        //
        // Preprocess IRP and dispatch to KMDF
        //
        status = PreprocessIrpAndDispatch(Device, 
                                          Irp, 
                                          FALSE /* StackLocationAdjusted */);
    }
        
    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PfhWdmDispatchPreprocessedPowerIrp(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp
    )
// See comments in WdfPoFx.h
{
    PIO_STACK_LOCATION stackLoc;

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p.",
              Device);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Verify that this is a power IRP
    //
    stackLoc = IoGetNextIrpStackLocation(Irp);
    if (IRP_MJ_POWER != stackLoc->MajorFunction) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhWdmDispatchPreprocessedPowerIrp must be called "
              "for power IRPs only. It has been called for IRP %p, which is not"
              " a power IRP.",
              Irp);
        WdfVerifierDbgBreakPoint();
    }

    //
    // The driver layer would have already adjusted the stack location for KMDF
    //
    return PreprocessIrpAndDispatch(Device, 
                                    Irp, 
                                    TRUE /* StackLocationAdjusted */);
}

NTSTATUS
_PfhEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++
Routine Description:

    In this routine, we perform operations that are needed when the device 
    enters the D0 power state.

Arguments:

    Device - Handle to the KMDF device object

    PreviousState - A WDF_POWER_DEVICE_STATE-typed enumerator that identifies 
        the previous device power state.

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    PFN_WDF_DEVICE_D0_ENTRY evtDeviceD0Entry;

    if (FALSE == IsDeviceInitialized(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializeDeviceSettings has not yet been "
              "called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    //
    // If the driver layer supplied a D0Entry callback, invoke it
    //
    evtDeviceD0Entry = devCtx->DeviceInitSettings.EvtDeviceD0Entry;
    if (NULL != evtDeviceD0Entry) {
        status = evtDeviceD0Entry(Device, PreviousState);
        if (FALSE == NT_SUCCESS(status)) {
            goto exit;
        }
    }
    
    if (devCtx->ShouldReportDevicePoweredOn) {
        //
        // This is the first time we are entering D0 after receiving an S0 IRP.
        // We should call PoFxReportDevicePoweredOn.
        //
        devCtx->ShouldReportDevicePoweredOn = FALSE;
        PoFxReportDevicePoweredOn(devCtx->PoHandle);
    }

    status = STATUS_SUCCESS;

exit:

    return status;
}

NTSTATUS
_PfhEvtSelfManagedIoRestart(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    In this routine, we restart self-managed I/O operations.

Arguments:

    Device - Handle to the KMDF device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART evtDeviceSelfManagedIoRestart;

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }
    
    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    //
    // If the driver layer supplied a self-managed-IO-restart callback, invoke
    // it
    //
    evtDeviceSelfManagedIoRestart = 
        devCtx->DeviceInitSettings.EvtDeviceSelfManagedIoRestart;
    if (NULL != evtDeviceSelfManagedIoRestart) {
        status = evtDeviceSelfManagedIoRestart(Device);
        if (FALSE == NT_SUCCESS(status)) {
            goto exit;
        }
    }

    //
    // For a PDO, if this is the first start after the device was disabled and 
    // if S0-idle power management is supported for the device, then we prevent
    // the device from powering down to Dx due to S0-idle. We will allow the 
    // device to power down to Dx only after our device-power-not-required 
    // callback is invoked.
    //
    // NOTE: The function below figures out whether this is the first start 
    // after the device was disabled and does the right thing based on that.
    //
    STOP_DEVICE_IDLE_ON_DEVICE_START(status, Device, devCtx, exit);
    
    //
    // For a PDO, we need to register with the power framework in the
    // self-managed-IO-restart callback if the device was disabled and then 
    // re-enabled. In this case, we would have unregistered with the power 
    // framework in the self-managed-IO-flush callback. Therefore, we need to 
    // re-register now.
    //
    // NOTE: The function below handles the case where we are already registered
    // with the power framework.
    //
    status = RegisterWithPowerFrameworkWorker(Device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

VOID
_PfhDevicePowerRequiredCallback(
    _In_ PVOID Context
    )
/*++
Routine Description:

    The power framework invokes this routine to notify us that we need to
    enter/remain in the D0 state

Arguments:

    Context - Context that we passed in to the power framework
      
Return Value:

    None
    
--*/
{
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    
    //
    // Get the handle to the KMDF device object
    //
    device = (WDFDEVICE) Context;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Perform actions specific to S0-idle power management (if enabled)
    //
    DEVICE_POWER_REQUIRED_FOR_S0_IDLE(device, devCtx);
    return;
}

VOID
_PfhDevicePowerNotRequiredCallback(
    _In_ PVOID Context
    )
/*++
Routine Description:

    The power framework invokes this routine to notify us that we may enter
    a low-power Dx state

Arguments:

    Context - Context that we passed in to the power framework
      
Return Value:

    None
    
--*/
{
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    
    //
    // Get the handle to the KMDF device object
    //
    device = (WDFDEVICE) Context;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Perform actions specific to S0-idle power management (if enabled)
    //
    DEVICE_POWER_NOT_REQUIRED_FOR_S0_IDLE(device, devCtx);
    
    //
    // Tell the power framework that we've finished processing device-power-
    // not-required
    //
    PoFxCompleteDevicePowerNotRequired(devCtx->PoHandle);

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhRegisterDeviceProactive(
    _In_ WDFDEVICE Device
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    PPOFX_DEVICE_CONTEXT devCtx;

    if (FALSE == ArePowerFrameworkSettingsAvailable(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet "
              "been called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    if (NULL != devCtx->PoHandle) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhRegisterDeviceProactive was called when the "
              "device was already registered with the power framework. "
              "%!status!.",
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }
    
    status = RegisterWithPowerFrameworkWorker(Device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    status = STATUS_SUCCESS;
    
exit:
    return status;
}
