/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    device.cpp

Abstract:

    This module contains WDF device initialization 
    and SPB callback functions for the controller driver.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "device.h"
#include "controller.h"

#include "device.tmh"


/////////////////////////////////////////////////
//
// WDF and SPB DDI callbacks.
//
/////////////////////////////////////////////////

NTSTATUS
OnPrepareHardware(
    _In_  WDFDEVICE    FxDevice,
    _In_  WDFCMRESLIST FxResourcesRaw,
    _In_  WDFCMRESLIST FxResourcesTranslated
    )
/*++
 
  Routine Description:

    This routine maps the hardware resources to the SPB
    controller register structure.

  Arguments:

    FxDevice - a handle to the framework device object
    FxResourcesRaw - list of translated hardware resources that 
        the PnP manager has assigned to the device
    FxResourcesTranslated - list of raw hardware resources that 
        the PnP manager has assigned to the device

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);
    NT_ASSERT(pDevice != NULL);
    
    NTSTATUS status = STATUS_SUCCESS; 

    UNREFERENCED_PARAMETER(FxResourcesRaw);
    
    //
    // Get the register base for the I2C controller.
    //

    {
        ULONG resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

        for(ULONG i = 0; i < resourceCount; i++)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR res;
           
            res = WdfCmResourceListGetDescriptor(FxResourcesTranslated, i);

            if (res->Type == CmResourceTypeMemory)
            {
                pDevice->pRegisters = 
                    (PSKELETONI2C_REGISTERS) MmMapIoSpaceEx(
                        res->u.Memory.Start,
                        res->u.Memory.Length,
                        PAGE_NOCACHE | PAGE_READWRITE);

                pDevice->RegistersCb = res->u.Memory.Length;

                if (pDevice->pRegisters == NULL)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;

                    Trace(
                        TRACE_LEVEL_ERROR,
                        TRACE_FLAG_WDFLOADING,
                        "Error mapping controller registers (PA:%I64x, length:%d) "
                        "for WDFDEVICE %p - %!STATUS!", 
                        res->u.Memory.Start.QuadPart,
                        res->u.Memory.Length,
                        pDevice->FxDevice,
                        status);
                    
                    NT_ASSERT(pDevice->pRegisters != NULL);

                    goto exit;
                }

                //
                // Save the physical address to help identify
                // the underlying controller while debugging.
                //

                pDevice->pRegistersPhysicalAddress = res->u.Memory.Start;

                Trace(
                    TRACE_LEVEL_INFORMATION, 
                    TRACE_FLAG_WDFLOADING, 
                    "I2C controller @ paddr %I64x vaddr @ %p for WDFDEVICE %p", 
                    pDevice->pRegistersPhysicalAddress.QuadPart,
                    pDevice->pRegisters,
                    pDevice->FxDevice);
            }
        }
    }

exit:
    
    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnReleaseHardware(
    _In_  WDFDEVICE    FxDevice,
    _In_  WDFCMRESLIST FxResourcesTranslated
    )
/*++
 
  Routine Description:

    This routine unmaps the SPB controller register structure.

  Arguments:

    FxDevice - a handle to the framework device object
    FxResourcesRaw - list of translated hardware resources that 
        the PnP manager has assigned to the device
    FxResourcesTranslated - list of raw hardware resources that 
        the PnP manager has assigned to the device

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);
    NT_ASSERT(pDevice != NULL);
    
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(FxResourcesTranslated);
    
    if (pDevice->pRegisters != NULL)
    {
        MmUnmapIoSpace(pDevice->pRegisters, pDevice->RegistersCb);
        
        pDevice->pRegisters = NULL;
        pDevice->RegistersCb = 0;
    }

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnD0Entry(
    _In_  WDFDEVICE              FxDevice,
    _In_  WDF_POWER_DEVICE_STATE FxPreviousState
    )
/*++
 
  Routine Description:

    This routine allocates objects needed by the driver 
    and initializes the controller hardware.

  Arguments:

    FxDevice - a handle to the framework device object
    FxPreviousState - previous power state

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);
    
    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);
    NT_ASSERT(pDevice != NULL);
    
    UNREFERENCED_PARAMETER(FxPreviousState);

    //
    // Initialize controller.
    //

    pDevice->pCurrentTarget = NULL;

    ControllerInitialize(pDevice);
    
    FuncExit(TRACE_FLAG_WDFLOADING);

    return STATUS_SUCCESS;
}

NTSTATUS
OnD0Exit(
    _In_  WDFDEVICE              FxDevice,
    _In_  WDF_POWER_DEVICE_STATE FxPreviousState
    )
/*++
 
  Routine Description:

    This routine destroys objects needed by the driver 
    and uninitializes the controller hardware.

  Arguments:

    FxDevice - a handle to the framework device object
    FxPreviousState - previous power state

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);
    NT_ASSERT(pDevice != NULL);
    
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(FxPreviousState);

    //
    // Uninitialize controller.
    //

    ControllerUninitialize(pDevice);

    pDevice->pCurrentTarget = NULL;

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnSelfManagedIoInit(
    _In_  WDFDEVICE  FxDevice
    )
/*++
 
  Routine Description:

    Initializes and starts the device's self-managed I/O operations.

  Arguments:
  
    FxDevice - a handle to the framework device object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);
    NTSTATUS status;

    // 
    // Register for monitor power setting callback. This will be
    // used to dynamically set the idle timeout delay according
    // to the monitor power state.
    // 

    NT_ASSERT(pDevice->pMonitorPowerSettingHandle == NULL);
    
    status = PoRegisterPowerSettingCallback(
        WdfDeviceWdmGetDeviceObject(pDevice->FxDevice), 
        &GUID_MONITOR_POWER_ON,
        OnMonitorPowerSettingCallback, 
        (PVOID)pDevice->FxDevice, 
        &pDevice->pMonitorPowerSettingHandle);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_WDFLOADING,
            "Failed to register monitor power setting callback - %!STATUS!",
            status);
                
        goto exit;
    }

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

VOID
OnSelfManagedIoCleanup(
    _In_  WDFDEVICE  FxDevice
    )
/*++
 
  Routine Description:

    Cleanup for the device's self-managed I/O operations.

  Arguments:
  
    FxDevice - a handle to the framework device object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice = GetDeviceContext(FxDevice);

    //
    // Unregister for monitor power setting callback.
    //

    if (pDevice->pMonitorPowerSettingHandle != NULL)
    {
        PoUnregisterPowerSettingCallback(pDevice->pMonitorPowerSettingHandle);
        pDevice->pMonitorPowerSettingHandle = NULL;
    }

    FuncExit(TRACE_FLAG_WDFLOADING);
}

__drv_functionClass(POWER_SETTING_CALLBACK)
_IRQL_requires_same_
NTSTATUS
OnMonitorPowerSettingCallback(
    _In_                          LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID   Value,
    _In_                          ULONG   ValueLength,
    _Inout_opt_                   PVOID   Context
   )
/*++
 
  Routine Description:

    This routine updates the idle timeout delay according 
    to the current monitor power setting.

  Arguments:

    SettingGuid - the setting GUID
    Value - pointer to the new value of the power setting that changed
    ValueLength - value of type ULONG that specifies the size, in bytes, 
                  of the new power setting value 
    Context - the WDFDEVICE pointer context

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    UNREFERENCED_PARAMETER(ValueLength);

    WDFDEVICE Device;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    BOOLEAN isMonitorOff;
    NTSTATUS status = STATUS_SUCCESS;

    if (Context == NULL)
    {
        status = STATUS_INVALID_PARAMETER;

        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_WDFLOADING,
            "%!FUNC! parameter Context is NULL - %!STATUS!",
            status);

        goto exit;
    }

    Device = (WDFDEVICE)Context;

    //
    // We only expect GUID_MONITOR_POWER_ON notifications
    // in this callback, but let's check just to be sure.
    //

    if (IsEqualGUID(*SettingGuid, GUID_MONITOR_POWER_ON))
    {
        NT_ASSERT(Value != NULL);
        NT_ASSERT(ValueLength == sizeof(ULONG));

        //
        // Determine power setting.
        //

        isMonitorOff = ((*(PULONG)Value) == MONITOR_POWER_OFF);

        //
        // Update the idle timeout delay.
        //

        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
            &idleSettings, 
            IdleCannotWakeFromS0);

        idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

        if (isMonitorOff)
        { 
            idleSettings.IdleTimeout = IDLE_TIMEOUT_MONITOR_OFF;
        }
        else
        {
            idleSettings.IdleTimeout = IDLE_TIMEOUT_MONITOR_ON;
 
        }

        status = WdfDeviceAssignS0IdleSettings(
            Device, 
            &idleSettings);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_WDFLOADING,
                "Failed to assign S0 idle settings - %!STATUS!",
                status);
                
            goto exit;
        }
    }

exit:
    
    FuncExit(TRACE_FLAG_WDFLOADING);
 
    return status;
}

NTSTATUS
OnTargetConnect(
    _In_  WDFDEVICE  SpbController,
    _In_  SPBTARGET  SpbTarget
    )
/*++
 
  Routine Description:

    This routine is invoked whenever a peripheral driver opens
    a target.  It retrieves target-specific settings from the
    Resource Hub and saves them in the target's context.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    PPBC_DEVICE pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET pTarget  = GetTargetContext(SpbTarget);
    
    NT_ASSERT(pDevice != NULL);
    NT_ASSERT(pTarget != NULL);
    
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Get target connection parameters.
    //

    SPB_CONNECTION_PARAMETERS params;
    SPB_CONNECTION_PARAMETERS_INIT(&params);

    SpbTargetGetConnectionParameters(SpbTarget, &params);

    //
    // Retrieve target settings.
    //

    status = PbcTargetGetSettings(pDevice,
                                  params.ConnectionParameters,
                                  &pTarget->Settings
                                  );
    
    //
    // Initialize target context.
    //

    if (NT_SUCCESS(status))
    {
        pTarget->SpbTarget = SpbTarget;
        pTarget->pCurrentRequest = NULL;

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_SPBDDI,
            "Connected to SPBTARGET %p at address 0x%lx from WDFDEVICE %p",
            pTarget->SpbTarget,
            pTarget->Settings.Address,
            pDevice->FxDevice);
    }
    
    FuncExit(TRACE_FLAG_SPBDDI);

    return status;
}

VOID
OnControllerLock(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest
    )
/*++
 
  Routine Description:

    This routine is invoked whenever the controller is to
    be locked for a single target. The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object

  Return Value:

    None.  The request is completed synchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    PPBC_DEVICE  pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET  pTarget  = GetTargetContext(SpbTarget);
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Assign current target.
    //

    NT_ASSERT(pDevice->pCurrentTarget == NULL);

    pDevice->pCurrentTarget = pTarget;

    WdfSpinLockRelease(pDevice->Lock);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBDDI,
        "Controller locked for SPBTARGET %p at address 0x%lx (WDFDEVICE %p)",
        pTarget->SpbTarget,
        pTarget->Settings.Address,
        pDevice->FxDevice);

    //
    // Complete lock request.
    //

    SpbRequestComplete(SpbRequest, STATUS_SUCCESS);
    
    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnControllerUnlock(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest
    )
/*++
 
  Routine Description:

    This routine is invoked whenever the controller is to
    be unlocked for a single target. The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object

  Return Value:

    None.  The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    PPBC_DEVICE  pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET  pTarget  = GetTargetContext(SpbTarget);
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    // TODO: Check if there is an active sequence
    //       and if so perform any action necessary
    //       to stop the transfer in process.

    //
    // Remove current target.
    //

    NT_ASSERT(pDevice->pCurrentTarget == pTarget);

    pDevice->pCurrentTarget = NULL;
    
    WdfSpinLockRelease(pDevice->Lock);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBDDI,
        "Controller unlocked for SPBTARGET %p at address 0x%lx (WDFDEVICE %p)",
        pTarget->SpbTarget,
        pTarget->Settings.Address,
        pDevice->FxDevice);

    //
    // Complete lock request.
    //

    SpbRequestComplete(SpbRequest, STATUS_SUCCESS);
    
    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnRead(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest,
    _In_  size_t      Length
    )
/*++
 
  Routine Description:

    This routine sets up a read from the target device using
    the supplied buffers.  The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object
    Length - the number of bytes to read from the target

  Return Value:

    None.  The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBDDI,
        "Received read request %p of length %Iu for SPBTARGET %p "
        "(WDFDEVICE %p)",
        SpbRequest,
        Length,
        SpbTarget,
        SpbController);

    PbcRequestConfigureForNonSequence(
        SpbController,
        SpbTarget,
        SpbRequest,
        Length);

    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnWrite(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest,
    _In_  size_t      Length
    )
/*++
 
  Routine Description:

    This routine sets up a write to the target device using
    the supplied buffers.  The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object
    Length - the number of bytes to write to the target

  Return Value:

    None.  The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBDDI,
        "Received write request %p of length %Iu for SPBTARGET %p "
        "(WDFDEVICE %p)",
        SpbRequest,
        Length,
        SpbTarget,
        SpbController);

    PbcRequestConfigureForNonSequence(
        SpbController,
        SpbTarget,
        SpbRequest,
        Length);

    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnSequence(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest,
    _In_  ULONG       TransferCount
    )
/*++
 
  Routine Description:

    This routine sets up a sequence of reads and writes.  It 
    validates parameters as necessary.  The request is only 
    completed if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object
    TransferCount - number of individual transfers in the sequence

  Return Value:

    None.  The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    PPBC_DEVICE  pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET  pTarget  = GetTargetContext(SpbTarget);
    PPBC_REQUEST pRequest = GetRequestContext(SpbRequest);
    BOOLEAN completeRequest = FALSE;
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);
    NT_ASSERT(pRequest != NULL);
    
    NTSTATUS status = STATUS_SUCCESS;
    
    //
    // Get request parameters.
    //

    SPB_REQUEST_PARAMETERS params;
    SPB_REQUEST_PARAMETERS_INIT(&params);
    SpbRequestGetParameters(SpbRequest, &params);
    
    NT_ASSERT(params.Position == SpbRequestSequencePositionSingle);
    NT_ASSERT(params.Type == SpbRequestTypeSequence);

    //
    // Initialize request context.
    //
    
    pRequest->SpbRequest = SpbRequest;
    pRequest->Type = params.Type;
    pRequest->TotalInformation = 0;
    pRequest->TransferCount = TransferCount;
    pRequest->TransferIndex = 0;
    pRequest->bIoComplete = FALSE;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBDDI,
        "Received sequence request %p with transfer count %d for SPBTARGET %p "
        "(WDFDEVICE %p)",
        pRequest->SpbRequest,
        pRequest->TransferCount,
        SpbTarget,
        SpbController);

    //
    // Validate the request before beginning the transfer.
    //
    
    status = PbcRequestValidate(pRequest);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
    //
    // Configure the request.
    //

    status = PbcRequestConfigureForIndex(pRequest, 0);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_SPBDDI, 
            "Error configuring request context for SPBREQUEST %p "
            "(SPBTARGET %p) - %!STATUS!",
            pRequest->SpbRequest,
            SpbTarget,
            status);

        goto exit;
    }

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Mark request cancellable (if cancellation supported).
    //

    status = WdfRequestMarkCancelableEx(
        pRequest->SpbRequest, OnCancel);

    if (!NT_SUCCESS(status))
    {
        //
        // WdfRequestMarkCancelableEx should only fail if the request
        // has already been cancelled. If it does fail the request
        // must be completed with the corresponding status.
        //

        NT_ASSERTMSG("WdfRequestMarkCancelableEx should only fail if the request has already been cancelled",
            status == STATUS_CANCELLED);

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_TRANSFER,
            "Failed to mark SPBREQUEST %p cancellable - %!STATUS!",
            pRequest->SpbRequest,
            status);
        
        WdfSpinLockRelease(pDevice->Lock);
        goto exit;
    }
    
    //
    // Update device and target contexts.
    //

    NT_ASSERT(pDevice->pCurrentTarget == NULL);
    NT_ASSERT(pTarget->pCurrentRequest == NULL);
    
    pDevice->pCurrentTarget = pTarget;
    pTarget->pCurrentRequest = pRequest;
    
    //
    // Configure controller and kick-off read.
    // Request will be completed asynchronously.
    //
    
    PbcRequestDoTransfer(pDevice, pRequest);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (pRequest->bIoComplete)
    {
        completeRequest = TRUE;
    }
    
    WdfSpinLockRelease(pDevice->Lock);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (completeRequest)
    {
        PbcRequestComplete(pRequest);
    }

exit:

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBDDI,
            "Error configuring sequence, completing "
            "SPBREQUEST %p synchronously - %!STATUS!", 
            pRequest->SpbRequest,
            status);

        SpbRequestComplete(SpbRequest, status);
    }
    
    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnOtherInCallerContext(
    _In_  WDFDEVICE   SpbController,
    _In_  WDFREQUEST  FxRequest
    )
/*++
 
  Routine Description:

    This routine preprocesses custom IO requests before the framework
    places them in an IO queue. For requests using the SPB transfer list
    format, it calls SpbRequestCaptureIoOtherTransferList to capture the
    client's buffers.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbRequest - a handle to the SPBREQUEST object

  Return Value:

    None.  The request is either completed or enqueued asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);

    NTSTATUS status;

    //
    // Check for custom IOCTLs that this driver handles. If
    // unrecognized mark as STATUS_NOT_SUPPORTED and complete.
    //

    WDF_REQUEST_PARAMETERS fxParams;
    WDF_REQUEST_PARAMETERS_INIT(&fxParams);

    WdfRequestGetParameters(FxRequest, &fxParams);

    if ((fxParams.Type != WdfRequestTypeDeviceControl) &&
        (fxParams.Type != WdfRequestTypeDeviceControlInternal))
    {
        status = STATUS_NOT_SUPPORTED;
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBDDI,
            "FxRequest %p is of unsupported request type - %!STATUS!",
            FxRequest,
            status
            );
        goto exit;
    }

    //
    // TODO: verify the driver supports this DeviceIoContol code,
    //       otherwise mark as STATUS_NOT_SUPPORTED and complete.
    //

    //
    // For custom IOCTLs that use the SPB transfer list format
    // (i.e. sequence formatting), call SpbRequestCaptureIoOtherTransferList
    // so that the driver can leverage other SPB DDIs for this request.
    //

    status = SpbRequestCaptureIoOtherTransferList((SPBREQUEST)FxRequest);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBDDI,
            "Failed to capture transfer list for custom SpbRequest %p"
            " - %!STATUS!",
            FxRequest,
            status
            );
        goto exit;
    }

    //
    // Preprocessing has succeeded, enqueue the request.
    //

    status = WdfDeviceEnqueueRequest(SpbController, FxRequest);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

exit:

    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(FxRequest, status);
    }
    
    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnOther(
    _In_  WDFDEVICE   SpbController,
    _In_  SPBTARGET   SpbTarget,
    _In_  SPBREQUEST  SpbRequest,
    _In_  size_t      OutputBufferLength,
    _In_  size_t      InputBufferLength,
    _In_  ULONG       IoControlCode
    )
/*++
 
  Routine Description:

    This routine processes custom IO requests that are not natively
    supported by the SPB framework extension. For requests using the 
    SPB transfer list format, SpbRequestCaptureIoOtherTransferList 
    must have been called in the driver's OnOtherInCallerContext routine.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object
    OutputBufferLength - the request's output buffer length
    InputBufferLength - the requests input buffer length
    IoControlCode - the device IO control code

  Return Value:

    None.  The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_SPBDDI);
    
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(SpbController);
    UNREFERENCED_PARAMETER(SpbTarget);
    UNREFERENCED_PARAMETER(SpbRequest);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    //
    // TODO: the driver should take the following steps
    //
    //    1. Verify this specific DeviceIoContol code is supported,
    //       otherwise mark as STATUS_NOT_SUPPORTED and complete.
    //
    //    2. If this IOCTL uses SPB_TRANSFER_LIST and the driver has
    //       called SpbRequestCaptureIoOtherTransferList previously,
    //       validate the request format. The driver can make use of
    //       SpbRequestGetTransferParameters to retrieve each transfer
    //       descriptor.
    //
    //       If this IOCTL uses some proprietary buffer formating 
    //       instead of SPB_TRANSFER_LIST, validate appropriately.
    //
    //    3. Setup the device, target, and request contexts as necessary,
    //       and program the hardware for the transfer.
    //


    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. Note this must be done outside
    //       of any locked code.
    SpbRequestComplete(SpbRequest, status);
    
    FuncExit(TRACE_FLAG_SPBDDI);
}

VOID
OnCancel(
  _In_  WDFREQUEST  FxRequest
)

/*++
 
  Routine Description:

    This routine cancels an outstanding request. It
    must synchronize with other driver callbacks.

  Arguments:

    wdfRequest - a handle to the WDFREQUEST object

  Return Value:

    None.  The request is completed with status.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    SPBREQUEST spbRequest = (SPBREQUEST) FxRequest;
    PPBC_DEVICE pDevice;
    PPBC_TARGET pTarget;
    PPBC_REQUEST pRequest;
    BOOLEAN bTransferCompleted = FALSE;

    //
    // Get the contexts.
    //

    pDevice = GetDeviceContext(SpbRequestGetController(spbRequest));
    pTarget = GetTargetContext(SpbRequestGetTarget(spbRequest));
    pRequest = GetRequestContext(spbRequest);
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);
    NT_ASSERT(pRequest != NULL);

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Make sure the current target and request 
    // are valid.
    //
    
    if (pTarget != pDevice->pCurrentTarget)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "Cancel callback without a valid current target for WDFDEVICE %p, "
            "this should only occur if SPBREQUEST %p was already completed",
            pDevice->FxDevice,
            spbRequest
            );

        goto exit;
    }

    if (pRequest != pTarget->pCurrentRequest)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "Cancel callback without a valid current request for SPBTARGET %p, "
            "this should only occur if SPBREQUEST %p was already completed",
            pTarget->SpbTarget,
            spbRequest);

        goto exit;
    }

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Cancel callback with outstanding SPBREQUEST %p, "
        "stop IO and complete it",
        spbRequest);

    //
    // Stop delay timer.
    //

    if(WdfTimerStop(pDevice->DelayTimer, FALSE))
    {
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_TRANSFER,
            "Delay timer previously schedule, now stopped");
    }
    
    //
    // Disable interrupts and clear saved stat for DPC. 
    // Must synchronize with ISR.
    //

    NT_ASSERT(pDevice->InterruptObject != NULL);

    // TODO: Uncomment when using interrupts.
    //WdfInterruptAcquireLock(pDevice->InterruptObject);

    ControllerDisableInterrupts(pDevice);
    pDevice->InterruptStatus = 0;
    
    //
    // TODO: Implement any necessary logic to abort the
    //       current IO operation. For I2C this requires
    //       driving a stop bit on the bus.
    //
    
    // TODO: Uncomment when using interrupts.
    //WdfInterruptReleaseLock(pDevice->InterruptObject);

    //
    // Mark request as cancelled and complete.
    //

    pRequest->Status = STATUS_CANCELLED;

    ControllerCompleteTransfer(pDevice, pRequest, TRUE);
    NT_ASSERT(pRequest->bIoComplete == TRUE);
    bTransferCompleted = TRUE;

exit:

    //
    // Release the device lock.
    //

    WdfSpinLockRelease(pDevice->Lock);

    //
    // Complete the request. There shouldn't be more IO.
    // This must be done outside of the locked code.
    //

    if (bTransferCompleted)
    {
        PbcRequestComplete(pRequest);
    }

    FuncExit(TRACE_FLAG_SPBDDI);
}


/////////////////////////////////////////////////
//
// Interrupt handling functions.
//
/////////////////////////////////////////////////

BOOLEAN
OnInterruptIsr(
    _In_  WDFINTERRUPT Interrupt,
    _In_  ULONG        MessageID
    )
/*++
 
  Routine Description:

    This routine responds to interrupts generated by the
    controller. If one is recognized, it queues a DPC for 
    processing. The interrupt is acknowledged and subsequent
    interrupts are temporarily disabled.

  Arguments:

    Interrupt - a handle to a framework interrupt object
    MessageID - message number identifying the device's
        hardware interrupt message (if using MSI)

  Return Value:

    TRUE if interrupt recognized.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    BOOLEAN interruptRecognized = FALSE;
    ULONG stat;
    PPBC_DEVICE pDevice = GetDeviceContext(
        WdfInterruptGetDevice(Interrupt));

    UNREFERENCED_PARAMETER(MessageID);

    NT_ASSERT(pDevice  != NULL);

    //
    // Queue a DPC if the device's interrupt
    // is enabled and active.
    //

    stat = ControllerGetInterruptStatus(
        pDevice,
        PbcDeviceGetInterruptMask(pDevice));

    if (stat > 0)
    {
        Trace(
            TRACE_LEVEL_VERBOSE,
            TRACE_FLAG_TRANSFER,
            "Interrupt with status 0x%lx for WDFDEVICE %p",
            stat,
            pDevice->FxDevice);

        //
        // Save the interrupt status and disable all other
        // interrupts for now.  They will be re-enabled
        // in OnInterruptDpc.  Queue the DPC.
        //

        interruptRecognized = TRUE;
        
        pDevice->InterruptStatus |= (stat);
        ControllerDisableInterrupts(pDevice);
        
        if(!WdfInterruptQueueDpcForIsr(Interrupt))
        {
            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_TRANSFER,
                "Interrupt with status 0x%lx occurred with "
                "DPC already queued for WDFDEVICE %p",
                stat,
                pDevice->FxDevice);
        }
    }

    FuncExit(TRACE_FLAG_TRANSFER);
    
    return interruptRecognized;
}

VOID
OnInterruptDpc(
    _In_  WDFINTERRUPT Interrupt,
    _In_  WDFOBJECT    WdfDevice
    )
/*++
 
  Routine Description:

    This routine processes interrupts from the controller.
    When finished it reenables interrupts as appropriate.

  Arguments:

    Interrupt - a handle to a framework interrupt object
    WdfDevice - a handle to the framework device object

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    PPBC_DEVICE pDevice;
    PPBC_TARGET pTarget;
    PPBC_REQUEST pRequest = NULL;
    ULONG stat;
    BOOLEAN bInterruptsProcessed = FALSE;
    BOOLEAN completeRequest = FALSE;

    UNREFERENCED_PARAMETER(Interrupt);

    pDevice = GetDeviceContext(WdfDevice);
    NT_ASSERT(pDevice != NULL);

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Make sure the target and request are
    // still valid.
    //

    pTarget = pDevice->pCurrentTarget;
    
    if (pTarget == NULL)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "DPC scheduled without a valid current target for WDFDEVICE %p, "
            "this should only occur if the request was already cancelled",
            pDevice->FxDevice);

        goto exit;
    }

    pRequest = pTarget->pCurrentRequest;

    if (pRequest == NULL)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "DPC scheduled without a valid current request for SPBTARGET %p, "
            "this should only occur if the request was already cancelled",
            pTarget->SpbTarget);

        goto exit;
    }

    NT_ASSERT(pRequest->SpbRequest != NULL);

    //
    // Synchronize shared data buffers with ISR.
    // Copy interrupt status and clear shared buffer.
    // If there is a current target and request,
    // a DPC should never occur with interrupt status 0.
    //

    // TODO: Uncomment when using interrupts.
    //WdfInterruptAcquireLock(Interrupt);

    stat = pDevice->InterruptStatus;
    pDevice->InterruptStatus = 0;

    // TODO: Uncomment when using interrupts.
    //WdfInterruptReleaseLock(Interrupt);

    if (stat == 0)
    {
        goto exit;
    }

    Trace(
        TRACE_LEVEL_VERBOSE,
        TRACE_FLAG_TRANSFER,
        "DPC for interrupt with status 0x%lx for WDFDEVICE %p",
        stat,
        pDevice->FxDevice);

    //
    // Acknowledge and process interrupts.
    //

    ControllerAcknowledgeInterrupts(pDevice, stat);

    ControllerProcessInterrupts(pDevice, pRequest, stat);
    bInterruptsProcessed = TRUE;
    if (pRequest->bIoComplete)
    {
        completeRequest = TRUE;
    }

    //
    // Re-enable interrupts if necessary. Synchronize with ISR.
    //

    // TODO: Uncomment when using interrupts.
    //WdfInterruptAcquireLock(Interrupt);

    ULONG mask = PbcDeviceGetInterruptMask(pDevice);

    if (mask > 0)
    {
        Trace(
            TRACE_LEVEL_VERBOSE,
            TRACE_FLAG_TRANSFER,
            "Re-enable interrupts with mask 0x%lx for WDFDEVICE %p",
            mask,
            pDevice->FxDevice);

        ControllerEnableInterrupts(pDevice, mask);
    }

    // TODO: Uncomment when using interrupts.
    //WdfInterruptReleaseLock(Interrupt);

exit:

    //
    // Release the device lock.
    //

    WdfSpinLockRelease(pDevice->Lock);

    //
    // Complete the request if necessary.
    // This must be done outside of the locked code.
    //

    if (bInterruptsProcessed)
    {
        if (completeRequest)
        {
            PbcRequestComplete(pRequest);
        }
    }

    FuncExit(TRACE_FLAG_TRANSFER);
}


/////////////////////////////////////////////////
//
// PBC functions.
//
/////////////////////////////////////////////////

NTSTATUS
PbcTargetGetSettings(
    _In_  PPBC_DEVICE                pDevice,
    _In_  PVOID                      ConnectionParameters,
    _Out_ PPBC_TARGET_SETTINGS       pSettings
    )
/*++
 
  Routine Description:

    This routine populates the target's settings.

  Arguments:

    pDevice - a pointer to the PBC device context
    ConnectionParameters - a pointer to a blob containing the 
        connection parameters
    Settings - a pointer the the target's settings

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_PBCLOADING);

    UNREFERENCED_PARAMETER(pDevice);

    NT_ASSERT(ConnectionParameters != nullptr);
    NT_ASSERT(pSettings != nullptr);

    PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER connection;
    PPNP_SERIAL_BUS_DESCRIPTOR descriptor;
    PPNP_I2C_SERIAL_BUS_DESCRIPTOR i2cDescriptor;

    connection = (PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER)
        ConnectionParameters;

    if (connection->PropertiesLength < sizeof(PNP_SERIAL_BUS_DESCRIPTOR))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_PBCLOADING,
            "Invalid connection properties (length = %lu, "
            "expected = %Iu)",
            connection->PropertiesLength,
            sizeof(PNP_SERIAL_BUS_DESCRIPTOR));

        return STATUS_INVALID_PARAMETER;
    }

    descriptor = (PPNP_SERIAL_BUS_DESCRIPTOR)
        connection->ConnectionProperties;

    if (descriptor->SerialBusType != I2C_SERIAL_BUS_TYPE)
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_PBCLOADING,
            "Bus type %c not supported, only I2C",
            descriptor->SerialBusType);

        return STATUS_INVALID_PARAMETER;
    }

    i2cDescriptor = (PPNP_I2C_SERIAL_BUS_DESCRIPTOR)
        connection->ConnectionProperties;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_PBCLOADING,
        "I2C Connection Descriptor %p "
        "ConnectionSpeed:%lu "
        "Address:0x%hx",
        i2cDescriptor,
        i2cDescriptor->ConnectionSpeed,
        i2cDescriptor->SlaveAddress);

    // Target address
    pSettings->Address = (ULONG)i2cDescriptor->SlaveAddress;

    // Address mode
    USHORT i2cFlags = i2cDescriptor->SerialBusDescriptor.TypeSpecificFlags;
    pSettings->AddressMode = 
        ((i2cFlags & I2C_SERIAL_BUS_SPECIFIC_FLAG_10BIT_ADDRESS) == 0) ? 
            AddressMode7Bit : AddressMode10Bit;

    // Clock speed
    pSettings->ConnectionSpeed = i2cDescriptor->ConnectionSpeed;

    FuncExit(TRACE_FLAG_PBCLOADING);

    return STATUS_SUCCESS;
}

NTSTATUS
PbcRequestValidate(
    _In_  PPBC_REQUEST               pRequest)
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    SPB_TRANSFER_DESCRIPTOR descriptor;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Validate each transfer descriptor.
    //

    for (ULONG i = 0; i < pRequest->TransferCount; i++)
    {
        //
        // Get transfer parameters for index.
        //

        SPB_TRANSFER_DESCRIPTOR_INIT(&descriptor);

        SpbRequestGetTransferParameters(
            pRequest->SpbRequest, 
            i, 
            &descriptor, 
            nullptr);

        //
        // Validate the transfer length.
        //
    
        if (descriptor.TransferLength > SI2C_MAX_TRANSFER_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_TRANSFER, 
                "Transfer length %Iu is too large for controller driver, "
                "max supported is %d (SPBREQUEST %p, index %lu) - %!STATUS!",
                descriptor.TransferLength,
                SI2C_MAX_TRANSFER_LENGTH,
                pRequest->SpbRequest,
                i,
                status);

            goto exit;
        }
    }

exit:

    FuncExit(TRACE_FLAG_TRANSFER);

    return status;
}

VOID
PbcRequestConfigureForNonSequence(
    _In_  WDFDEVICE                  SpbController,
    _In_  SPBTARGET                  SpbTarget,
    _In_  SPBREQUEST                 SpbRequest,
    _In_  size_t                     Length
    )
/*++
 
  Routine Description:

    This is a generic helper routine used to configure
    the request context and controller hardware for a non-
    sequence SPB request. It validates parameters and retrieves
    the transfer buffer as necessary.

  Arguments:

    pDevice - a pointer to the PBC device context
    pTarget - a pointer to the PBC target context
    pRequest - a pointer to the PBC request context
    Length - the number of bytes to read from the target
    Direction - direction of the transfer

  Return Value:

    STATUS

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    PPBC_DEVICE  pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET  pTarget  = GetTargetContext(SpbTarget);
    PPBC_REQUEST pRequest = GetRequestContext(SpbRequest);
    BOOLEAN completeRequest = FALSE;
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);
    NT_ASSERT(pRequest != NULL);

    UNREFERENCED_PARAMETER(Length);

    NTSTATUS status;
    
    //
    // Get the request parameters.
    //

    SPB_REQUEST_PARAMETERS params;
    SPB_REQUEST_PARAMETERS_INIT(&params);
    SpbRequestGetParameters(SpbRequest, &params);

    //
    // Initialize request context.
    //
    
    pRequest->SpbRequest = SpbRequest;
    pRequest->Type = params.Type;
    pRequest->SequencePosition = params.Position;
    pRequest->TotalInformation = 0;
    pRequest->TransferCount = 1;
    pRequest->TransferIndex = 0;
    pRequest->bIoComplete = FALSE;

    //
    // Validate the request before beginning the transfer.
    //
    
    status = PbcRequestValidate(pRequest);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
    //
    // Configure the request.
    //

    status = PbcRequestConfigureForIndex(
        pRequest, 
        pRequest->TransferIndex);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_SPBDDI, 
            "Error configuring request context for SPBREQUEST %p (SPBTARGET %p)"
            "- %!STATUS!", 
            pRequest->SpbRequest,
            SpbTarget,
            status);

        goto exit;
    }

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Mark request cancellable (if cancellation supported).
    //

    status = WdfRequestMarkCancelableEx(
        pRequest->SpbRequest, OnCancel);

    if (!NT_SUCCESS(status))
    {
        //
        // WdfRequestMarkCancelableEx should only fail if the request
        // has already been cancelled. If it does fail the request
        // must be completed with the corresponding status.
        //

        NT_ASSERTMSG("WdfRequestMarkCancelableEx should only fail if the request has already been cancelled",
            status == STATUS_CANCELLED);

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_TRANSFER,
            "Failed to mark SPBREQUEST %p cancellable - %!STATUS!",
            pRequest->SpbRequest,
            status);

        WdfSpinLockRelease(pDevice->Lock);
        goto exit;
    }
    
    //
    // If sequence position is...
    //   - single:     ensure there is not a current target
    //   - not single: ensure that the current target is the
    //                 same as this target
    //
    
    if (params.Position == SpbRequestSequencePositionSingle)
    {        
        NT_ASSERT(pDevice->pCurrentTarget == NULL);
    }
    else
    {   
        NT_ASSERT(pDevice->pCurrentTarget == pTarget);
    }
    
    //
    // Ensure there is not a current request.
    //
    
    NT_ASSERT(pTarget->pCurrentRequest == NULL);
    
    //
    // Update the device and target contexts.
    //
    
    if (pRequest->SequencePosition == SpbRequestSequencePositionSingle)
    {
        pDevice->pCurrentTarget = pTarget;
    }
    
    pTarget->pCurrentRequest = pRequest;
    
    //
    // Configure controller and kick-off read.
    // Request will be completed asynchronously.
    //
    
    PbcRequestDoTransfer(pDevice, pRequest);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (pRequest->bIoComplete)
    {
        completeRequest = TRUE;
    }

    WdfSpinLockRelease(pDevice->Lock);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (completeRequest)
    {
        PbcRequestComplete(pRequest);
    }

exit:
    
    if (!NT_SUCCESS(status))
    {
        SpbRequestComplete(SpbRequest, status);
    }
    
    FuncExit(TRACE_FLAG_TRANSFER);
}

NTSTATUS
PbcRequestConfigureForIndex(
    _Inout_  PPBC_REQUEST            pRequest,
    _In_     ULONG                   Index
    )
/*++
 
  Routine Description:

    This is a helper routine used to configure the request
    context and controller hardware for a transfer within a 
    sequence. It validates parameters and retrieves
    the transfer buffer as necessary.

  Arguments:

    pRequest - a pointer to the PBC request context
    Index - index of the transfer within the sequence

  Return Value:

    STATUS

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pRequest != NULL);
 
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Get transfer parameters for index.
    //

    SPB_TRANSFER_DESCRIPTOR descriptor;
    PMDL pMdl;
    
    SPB_TRANSFER_DESCRIPTOR_INIT(&descriptor);

    SpbRequestGetTransferParameters(
        pRequest->SpbRequest, 
        Index, 
        &descriptor, 
        &pMdl);
       
    NT_ASSERT(pMdl != NULL);
    
    //
    // Configure request context.
    //

    pRequest->pMdlChain = pMdl;
    pRequest->Length = descriptor.TransferLength;
    pRequest->Information = 0;
    pRequest->Direction = descriptor.Direction;
    pRequest->DelayInUs = descriptor.DelayInUs;

    //
    // Update sequence position if request is type sequence.
    //

    if (pRequest->Type == SpbRequestTypeSequence)
    {
        if   (pRequest->TransferCount == 1)
        {
            pRequest->SequencePosition = SpbRequestSequencePositionSingle;
        }
        else if (Index == 0)
        {
            pRequest->SequencePosition = SpbRequestSequencePositionFirst;
        }
        else if (Index == (pRequest->TransferCount - 1))
        {
            pRequest->SequencePosition = SpbRequestSequencePositionLast;
        }
        else
        {
            pRequest->SequencePosition = SpbRequestSequencePositionContinue;
        }
    }

    PPBC_TARGET pTarget = GetTargetContext(SpbRequestGetTarget(pRequest->SpbRequest));
    NT_ASSERT(pTarget != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Request context configured for %s (index %lu) "
        "to address 0x%lx (SPBTARGET %p)",
        pRequest->Direction == SpbTransferDirectionFromDevice ? "read" : "write",
        Index,
        pTarget->Settings.Address,
        pTarget->SpbTarget);

    FuncExit(TRACE_FLAG_TRANSFER);

    return status;
}

VOID
PbcRequestDoTransfer(
    _In_     PPBC_DEVICE             pDevice,
    _In_     PPBC_REQUEST            pRequest
    )
/*++
 
  Routine Description:

    This routine either starts the delay timer or
    kicks off the transfer depending on the request
    parameters.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);
    
    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    //
    // Start delay timer if necessary for this request,
    // otherwise continue transfer. 
    //
    // NOTE: Note using a timer to implement IO delay is only 
    //       applicable for sufficiently long delays (> 15ms).
    //       For shorter delays, especially on the order of
    //       microseconds, consider using a different mechanism.
    //

    if (pRequest->DelayInUs > 0)
    {
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_TRANSFER,
            "Delaying %lu us before configuring transfer for WDFDEVICE %p",
            pRequest->DelayInUs,
            pDevice->FxDevice);

        BOOLEAN bTimerAlreadyStarted;

        bTimerAlreadyStarted = WdfTimerStart(
            pDevice->DelayTimer, 
            WDF_REL_TIMEOUT_IN_US(pRequest->DelayInUs));

        //
        // There should never be another request
        // scheduled for delay.
        //

        if (bTimerAlreadyStarted == TRUE)
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_TRANSFER,
                "The delay timer should not be started");
        }
    }
    else
    {
        ControllerConfigureForTransfer(pDevice, pRequest);
    }

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
OnDelayTimerExpired(
    _In_  WDFTIMER  Timer
    )
/*++
 
  Routine Description:

    This routine is invoked whenever the driver's delay
    timer expires. It kicks off the transfer for the request.

  Arguments:

    Timer - a handle to a framework timer object

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    WDFDEVICE fxDevice;
    PPBC_DEVICE pDevice;
    PPBC_TARGET pTarget = NULL;
    PPBC_REQUEST pRequest = NULL;
    BOOLEAN completeRequest = FALSE;
    
    fxDevice = (WDFDEVICE) WdfTimerGetParentObject(Timer);
    pDevice = GetDeviceContext(fxDevice);

    NT_ASSERT(pDevice != NULL);

    //
    // Acquire the device lock.
    //

    WdfSpinLockAcquire(pDevice->Lock);

    //
    // Make sure the target and request are
    // still valid.
    //

    pTarget = pDevice->pCurrentTarget;
    
    if (pTarget == NULL)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "Delay timer expired without a valid current target for WDFDEVICE %p, "
            "this should only occur if the request was already completed",
            pDevice->FxDevice);

        goto exit;
    }
    
    pRequest = pTarget->pCurrentRequest;

    if (pRequest == NULL)
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_TRANSFER,
            "Delay timer expired without a valid current request for SPBTARGET %p, "
            "this should only occur if the request was already cancelled",
            pTarget->SpbTarget);

        goto exit;
    }

    NT_ASSERT(pRequest->SpbRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Delay timer expired, ready to configure transfer for WDFDEVICE %p",
        pDevice->FxDevice);

    ControllerConfigureForTransfer(pDevice, pRequest);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (pRequest->bIoComplete)
    {
        completeRequest = TRUE;
    }

exit:

    //
    // Release the device lock.
    //

    WdfSpinLockRelease(pDevice->Lock);
    
    // TODO: Remove this block. For the purpose of this 
    //       skeleton sample, simply complete the request 
    //       synchronously. This must be done outside of 
    //       the locked code.
    if (completeRequest)
    {
        PbcRequestComplete(pRequest);
    }

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
PbcRequestComplete(
    _In_     PPBC_REQUEST            pRequest
    )
/*++
 
  Routine Description:

    This routine completes the SpbRequest associated with
    the PBC_REQUEST context.

  Arguments:

    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Completing SPBREQUEST %p with %!STATUS!, transferred %Iu bytes",
        pRequest->SpbRequest,
        pRequest->Status,
        pRequest->TotalInformation);

    WdfRequestSetInformation(
        pRequest->SpbRequest,
        pRequest->TotalInformation);

    SpbRequestComplete(
        pRequest->SpbRequest, 
        pRequest->Status);

    FuncExit(TRACE_FLAG_TRANSFER);
}
