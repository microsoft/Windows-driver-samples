//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "device.h"
#include "rxqueue.h"

#define MBB_NTH_GET_FIRST_NDP_OFFSET(IS32BIT, NTH) \
    ((IS32BIT) ? MBB_NTH32_GET_FIRST_NDP_OFFSET((PNCM_NTH32)NTH) : MBB_NTH16_GET_FIRST_NDP_OFFSET((PNCM_NTH16)NTH))

void MbbNotifyRxReady(_In_ NETPACKETQUEUE RxQueue)
{
    PMBB_RXQUEUE_CONTEXT rxQueueContext = MbbGetRxQueueContext(RxQueue);
    if (InterlockedExchange(&rxQueueContext->NotificationEnabled, FALSE) == TRUE)
    {
        NetRxQueueNotifyMoreReceivedPacketsAvailable(RxQueue);
    }
}

VOID MbbRecvCleanup(_In_ PMBB_NDIS_RECEIVE_CONTEXT Receive)
{
    MbbBusReturnReceiveBuffer(Receive->BusHandle, Receive->BusContext, Receive->Mdl);

    WdfObjectDelete(Receive->ReceiveLookasideBufferMemory);
}

VOID MbbRecvReturnNdp(_In_ PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext, _In_opt_ PMBB_RECEIVE_NDP_CONTEXT ReceiveNdpContext)
{
    LONG newCompletedCount = InterlockedIncrement(&ReceiveContext->CompletedNdpCount);
    if (newCompletedCount == ReceiveContext->TotalNdpCount)
    {
        MbbRecvCleanup(ReceiveContext);
    }
    if (ReceiveNdpContext != NULL)
    {
        WdfObjectDelete(ReceiveNdpContext->ReceiveNdpLookasideBufferMemory);
    }
}

_Use_decl_annotations_ VOID MbbRecvCancelNdps(PWMBCLASS_DEVICE_CONTEXT DeviceContext, ULONG SessionId)
{
    LIST_ENTRY tempList;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY nextEntry;
    PMBB_RECEIVE_NDP_CONTEXT receiveNdpContext = NULL;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = DeviceContext->Sessions[SessionId].NetAdapterContext;

    InitializeListHead(&tempList);

    WdfSpinLockAcquire(DeviceContext->Sessions[SessionId].WdfRecvSpinLock);
    InsertHeadList(&netAdapterContext->ReceiveNdpList, &tempList);
    RemoveEntryList(&netAdapterContext->ReceiveNdpList);
    InitializeListHead(&netAdapterContext->ReceiveNdpList);
    // Cannot cache the received Ndps anymore
    netAdapterContext->AllowRxTraffic = FALSE;
    WdfSpinLockRelease(DeviceContext->Sessions[SessionId].WdfRecvSpinLock);

    for (listEntry = tempList.Flink; listEntry != &tempList; listEntry = nextEntry)
    {
        nextEntry = listEntry->Flink;
        receiveNdpContext = CONTAINING_RECORD(listEntry, MBB_RECEIVE_NDP_CONTEXT, ReceiveNdpNode);
        MbbRecvReturnNdp(receiveNdpContext->ReceiveContext, receiveNdpContext);
    }
}

void MbbReceiveDssData(__in PWMBCLASS_DEVICE_CONTEXT DeviceContext, __in ULONG SessionId, __in_bcount_opt(InBufferSize) PUCHAR InBuffer, __in ULONG InBufferSize)
{
    NTSTATUS status;
    WDFMEMORY data;

    status = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, InBuffer, InBufferSize, &data);

    if (NT_SUCCESS(status))
    {
        DeviceContext->DSSPacketsReceivedCount++;

        MbbDeviceReceiveDeviceServiceSessionData(DeviceContext->WdfDevice, SessionId, data);
        WdfObjectDelete(data);
    }
}

NTSTATUS
MbbRecvNdpUnpackDss32(_In_ PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext, _In_ PNCM_NTH32 Nth, _In_ PNCM_NDP32 Ndp)
{
    UNREFERENCED_PARAMETER(Nth);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG datagramCount = MBB_NDP32_GET_DATAGRAM_COUNT(Ndp);
    ULONG sessionId = MBB_NDP32_GET_SESSIONID(Ndp);
    for (ULONG index = 0; index < datagramCount; index++)
    {
        if (MBB_NTB32_IS_END_DATAGRAM(Nth, Ndp, index))
            break;

        // Forward to the dss receive handler
        MbbReceiveDssData(
            ReceiveContext->WmbDeviceContext, sessionId, (PUCHAR)MBB_NDP32_GET_DATAGRAM(Nth, Ndp, index), MBB_NDP32_GET_DATAGRAM_LENGTH(Ndp, index));
    }

    MbbRecvReturnNdp(ReceiveContext, NULL);
    return status;
}

