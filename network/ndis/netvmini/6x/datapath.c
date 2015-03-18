/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    DataPath.C

Abstract:

    This module implements the data path of the netvmini miniport.

    In order to excercise the data path of this driver,
    you should install more than one instance of the miniport. If there
    is only one instance installed, the driver throws the send packet on
    the floor and completes the send successfully. If there are more
    instances present, it indicates the incoming send packet to the other
    instances. For example, if there 3 instances: A, B, & C installed.
    Frames sent on instance A would be received on B & C; frames
    sent on B would be received on C, & A; and frames sent on C
    would be received on A & B.

    This sample miniport goes to some extra lengths so that the data path's
    design resembles the design of a real hardware miniport's data path.  For
    example, this sample has both send and receive queues, even though all the
    miniports on the simulated network are on the same computer (and thus could
    take some shortcuts when passing data buffers back and forth).



--*/

#include "netvmin6.h"
#include "datapath.tmh"

static
VOID
TXQueueNetBufferForSend(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER       NetBuffer);

static
VOID
TXTransmitQueuedSends(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fAtDispatch);

static
VOID
TXScheduleTheSendComplete(
    _In_  PMP_ADAPTER  Adapter);

static
VOID
RXQueueFrameOnAdapter(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  PFRAME       Frame);

static
VOID
RXScheduleTheReceiveIndication(
    _In_     PMP_ADAPTER Adapter,
    _In_     PRCB Rcb);

_Must_inspect_result_
static
PTCB
TXGetNextTcbToSend(
    _In_  PMP_ADAPTER      Adapter);


VOID
RXRequeueRcbToReceive(
    _In_ PMP_ADAPTER Adapter,
    _In_ PRCB Rcb);

VOID
RXReceiveIndicate(
    _In_ PMP_ADAPTER Adapter,
    _In_ PMP_ADAPTER_RECEIVE_DPC AdapterDpc,
    BOOLEAN AtDpc);

NDIS_IO_WORKITEM_FUNCTION RXReceiveIndicateWorkItem;
NDIS_IO_WORKITEM_FUNCTION TXSendCompleteWorkItem;

#pragma NDIS_PAGEABLE_FUNCTION(NICStartTheDatapath)
#pragma NDIS_PAGEABLE_FUNCTION(NICStopTheDatapath)

VOID
MPSendNetBufferLists(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST        NetBufferLists,
    _In_  NDIS_PORT_NUMBER        PortNumber,
    _In_  ULONG                   SendFlags)
/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our miniport sends one or more packets.

    The input packet descriptor pointers have been ordered according
    to the order in which the packets should be sent over the network
    by the protocol driver that set up the packet array. The NDIS
    library preserves the protocol-determined ordering when it submits
    each packet array to MiniportSendPackets

    As a deserialized driver, we are responsible for holding incoming send
    packets in our internal queue until they can be transmitted over the
    network and for preserving the protocol-determined ordering of packet
    descriptors incoming to its MiniportSendPackets function.
    A deserialized miniport driver must complete each incoming send packet
    with NdisMSendComplete, and it cannot call NdisMSendResourcesAvailable.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              Head of a list of NBLs to send
    PortNumber                  A miniport adapter port.  Default is 0.
    SendFlags                   Additional flags for the send operation

Return Value:

    None.  Write status directly into each NBL with the NET_BUFFER_LIST_STATUS
    macro.

--*/
{
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    PNET_BUFFER_LIST  Nbl;
    PNET_BUFFER_LIST  NextNbl = NULL;
    BOOLEAN           fAtDispatch = (SendFlags & NDIS_SEND_FLAGS_DISPATCH_LEVEL) ? TRUE:FALSE;
    NDIS_STATUS       Status;
    ULONG             NumNbls=0;

    DEBUGP(MP_TRACE, "[%p] ---> MPSendNetBufferLists\n", Adapter);

    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(SendFlags);
    ASSERT(PortNumber == 0); // Only the default port is supported


    //
    // Each NET_BUFFER_LIST has a list of NET_BUFFERs.
    // Loop over all the NET_BUFFER_LISTs, sending each NET_BUFFER.
    //
    for (
        Nbl = NetBufferLists;
        Nbl!= NULL;
        Nbl = NextNbl, ++NumNbls)
    {
        PNET_BUFFER NetBuffer;

        NextNbl = NET_BUFFER_LIST_NEXT_NBL(Nbl);

        //
        // Unlink the NBL and prepare our bookkeeping.
        //
        // We use a reference count to make sure that we don't send complete
        // the NBL until we're done reading each NB on the NBL.
        //
        NET_BUFFER_LIST_NEXT_NBL(Nbl) = NULL;
        SEND_REF_FROM_NBL(Nbl) = 0;

        Status = TXNblReference(Adapter, Nbl);

        if(Status == NDIS_STATUS_SUCCESS)
        {
            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_SUCCESS;

            //
            // Queue each NB for transmission.
            //
            for (
                NetBuffer = NET_BUFFER_LIST_FIRST_NB(Nbl);
                NetBuffer != NULL;
                NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                NBL_FROM_SEND_NB(NetBuffer) = Nbl;
                TXQueueNetBufferForSend(Adapter, NetBuffer);
            }

            TXNblRelease(Adapter, Nbl, fAtDispatch);
        }
        else
        {
            //
            // We can't send this NBL now.  Indicate failure.
            //
            if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESET_IN_PROGRESS;
            }
            else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS|fMP_ADAPTER_PAUSED))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_PAUSED;
            }
            else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LOW_POWER))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_LOW_POWER_STATE;
            }
            else
            {
                NET_BUFFER_LIST_STATUS(Nbl) = Status;
            }

            NdisMSendNetBufferListsComplete(
                    Adapter->AdapterHandle,
                    Nbl,
                    fAtDispatch ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL:0);

            continue;
        }
    }

    DEBUGP(MP_TRACE, "[%p] %i NBLs processed.\n", Adapter, NumNbls);

    //
    // Now actually go send each of the queued NBs.
    //
    TXTransmitQueuedSends(Adapter, fAtDispatch);

    DEBUGP(MP_TRACE, "[%p] <--- MPSendNetBufferLists\n", Adapter);
}

