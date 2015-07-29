/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    NDIS protocol entry points and utility routines to handle receiving
    data.

Environment:

    Kernel mode only.

--*/

#include "precomp.h"

#define __FILENUMBER 'VCER'


VOID
NdisProtEvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_READ requests.
    We will just read the file.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Length  - number of bytes to be read.

Return Value:

  None.

--*/
{
    NTSTATUS                 NtStatus;
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    WDFFILEOBJECT            fileObject;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    fileObject = WdfRequestGetFileObject(Request);

    pOpenContext = GetFileObjectContext(fileObject)->OpenContext;

    do
    {
        //
        // Validate!
        //
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL FsContext on FileObject %p\n", fileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        //
        // Forward this request to the pending read Request queue. We don't have to
        // worry about cancelation of requests pending in the queue because
        // framework takes care of that.
        //
        NtStatus = WdfRequestForwardToIoQueue(Request,
                                    pOpenContext->ReadQueue);

    }
    while (FALSE);

    if (!NT_SUCCESS(NtStatus))
    {
        WdfRequestCompleteWithInformation(Request, NtStatus, 0);
    }

    return;
}

VOID
ndisprotEvtNotifyReadQueue(
    IN WDFQUEUE            Queue,
    IN WDFCONTEXT          Context
    )
/*++

Routine Description:

    This event will be called every time the number of requests in the
    queue changes from 0 to 1.

Arguments:

    Queue - Read queue
    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext = (PNDISPROT_OPEN_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Queue);

    ndisprotServiceReads(pOpenContext);
}


VOID
ndisprotServiceReads(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Utility routine to copy received data into user buffers and
    complete READ IRPs.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PNET_BUFFER_LIST    pRcvNetBufList;
    PLIST_ENTRY         pRcvNetBufListEntry;
    PUCHAR              pSrc, pDst;
    ULONG               BytesRemaining; // at pDst
    PMDL                pMdl;
    ULONG               BytesAvailable;
    NTSTATUS            ntStatus = STATUS_UNSUCCESSFUL;
    WDFREQUEST          request;
    ULONG               bytesCopied = 0, totalLength;

    DEBUGP(DL_VERY_LOUD, ("ServiceReads: open %p/%x\n",
            pOpenContext, pOpenContext->Flags));

    NPROT_REF_OPEN(pOpenContext);  // temp ref - service reads

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

    while (NPROT_IS_LIST_EMPTY(&pOpenContext->RecvNetBufListQueue) == FALSE)
    {
        //
        //  Get the first pended Read Request
        //
        ntStatus = WdfIoQueueRetrieveNextRequest(
                         pOpenContext->ReadQueue,
                         &request
                         );
        if(!NT_SUCCESS(ntStatus)){
            ASSERTMSG("WdfIoQueueRetrieveNextRequest failed",  ntStatus == STATUS_NO_MORE_ENTRIES);
            break;
        }

        ntStatus = WdfRequestRetrieveOutputWdmMdl(request, &pMdl);
        if (!NT_SUCCESS(ntStatus))
        {
            DEBUGP(DL_FATAL, ("Read: WdfRequestRetrieveOutputWdmMdl %x\n", ntStatus));
            break;
        }

        pDst = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
        if (pDst == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: MmGetSystemAddr failed for Request %p, MDL %p\n",
                    request, pDst));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  Get the first queued receive packet
        //
        pRcvNetBufListEntry = pOpenContext->RecvNetBufListQueue.Flink;
        NPROT_REMOVE_ENTRY_LIST(pRcvNetBufListEntry);

        pOpenContext->RecvNetBufListCount --;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        NPROT_DEREF_OPEN(pOpenContext);  // Service: dequeue rcv packet


        pRcvNetBufList = NPROT_RCV_NBL_FROM_LIST_ENTRY(pRcvNetBufListEntry);
        NPROT_RCV_NBL_FROM_LIST_ENTRY(pRcvNetBufListEntry) = NULL;

		NPROT_ASSERT(pRcvNetBufList != NULL);
		NPROT_ASSERT(pRcvNetBufList->FirstNetBuffer != NULL);

		totalLength = BytesRemaining = MmGetMdlByteCount(pMdl);
        pMdl = pRcvNetBufList->FirstNetBuffer->MdlChain;

        //
        // Copy the data in the received packet into the buffer provided by the client.
        // If the length of the receive packet is greater than length of the given buffer,
        // we just copy as many bytes as we can. Once the buffer is full, we just discard
        // the rest of the data, and complete the IRP sucessfully even we only did a partial copy.
        //

        while (BytesRemaining && (pMdl != NULL))
        {
            pSrc = NULL;
            NdisQueryMdl(pMdl, &pSrc, &BytesAvailable, NormalPagePriority);

            if (pSrc == NULL)
            {
                DEBUGP(DL_FATAL,
                    ("ServiceReads: Open %p, NdisQueryMdl failed for MDL %p\n",
                            pOpenContext, pMdl));
                break;
            }

            if (BytesAvailable)
            {
                ULONG       BytesToCopy = MIN(BytesAvailable, BytesRemaining);

                NPROT_COPY_MEM(pDst, pSrc, BytesToCopy);
                BytesRemaining -= BytesToCopy;
                pDst += BytesToCopy;
            }

            NdisGetNextMdl(pMdl, &pMdl);
        }

        //
        //  Complete the request.
        //
        bytesCopied = totalLength - BytesRemaining;

        DEBUGP(DL_INFO, ("ServiceReads: Open %p, IRP %p completed with %d bytes\n",
            pOpenContext, request, bytesCopied));

        WdfRequestCompleteWithInformation(request, STATUS_SUCCESS, bytesCopied);

        ndisprotFreeReceiveNetBufferList(pOpenContext, pRcvNetBufList,FALSE);

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
        pOpenContext->PendedReadCount--;

    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

    NPROT_DEREF_OPEN(pOpenContext);    // temp ref - service reads
}


VOID
NdisprotReceiveNetBufferLists(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNET_BUFFER_LIST             pNetBufferLists,
    IN NDIS_PORT_NUMBER             PortNumber,
    IN ULONG                        NumberOfNetBufferLists,
    IN ULONG                        ReceiveFlags
    )
/*++

Routine Description:

    Protocol entry point called by NDIS if the driver below
    uses NDIS 6 net buffer list indications.

    If the miniport allows us to hold on to this net buffer list, we
    use it as is, otherwise we make a copy.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNetBufferLists - a list of the Net Buffer lists being indicated up.
    PortNumber - Port on which the Net Bufer list was received
    NumberOfNetBufferLists - the number of NetBufferLists in this indication
    ReceiveFlags - indicates whether the NetBufferLists can be pended in
                   the protocol driver.

Return Value:

--*/
{
    PNDISPROT_OPEN_CONTEXT  pOpenContext;
    PMDL                    pMdl = NULL;
    UINT                    BufferLength;
    PNDISPROT_ETH_HEADER    pEthHeader = NULL;
    PNET_BUFFER_LIST        pCopyNetBufList;
    PUCHAR                  pCopyBuf;
    UINT                    TotalLength;
    SIZE_T                  BytesCopied;
    PNET_BUFFER_LIST        pNetBufList;
    PNET_BUFFER_LIST        pNextNetBufList;
    PNET_BUFFER_LIST        pReturnNetBufList = NULL;
    PNET_BUFFER_LIST        pLastReturnNetBufList = NULL;
    NTSTATUS                NtStatus;
    BOOLEAN                 bAcceptedReceive;
    ULONG                   Offset;
    ULONG                   ReturnFlags = 0;
    BOOLEAN                 DispatchLevel;

    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(NumberOfNetBufferLists);

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;

    if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags))
    {
        NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
    }


    NPROT_STRUCT_ASSERT(pOpenContext, oc);
    if ((pOpenContext->State == NdisprotPausing)
        || (pOpenContext->State == NdisprotPaused))
    {
        if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags) == TRUE)
        {

            NdisReturnNetBufferLists(pOpenContext->BindingHandle,
                                    pNetBufferLists,
                                    ReturnFlags);
        }
        return;
    }

    pNetBufList = pNetBufferLists;

    while (pNetBufList != NULL)
    {
        pNextNetBufList = NET_BUFFER_LIST_NEXT_NBL (pNetBufList);

        NBL_CLEAR_PROT_RSVD_FLAG(pNetBufList, NBL_PROT_RSVD_FLAGS);
        bAcceptedReceive = FALSE;

        //
        // Get first MDL and data length in the list
        //
        pMdl = pNetBufList->FirstNetBuffer->CurrentMdl;
        TotalLength = pNetBufList->FirstNetBuffer->DataLength;
        Offset = pNetBufList->FirstNetBuffer->CurrentMdlOffset;
        BufferLength = 0;

        do
        {
            ASSERT(pMdl != NULL);
            if (pMdl)
            {
                NdisQueryMdl(
                    pMdl,
                    &pEthHeader,
                    &BufferLength,
                    NormalPagePriority);
            }

            if (pEthHeader == NULL)
            {
                //
                //  The system is low on resources. Set up to handle failure
                //  below.
                //
                BufferLength = 0;
                break;
            }

            if (BufferLength == 0)
            {
                break;
            }

            ASSERT(BufferLength > Offset);

            BufferLength -= Offset;
            pEthHeader = (PNDISPROT_ETH_HEADER)((PUCHAR)pEthHeader + Offset);

            if (BufferLength < sizeof(NDISPROT_ETH_HEADER))
            {
                DEBUGP(DL_WARN,
                    ("ReceiveNetBufferList: Open %p, runt nbl %p, first buffer length %d\n",
                        pOpenContext, pNetBufList, BufferLength));

                break;
            }

            //
            //  Check the EtherType. If the Ether type indicates presence of
            //  a tag, then the "real" Ether type is 4 bytes further down.
            //
            if (pEthHeader->EthType == NPROT_8021P_TAG_TYPE)
            {
                USHORT UNALIGNED *pEthType;

                if (BufferLength < (sizeof(NDISPROT_ETH_HEADER) + 4))
                {
                    break;
                }

                pEthType = (USHORT UNALIGNED *)((PUCHAR)&pEthHeader->EthType + 4);

                if (*pEthType != Globals.EthType)
                {
                    break;
                }
            }
            else if (pEthHeader->EthType != Globals.EthType)
            {
                break;
            }

            bAcceptedReceive = TRUE;
            DEBUGP(DL_LOUD, ("ReceiveNetBufferList: Open %p, interesting nbl %p\n",
                        pOpenContext, pNetBufList));

            //
            //  If the miniport is out of resources, we can't queue
            //  this list of net buffer list - make a copy if this is so.
            //
            DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);

            if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags))
            {
                pCopyNetBufList = ndisprotAllocateReceiveNetBufferList(
                                pOpenContext,
                                TotalLength,
                                &pCopyBuf);

                if (pCopyNetBufList == NULL)
                {
                    DEBUGP(DL_FATAL, ("ReceiveNetBufferList: Open %p, failed to"
                        " alloc copy, %d bytes\n", pOpenContext, TotalLength));
                    break;
                }
                NBL_SET_PROT_RSVD_FLAG(pCopyNetBufList, NPROT_ALLOCATED_NBL);
                //
                // Copy the data to the new allocated NetBufferList
                //
                NtStatus = ndisprotCopyMdlToMdl(
                        pNetBufList->FirstNetBuffer->MdlChain,
                        pNetBufList->FirstNetBuffer->DataOffset,
                        pCopyNetBufList->FirstNetBuffer->MdlChain,
                        0,
                        TotalLength,
                        &BytesCopied);

                if (NtStatus != STATUS_SUCCESS)
                {
                    DEBUGP(DL_FATAL, ("ReceiveNetBufferList: Open %p, failed to"
                        " copy the data, %d bytes\n", pOpenContext, TotalLength));
                    //
                    // Free the NetBufferList and memory allocate before
                    //
                    ndisprotFreeReceiveNetBufferList(pOpenContext,
                                                    pCopyNetBufList,
                                                    DispatchLevel);
                    break;
                }

                NPROT_ASSERT(BytesCopied == TotalLength);
                pNetBufList = pCopyNetBufList;

            }
            //
            //  Queue this up and service any pending Read IRPs.
            //
            ndisprotQueueReceiveNetBufferList(pOpenContext, pNetBufList, DispatchLevel);

        }
        while (FALSE);

        //
        // Ndisprot is not interested this NetBufferList, return the
        // NetBufferList back to the miniport if the miniport gave us
        // ownership of it
        //
        if ((bAcceptedReceive == FALSE) &&
            (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags) == TRUE))
        {
            if (pReturnNetBufList == NULL)
            {
                pReturnNetBufList = pNetBufList;
            }
            else
            {
                NET_BUFFER_LIST_NEXT_NBL(pLastReturnNetBufList) = pNetBufList;
            }
            pLastReturnNetBufList = pNetBufList;
            NET_BUFFER_LIST_NEXT_NBL(pNetBufList) = NULL;

        }

        pNetBufList = pNextNetBufList;
    } // end of the for loop

    if (pReturnNetBufList != NULL)
    {
        NdisReturnNetBufferLists(pOpenContext->BindingHandle,
                                    pReturnNetBufList,
                                    ReturnFlags);
    }

}