NTSTATUS
MbbRecvNdpUnpackDss16(_In_ PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext, _In_ PNCM_NTH16 Nth, _In_ PNCM_NDP16 Ndp)
{
    UNREFERENCED_PARAMETER(Nth);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG datagramCount = MBB_NDP16_GET_DATAGRAM_COUNT(Ndp);
    ULONG sessionId = MBB_NDP16_GET_SESSIONID(Ndp);
    for (ULONG index = 0; index < datagramCount; index++)
    {
        if (MBB_NTB16_IS_END_DATAGRAM(Nth, Ndp, index))
            break;

        // Forward to the dss receive handler
        MbbReceiveDssData(
            ReceiveContext->WmbDeviceContext, sessionId, (PUCHAR)MBB_NDP16_GET_DATAGRAM(Nth, Ndp, index), MBB_NDP16_GET_DATAGRAM_LENGTH(Ndp, index));
    }
    MbbRecvReturnNdp(ReceiveContext, NULL);
    return status;
}

NTSTATUS
MbbRecvAddNdp(_In_ PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext, _In_ PVOID Nth, _In_ PVOID Ndp, _In_ ULONG SessionId)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = ReceiveContext->WmbDeviceContext;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = NULL;
    WDFMEMORY ndpContextMemory;
    PMBB_RECEIVE_NDP_CONTEXT ndpContext = NULL;
    size_t ndpContextSize;
    BOOLEAN returnNdp = FALSE;

    WdfSpinLockAcquire(deviceContext->Sessions[SessionId].WdfRecvSpinLock);
    do
    {
        netAdapterContext = deviceContext->Sessions[SessionId].NetAdapterContext;
        if (netAdapterContext == NULL)
        {
            returnNdp = TRUE;
            break;
        }

        if (!NT_SUCCESS(status = WdfMemoryCreateFromLookaside(netAdapterContext->ReceiveNdpLookasideList, &ndpContextMemory)))
        {
            returnNdp = TRUE;
            break;
        }

        ndpContext = (PMBB_RECEIVE_NDP_CONTEXT)WdfMemoryGetBuffer(ndpContextMemory, &ndpContextSize);
        RtlZeroMemory(ndpContext, ndpContextSize);
        ndpContext->Nth = Nth;
        ndpContext->Ndp = Ndp;
        ndpContext->ReceiveContext = ReceiveContext;
        ndpContext->ReceiveNdpLookasideBufferMemory = ndpContextMemory;
        ndpContext->NetAdapterContext = netAdapterContext;
        if (netAdapterContext->AllowRxTraffic)
        {
            InsertTailList(&netAdapterContext->ReceiveNdpList, &ndpContext->ReceiveNdpNode);
        }
        else
        {
            returnNdp = TRUE;
            break;
        }

        // If RxQueueCreate hasn't completed yet, we only queue the received Ndp and don't need notify RxReady
        if (netAdapterContext->RxQueue)
        {
            MbbNotifyRxReady(netAdapterContext->RxQueue);
        }
    } while (FALSE);
    WdfSpinLockRelease(deviceContext->Sessions[SessionId].WdfRecvSpinLock);

    if (returnNdp)
    {
        MbbRecvReturnNdp(ReceiveContext, ndpContext);
    }
    return status;
}

