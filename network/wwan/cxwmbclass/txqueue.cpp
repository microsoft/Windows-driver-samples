//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "device.h"
#include "txqueue.h"

void MbbNotifyTxReady(_In_ NETPACKETQUEUE TxQueue)
{
    PMBB_TXQUEUE_CONTEXT txQueueContext = MbbGetTxQueueContext(TxQueue);

    if (InterlockedExchange(&txQueueContext->NotificationEnabled, FALSE) == TRUE)
    {
        NetTxQueueNotifyMoreCompletedPacketsAvailable(TxQueue);
    }
}

ULONG
MbbGetNetPacketDataLength(_In_ NET_PACKET* NetPacket, _In_ NET_RING_COLLECTION const* Rings)
{
    ULONG length = 0;
    auto fr = NetRingCollectionGetFragmentRing(Rings);
    for (UINT32 i = 0; i < NetPacket->FragmentCount; i++)
    {
        auto fragment = NetRingGetFragmentAtIndex(fr, (NetPacket->FragmentIndex + i) & fr->ElementIndexMask);
        length += static_cast<ULONG>(fragment->ValidLength);
    }

    return length;
}

VOID MbbPacketRestoreMdl(_In_ PMBB_PACKET_CONTEXT PacketContext)
{
    if (PacketContext->ModifiedMdl != NULL)
    {
        *PacketContext->ModifiedMdl = PacketContext->OriginalMdl;
        PacketContext->ModifiedMdl = NULL;
    }
}

VOID MbbPacketCleanupContext(_In_ PMBB_PACKET_CONTEXT PacketContext)
{
    MbbPacketRestoreMdl(PacketContext);
    if (PacketContext->PaddingMdl != NULL)
    {
        IoFreeMdl(PacketContext->PaddingMdl);
        PacketContext->PaddingMdl = NULL;
    }
    if (PacketContext->DataEndMdl != NULL)
    {
        IoFreeMdl(PacketContext->DataEndMdl);
        PacketContext->DataEndMdl = NULL;
    }
    if (PacketContext->DataStartMdl != NULL)
    {
        IoFreeMdl(PacketContext->DataStartMdl);
        PacketContext->DataStartMdl = NULL;
    }
}

FORCEINLINE
PMDL MbbPacketGetFirstMdl(_In_ PMBB_PACKET_CONTEXT PacketContext)
{
    if (PacketContext->PaddingMdl != NULL)
        return PacketContext->PaddingMdl;
    else
        return PacketContext->DataStartMdl;
}

FORCEINLINE
PMDL MbbPacketGetLastMdl(_In_ PMBB_PACKET_CONTEXT PacketContext)
{
    if (PacketContext->DataEndMdl != NULL)
        return PacketContext->DataEndMdl;
    else
        return PacketContext->DataStartMdl;
}

VOID MbbPacketSaveAndSetMdl(_In_ PMBB_PACKET_CONTEXT PacketContext, _In_ PMDL MdlToSave, _In_ PMDL MdlToSet)
{
    if (PacketContext->ModifiedMdl == NULL)
    {
        PacketContext->ModifiedMdl = MdlToSave;
        PacketContext->OriginalMdl = *MdlToSave;
    }
    *MdlToSave = *MdlToSet;
}

VOID MbbCleanupDssPacket(_In_ PDSS_PACKET Packet)
{
    if (Packet->Mdl != NULL)
    {
        IoFreeMdl(Packet->Mdl);
    }
    if (Packet != NULL)
    {
        FREE_POOL(Packet);
    }
}

VOID MbbNtbCleanupContext(_In_ PMBB_NTB_BUILD_CONTEXT NtbContext, _In_ NTSTATUS NtStatus)
{
    ULONG DatagramIndex;
    BOOLEAN NeedReturnCompletedPackets = FALSE;

    for (DatagramIndex = 0; DatagramIndex < NtbContext->DatagramCount; DatagramIndex++)
    {
        MbbPacketCleanupContext(&NtbContext->NdpDatagramEntries[DatagramIndex].NetPacketContext);
        if (NtbContext->NdpDatagramEntries[DatagramIndex].NdpType == MbbNdpTypeIps)
        {
            NtbContext->NdpDatagramEntries[DatagramIndex].NetPacket->Scratch = 1;
            NeedReturnCompletedPackets = TRUE;
        }
        else if (NtbContext->NdpDatagramEntries[DatagramIndex].NdpType == MbbNdpTypeVendor_1)
        {
            MbbDeviceSendDeviceServiceSessionDataComplete(NtbContext->NdpDatagramEntries[DatagramIndex].DssPacket->Data, NtStatus);

            MbbCleanupDssPacket(NtbContext->NdpDatagramEntries[DatagramIndex].DssPacket);
        }
    }
    if (NtbContext->NdpMdl != NULL)
    {
        IoFreeMdl(NtbContext->NdpMdl);
    }
    if (NtbContext->NthMdl != NULL)
    {
        IoFreeMdl(NtbContext->NthMdl);
    }
    if (NtbContext->NdpBufferMemory != NULL)
    {
        WdfObjectDelete(NtbContext->NdpBufferMemory);
    }
#if DBG
    if (NtbContext->ScratchBuffer != NULL)
    {
        FREE_POOL(NtbContext->ScratchBuffer);
    }
#endif
    if (NeedReturnCompletedPackets)
    {
        MbbNotifyTxReady(NtbContext->NetTxQueue);
    }
    WdfObjectDelete(NtbContext->NtbLookasideBufferMemory);
}