VOID
TXQueueNetBufferForSend(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER       NetBuffer)
/*++

Routine Description:

    This routine inserts the NET_BUFFER into the SendWaitList, then calls
    TXTransmitQueuedSends to start sending data from the list.

    We use this indirect queue to send data because the miniport should try to
    send frames in the order in which the protocol gave them.  If we just sent
    the NET_BUFFER immediately, then it would be out-of-order with any data on
    the SendWaitList.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Adapter that is transmitting this NB
    NetBuffer                   NB to be transfered

Return Value:

    None.

--*/
{
    NDIS_STATUS       Status;
    UCHAR             DestAddress[NIC_MACADDR_SIZE];

    DEBUGP(MP_TRACE, "[%p] ---> TXQueueNetBufferForSend, NB= 0x%p\n", Adapter, NetBuffer);

    do
    {
        //
        // First, do a sanity check on the frame data.
        //
        Status = HWGetDestinationAddress(NetBuffer, DestAddress);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NET_BUFFER_LIST_STATUS(NBL_FROM_SEND_NB(NetBuffer)) = NDIS_STATUS_INVALID_DATA;
            break;
        }

        //
        // Stash away the frame type.  We'll use that later, when updating
        // our send statistics (since we don't have NIC hardware to compute the
        // send statistics for us).
        //
        FRAME_TYPE_FROM_SEND_NB(NetBuffer) = NICGetFrameTypeFromDestination(DestAddress);

        //
        // Pin the original NBL with a reference, so it isn't completed until
        // we're done with its NB.
        //
        Status = TXNblReference(Adapter, NBL_FROM_SEND_NB(NetBuffer));
        if(Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Insert the NB into the queue.  The caller will flush the queue when
            // it's done adding items to the queue.
            //
            NdisInterlockedInsertTailList(
                    &Adapter->SendWaitList,
                    SEND_WAIT_LIST_FROM_NB(NetBuffer),
                    &Adapter->SendWaitListLock);
        }

    } while (FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- TXQueueNetBufferForSend\n", Adapter);
}


VOID
#pragma prefast(suppress: 28167, "PREfast does not recognize IRQL is conditionally raised and lowered")
TXTransmitQueuedSends(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fAtDispatch)
/*++

Routine Description:

    This routine sends as many frames from the SendWaitList as it can.

    If there are not enough resources to send immediately, this function stops
    and leaves the remaining frames on the SendWaitList, to be sent once there
    are enough resources.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Our adapter
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{
    BOOLEAN fScheduleTheSendCompleteDpc = FALSE;
    ULONG NumFramesSent = 0;
    KIRQL OldIrql = PASSIVE_LEVEL;

    DEBUGP(MP_TRACE,
           "[%p] ---> TXTransmitQueuedSends\n",
           Adapter);

    //
    // This guard ensures that only one CPU is running this function at a time.
    // We check this so that items from the SendWaitList get sent to the
    // receiving adapters in the same order that they were queued.
    //
    // You could remove this guard and everything will still work ok, but some
    // frames might be delivered out-of-order.
    //
    // Generally, this mechanism wouldn't be applicable to real hardware, since
    // the hardware would have its own mechanism to ensure sends are transmitted
    // in the correct order.
    //
    if (!fAtDispatch)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }

    if (KeTryToAcquireSpinLockAtDpcLevel(&Adapter->SendPathSpinLock))
    {
        for (NumFramesSent = 0; NumFramesSent < NIC_MAX_SENDS_PER_DPC; NumFramesSent++)
        {
            PLIST_ENTRY pTcbEntry = NULL;
            PTCB Tcb = NULL;
            PLIST_ENTRY pQueuedSend = NULL;
            PNET_BUFFER NetBuffer;

            //
            // Get the next available TCB.
            //
            pTcbEntry = NdisInterlockedRemoveHeadList(
                    &Adapter->FreeTcbList,
                    &Adapter->FreeTcbListLock);
            if (!pTcbEntry)
            {
                //
                // The adapter can't handle any more simultaneous transmit
                // operations.  Keep any remaining sends in the SendWaitList and
                // we'll come back later when there are TCBs available.
                //
                break;
            }

            Tcb = CONTAINING_RECORD(pTcbEntry, TCB, TcbLink);

            //
            // Get the next NB that needs sending.
            //
            pQueuedSend = NdisInterlockedRemoveHeadList(
                    &Adapter->SendWaitList,
                    &Adapter->SendWaitListLock);
            if (!pQueuedSend)
            {
                //
                // There's nothing left that needs sending.  We're all done.
                //
                NdisInterlockedInsertTailList(
                        &Adapter->FreeTcbList,
                        &Tcb->TcbLink,
                        &Adapter->FreeTcbListLock);
                break;
            }

            NetBuffer = NB_FROM_SEND_WAIT_LIST(pQueuedSend);


            //
            // We already packed the frame type into the net buffer before accepting
            // it for send.  Now that we have a TCB to keep track of the data, let's
            // pull it out and keep it in a proper variable.
            //
            Tcb->FrameType = FRAME_TYPE_FROM_SEND_NB(NetBuffer);

            HWProgramDmaForSend(Adapter, Tcb, NetBuffer, fAtDispatch);

            NdisInterlockedInsertTailList(
                &Adapter->BusyTcbList,
                    &Tcb->TcbLink,
                    &Adapter->BusyTcbListLock);

            fScheduleTheSendCompleteDpc = TRUE;
        }

        KeReleaseSpinLock(&Adapter->SendPathSpinLock, DISPATCH_LEVEL);
    }

    if (!fAtDispatch)
    {
        KeLowerIrql(OldIrql);
    }

    DEBUGP(MP_TRACE, "[%p] %i Frames transmitted.\n", Adapter, NumFramesSent);

    if (fScheduleTheSendCompleteDpc)
    {
        TXScheduleTheSendComplete(Adapter);
    }

    DEBUGP(MP_TRACE, "[%p] <-- TXTransmitQueuedSends\n", Adapter);
}


VOID
TXScheduleTheSendComplete(
    _In_  PMP_ADAPTER  Adapter)
/*++

Routine Description:

    This function schedules the transmit DPC on the sending miniport.

Arguments:

    FunctionContext             Pointer to the adapter that is sending frames

Return Value:

    None.

--*/
{
    LARGE_INTEGER liDelay;

    if (!Adapter->SendCompleteWorkItemQueued)
    {
        liDelay.QuadPart = -(NIC_SIMULATED_LATENCY);
        NdisSetTimerObject(Adapter->SendCompleteTimer, liDelay, 0, NULL);
    }

    DEBUGP(MP_TRACE, "[%p] Scheduled Send Complete DPC [Delay: %i].\n", Adapter, NIC_SIMULATED_LATENCY);

}


