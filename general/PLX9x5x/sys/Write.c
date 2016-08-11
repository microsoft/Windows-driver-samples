/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Write.c

Abstract:


Environment:

    Kernel mode

--*/

#include "precomp.h"

#include "Write.tmh"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID
PLxEvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t            Length
    )
/*++

Routine Description:

    Called by the framework as soon as it receives a write request.
    If the device is not ready, fail the request.
    Otherwise get scatter-gather list for this request and send the
    packet to the hardware for DMA.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

--*/
{
    NTSTATUS          status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION devExt = NULL;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                "--> PLxEvtIoWrite: Request %p", Request);

    //
    // Get the DevExt from the Queue handle
    //
    devExt = PLxGetDeviceContext(WdfIoQueueGetDevice(Queue));

    //
    // Validate the Length parameter.
    //
    if (Length > PCI9656_SRAM_SIZE)  {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto CleanUp;
    }

    //
    // Set the transaction's Single Transfer Requirement if
    // the user application asked for it. This needs to be
    // set before initializing the transaction.
    //
    WdfDmaTransactionSetSingleTransferRequirement(
        devExt->WriteDmaTransaction,
        devExt->RequireSingleTransfer);

    //
    // Following code illustrates two different ways of initializing a DMA
    // transaction object. If ASSOC_WRITE_REQUEST_WITH_DMA_TRANSACTION is
    // defined in the sources file, the first section will be used.
    //
#ifdef ASSOC_WRITE_REQUEST_WITH_DMA_TRANSACTION

    //
    // This section illustrates how to create and initialize
    // a DmaTransaction using a WDF Request.
    // This type of coding pattern would probably be the most commonly used
    // for handling client Requests.
    //
    status = WdfDmaTransactionInitializeUsingRequest(
                                           devExt->WriteDmaTransaction,
                                           Request,
                                           PLxEvtProgramWriteDma,
                                           WdfDmaDirectionWriteToDevice );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "WdfDmaTransactionInitializeUsingRequest failed: "
                    "%!STATUS!", status);
        goto CleanUp;
    }
#else
    //
    // This section illustrates how to create and initialize
    // a DmaTransaction via direct parameters (e.g. not using a WDF Request).
    // This type of coding pattern might be used for driver-initiated DMA
    // operations (e.g. DMA operations not based on driver client requests.)
    //
    // NOTE: This example unpacks the WDF Request in order to get a set of
    //       parameters for the call to WdfDmaTransactionInitialize. While
    //       this is completely legimate, the represenative usage pattern
    //       for WdfDmaTransactionIniitalize would have the driver create/
    //       initialized a DmaTransactin without a WDF Request. A simple
    //       example might be where the driver needs to DMA the devices's
    //       firmware to it during device initialization. There would be
    //       no WDF Request; the driver would supply the parameters for
    //       WdfDmaTransactionInitialize directly.
    //
    {
        PTRANSACTION_CONTEXT  transContext;
        PMDL                  mdl;
        PVOID                 virtualAddress;
        size_t                length;

#ifndef SIMULATE_MEMORY_FRAGMENTATION
        //
        // Initialize this new DmaTransaction with direct parameters.
        //
        status = WdfRequestRetrieveInputWdmMdl(Request, &mdl);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "WdfRequestRetrieveInputWdmMdl failed: %!STATUS!", status);
            goto CleanUp;
        }

        virtualAddress = MmGetMdlVirtualAddress(mdl);
        length = MmGetMdlByteCount(mdl);
#else
        PVOID buffer;

        //
        // For illustrative purposes, copy the Request's memory to a
        // heavily fragmented MDL chain. If RequireSingleTransfer was set,
        // then KMDF will attempt to fulfill the DMA transaction in a single
        // transfer. Otherwise, the transaction may require several operations
        // in which case PLxEvtProgramWriteDma will be invoked multiple times.
        //
        status = WdfRequestRetrieveInputBuffer(Request, 0, &buffer, &length);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "WdfRequestRetrieveInputBuffer failed: %!STATUS!", status);
            goto CleanUp;
        }

        mdl = devExt->WriteMdlChain;
        virtualAddress = MmGetMdlVirtualAddress(mdl);
        CopyBufferToMdlChain(buffer, length, mdl);
