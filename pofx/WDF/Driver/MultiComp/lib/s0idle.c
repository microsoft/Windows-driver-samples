/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    s0idle.c

Abstract:
    This module contains routines that are used to implement support for 
    S0-idle power management. Note the use of the compile-time switch 
    PFH_S0IDLE_SUPPORTED to control whether or not S0-idle power management 
    support is implemented in the power framework helper library. Drivers that 
    do not require S0-idle power management support should set the 
    PFH_S0IDLE_SUPPORTED compile-time switch to 0 in order to reduce the 
    code size of the power framework helper library.

Environment:

    Kernel mode

--*/

#if PFH_S0IDLE_SUPPORTED

#include "WdfPoFxPriv.h"
#include "s0idle.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PfhSetS0IdleConfiguration)
#pragma alloc_text(PAGE, InitializeDeviceSettingsForS0Idle)
#pragma alloc_text(PAGE, _PowerRequiredPassiveHandler)
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhSetS0IdleConfiguration(
    _In_ WDFOBJECT Initializer,
    _In_ PFH_S0IDLE_CONFIG S0IdleConfig
    )
// See comments in WdfPoFx.h
{
    PHELPER_DEVICE_INIT deviceInitSettings = NULL;

    PAGED_CODE();

    deviceInitSettings = GetDeviceInitSettings(Initializer);

    if (deviceInitSettings->S0IdleConfigSet) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhSetS0IdleConfiguration has already been called on "
              "initialier %p. It should not be called again before the "
              "initializer has been used to initialize a KMDF device object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Save the driver layer's S0-idle configuration
    //
    deviceInitSettings->S0IdleConfig = S0IdleConfig;
    
    deviceInitSettings->S0IdleConfigSet = TRUE;
    return;
}

NTSTATUS
InitializeDeviceSettingsForS0Idle(
    _In_ WDFDEVICE Device,
    _Inout_ PPOFX_DEVICE_CONTEXT DevCtx
    )
/*++
Routine Description:
    In this routine we initialize our S0-idle power management related settings 
    for a device.
    
Arguments:

    Device - Handle to the KMDF device object

    DevCtx - Pointer to our context space for the device object

Return Value:

    An NTSTATUS value representing success or failure of the function.
--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDF_WORKITEM_CONFIG workItemConfig;

    PAGED_CODE();

    //
    // Create a work item that we'll queue in response to the device-power-
    // required callback. This is only needed if S0-idle power management is 
    // supported. Also, queuing a work item can cause pageable data to be 
    // accessed, so we do this only if the device is not in the paging path. 
    // Therefore, we create the work item only if we know that the device will 
    // never be in the paging path.
    //
    if (PfhS0IdleSupportedPowerPageable == 
                DevCtx->DeviceInitSettings.S0IdleConfig) {

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        objectAttributes.ParentObject = Device;//auto-delete when device deleted

        WDF_WORKITEM_CONFIG_INIT(&workItemConfig, _PowerRequiredPassiveHandler);
        workItemConfig.AutomaticSerialization = FALSE;
        
        status = WdfWorkItemCreate(&workItemConfig, 
                                   &objectAttributes, 
                                   &(DevCtx->PowerRequiredWorkItem));
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - WdfWorkItemCreate failed with %!status!", 
                  status);
            goto exit;
        }
    }

    
    //
    // Initialize StopIdleInvokedOnDeviceStart to FALSE so that when the device
    // is started, we invoke WdfDeviceStopIdle to prevent device idling until
    // the power framework permits it.
    //
    DevCtx->StopIdleInvokedOnDeviceStart = FALSE;

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

NTSTATUS
StopDeviceIdleOnDeviceStart(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    )
/*++
Routine Description:
    This routine is invoked during device start. In this routine, we invoke 
    WdfDeviceStopIdle in order to prevent the device from going to Dx as a 
    result of S0-idle power management. We do this only if S0-idle power 
    management is enabled and then only in the following cases:
        1. the device is being started for the first time after creation
        2. the device is being started for the first time after being disabled 
          (applicable to PDOs only)

    When WdfDeviceStopIdle is called KMDF increments its power idle reference 
    count. Device idling can occur only when the power idle reference count 
    drops to 0. Thus by calling WdfDeviceStopIdle, we prevent device idling 
    until we make a balancing call to WdfDeviceResumeIdle to decrement the power
    idle reference count.

    When a device is disabled, KMDF resets the power idle reference count of the
    device to 0. This is the reason we need to call WdfDeviceStopIdle in case #2
    above.

Arguments:

    Device - Handle to the KMDF device object

    DevCtx - Pointer to our context space for the device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;

    if (DevCtx->StopIdleInvokedOnDeviceStart) {
        //
        // This is neither the first device start after creation nor the first
        // device start after it was disabled. So nothing to do here.
        //
        status = STATUS_SUCCESS;
        goto exit;
    }

    DevCtx->StopIdleInvokedOnDeviceStart = TRUE;

    //
    // If S0-idle power management is supported, prevent device idling until the
    // power framework permits.
    //
    if ((PfhS0IdleSupportedPowerPageable == 
            DevCtx->DeviceInitSettings.S0IdleConfig) ||
        (PfhS0IdleSupportedNotPowerPageable == 
            DevCtx->DeviceInitSettings.S0IdleConfig)) {
        //
        // We are currently in the process of entering D0. Therefore we specify 
        // 'FALSE' for the WaitForD0 parameter in the WdfDeviceDeviceStopIdle 
        // call below. Specifying 'TRUE' would result in the deadlock because we
        //  can't block waiting for D0 while we are in the process of entering 
        // D0. However, the call to WdfDeviceStopIdle will ensure that once we 
        // enter D0, we will remain in D0 until WdfDeviceResumeIdle is called.
        //
        status = WdfDeviceStopIdle(Device, FALSE /* WaitForD0 */);
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - WdfDeviceStopIdle failed for WDFDEVICE %p. "
                  "%!status!.",
                  Device,
                  status);
            goto exit;
        }
    }

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