_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN
WorkItemQueuedForWatchdogAvoidance(
        _In_ NDIS_HANDLE WorkItem,
        _In_ volatile LONG* WorkItemQueued,
        _In_ NDIS_IO_WORKITEM_ROUTINE WIRoutine,
        _In_ PVOID WIContext
        )
/*++

Routine Description:

    This function should be called from the receive or send-complete DPCs. It queues a work item to do the receives or send-completes
    if the DPC watchdog timer is within 25% of the limit. This allows the processor to reach PASSIVE_LEVEL and reset the watchdog.

    Runs at IRQL = DISPATCH_LEVEL.

Arguments:

    WorkItem        The work item to queue
    WorkItemQueued  Variable that stores whether a work item is currently queued
    WIRoutine       The work routine
    WIContext       The context passed to the work routine

Return Value:

    TRUE - Work item queued due to watchdog timer, caller should exit DPC
    FALSE - Ok to continue in DPC

--*/
{
    KDPC_WATCHDOG_INFORMATION WatchdogInfo;
    NTSTATUS Status;

    if(*WorkItemQueued)
    {
        //
        // We've already queued up the work item, no need to check watchdog information
        //
        return TRUE;
    }

    Status = KeQueryDpcWatchdogInformation(&WatchdogInfo);
    if (NT_SUCCESS(Status)
            //
            // Verify the watchdog is enabled
            //
            && WatchdogInfo.DpcWatchdogLimit != 0
            //
            // Once we go below 25% of the watchdog limit we fall back on the work item to allow the watchdog to reset
            //
            && WatchdogInfo.DpcWatchdogCount < WatchdogInfo.DpcWatchdogLimit / 4)
    {
        //
        // Make sure we don't queue the work item if it's already been queued for this DPC
        //
        LONG AlreadyQueued = InterlockedCompareExchange(
                                  WorkItemQueued,
                                  TRUE,
                                  FALSE);
        if(!AlreadyQueued)
        {
            //
            // We've crossed our threshold for consecutive DPCs, schedule work item to complete this receive
            //
            DEBUGP(MP_TRACE, "Processor has spent too much time in DPC. Queueing work item to handle next receives/send-completes.\n");
            NdisQueueIoWorkItem(WorkItem, WIRoutine, WIContext);
        }
        return TRUE;
    }

    //
    // We're still within acceptable time limits
    //
    return FALSE;
}


_IRQL_requires_(DISPATCH_LEVEL)
VOID
TXSendComplete(
    _In_ PMP_ADAPTER Adapter)
/*++

Routine Description:

    This routine completes pending sends for the given adapter.

    Each busy TCB is popped from the BusyTcbList and its corresponding NB is
    released.  If there was an error sending the frame, the NB's NBL's status
    is updated.

--*/
{
    BOOLEAN fRescheduleThisDpcAgain = TRUE;
    ULONG NumFramesSent = 0;

    DEBUGP(MP_TRACE, "[%p] ---> TXSendComplete.\n", Adapter);

    for (NumFramesSent = 0; NumFramesSent < NIC_MAX_SENDS_PER_DPC; NumFramesSent++)
    {
        ULONG BytesSent;

        PTCB Tcb = TXGetNextTcbToSend(Adapter);
        if (!Tcb)
        {
            //
            // There are no more TCBs remaining to send.  We're all done.
            //
            fRescheduleThisDpcAgain = FALSE;
            break;
        }


        //
        // Finish the transmit operation.  For our hardware, that means the
        // frame is pushed onto the RecvWaitLists of each other adapter.
        //

        BytesSent = HWGetBytesSent(Adapter, Tcb);

        if (BytesSent == 0)
        {
            //
            // Failed to send the frame.
            //

            Adapter->TransmitFailuresOther++;
            NET_BUFFER_LIST_STATUS(NBL_FROM_SEND_NB(Tcb->NetBuffer)) = NDIS_STATUS_RESOURCES;
        }
        else
        {
            //
            // We've finished sending this NB successfully; update the stats.
            //
            switch (Tcb->FrameType)
            {
                case NDIS_PACKET_TYPE_BROADCAST:
                    Adapter->FramesTxBroadcast++;
                    Adapter->BytesTxBroadcast += BytesSent;
                    break;

                case NDIS_PACKET_TYPE_MULTICAST:
                    Adapter->FramesTxMulticast++;
                    Adapter->BytesTxMulticast += BytesSent;
                    break;

                case NDIS_PACKET_TYPE_DIRECTED:
                default:
                    Adapter->FramesTxDirected++;
                    Adapter->BytesTxDirected += BytesSent;
            }
        }


        //
        // Now that we've finished using the TCB and its associated NET_BUFFER,
        // we can release the NET_BUFFER back to the protocol and the TCB back
        // to the free list.
        //
        ReturnTCB(Adapter, Tcb);
    }

    TXTransmitQueuedSends(Adapter, TRUE);

    if (fRescheduleThisDpcAgain)
    {
        TXScheduleTheSendComplete(Adapter);
    }

    DEBUGP(MP_TRACE, "[%p] <--- TXSendComplete.\n", Adapter);
}

