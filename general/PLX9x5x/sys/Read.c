/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Read.c

Abstract:


Environment:

    Kernel mode

--*/

#include "precomp.h"

#include "Read.tmh"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID
PLxEvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t            Length
    )
/*++

Routine Description:

    Called by the framework as soon as it receives a read request.
    If the device is not ready, fail the request.
    Otherwise get scatter-gather list for this request and send the
    packet to the hardware for DMA.

Arguments:

    Queue      - Default queue handle
    Request    - Handle to the write request
    Lenght - Length of the data buffer associated with the request.
                     The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

--*/
{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION       devExt;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                "--> PLxEvtIoRead: Request %p", Request);

    //
    // Get the DevExt from the Queue handle
    //
    devExt = PLxGetDeviceContext(WdfIoQueueGetDevice(Queue));

    do {
        //
        // Validate the Length parameter.
        //
        if (Length > PCI9656_SRAM_SIZE)  {
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // Set the transaction's Single Transfer Requirement if
        // the user application asked for it. This needs to be
        // set before initializing the transaction.
        //
        WdfDmaTransactionSetSingleTransferRequirement(
            devExt->ReadDmaTransaction,
            devExt->RequireSingleTransfer);

#ifndef SIMULATE_MEMORY_FRAGMENTATION
        //
        // Initialize this new DmaTransaction.
        //
        status = WdfDmaTransactionInitializeUsingRequest(
                                              devExt->ReadDmaTransaction,
                                              Request,
                                              PLxEvtProgramReadDma,
                                              WdfDmaDirectionReadFromDevice );

        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                        "WdfDmaTransactionInitializeUsingRequest "
                        "failed: %!STATUS!", status);
            break;
        }
#else
        PMDL mdl;
        PVOID virtualAddress;
        PTRANSACTION_CONTEXT context;

        //
        // For illustrative purposes, initialize the DMA transaction with a
        // heavily fragmented MDL chain instead of the request's output buffer.
        // If RequireSingleTransfer was set, then KMDF will attempt to fulfill
        // the DMA transaction in a single transfer. Otherwise, the transaction
        // may require several operations in which case PLxEvtProgramReadDma
        // will be invoked multiple times.
        //
        mdl = devExt->ReadMdlChain;
        virtualAddress = MmGetMdlVirtualAddress(mdl);

        status = WdfDmaTransactionInitialize(devExt->ReadDmaTransaction,
                                             PLxEvtProgramReadDma,
                                             WdfDmaDirectionReadFromDevice,
                                             mdl,
                                             virtualAddress,
                                             Length);
        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "WdfDmaTransactionInitialize failed: %!STATUS!", status);
            break;
        }

        //
        // Get this DMA transaction's context and save the Request handle
        // to it, since the Request isn't otherwise associated with the
        // transaction object.
        //
        context = PLxGetTransactionContext(devExt->ReadDmaTransaction);
        context->Request = Request;
#endif // SIMULATE_MEMORY_FRAGMENTATION

#if 0 // FYI
        //
        // Modify the MaximumLength for this DmaTransaction only.
        //
        // Note: The new length must be less than or equal to that set when
        //       the DmaEnabler was created.
        //
        {
            ULONG length =  devExt->MaximumTransferLength / 2;

            //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
            //            "Setting a new MaxLen %d\n", length);

            WdfDmaTransactionSetMaximumLength( devExt->ReadDmaTransaction, 
                                               length );
        }
#endif

        //
        // Execute this DmaTransaction.
        //
        status = WdfDmaTransactionExecute( devExt->ReadDmaTransaction, 
                                           WDF_NO_CONTEXT);

        if(!NT_SUCCESS(status)) {
            //
            // Couldn't execute this DmaTransaction, so fail Request.
            //
            TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                        "WdfDmaTransactionExecute failed: %!STATUS!", status);
            break;
        }

        //
        // Indicate that Dma transaction has been started successfully.
        // The request will be complete by the Dpc routine when the DMA
        // transaction completes.
        //
        status = STATUS_SUCCESS;

    } while (0);

    //
    // If there are errors, then clean up and complete the Request.
    //
    if (!NT_SUCCESS(status )) {
        WdfDmaTransactionRelease(devExt->ReadDmaTransaction);
        WdfRequestComplete(Request, status);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                "<-- PLxEvtIoRead: status %!STATUS!", status);

    return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN
PLxEvtProgramReadDma(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  WDFCONTEXT              Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    SgList
    )