VOID
ndisprotQueueReceiveNetBufferList(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNET_BUFFER_LIST              pRcvNetBufList,
    IN BOOLEAN                       DispatchLevel
    )
/*++

Routine Description:

    Queue up a received net buffer list on the open context structure.
    If the queue size goes beyond a water mark, discard a Net Buffer list
    at the head of the queue.

    Finally, run the queue service routine.

Arguments:

    pOpenContext - pointer to open context
    pRcvPacket - the received packet
    DipatchLevel - the irql level

Return Value:

    None

--*/
{
    PLIST_ENTRY        pEnt;
    PLIST_ENTRY        pDiscardEnt;
    PNET_BUFFER_LIST   pDiscardNetBufList;

    do
    {

        NPROT_REF_OPEN(pOpenContext);    // queued rcv net buffer list

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, DispatchLevel);

        if ((pOpenContext->State == NdisprotPaused)
            || (pOpenContext->State == NdisprotPausing))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, DispatchLevel);

            ndisprotFreeReceiveNetBufferList(pOpenContext, pRcvNetBufList, DispatchLevel);
            break;
        }

        //
        //  Check if the binding is in the proper state to receive
        //  this net buffer list.
        //
        if (NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE) &&
            (pOpenContext->PowerState == NetDeviceStateD0))
        {

            //
            // Queue the net buffer list
            //
            pEnt = NPROT_RCV_NBL_TO_LIST_ENTRY(pRcvNetBufList);
            NPROT_INSERT_TAIL_LIST(&pOpenContext->RecvNetBufListQueue, pEnt);
            NPROT_RCV_NBL_FROM_LIST_ENTRY(pEnt) = pRcvNetBufList;
            pOpenContext->RecvNetBufListCount++;

            DEBUGP(DL_VERY_LOUD, ("QueueReceiveNetBufferList: open %p,"
                    " queued nbl %p, queue size %d\n",
                    pOpenContext, pRcvNetBufList, pOpenContext->RecvNetBufListCount));

        }
        else
        {
            //
            //  Received this net buffer list when the binding is going away.
            //  Drop this.
            //
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, DispatchLevel);

            ndisprotFreeReceiveNetBufferList(pOpenContext, pRcvNetBufList, DispatchLevel);

            NPROT_DEREF_OPEN(pOpenContext);  // dropped rcv packet - bad state
            break;
        }


        //
        //  Trim the queue if it has grown too big.
        //
        if (pOpenContext->RecvNetBufListCount > MAX_RECV_QUEUE_SIZE)
        {
            //
            //  Remove the head of the queue.
            //
            pDiscardEnt = pOpenContext->RecvNetBufListQueue.Flink;
            NPROT_REMOVE_ENTRY_LIST(pDiscardEnt);

            pOpenContext->RecvNetBufListCount --;

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, DispatchLevel);

            pDiscardNetBufList = NPROT_RCV_NBL_FROM_LIST_ENTRY(pDiscardEnt);

            NPROT_RCV_NBL_FROM_LIST_ENTRY(pDiscardEnt) = NULL;

            ndisprotFreeReceiveNetBufferList(pOpenContext, pDiscardNetBufList, DispatchLevel);

            NPROT_DEREF_OPEN(pOpenContext);  // dropped rcv packet - queue too long

            DEBUGP(DL_INFO, ("QueueReceiveNetBufferList: open %p queue"
                    " too long, discarded  %p\n",
                    pOpenContext, pDiscardNetBufList));
        }
        else
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, DispatchLevel);
        }

        //
        //  Run the receive queue service routine now.
        //
        ndisprotServiceReads(pOpenContext);
    }
    while (FALSE);
}


