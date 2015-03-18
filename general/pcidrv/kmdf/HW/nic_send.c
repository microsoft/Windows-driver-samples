/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    nic_send.c

Abstract:
    This module contains routines to write packets.

Environment:
    Kernel mode

--*/

#include "precomp.h"

#if defined(EVENT_TRACING)
#include "nic_send.tmh"
#endif

_IRQL_requires_same_
_IRQL_requires_(DISPATCH_LEVEL)
_Requires_lock_held_(FdoData->SendLock)
__inline
VOID
MP_FREE_SEND_PACKET(
    IN  PFDO_DATA   FdoData,
    IN  PMP_TCB     pMpTcb,
    IN  NTSTATUS    Status
    )
/*++
Routine Description:

    Recycle a MP_TCB and complete the packet if necessary

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData
    pMpTcb      Pointer to MP_TCB

Return Value:

    None

--*/
{

    WDFREQUEST          request;
    WDFDMATRANSACTION   dmaTransaction;
    size_t              length;

    ASSERT(MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));

    dmaTransaction = pMpTcb->DmaTransaction;
    pMpTcb->DmaTransaction = NULL;

    MP_CLEAR_FLAGS(pMpTcb);

    FdoData->CurrSendHead = FdoData->CurrSendHead->Next;
    FdoData->nBusySend--;

    request = WdfDmaTransactionGetRequest(dmaTransaction);
    length  = WdfDmaTransactionGetBytesTransferred(dmaTransaction);

    WdfObjectDelete( dmaTransaction );

    if (request)
    {
        WdfSpinLockRelease(FdoData->SendLock);
        WdfRequestCompleteWithInformation(request, Status, length);
        FdoData->BytesTransmitted += length;

        WdfSpinLockAcquire(FdoData->SendLock);
    }
}