#endif // SIMULATE_MEMORY_FRAGMENTATION

        _Analysis_assume_(length > 0);
        status = WdfDmaTransactionInitialize( devExt->WriteDmaTransaction,
                                              PLxEvtProgramWriteDma,
                                              WdfDmaDirectionWriteToDevice,
                                              mdl,
                                              virtualAddress,
                                              length );

        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "WdfDmaTransactionInitialize failed: %!STATUS!", status);
              goto CleanUp;
        }

        //
        // Retreive this DmaTransaction's context ptr (aka TRANSACTION_CONTEXT)
        // and fill it in with info.
        //
        transContext = PLxGetTransactionContext( devExt->WriteDmaTransaction );
        transContext->Request = Request;
    }
#endif // ASSOC_WRITE_REQUEST_WITH_DMA_TRANSACTION

#if 0 //FYI
        //
        // Modify the MaximumLength for this DmaTransaction only.
        //
        // Note: The new length must be less than or equal to that set when
        //       the DmaEnabler was created.
        //
        {
            ULONG length =  devExt->MaximumTransferLength / 2;

            //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
            //            "Setting a new MaxLen %d", length);

            WdfDmaTransactionSetMaximumLength( devExt->WriteDmaTransaction, length );
        }
#endif

    //
    // Execute this DmaTransaction transaction.
    //
    status = WdfDmaTransactionExecute( devExt->WriteDmaTransaction, 
                                       WDF_NO_CONTEXT);

    if(!NT_SUCCESS(status)) {

        //
        // Couldn't execute this DmaTransaction, so fail Request.
        //
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "WdfDmaTransactionExecute failed: %!STATUS!", status);
        goto CleanUp;
    }

    //
    // Indicate that Dma transaction has been started successfully. The request
    // will be complete by the Dpc routine when the DMA transaction completes.
    //
    status = STATUS_SUCCESS;

CleanUp:

    //
    // If there are errors, then clean up and complete the Request.
    //
    if (!NT_SUCCESS(status)) {
        WdfDmaTransactionRelease(devExt->WriteDmaTransaction);        
        WdfRequestComplete(Request, status);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                "<-- PLxEvtIoWrite: %!STATUS!", status);

    return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN
PLxEvtProgramWriteDma(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  PVOID                   Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    SgList
    )