_Use_decl_annotations_
VOID
TXSendCompleteWorkItem(
    PVOID       FunctionContext,
    NDIS_HANDLE WorkItem)
/*++

Routine Description:

    This work item handler is used to do send completions in the case when we are trying
    to avoid a DPC watchdog timeout

Arguments:

    FunctionContext - The Adapter object for which send-completions are to be done

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(FunctionContext);
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(WorkItem);

	ASSERT(Adapter != NULL);
	_Analysis_assume_(Adapter != NULL);
    DEBUGP(MP_TRACE, "[%p] ---> TXSendCompleteWorkItem.\n", Adapter);

    Adapter->SendCompleteWorkItemRunning = TRUE;
    KeMemoryBarrier();

    Adapter->SendCompleteWorkItemQueued = FALSE;
    KeMemoryBarrier();

    NDIS_RAISE_IRQL_TO_DISPATCH(&OldIrql);
    TXSendComplete(Adapter);
    NDIS_LOWER_IRQL(OldIrql,DISPATCH_LEVEL);

    KeMemoryBarrier();
    Adapter->SendCompleteWorkItemRunning = FALSE;

    DEBUGP(MP_TRACE, "[%p] <--- TXSendCompleteWorkItem.\n", Adapter);
}

_Use_decl_annotations_
VOID
TXSendCompleteDpc(
    PVOID             UnusedParameter1,
    PVOID             FunctionContext,
    PVOID             UnusedParameter2,
    PVOID             UnusedParameter3)
/*++

Routine Description:

    This routine simulates the DPC handler of a send complete hardware
    interrupt.

Arguments:

    FunctionContext - The Adapter object for which send-completions are to be done

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(FunctionContext);

    UNREFERENCED_PARAMETER(UnusedParameter1);
    UNREFERENCED_PARAMETER(UnusedParameter2);
    UNREFERENCED_PARAMETER(UnusedParameter3);

    DEBUGP(MP_TRACE, "[%p] ---> TXSendCompleteDpc\n", Adapter);

    if (!WorkItemQueuedForWatchdogAvoidance(Adapter->SendCompleteWorkItem,
                                            &Adapter->SendCompleteWorkItemQueued,
                                            TXSendCompleteWorkItem,
                                            Adapter))
    {
        TXSendComplete(Adapter);
    }

    DEBUGP(MP_TRACE, "[%p] <--- TXSendCompleteDpc\n", Adapter);
}

NDIS_STATUS
TXNblReference(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList)
/*++

Routine Description:

    Adds a reference on a NBL that is being transmitted.
    The NBL won't be returned to the protocol until the last reference is
    released.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to reference

Return Value:

    NDIS_STATUS_SUCCESS if reference was acquired succesfully.
    NDIS_STATUS_ADAPTER_NOT_READY if the adapter state is such that we should not acquire new references to resources

--*/
{
    NdisInterlockedIncrement(&Adapter->nBusySend);

    //
    // Make sure the increment happens before ready state check
    //
    KeMemoryBarrier();

    //
    // If the adapter is not ready, undo the reference and fail the call
    //
    if(!MP_IS_READY(Adapter))
    {
        InterlockedDecrement(&Adapter->nBusySend);
        DEBUGP(MP_LOUD, "[%p] Could not acquire transmit reference, the adapter is not ready.\n", Adapter);
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    NdisInterlockedIncrement(&SEND_REF_FROM_NBL(NetBufferList));
    return NDIS_STATUS_SUCCESS;

}


VOID
TXNblRelease(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList,
    _In_  BOOLEAN           fAtDispatch)
/*++

Routine Description:

    Releases a reference on a NBL that is being transmitted.
    If the last reference is released, the NBL is returned to the protocol.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to release
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{

    if (0 == NdisInterlockedDecrement(&SEND_REF_FROM_NBL(NetBufferList)))
    {
        DEBUGP(MP_TRACE, "[%p] Send NBL %p complete.\n", Adapter, NetBufferList);

        NET_BUFFER_LIST_NEXT_NBL(NetBufferList) = NULL;

        NdisMSendNetBufferListsComplete(
                Adapter->AdapterHandle,
                NetBufferList,
                fAtDispatch ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL:0);
    }
    else
    {
        DEBUGP(MP_TRACE, "[%p] Send NBL %p not complete. RefCount: %i.\n", Adapter, NetBufferList, SEND_REF_FROM_NBL(NetBufferList));
    }

    NdisInterlockedDecrement(&Adapter->nBusySend);
}


_Must_inspect_result_
PTCB
TXGetNextTcbToSend(
    _In_  PMP_ADAPTER      Adapter)
/*++

Routine Description:

    Returns the next TCB queued on the send list, or NULL if the list was empty.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    NULL if there was no TCB queued.
    Else, a pointer to the TCB that was popped off the top of the BusyTcbList.

--*/
{
    PTCB Tcb;
    PLIST_ENTRY pTcbEntry = NdisInterlockedRemoveHeadList(
            &Adapter->BusyTcbList,
            &Adapter->BusyTcbListLock);

    if (! pTcbEntry)
    {
        // End of list -- no more items to receive.
        return NULL;
    }

    Tcb = CONTAINING_RECORD(pTcbEntry, TCB, TcbLink);

    ASSERT(Tcb);
    ASSERT(Tcb->NetBuffer);

    return Tcb;
}


VOID
TXFlushSendQueue(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_STATUS  CompleteStatus)
/*++

Routine Description:

    This routine is called by the Halt or Reset handler to fail all
    the queued up Send NBLs because the device is either gone, being
    stopped for resource rebalance, or reset.

Arguments:

    Adapter                     Pointer to our adapter
    CompleteStatus              The status code with which to complete each NBL

Return Value:

    None.

--*/
{
    PTCB Tcb;

    DEBUGP(MP_TRACE, "[%p] ---> TXFlushSendQueue Status = 0x%08x\n", Adapter, CompleteStatus);


    //
    // First, free anything queued in the driver.
    //

    while (TRUE)
    {
        PLIST_ENTRY pEntry;
        PNET_BUFFER NetBuffer;
        PNET_BUFFER_LIST NetBufferList;

        pEntry = NdisInterlockedRemoveHeadList(
                &Adapter->SendWaitList,
                &Adapter->SendWaitListLock);

        if (!pEntry)
        {
            // End of list -- nothing left to free.
            break;
        }

        NetBuffer = NB_FROM_SEND_WAIT_LIST(pEntry);
        NetBufferList = NBL_FROM_SEND_NB(NetBuffer);

        DEBUGP(MP_TRACE, "[%p] Dropping Send NB: 0x%p.\n", Adapter, NetBuffer);

        NET_BUFFER_LIST_STATUS(NetBufferList) = CompleteStatus;
        TXNblRelease(Adapter, NetBufferList, FALSE);
    }


    //
    // Next, cancel anything queued in the hardware.
    //

    while (NULL != (Tcb = TXGetNextTcbToSend(Adapter)))
    {
        NET_BUFFER_LIST_STATUS(NBL_FROM_SEND_NB(Tcb->NetBuffer)) = CompleteStatus;
        ReturnTCB(Adapter, Tcb);
    }


    DEBUGP(MP_TRACE, "[%p] <--- TXFlushSendQueue\n", Adapter);
}


VOID
RXDeliverFrameToEveryAdapter(
    _In_  PMP_ADAPTER  SendAdapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  PFRAME       Frame,
    _In_  BOOLEAN      fAtDispatch)
/*++

Routine Description:

    This routine sends a TCB to each netvmini 6.x adapter (besides the sending
    adapter itself)

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    SendAdapter                 Our adapter that is doing the sending
    Nbl1QInfo                   8021Q Tag information for the FRAME to be sent
    Frame                       The FRAME to be sent
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{
    MP_LOCK_STATE  LockState;
    PLIST_ENTRY AdapterLink;


    DEBUGP(MP_TRACE, "[%p] ---> RXDeliverFrameToEveryAdapter. Frame=0x%p\n", SendAdapter, Frame);

    LOCK_ADAPTER_LIST_FOR_READ(&LockState, fAtDispatch ? NDIS_RWL_AT_DISPATCH_LEVEL:0);
    UNREFERENCED_PARAMETER(fAtDispatch);

    //
    // Go through the adapter list and queue packet for
    // indication on them if there are any. Otherwise
    // just drop the packet on the floor and tell NDIS that
    // you have completed send.
    //

    for (
        AdapterLink = GlobalData.AdapterList.Flink;
        AdapterLink != &GlobalData.AdapterList;
        AdapterLink = AdapterLink->Flink
        )
    {
        PMP_ADAPTER DestAdapter = CONTAINING_RECORD(AdapterLink, MP_ADAPTER, List);

        if (DestAdapter == SendAdapter)
        {
            // Don't loopback packets to the sending adapter.
            continue;
        }

        RXQueueFrameOnAdapter(DestAdapter, Nbl1QInfo, Frame);
    }

    UNLOCK_ADAPTER_LIST(&LockState);

    DEBUGP(MP_TRACE, "[%p] <-- RXDeliverFrameToEveryAdapter\n", SendAdapter);

}

VOID
RXQueueFrameOnAdapter(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  PFRAME       Frame)
/*++

Routine Description:

    This routine queues the send packet in to the destination
    adapters RecvWaitList and fires a timer DPC so that it
    can be indicated as soon as possible.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Pointer to the destination adapter
    Nbl1QInfo                   8021Q Tag information for the FRAME to be sent
    Frame                       Pointer to FRAME that contains the data payload


Return Value:

    None.

--*/
{
    DEBUGP(MP_TRACE, "[%p] ---> RXQueueFrameOnAdapter\n", Adapter);

    do
    {
        PRCB          Rcb;
        UCHAR         DestAddress[NIC_MACADDR_SIZE];
        ULONG         FrameType;


        if (!MP_IS_READY(Adapter))
        {
            //
            // The NIC is not receiving any data.
            //
            break;
        }

        if (Frame->ulSize < HW_MIN_FRAME_SIZE)
        {
            //
            // This frame is malformed.  Drop it.
            //
            Adapter->RxRuntErrors++;
            break;
        }

        GET_DESTINATION_OF_FRAME(DestAddress, Frame->Data);
        FrameType = NICGetFrameTypeFromDestination(DestAddress);

        if(VMQ_ENABLED(Adapter) && FrameType == NDIS_PACKET_TYPE_DIRECTED)
        {
            //
            // Defer decision whether to drop until we check for VMQ matches
            //
        }
        else if (!HWIsFrameAcceptedByPacketFilter(Adapter, DestAddress, FrameType))
        {
            //
            // Our NIC "hardware" has a packet filter that eliminates frames
            // that weren't sent to us.  This frame didn't match the filter,
            // so pretend we never saw this frame.
            //
            break;
        }

        //
        // Allocate memory for RCB.
        //
        Rcb = GetRCB(Adapter, Nbl1QInfo, Frame);
        if (!Rcb)
        {
            DEBUGP(MP_TRACE, "[%p] GetRCB did not return an RCB.\n", Adapter);
            break;
        }


        switch (FrameType)
        {
            case NDIS_PACKET_TYPE_BROADCAST:
                Adapter->FramesRxBroadcast++;
                Adapter->BytesRxBroadcast += Frame->ulSize;
                break;

            case NDIS_PACKET_TYPE_MULTICAST:
                Adapter->FramesRxMulticast++;
                Adapter->BytesRxMulticast += Frame->ulSize;
                break;

            case NDIS_PACKET_TYPE_DIRECTED:
            default:
                Adapter->FramesRxDirected++;
                Adapter->BytesRxDirected += Frame->ulSize;
        }


        //
        // If VMQ is enabled, queue Rcb on the owner VMQ, otherwise
        // use global receive wait list
        //
        if(VMQ_ENABLED(Adapter))
        {
            //
            // Queue on owner VMQ receive block
            //
            AddPendingRcbToRxQueue(Adapter, Rcb);
        }
        else
        {
            //
            // Queue on global receive block
            //
            NdisInterlockedInsertTailList(&Adapter->ReceiveBlock[0].ReceiveList, &Rcb->RcbLink, &Adapter->ReceiveBlock[0].ReceiveListLock);
        }

        RXScheduleTheReceiveIndication(Adapter, Rcb);


    } while (FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- RXQueueFrameOnAdapter\n", Adapter);
}

VOID
RXScheduleTheReceiveIndication(
    _In_     PMP_ADAPTER  Adapter,
    _In_     PRCB Rcb)
/*++

Routine Description:

    This function schedules the receive DPC on the receiving miniport.

Arguments:

    FunctionContext             Pointer to the adapter that is receiving frames

Return Value:

    None.

--*/
{

    //
    // Use default DPC unless VMQ is enabled, in which case you use the Queue's DPC
    //
    PMP_ADAPTER_RECEIVE_DPC AdapterDpc = Adapter->DefaultRecvDpc;

    if(VMQ_ENABLED(Adapter))
    {
        //
        // Add Rcb to owner Queue's pending List
        //
        AdapterDpc = GetRxQueueDpc(Adapter, NET_BUFFER_LIST_RECEIVE_QUEUE_ID(Rcb->Nbl));
    }
    else
    {
        UNREFERENCED_PARAMETER(Rcb);
    }

    //
    // Schedule DPC
    //
    if(AdapterDpc->WorkItemQueued)
    {
        //
        // We've queued up receive work item to avoid DPC watchdog timeout. Let's wait for it to start rather
        // than queue up the DPC.
        //
        DEBUGP(MP_TRACE, "[%p] Receive DPC not scheduled, receive work item is pending. Processor: %i\n", Adapter, AdapterDpc->ProcessorNumber);
    }
    else
    {
        KeInsertQueueDpc(&AdapterDpc->Dpc, AdapterDpc, NULL);
        DEBUGP(MP_TRACE, "[%p] Scheduled Receive DPC. Processor: %i\n", Adapter, AdapterDpc->ProcessorNumber);
    }

}

VOID
RXReceiveIndicateDpc(
    _In_ struct _KDPC  *Dpc,
    _In_opt_ PVOID  DeferredContext,
    _In_opt_ PVOID  SystemArgument1,
    _In_opt_ PVOID  SystemArgument2)
/*++

Routine Description:

    DPC function for Receive Indication. Please note that receive
    timer DPC is not required when you are talking to a real device. In real
    miniports, this DPC is usually provided by NDIS as MPHandleInterrupt
    callback whenever the device interrupts for receive indication.

Arguments:

    DeferredContext             Pointer to our adapter
    SystemArgument1             PMP_ADAPTER_RECEIVE_DPC structure for this DPC

Return Value:

    None.

--*/
{

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ASSERT(DeferredContext != NULL);
    ASSERT(SystemArgument1 != NULL);
    _Analysis_assume_(DeferredContext != NULL);
    _Analysis_assume_(SystemArgument1 != NULL);

    RXReceiveIndicate((PMP_ADAPTER)DeferredContext, (PMP_ADAPTER_RECEIVE_DPC)SystemArgument1, TRUE);
}

_Use_decl_annotations_
VOID
RXReceiveIndicateWorkItem(
        PVOID  WorkItemContext,
        NDIS_HANDLE NdisIoWorkItemHandle)
/*++

Routine Description:

    Work Item function for Receive Indication. The work item is invoked if the corresponding receive
    DPC has run enough times on the processor without a transition to PASSIVE to risk hitting the DPC
    watchdog timer.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    WorkItemContext                     PMP_ADAPTER_RECEIVE_DPC structure for the corresponding receive DPC
    NdisIoWorkItemHandle                Workitem handle, unused

Return Value:

    None.

--*/
{
    PMP_ADAPTER_RECEIVE_DPC AdapterDpc = (PMP_ADAPTER_RECEIVE_DPC)WorkItemContext;
    UNREFERENCED_PARAMETER(NdisIoWorkItemHandle);
	ASSERT(AdapterDpc != NULL);
	_Analysis_assume_(AdapterDpc != NULL);
    RXReceiveIndicate(AdapterDpc->Adapter, AdapterDpc, FALSE);
}

VOID
RXReceiveIndicate(
    _In_ PMP_ADAPTER Adapter,
    _In_ PMP_ADAPTER_RECEIVE_DPC AdapterDpc,
    BOOLEAN AtDpc)
/*++

Routine Description:

    This function performs the receive indications for the specified RECEIVE_DPC structure.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter             Pointer to our adapter
    AdapterDpc          PMP_ADAPTER_RECEIVE_DPC structure for this receive
    AtDpc               TRUE if the function was called from the context of the DPC, FALSE if called from work item (to avoid watchdog)

Return Value:

    None.

--*/

{

    ULONG NumNblsReceived = 0;
    PNET_BUFFER_LIST FirstNbl = NULL, LastNbl = NULL;
    USHORT CurrentQueue;

    DEBUGP(MP_TRACE, "[%p] ---> RXReceiveIndicate. Processor: %i, AtDpc: %i\n", Adapter, AdapterDpc->ProcessorNumber, AtDpc);

    //
    // Exit DPC if we've queued a work item to avoid DPC watchdog timer expiration
    //
    if(AtDpc && WorkItemQueuedForWatchdogAvoidance(AdapterDpc->WorkItem,
                                                   &AdapterDpc->WorkItemQueued,
                                                   RXReceiveIndicateWorkItem,
                                                   AdapterDpc))
    {
        DEBUGP(MP_TRACE, "[%p] <--- RXReceiveIndicate. Processor: %i\n", Adapter, AdapterDpc->ProcessorNumber);
        return;
    }

    for(CurrentQueue = 0; CurrentQueue <NIC_SUPPORTED_NUM_QUEUES; ++CurrentQueue)
    {
        //
        // Consume RCBs for queue if we're the assigned consumer
        //
        if(AdapterDpc->RecvBlock[CurrentQueue])
        {
            PMP_ADAPTER_RECEIVE_BLOCK ReceiveBlock = &Adapter->ReceiveBlock[CurrentQueue];
            FirstNbl = LastNbl = NULL;

            //
            // Collect pending NBLs, indicate up to MaxNblCountPerIndicate per receive block
            //
            for(NumNblsReceived=0; NumNblsReceived < AdapterDpc->MaxNblCountPerIndicate; ++NumNblsReceived)
            {
                PLIST_ENTRY Entry;
                PRCB Rcb = NULL;

                Entry = NdisInterlockedRemoveHeadList(&ReceiveBlock->ReceiveList, &ReceiveBlock->ReceiveListLock);
                if(Entry)
                {
                    Rcb = CONTAINING_RECORD(Entry, RCB, RcbLink);
                }

                if(!Rcb)
                {
                    break;
                }

                ASSERT(Rcb->Data);

                //
                // The recv NBL's data was filled out by the hardware.  Now just update
                // its bookkeeping.
                //
                NET_BUFFER_LIST_STATUS(Rcb->Nbl) = NDIS_STATUS_SUCCESS;
                Rcb->Nbl->SourceHandle = Adapter->AdapterHandle;

                //
                // Add this NBL to the chain of NBLs to indicate up.
                //
                if (!FirstNbl)
                {
                    LastNbl = FirstNbl = Rcb->Nbl;
                }
                else
                {
                    NET_BUFFER_LIST_NEXT_NBL(LastNbl) = Rcb->Nbl;
                    LastNbl = Rcb->Nbl;
                }
            }

            //
            // Indicate NBLs
            //
            if (FirstNbl)
            {
                DEBUGP(MP_TRACE, "[%p] Receive Block %i: %i frames indicated.\n", Adapter, CurrentQueue, NumNblsReceived);

                NET_BUFFER_LIST_NEXT_NBL(LastNbl) = NULL;

                //
                // Indicate up the NBLs.
                //
                // The NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL allows a perf optimization:
                // NDIS doesn't have to check and raise the current IRQL, since we
                // promise that the current IRQL is exactly DISPATCH_LEVEL already.
                //
                NdisMIndicateReceiveNetBufferLists(
                        Adapter->AdapterHandle,
                        FirstNbl,
                        0,  // default port
                        NumNblsReceived,
                        (AtDpc?NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL:0)
                        | NDIS_RECEIVE_FLAGS_PERFECT_FILTERED
#if (NDIS_SUPPORT_NDIS620)
                        | NDIS_RECEIVE_FLAGS_SINGLE_QUEUE
                        | (CurrentQueue?NDIS_RECEIVE_FLAGS_SHARED_MEMORY_INFO_VALID:0) //non-default queues use shared memory
#endif
                        );
            }

            if(!AtDpc)
            {
                //
                // Clear work item flag to allow DPCs to be queued
                //
                InterlockedExchange(&AdapterDpc->WorkItemQueued, FALSE);
            }

            if (!IsListEmpty(&ReceiveBlock->ReceiveList))
            {
                //
                // More left to indicate for this receive block, queue this DPC again
                //
                DEBUGP(MP_TRACE, "[%p] Receive Block %i: Requeued DPC.\n", Adapter, CurrentQueue);
                KeInsertQueueDpc(&AdapterDpc->Dpc, AdapterDpc, NULL);
            }

        }

    }

    DEBUGP(MP_TRACE, "[%p] <--- RXReceiveIndicate. Processor: %i\n", Adapter, AdapterDpc->ProcessorNumber);
}


VOID
MPReturnNetBufferLists(
    _In_  NDIS_HANDLE       MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST  NetBufferLists,
    _In_  ULONG             ReturnFlags)
/*++

Routine Description:

    NDIS Miniport entry point called whenever protocols are done with one or
    NBLs that we indicated up with NdisMIndicateReceiveNetBufferLists.

    Note that the list of NBLs may be chained together from multiple separate
    lists that were indicated up individually.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              NBLs being returned
    ReturnFlags                 May contain the NDIS_RETURN_FLAGS_DISPATCH_LEVEL
                                flag, which if is set, indicates we can get a
                                small perf win by not checking or raising the
                                IRQL

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(ReturnFlags);

    DEBUGP(MP_TRACE, "[%p] ---> MPReturnNetBufferLists\n", Adapter);

    while (NetBufferLists)
    {
        PRCB Rcb = RCB_FROM_NBL(NetBufferLists);
        ReturnRCB(Adapter, Rcb);
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);
    }

    DEBUGP(MP_TRACE, "[%p] <--- MPReturnNetBufferLists\n", Adapter);
}

VOID
RXFlushReceiveQueue(
    _In_ PMP_ADAPTER Adapter,
    _In_ PMP_ADAPTER_RECEIVE_DPC AdapterDpc)
/*++

Routine Description:

    This routine is called by the Halt handler to fail all
    the queued up RecvNbls if it succeeds in cancelling
    the RecvIndicate timer DPC.

Arguments:

    Adapter                     Our adapter
    AdapterDpc                  DPC to be flushed

Return Value:

    None.

--*/
{
    DEBUGP(MP_TRACE, "[%p] ---> RXFlushReceiveQueue\n", Adapter);

    //
    // If VMQ enabled, then flush the receive queues for this DPC
    //
    if(VMQ_ENABLED(Adapter))
    {
        USHORT index;
        for(index =0; index < NIC_SUPPORTED_NUM_QUEUES; index++)
        {
            if(AdapterDpc->RecvBlock[index])
            {
                NICFlushReceiveBlock(Adapter, index);
            }
        }
    }
    else
    {
        NICFlushReceiveBlock(Adapter, 0);
        UNREFERENCED_PARAMETER(AdapterDpc);
    }

    DEBUGP(MP_TRACE, "[%p] <--- RXFlushReceiveQueue\n", Adapter);

}


VOID
MPCancelSend(
    _In_  NDIS_HANDLE     MiniportAdapterContext,
    _In_  PVOID           CancelId)
/*++

Routine Description:

    MiniportCancelSend cancels the transmission of all NET_BUFFER_LISTs that
    are marked with a specified cancellation identifier. Miniport drivers
    that queue send packets for more than one second should export this
    handler. When a protocol driver or intermediate driver calls the
    NdisCancelSendNetBufferLists function, NDIS calls the MiniportCancelSend
    function of the appropriate lower-level driver (miniport driver or
    intermediate driver) on the binding.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the packets with this Id should be cancelled

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(CancelId);


    DEBUGP(MP_TRACE, "[%p] ---> MPCancelSend\n", Adapter);

    //
    // This miniport completes its sends quickly, so it isn't strictly
    // neccessary to implement MiniportCancelSend.
    //
    // If we did implement it, we'd have to walk the Adapter->SendWaitList
    // and look for any NB that points to a NBL where the CancelId matches
    // NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(Nbl).  For any NB that so matches,
    // we'd remove the NB from the SendWaitList and set the NBL's status to
    // NDIS_STATUS_SEND_ABORTED, then complete the NBL.
    //

    DEBUGP(MP_TRACE, "[%p] <--- MPCancelSend\n", Adapter);
}


VOID
NICStartTheDatapath(
    _In_  PMP_ADAPTER  Adapter)
/*++

Routine Description:

    This function enables sends and receives on the data path.  It is the
    reciprocal of NICStopTheDatapath.

    Runs at IRQL == PASSIVE_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    None.

--*/
{
    PAGED_CODE();

    MPAttachAdapter(Adapter);
}


VOID
NICStopTheDatapath(
    _In_  PMP_ADAPTER  Adapter)
/*++

Routine Description:

    This function prevents future sends and receives on the data path, then
    prepares the adapter to reach an idle state.

    Although the adapter is entering an idle state, there may still be
    outstanding NBLs that haven't been returned by a protocol.  Call NICIsBusy
    to check if NBLs are still outstanding.

    Runs at IRQL == PASSIVE_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    None.

--*/
{
    BOOLEAN fResetCancelled, fSendCancelled;
    PLIST_ENTRY ReceiveListEntry;

    DEBUGP(MP_TRACE, "[%p] ---> NICStopTheDatapath.\n", Adapter);

    PAGED_CODE();

    //
    // Remove this adapter from consideration for future receives.
    //
    MPDetachAdapter(Adapter);

    //
    // Free any queued send operations
    //
    TXFlushSendQueue(Adapter, NDIS_STATUS_FAILURE);

    //
    // Prevent new calls to NICAsyncResetOrPauseDpc
    //
    fResetCancelled = NdisCancelTimerObject(Adapter->AsyncBusyCheckTimer);

    //
    // Prevent new calls to RXReceiveIndicateDpc.
    //

    for(ReceiveListEntry = Adapter->RecvDpcList.Flink;
         ReceiveListEntry != &Adapter->RecvDpcList;
         ReceiveListEntry = ReceiveListEntry->Flink)
    {
        PMP_ADAPTER_RECEIVE_DPC ReceiveDpc = CONTAINING_RECORD(ReceiveListEntry, MP_ADAPTER_RECEIVE_DPC, Entry);
        KeRemoveQueueDpc(&ReceiveDpc->Dpc);
    }

    //
    // Prevent new calls to TXSendCompleteDpc.
    //
    fSendCancelled = NdisCancelTimerObject(Adapter->SendCompleteTimer);

    //
    // Wait for any DPCs (like our reset and recv timers) that were in-progress
    // to run to completion.  This is slightly expensive to call, but we don't
    // mind calling it during MiniportHaltEx, since it's not a performance-
    // sensitive path.
    //
    KeFlushQueuedDpcs();

    if (fSendCancelled)
    {
        // Free resources associated with a pending (but cancelled) send
    }

    if (fResetCancelled)
    {
        // Free resources associated with a pending (but cancelled) reset
    }

    //
    // Double-check that there are still no queued receive operations
    //
    for(ReceiveListEntry = Adapter->RecvDpcList.Flink;
         ReceiveListEntry != &Adapter->RecvDpcList;
         ReceiveListEntry = ReceiveListEntry->Flink)
    {
        RXFlushReceiveQueue(Adapter, CONTAINING_RECORD(ReceiveListEntry, MP_ADAPTER_RECEIVE_DPC, Entry));
    }

    //
    // Double-check that there are still no queued send operations
    //
    TXFlushSendQueue(Adapter, NDIS_STATUS_FAILURE);


    DEBUGP(MP_TRACE, "[%p] <--- NICStopTheDatapath.\n", Adapter);
}

ULONG
NICGetFrameTypeFromDestination(
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress)
/*++

Routine Description:

    Reads the network frame's destination address to determine the type
    (broadcast, multicast, etc)

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    DestAddress                 The frame's destination address

Return Value:

    NDIS_PACKET_TYPE_BROADCAST
    NDIS_PACKET_TYPE_MULTICAST
    NDIS_PACKET_TYPE_DIRECTED

--*/
{
    if (NIC_ADDR_IS_BROADCAST(DestAddress))
    {
        return NDIS_PACKET_TYPE_BROADCAST;
    }
    else if(NIC_ADDR_IS_MULTICAST(DestAddress))
    {
        return NDIS_PACKET_TYPE_MULTICAST;
    }
    else
    {
        return NDIS_PACKET_TYPE_DIRECTED;
    }
}