PNET_BUFFER_LIST
ndisprotAllocateReceiveNetBufferList(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN UINT                          DataLength,
    OUT PUCHAR *                     ppDataBuffer
    )
/*++

Routine Description:

    Allocate resources to copy and queue a received net buffer list

Arguments:

    pOpenContext - pointer to open context for received packet
    DataLength - total length in bytes of the net buffer list's first net buffer
    ppDataBuffer - place to return pointer to allocated buffer

Return Value:

    Pointer to NDIS packet if successful, else NULL.

--*/
{
    PNET_BUFFER_LIST            pNetBufList;
    PMDL                        pMdl;
    PUCHAR                      pDataBuffer;

    pNetBufList = NULL;
    pMdl = NULL;
    pDataBuffer = NULL;

    do
    {
        NPROT_ALLOC_MEM(pDataBuffer, DataLength);

        if (pDataBuffer == NULL)
        {
            DEBUGP(DL_FATAL, ("AllocRcvNbl: open %p, failed to alloc"
                " data buffer %d bytes\n", pOpenContext, DataLength));
            break;
        }

        //
        //  Make this an NDIS buffer.
        //
        pMdl = NdisAllocateMdl(pOpenContext->BindingHandle, pDataBuffer, DataLength);

        if (pMdl == NULL)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvNbl: open %p, failed to alloc"
                " MDL, %d bytes\n", pOpenContext, DataLength));
            break;
        }

        pNetBufList = NdisAllocateNetBufferAndNetBufferList(
                        pOpenContext->RecvNetBufferListPool,
                        0,                              // ContextSize
                        0,                              // ContextBackfill
                        pMdl,                    // MdlChain
                        0,                              // DataOffset
                        DataLength);                   // DataLength

        if (pNetBufList == NULL)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvNbl: open %p, failed to alloc"
                " Net Buffer List, %d bytes\n", pOpenContext, DataLength));
            break;
        }


        *ppDataBuffer = pDataBuffer;

    }
    while (FALSE);

    if (pNetBufList == NULL)
    {
        //
        //  Clean up
        //
        if (pMdl != NULL)
        {
            NdisFreeMdl(pMdl);
        }

        if (pDataBuffer != NULL)
        {
            NPROT_FREE_MEM(pDataBuffer);
        }
    }

    return (pNetBufList);
}