FORCEINLINE
ULONG
MbbSendQGetNtbSequence(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext)
{
    return InterlockedIncrement(&DeviceContext->NtbSequenceNumber);
}

PMBB_NTB_BUILD_CONTEXT
MbbNtbAllocateContext(_In_ WDFLOOKASIDE NtbLookasideList, _In_ PMBB_BUS_PARAMETERS BusParams, _In_ PVOID PaddingBuffer, _In_ ULONG NtbSequence)
{
    NTSTATUS status = NDIS_STATUS_SUCCESS;
    size_t ntbSize;
    PMBB_NTB_BUILD_CONTEXT ntbContext = NULL;
    WDFMEMORY ntbContextMemory;

    do
    {
#pragma prefast(suppress \
                : __WARNING_MEMORY_LEAK, "By Design: Allocate ntb context from lookaside pool, released when send completes.")
        if (!NT_SUCCESS(status = WdfMemoryCreateFromLookaside(NtbLookasideList, &ntbContextMemory)))
        {
            break;
        }

        ntbContext = (PMBB_NTB_BUILD_CONTEXT)WdfMemoryGetBuffer(ntbContextMemory, &ntbSize);
        RtlZeroMemory(ntbContext, ntbSize);
        ntbContext->PaddingBuffer = PaddingBuffer;
        ntbContext->NtbLookasideList = NtbLookasideList;
        ntbContext->NtbLookasideBufferMemory = ntbContextMemory;

#if DBG
        ntbContext->ScratchLength = BusParams->MaxOutNtb;
        if ((ntbContext->ScratchBuffer = (PCHAR)ALLOCATE_NONPAGED_POOL(ntbContext->ScratchLength)) == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
#endif
        //
        // Initialize the NTH and the NTH MDL.
        //
        if ((ntbContext->IsNtb32Bit = BusParams->CurrentMode32Bit) == TRUE)
        {
            if ((ntbContext->NthMdl = AllocateNonPagedMdl(&ntbContext->Nth32, sizeof(ntbContext->Nth32))) == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            ntbContext->Nth32.dwSignature = NCM_NTH32_SIG;
            ntbContext->Nth32.wHeaderLength = sizeof(NCM_NTH32);
            ntbContext->Nth32.wSequence = (USHORT)NtbSequence;

            ntbContext->NtbHeaderSize = sizeof(NCM_NTH32);
            ntbContext->NdpHeaderFixedSize = sizeof(NCM_NDP32);
            ntbContext->NdpDatagramEntrySize = sizeof(NCM_NDP32_DATAGRAM);
        }
        else
        {
            if ((ntbContext->NthMdl = AllocateNonPagedMdl(&ntbContext->Nth16, sizeof(ntbContext->Nth16))) == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            ntbContext->Nth16.dwSignature = NCM_NTH16_SIG;
            ntbContext->Nth16.wHeaderLength = sizeof(NCM_NTH16);
            ntbContext->Nth16.wSequence = (USHORT)NtbSequence;

            ntbContext->NtbHeaderSize = sizeof(NCM_NTH16);
            ntbContext->NdpHeaderFixedSize = sizeof(NCM_NDP16);
            ntbContext->NdpDatagramEntrySize = sizeof(NCM_NDP16_DATAGRAM);
        }

        ntbContext->NtbOutMaxSize = BusParams->MaxOutNtb;
        ntbContext->NtbOutMaxDatagrams = BusParams->MaxOutDatagrams;
        ntbContext->NdpOutDivisor = BusParams->NdpOutDivisor;
        ntbContext->NdpOutPayloadRemainder = BusParams->NdpOutRemainder;
        ntbContext->NdpOutAlignment = BusParams->NdpOutAlignment;
    } while (FALSE);

    if (!NT_SUCCESS(status))
    {
        if (ntbContext != NULL)
        {
            MbbNtbCleanupContext(ntbContext, status);
        }
        ntbContext = NULL;
    }
    return ntbContext;
}

NTSTATUS
MbbFillPacketContext(
    _In_ PMBB_PACKET_CONTEXT PacketContext,
    _In_ PMDL PacketDataStartMdl,
    _In_ ULONG PacketDataStartMdlDataOffset,
    _In_ ULONG DatagramLength,
    _In_ PVOID PaddingBuffer,
    _In_ ULONG PaddingLength)
{
    ULONG packetDataStartMdlDataLength;
    PCHAR packetDataStartBuffer;
    PMDL packetDataEndMdl;
    ULONG packetDataEndMdlDataLength;
    PCHAR packetDataEndBuffer;
    PMDL packetMdl;
    ULONG packetMdlOffset;
    ULONG packetMdlLength;
    PMDL packetPenultimateMdl;
    NTSTATUS status = STATUS_SUCCESS;

    do
    {
        if ((packetDataStartBuffer = (PCHAR)MmGetSystemAddressForMdlSafe(PacketDataStartMdl, NormalPagePriority | MdlMappingNoExecute)) == NULL)
        {
            status = STATUS_RESOURCE_DATA_NOT_FOUND;
            break;
        }
        packetDataStartBuffer += PacketDataStartMdlDataOffset;
        //
        // Create new DataStart and DataEnd Mdls
        // to remove the unused data space.
        //
        packetDataStartMdlDataLength = MmGetMdlByteCount(PacketDataStartMdl);
        packetDataStartMdlDataLength -= PacketDataStartMdlDataOffset;

        if ((PacketContext->DataStartMdl =
                 AllocateNonPagedMdl(packetDataStartBuffer, (ULONG)(MIN(DatagramLength, packetDataStartMdlDataLength)))) == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        PacketContext->DataStartMdl->Next = PacketDataStartMdl->Next;
        //
        // Find the end MDL and the amount of data in the end MDL
        //
        packetMdl = PacketDataStartMdl;
        packetMdlOffset = PacketDataStartMdlDataOffset;
        packetPenultimateMdl = NULL;

        for (packetDataEndMdlDataLength = (ULONG)DatagramLength; packetDataEndMdlDataLength > (MmGetMdlByteCount(packetMdl) - packetMdlOffset);
             packetDataEndMdlDataLength -= packetMdlLength)
        {
            packetPenultimateMdl = packetMdl;
            packetMdlLength = MmGetMdlByteCount(packetMdl) - packetMdlOffset;
            packetMdlOffset = 0;
            packetMdl = packetMdl->Next;
        }
        packetDataEndMdl = packetMdl;
        //
        // If the starting and ending MDLs are not the same
        // then build another partial MDL removing any unused
        // data space.
        //
        if (packetDataEndMdl != PacketDataStartMdl)
        {
            if ((packetDataEndBuffer = (PCHAR)MmGetSystemAddressForMdlSafe(packetDataEndMdl, NormalPagePriority | MdlMappingNoExecute)) == NULL)
            {
                status = STATUS_RESOURCE_DATA_NOT_FOUND;
                break;
            }

            if ((PacketContext->DataEndMdl = AllocateNonPagedMdl(packetDataEndBuffer, packetDataEndMdlDataLength)) == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            PacketContext->DataEndMdl->Next = NULL;

            if (packetPenultimateMdl != PacketDataStartMdl)
            {
                MDL TempMdl = *packetPenultimateMdl;
                TempMdl.Next = PacketContext->DataEndMdl;

                MbbPacketSaveAndSetMdl(PacketContext, packetPenultimateMdl, &TempMdl);
            }

            if (PacketContext->DataStartMdl->Next == packetDataEndMdl)
            {
                PacketContext->DataStartMdl->Next = PacketContext->DataEndMdl;
            }
        }
        //
        // Allocate padding, if needed. The padding buffer is a share buffer.
        // Every padding MDL points to this same buffer. The buffer contains
        // all 0s.
        //
        if (PaddingLength != 0)
        {
            if ((PacketContext->PaddingMdl = AllocateNonPagedMdl(PaddingBuffer, PaddingLength)) == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            PacketContext->PaddingMdl->Next = PacketContext->DataStartMdl;
        }
    } while (FALSE);

    return status;
}

VOID MbbNtbChainNb(_In_ PMBB_NTB_BUILD_CONTEXT NtbContext, _In_ PMBB_PACKET_CONTEXT PacketContext)
{
    if (NtbContext->DatagramLastMdl != NULL)
    {
        NtbContext->DatagramLastMdl->Next = MbbPacketGetFirstMdl(PacketContext);
    }
    else
    {
        NtbContext->NthMdl->Next = MbbPacketGetFirstMdl(PacketContext);
    }

    NtbContext->DatagramLastMdl = MbbPacketGetLastMdl(PacketContext);
}

NTSTATUS
MbbNtbAddPacket(
    _In_ PMBB_NTB_BUILD_CONTEXT NtbContext,
    _In_ PVOID PacketContext,
    _In_ ULONG DatagramLength,
    _In_ PMDL PacketDataStartMdl,
    _In_ ULONG PacketDataStartMdlDataOffset,
    _In_ MBB_NDP_TYPE CurrentNdpType,
    _In_ ULONG SessionId)
{
    ULONG datagramOffset;
    ULONG paddingLength;
    ULONG ndpSize;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG totalLength;
    PMBB_PACKET_CONTEXT packetContext = NULL;

    do
    {
        if ((NtbContext->DatagramCount + 1) > NtbContext->NtbOutMaxDatagrams)
        {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        //
        // Size of passed in NET_BUFFER, to be updated later in the NDP Context.
        //
        totalLength = NtbContext->NtbHeaderSize + NtbContext->DatagramLength;
        paddingLength = ALIGN_AT_OFFSET(totalLength, NtbContext->NdpOutDivisor, NtbContext->NdpOutPayloadRemainder) - totalLength;
        //
        // Calculate the new NTB size based on the passed in MBB_PACKET
        //

        //
        // Fixed size NTH & DatagramSize along with Padding for all NDPs
        //
        datagramOffset = totalLength + paddingLength;
        totalLength += DatagramLength + paddingLength;

        //
        // Calculate NDP HeaderSize for all NDPs
        //
        ndpSize = totalLength;
        totalLength = ALIGN(totalLength, NtbContext->NdpOutAlignment);
        totalLength += NtbContext->NdpHeaderFixedSize;
        totalLength += ((NtbContext->DatagramCount + 1) * NtbContext->NdpDatagramEntrySize);
        ndpSize = totalLength - ndpSize;
        //
        // Can everything fit?
        //
        if (totalLength > NtbContext->NtbOutMaxSize)
        {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }
        packetContext = &NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NetPacketContext;
        if (!NT_SUCCESS(
                status = MbbFillPacketContext(
                    packetContext, PacketDataStartMdl, PacketDataStartMdlDataOffset, DatagramLength, NtbContext->PaddingBuffer, paddingLength)))
        {
            break;
        }
        //
        // Update the NTB Context for the new NET_BUFFER.
        //
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].DatagramOffset = datagramOffset;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].DatagramLength = DatagramLength;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NdpType = CurrentNdpType;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].SessionId = SessionId;
        if (CurrentNdpType == MbbNdpTypeIps)
        {
            NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NetPacket = (NET_PACKET*)PacketContext;
        }
        else if (CurrentNdpType == MbbNdpTypeVendor_1)
        {
            NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].DssPacket = (PDSS_PACKET)PacketContext;
        }

        NtbContext->NdpSize = ndpSize;
        NtbContext->DatagramCount += 1;
        NtbContext->DatagramLength += (DatagramLength + paddingLength);

        MbbNtbChainNb(NtbContext, packetContext);
    } while (FALSE);

    if (!NT_SUCCESS(status))
    {
        if (packetContext != NULL)
        {
            MbbPacketCleanupContext(packetContext);
        }
    }
    return status;
}

FORCEINLINE
PMDL MbbNtbGetMdlChainHead(_In_ PMBB_NTB_BUILD_CONTEXT NtbContext)
{
    return NtbContext->NthMdl;
}

VOID MbbSendQCompleteNtb(_In_ MBB_PROTOCOL_HANDLE ProtocolHandle, _In_ MBB_REQUEST_HANDLE RequestHandle, _In_ NTSTATUS NtStatus, _In_ PMDL Mdl)
{
    UNREFERENCED_PARAMETER(ProtocolHandle);
    PMBB_NTB_BUILD_CONTEXT ntbContext = (PMBB_NTB_BUILD_CONTEXT)RequestHandle;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetNetAdapterContext(ntbContext->NetAdapter)->WmbDeviceContext;
    UNREFERENCED_PARAMETER(Mdl);

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus == STATUS_NDIS_ADAPTER_NOT_READY)
        {
        }
        else if (NtStatus == STATUS_CANCELLED)
        {
        }
        else if (NtStatus == STATUS_NO_SUCH_DEVICE)
        {
        }
        else
        {
            MbbBusResetDataPipes(deviceContext->BusHandle);
        }
    }

    MbbNtbCleanupContext(ntbContext, NtStatus);
}