VOID
DevicePowerRequiredForS0Idle(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    )
/*++
Routine Description:

    In this routine we perform actions that are necessary in response to the
    PO_FX_DEVICE_POWER_REQUIRED_CALLBACK callback when S0-idle power management 
    support is enabled for the device.
    
Arguments:

    Device - Handle to the KMDF device object

    DevCtx - Pointer to our context space for the device object
      
Return Value:

    None
    
--*/
{
    PPO_FX_DEVICE_POWER_REQUIRED_CALLBACK devicePowerRequiredCallback = NULL;
    PDEVICE_OBJECT wdmDeviceObject = NULL;
    
    //
    // If the driver layer supplied a device-power-required callback, invoke it
    //
    devicePowerRequiredCallback = 
        DevCtx->DriverLayerPoFxCallbacks.DevicePowerRequiredCallback;
    if (NULL != devicePowerRequiredCallback) {
        devicePowerRequiredCallback(DevCtx->DriverLayerPoFxContext);
    }

    if (PfhS0IdleNotSupported == DevCtx->DeviceInitSettings.S0IdleConfig) {
        //
        // S0-idle power management is not supported. This means that the device
        // always remains in D0, unless there is a system sleep transition, 
        // resource rebalance or device removal. In other words, the device 
        // never leaves D0 because of S0-idle. So we can immediately report to 
        // the power framework that the device is powered on.
        //
        PoFxReportDevicePoweredOn(DevCtx->PoHandle);    
        
    } else if (PfhS0IdleSupportedPowerPageable == 
                        DevCtx->DeviceInitSettings.S0IdleConfig) {
        //
        // S0-idle power management is supported and the device is not in the 
        // paging path. This means that we can access pageable data during the
        // power transition.
        //
        // We need to bring the device to D0 and invoke 
        // PoFxReportDevicePoweredOn when the device is in D0. In order to 
        // achieve this we need to make a blocking call to WdfDeviceStopIdle, 
        // which causes KMDF to bring the device to D0. 
        //
        // We cannot make a blocking call here because:
        //   * Device-power-required can be invoked at dispatch level
        //   * Even if device-power-required is invoked at passive level, it 
        //     might get invoked in the context of an I/O dispatch routine of a
        //     power-managed queue. KMDF does not support blocking calls to 
        //     WdfDeviceStopIdle from within the context of an I/O dispatch 
        //     routine of a power-managed queue because it can lead to a 
        //     deadlock in some situations.
        // Therefore we queue a system work item for making the blocking call to
        // WdfDeviceStopIdle.
        //
        //  !!! IMPORTANT NOTE !!!
        //
        // Queuing a system work item can cause pageable data to be accessed. 
        // Therefore, we can do it only if we know that the device is not in the 
        // paging path and that we can safely access pageable data during power 
        // transitions.
        //
        
        //
        // Verify that the DO_POWER_PAGABLE flag is set in the device object
        //
        wdmDeviceObject = WdfDeviceWdmGetDeviceObject(Device);
        if (0 == (wdmDeviceObject->Flags & DO_POWER_PAGABLE)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - WDFDEVICE %p is in the paging path, but "
                  "PfhSetNotPowerPageable has not been invoked for it.",
                  Device);
            WdfVerifierDbgBreakPoint();
        }
        
        //
        // As described in the above comments, we queue a work item to bring the
        // device to D0.
        //
        WdfWorkItemEnqueue(DevCtx->PowerRequiredWorkItem);
        
    } else {
        //
        //  !!! IMPORTANT NOTE !!!
        //
        // S0-idle power management is supported and the device might be in the 
        // paging path. This means that we cannot access pageable data during 
        // the power transition. As described in the comments above, queueing a
        // system work item to bring the device to D0 can cause pageable data to
        // be accessed. 
        //
        // Therefore, the current implementation of the power framework helper 
        // library does not automatically bring the device to D0 in response to
        // a device-power-required callback, if the device is in the paging path
        // and cannot access pageable data during power transitions. In such 
        // cases the driver layer should supply its own device-power-required 
        // callback. In this callback, it should queue work to its own dedicated
        // worker thread (instead of a system work item) to make a blocking call
        // to WdfDeviceStopIdle. When WdfDeviceStopIdle returns, the device is
        // guaranteed to be in D0 so the driver can call 
        // PoFxReportDevicePoweredOn at that point.
        //
        ASSERT(PfhS0IdleSupportedNotPowerPageable == 
                        DevCtx->DeviceInitSettings.S0IdleConfig);

        DO_NOTHING();
    }
    
    return;
}