VOID
ndisprotFreeReceiveNetBufferList(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNET_BUFFER_LIST              pNetBufferList,
    IN BOOLEAN                       DispatchLevel
    )
/*++

Routine Description:

    Free up all resources associated with a received net buffer list. If this
    is a local copy, free the net buffer list to our receive pool, else return
    this to the miniport.

Arguments:

    pOpenContext - pointer to open context
    pNetBufferList - pointer to net buffer list to be freed.
    DipatchLevel - the irql level

Return Value:

    None

--*/
{
    PMDL                pMdl;
    UINT                TotalLength;
    UINT                BufferLength;
    PUCHAR              pCopyData = NULL;
    ULONG               ReturnFlags = 0;


    do
    {
        if (NBL_TEST_PROT_RSVD_FLAG(pNetBufferList, NPROT_ALLOCATED_NBL))
        {
            //
            //  This is a local copy.
            //

            pMdl = pNetBufferList->FirstNetBuffer->MdlChain;
            TotalLength = pNetBufferList->FirstNetBuffer->DataLength;

            NPROT_ASSERT(pMdl != NULL);

            NdisQueryMdl(
                pMdl,
                (PVOID *)&pCopyData,
                &BufferLength,
                NormalPagePriority);

            NPROT_ASSERT(BufferLength == TotalLength);


            NPROT_ASSERT(pCopyData != NULL); // we would have allocated non-paged pool


            NdisFreeNetBufferList(pNetBufferList);

            NdisFreeMdl(pMdl);

            NPROT_FREE_MEM(pCopyData);
            break;
        }
        //
        // The NetBufferList should be returned

        NET_BUFFER_LIST_NEXT_NBL(pNetBufferList) = NULL;

        if (DispatchLevel)
        {
            NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
        }

        NdisReturnNetBufferLists(pOpenContext->BindingHandle,
                                  pNetBufferList,
                                  ReturnFlags);
    }
    while (FALSE);

}


