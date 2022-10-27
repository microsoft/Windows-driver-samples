#include "precomp.h"
#include "MbbFastIODataPlane.tmh"

VOID
FastIOSendNetBufferListsComplete(
    _In_ PVOID                    MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST         NetBufferLists,
    _In_ ULONG                    SendCompleteFlags
)
{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    MBB_NBL_TYPE NblType = MBB_NBL_TYPE_MAXIMUM, LastNblType = MBB_NBL_TYPE_MAXIMUM;
    PVOID  NblContext;
    USHORT NblContextSize;
    MBB_REQUEST_HANDLE    RequestHandle = NULL;
    PNET_BUFFER_LIST      NetBufferList = NetBufferLists, NextNetBufferList = NULL;
    PNET_BUFFER           NetBuffer = NULL;

    ASSERT(Adapter != NULL);
    for (NetBufferList = NetBufferList;
        NetBufferList != NULL;
        NetBufferList = NextNetBufferList)
    {
        NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(NetBufferList);
        NblContext = NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
        NblContextSize = ALIGN(sizeof(MBB_NBL_TYPE), MEMORY_ALLOCATION_ALIGNMENT);

        ASSERT(NblContext != NULL);

        NblType = *((MBB_NBL_TYPE*)NblContext);
        NdisFreeNetBufferListContext(NetBufferList, NblContextSize);

        if (LastNblType != MBB_NBL_TYPE_MAXIMUM && LastNblType != NblType)
        {
            TraceError(WMBCLASS_SEND, "[FastIOSend] Doesn't allow mixed NBL data type in the list, OS will crash. [NblType=%!MbbNblType!] [LastNblType=%!MbbNblType!]  [NBL=0x%p]",
                NblType, LastNblType, NetBufferList);
            ASSERT(FALSE);
            return;
        }

        LastNblType = NblType;

        switch (NblType)
        {
        case MBB_NBL_TYPE_IP:
            for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
                NetBuffer != NULL;
                NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                InterlockedAdd64(&Adapter->Stats.ifHCOutOctets, NET_BUFFER_DATA_LENGTH(NetBuffer));
                InterlockedIncrement64(&Adapter->GenXmitFramesOk);
                InterlockedIncrement64(&Adapter->Stats.ifHCOutUcastPkts);
            }
            break;
        case MBB_NBL_TYPE_DSS:
            RequestHandle = (MBB_REQUEST_HANDLE)NET_BUFFER_LIST_MINIPORT_RESERVED(NetBufferList)[0];
            MbbNdisDeviceServiceSessionSendComplete(
                RequestHandle,
                NetBufferList->Status
            );

            // Free the Nbl & stuff associated with the request
            MbbCleanupDssNbl(&Adapter->SendQueue, NetBufferList);
            break;
        default:
            TraceError(WMBCLASS_SEND, "[FastIOSend] Unknown MBB_NBL_TYPE [%!MbbNblType!], OS will crash. [NBL=0x%p]", NblType, NetBufferList);
            ASSERT(FALSE);
            return;
        }
    }

    if (NblType == MBB_NBL_TYPE_IP)
    {
        NdisMSendNetBufferListsComplete(Adapter->MiniportAdapterHandle, NetBufferLists, SendCompleteFlags);
    }
}


VOID
FastIONdisDeviceServiceSessionReceive(
    _In_ PMINIPORT_ADAPTER_CONTEXT          AdapterContext,
    _In_ PNET_BUFFER_LIST                   NetBufferList,
    _In_ ULONG                              SessionId,
    _In_ ULONG                              ReceiveFlags
)
{
    PNET_BUFFER     NetBuffer;
    PMDL            Mdl;
    PCHAR           MdlDataBuffer = NULL;
    ULONG           MdlDataOffset;
    ULONG           MdlDataLength;
    ULONG           datagramLength;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ NdisWwanDssRead = NULL;
    PCHAR                   NdisWwanDssReadBuffer = NULL;
    ULONG                   TotalNdisWwanSize = 0;
    ULONG                   ReturnFlags = (ReceiveFlags & NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL) ? NDIS_RETURN_FLAGS_DISPATCH_LEVEL : 0;

    for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
        NetBuffer != NULL;
        NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
    {

        datagramLength = NET_BUFFER_DATA_LENGTH(NetBuffer);
        TotalNdisWwanSize = sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_READ) + datagramLength;
        // Allocate memory for the output buffer
        if ((NdisWwanDssRead = (PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ)ALLOCATE_NONPAGED_POOL(TotalNdisWwanSize)) == NULL)
        {
            TraceError(WMBCLASS_RECEIVE, "[Receive][DSS Read] FAILED to allocate for PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ %d bytes",
                TotalNdisWwanSize
            );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisWwanDssRead->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWwanDssRead->Header.Size = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SESSION_READ_1;
        NdisWwanDssRead->Header.Revision = NDIS_WWAN_DEVICE_SERVICE_SESSION_READ_REVISION_1;

        NdisWwanDssRead->ReadData.uDataSize = datagramLength;
        NdisWwanDssRead->ReadData.uSessionID = SessionId;

        // Copy the data. We cannot use the original buffer because it doesnt
        // have space for the session Id, etc
        Mdl = NET_BUFFER_CURRENT_MDL(NetBuffer);
        MdlDataOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
        NdisWwanDssReadBuffer = (PCHAR)(NdisWwanDssRead + 1);

        for (;
            datagramLength  > 0;
            datagramLength -= MdlDataLength)
        {
            MdlDataLength = MmGetMdlByteCount(Mdl) - MdlDataOffset;
            if ((MdlDataBuffer = (PCHAR)MmGetSystemAddressForMdlSafe(
                Mdl,
                NormalPagePriority | MdlMappingNoExecute
            )) == NULL)
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            MdlDataBuffer = MdlDataBuffer + MdlDataOffset;
            MdlDataLength = MIN(datagramLength, MdlDataLength);

            RtlCopyMemory(
                NdisWwanDssReadBuffer,
                MdlDataBuffer,
                MdlDataLength
            );

            NdisWwanDssReadBuffer = NdisWwanDssReadBuffer + MdlDataLength;
            Mdl = Mdl->Next;
            MdlDataOffset = 0;
        }

        // Create the status indication
        MBB_UTIL_INITIALIZE_NDIS_STATUS_INDICATION(
            &StatusIndication,
            AdapterContext->MiniportAdapterHandle,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ
        );

        StatusIndication.StatusBuffer = NdisWwanDssRead;
        StatusIndication.StatusBufferSize = TotalNdisWwanSize;

        TraceInfo(WMBCLASS_RECEIVE, "[Receive][DSS Read] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ");

        MbbUtilNdisMiniportIndicateStatusEx(
            AdapterContext,
            &StatusIndication
        );
        if (NdisWwanDssRead)
        {
            FREE_POOL(NdisWwanDssRead);
        }
    }

    NET_BUFFER_LIST_STATUS(NetBufferList) = NdisStatus;
    if ((ReceiveFlags & NDIS_RECEIVE_FLAGS_RESOURCES) == 0)
        MbbBusReturnNetBufferLists(AdapterContext->BusHandle, NetBufferList, ReturnFlags);
}

