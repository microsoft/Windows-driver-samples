/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    peripheral.cpp

Abstract:

    This module contains the function for interaction 
    with the SPB API.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "peripheral.h"

#include "peripheral.tmh"

NTSTATUS
SpbPeripheralOpen(
    _In_  PDEVICE_CONTEXT  pDevice
    )
/*++
 
  Routine Description:

    This routine opens a handle to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    WDF_IO_TARGET_OPEN_PARAMS  openParams;
    NTSTATUS status;

    //
    // Create the device path using the connection ID.
    //

    DECLARE_UNICODE_STRING_SIZE(DevicePath, RESOURCE_HUB_PATH_SIZE);

    RESOURCE_HUB_CREATE_PATH_FROM_ID(
        &DevicePath,
        pDevice->PeripheralId.LowPart,
        pDevice->PeripheralId.HighPart);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Opening handle to SPB target via %wZ",
        &DevicePath);

    //
    // Open a handle to the SPB controller.
    //

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &DevicePath,
        (GENERIC_READ | GENERIC_WRITE));
    
    openParams.ShareAccess = 0;
    openParams.CreateDisposition = FILE_OPEN;
    openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    
    status = WdfIoTargetOpen(
        pDevice->SpbController,
        &openParams);
     
    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to open SPB target - %!STATUS!",
            status);
    }

    FuncExit(TRACE_FLAG_SPBAPI);

    return status;
}

NTSTATUS
SpbPeripheralClose(
    _In_  PDEVICE_CONTEXT  pDevice
    )
/*++
 
  Routine Description:

    This routine closes a handle to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Closing handle to SPB target");

    WdfIoTargetClose(pDevice->SpbController);

    FuncExit(TRACE_FLAG_SPBAPI);
    
    return STATUS_SUCCESS;
}

VOID
SpbPeripheralLock(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends a lock command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_LOCK_CONTROLLER",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for lock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_LOCK_CONTROLLER,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            FxRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_LOCK_CONTROLLER - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralUnlock(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends an unlock command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_UNLOCK_CONTROLLER",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for unlock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_UNLOCK_CONTROLLER,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            FxRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_UNLOCK_CONTROLLER - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralLockConnection(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends a lock connection command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_LOCK_CONNECTION",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for lock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_LOCK_CONNECTION,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            FxRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_LOCK_CONNECTION - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralUnlockConnection(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends an unlock connection command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_UNLOCK_CONNECTION",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for unlock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_UNLOCK_CONNECTION,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            FxRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_UNLOCK_CONNECTION - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralRead(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine reads from the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    WDFMEMORY memory = nullptr;
    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for read",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for read and send.
    //

    status = WdfRequestRetrieveOutputMemory(
        FxRequest,
        &memory);

    if (NT_SUCCESS(status))
    {
        status = WdfIoTargetFormatRequestForRead(
            pDevice->SpbController,
            pDevice->SpbRequest,
            memory,
            nullptr,
            nullptr);

        if (NT_SUCCESS(status))
        {
            status = SpbPeripheralSendRequest(
                pDevice,
                pDevice->SpbRequest,
                FxRequest);
        }
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "read - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralWrite(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine writes to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    WDFMEMORY memory = nullptr;
    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for write",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Initialize the SPB request for write and send.
    //

    status = WdfRequestRetrieveInputMemory(
        FxRequest,
        &memory);

    if (NT_SUCCESS(status))
    {
        status = WdfIoTargetFormatRequestForWrite(
            pDevice->SpbController,
            pDevice->SpbRequest,
            memory,
            nullptr,
            nullptr);

        if (NT_SUCCESS(status))
        {
            status = SpbPeripheralSendRequest(
                pDevice,
                pDevice->SpbRequest,
                FxRequest);
        }
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "write - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralWriteRead(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends a write-read sequence to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    PVOID pInputBuffer = nullptr;
    PVOID pOutputBuffer = nullptr;
    size_t inputBufferLength = 0;
    size_t outputBufferLength = 0;
    WDF_OBJECT_ATTRIBUTES attributes;
    PREQUEST_CONTEXT pRequest;
    NTSTATUS status;

    pRequest = GetRequestContext(pDevice->SpbRequest);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_EXECUTE_SEQUENCE",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Get input and output buffers.
    //

    status = WdfRequestRetrieveInputBuffer(
        FxRequest,
        0,
        &pInputBuffer,
        &inputBufferLength);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to retrieve input buffer - %!STATUS!",
            status);

        goto Done;
    }

    status = WdfRequestRetrieveOutputBuffer(
        FxRequest,
        0,
        &pOutputBuffer,
        &outputBufferLength);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to retrieve output buffer - %!STATUS!",
            status);

        goto Done;
    }

    //
    // Build SPB sequence.
    //
    
    const ULONG transfers = 2;

    SPB_TRANSFER_LIST_AND_ENTRIES(transfers) seq;
    SPB_TRANSFER_LIST_INIT(&(seq.List), transfers);

    {
        //
        // PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
        // "struct hack" size but using an index variable quiets 
        // the warning. This is a false positive from OACR.
        // 

        ULONG index = 0;
        seq.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionToDevice,
            0,
            pInputBuffer,
            (ULONG)inputBufferLength);

        seq.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionFromDevice,
            0,
            pOutputBuffer,
            (ULONG)outputBufferLength);
    }

    //
    // Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
    // so the memory doesn't have to persist until the request is
    // completed.
    //

    NT_ASSERT(pDevice->InputMemory == WDF_NO_HANDLE);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    status = WdfMemoryCreatePreallocated(
        &attributes,
        (PVOID)&seq,
        sizeof(seq),
        &pDevice->InputMemory);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to create WDFMEMORY - %!STATUS!",
            status);

        goto Done;
    }

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Built write-read sequence %p with byte length=%lu",
        &seq,
        (ULONG)(inputBufferLength + outputBufferLength));

    //
    // Send sequence IOCTL.
    //

    //
    // Mark SPB request as sequence and save length.
    // These will be used in the completion callback
    // to complete the client request with the correct
    // number of bytes
    //

    pRequest->IsSpbSequenceRequest = TRUE;
    pRequest->SequenceWriteLength = (ULONG_PTR)inputBufferLength;

    //
    // Format and send the SPB sequence request.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_EXECUTE_SEQUENCE,
        pDevice->InputMemory,
        nullptr,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to format request - %!STATUS!",
            status);

        goto Done;
    }

    status = SpbPeripheralSendRequest(
        pDevice,
        pDevice->SpbRequest,
        FxRequest);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_EXECUTE_SEQUENCE - %!STATUS!",
            pDevice->SpbRequest,
            status);

        goto Done;
    }

Done:

    if (!NT_SUCCESS(status))
    {
        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralFullDuplex(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
 
  Routine Description:

    This routine sends a full duplex transfer to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    FxRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(FxRequest);

    PVOID pInputBuffer = nullptr;
    PVOID pOutputBuffer = nullptr;
    size_t inputBufferLength = 0;
    size_t outputBufferLength = 0;
    WDF_OBJECT_ATTRIBUTES attributes;
    PREQUEST_CONTEXT pRequest;
    NTSTATUS status;

    pRequest = GetRequestContext(pDevice->SpbRequest);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_FULL_DUPLEX",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = FxRequest;

    //
    // Get input and output buffers.
    //

    status = WdfRequestRetrieveInputBuffer(
        FxRequest,
        0,
        &pInputBuffer,
        &inputBufferLength);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to retrieve input buffer - %!STATUS!",
            status);

        goto Done;
    }

    status = WdfRequestRetrieveOutputBuffer(
        FxRequest,
        0,
        &pOutputBuffer,
        &outputBufferLength);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to retrieve output buffer - %!STATUS!",
            status);

        goto Done;
    }

    //
    // Build full duplex transfer using SPB transfer list.
    //
    
    const ULONG transfers = 2;

    SPB_TRANSFER_LIST_AND_ENTRIES(transfers) seq;
    SPB_TRANSFER_LIST_INIT(&(seq.List), transfers);

    {
        //
        // PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
        // "struct hack" size but using an index variable quiets 
        // the warning. This is a false positive from OACR.
        // 

        ULONG index = 0;
        seq.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionToDevice,
            0,
            pInputBuffer,
            (ULONG)inputBufferLength);

        seq.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionFromDevice,
            0,
            pOutputBuffer,
            (ULONG)outputBufferLength);
    }

    //
    // Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
    // so the memory doesn't have to persist until the request is
    // completed.
    //

    NT_ASSERT(pDevice->InputMemory == WDF_NO_HANDLE);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    status = WdfMemoryCreatePreallocated(
        &attributes,
        (PVOID)&seq,
        sizeof(seq),
        &pDevice->InputMemory);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to create WDFMEMORY - %!STATUS!",
            status);

        goto Done;
    }

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Built full duplex transfer %p with byte length=%lu",
        &seq,
        (ULONG)(inputBufferLength + outputBufferLength));

    //
    // Send full duplex IOCTL.
    //

    //
    // Mark SPB request as full duplex (sequence format)
    // and save length. These will be used in the completion 
    // callback to complete the client request with the correct
    // number of bytes
    //

    pRequest->IsSpbSequenceRequest = TRUE;
    pRequest->SequenceWriteLength = (ULONG_PTR)inputBufferLength;

    //
    // Format and send the full duplex request.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->SpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_FULL_DUPLEX,
        pDevice->InputMemory,
        nullptr,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to format request - %!STATUS!",
            status);

        goto Done;
    }

    status = SpbPeripheralSendRequest(
        pDevice,
        pDevice->SpbRequest,
        FxRequest);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_FULL_DUPLEX - %!STATUS!",
            pDevice->SpbRequest,
            status);

        goto Done;
    }

Done:

    if (!NT_SUCCESS(status))
    {
        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralSignalInterrupt(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
Routine Description:

    This routine signals the interrupt service routine
    to continue.

Arguments:

    pDevice - the device context
    FxRequest - the framework request object

Return Value:

   None.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    NTSTATUS status = STATUS_SUCCESS;

    if (pDevice->Interrupt != nullptr)
    {
        //
        // Signal ISR to continue.
        //

        KeSetEvent(
            &pDevice->IsrWaitEvent,
            IO_NO_INCREMENT,
            FALSE);

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_SPBAPI,
            "Setting ISR wait event");
    }
    else
    {
        status = STATUS_NOT_FOUND;
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "No interrupt object found, ignoring - %!STATUS!",
            status);
    }

    WdfRequestComplete(FxRequest, status);

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralWaitOnInterrupt(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest
    )
/*++
Routine Description:

    This routine pends the WaitOnInterrupt request.

Arguments:

    pDevice - the device context
    FxRequest - the framework request object

Return Value:

   None.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PREQUEST_CONTEXT pRequest = GetRequestContext(FxRequest);
    NTSTATUS status = STATUS_SUCCESS;

    if (pDevice->WaitOnInterruptRequest == nullptr)
    {
        //
        // Mark request cancellable.
        //

        pRequest->FxDevice = pDevice->FxDevice;

        status = WdfRequestMarkCancelableEx(
            FxRequest,
            SpbPeripheralOnWaitOnInterruptCancel);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPBAPI,
                "Failed to mark WaitOnInterrupt request %p cancellable - %!STATUS!",
                FxRequest,
                status);
        }

        //
        // Pend the WaitOnInterrupt request.
        //

        if (NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_SPBAPI,
                "WaitOnInterrupt request %p pended",
                FxRequest);

            pDevice->WaitOnInterruptRequest = FxRequest;
        }
    }
    else
    {
        status = STATUS_INVALID_DEVICE_STATE;
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "Cannont pend multiple WaitOnInterrupt requests, ignoring - %!STATUS!",
            status);
    }

    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(FxRequest, status);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralOnWaitOnInterruptCancel(
    _In_  WDFREQUEST  FxRequest
    )
/*++
Routine Description:

    This event is called when the WaitOnInterrupt request is cancelled.

Arguments:

    FxRequest - the framework request object

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PREQUEST_CONTEXT pRequest;
    PDEVICE_CONTEXT pDevice;

    pRequest = GetRequestContext(FxRequest);
    pDevice = GetDeviceContext(pRequest->FxDevice);

    //
    // Complete the request as cancelled
    //

    if (FxRequest == pDevice->WaitOnInterruptRequest)
    {    
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_SPBAPI,
            "WaitOnInterrupt request %p cancelled",
            FxRequest);

        pDevice->WaitOnInterruptRequest = nullptr;
        WdfRequestComplete(FxRequest, STATUS_CANCELLED);
    }
    else
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "Cancel for WDFREQUEST %p without WaitOnInterrupt request pended,"
            "will complete as cancelled anyway",
            FxRequest);

        WdfRequestComplete(FxRequest, STATUS_CANCELLED);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

BOOLEAN
SpbPeripheralInterruptNotify(
    _In_  PDEVICE_CONTEXT  pDevice
    )
/*++
Routine Description:

    This routine completes the pending WaitOnInterrupt request.

Arguments:

    pDevice - the device context

Return Value:

   TRUE if notification sent, false otherwise.

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    WDFREQUEST request;
    BOOLEAN fNotificationSent = FALSE;
    NTSTATUS status;

    if (pDevice->WaitOnInterruptRequest != nullptr)
    {
        //
        // Complete the WaitOnInterrupt request.
        //

        request = pDevice->WaitOnInterruptRequest;
        pDevice->WaitOnInterruptRequest = nullptr;

        status = WdfRequestUnmarkCancelable(request);

        if (NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_SPBAPI,
                "Interrupt detected, WaitOnInterrupt request %p completed",
                pDevice->WaitOnInterruptRequest);

            WdfRequestComplete(request, STATUS_SUCCESS);
            fNotificationSent = TRUE;
        }
        else if (status == STATUS_CANCELLED)
        {
            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_SPBAPI,
                "Interrupt detected with WaitOnInterrupt request %p "
                "already completed - %!STATUS!",
                pDevice->WaitOnInterruptRequest,
                status);
        }
        else
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPBAPI,
                "Interrupt detected but failed to unmark WaitOnInterrupt "
                "request %p as cancellable - %!STATUS!",
                pDevice->WaitOnInterruptRequest,
                status);
        }
    }
    else
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_SPBAPI,
            "Interrupt detected without a pended WaitOnInterrupt request");
    }

    FuncExit(TRACE_FLAG_SPBAPI);

    return fNotificationSent;
}

NTSTATUS
SpbPeripheralSendRequest(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       SpbRequest,
    _In_  WDFREQUEST       ClientRequest
    )
/*++
 
  Routine Description:

    This routine sends a write-read sequence to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    SpbRequest - the SPB request object
    ClientRequest - the client request object

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    PREQUEST_CONTEXT pRequest = GetRequestContext(ClientRequest);
    NTSTATUS status = STATUS_SUCCESS;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Saving client request %p, and "
        "sending SPB request %p",
        ClientRequest,
        SpbRequest);

    //
    // Init client request context.
    //

    pRequest->FxDevice = pDevice->FxDevice;
    pRequest->IsSpbSequenceRequest = FALSE;
    pRequest->SequenceWriteLength = 0;

    //
    // Mark the client request as cancellable.
    //

    if (NT_SUCCESS(status))
    {
        status = WdfRequestMarkCancelableEx(
            ClientRequest,
            SpbPeripheralOnCancel);
    }

    //
    // Send the SPB request.
    //

    if (NT_SUCCESS(status))
    {
        WdfRequestSetCompletionRoutine(
            SpbRequest,
            SpbPeripheralOnCompletion,
            GetRequestContext(SpbRequest));

        BOOLEAN fSent = WdfRequestSend(
            SpbRequest,
            pDevice->SpbController,
            WDF_NO_SEND_OPTIONS);

        if (!fSent)
        {
            status = WdfRequestGetStatus(SpbRequest);

            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPBAPI,
                "Failed to send SPB request %p - %!STATUS!",
                SpbRequest,
                status);

            NTSTATUS cancelStatus;
            cancelStatus = WdfRequestUnmarkCancelable(ClientRequest);

            if (!NT_SUCCESS(cancelStatus))
            {
                NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if request has already been cancelled",
                    cancelStatus == STATUS_CANCELLED);

                Trace(
                    TRACE_LEVEL_INFORMATION, 
                    TRACE_FLAG_SPBAPI, 
                    "Client request %p has already been cancelled - "
                    "%!STATUS!",
                    ClientRequest,
                    cancelStatus);
            }
        }
    }

    FuncExit(TRACE_FLAG_SPBAPI);

    return status;
}

VOID 
SpbPeripheralOnCompletion(
    _In_  WDFREQUEST                      FxRequest,
    _In_  WDFIOTARGET                     FxTarget,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_  WDFCONTEXT                      Context
    )
/*++
 
  Routine Description:

    This routine is called when a request completes.

  Arguments:

    FxRequest - the framework request object
    FxTarget - the framework IO target object
    Params - a pointer to the request completion parameters
    Context - the request context

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    UNREFERENCED_PARAMETER(FxTarget);
    UNREFERENCED_PARAMETER(Context);
    
    PREQUEST_CONTEXT pRequest;
    PDEVICE_CONTEXT pDevice;
    NTSTATUS status;
    NTSTATUS cancelStatus;
    ULONG_PTR bytesCompleted;

    pRequest = GetRequestContext(FxRequest);
    pDevice = GetDeviceContext(pRequest->FxDevice);

    status = Params->IoStatus.Status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Completion callback received for SPB request %p with %!STATUS!",
        FxRequest,
        status);

    //
    // Unmark the client request as cancellable
    //

    cancelStatus = WdfRequestUnmarkCancelable(pDevice->ClientRequest);

    if (!NT_SUCCESS(cancelStatus))
    {
        NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if request has already been cancelled",
            cancelStatus == STATUS_CANCELLED);

        Trace(
            TRACE_LEVEL_INFORMATION, 
            TRACE_FLAG_SPBAPI, 
            "Client request %p has already been cancelled - %!STATUS!",
            pDevice->ClientRequest,
            cancelStatus);
    }

    //
    // Complete the request pair
    //

    if (pRequest->IsSpbSequenceRequest == TRUE)
    {
        //
        // The client DeviceIoControl should only be
        // completed with bytesReturned and not total
        // bytes transferred.  Here we infer the number
        // of bytes read by substracting the write
        // length from the total.
        //

        bytesCompleted = 
            Params->IoStatus.Information < pRequest->SequenceWriteLength ?
            0 : Params->IoStatus.Information - pRequest->SequenceWriteLength;
    }
    else
    {
        bytesCompleted = Params->IoStatus.Information;
    }

    SpbPeripheralCompleteRequestPair(
        pDevice,
        status,
        bytesCompleted);
    
    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralOnCancel(
    _In_  WDFREQUEST  FxRequest
    )
/*++
Routine Description:

    This event is called when the client request is cancelled.

Arguments:

    FxRequest - the framework request object

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PREQUEST_CONTEXT pRequest;
    PDEVICE_CONTEXT pDevice;

    pRequest = GetRequestContext(FxRequest);
    pDevice = GetDeviceContext(pRequest->FxDevice);

    //
    // Attempt to cancel the SPB request
    //
    
    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Cancel received for client request %p, "
        "attempting to cancel SPB request %p",
        FxRequest,
        pDevice->SpbRequest);

    WdfRequestCancelSentRequest(pDevice->SpbRequest);

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralCompleteRequestPair(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  NTSTATUS         status,
    _In_  ULONG_PTR        bytesCompleted
    )
/*++
Routine Description:

    This routine marks the SpbRequest as reuse
    and completes the client request.

Arguments:

    pDevice - the device context
    status - the client completion status
    bytesCompleted - the number of bytes completed
        for the client request

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PREQUEST_CONTEXT pRequest;
    pRequest = GetRequestContext(pDevice->SpbRequest);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Marking SPB request %p for reuse, and completing "
        "client request %p with %!STATUS! and bytes=%lu",
        pDevice->SpbRequest,
        pDevice->ClientRequest,
        status,
        (ULONG)bytesCompleted);

    //
    // Mark the SPB request as reuse
    //

    pRequest->IsSpbSequenceRequest = FALSE;
    pRequest->SequenceWriteLength = 0;

    WDF_REQUEST_REUSE_PARAMS params;
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &params,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    WdfRequestReuse(pDevice->SpbRequest, &params);

    if (pDevice->InputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(pDevice->InputMemory);
        pDevice->InputMemory = WDF_NO_HANDLE;
    }

    //
    // Complete the client request
    //

    if (pDevice->ClientRequest != nullptr)
    {
        WDFREQUEST clientRequest = pDevice->ClientRequest;
        pDevice->ClientRequest = nullptr;

        //
        // In order to satisfy SDV, assume clientRequest
        // is equal to pDevice->ClientRequest. This suppresses
        // a warning in the driver's cancellation path. 
        //
        // Typically when WdfRequestUnmarkCancelable returns 
        // STATUS_CANCELLED a driver does not go on to complete 
        // the request in that context. This sample, however, 
        // driver has handled this condition appropriately by 
        // not completing the cancelled request in its 
        // EvtRequestCancel callback. Developers should be 
        // cautious when copying code from this sample, paying 
        // close attention to the cancellation logic.
        //
        _Analysis_assume_(clientRequest == pDevice->ClientRequest);
        
        WdfRequestCompleteWithInformation(
            clientRequest,
            status,
            bytesCompleted);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}