/*++

Routine Description:

  The framework calls a driver's EvtProgramDma event callback function
  when the driver calls WdfDmaTransactionExecute and the system has
  enough map registers to do the transfer. The callback function must
  program the hardware to start the transfer. A single transaction
  initiated by calling WdfDmaTransactionExecute may result in multiple
  calls to this function if the buffer is too large and there aren't
  enough map registers to do the whole transfer.


Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION        devExt;
    size_t                   offset;
    PDMA_TRANSFER_ELEMENT    dteVA;
    ULONG_PTR                dteLA;
    BOOLEAN                  errors;
    ULONG                    i;

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Direction );

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                "--> PLxEvtProgramReadDma");

    //
    // Initialize locals
    //
    devExt = PLxGetDeviceContext(Device);
    errors = FALSE;

    //
    // Get the number of bytes as the offset to the beginning of this
    // Dma operations transfer location in the buffer.
    //
    offset = WdfDmaTransactionGetBytesTransferred(Transaction);

    //
    // Setup the pointer to the next DMA_TRANSFER_ELEMENT
    // for both virtual and physical address references.
    //
    dteVA = (PDMA_TRANSFER_ELEMENT) devExt->ReadCommonBufferBase;
    dteLA = (devExt->ReadCommonBufferBaseLA.LowPart +
                        sizeof(DMA_TRANSFER_ELEMENT));

    //
    // Translate the System's SCATTER_GATHER_LIST elements
    // into the device's DMA_TRANSFER_ELEMENT elements.
    //
    for (i=0; i < SgList->NumberOfElements; i++) {

        //
        // Construct this DTE.
        //
        // NOTE: The LocalAddress is the offset into the SRAM from
        //       where this Read will start.
        //
        dteVA->PciAddressLow  = SgList->Elements[i].Address.LowPart;
        dteVA->PciAddressHigh = SgList->Elements[i].Address.HighPart;
        dteVA->TransferSize   = SgList->Elements[i].Length;

        dteVA->LocalAddress   = (ULONG) offset;

        dteVA->DescPtr.DescLocation  = DESC_PTR_DESC_LOCATION__PCI;
        dteVA->DescPtr.TermCountInt  = FALSE;
        dteVA->DescPtr.LastElement   = FALSE;
        dteVA->DescPtr.DirOfTransfer = DESC_PTR_DIRECTION__FROM_DEVICE;
        dteVA->DescPtr.Address       = DESC_PTR_ADDR( dteLA );

        //
        // Increment the DmaTransaction length by this element length
        //
        offset += SgList->Elements[i].Length;

        //
        // If at end of SgList, then set LastElement bit in final NTE.
        //
        if (i == SgList->NumberOfElements - 1) {

            dteVA->DescPtr.LastElement = TRUE;

            //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
            //            "\tDTE[%d] : Addr #%X%08X Len %5d, Local %08X, "
            //            "Loc(%d), Last(%d), TermInt(%d), ToPci(%d)\n",
            //            i,
            //            dteVA->PciAddressHigh,
            //            dteVA->PciAddressLow,
            //            dteVA->TransferSize,
            //            dteVA->LocalAddress,
            //            dteVA->DescPtr.DescLocation,
            //            dteVA->DescPtr.LastElement,
            //            dteVA->DescPtr.TermCountInt,
            //            dteVA->DescPtr.DirOfTransfer );
            break;
        }

        //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
        //            "\tDTE[%d] : Addr #%X%08X Len %5d, Local %08X, "
        //            "Loc(%d), Last(%d), TermInt(%d), ToPci(%d)\n",
        //            i,
        //            dteVA->PciAddressHigh,
        //            dteVA->PciAddressLow,
        //            dteVA->TransferSize,
        //            dteVA->LocalAddress,
        //            dteVA->DescPtr.DescLocation,
        //            dteVA->DescPtr.LastElement,
        //            dteVA->DescPtr.TermCountInt,
        //            dteVA->DescPtr.DirOfTransfer );

        //
        // Adjust the next DMA_TRANSFER_ELEMEMT
        //
        dteVA++;
        dteLA += sizeof(DMA_TRANSFER_ELEMENT);
    }

    //
    // Start the DMA operation.
    // Acquire this device's InterruptSpinLock.
    //
    WdfInterruptAcquireLock( devExt->Interrupt );

    //
    // DMA 1 Mode Register - (DMAMODE1)
    // Enable Scatter/Gather Mode, Interrupt On Done,
    // and route Ints to PCI.
    //
    {
        union {
            DMA_MODE  bits;
            ULONG     ulong;
        } dmaMode;

        dmaMode.ulong =
            READ_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma1_Mode );

        dmaMode.bits.SgModeEnable   = TRUE;
        dmaMode.bits.DoneIntEnable  = TRUE;
        dmaMode.bits.IntToPci       = TRUE;

        dmaMode.bits.ClearCountMode = TRUE;

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma1_Mode,
                              dmaMode.ulong );
    }

    //
    // Interrupt CSR Register - (INTCSR)
    // Enable PCI Ints and DMA Channel 1 Ints.
    //
    {
        union {
            INT_CSR   bits;
            ULONG     ulong;
        } intCSR;

        intCSR.ulong =
            READ_REGISTER_ULONG( (PULONG) &devExt->Regs->Int_Csr );

        intCSR.bits.PciIntEnable      = TRUE;
        intCSR.bits.DmaChan1IntEnable = TRUE;

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Int_Csr,
                              intCSR.ulong );
    }

    //
    // DMA 1 Descriptor Pointer Register - (DMADPR1)
    // Write the base LOGICAL address of the DMA_TRANSFER_ELEMENT list.
    //
    {
        union {
            DESC_PTR  bits;
            ULONG     ulong;
        } ptr;

        ptr.bits.DescLocation = DESC_PTR_DESC_LOCATION__PCI;
        ptr.bits.TermCountInt = TRUE;
        ptr.bits.Address      =
            DESC_PTR_ADDR( devExt->ReadCommonBufferBaseLA.LowPart );

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma1_Desc_Ptr,
                              ptr.ulong );
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                "    PLxEvtProgramReadDma: Start a Read DMA operation");

    //
    // DMA 1 CSR Register - (DMACSR1)
    // Start the DMA operation: Set Enable and Start bits.
    //
    {
        union {
            DMA_CSR  bits;
            UCHAR    uchar;
        } dmaCSR;

        dmaCSR.uchar =
            READ_REGISTER_UCHAR( (PUCHAR) &devExt->Regs->Dma1_Csr );

        dmaCSR.bits.Enable = TRUE;
        dmaCSR.bits.Start  = TRUE;

        WRITE_REGISTER_UCHAR( (PUCHAR) &devExt->Regs->Dma1_Csr,
                              dmaCSR.uchar );
    }

    //
    // Release our interrupt spinlock
    //
    WdfInterruptReleaseLock( devExt->Interrupt );

    //
    // NOTE: This shows how to process errors which occur in the
    //       PFN_WDF_PROGRAM_DMA function in general.
    //       Basically the DmaTransaction must be deleted and
    //       the Request must be completed.
    //
    if (errors) {
        NTSTATUS status;

        //
        // Must abort the transaction before deleting.
        //
        (VOID) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        ASSERT(NT_SUCCESS(status));

        PLxReadRequestComplete( Transaction, STATUS_INVALID_DEVICE_STATE );
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "<-- PLxEvtProgramReadDma: errors ****");
        return FALSE;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_READ,
                "<-- PLxEvtProgramReadDma");

    return TRUE;
}

VOID
PLxReadRequestComplete(
    IN WDFDMATRANSACTION  DmaTransaction,
    IN NTSTATUS           Status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    WDFREQUEST         request;
    size_t             bytesTransferred;

#ifndef SIMULATE_MEMORY_FRAGMENTATION
    //
    // Get the associated request from the transaction.
    //
    request = WdfDmaTransactionGetRequest(DmaTransaction);
#else
    PVOID buffer;
    size_t length;
    WDFDEVICE device;
    PDEVICE_EXTENSION devExt;

    device = WdfDmaTransactionGetDevice(DmaTransaction);
    devExt = PLxGetDeviceContext(device);

    //
    // This means that the DMA transaction was not initialized
    // with WdfDmaTransactionInitializeUsingRequest, so the
    // request was not implicitly associated with it. Instead,
    // we saved it in the object's context space.
    //
    request = PLxGetTransactionContext(DmaTransaction)->Request;

    //
    // Copy the MDL chain's contents back to the original request buffer.
    //
    Status = WdfRequestRetrieveOutputBuffer(request, 0, &buffer, &length);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_READ,
                    "WdfRequestRetrieveOutputBuffer failed: %!STATUS!",
                    Status);
    }
    else {
        CopyMdlChainToBuffer(devExt->ReadMdlChain, buffer, length);
    }
#endif // SIMULATE_MEMORY_FRAGMENTATION

    ASSERT(request);

    //
    // Get the final bytes transferred count.
    //
    bytesTransferred =  WdfDmaTransactionGetBytesTransferred( DmaTransaction );

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_DPC,
                "PLxReadRequestComplete:  Request %p, Status %!STATUS!, "
                "bytes transferred %d\n",
                 request, Status, (int) bytesTransferred );

    WdfDmaTransactionRelease(DmaTransaction);

    //
    // Complete this Request.
    //
    WdfRequestCompleteWithInformation( request, Status, bytesTransferred);

}

