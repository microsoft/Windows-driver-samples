/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    device.cpp

Abstract:

    This module contains WDF device initialization 
    functions for the peripheral driver.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "peripheral.h"
#include "device.h"

#include "device.tmh"


/////////////////////////////////////////////////
//
// WDF callbacks.
//
/////////////////////////////////////////////////

NTSTATUS
OnPrepareHardware(
    _In_  WDFDEVICE     FxDevice,
    _In_  WDFCMRESLIST  FxResourcesRaw,
    _In_  WDFCMRESLIST  FxResourcesTranslated
    )
/*++
 
  Routine Description:

    This routine caches the SPB resource connection ID.

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

    PDEVICE_CONTEXT pDevice = GetDeviceContext(FxDevice);
    BOOLEAN fSpbResourceFound = FALSE;
    BOOLEAN fInterruptResourceFound = FALSE;
    ULONG interruptIndex = 0;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(FxResourcesRaw);

    //
    // Parse the peripheral's resources.
    //

    ULONG resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

    for(ULONG i = 0; i < resourceCount; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
        UCHAR Class;
        UCHAR Type;

        pDescriptor = WdfCmResourceListGetDescriptor(
            FxResourcesTranslated, i);

        switch (pDescriptor->Type)
        {
            case CmResourceTypeConnection:

                //
                // Look for I2C or SPI resource and save connection ID.
                //

                Class = pDescriptor->u.Connection.Class;
                Type = pDescriptor->u.Connection.Type;

                if ((Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL) &&
                    ((Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C) ||
                        (Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI)))
                {
                    if (fSpbResourceFound == FALSE)
                    {
                        pDevice->PeripheralId.LowPart =
                            pDescriptor->u.Connection.IdLowPart;
                        pDevice->PeripheralId.HighPart =
                            pDescriptor->u.Connection.IdHighPart;

                        fSpbResourceFound = TRUE;

                        Trace(
                            TRACE_LEVEL_INFORMATION,
                            TRACE_FLAG_WDFLOADING,
                            "SPB resource found with ID=0x%llx",
                            pDevice->PeripheralId.QuadPart);
                    }
                    else
                    {
                        Trace(
                            TRACE_LEVEL_WARNING,
                            TRACE_FLAG_WDFLOADING,
                            "Duplicate SPB resource found with ID=0x%llx",
                            pDevice->PeripheralId.QuadPart);
                    }
                }

                break;

            case CmResourceTypeInterrupt:
                
                if (fInterruptResourceFound == FALSE)
                {
                    fInterruptResourceFound = TRUE;
                    interruptIndex = i;

                    Trace(
                        TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_WDFLOADING,
                        "Interrupt resource found");
                }
                else
                {
                    Trace(
                        TRACE_LEVEL_WARNING,
                        TRACE_FLAG_WDFLOADING,
                        "Duplicate interrupt resource found");
                }

                break;

            default:

                //
                // Ignoring all other resource types.
                //

                break;
        }
    }

    //
    // An SPB resource is required.
    //

    if (fSpbResourceFound == FALSE)
    {
        status = STATUS_NOT_FOUND;
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_WDFLOADING,
            "SPB resource not found - %!STATUS!", 
            status);
    }

    //
    // Create the interrupt if an interrupt
    // resource was found.
    //

    if (NT_SUCCESS(status))
    {
        if ((pDevice->ConnectInterrupt == TRUE) && 
            (fInterruptResourceFound == TRUE))
        {
            WDF_INTERRUPT_CONFIG interruptConfig;
            WDF_INTERRUPT_CONFIG_INIT(
                            &interruptConfig,
                            OnInterruptIsr,
                            NULL);

            interruptConfig.PassiveHandling = TRUE;
            interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(
                FxResourcesTranslated, 
                interruptIndex);
            interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(
                FxResourcesRaw, 
                interruptIndex);

            status = WdfInterruptCreate(
                            pDevice->FxDevice,
                            &interruptConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &pDevice->Interrupt);

            if (!NT_SUCCESS(status))
            {
                Trace(
                        TRACE_LEVEL_ERROR, 
                        TRACE_FLAG_WDFLOADING,
                        "WdfInterruptCreate failed - %!STATUS!",
                        status);
            }

            if (NT_SUCCESS(status))
            {
                KeInitializeEvent(
                    &pDevice->IsrWaitEvent,
                    SynchronizationEvent,
                    FALSE);
            }
        }
    }

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnReleaseHardware(
    _In_  WDFDEVICE     FxDevice,
    _In_  WDFCMRESLIST  FxResourcesTranslated
    )
/*++
 
  Routine Description:

  Arguments:

    FxDevice - a handle to the framework device object
    FxResourcesTranslated - list of raw hardware resources that 
        the PnP manager has assigned to the device

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);
    
    PDEVICE_CONTEXT pDevice = GetDeviceContext(FxDevice);
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(FxResourcesTranslated);

    if (pDevice->Interrupt != nullptr)
    {
        WdfObjectDelete(pDevice->Interrupt);
    }

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnD0Entry(
    _In_  WDFDEVICE               FxDevice,
    _In_  WDF_POWER_DEVICE_STATE  FxPreviousState
    )
/*++
 
  Routine Description:

    This routine allocates objects needed by the driver.

  Arguments:

    FxDevice - a handle to the framework device object
    FxPreviousState - previous power state

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);
    
    UNREFERENCED_PARAMETER(FxPreviousState);

    PDEVICE_CONTEXT pDevice = GetDeviceContext(FxDevice);
    NTSTATUS status;

    //
    // Create the SPB target.
    //

    WDF_OBJECT_ATTRIBUTES targetAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&targetAttributes);
    
    status = WdfIoTargetCreate(
        pDevice->FxDevice,
        &targetAttributes,
        &pDevice->SpbController);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_WDFLOADING,
            "Failed to create IO target - %!STATUS!",
            status);
    }

    //
    // InputMemory will be created when an SPB request is about to be
    // sent. Indicate that it is not yet initialized.
    //

    pDevice->InputMemory = WDF_NO_HANDLE;

    //
    // Create the SPB request.
    //

    if (NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES requestAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REQUEST_CONTEXT);
    
        status = WdfRequestCreate(
            &requestAttributes,
            nullptr,
            &pDevice->SpbRequest);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_WDFLOADING,
                "Failed to create IO request - %!STATUS!",
                status);
        }

        if (NT_SUCCESS(status))
        {
            PREQUEST_CONTEXT pRequest = GetRequestContext(
                pDevice->SpbRequest);

            pRequest->FxDevice = pDevice->FxDevice;
            pRequest->IsSpbSequenceRequest = FALSE;
            pRequest->SequenceWriteLength = 0;
        }
    }

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

NTSTATUS
OnD0Exit(
    _In_  WDFDEVICE               FxDevice,
    _In_  WDF_POWER_DEVICE_STATE  FxPreviousState
    )
/*++
 
  Routine Description:

    This routine destroys objects needed by the driver.

  Arguments:

    FxDevice - a handle to the framework device object
    FxPreviousState - previous power state

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);
    
    UNREFERENCED_PARAMETER(FxPreviousState);

    PDEVICE_CONTEXT pDevice = GetDeviceContext(FxDevice);

    if (pDevice->SpbController != WDF_NO_HANDLE)
    {
        WdfObjectDelete(pDevice->SpbController);
        pDevice->SpbController = WDF_NO_HANDLE;
    }

    if (pDevice->SpbRequest != WDF_NO_HANDLE)
    {
        WdfObjectDelete(pDevice->SpbRequest);
        pDevice->SpbRequest = WDF_NO_HANDLE;
    }

    if (pDevice->InputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(pDevice->InputMemory);
        pDevice->InputMemory = WDF_NO_HANDLE;
    }

    FuncExit(TRACE_FLAG_WDFLOADING);

    return STATUS_SUCCESS;
}

VOID
OnFileCleanup(
    _In_  WDFFILEOBJECT  FileObject
    )
/*++
 
  Routine Description:

    This routine is called before a device is stopped.

  Arguments:

    FileObject - a handle to the framework file object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;

    device = WdfFileObjectGetDevice(FileObject);
    pDevice = GetDeviceContext(device);

    //
    // The client app has closed the device handle,
    // make sure the SPB target is closed.
    //

    SpbPeripheralClose(pDevice);

    //
    // Signal the ISR wait event just
    // in case the callback is stalled.
    //    

    if (pDevice->Interrupt != nullptr)
    {
        KeSetEvent(
            &pDevice->IsrWaitEvent,
            IO_NO_INCREMENT,
            FALSE);
    }

    FuncExit(TRACE_FLAG_WDFLOADING);
}

VOID
OnTopLevelIoDefault(
    _In_  WDFQUEUE    FxQueue,
    _In_  WDFREQUEST  FxRequest
    )
/*++

  Routine Description:

    Accepts all incoming requests and pends or forwards appropriately.

  Arguments:

    FxQueue -  Handle to the framework queue object that is associated with the
        I/O request.
    FxRequest - Handle to a framework request object.

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    UNREFERENCED_PARAMETER(FxQueue);

    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;
    WDF_REQUEST_PARAMETERS params;
    NTSTATUS status;

    device = WdfIoQueueGetDevice(FxQueue);
    pDevice = GetDeviceContext(device);

    WDF_REQUEST_PARAMETERS_INIT(&params);

    WdfRequestGetParameters(FxRequest, &params);

    if ((params.Type == WdfRequestTypeDeviceControl) &&
        (params.Parameters.DeviceIoControl.IoControlCode == 
            IOCTL_SPBTESTTOOL_WAIT_ON_INTERRUPT))
    {
        SpbPeripheralWaitOnInterrupt(pDevice, FxRequest);
    }
    else
    {
        status = WdfRequestForwardToIoQueue(FxRequest, pDevice->SpbQueue);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPBAPI,
                "Failed to forward WDFREQUEST %p to SPB queue %p - %!STATUS!",
                FxRequest,
                pDevice->SpbQueue,
                status);

            WdfRequestComplete(FxRequest, status);
        }
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
OnIoRead (
    _In_  WDFQUEUE    FxQueue,
    _In_  WDFREQUEST  FxRequest,
    _In_  size_t      Length
    )
/*++

  Routine Description:

    Performs read from the toaster device. This event is called when the
    framework receives IRP_MJ_READ requests.

  Arguments:

    FxQueue -  Handle to the framework queue object that is associated with the
        I/O request.
    FxRequest - Handle to a framework request object.
    Length - Length of the data buffer associated with the request.
        By default, the queue does not dispatch zero length read & write 
        requests to the driver and instead to complete such requests with 
        status success. So we will never get a zero length request.

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    UNREFERENCED_PARAMETER(Length);
    
    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Read request %p received",
        FxRequest);

    device = WdfIoQueueGetDevice(FxQueue);
    pDevice = GetDeviceContext(device);

    //
    // Send the read request.
    //
    
    SpbPeripheralRead(pDevice,FxRequest);

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
OnIoWrite (
    _In_  WDFQUEUE    FxQueue,
    _In_  WDFREQUEST  FxRequest,
    _In_  size_t      Length
    )
/*++

Routine Description:

    Performs write to the toaster device. This event is called when the
    framework receives IRP_MJ_WRITE requests.

Arguments:

    Queue -  Handle to the framework queue object that is associated 
        with the I/O request.
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
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(Length);
    
    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Write request %p received",
        FxRequest);

    device = WdfIoQueueGetDevice(FxQueue);
    pDevice = GetDeviceContext(device);

    //
    // Send the write request.
    //
    
    SpbPeripheralWrite(pDevice,FxRequest);

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
OnIoDeviceControl(
    _In_  WDFQUEUE    FxQueue,
    _In_  WDFREQUEST  FxRequest,
    _In_  size_t      OutputBufferLength,
    _In_  size_t      InputBufferLength,
    _In_  ULONG       IoControlCode
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    FxQueue - Handle to the framework queue object that is associated
        with the I/O request.
    FxRequest - Handle to a framework request object.
    OutputBufferLength - length of the request's output buffer,
        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
        if an input buffer is available.
    IoControlCode - the driver-defined or system-defined I/O control code
        (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;
    BOOLEAN fSync = FALSE;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "DeviceIoControl request %p received with IOCTL=%lu",
        FxRequest,
        IoControlCode);

    device = WdfIoQueueGetDevice(FxQueue);
    pDevice = GetDeviceContext(device);

    //
    // Translate the test IOCTL into the appropriate 
    // SPB API method.  Open and close are completed 
    // synchronously.
    //

    switch (IoControlCode)
    {
    case IOCTL_SPBTESTTOOL_OPEN:
        fSync = TRUE;
        status = SpbPeripheralOpen(pDevice);
        break;

    case IOCTL_SPBTESTTOOL_CLOSE:
        fSync = TRUE;
        status = SpbPeripheralClose(pDevice);
        break;

    case IOCTL_SPBTESTTOOL_LOCK:
        SpbPeripheralLock(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_UNLOCK:
        SpbPeripheralUnlock(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_LOCK_CONNECTION:
        SpbPeripheralLockConnection(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_UNLOCK_CONNECTION:
        SpbPeripheralUnlockConnection(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_WRITEREAD:
        SpbPeripheralWriteRead(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_FULL_DUPLEX:
        SpbPeripheralFullDuplex(pDevice, FxRequest);
        break;

    case IOCTL_SPBTESTTOOL_SIGNAL_INTERRUPT:
        SpbPeripheralSignalInterrupt(pDevice, FxRequest);
        break;
        
    default:
        fSync = TRUE;
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "Request %p received with unexpected IOCTL=%lu",
            FxRequest,
            IoControlCode);
    }

    //
    // Complete the request if necessary.
    //

    if (fSync)
    {
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_SPBAPI,
            "Completing request %p with %!STATUS!",
            FxRequest,
            status);

        WdfRequestComplete(FxRequest, status);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

BOOLEAN
OnInterruptIsr(
    _In_  WDFINTERRUPT FxInterrupt,
    _In_  ULONG        MessageID
    )
/*++
 
  Routine Description:

    This routine responds to interrupts generated by the H/W.
    It then waits indefinitely for the user to signal that
    the interrupt has been acknowledged, allowing the ISR to
    return. This ISR is called at PASSIVE_LEVEL.

  Arguments:
  
    Interrupt - a handle to a framework interrupt object
    MessageID - message number identifying the device's
        hardware interrupt message (if using MSI)

  Return Value:

    TRUE if interrupt recognized.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    BOOLEAN fInterruptRecognized = TRUE;
    BOOLEAN fNotificationSent;
    WDFDEVICE device;
    PDEVICE_CONTEXT pDevice;

    UNREFERENCED_PARAMETER(MessageID);

    device = WdfInterruptGetDevice(FxInterrupt);
    pDevice = GetDeviceContext(device);
    
    //
    // Notify the app that an interrupt has occurred.
    //

    fNotificationSent = SpbPeripheralInterruptNotify(pDevice);

    if (fNotificationSent)
    {
        //
        // Stall in ISR until acknowledged by user.
        //
        // Note: In a 'real' driver, the ISR should directly
        //       acknowledge the interrupt and then queue
        //       a workitem to carry out any additional
        //       processing. The ISR should never call
        //       KeWaitForSingleObject as done below.
        //

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_SPBAPI,
            "Stalling in ISR until continue command received");

        KeClearEvent(&pDevice->IsrWaitEvent);

        KeWaitForSingleObject(
            &pDevice->IsrWaitEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }
    else
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "Interrupt detected, but failed to send notification, ignoring");
    }

    FuncExit(TRACE_FLAG_SPBAPI);

    return fInterruptRecognized;
}