ULONG
MbbNtbMapNdpTypeToSignature(_In_ MBB_NDP_TYPE MbbNdpType, _In_ BOOLEAN Is32Bit, _In_ ULONG SessionId)
{
    ULONG SessionMask = (SessionId << NCM_NDP_SESSION_SHIFT);

    switch (MbbNdpType)
    {
    case MbbNdpTypeIps: return ((Is32Bit == TRUE) ? NCM_NDP32_IPS | SessionMask : NCM_NDP16_IPS | SessionMask);
    default:
        if ((MbbNdpType >= MbbNdpTypeVendor_1) && (MbbNdpType <= MbbNdpTypeVendor_Max))
        {
            return ((Is32Bit == TRUE) ? NCM_NDP32_VENDOR | SessionMask : NCM_NDP16_VENDOR | SessionMask);
        }
    }
    return 0;
}

VOID MbbNtbFillNdp32Header(_In_ PNCM_NDP32 Ndp, _In_ MBB_NDP_TYPE NdpType, _In_ PMBB_NTB_BUILD_CONTEXT NtbContext)
{
    ULONG datagramIndex = 0;
    ULONG ndpDatagramIndex = 0;
    PNCM_NDP32_DATAGRAM ndpDatagramEntries;

    Ndp->dwSignature = MbbNtbMapNdpTypeToSignature(NdpType, TRUE, NtbContext->NdpDatagramEntries[datagramIndex].SessionId);
    Ndp->dwNextFpIndex = 0;
    ndpDatagramEntries = Ndp->Datagram;
    //
    // Add datagram entries to the NDP Table
    //

    for (datagramIndex = 0; datagramIndex < NtbContext->DatagramCount; datagramIndex++)
    {
        ndpDatagramEntries[ndpDatagramIndex].dwDatagramIndex = NtbContext->NdpDatagramEntries[datagramIndex].DatagramOffset;
        ndpDatagramEntries[ndpDatagramIndex].dwDatagramLength = NtbContext->NdpDatagramEntries[datagramIndex].DatagramLength;
        ndpDatagramIndex++;
    }
    //
    // Terminating entry is taken in to account
    // in the fixed size NDP Header.
    //
    ndpDatagramEntries[ndpDatagramIndex].dwDatagramIndex = 0;
    ndpDatagramEntries[ndpDatagramIndex].dwDatagramLength = 0;

    Ndp->wLength = (USHORT)(NtbContext->NdpHeaderFixedSize + (ndpDatagramIndex * NtbContext->NdpDatagramEntrySize));
}

