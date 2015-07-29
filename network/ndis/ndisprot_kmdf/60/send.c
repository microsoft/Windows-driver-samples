/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    send.c

Abstract:

    NDIS protocol entry points and utility routines to handle sending
    data.

Environment:

    Kernel mode only.

--*/

#include "precomp.h"

#define __FILENUMBER 'DNES'

VOID
NdisProtEvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
/*++

Routine Description:

    Dispatch routine to handle Request_MJ_WRITE.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

    VOID

--*/
{
    ULONG                   DataLength;
    NTSTATUS                NtStatus;
    PNDISPROT_OPEN_CONTEXT  pOpenContext;
    PNET_BUFFER_LIST        pNetBufferList;
    NDISPROT_ETH_HEADER UNALIGNED *pEthHeader;
    PVOID                   CancelId;
    ULONG                   SendFlags = 0;
    PMDL                    pMdl = NULL;
    WDFFILEOBJECT           fileObject;
    PREQUEST_CONTEXT        reqContext;
    WDF_OBJECT_ATTRIBUTES   attributes;

    UNREFERENCED_PARAMETER(Queue);

    fileObject = WdfRequestGetFileObject(Request);
    pOpenContext = GetFileObjectContext(fileObject)->OpenContext;

    do
    {
        //
        // Create a context to track the length of transfer and NDIS packet
        // associated with this request.
        //
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);

        NtStatus = WdfObjectAllocateContext(Request, &attributes, &reqContext);
        if(!NT_SUCCESS(NtStatus)){
            DEBUGP(DL_WARN, ("Write: WdfObjectAllocateContext failed: %x\n", NtStatus));
            NtStatus = STATUS_INVALID_HANDLE;
            break;

        }

        reqContext->Length = (ULONG) Length;

        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("Write: FileObject %p not yet associated with a device\n",
                fileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        NtStatus = WdfRequestRetrieveInputWdmMdl(Request, &pMdl);
        if (!NT_SUCCESS(NtStatus))
        {
            DEBUGP(DL_FATAL, ("Write: WdfRequestRetrieveInputWdmMdl failed %x\n", NtStatus));
            break;
        }

        //
        // Try to get a virtual address for the MDL.
        //
        pEthHeader = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

        if (pEthHeader == NULL)
        {
            DEBUGP(DL_FATAL, ("Write: MmGetSystemAddr failed for"
                    " Request %p, MDL %p\n",
                    Request, pMdl));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Sanity-check the length.
        //
        DataLength = MmGetMdlByteCount(pMdl);
        if (DataLength < sizeof(NDISPROT_ETH_HEADER))
        {
            DEBUGP(DL_WARN, ("Write: too small to be a valid packet (%d bytes)\n",
                DataLength));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (DataLength > (pOpenContext->MaxFrameSize + sizeof(NDISPROT_ETH_HEADER)))
        {
            DEBUGP(DL_WARN, ("Write: Open %p: data length (%d)"
                    " larger than max frame size (%d)\n",
                    pOpenContext, DataLength, pOpenContext->MaxFrameSize));

            NtStatus = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // To prevent applications from sending packets with spoofed
        // mac address, we will do the following check to make sure the source
        // address in the packet is same as the current MAC address of the NIC.
        //
        if ((WdfRequestGetRequestorMode(Request) == UserMode) &&
            !NPROT_MEM_CMP(pEthHeader->SrcAddr, pOpenContext->CurrentAddress, NPROT_MAC_ADDR_LEN))
        {
            DEBUGP(DL_WARN, ("Write: Failing with invalid Source address"));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            DEBUGP(DL_FATAL, ("Write: Open %p is not bound"
            " or in low power state\n", pOpenContext));

            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        if ((pOpenContext->State == NdisprotPaused)
            || (pOpenContext->State == NdisprotPausing))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            DEBUGP(DL_INFO, ("Device is paused.\n"));
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        NPROT_ASSERT(pOpenContext->SendNetBufferListPool != NULL);
        pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
                                pOpenContext->SendNetBufferListPool,
                                sizeof(NPROT_SEND_NETBUFLIST_RSVD), //Request control offset delta
                                0,           // back fill size
                                pMdl,
                                0,          // Data offset
                                DataLength);

        if (pNetBufferList == NULL)
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            DEBUGP(DL_FATAL, ("Write: open %p, failed to alloc send net buffer list\n",
                    pOpenContext));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        pOpenContext->PendedSendCount++;

        NPROT_REF_OPEN(pOpenContext);  // pended send

        //
        //  Initialize the NetBufferList ref count. This NetBufferList will be freed
        //  when this count goes to zero.
        //
        NPROT_SEND_NBL_RSVD(pNetBufferList)->RefCount = 1;

        //
        //  We set up a cancel ID on each send NetBufferList (which maps to a Write IRP),
        //  and save the NetBufferList pointer in the IRP. If the IRP gets cancelled, we use
        //  NdisCancelSendNetBufferLists() to cancel the NetBufferList.
        //
        //  Note that this sample code does not implement the cancellation logic. An actual
        //  driver may find value in implementing this.
        //

        CancelId = NPROT_GET_NEXT_CANCEL_ID();
        NDIS_SET_NET_BUFFER_LIST_CANCEL_ID(pNetBufferList, CancelId);
        reqContext->NetBufferList = (PVOID)pNetBufferList;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Set a back pointer from the packet to the IRP.
        //
        NPROT_REQUEST_FROM_SEND_NBL(pNetBufferList) = Request;

        NtStatus = STATUS_PENDING;

        pNetBufferList->SourceHandle = pOpenContext->BindingHandle;
        NPROT_ASSERT (pMdl->Next == NULL);

        SendFlags |= NDIS_SEND_FLAGS_CHECK_FOR_LOOPBACK;

        NdisSendNetBufferLists(
                        pOpenContext->BindingHandle,
                        pNetBufferList,
                        NDIS_DEFAULT_PORT_NUMBER,
                        SendFlags);

    }
    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        WdfRequestComplete(Request, NtStatus);
    }

    return;
}


VOID
NdisprotSendComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNET_BUFFER_LIST             pNetBufferList,
    IN ULONG                        SendCompleteFlags
    )
/*++

Routine Description:

    NDIS entry point called to signify completion of a packet send.
    We pick up and complete the Write IRP corresponding to this packet.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNetBufferList - NetBufferList that completed send
    SendCompleteFlags - Specifies if the caller is at DISPATCH level

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT      pOpenContext;
    PNET_BUFFER_LIST            CurrNetBufferList = NULL;
    PNET_BUFFER_LIST            NextNetBufferList;
    NDIS_STATUS                 CompletionStatus;
    BOOLEAN                     DispatchLevel;
    WDFREQUEST                  request;
    PREQUEST_CONTEXT            reqContext;


    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);
    DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendCompleteFlags);


    for (CurrNetBufferList = pNetBufferList;
            CurrNetBufferList != NULL;
            CurrNetBufferList = NextNetBufferList)
    {
        NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrNetBufferList);

        request = NPROT_REQUEST_FROM_SEND_NBL(CurrNetBufferList);
        reqContext = GetRequestContext(request);
        CompletionStatus = NET_BUFFER_LIST_STATUS(CurrNetBufferList);

        DEBUGP(DL_INFO, ("SendComplete: NetBufferList %p/IRP %p/Length %d "
                        "completed with status %x\n",
                        CurrNetBufferList, request, reqContext->Length,
                        CompletionStatus));

		//
        //  We are done with the NDIS_PACKET:
        //
        NPROT_DEREF_SEND_NBL(CurrNetBufferList, DispatchLevel);
        CurrNetBufferList = NULL;

        if (CompletionStatus == NDIS_STATUS_SUCCESS)
        {
            WdfRequestCompleteWithInformation(request, STATUS_SUCCESS, reqContext->Length);
         }
        else
        {
            WdfRequestCompleteWithInformation(request, STATUS_UNSUCCESSFUL, 0);
        }

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, DispatchLevel);
        pOpenContext->PendedSendCount--;

        if ((NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_CLOSING))
                && (pOpenContext->PendedSendCount == 0))
        {
            NPROT_ASSERT(pOpenContext->ClosingEvent != NULL);
            NPROT_SIGNAL_EVENT(pOpenContext->ClosingEvent);
            pOpenContext->ClosingEvent = NULL;
        }
        NPROT_RELEASE_LOCK(&pOpenContext->Lock, DispatchLevel);

        NPROT_DEREF_OPEN(pOpenContext); // send complete - dequeued send IRP
    }

}