VOID
PciDrvEvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
/*++

Routine Description:

    Called by the framework as soon as it receive a write IRP.
    If the device is not ready, fail the request. Otherwise
    get scatter-gather list for this request and send the
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
    NTSTATUS        status;
    PFDO_DATA       FdoData;
    WDFDEVICE       hDevice;
    PMDL            mdl = NULL;

    UNREFERENCED_PARAMETER(Length);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> PciDrvEvtIoWrite Request %p\n", Request);

    hDevice = WdfIoQueueGetDevice(Queue);
    FdoData = FdoGetData(hDevice);

    status = WdfRequestRetrieveInputWdmMdl(Request, &mdl);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                    "WdfRequestRetrieveInputWdmMdl failed %x\n", status);
        WdfRequestCompleteWithInformation(Request, status, 0);

    } else {

        status = NICInitiateDmaTransfer(FdoData, Request);
        if(!NT_SUCCESS(status)) {

            WdfRequestCompleteWithInformation(Request, status, 0);
        }
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- PciDrvEvtIoWrite %X\n", status);

    return;
}

NTSTATUS
NICInitiateDmaTransfer(
    IN PFDO_DATA        FdoData,
    IN WDFREQUEST       Request
    )
{
    WDFDMATRANSACTION   dmaTransaction;
    NTSTATUS            status;
    BOOLEAN             bCreated = FALSE;

    do {
        //
        // Create a new DmaTransaction.
        //
        status = WdfDmaTransactionCreate( FdoData->WdfDmaEnabler,
                                          WDF_NO_OBJECT_ATTRIBUTES,
                                          &dmaTransaction );

        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "WdfDmaTransactionCreate failed %X\n", status);
            break;
        }

        bCreated = TRUE;
        //
        // Initialize the new DmaTransaction.
        //

        status = WdfDmaTransactionInitializeUsingRequest(
                                     dmaTransaction,
                                     Request,
                                     NICEvtProgramDmaFunction,
                                     WdfDmaDirectionWriteToDevice );

        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                       "WdfDmaTransactionInitalizeUsingRequest failed %X\n",
                       status);
            break;
        }

        //
        // Execute this DmaTransaction.
        //
        status = WdfDmaTransactionExecute( dmaTransaction,
                                           dmaTransaction );

        if(!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                            "WdfDmaTransactionExecute failed %X\n", status);
            break;
        }

    } WHILE (FALSE);

    if(!NT_SUCCESS(status)){

        if(bCreated) {
            WdfObjectDelete( dmaTransaction );

        }
    }

    return status;
}


BOOLEAN
NICEvtProgramDmaFunction(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  PVOID                   Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    ScatterGather
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PFDO_DATA           fdoData;
    WDFREQUEST          request;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Direction );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICEvtProgramDmaFunction\n");

    fdoData = FdoGetData(Device);
    request = WdfDmaTransactionGetRequest(Transaction);


    WdfSpinLockAcquire(fdoData->SendLock);

    //
    // If tcb or link is not available, queue the request
    //
    if (!MP_TCB_RESOURCES_AVAIABLE(fdoData) ||
         MP_TEST_FLAG(fdoData, fMP_ADAPTER_LINK_DETECTION))
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "Resource is not available: queue Request %p\n", request);

        //
        // Must abort the transaction before deleting.
        //
        (VOID) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        ASSERT(NT_SUCCESS(status));
        WdfObjectDelete( Transaction );

        //
        // Queue the request for later processing.
        //
        status = WdfRequestForwardToIoQueue(request,
                                           fdoData->PendingWriteQueue);

        if(!NT_SUCCESS(status)) {
            ASSERTMSG(" WdfRequestForwardToIoQueue failed ", FALSE);
            WdfSpinLockRelease(fdoData->SendLock);
            WdfRequestCompleteWithInformation(request, STATUS_UNSUCCESSFUL, 0);
            return FALSE;
        }
        fdoData->nWaitSend++;

    } else {

        status = NICWritePacket(fdoData, Transaction, ScatterGather);

        if(!NT_SUCCESS(status)){

            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "<-- NICEvtProgramDmaFunction returning %!STATUS!\n",
                        status);
            //
            // Must abort the transaction before deleting.
            //
            (VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
            ASSERT(NT_SUCCESS(status));
            WdfObjectDelete( Transaction );

            WdfSpinLockRelease(fdoData->SendLock);
            WdfRequestCompleteWithInformation(request, STATUS_UNSUCCESSFUL, 0);
            return FALSE;
        }
    }


    WdfSpinLockRelease(fdoData->SendLock);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- NICEvtProgramDmaFunction\n");

    return TRUE;
}


NTSTATUS
NICWritePacket(
    IN  PFDO_DATA               FdoData,
    IN  WDFDMATRANSACTION       DmaTransaction,
    IN  PSCATTER_GATHER_LIST    SGList
    )
/*++
Routine Description:

    Do the work to send a packet

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

--*/
{
    PMP_TCB             pMpTcb = NULL;
    NTSTATUS status;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICWritePacket: SGList %p\n", SGList);

    //
    // Initialize the Transfer Control Block.
    //
    pMpTcb = FdoData->CurrSendTail;
    ASSERT(!MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));

    pMpTcb->DmaTransaction = DmaTransaction;

    MP_SET_FLAG(pMpTcb, fMP_TCB_IN_USE);

    //
    // Call the send handler, it only needs to deal with the ScatterGather list
    //
    status = NICSendPacket(FdoData, pMpTcb, SGList);
    if(!NT_SUCCESS(status)){
        MP_CLEAR_FLAG(pMpTcb, fMP_TCB_IN_USE);
        return status;
    }

    FdoData->nBusySend++;
    ASSERT(FdoData->nBusySend <= FdoData->NumTcb);

    FdoData->CurrSendTail = FdoData->CurrSendTail->Next;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "<-- NICWritePacket\n");

    return status;
}

NTSTATUS
NICSendPacket(
    IN  PFDO_DATA              FdoData,
    IN  PMP_TCB                pMpTcb,
    IN  PSCATTER_GATHER_LIST   ScatterGather
    )