VOID MbbRecvNtbParse(_In_ PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNCM_NTH32 nth32;
    PNCM_NTH16 nth16;
    PNCM_NDP32 ndp32;
    PNCM_NDP16 ndp16;
    PVOID nth = ReceiveContext->ReceiveNtbBuffer;

    if (MBB_NTB_IS_32BIT(nth))
    {
        nth32 = (PNCM_NTH32)nth;
        for (ndp32 = MBB_NTH32_GET_FIRST_NDP(nth32); ndp32 != NULL; ndp32 = MBB_NDP32_GET_NEXT_NDP(nth32, ndp32))
        {
            if (MBB_NDP32_GET_SIGNATURE_TYPE(ndp32) == NCM_NDP32_VENDOR)
            {
                status = MbbRecvNdpUnpackDss32(ReceiveContext, nth32, ndp32);
                if (!NT_SUCCESS(status))
                {
                }
            }
            else if (MBB_NDP32_GET_SIGNATURE_TYPE(ndp32) == NCM_NDP32_IPS)
            {
                status = MbbRecvAddNdp(ReceiveContext, nth32, ndp32, MBB_NDP32_GET_SESSIONID(ndp32));
                if (!NT_SUCCESS(status))
                {
                }
            }
            else
            {
                MbbRecvReturnNdp(ReceiveContext, NULL);
            }
        }
    }
    else if (MBB_NTB_IS_16BIT(nth))
    {
        nth16 = (PNCM_NTH16)nth;
        for (ndp16 = MBB_NTH16_GET_FIRST_NDP(nth16); ndp16 != NULL; ndp16 = MBB_NDP16_GET_NEXT_NDP(nth16, ndp16))
        {
            if (MBB_NDP16_GET_SIGNATURE_TYPE(ndp16) == NCM_NDP16_VENDOR)
            {
                status = MbbRecvNdpUnpackDss16(ReceiveContext, nth16, ndp16);
                if (!NT_SUCCESS(status))
                {
                }
            }
            else if (MBB_NDP16_GET_SIGNATURE_TYPE(ndp16) == NCM_NDP16_IPS)
            {
                status = MbbRecvAddNdp(ReceiveContext, nth16, ndp16, MBB_NDP32_GET_SESSIONID(ndp16));
                if (!NT_SUCCESS(status))
                {
                }
            }
            else
            {
                MbbRecvReturnNdp(ReceiveContext, NULL);
            }
        }
    }
}