VOID MbbNtbFillNdp16Header(__in PNCM_NDP16 Ndp, __in MBB_NDP_TYPE NdpType, __in PMBB_NTB_BUILD_CONTEXT NtbContext)
{
    ULONG datagramIndex = 0;
    ULONG ndpDatagramIndex = 0;
    PNCM_NDP16_DATAGRAM ndpDatagramEntries;

    Ndp->dwSignature = MbbNtbMapNdpTypeToSignature(NdpType, FALSE, NtbContext->NdpDatagramEntries[datagramIndex].SessionId);

    Ndp->wNextFpIndex = 0;
    ndpDatagramEntries = Ndp->Datagram;
    //
    // Add datagram entries to the NDP Table
    //
    for (datagramIndex = 0; datagramIndex < NtbContext->DatagramCount; datagramIndex++)
    {
        ndpDatagramEntries[ndpDatagramIndex].wDatagramIndex = (USHORT)NtbContext->NdpDatagramEntries[datagramIndex].DatagramOffset;
        ndpDatagramEntries[ndpDatagramIndex].wDatagramLength = (USHORT)NtbContext->NdpDatagramEntries[datagramIndex].DatagramLength;
        ndpDatagramIndex++;
    }
    //
    // Terminating entry is taken in to account
    // in the fixed size NDP Header.
    //
    ndpDatagramEntries[ndpDatagramIndex].wDatagramIndex = 0;
    ndpDatagramEntries[ndpDatagramIndex].wDatagramLength = 0;

    Ndp->wLength = (USHORT)(NtbContext->NdpHeaderFixedSize + (ndpDatagramIndex * NtbContext->NdpDatagramEntrySize));
}

