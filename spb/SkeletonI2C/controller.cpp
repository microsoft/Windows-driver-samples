/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    controller.cpp

Abstract:

    This module contains the controller-specific functions
    for handling transfers and implementing interrupts.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "device.h"

#include "controller.tmh"

const PBC_TRANSFER_SETTINGS g_TransferSettings[] = 
{
    // TODO: Update this array to reflect changes
    //       made to the PBC_TRANSFER_SETTINGS
    //       structure in internal.h.

    // Bus condition        IsStart  IsEnd
    {BusConditionDontCare,  FALSE,   FALSE}, // SpbRequestTypeInvalid
    {BusConditionFree,      TRUE,    TRUE},  // SpbRequestTypeSingle
    {BusConditionFree,      TRUE,    FALSE}, // SpbRequestTypeFirst
    {BusConditionBusy,      FALSE,   FALSE}, // SpbRequestTypeContinue
    {BusConditionBusy,      FALSE,   TRUE}   // SpbRequestTypeLast
};

VOID
ControllerInitialize(
    _In_  PPBC_DEVICE  pDevice
    )
/*++
 
  Routine Description:

    This routine initializes the controller hardware.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_PBCLOADING);
                        
    NT_ASSERT(pDevice != NULL);

    // TODO: Initialize controller hardware via the
    //       pDevice->pRegisters->* register interface.
    //       Work may include configuring operating modes,
    //       FIFOs, clock, interrupts, etc.

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_PBCLOADING);
}

VOID
ControllerUninitialize(
    _In_  PPBC_DEVICE  pDevice
    )
/*++
 
  Routine Description:

    This routine uninitializes the controller hardware.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_PBCLOADING);

    NT_ASSERT(pDevice != NULL);

    // TODO: Uninitialize controller hardware via the
    //       pDevice->pRegisters->* register interface
    //       if necessary.  Work may include disabling
    //       interrupts, etc.

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_PBCLOADING);
}

VOID
ControllerConfigureForTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest
    )
/*++
 
  Routine Description:

    This routine configures and starts the controller
    for a transfer.

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
    // Initialize request context for transfer.
    //

    pRequest->Settings = g_TransferSettings[pRequest->SequencePosition];
    pRequest->Status = STATUS_SUCCESS;

    //
    // Configure hardware for transfer.
    //

    // TODO: Initialize controller hardware for a general
    //       transfer via the pDevice->pRegisters->* register 
    //       interface.  Work may include setting up transfer, 
    //       configuring  FIFOs, selecting address, etc.

    if (pRequest->Settings.IsStart)
    {
        // TODO: Perform any action to program a start bit.
    }
    else if (pRequest->Settings.IsEnd)
    {
        // TODO: Perform any action to program a stop bit.
    }

    if (pRequest->Direction == SpbTransferDirectionToDevice)
    {
        // TODO: Perform write-specific configuration,
        //       i.e. pRequest->DataReadyFlag = ...
    }
    else if (pRequest->Direction == SpbTransferDirectionFromDevice)
    {
        // TODO: Perform read-specific configuration,
        //       i.e. pRequest->DataReadyFlag = ...
    }

    //
    // Synchronize access to device context with ISR.
    //

    // TODO: Uncomment when using interrupts.
    //WdfInterruptAcquireLock(pDevice->InterruptObject);

    //
    // Set interrupt mask and clear current status.
    //

    // TODO: Save desired interrupt mask.
    //       PbcDeviceSetInterruptMask(pDevice, mask)

    pDevice->InterruptStatus = 0;

    Trace(
        TRACE_LEVEL_VERBOSE,
        TRACE_FLAG_TRANSFER,
        "Controller configured for %s of %Iu bytes to address 0x%lx "
        "(SPBREQUEST %p, WDFDEVICE %p)",
        pRequest->Direction == SpbTransferDirectionFromDevice ? "read" : "write",
        pRequest->Length,
        pDevice->pCurrentTarget->Settings.Address,
        pRequest->SpbRequest,
        pDevice->FxDevice);

    // TODO: Perform necessary action to begin transfer.

    ControllerEnableInterrupts(
        pDevice, 
        PbcDeviceGetInterruptMask(pDevice));

    // TODO: Uncomment when using interrupts.
    //WdfInterruptReleaseLock(pDevice->InterruptObject);

    // TODO: For the purpose of this skeleton sample,
    //       simply complete the request synchronously.

    ControllerCompleteTransfer(pDevice, pRequest, FALSE);

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerProcessInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  ULONG         InterruptStatus
    )
/*++
 
  Routine Description:

    This routine processes a hardware interrupt. Activities
    include checking for errors and transferring data.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context
    InterruptStatus - saved interrupt status bits from the ISR.
        These have already been acknowledged and disabled

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NTSTATUS status;

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Ready to process interrupts with status 0x%lx for WDFDEVICE %p",
        InterruptStatus,
        pDevice->FxDevice);
    
    //
    // Check for address NACK.
    //

    if (TestAnyBits(InterruptStatus, SI2C_STATUS_ADDRESS_NACK /*update with nack flag*/))
    {        
        //
        // An address NACK indicates that a device is
        // not present at that address or is not responding.
        // Set the error status accordingly.
        //
        
        pRequest->Status = STATUS_NO_SUCH_DEVICE;
        pRequest->Information = 0;
        
        // TODO: Perform any additional action needed to handle NACK.

        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_TRANSFER,
            "NACK on address 0x%lx (WDFDEVICE %p) - %!STATUS!",
            pDevice->pCurrentTarget->Settings.Address,
            pDevice->FxDevice,
            pRequest->Status);

        //
        // Complete the transfer and stop processing
        // interrupts.
        //

        ControllerCompleteTransfer(pDevice, pRequest, TRUE);
        goto exit;
    }
    
    //
    // Check for data NACK.
    //

    if (TestAnyBits(InterruptStatus, SI2C_STATUS_DATA_NACK /*update with nack flag*/))
    {        
        //
        // A data NACK is not necessarily an error.
        // Set the error status to STATUS_SUCCESS and
        // indicate the number of bytes successfully
        // transferred in the information field. The
        // client will determine success or failure of
        // the IO based on this length.
        //
        
        pRequest->Status = STATUS_SUCCESS;
        
        // TODO: Assuming this info is available, set
        //       information to the actual number of
        //       bytes successfully transferred.
        //pRequest->Information = 0;
        
        // TODO: Perform any additional action needed to handle NACK.

        Trace(
            TRACE_LEVEL_WARNING, 
            TRACE_FLAG_TRANSFER,
            "NACK after %Iu bytes transferred for address 0x%lx "
            "(WDFDEVICE %p)- %!STATUS!", 
            pRequest->Information, 
            pDevice->pCurrentTarget->Settings.Address,
            pDevice->FxDevice,
            pRequest->Status);

        //
        // Complete the transfer and stop processing
        // interrupts.
        //

        ControllerCompleteTransfer(pDevice, pRequest, TRUE);
        goto exit;
    }

    // TODO: Check for other errors.  

    if (TestAnyBits(InterruptStatus, SI2C_STATUS_GENERIC_ERROR /*update with error flag*/))
    {
        // TODO: Perform any action needed to handle error,
        //       i.e. set status or bytes transferred accordingly.
        
        pRequest->Status = STATUS_UNSUCCESSFUL;
        pRequest->Information = 0;

        Trace(
            TRACE_LEVEL_WARNING, 
            TRACE_FLAG_TRANSFER,
            "Error after %Iu bytes transferred for address 0x%lx "
            "(WDFDEVICE %p)- %!STATUS!", 
            pRequest->Information, 
            pDevice->pCurrentTarget->Settings.Address,
            pDevice->FxDevice,
            pRequest->Status);

        //
        // Complete the transfer and stop processing
        // interrupts.
        //

        ControllerCompleteTransfer(pDevice, pRequest, TRUE);
        goto exit;
    }

    //
    // Check if controller is ready to transfer more data.
    //

    if (TestAnyBits(InterruptStatus, pRequest->DataReadyFlag))
    {
        //
        // Transfer data.
        //

        status = ControllerTransferData(pDevice, pRequest);

        if (!NT_SUCCESS(status))
        {
            pRequest->Status = status;

            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_TRANSFER,
                "Unexpected error while transferring data for address 0x%lx, "
                "completing transfer and resetting controller - %!STATUS!",
                pDevice->pCurrentTarget->Settings.Address,
                pRequest->Status);

            //
            // Complete the transfer and stop processing
            // interrupts.
            //

            ControllerCompleteTransfer(pDevice, pRequest, TRUE);
            goto exit;
        }

        //
        // If finished transferring data, stop listening for
        // data ready interrupt.  Do not complete transfer
        // until transfer complete interrupt occurs.
        //

        if (PbcRequestGetInfoRemaining(pRequest) == 0)
        {
            Trace(
                TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_TRANSFER,
                "No bytes remaining in transfer for address 0x%lx, wait for "
                "transfer complete interrupt",
                pDevice->pCurrentTarget->Settings.Address);

            PbcDeviceAndInterruptMask(pDevice, ~pRequest->DataReadyFlag);
        }
    }

    //
    // Check if transfer is complete.
    //

    if (TestAnyBits(InterruptStatus, 0 /*update with transfer complete flag*/))
    {
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_TRANSFER,
            "Transfer complete for address 0x%lx with %Iu bytes remaining",
            pDevice->pCurrentTarget->Settings.Address,
            PbcRequestGetInfoRemaining(pRequest));
        
        //
        // If transfer complete interrupt occured and there
        // are still bytes remaining, transfer data. This occurs
        // when the number of bytes remaining is less than
        // the FIFO transfer level to trigger a data ready interrupt.
        //

        if (PbcRequestGetInfoRemaining(pRequest) > 0)
        {
            status = ControllerTransferData(pDevice, pRequest);

            if (!NT_SUCCESS(status))
            {
                pRequest->Status = status;

                Trace(
                    TRACE_LEVEL_ERROR, 
                    TRACE_FLAG_TRANSFER,
                    "Unexpected error while transferring data for address 0x%lx, "
                    "completing transfer and resetting controller "
                    "(WDFDEVICE %p) - %!STATUS!",
                    pDevice->pCurrentTarget->Settings.Address,
                    pDevice->FxDevice,
                    pRequest->Status);

                //
                // Complete the transfer and stop processing
                // interrupts.
                //

                ControllerCompleteTransfer(pDevice, pRequest, TRUE);
                goto exit;
            }
        }

        //
        // Complete the transfer.
        //

        ControllerCompleteTransfer(pDevice, pRequest, FALSE);
    }