/*++

Routine Description:

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

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                "--> PLxEvtProgramWriteDma");

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
    dteVA = (PDMA_TRANSFER_ELEMENT) devExt->WriteCommonBufferBase;
    dteLA = (devExt->WriteCommonBufferBaseLA.LowPart +
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
        //       where this Write will start.
        //
        dteVA->PciAddressLow  = SgList->Elements[i].Address.LowPart;
        dteVA->PciAddressHigh = SgList->Elements[i].Address.HighPart;
        dteVA->TransferSize   = SgList->Elements[i].Length;

        dteVA->LocalAddress   = (ULONG) offset;

        dteVA->DescPtr.DescLocation  = DESC_PTR_DESC_LOCATION__PCI;
        dteVA->DescPtr.TermCountInt  = FALSE;
        dteVA->DescPtr.LastElement   = FALSE;
        dteVA->DescPtr.DirOfTransfer = DESC_PTR_DIRECTION__TO_DEVICE;
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

#if 0 // set to 1 for recording the details
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                        "\tDTE[%d] : Addr #%X%08X Len %5d, Local %08X, "
                        "Loc(%d), Last(%d), TermInt(%d), ToPci(%d)\n",
                        i,
                        dteVA->PciAddressHigh,
                        dteVA->PciAddressLow,
                        dteVA->TransferSize,
                        dteVA->LocalAddress,
                        dteVA->DescPtr.DescLocation,
                        dteVA->DescPtr.LastElement,
                        dteVA->DescPtr.TermCountInt,
                        dteVA->DescPtr.DirOfTransfer );
#endif
            break;
        }

#if 0 // set to 1 for recording the details
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                    "\tDTE[%d] : Addr #%X%08X Len %5d, Local %08X, "
                    "Loc(%d), Last(%d), TermInt(%d), ToPci(%d)\n",
                    i,
                    dteVA->PciAddressHigh,
                    dteVA->PciAddressLow,
                    dteVA->TransferSize,
                    dteVA->LocalAddress,
                    dteVA->DescPtr.DescLocation,
                    dteVA->DescPtr.LastElement,
                    dteVA->DescPtr.TermCountInt,
                    dteVA->DescPtr.DirOfTransfer );
#endif

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
    // DMA 0 Mode Register - (DMAMODE0)
    // Enable Scatter/Gather Mode, Interrupt On Done,
    // and route Ints to PCI.
    //
    {
        union {
            DMA_MODE  bits;
            ULONG     ulong;
        } dmaMode;

        dmaMode.ulong =
            READ_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma0_Mode );

        dmaMode.bits.SgModeEnable  = TRUE;
        dmaMode.bits.DoneIntEnable = TRUE;
        dmaMode.bits.IntToPci      = TRUE;

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma0_Mode,
                              dmaMode.ulong );
    }

    //
    // Interrupt CSR Register - (INTCSR)
    // Enable PCI Ints and DMA Channel 0 Ints.
    //
    {
        union {
            INT_CSR   bits;
            ULONG     ulong;
        } intCSR;

        intCSR.ulong =
            READ_REGISTER_ULONG( (PULONG) &devExt->Regs->Int_Csr );

        intCSR.bits.PciIntEnable      = TRUE;
        intCSR.bits.DmaChan0IntEnable = TRUE;

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Int_Csr,
                              intCSR.ulong );
    }

    //
    // DMA 0 Descriptor Pointer Register - (DMADPR0)
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
            DESC_PTR_ADDR( devExt->WriteCommonBufferBaseLA.LowPart );

        WRITE_REGISTER_ULONG( (PULONG) &devExt->Regs->Dma0_Desc_Ptr,
                              ptr.ulong );
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                "    PLxEvtProgramWriteDma: Start a Write DMA operation");

    //
    // DMA 0 CSR Register - (DMACSR0)
    // Start the DMA operation: Set Enable and Start bits.
    //
    {
        union {
            DMA_CSR  bits;
            UCHAR    uchar;
        } dmaCSR;

        dmaCSR.uchar =
            READ_REGISTER_UCHAR( (PUCHAR) &devExt->Regs->Dma0_Csr );

        dmaCSR.bits.Enable = TRUE;
        dmaCSR.bits.Start  = TRUE;

        WRITE_REGISTER_UCHAR( (PUCHAR) &devExt->Regs->Dma0_Csr,
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
        //
        // Must abort the transaction before deleting it.
        //
        NTSTATUS status;

        (VOID) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        ASSERT(NT_SUCCESS(status));
        PLxWriteRequestComplete( Transaction, STATUS_INVALID_DEVICE_STATE );
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "<-- PLxEvtProgramWriteDma: error ****");
        return FALSE;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE,
                "<-- PLxEvtProgramWriteDma");

    return TRUE;
}


VOID
PLxWriteRequestComplete(
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

    //
    // Initialize locals
    //

#ifdef ASSOC_WRITE_REQUEST_WITH_DMA_TRANSACTION

    request = WdfDmaTransactionGetRequest(DmaTransaction);

#else
    //
    // If CreateDirect was used then there will be no assoc. Request.
    //
    {
        PTRANSACTION_CONTEXT transContext = PLxGetTransactionContext(DmaTransaction);

        request = transContext->Request;
        transContext->Request = NULL;
        
    }
#endif

    //
    // Get the final bytes transferred count.
    //
    bytesTransferred =  WdfDmaTransactionGetBytesTransferred( DmaTransaction );

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_DPC,
                "PLxWriteRequestComplete:  Request %p, Status %!STATUS!, "
                "bytes transferred %d\n",
                 request, Status, (int) bytesTransferred );

    WdfDmaTransactionRelease(DmaTransaction);        

    WdfRequestCompleteWithInformation( request, Status, bytesTransferred);

}