VOID
FastIOIndicateReceiveNetBufferLists(
    _In_ PVOID                    MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST         NetBufferLists,
    _In_ ULONG                    SessionId,
    _In_ ULONG                    NumberOfNetBufferLists,
    _In_ ULONG                    ReceiveFlags
)
{
    PVOID  NblContext = NULL;
    PMINIPORT_ADAPTER_CONTEXT Adapter;
    SESSIONID_PORTNUMBER_ENTRY  SessionIdPortNumberEntry = { 0 };
    MBB_NBL_TYPE NblType;
    ULONG ReturnFlags = (ReceiveFlags & NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL) ? NDIS_RETURN_FLAGS_DISPATCH_LEVEL : 0;
    PNET_BUFFER_LIST NetBufferList = NetBufferLists, NextNetBufferList = NULL;
    PNET_BUFFER      NetBuffer = NULL;

    if (NetBufferList == NULL)
    {
        return;
    }

    NblContext = NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
    Adapter = (PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;

    ASSERT(NblContext != NULL);
    ASSERT(Adapter != NULL);

    NblType = *((MBB_NBL_TYPE*)NblContext);
    SessionIdPortNumberEntry = Adapter->SessionIdPortTable[SessionId];

    switch (NblType)
    {
    case MBB_NBL_TYPE_IP:
        if (!SessionIdPortNumberEntry.InUse)
        {
            //discard
            TraceError(WMBCLASS_RECEIVE, "[FastIORecv][NBL=0x%p] FAILED to receive NBL as session id is not in use. [Session Id = %lu]",
                NetBufferList,
                SessionId
            );
            NET_BUFFER_LIST_STATUS(NetBufferList) = NDIS_STATUS_INVALID_PARAMETER;
            InterlockedIncrement64(&Adapter->Stats.ifInDiscards);
            MbbBusReturnNetBufferLists(Adapter->BusHandle, NetBufferList, ReturnFlags);
            return;
        }
        for (;
            NetBufferList != NULL;
            NetBufferList = NextNetBufferList)
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(NetBufferList);
            NetBufferList->SourceHandle = Adapter->MiniportAdapterHandle;
            for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
                NetBuffer != NULL;
                NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                InterlockedAdd64(&Adapter->Stats.ifHCInOctets, NET_BUFFER_DATA_LENGTH(NetBuffer));
                InterlockedIncrement64(&Adapter->GenRcvFramesOk);
                InterlockedIncrement64(&Adapter->Stats.ifHCInUcastPkts);
            }
        }
        NdisMIndicateReceiveNetBufferLists(Adapter->MiniportAdapterHandle, NetBufferLists, SessionIdPortNumberEntry.PortNumber, NumberOfNetBufferLists, ReceiveFlags);
        break;
    case MBB_NBL_TYPE_DSS:

        for (;
            NetBufferList != NULL;
            NetBufferList = NextNetBufferList)
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(NetBufferList);
            NET_BUFFER_LIST_NEXT_NBL(NetBufferList) = NULL;
            FastIONdisDeviceServiceSessionReceive(Adapter, NetBufferList, SessionId, ReceiveFlags);
        }
        break;
    default:
        TraceError(WMBCLASS_RECEIVE, "[FastIOReceive] Unknown MBB_NBL_TYPE [%!MbbNblType!], OS will crash. [NBL=0x%p]", NblType, NetBufferLists);
        ASSERT(FALSE);
        break;
    }
}