VOID
ndisprotFlushReceiveQueue(
    IN PNDISPROT_OPEN_CONTEXT            pOpenContext
    )
/*++

Routine Description:

    Free any receive packets queued up on the specified open

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PLIST_ENTRY         pRcvNetBufListEntry;
    PNET_BUFFER_LIST    pRcvNetBufList;

    NPROT_REF_OPEN(pOpenContext);  // temp ref - flushRcvQueue

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

    while (!NPROT_IS_LIST_EMPTY(&pOpenContext->RecvNetBufListQueue))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvNetBufListEntry = pOpenContext->RecvNetBufListQueue.Flink;
        NPROT_REMOVE_ENTRY_LIST(pRcvNetBufListEntry);

        pOpenContext->RecvNetBufListCount--;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        pRcvNetBufList = NPROT_RCV_NBL_FROM_LIST_ENTRY(pRcvNetBufListEntry);
        NPROT_RCV_NBL_FROM_LIST_ENTRY(pRcvNetBufListEntry) = NULL;

        DEBUGP(DL_LOUD, ("FlushReceiveQueue: open %p, nbl %p\n",
            pOpenContext, pRcvNetBufList));

        ndisprotFreeReceiveNetBufferList(pOpenContext, pRcvNetBufList, FALSE);

        NPROT_DEREF_OPEN(pOpenContext);    // took out pended Read

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

    NPROT_DEREF_OPEN(pOpenContext);    // temp ref - flushRcvQueue
}



NTSTATUS
ndisprotCopyMdlToMdl(
    IN PMDL     SourceMdl,
    IN SIZE_T   SourceOffset,
    IN PMDL     TargetMdl,
    IN SIZE_T   TargetOffset,
    IN SIZE_T   BytesToCopy,
    OUT SIZE_T* BytesCopied
    )

/*++

Routine Description:

    This routine copies the contents of one MDL chain into another MDL chain.
    A maximum of BytesToCopy bytes will be copied.
    The actual number of bytes copied is returned in BytesCopied.

Arguments:

    SourceMdl - Supplies the source of the data to be copied.

    SourceOffset - Supplies the offset into the source from which to begin.

    TargetMdl - Supplies the target to which data should be copied.

    TargetOffset - Supplies the offset into the target at which to begin.

    BytesToCopy - Supplies the number of bytes to copy.

    BytesCopied - Returns the actual number of bytes copied.

Return Value:

    STATUS_SUCCESS - BytesCopied indicates the number of bytes successfully
        transferred.

    STATUS_INSUFFICIENT_RESOURCES - if one of the MDLs could not be mapped.

Caller IRQL: Must be running at IRQL <= DISPATCH_LEVEL.

--*/