NTSTATUS
MbbNtbAddNdpHeaders(_In_ PMBB_NTB_BUILD_CONTEXT NtbContext)
{
    // Offset from the start of the NTB buffer to the start of NDP headers
    ULONG ndpStartOffset;
    // Offset from the start of the NTB Buffer to the current position.
    ULONG ntbOffset;
    PCHAR ndpBuffer;
    MBB_NDP_TYPE ndpType;
    PNCM_NDP16 ndp16;
    PNCM_NDP32 ndp32;
    NTSTATUS status = STATUS_SUCCESS;

    do
    {
        //
        // Allocate buffer for all NDP headers.
        // This includes padding for NDP Header alignment.
        //
        status = CreateNonPagedWdfMemory(
            NtbContext->NdpSize,
            &NtbContext->NdpBufferMemory,
            &NtbContext->NdpBuffer,
            NtbContext->NetTxQueue == NULL ? (WDFOBJECT)NtbContext->NetAdapter : NtbContext->NetTxQueue,
            MbbPoolTagNtbSend);
        if (!NT_SUCCESS(status))
        {
            break;
        }
        RtlZeroMemory(NtbContext->NdpBuffer, NtbContext->NdpSize);
        ndpBuffer = (PCHAR)(NtbContext->NdpBuffer);
        //
        // Chain the NDP Header through its MDL to datagram MDL
        //
        if ((NtbContext->NdpMdl = AllocateNonPagedMdl(NtbContext->NdpBuffer, NtbContext->NdpSize)) == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        NtbContext->DatagramLastMdl->Next = NtbContext->NdpMdl;
        ndpStartOffset = NtbContext->NtbHeaderSize + NtbContext->DatagramLength;

        if (NtbContext->IsNtb32Bit)
            NtbContext->Nth32.dwFpIndex = ALIGN(ndpStartOffset, NtbContext->NdpOutAlignment);
        else
            NtbContext->Nth16.wFpIndex = (USHORT)ALIGN(ndpStartOffset, NtbContext->NdpOutAlignment);

        ntbOffset = ALIGN(ndpStartOffset, NtbContext->NdpOutAlignment);
        ndpType = NtbContext->NdpDatagramEntries[0].NdpType;

        if (NtbContext->IsNtb32Bit == TRUE)
        {
            ndp32 = (PNCM_NDP32)(ndpBuffer + (ntbOffset - ndpStartOffset));

            MbbNtbFillNdp32Header(ndp32, ndpType, NtbContext);
        }
        else
        {
            ndp16 = (PNCM_NDP16)(ndpBuffer + (ntbOffset - ndpStartOffset));

            MbbNtbFillNdp16Header(ndp16, ndpType, NtbContext);
        }

        if (NtbContext->IsNtb32Bit == TRUE)
        {
            NtbContext->Nth32.dwBlockLength = NtbContext->NtbHeaderSize;
            NtbContext->Nth32.dwBlockLength += NtbContext->DatagramLength;
            NtbContext->Nth32.dwBlockLength += NtbContext->NdpSize;
        }
        else
        {
            NtbContext->Nth16.wBlockLength = (USHORT)(NtbContext->NtbHeaderSize);
            NtbContext->Nth16.wBlockLength += (USHORT)(NtbContext->DatagramLength);
            NtbContext->Nth16.wBlockLength += (USHORT)(NtbContext->NdpSize);
        }
    } while (FALSE);
    //
    // No cleanup. Cleanup done by caller.
    //
    return status;
}

NTSTATUS
MbbTestValidateNtb(_In_ PMBB_NTB_BUILD_CONTEXT NtbContext, _In_reads_bytes_(ScratchLength) PCHAR ScratchBuffer, _In_ ULONG ScratchLength)
{
    PMDL currentMdl;
    ULONGLONG ntbLength;
    ULONG mdlLength;
    PVOID mdlVa;
    PVOID nth;

    nth = ScratchBuffer;
    ntbLength = 0;

    for (currentMdl = MbbNtbGetMdlChainHead(NtbContext); currentMdl != NULL; currentMdl = currentMdl->Next)
    {
        mdlLength = MmGetMdlByteCount(currentMdl);

        if ((mdlVa = MmGetSystemAddressForMdlSafe(currentMdl, NormalPagePriority | MdlMappingNoExecute)) == NULL)
        {
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        }

        if ((ntbLength + mdlLength) > ScratchLength)
        {
            return STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory(ScratchBuffer, mdlVa, mdlLength);

        ScratchBuffer += mdlLength;
        ntbLength += mdlLength;
    }

    return MbbNtbValidate(nth, (ULONG)ntbLength, NtbContext->IsNtb32Bit, NULL);
}

void EvtTxQueueDestroy(_In_ WDFOBJECT TxQueue)
{
    PMBB_TXQUEUE_CONTEXT txQueueContext = MbbGetTxQueueContext(TxQueue);

    txQueueContext->NetAdapterContext->TxQueue = NULL;
}

VOID EvtTxQueueSetNotificationEnabled(_In_ NETPACKETQUEUE TxQueue, _In_ BOOLEAN NotificationEnabled)
{
    PMBB_TXQUEUE_CONTEXT txQueueContext = MbbGetTxQueueContext(TxQueue);

    InterlockedExchange(&txQueueContext->NotificationEnabled, NotificationEnabled);
}

void EvtTxQueueCancel(_In_ NETPACKETQUEUE TxQueue)
{
    NET_RING_COLLECTION const* rings = MbbGetTxQueueContext(TxQueue)->DatapathDescriptor;

    NET_RING * pr = NetRingCollectionGetPacketRing(rings);
    while (pr->BeginIndex != pr->EndIndex)
    {
        UINT32 const packetIndex = pr->BeginIndex;
        NetRingGetPacketAtIndex(pr, packetIndex)->Scratch = 1;
        pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
    }
}

bool MbbEnableTxBatching(_In_ NET_RING_COLLECTION const* Rings)
{
    NET_RING * ring = Rings->Rings[NetRingTypeFragment];
    return ((ring->EndIndex - ring->BeginIndex) & ring->ElementIndexMask) >  (NetRingCollectionGetPacketRing(Rings)->NumberOfElements / 2);
}

inline
VOID
CompleteTxPacketsBatch(
    _In_ NET_RING_COLLECTION const * Rings,
    _In_ UINT32 BatchSize
)
{
    UINT32 packetCount = 0;

    NET_RING * pr = NetRingCollectionGetPacketRing(Rings);

    while (pr->BeginIndex != pr->EndIndex)
    {
        UINT32 const packetIndex = pr->BeginIndex;
        auto packet = NetRingGetPacketAtIndex(pr, packetIndex);

        // this function uses Scratch field as the bit for testing completion
        if (!packet->Scratch)
        {
            break;
        }

        packetCount++;

        NET_RING * fr = NetRingCollectionGetFragmentRing(Rings);
        fr->BeginIndex = fr->EndIndex;
        pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);

        if (packetCount >= BatchSize)
        {
            Rings->Rings[NetRingTypeFragment]->BeginIndex = fr->BeginIndex;
        }
    }
}

void EvtTxQueueAdvance(_In_ NETPACKETQUEUE TxQueue)
{
    NTSTATUS status = STATUS_SUCCESS;
    PMBB_TXQUEUE_CONTEXT txQueueContext = MbbGetTxQueueContext(TxQueue);
    NET_RING_COLLECTION const* rings = txQueueContext->DatapathDescriptor;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = txQueueContext->NetAdapterContext;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = netAdapterContext->WmbDeviceContext;
    PMBB_NTB_BUILD_CONTEXT ntbContext = NULL;
    ULONG sessionId = netAdapterContext->SessionId;
    ULONG batchSize = MbbEnableTxBatching(rings) ? txQueueContext->CompletionBatchSize : 1;

    NET_RING * pr = NetRingCollectionGetPacketRing(rings);
     while (pr->BeginIndex != pr->EndIndex)
    {
        UINT32 packetIndex = pr->BeginIndex;
        auto packet = NetRingGetPacketAtIndex(pr, packetIndex);
        if (packet->Ignore)
        {
            packet->Scratch = 1;
            pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
            continue;
        }

        ntbContext = MbbNtbAllocateContext(
            netAdapterContext->NtbLookasideList, &deviceContext->BusParams, deviceContext->sharedPaddingBuffer, MbbSendQGetNtbSequence(deviceContext));

        if (ntbContext == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ntbContext->NetTxQueue = TxQueue;
        ntbContext->NetAdapter = netAdapterContext->NetAdapter;
        ntbContext->NetDatapathDescriptor = rings;

        auto fr = NetRingCollectionGetFragmentRing(rings);
        while (NT_SUCCESS(
            status = MbbNtbAddPacket(
                ntbContext,
                packet,
                MbbGetNetPacketDataLength(packet, rings),
                NetExtensionGetFragmentMdl(&txQueueContext->MdlExtension, packet->FragmentIndex)->Mdl,
                NetRingGetFragmentAtIndex(fr, packet->FragmentIndex)->Offset,
                MbbNdpTypeIps,
                sessionId)))
        {
            pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);

            // the ring buffer has no more net packets to send,
            // so packing is done. break now to make USB request
            if (pr->BeginIndex == pr->EndIndex)
            {
                break;
            }

            packetIndex = pr->BeginIndex;
            packet = NetRingGetPacketAtIndex(pr, packetIndex);
        }

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // If the NTB was empty and this packet couldnt be added
            // then ignore this packet to prevent retrying forever.
            // Or send this NTB to bus then start a new NTB
            //
            if (ntbContext->DatagramCount == 0)
            {
                packet->Scratch = 1;
                pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
            }
        }
        else if (status != STATUS_SUCCESS)
        {
            //
            // MbbNtbAddPacket failed, we should ignore this packet, or the loop may never complete
            //
            packet->Scratch = 1;
            pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
        }

        if (ntbContext->DatagramCount > 0)
        {
            status = MbbNtbAddNdpHeaders(ntbContext);
            if (!NT_SUCCESS(status))
            {
            }
            else
            {
#if DBG
                if (!NT_SUCCESS(MbbTestValidateNtb(ntbContext, ntbContext->ScratchBuffer, ntbContext->ScratchLength)))
                {
                    ASSERT(FALSE);
                }
#endif
                //
                // Send the data. On failure, cleanup.
                //
                status = MbbBusWriteData(deviceContext->BusHandle, ntbContext, MbbNtbGetMdlChainHead(ntbContext), MbbSendQCompleteNtb);
                if (!NT_SUCCESS(status))
                {
                }
            }
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        if (!NT_SUCCESS(status))
        {
            MbbSendQCompleteNtb(netAdapterContext, ntbContext, status, MbbNtbGetMdlChainHead(ntbContext));
        }
    }

    CompleteTxPacketsBatch(rings, batchSize);
}

PDSS_PACKET MbbAllocateDssPacket(_In_ WDFMEMORY Data, _In_ PVOID DataBuffer, _In_ size_t DataSize)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDSS_PACKET packet = NULL;
    do
    {
// By Design: Allocate packet from NtbSend Pool, released in function MbbNtbCleanupContext called from MbbSendQCompleteNtb.
#pragma prefast(suppress : __WARNING_MEMORY_LEAK, "Released in function MbbNtbCleanupContext")
        packet = (PDSS_PACKET)ALLOCATE_NONPAGED_POOL_WITH_TAG(sizeof(DSS_PACKET), MbbPoolTagNtbSend);
        if (packet == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        RtlZeroMemory(packet, sizeof(DSS_PACKET));

        packet->Data = Data;
        packet->Mdl = AllocateNonPagedMdl(DataBuffer, (ULONG)DataSize);

        if (packet->Mdl == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    } while (FALSE);

    if (!NT_SUCCESS(status))
    {
        if (packet != NULL)
        {
            MbbCleanupDssPacket(packet);
            packet = NULL;
        }
    }

    return packet;
}

_Use_decl_annotations_ VOID EvtMbbDeviceSendDeviceServiceSessionData(WDFDEVICE Device, DSS_SESSION_ID SessionId, WDFMEMORY Data)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);
    NTSTATUS status = STATUS_SUCCESS;

    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = deviceContext->Sessions[MBB_DEFAULT_SESSION_ID].NetAdapterContext;

    PMBB_NTB_BUILD_CONTEXT ntbContext = NULL;
    PDSS_PACKET packet = NULL;
    BOOLEAN completeNow = TRUE;

    do
    {
        ntbContext = MbbNtbAllocateContext(
            netAdapterContext->NtbLookasideList, &deviceContext->BusParams, deviceContext->sharedPaddingBuffer, MbbSendQGetNtbSequence(deviceContext));

        if (ntbContext == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ntbContext->NetTxQueue = NULL;
        ntbContext->NetAdapter = netAdapterContext->NetAdapter;
        size_t bufferSize = 0;
        PVOID buffer = WdfMemoryGetBuffer(Data, &bufferSize);

        packet = MbbAllocateDssPacket(Data, buffer, bufferSize);
        if (packet == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = MbbNtbAddPacket(ntbContext, packet, (ULONG)bufferSize, packet->Mdl, 0, MbbNdpTypeVendor_1, SessionId);
        if (!NT_SUCCESS(status))
        {
            MbbCleanupDssPacket(packet);
            break;
        }

        completeNow = FALSE;

        status = MbbNtbAddNdpHeaders(ntbContext);
        if (!NT_SUCCESS(status))
        {
            break;
        }

#if DBG
        if (!NT_SUCCESS(MbbTestValidateNtb(ntbContext, ntbContext->ScratchBuffer, ntbContext->ScratchLength)))
        {
            ASSERT(FALSE);
        }
#endif

        //
        // Send the data. On failure, cleanup. It will return STATUS_PENDING when success
        //
        status = MbbBusWriteData(deviceContext->BusHandle, ntbContext, MbbNtbGetMdlChainHead(ntbContext), MbbSendQCompleteNtb);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        deviceContext->DSSPacketsSentCount++;
    } while (FALSE);

    if (!NT_SUCCESS(status))
    {
        if (ntbContext != NULL)
        {
            MbbSendQCompleteNtb(deviceContext, ntbContext, status, MbbNtbGetMdlChainHead(ntbContext));
        }
        if (completeNow)
        {
            MbbDeviceSendDeviceServiceSessionDataComplete(Data, status);
        }
    }
}