NTSTATUS
MbbRecvNtbUnpackIpNdp32(_In_ PMBB_RECEIVE_NDP_CONTEXT ReceiveNdpContext, _In_ PNCM_NTH32 Nth32, _In_ PNCM_NDP32 Ndp32, _Inout_ ULONG* IncompletedDatagramIndex, _In_ PMBB_RXQUEUE_CONTEXT RxQueueContext)
{
    PCHAR incompletedDatagram;
    ULONG incompletedDatagramLength;
    NET_PACKET* packet;
    NET_FRAGMENT* fragment;
    NET_FRAGMENT_RETURN_CONTEXT* returnContext;
    NET_FRAGMENT_VIRTUAL_ADDRESS* virtualAddress;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG datagramCount = MBB_NDP32_GET_DATAGRAM_COUNT(Ndp32);

    NET_RING * pr = NetRingCollectionGetPacketRing(RxQueueContext->DatapathDescriptor);
    NET_RING * fr = NetRingCollectionGetFragmentRing(RxQueueContext->DatapathDescriptor);

    while ((incompletedDatagram = MBB_NDP32_GET_DATAGRAM(Nth32, Ndp32, *IncompletedDatagramIndex)) != NULL)
    {
        if (*IncompletedDatagramIndex >= datagramCount)
        {
            break;
        }

        if (fr->BeginIndex == fr->EndIndex)
        {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        incompletedDatagramLength = MBB_NDP32_GET_DATAGRAM_LENGTH(Ndp32, *IncompletedDatagramIndex);
        
        UINT32 const fragmentIndex = fr->BeginIndex;
        fragment = NetRingGetFragmentAtIndex(fr, fragmentIndex);
        fragment->Capacity = incompletedDatagramLength;
        fragment->ValidLength = incompletedDatagramLength;
        fragment->Offset = 0;

        returnContext = NetExtensionGetFragmentReturnContext(&RxQueueContext->ReturnContextExtension, fragmentIndex);
        virtualAddress =
            NetExtensionGetFragmentVirtualAddress(&RxQueueContext->VirtualAddressExtension, fragmentIndex);


        returnContext->Handle = (NET_FRAGMENT_RETURN_CONTEXT_HANDLE)ReceiveNdpContext;
        virtualAddress->VirtualAddress = incompletedDatagram;

        (ReceiveNdpContext->IndicatedPackets)++;
        (*IncompletedDatagramIndex)++;

        UINT32 const packetIndex = pr->BeginIndex;
        packet = NetRingGetPacketAtIndex(pr, packetIndex);
        packet->FragmentIndex = fragmentIndex;
        packet->FragmentCount = 1;
        packet->Layout = {};

        pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
        fr->BeginIndex = NetRingIncrementIndex(fr, fr->BeginIndex);    
    }
    return status;
}

NTSTATUS
MbbRecvNtbUnpackIpNdp16(_In_ PMBB_RECEIVE_NDP_CONTEXT ReceiveNdpContext, _In_ PNCM_NTH16 Nth16, _In_ PNCM_NDP16 Ndp16, _Inout_ ULONG* IncompletedDatagramIndex, _In_ PMBB_RXQUEUE_CONTEXT RxQueueContext)
{
    PCHAR incompletedDatagram;
    ULONG incompletedDatagramLength;
    NET_PACKET* packet;
    NET_FRAGMENT* fragment;
    NET_FRAGMENT_RETURN_CONTEXT* returnContext;
    NET_FRAGMENT_VIRTUAL_ADDRESS* virtualAddress;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG datagramCount = MBB_NDP16_GET_DATAGRAM_COUNT(Ndp16);

    NET_RING * pr = NetRingCollectionGetPacketRing(RxQueueContext->DatapathDescriptor);
    NET_RING * fr = NetRingCollectionGetFragmentRing(RxQueueContext->DatapathDescriptor);

    while ((incompletedDatagram = MBB_NDP16_GET_DATAGRAM(Nth16, Ndp16, *IncompletedDatagramIndex)) != NULL)
    {
        if (*IncompletedDatagramIndex >= datagramCount)
        {
            break;
        }

        if (fr->BeginIndex == fr->EndIndex)
        {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        incompletedDatagramLength = MBB_NDP16_GET_DATAGRAM_LENGTH(Ndp16, *IncompletedDatagramIndex);

        UINT32 const fragmentIndex = fr->BeginIndex;
        fragment = NetRingGetFragmentAtIndex(fr, fragmentIndex);
        fragment->Capacity = incompletedDatagramLength;
        fragment->ValidLength = incompletedDatagramLength;
        fragment->Offset = 0;

        returnContext = NetExtensionGetFragmentReturnContext(&RxQueueContext->ReturnContextExtension, fragmentIndex);
        virtualAddress =
            NetExtensionGetFragmentVirtualAddress(&RxQueueContext->VirtualAddressExtension, fragmentIndex);

        returnContext->Handle = (NET_FRAGMENT_RETURN_CONTEXT_HANDLE)ReceiveNdpContext;
        virtualAddress->VirtualAddress = incompletedDatagram;

        (ReceiveNdpContext->IndicatedPackets)++;
        (*IncompletedDatagramIndex)++;

        UINT32 const packetIndex = pr->BeginIndex;
        packet = NetRingGetPacketAtIndex(pr, packetIndex);
        packet->FragmentIndex = fragmentIndex;
        packet->FragmentCount = 1;
        packet->Layout = {};

        pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
        fr->BeginIndex = NetRingIncrementIndex(fr, fr->BeginIndex); 
    }

    return status;
}

NTSTATUS
MbbRecvNdpUnpackIps(_In_ PMBB_RECEIVE_NDP_CONTEXT ReceiveNdpContext, _In_ PMBB_RXQUEUE_CONTEXT RxQueueContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNCM_NTH32 nth32;
    PNCM_NTH16 nth16;
    PNCM_NDP32 ndp32;
    PNCM_NDP16 ndp16;
    PMBB_NDIS_RECEIVE_CONTEXT receiveContext = ReceiveNdpContext->ReceiveContext;
    PVOID nth = receiveContext->ReceiveNtbBuffer;

    if (MBB_NTB_IS_32BIT(nth))
    {
        nth32 = (PNCM_NTH32)nth;
        ndp32 = (PNCM_NDP32)ReceiveNdpContext->Ndp;
        status = MbbRecvNtbUnpackIpNdp32(ReceiveNdpContext, nth32, ndp32, &ReceiveNdpContext->CurrentDatagramIndex, RxQueueContext);
    }
    else if (MBB_NTB_IS_16BIT(nth))
    {
        nth16 = (PNCM_NTH16)nth;
        ndp16 = (PNCM_NDP16)ReceiveNdpContext->Ndp;
        status = MbbRecvNtbUnpackIpNdp16(ReceiveNdpContext, nth16, ndp16, &ReceiveNdpContext->CurrentDatagramIndex, RxQueueContext);
    }

    return status;
}

PMBB_NDIS_RECEIVE_CONTEXT
MbbRecvQQueueReceive(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext, _In_ MBB_RECEIVE_CONTEXT BusContext, _In_ PMDL Mdl, _In_reads_(sizeof(NCM_NTH32)) PUCHAR ReceiveNtbBuffer)
{
    PMBB_NDIS_RECEIVE_CONTEXT receiveContext = NULL;
    WDFMEMORY receiveContextMemory;
    size_t receiveContextSize;
    NTSTATUS status = STATUS_SUCCESS;

    do
    {
        if (!NT_SUCCESS(status = WdfMemoryCreateFromLookaside(DeviceContext->ReceiveLookasideList, &receiveContextMemory)))
        {
            break;
        }

        receiveContext = (PMBB_NDIS_RECEIVE_CONTEXT)WdfMemoryGetBuffer(receiveContextMemory, &receiveContextSize);
        RtlZeroMemory(receiveContext, receiveContextSize);

        receiveContext->Mdl = Mdl;
        receiveContext->WmbDeviceContext = DeviceContext;
        receiveContext->BusHandle = DeviceContext->BusHandle;
        receiveContext->BusContext = BusContext;
        receiveContext->ReceiveLookasideList = DeviceContext->ReceiveLookasideList;
        receiveContext->ReceiveLookasideBufferMemory = receiveContextMemory;
        receiveContext->ReceiveNtbBuffer = ReceiveNtbBuffer;
        receiveContext->NtbSequence = MBB_NTB_GET_SEQUENCE(ReceiveNtbBuffer);
    } while (FALSE);

    return receiveContext;
}

VOID MbbNdisReceiveCallback(_In_ MBB_PROTOCOL_HANDLE ProtocolHandle, _In_ MBB_RECEIVE_CONTEXT ReceiveContext, _In_ PMDL Mdl)
{
    PUCHAR dataBuffer;
    ULONG dataLength;
    PVOID nth;
    BOOLEAN returnBuffer = TRUE;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = (PWMBCLASS_DEVICE_CONTEXT)ProtocolHandle;
    PMBB_NDIS_RECEIVE_CONTEXT receive = NULL;
    ULONG ndpCount = 1;
    NTSTATUS status = STATUS_SUCCESS;
    do
    {
        if ((dataBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority | MdlMappingNoExecute)) == NULL)
        {
            status = STATUS_RESOURCE_DATA_NOT_FOUND;
            break;
        }
        dataLength = MmGetMdlByteCount(Mdl);

        nth = dataBuffer;

        if (!NT_SUCCESS(status = MbbNtbValidate(nth, dataLength, deviceContext->BusParams.CurrentMode32Bit, &ndpCount)))
        {
            {
                ULONG i;
                ULONG j;

                PUCHAR buffer = (PUCHAR)nth;

                for (i = 0; i < dataLength; i += 16)
                {
                    UCHAR numString[16 * 3 + 1];
                    UCHAR asciiBuffer[40];
                    UCHAR value = 0;
                    const char translateTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

                    RtlZeroMemory(numString, sizeof(numString));
                    RtlZeroMemory(asciiBuffer, sizeof(asciiBuffer));

                    for (j = 0; j < 16; j++)
                    {
                        if (i + j >= dataLength)
                        {
                            //
                            //  past the end, just put spaces
                            //
                            numString[j * 3] = ' ';
                            numString[j * 3 + 1] = ' ';
                            numString[j * 3 + 2] = ' ';
                        }
                        else
                        {
                            value = buffer[i + j];

                            numString[j * 3] = translateTable[value >> 4];
                            numString[j * 3 + 1] = translateTable[value & 0xf];
                            numString[j * 3 + 2] = ' ';

                            if ((buffer[i + j] >= 32) && (buffer[i + j] < 128))
                            {
                                asciiBuffer[j] = value;
                            }
                            else
                            {
                                asciiBuffer[j] = '.';
                            }
                        }
                    }
                }
            }
            break;
        }

        if ((receive = MbbRecvQQueueReceive(deviceContext, ReceiveContext, Mdl, (PUCHAR)nth)) == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        returnBuffer = FALSE;
        receive->TotalNdpCount = ndpCount;

        // Receive DSS, IPs will be received in EvtRxQueueAdvance
        MbbRecvNtbParse(receive);
    } while (FALSE);

    if (returnBuffer)
    {
        if (receive != NULL)
        {
            MbbRecvCleanup(receive);
        }
        else
        {
            MbbBusReturnReceiveBuffer(deviceContext->BusHandle, ReceiveContext, Mdl);
        }
    }
}

void EvtRxQueueDestroy(_In_ WDFOBJECT RxQueue)
{
    PMBB_RXQUEUE_CONTEXT rxQueueContext = MbbGetRxQueueContext(RxQueue);
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = rxQueueContext->NetAdapterContext;
    netAdapterContext->RxQueue = NULL;
}

VOID EvtRxQueueSetNotificationEnabled(_In_ NETPACKETQUEUE RxQueue, _In_ BOOLEAN NotificationEnabled)
{
    PMBB_RXQUEUE_CONTEXT txQueueContext = MbbGetRxQueueContext(RxQueue);
    InterlockedExchange(&txQueueContext->NotificationEnabled, NotificationEnabled);
}

void EvtRxQueueAdvance(_In_ NETPACKETQUEUE RxQueue)
{
    PMBB_RXQUEUE_CONTEXT rxQueueContext = MbbGetRxQueueContext(RxQueue);
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = rxQueueContext->NetAdapterContext;
    WDFSPINLOCK wdfRecvSpinLock = netAdapterContext->WmbDeviceContext->Sessions[netAdapterContext->SessionId].WdfRecvSpinLock;
    PMBB_RECEIVE_NDP_CONTEXT receiveNdpContext;
    NTSTATUS status = STATUS_SUCCESS;

    WdfSpinLockAcquire(wdfRecvSpinLock);
    while (!IsListEmpty(&netAdapterContext->ReceiveNdpList))
    {
        receiveNdpContext = CONTAINING_RECORD(RemoveHeadList(&netAdapterContext->ReceiveNdpList), MBB_RECEIVE_NDP_CONTEXT, ReceiveNdpNode);
        receiveNdpContext->ReceiveNdpNode.Flink = receiveNdpContext->ReceiveNdpNode.Blink = NULL;
        WdfSpinLockRelease(wdfRecvSpinLock);

        status = MbbRecvNdpUnpackIps(receiveNdpContext, rxQueueContext);
        if (status == STATUS_BUFFER_OVERFLOW)
        {
            WdfSpinLockAcquire(wdfRecvSpinLock);
            InsertHeadList(&netAdapterContext->ReceiveNdpList, &receiveNdpContext->ReceiveNdpNode);
            break;
        }
        else
        {
            if (!NT_SUCCESS(status))
            {
            }
        }

        WdfSpinLockAcquire(wdfRecvSpinLock);
    }
    WdfSpinLockRelease(wdfRecvSpinLock);
}

void EvtRxQueueCancel(_In_ NETPACKETQUEUE RxQueue)
{
    PMBB_RXQUEUE_CONTEXT rxQueueContext = MbbGetRxQueueContext(RxQueue);
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = rxQueueContext->NetAdapterContext;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = netAdapterContext->WmbDeviceContext;
    NET_RING_COLLECTION const* rings = rxQueueContext->DatapathDescriptor;

    NET_RING * pr = NetRingCollectionGetPacketRing(rings);

    while (pr->BeginIndex != pr->EndIndex)
    {
        NET_PACKET * packet = NetRingGetPacketAtIndex(pr, pr->BeginIndex);
        packet->Ignore = 1;
        pr->BeginIndex = NetRingIncrementIndex(pr, pr->BeginIndex);
    }

    NET_RING * fr = NetRingCollectionGetFragmentRing(rings);
    fr->BeginIndex = fr->EndIndex; 

    // Cancel received Ndps which may be received after EvtRxQueueCancel started, these Ndps may never be processed by EvtRxQueueAdvance
    MbbRecvCancelNdps(deviceContext, netAdapterContext->SessionId);
}

VOID EvtAdapterReturnRxBuffer(_In_ NETADAPTER Adapter, _In_ NET_FRAGMENT_RETURN_CONTEXT_HANDLE RxBufferReturnContext)
{
    PMBB_RECEIVE_NDP_CONTEXT receiveNdpContext = (PMBB_RECEIVE_NDP_CONTEXT)RxBufferReturnContext;
    UNREFERENCED_PARAMETER(Adapter);
    NT_ASSERT(receiveNdpContext->IndicatedPackets);

    receiveNdpContext->IndicatedPackets--;
    if ((receiveNdpContext->IndicatedPackets == 0) && (receiveNdpContext->ReceiveNdpNode.Blink == NULL) &&
        (receiveNdpContext->ReceiveNdpNode.Flink == NULL))
    {
        MbbRecvReturnNdp(receiveNdpContext->ReceiveContext, receiveNdpContext);
    }
}