{
    SIZE_T SourceByteCount, TargetByteCount, BytesRemaining, CopySize;
    PUCHAR SourceVa, TargetVa;

    //
    // Skip any offsets specified by the caller. Note that this also serves
    // to skip any zero-length MDLs at the front of the chains,
    // simplifying the logic below.
    //

    while (SourceMdl && SourceOffset >= MmGetMdlByteCount(SourceMdl))
    {
        SourceOffset -= MmGetMdlByteCount(SourceMdl);
        SourceMdl = SourceMdl->Next;
    }

    while (TargetMdl && TargetOffset >= MmGetMdlByteCount(TargetMdl))
    {
        TargetOffset -= MmGetMdlByteCount(TargetMdl);
        TargetMdl = TargetMdl->Next;
    }

    //
    // Determine whether any data transfer will actually occur and,
    // if so, enter the main transfer stage.
    //
    if (BytesToCopy && SourceMdl && TargetMdl)
    {
        BytesRemaining = BytesToCopy;

        //
        // Compute the length for the first source MDL,
        // and obtain a virtual address for it.
        //

        SourceByteCount = MmGetMdlByteCount(SourceMdl) - SourceOffset;
        if (SourceByteCount > BytesRemaining)
        {
            SourceByteCount = BytesRemaining;
        }

        SourceVa = MmGetSystemAddressForMdlSafe(SourceMdl, LowPagePriority);
        if (SourceVa == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        SourceVa += SourceOffset;

        //
        // Compute the length for the first target MDL,
        // and obtain a virtual address for it.
        //

        TargetByteCount = MmGetMdlByteCount(TargetMdl) - TargetOffset;

        TargetVa = MmGetSystemAddressForMdlSafe(TargetMdl, LowPagePriority);
        if (TargetVa == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        TargetVa += TargetOffset;

        //
        // Transfer data between the MDL chains until the data are exhausted
        // or the end of one of the MDL chains is encountered.
        //

        for (;;)
        {

            //
            // Copy the current installment and return if done.
            // Otherwise, update the count of bytes remaining.
            //
            CopySize = min(TargetByteCount, SourceByteCount);
            RtlCopyMemory(TargetVa, SourceVa, CopySize);

            if (BytesRemaining == CopySize)
            {
                *BytesCopied = BytesToCopy;
                return STATUS_SUCCESS;
            }

            BytesRemaining -= CopySize;

            //
            // Advance to the next MDL in the target chain if necessary,
            // otherwise update our pointer into the current entry.
            //

            if (TargetByteCount == CopySize)
            {

                do
                {
                    TargetMdl = TargetMdl->Next;
                    if (TargetMdl == NULL)
                    {
                        *BytesCopied = BytesToCopy - BytesRemaining;
                        return STATUS_SUCCESS;
                    }
                    TargetByteCount = MmGetMdlByteCount(TargetMdl);
                } while(TargetByteCount == 0);

                TargetVa = MmGetSystemAddressForMdlSafe(TargetMdl,
                                                        LowPagePriority);
                if (TargetVa == NULL)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                TargetVa += CopySize;
                TargetByteCount -= CopySize;
            }

            //
            // Advance to the next MDL in the source chain if necessary,
            // otherwise update our pointer into the current entry.
            //
            if (SourceByteCount == CopySize)
            {

                do
                {
                    SourceMdl = SourceMdl->Next;
                    if (SourceMdl == NULL)
                    {
                        *BytesCopied = BytesToCopy - BytesRemaining;
                        return STATUS_SUCCESS;
                    }
                    SourceByteCount = MmGetMdlByteCount(SourceMdl);
                } while(SourceByteCount == 0);

                if (SourceByteCount > BytesRemaining)
                {
                    SourceByteCount = BytesRemaining;
                }
                SourceVa = MmGetSystemAddressForMdlSafe(SourceMdl,
                                                        LowPagePriority);
                if (SourceVa == NULL)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                SourceVa += CopySize;
                SourceByteCount -= CopySize;
            }
        }
    }

    *BytesCopied = 0;
    return STATUS_SUCCESS;
}