exit:

    FuncExit(TRACE_FLAG_TRANSFER);
}

NTSTATUS
ControllerTransferData(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest
    )
/*++
 
  Routine Description:

    This routine transfers data to or from the device.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    UNREFERENCED_PARAMETER(pDevice);

    size_t bytesToTransfer = 0;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Write
    //

    if (pRequest->Direction == SpbTransferDirectionToDevice)
    {
        Trace(
            TRACE_LEVEL_INFORMATION, 
            TRACE_FLAG_TRANSFER,
            "Ready to write %Iu byte(s) for address 0x%lx", 
            bytesToTransfer,
            pDevice->pCurrentTarget->Settings.Address);

        // TODO: Perform write. May need to use 
        //       PbcRequestGetByte() or some variation.
    }

    //
    // Read
    //

    else
    {

        Trace(
            TRACE_LEVEL_INFORMATION, 
            TRACE_FLAG_TRANSFER,
            "Ready to read %Iu byte(s) for address 0x%lx", 
            bytesToTransfer,
            pDevice->pCurrentTarget->Settings.Address);

        // TODO: Perform read. May need to use 
        //       PbcRequestSetByte() or some variation.
    }

    //
    // Update request context with bytes transferred.
    //

    pRequest->Information += bytesToTransfer;

    FuncExit(TRACE_FLAG_TRANSFER);
    
    return status;
}

VOID
ControllerCompleteTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  BOOLEAN       AbortSequence
    )
/*++
 
  Routine Description:

    This routine completes a data transfer. Unless there are
    more transfers remaining in the sequence, the request is
    completed.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context
    AbortSequence - specifies whether the driver should abort the
        ongoing sequence or begin the next transfer

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Transfer (index %lu) %s with %Iu bytes for address 0x%lx "
        "(SPBREQUEST %p)",
        pRequest->TransferIndex,
        NT_SUCCESS(pRequest->Status) ? "complete" : "error",
        pRequest->Information,
        pDevice->pCurrentTarget->Settings.Address,
        pRequest->SpbRequest);

    //
    // Update request context with information from this transfer.
    //

    pRequest->TotalInformation += pRequest->Information;
    pRequest->Information = 0;

    //
    // Check if there are more transfers
    // in the sequence.
    //

    if (!AbortSequence)
    {
        pRequest->TransferIndex++;

        if (pRequest->TransferIndex < pRequest->TransferCount)
        {
            //
            // Configure the request for the next transfer.
            //

            pRequest->Status = PbcRequestConfigureForIndex(
                pRequest, 
                pRequest->TransferIndex);

            if (NT_SUCCESS(pRequest->Status))
            {
                //
                // Configure controller and kick-off read.
                // Request will be completed asynchronously.
                //

                PbcRequestDoTransfer(pDevice,pRequest);
                goto exit;
            }
        }
    }

    //
    // If not already cancelled, unmark request cancellable.
    //

    if (pRequest->Status != STATUS_CANCELLED)
    {
        NTSTATUS cancelStatus;
        cancelStatus = WdfRequestUnmarkCancelable(pRequest->SpbRequest);

        if (!NT_SUCCESS(cancelStatus))
        {
            //
            // WdfRequestUnmarkCancelable should only fail if the request
            // has already been or is about to be cancelled. If it does fail 
            // the request must NOT be completed - the cancel callback will do
            // this.
            //

            NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if the request has already been or is about to be cancelled",
                cancelStatus == STATUS_CANCELLED);

            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_TRANSFER,
                "Failed to unmark SPBREQUEST %p as cancelable - %!STATUS!",
                pRequest->SpbRequest,
                cancelStatus);

            goto exit;
        }
    }

    //
    // Done or error occurred. Set interrupt mask to 0.
    // Doing this keeps the DPC from re-enabling interrupts.
    //

    PbcDeviceSetInterruptMask(pDevice, 0);

    //
    // Clear the target's current request. This will prevent
    // the request context from being accessed once the request
    // is completed (and the context is invalid).
    //

    pDevice->pCurrentTarget->pCurrentRequest = NULL;

    //
    // Clear the controller's current target if any of
    //   1. request is type sequence
    //   2. request position is single 
    //      (did not come between lock/unlock)
    // Otherwise wait until unlock.
    //

    if ((pRequest->Type == SpbRequestTypeSequence) ||
        (pRequest->SequencePosition == SpbRequestSequencePositionSingle))
    {
        pDevice->pCurrentTarget = NULL;
    }

    //
    // Mark the IO complete. Request not
    // completed here.
    //

    pRequest->bIoComplete = TRUE;

exit:

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerEnableInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask
    )
/*++
 
  Routine Description:

    This routine enables the hardware interrupts for the
    specificed mask.

  Arguments:

    pDevice - a pointer to the PBC device context
    InterruptMask - interrupt bits to enable

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice != NULL);

    Trace(
        TRACE_LEVEL_VERBOSE,
        TRACE_FLAG_TRANSFER,
        "Enable interrupts with mask 0x%lx (WDFDEVICE %p)",
        InterruptMask,
        pDevice->FxDevice);

    // TODO: Enable interrupts as requested.

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerDisableInterrupts(
    _In_  PPBC_DEVICE   pDevice
    )
/*++
 
  Routine Description:

    This routine disables all controller interrupts.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice != NULL);

    // TODO: Disable all interrupts.

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_TRANSFER);
}

ULONG
ControllerGetInterruptStatus(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask
    )
/*++
 
  Routine Description:

    This routine gets the interrupt status of the
    specificed interrupt bits.

  Arguments:

    pDevice - a pointer to the PBC device context
    InterruptMask - interrupt bits to check

  Return Value:

    A bitmap indicating which interrupts are set.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    ULONG interruptStatus = 0;

    NT_ASSERT(pDevice != NULL);

    // TODO: Check if any of the interrupt mask
    //       bits have triggered an interrupt.
    
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(InterruptMask);

    FuncExit(TRACE_FLAG_TRANSFER);

    return interruptStatus;
}

VOID
ControllerAcknowledgeInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask
    )
/*++
 
  Routine Description:

    This routine acknowledges the
    specificed interrupt bits.

  Arguments:

    pDevice - a pointer to the PBC device context
    InterruptMask - interrupt bits to acknowledge

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice != NULL);

    // TODO: Acknowledge requested interrupts.
    
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(InterruptMask);

    FuncExit(TRACE_FLAG_TRANSFER);
}