/*++
Routine Description:

    NIC specific send handler

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData
    pMpTcb      Pointer to MP_TCB
    ScatterGather   The pointer to the frag list to be filled

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS    status;
    ULONG       index;
    UCHAR       TbdCount = 0;

    PHW_TCB      pHwTcb = pMpTcb->HwTcb;
    PTBD_STRUC   pHwTbd = pMpTcb->HwTbd;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "--> NICSendPacket\n");

    for (index = 0; index < ScatterGather->NumberOfElements; index++)
    {
        if (ScatterGather->Elements[index].Length)
        {
            pHwTbd->TbdBufferAddress =
                ScatterGather->Elements[index].Address.LowPart;

            pHwTbd->TbdCount = ScatterGather->Elements[index].Length;

            pHwTbd++;
            TbdCount++;
        }
    }

    pHwTcb->TxCbHeader.CbStatus = 0;
    pHwTcb->TxCbHeader.CbCommand = CB_S_BIT | CB_TRANSMIT | CB_TX_SF_BIT;

    pHwTcb->TxCbTbdPointer = pMpTcb->HwTbdPhys;
    pHwTcb->TxCbTbdNumber = TbdCount;
    pHwTcb->TxCbCount = 0;
    pHwTcb->TxCbThreshold = (UCHAR) FdoData->AiThreshold;


    status = NICStartSend(FdoData, pMpTcb);

    if(!NT_SUCCESS(status)){
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                   "NICStartSend returned error %x\n", status);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "<-- NICSendPacket\n");

    return status;
}

NTSTATUS
NICStartSend(
    IN  PFDO_DATA  FdoData,
    IN  PMP_TCB    pMpTcb
    )
/*++
Routine Description:

    Issue a send command to the NIC

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData
    pMpTcb      Pointer to MP_TCB

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS     status;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "--> NICStartSend\n");

    //
    // If the transmit unit is idle (very first transmit) then we must
    // setup the general pointer and issue a full CU-start
    //
    if (FdoData->TransmitIdle)
    {

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "CU is idle -- First TCB added to Active List\n");

        //
        // Wait for the SCB to clear before we set the general pointer
        //
        if (!WaitScb(FdoData))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "NICStartSend -- WaitScb returned error\n");
            status = STATUS_DEVICE_DATA_ERROR;
            goto exit;
        }

        //
        // Don't try to start the transmitter if the command unit is not
        // idle ((not idle) == (Cu-Suspended or Cu-Active)).
        //
        if ((FdoData->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_IDLE)
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "FdoData = %p, CU Not IDLE\n", FdoData);
            MP_SET_HARDWARE_ERROR(FdoData);
            KeStallExecutionProcessor(25);
        }

        FdoData->CSRAddress->ScbGeneralPointer = pMpTcb->HwTcbPhys;

        status = D100IssueScbCommand(FdoData, SCB_CUC_START, FALSE);

        FdoData->TransmitIdle = FALSE;
        FdoData->ResumeWait = TRUE;
    }
    else
    {
        //
        // If the command unit has already been started, then append this
        // TCB onto the end of the transmit chain, and issue a CU-Resume.
        //
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "adding TCB to Active chain\n");

        //
        // Clear the suspend bit on the previous packet.
        //
        pMpTcb->PrevHwTcb->TxCbHeader.CbCommand &= ~CB_S_BIT;

        //
        // Issue a CU-Resume command to the device.  We only need to do a
        // WaitScb if the last command was NOT a RESUME.
        //
        status = D100IssueScbCommand(FdoData,
                                     SCB_CUC_RESUME,
                                     FdoData->ResumeWait);
    }

    exit:

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE, "<-- NICStartSend\n");

    return status;
}

_Requires_lock_held_(FdoData->SendLock)
NTSTATUS
NICHandleSendInterrupt(
    IN  PFDO_DATA  FdoData
    )
/*++
Routine Description:

    Interrupt handler for sending processing. Re-claim the send resources,
    complete sends and get more to send from the send wait queue.

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS   status = STATUS_SUCCESS;
    PMP_TCB    pMpTcb;

#if DBG
    ULONG      i;
#endif

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICHandleSendInterrupt\n");

    //
    // Any packets being sent? Any packet waiting in the send queue?
    //
    if (FdoData->nBusySend == 0)
    {
        ASSERT(FdoData->CurrSendHead == FdoData->CurrSendTail);
        return status;
    }

    //
    // Check the first TCB on the send list
    //
    while (FdoData->nBusySend > 0)
    {

#if DBG
        pMpTcb = FdoData->CurrSendHead;
        for (i = 0; i < FdoData->nBusySend; i++)
        {
            pMpTcb = pMpTcb->Next;
        }

        if (pMpTcb != FdoData->CurrSendTail)
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "nBusySend= %d\n", FdoData->nBusySend);
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "CurrSendhead= %p\n", FdoData->CurrSendHead);
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,
                        "CurrSendTail= %p\n", FdoData->CurrSendTail);
            ASSERT(FALSE);
        }
#endif

        pMpTcb = FdoData->CurrSendHead;

        //
        // Is this TCB completed?
        //
        if (pMpTcb->HwTcb->TxCbHeader.CbStatus & CB_STATUS_COMPLETE)
        {
            //
            // Check if this is a multicast hw workaround packet
            //
            if ((pMpTcb->HwTcb->TxCbHeader.CbCommand & CB_CMD_MASK) != CB_MULTICAST)
            {
                BOOLEAN transactionComplete;

                ASSERT(pMpTcb->DmaTransaction);

                //
                // Indicate this DMA operation has completed:
                // This may drive the transfer on the next packet if
                // there is still data to be transfered in the DmaTransaction.
                //
                transactionComplete =
                    WdfDmaTransactionDmaCompleted( pMpTcb->DmaTransaction,
                                                   &status );

                if(transactionComplete == TRUE) {

                    ASSERT(status == STATUS_SUCCESS);
                    MP_FREE_SEND_PACKET(FdoData, pMpTcb, status);

                } else {
                    //
                    // NOTE: For this ethernet driver this should never
                    //       be returned as the packets are <= 1514 bytes.
                    //       It is included to show the complete DmaTransaction
                    //       coding pattern.
                    //
                    ASSERT(!"STATUS_MORE_PROCESSING_REQUIRED");
                }

            }
            else
            {

                // Multicast workaround would be here (???)

            }
        }
        else
        {
            break;
        }
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- NICHandleSendInterrupt\n");
    return status;
}

VOID
NICCheckForQueuedSends(
    IN  PFDO_DATA  FdoData
    )
/*++
Routine Description:

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

--*/
{
    WDFREQUEST         request;
    WDFDMATRANSACTION  dmaTransaction;
    NTSTATUS           status;

    UNREFERENCED_PARAMETER( dmaTransaction );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICCheckForQueuedSends\n");

    //
    // If we queued any transmits because we didn't have any TCBs earlier,
    // dequeue and send those packets now, as long as we have free TCBs.
    //
    while (MP_TCB_RESOURCES_AVAIABLE(FdoData))
    {
        status = WdfIoQueueRetrieveNextRequest(
                     FdoData->PendingWriteQueue,
                     &request
                     );

        if(!NT_SUCCESS(status) ) {
            if(STATUS_NO_MORE_ENTRIES != status) {
                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "WdfIoQueueRetrieveNextRequest failed %X\n", status);
            }
            break;
        }

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "\t processing Request %p \n", request);

        status = NICInitiateDmaTransfer(FdoData, request);
        if(!NT_SUCCESS(status)) {
            WdfRequestCompleteWithInformation(request, status, 0);
        }

        FdoData->nWaitSend--;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- NICCheckForQueuedSends\n");
}