VOID
DevicePowerNotRequiredForS0Idle(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    )
/*++
Routine Description:

    In this routine we perform actions that are necessary in response to the
    PO_FX_DEVICE_POWER_NOT_REQUIRED_CALLBACK callback when S0-idle power 
    management support is enabled for the device.
    
Arguments:

    Device - Handle to the KMDF device object

    DevCtx - Pointer to our context space for the device object
      
Return Value:

    None
    
--*/
{
    PPO_FX_DEVICE_POWER_NOT_REQUIRED_CALLBACK 
                devicePowerNotRequiredCallback = NULL;
    
    //
    // If the driver layer supplied a device-power-not-required callback, invoke
    // it
    //
    devicePowerNotRequiredCallback = 
        DevCtx->DriverLayerPoFxCallbacks.DevicePowerNotRequiredCallback;
    if (NULL != devicePowerNotRequiredCallback) {
        devicePowerNotRequiredCallback(DevCtx->DriverLayerPoFxContext);
    }

    if ((PfhS0IdleSupportedPowerPageable == 
            DevCtx->DeviceInitSettings.S0IdleConfig) ||
        (PfhS0IdleSupportedNotPowerPageable == 
            DevCtx->DeviceInitSettings.S0IdleConfig)) {
        //
        // If S0-idle power management is supported, enable KMDF to power down 
        // the device to Dx.
        //
        WdfDeviceResumeIdle(Device);
    }
    
    return;
}

VOID
_PowerRequiredPassiveHandler(
    _In_ WDFWORKITEM WorkItem
    )
/*++
Routine Description:

    This is the callback function invoked at passive level in the context of a 
    system worker thread when we queue a work item in response to the device-
    power-required callback.

Arguments:

    WorkItem - Handle to the KMDF work item object
      
Return Value:

    None
    
--*/
{
    NTSTATUS status;
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;

    PAGED_CODE();

    //
    // Get the handle to the KMDF device object
    //
    device = WdfWorkItemGetParentObject(WorkItem);
    
    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Make a blocking call to WdfDeviceStopIdle in order to get KMDF to bring
    // the device to D0 and/or keep it in D0.
    //
    status = WdfDeviceStopIdle(device, TRUE /*WaitForD0*/);
    if (FALSE == NT_SUCCESS(status)) {
        //
        // The call to WdfDeviceStopIdle failed. However, we need to call 
        // PoFxReportDevicePoweredOn regardless of success or failure because
        // we need to unblock the power framework which is waiting for us to 
        // make this call. In case of failure, KMDF would have already declared
        // the device to be in a failed state and initiated a PNP removal.
        //
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceStopIdle failed for WDFDEVICE %p. "
              "%!status!.",
              device,
              status);

        //
        // Fall through to PoFxReportDevicePoweredOn call below
        //  ||  ||  ||
        //  ||  ||  ||
        //  \/  \/  \/
        //
    }

    //
    // Inform the power framework that the device is now powered on
    //
    PoFxReportDevicePoweredOn(devCtx->PoHandle);
    return;
}

#endif // PFH_S0IDLE_SUPPORTED