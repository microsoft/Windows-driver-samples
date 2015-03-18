/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

        TcbRcb.C

Abstract:

    This module contains miniport functions for handling Send & Receive
    packets and other helper routines called by these miniport functions.

    In order to excercise the send and receive code path of this driver,
    you should install more than one instance of the miniport. If there
    is only one instance installed, the driver throws the send packet on
    the floor and completes the send successfully. If there are more
    instances present, it indicates the incoming send packet to the other
    instances. For example, if there 3 instances: A, B, & C installed.
    Packets coming in for A instance would be indicated to B & C; packets
    coming into B would be indicated to C, & A; and packets coming to C
    would be indicated to A & B.

Revision History:

Notes:

--*/

#include "netvmin6.h"
#include "tcbrcb.tmh"


VOID
ReturnTCB(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PTCB         Tcb)
{
    TXNblRelease(Adapter, NBL_FROM_SEND_NB(Tcb->NetBuffer), TRUE);
    Tcb->NetBuffer = NULL;

    NdisInterlockedInsertTailList(
            &Adapter->FreeTcbList,
            &Tcb->TcbLink,
            &Adapter->FreeTcbListLock);
}



_Must_inspect_result_
_Success_(return != NULL)
PRCB
GetRCB(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  PFRAME       Frame)
/*++

Routine Description:

    This routine gets an unused RCB from the pool.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     The receiving adapter
    Nbl1QInfo                   8021Q Tag information for the FRAME being received
    Frame                       The frame that will be attached to the RCB

Return Value:

    NULL if an RCB could not be allocated.
    Else, a pointer to an initialized RCB.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_RESOURCES;
    PRCB Rcb = NULL;

    DEBUGP(MP_TRACE, "[%p] ---> GetRCB.\n", Adapter);

    if(VMQ_ENABLED(Adapter))
    {
        //
        // Retrieve the RCB from the target VMQ queue for the frame
        //
        GetRcbForRxQueue(Adapter, Frame, Nbl1QInfo, &Rcb);
    }
    else
    {
        //
        // Retrieve the RCB from the global RCB pool
        //
        PLIST_ENTRY pEntry = NdisInterlockedRemoveHeadList(
                &Adapter->FreeRcbList,
                &Adapter->FreeRcbListLock);
        if (pEntry)
        {
            Rcb = CONTAINING_RECORD(pEntry, RCB, RcbLink);
            //
            // Receiving on the default receive queue, increment its pending count
            //
            Status = NICReferenceReceiveBlock(Adapter, 0);
            if(Status != NDIS_STATUS_SUCCESS)
            {
                //
                // The adapter is no longer in a ready state, so we were not able to take a reference on the
                // receive block. Add the RCB back to the free list and fail this receive. 
                //
                NdisInterlockedInsertTailList(
                    &Adapter->FreeRcbList,
                    &Rcb->RcbLink,
                    &Adapter->FreeRcbListLock);
                Rcb = NULL;
            }
        }
    }

    if (Rcb)
    {
        //
        // Simulate the hardware DMA'ing the received frame into the NB's MDL.
        //
        Status = HWBeginReceiveDma(Adapter, Nbl1QInfo, Rcb, Frame);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_TRACE, "[%p] HWBeginReceiveDma failed with error 0x%08x, aborting RCB allocation.\n", Adapter, Status);
            //
            // Increase failure counters if appropriate
            //
            if(Status == NDIS_STATUS_RESOURCES)
            {
                ++Adapter->RxResourceErrors;   
            }
            else if(Status != NDIS_STATUS_INVALID_ADDRESS)
            {
                ++Adapter->RxRuntErrors;
            }
            //
            // Recover RCB 
            //
            ReturnRCB(Adapter, Rcb);
            Rcb = NULL;
        }
    }
    else
    {
        DEBUGP(MP_LOUD, "[%p] An RCB could not be retrieved. Status: 0x%08x.\n", Adapter, Status);
        ++Adapter->RxResourceErrors;
    }

    DEBUGP(MP_LOUD, "[%p] Allocated RCB: %p.", Adapter, Rcb);
    DEBUGP(MP_TRACE, "[%p] <--- GetRCB.\n", Adapter);

    return Rcb;
}


VOID
ReturnRCB(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PRCB          Rcb)
/*++

Routine Description:

    This routine frees an RCB back to the unused pool, recovers relevant memory used in the RCB.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter     - The receiving adapter (the one that owns the RCB).
    Rcb         - The RCB to be freed.

Return Value:

    None.

--*/
{
    PUCHAR Data = Rcb->Data;
    ASSERT(Data); 

    DEBUGP(MP_TRACE, "[%p] ---> ReturnRCB. RCB: %p\n", Adapter, Rcb);

    if(VMQ_ENABLED(Adapter))
    {
        //
        // Recover RCB back to owner VMQ queue
        //
        RecoverRxQueueRcb(Adapter, Rcb);
    }
    else
    {
        //
        // Recover RCB to global RCB pool
        //
        NdisInterlockedInsertTailList(
                &Adapter->FreeRcbList,
                &Rcb->RcbLink,
                &Adapter->FreeRcbListLock);
        Rcb = NULL;
        //
        // We receive on the default receive queue, decrement its pending count
        //
        NICDereferenceReceiveBlock(Adapter, 0, NULL);
        HWFrameRelease((PFRAME)Data);
    }

    DEBUGP(MP_TRACE, "[%p] <--- ReturnRCB.\n", Adapter);

}