_Requires_lock_held_(FdoData->SendLock)
VOID
NICFreeBusySendPackets(
    IN  PFDO_DATA  FdoData
    )
/*++
Routine Description:

    Free and complete the stopped active sends

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    PMP_TCB  pMpTcb;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICFreeBusySendPackets\n");

    //
    // Any packets being sent? Check the first TCB on the send list
    //
    while (FdoData->nBusySend > 0)
    {
        pMpTcb = FdoData->CurrSendHead;

        //
        // Is this TCB completed?
        //
        if ((pMpTcb->HwTcb->TxCbHeader.CbCommand & CB_CMD_MASK) != CB_MULTICAST)
        {
            MP_FREE_SEND_PACKET(FdoData, pMpTcb, STATUS_SUCCESS);
        }
        else
        {
            break;
        }
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- NICFreeBusySendPackets\n");
}


_IRQL_requires_same_
_IRQL_requires_(DISPATCH_LEVEL)
_Requires_lock_held_(FdoData->SendLock)
VOID
NICFreeQueuedSendPackets(
    IN  PFDO_DATA  FdoData
    )
/*++
Routine Description:

    Free and complete the pended sends on SendQueueHead

    Assumption: This function is called with the Send SPINLOCK held.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    WDFREQUEST   request;
    NTSTATUS     status = MP_GET_STATUS_FROM_FLAGS(FdoData);

    if (STATUS_UNSUCCESSFUL == status) {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "MP_GET_STATUS_FROM_FLAGS failed %x\n", status);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "--> NICFreeQueuedSendPackets\n");

    do {
        status = WdfIoQueueRetrieveNextRequest(
                     FdoData->PendingWriteQueue,
                     &request
                     );

        if(!NT_SUCCESS(status) ) {
            if(STATUS_NO_MORE_ENTRIES != status){
                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                    "WdfIoQueueRetrieveNextRequest failed %x\n", status);
            }
            break;
        }

        FdoData->nWaitSend--;

        WdfSpinLockRelease(FdoData->SendLock);

        WdfRequestCompleteWithInformation(request, status, 0);

        WdfSpinLockAcquire(FdoData->SendLock);

    } WHILE (TRUE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_WRITE,
                "<-- NICFreeQueuedSendPackets\n");

}


