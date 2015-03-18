/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Vmq.c

Abstract:

   This module implements the VMQ related functionality for the adapter.

--*/

#include "netvmin6.h"
#include "vmq.tmh"

NDIS_STATUS
FreeAdapterQueueInfo(
    _In_ PMP_ADAPTER Adapter,
    USHORT QueueId,
    _In_opt_ PNDIS_OID_REQUEST   NdisSetRequest);

NDIS_STATUS
InitializeAdapterQueueInfo(
    _In_ PMP_ADAPTER Adapter,
    _In_opt_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams,
    _Out_ PMP_ADAPTER_QUEUE QueueInfo);

NDIS_STATUS
AllocateRxQueueReceiveMemory(
    _Inout_ struct _MP_ADAPTER *Adapter,
    PMP_ADAPTER_QUEUE Queue);

NDIS_IO_WORKITEM_FUNCTION FreeRxQueuesWorkItem;

NDIS_STATUS
AllocateVMQData(
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will initialize the basic fields necessary for a MP_ADAPTER_VMQ_DATA structure. The function
    should be called during adapter initialization, before any queues or filter requests are fielded.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter         - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    UINT i;

    DEBUGP(MP_TRACE, "[%p] ---> AllocateVMQData\n", Adapter);

    do
    {
        NdisZeroMemory(VMQData, sizeof(MP_ADAPTER_VMQ_DATA));

        //
        // Initialize the locks for the queues
        //
        for(i=0; i<NIC_SUPPORTED_NUM_QUEUES;i++)
        {
            VMQData->RxQueues[i].QueueLock = NdisAllocateRWLock(Adapter->AdapterHandle);
            if(!VMQData->RxQueues[i].QueueLock)
            {
                DEBUGP(MP_ERROR, "[%p] NdisAllocateRWLock failed for Queue %i.\n", Adapter, i);
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }

        if(Status != NDIS_STATUS_SUCCESS)
        {
            //
            // Failed allocating the locks, need to fail this allocation
            //
            break;
        }

    }while(FALSE);

    DEBUGP(MP_TRACE, "<--- [%p] AllocateVMQData Status 0x%08x\n", Adapter, Status);

    return Status;
}

NDIS_STATUS
AllocateDefaultRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will allocate the default receive queue (MP_ADAPTER_QUEUE). The function
    should be called during adapter initialization, before any queues or filter requests are fielded.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter         - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;

    do
    {
        //
        // Initialize the default queue.
        // No need to take lock since this is done during adapter initialization. We won't be fielding receives until this is complete.
        //
        Status = InitializeAdapterQueueInfo(Adapter, NULL, &VMQData->RxQueues[0]);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] Failed to initialize the default queue.\n", Adapter);
            break;
        }

        //
        // Allocate the receive memory (no shared memory is necessary)
        //
        Status = AllocateRxQueueReceiveMemory(Adapter, &VMQData->RxQueues[0]);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] Failed to allocate the receive memory for the default queue.\n", Adapter);
            break;
        }

        //
        // Set default as completed. At this point the queue is ready for receives.
        //
        QUEUE_SET_FLAG(&VMQData->RxQueues[0], fMPAQI_COMPLETION_FINISHED);
    } while(FALSE);

    return Status;
}

NDIS_STATUS
ReadRxQueueConfig(
    _In_ NDIS_HANDLE ConfigurationHandle,
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will read the VMQ configuration from the NDIS registry, and set the results to the VMQData flags field.

Arguments:

    ConfigurationHandle     - Adapter configuration handle
    Adapter                 - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING VMQKeyword = NDIS_STRING_CONST("*VMQ"),
                LookaheadKeyword = NDIS_STRING_CONST("*VMQLookaheadSplit"),
                FilteringKeyword = NDIS_STRING_CONST("*VMQVlanFiltering");

    DEBUGP(MP_TRACE, "[%p] ---> ReadRxQueueConfig\n", Adapter);

    do
    {
        //
        // Read the *VMQ flag (whether VMQ is enabled on the adapter).
        // Note: RSS and VMQ cannot be active simultaneously. If the miniport supports
        // RSS, and the *RSS keyword is set to 1, then the *RssOrVmqPreference keyword
        // must be used to determine which technology should be enabled on the adapter
        // (0: enabled RSS, disable VMQ, 1: disable RSS, enable VMQ).
        //
        //
        NdisReadConfiguration(
                &Status,
                &Parameter,
                ConfigurationHandle,
                &VMQKeyword,
                NdisParameterInteger);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] NdisReadConfiguration for *VMQ failed Status 0x%08x, defaulting to disabled.\n", Adapter, Status);
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        if(Parameter->ParameterData.IntegerData==1)
        {
            VMQ_SET_FLAG(Adapter, fMPVMQD_FILTERING_ENABLED);
        }

        //
        // Read the *VMQLookaheadSplit flag (whether lookahead split is enabled on the adapter)
        //
        NdisReadConfiguration(
                &Status,
                &Parameter,
                ConfigurationHandle,
                &LookaheadKeyword,
                NdisParameterInteger);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] NdisReadConfiguration for *VMQLookaheadSplit failed Status 0x%08x, defaulting to disabled.\n", Adapter, Status);
            Status = NDIS_STATUS_SUCCESS;
        }
        else if(Parameter->ParameterData.IntegerData==1)
        {
            VMQ_SET_FLAG(Adapter, fMPVMQD_LOOKAHEAD_ENABLED);
        }

        //
        // Read the *VMQVlanFiltering flag (whether lookahead split is enabled on the adapter)
        //
        NdisReadConfiguration(
                &Status,
                &Parameter,
                ConfigurationHandle,
                &FilteringKeyword,
                NdisParameterInteger);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] NdisReadConfiguration for *VMQVlanFiltering failed Status 0x%08x, defaulting to disabled.\n", Adapter, Status);
            Status = NDIS_STATUS_SUCCESS;
        }
        else if(Parameter->ParameterData.IntegerData==1)
        {
            VMQ_SET_FLAG(Adapter, fMPVMQD_VLANFILTER_ENABLED);
        }

    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- ReadRxQueueConfig Status 0x%08x\n", Adapter, Status);

    return Status;
}

NDIS_STATUS
InitializeRxQueueMPConfig(
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will set the supported and actual VMQ configuration for the miniport calling NdisMSetMiniportAttributes.

Arguments:

    Adapter               - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES HardwareAssistAttributes;
    NDIS_RECEIVE_FILTER_CAPABILITIES HwCapabilities, CurrentCapabilities;
    NDIS_NIC_SWITCH_CAPABILITIES HwSwitchCapabilities, CurrentSwitchCapabilities;

    DEBUGP(MP_TRACE, "[%p] ---> InitializeRxQueueMPConfig\n", Adapter);

    do
    {
        //
        // Set the VMQ adapter capabilities.
        //
        NdisZeroMemory(&HardwareAssistAttributes, sizeof(HardwareAssistAttributes));

        {C_ASSERT(sizeof(HardwareAssistAttributes) >= NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_2);}
        HardwareAssistAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES;
        HardwareAssistAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_2;
        HardwareAssistAttributes.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_2;

        //
        // Filter capabilities for VMQ
        //

        NdisZeroMemory(&HwCapabilities, sizeof(HwCapabilities));

        //
        // header fields
        //
        HwCapabilities.Header.Revision = NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_1;
        HwCapabilities.Header.Size = NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_1;
        HwCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        //
        // enable VMQ filters
        //
        HwCapabilities.EnabledFilterTypes = NDIS_RECEIVE_FILTER_VMQ_FILTERS_ENABLED;
        //
        // enable VMQ queues
        //
        HwCapabilities.EnabledQueueTypes = NDIS_RECEIVE_FILTER_VM_QUEUES_ENABLED;
        //
        // queue properties
        //
        HwCapabilities.NumQueues = NIC_SUPPORTED_NUM_QUEUES - 1;// First queue reserved for default
        HwCapabilities.SupportedQueueProperties = NDIS_RECEIVE_FILTER_VM_QUEUE_SUPPORTED | NDIS_RECEIVE_FILTER_LOOKAHEAD_SPLIT_SUPPORTED;

#if (NDIS_SUPPORT_NDIS630)
        HwCapabilities.SupportedQueueProperties |= NDIS_RECEIVE_FILTER_DYNAMIC_PROCESSOR_AFFINITY_CHANGE_SUPPORTED;
#endif
        
        HwCapabilities.SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED;
        HwCapabilities.SupportedHeaders = NDIS_RECEIVE_FILTER_MAC_HEADER_SUPPORTED;
        HwCapabilities.SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED | NDIS_RECEIVE_FILTER_MAC_HEADER_VLAN_ID_SUPPORTED;
        HwCapabilities.MaxMacHeaderFilters = NIC_MAX_HEADER_FILTERS;
        HwCapabilities.MinLookaheadSplitSize = NIC_MIN_LOOKAHEAD_SPLIT;
        HwCapabilities.MaxLookaheadSplitSize = NIC_MAX_LOOKAHEAD_SPLIT;
        HardwareAssistAttributes.HardwareReceiveFilterCapabilities = &HwCapabilities;

        //
        // Set the actual VMQ adapter configuration.
        //

        NdisZeroMemory(&CurrentCapabilities, sizeof(CurrentCapabilities));

        //
        // header fields
        //
        CurrentCapabilities.Header.Revision = NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_1;
        CurrentCapabilities.Header.Size = NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_1;
        CurrentCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;

        if(VMQ_ENABLED(Adapter))
        {
            //
            //set equal to HW capabilities, which has all the supported options set, and then remove optional disabled capabilties
            //
            memcpy(&CurrentCapabilities, &HwCapabilities, sizeof(CurrentCapabilities));
            if(!LOOKAHEAD_SPLIT_ENABLED(Adapter))
            {
                //
                // Remove lookahead split capability flag, as it's disabled in the driver configuration
                //
                CurrentCapabilities.SupportedQueueProperties &= ~NDIS_RECEIVE_FILTER_LOOKAHEAD_SPLIT_SUPPORTED;
            }
            if(!VLAN_FILTER_ENABLED(Adapter))
            {
                //
                // Remove lookahead vlan filtering capability flag, as it's disabled in the driver configuration
                //
                CurrentCapabilities.SupportedMacHeaderFields &= ~NDIS_RECEIVE_FILTER_MAC_HEADER_VLAN_ID_SUPPORTED;
            }
        }

        //
        // Set NIC switch adapter capabilities.
        //
        HardwareAssistAttributes.CurrentReceiveFilterCapabilities = &CurrentCapabilities;

        NdisZeroMemory(&HwSwitchCapabilities, sizeof(HwSwitchCapabilities));

        HwSwitchCapabilities.Header.Revision = NDIS_NIC_SWITCH_CAPABILITIES_REVISION_1;
        HwSwitchCapabilities.Header.Size = NDIS_SIZEOF_NIC_SWITCH_CAPABILITIES_REVISION_1;
        HwSwitchCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;

        //
        // The NIC's limits corresponds to the maximum number of filters supported.
        //
        HwSwitchCapabilities.NumTotalMacAddresses = NIC_MAX_HEADER_FILTERS;
        HwSwitchCapabilities.NumMacAddressesPerPort = NIC_MAX_HEADER_FILTERS;
        HwSwitchCapabilities.NumVlansPerPort = NIC_MAX_HEADER_FILTERS;

        HardwareAssistAttributes.HardwareNicSwitchCapabilities = &HwSwitchCapabilities;

        //
        // Set the actual NIC switch adapter configuration.
        //
        NdisZeroMemory(&CurrentSwitchCapabilities, sizeof(CurrentSwitchCapabilities));

        CurrentSwitchCapabilities.Header.Revision = NDIS_NIC_SWITCH_CAPABILITIES_REVISION_1;
        CurrentSwitchCapabilities.Header.Size = NDIS_SIZEOF_NIC_SWITCH_CAPABILITIES_REVISION_1;
        CurrentSwitchCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;

        if(VMQ_ENABLED(Adapter))
        {
            //
            // Set equal to HW capabilities, which has all the supported options set.
            //
            memcpy(&CurrentSwitchCapabilities, &HwSwitchCapabilities, sizeof(CurrentSwitchCapabilities));
        }

        HardwareAssistAttributes.CurrentNicSwitchCapabilities = &CurrentSwitchCapabilities;

        Status = NdisMSetMiniportAttributes(
                Adapter->AdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&HardwareAssistAttributes);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] NdisMSetMiniportAttributes Status 0x%08x\n", Adapter, Status);
            break;
        }

    }while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- InitializeRxQueueMPConfig Status 0x%08x\n", Adapter, Status);

    return Status;
}

BOOLEAN
ValidGroupAffinity(
_In_ PGROUP_AFFINITY GroupAffinity)
/*++
Routine Description:

    This routine will verify that the passed in group affinity contains valid values.

    Runs at any IRQL.

Arguments:

    GroupAffinity           - Affinity to verify

Return Value:

    TRUE    -   Affinity is valid
    FALSE   -   Affinity is invalid

--*/
{
    //
    // GetActiveProcessorCount will validate the group value
    //
    #pragma warning( suppress: 28193 ) // we  check ProcessorNumber result later in code
    UCHAR ProcessorNumber = RtlFindLeastSignificantBit((ULONGLONG)GroupAffinity->Mask);
    ULONG ActiveProcessors = KeQueryActiveProcessorCountEx(GroupAffinity->Group);

    if(!ActiveProcessors)
    {
        //
        // Group was invalid
        //
        DEBUGP(MP_TRACE, "ValidGroupAffinity failed (KeQueryActiveProcessorCountEx error). Group: 0x%x, Mask: 0x%I64x\n",
                            GroupAffinity->Group, GroupAffinity->Mask);
        return FALSE;
    }

    if(ProcessorNumber == -1)
    {
        //
        // The affinity mask must be non-zero (RtlFindLeastSignificantBit returns -1 if there aren't any non-zero bits)
        //
        DEBUGP(MP_TRACE, "ValidGroupAffinity failed (zero mask). Group: 0x%x, Mask: 0x%I64x\n", GroupAffinity->Group, GroupAffinity->Mask);
        return FALSE;
    }

    if(ProcessorNumber >= ActiveProcessors)
    {
        //
        // The processor number is out of range
        //
        DEBUGP(MP_TRACE, "ValidGroupAffinity failed (processor number out of range). Group: 0x%x, Mask: 0x%I64x, ProcessorNumber: %i, ActiveProcessors: %i\n",
                            GroupAffinity->Group, GroupAffinity->Mask, ProcessorNumber, ActiveProcessors);
        return FALSE;
    }

    return TRUE;
}

NDIS_STATUS
InitializeAdapterQueueInfo(
    _In_ PMP_ADAPTER Adapter,
    _In_opt_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams,
    _Out_ PMP_ADAPTER_QUEUE QueueInfo)
/*++
Routine Description:

    This routine will allocate the PMP_ADAPTER_QUEUE structure passed in using the QueueParams specified.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                 - Pointer to our adapter
    QueueParams             - NDIS_RECEIVE_QUEUE_PARAMETERS data that define the configuration for the queue
    QueueInfo               - Pointer to PMP_ADAPTER_QUEUE to derefence and allocate

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_RECEIVE_QUEUE_ID QueueId = 0;

    DEBUGP(MP_TRACE, "[%p] ---> InitializeAdapterQueueInfo\n", Adapter);

    do
    {
        if(QueueParams)
        {
            if(!ValidGroupAffinity(&QueueParams->ProcessorAffinity))
            {
                DEBUGP(MP_ERROR, "[%p] Requested group affinity failed validation\n", Adapter);
                Status = NDIS_STATUS_INVALID_PARAMETER;
                break;
            }

            QueueId = QueueParams->QueueId;
            //
            // Copy relevant data from the request
            //
            QueueInfo->QueueId = QueueId;
            QueueInfo->NumSuggestedReceiveBuffers = QueueParams->NumSuggestedReceiveBuffers;
            if(QueueParams->LookaheadSize)
            {
                if(QueueParams->LookaheadSize<NIC_MIN_LOOKAHEAD_SPLIT)
                {
                    //
                    // A smaller lookahead has been requested than what's supported, use our supported minimum
                    //
                    QueueInfo->LookaheadSize = NIC_MIN_LOOKAHEAD_SPLIT;
                    DEBUGP(MP_INFO, "[%p] Requested LA size (%i) for Queue %i is too small, using our minium (%i)\n", Adapter, QueueParams->LookaheadSize, QueueId, NIC_MIN_LOOKAHEAD_SPLIT);
                }
                else if(QueueParams->LookaheadSize>NIC_MAX_LOOKAHEAD_SPLIT)
                {
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    DEBUGP(MP_ERROR, "[%p] Requested LA size (%i) for Queue %i is above our maximum (%i), the request has failed.\n", Adapter, QueueParams->LookaheadSize, QueueId, NIC_MAX_LOOKAHEAD_SPLIT);
                    break;
                }
                else
                {
                    QueueInfo->LookaheadSize = QueueParams->LookaheadSize;
                }
            }


            QueueInfo->NdisFlags = QueueParams->Flags & (~NDIS_RECEIVE_QUEUE_PARAMETERS_CHANGE_MASK);
            QueueInfo->ProcessorAffinity = QueueParams->ProcessorAffinity;
        }

        //
        // Please note that for PCI hardware, rather than maintaining DPCs with differing affinities for receives as we do in this
        // software implementation, the affinity information would instead be applied to the MSI-X configuration.
        //

        if(QueueParams)
        {
            //
            // Set processor affininty for receive DPC
            //
            PROCESSOR_NUMBER ProcNumber = {0};
            ProcNumber.Group = QueueInfo->ProcessorAffinity.Group;
            ProcNumber.Number = RtlFindLeastSignificantBit((ULONGLONG)QueueInfo->ProcessorAffinity.Mask);
            ASSERT(ProcNumber.Number != -1);
            if(ProcNumber.Number == -1)
            {
                //
                // Queue has no affinity bits set.
                //
                Status = NDIS_STATUS_INVALID_PARAMETER;
                DEBUGP(MP_ERROR,
                       "[%p] Queue %i has no affinity bits set.\n",
                       Adapter,
                       QueueId);

                break;
            }
            QueueInfo->ReceiveDpc = NICAllocReceiveDpc(Adapter, ProcNumber.Number, ProcNumber.Group, QueueId);
            if(!QueueInfo->ReceiveDpc)
            {
                //
                // Could not allocate DPC
                //
                Status = NDIS_STATUS_RESOURCES;
                DEBUGP(MP_ERROR, "[%p] Could not allocate receive DPC for Queue %i\n", Adapter, QueueId);
                break;
            }
        }
        else
        {
            //
            // No configuration specified, use this is the default queue so use default DPC
            //
            QueueInfo->ReceiveDpc = NICGetDefaultReceiveDpc(Adapter, QueueId);
        }

        //
        // Initialize the receive block for the specified queue
        //
        Status = NICInitializeReceiveBlock(Adapter, QueueId);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] Could not initialize receive block for Queue %i\n", Adapter, QueueId);
            break;
        }

        //
        // Mark the Queue as initialized. It will not be used for receives until it is marked as completed.
        //
        QUEUE_SET_FLAG(QueueInfo, fMPAQI_INITIALIZED);

    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- InitializeAdapterQueueInfo Status 0x%08x\n", Adapter, Status);

    return Status;

}

NDIS_STATUS
AllocateRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams
)
/*++
Routine Description:

    This routine will allocate the corresponding queue in the passed in QueueParams,
    and persist any relevant configuration from the request.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter               - Pointer to our adapter
    QueueParams           - Pointer to the queue allocation parameters

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    PMP_ADAPTER_QUEUE QueueInfo = &VMQData->RxQueues[QueueParams->QueueId];
    LOCK_STATE_EX LockState;
    BOOLEAN ReleaseLock = FALSE;

    DEBUGP(MP_TRACE, "[%p] ---> AllocateRxQueue\n", Adapter);

    do
    {
        //
        // Verify queue is within the range of supported queues, and that it has not already
        // been allocated
        //
        if(QueueParams->QueueId>=NIC_SUPPORTED_NUM_QUEUES)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Synchronize assigning of allocated memory
        //
        NdisAcquireRWLockWrite(VMQData->RxQueues[QueueParams->QueueId].QueueLock, &LockState, 0);
        ReleaseLock = TRUE;

        if(QUEUE_INITIALIZED(QueueInfo))
        {
            //
            // For this sample, changes are not supported. To support changes check whether
            // Flags&NDIS_RECEIVE_QUEUE_PARAMETERS_CHANGE_MASK != 0. The
            // NDIS_RECEIVE_QUEUE_PARAMETERS_*_CHANGED flags can be checked against
            // to determine which fields are being updated, and apply those changes.
            //
            DEBUGP(MP_ERROR, "[%p] Queue %i already allocated, failing request.\n", Adapter, QueueParams->QueueId);
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }
        else if(QUEUE_FREEING(QueueInfo))
        {
            DEBUGP(MP_ERROR, "[%p] Queue %i is in the process of being freed (RefCount of %i).\n", Adapter, QueueParams->QueueId, RECEIVE_BLOCK_REFERENCE_COUNT(Adapter, QueueParams->QueueId));
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Initialize the queue information
        //
        Status = InitializeAdapterQueueInfo(Adapter, QueueParams, QueueInfo);

    } while(FALSE);

    if(ReleaseLock)
    {
        NdisReleaseRWLock(VMQData->RxQueues[QueueParams->QueueId].QueueLock, &LockState);
    }

    DEBUGP(MP_TRACE, "[%p] <--- AllocateRxQueue Status 0x%08x\n", Adapter, Status);

    return Status;
}

VOID
FreeQueueReceiveSharedMemory(
    _In_ PMP_ADAPTER Adapter,
    ULONG EntryCount,
    _In_reads_bytes_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR MemoryBlock,
    PLIST_ENTRY AllocationList)
/*++
Routine Description:

    This routine frees up the passed in receive shared and tracking memory.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                - Pointer to our adapter
    EntryCount             - Number of memory block entries
    MemoryBlock             - MP_ADAPTER_SHARED_MEMORY_BLOCK memory allocated to track shared memory
    AllocationList          - List of PMP_ADAPTER_SHARED_MEMORY allocations that build up the total amount of shared memory available for receive

Return Value:

    VOID

--*/
{
    ULONG BlockIndex;
    PLIST_ENTRY CurrentAllocation;

    DEBUGP(MP_TRACE, "[%p] ---> FreeQueueReceiveSharedMemory.\n", Adapter);

    if(MemoryBlock)
    {
        //
        // Free the MDLs for the shared memory blocks
        //
        for(BlockIndex = 0; BlockIndex < EntryCount; ++BlockIndex)
        {
            PMP_ADAPTER_SHARED_MEMORY_BLOCK SMBlock = ((PMP_ADAPTER_SHARED_MEMORY_BLOCK)MemoryBlock)+BlockIndex;
#pragma prefast(suppress: 6001, "PREfast doees not believe the MDL is initialized.")
            NdisFreeMdl(SMBlock->Mdl);
        }

        //
        // Free the tracking buffers
        //
        NdisFreeMemory(MemoryBlock, EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK), 0);
    }

    //
    // Free the shared memory
    //
    for(CurrentAllocation = RemoveTailList(AllocationList);
            CurrentAllocation != AllocationList;
                CurrentAllocation = RemoveTailList(AllocationList))
    {
        PMP_ADAPTER_SHARED_MEMORY CurrentSharedMemory = CONTAINING_RECORD(CurrentAllocation, MP_ADAPTER_SHARED_MEMORY, Entry);
        NdisFreeSharedMemory(Adapter->AdapterHandle, CurrentSharedMemory->AllocationHandle);
        NdisFreeMemory(CurrentSharedMemory, sizeof(MP_ADAPTER_SHARED_MEMORY), 0);
    }

    DEBUGP(MP_TRACE, "[%p] <--- FreeQueueReceiveSharedMemory.\n", Adapter);

}

VOID
    FreeRxQueueReceiveMemory(
        _Inout_ PMP_ADAPTER Adapter,
        PMP_ADAPTER_QUEUE QueueInfo)
/*++
Routine Description:

    This routine frees up any receive shared memory and tracking memory allocated for this queue.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                - Pointer to our adapter
    QueueInfo              - Pointer to shared memory owner queue

Return Value:

    VOID

--*/
{

    DEBUGP(MP_TRACE, "[%p] ---> FreeRxQueueReceiveMemory. QueueId: %i\n", Adapter, QueueInfo->QueueId);

    //
    // Free lookahead memory and clear values
    //
    FreeQueueReceiveSharedMemory(
        Adapter,
        QueueInfo->NumPostLookaheadBlocks,
        QueueInfo->PostLookaheadBlocks,
        &QueueInfo->PostLookaheadSharedMemoryList);

    QueueInfo->PostLookaheadBlocks = NULL;
    QueueInfo->NumPostLookaheadBlocks = 0;

    //
    // Free postlookahead memory and clear values
    //
    FreeQueueReceiveSharedMemory(
        Adapter,
        QueueInfo->NumLookaheadBlocks,
        QueueInfo->LookaheadBlocks,
        &QueueInfo->LookaheadSharedMemoryList);

    QueueInfo->LookaheadBlocks = NULL;
    QueueInfo->NumLookaheadBlocks = 0;

    DEBUGP(MP_TRACE, "[%p] <--- FreeRxQueueReceiveMemory.\n", Adapter);

}

NDIS_STATUS
    AssociateSharedMemoryAndBlocks(
        _Inout_ struct _MP_ADAPTER *Adapter,
        ULONG EntryCount,
        ULONG EntrySize,
        ULONG OffsetSize,
        _In_ PLIST_ENTRY AllocationList,
        _In_reads_bytes_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR MemoryBlock)
/*++
Routine Description:

    This routine populates each MP_ADAPTER_SHARED_MEMORY_BLOCK entry with its corresponding shared memory allocation.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                 - Pointer to our adapter
    EntryCount              - Number of memory block entries
    EntrySize               - Unaligned size of each shared memory entry (the actual size is aligned up to a memory aligned size)
    OffsetSize              - If this is a postlookahead buffer, this is the offset from the start of the buffer that must be left for lookahead
    AllocationList          - List of PMP_ADAPTER_SHARED_MEMORY allocations that build up the total amount of shared memory available for receive
    MemoryBlock             - MP_ADAPTER_SHARED_MEMORY_BLOCK memory allocated to track shared memory

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(MP_TRACE, "[%p] ---> AssociateSharedMemoryAndBlocks.\n", Adapter);

    do
    {
        ULONG BlockIndex, AllocationIndex, AlignedEntrySize = ALIGN_UP_BY(EntrySize, NdisGetSharedDataAlignment());
        PLIST_ENTRY CurrentAllocation = AllocationList->Flink;
        PMP_ADAPTER_SHARED_MEMORY CurrentSharedMemory;

        //
        // We should have at least one allocation
        //
        ASSERT(CurrentAllocation != AllocationList);

        CurrentSharedMemory = CONTAINING_RECORD(CurrentAllocation, MP_ADAPTER_SHARED_MEMORY, Entry);

        //
        // Populate the shared memory tracking blocks with the necessary information (MDL, NET_BUFFER_SHARED_MEM macros)
        //
        for(BlockIndex = 0, AllocationIndex = 0; BlockIndex < EntryCount; ++BlockIndex, ++AllocationIndex)
        {
            PMP_ADAPTER_SHARED_MEMORY_BLOCK CurrentBlock = (PMP_ADAPTER_SHARED_MEMORY_BLOCK)((MemoryBlock) + (BlockIndex*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)));

            //
            // Find the next free shared memory buffer
            //
            if(AllocationIndex >= CurrentSharedMemory->NumberOfEntries)
            {
                //
                // We've exhausted the current shared memory allocation, need to index off of the next one
                //
                CurrentAllocation = CurrentAllocation->Flink;
                ASSERT(CurrentAllocation != AllocationList);
                AllocationIndex = 0;
                CurrentSharedMemory = CONTAINING_RECORD(CurrentAllocation, MP_ADAPTER_SHARED_MEMORY, Entry);
            }

            CurrentBlock->Buffer = CurrentSharedMemory->Buffer+(AllocationIndex*AlignedEntrySize);
            //
            // Sanity check that the pointer is cache aligned
            //
            ASSERT(((ULONG_PTR)CurrentBlock->Buffer)%NdisGetSharedDataAlignment() == 0);

            //
            // Allocate the MDL
            //
            CurrentBlock->Mdl = NdisAllocateMdl(Adapter->AdapterHandle,
                                                CurrentBlock->Buffer + OffsetSize,
                                                EntrySize - OffsetSize);
            if(!CurrentBlock->Mdl)
            {
                DEBUGP(MP_ERROR, "[%p] Failed to allocate shared memory MDL.\n", Adapter);
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            //
            // Populate the NET_BUFFER_SHARED_MEMORY information
            //
            NET_BUFFER_SHARED_MEM_HANDLE(&CurrentBlock->BufferSharedMemoryData) = CurrentSharedMemory->MemoryHandle;
            NET_BUFFER_SHARED_MEM_LENGTH(&CurrentBlock->BufferSharedMemoryData) = EntrySize - OffsetSize;
            NET_BUFFER_SHARED_MEM_OFFSET(&CurrentBlock->BufferSharedMemoryData) = (ULONG)((CurrentBlock->Buffer + OffsetSize) - CurrentSharedMemory->Buffer);

            //
            // For now clear next segment field. If lookahead and postlookahead are allocated, the segment and MDLs are linked once allocation is complete
            //
            NET_BUFFER_SHARED_MEM_NEXT_SEGMENT(&CurrentBlock->BufferSharedMemoryData) = NULL;
        }
    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- AssociateSharedMemoryAndBlocks. Status=0x%x\n", Adapter, Status);

    return Status;
}

VOID
ChainMemoryBlocks(
    ULONG EntryCount,
    _In_reads_bytes_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR LookaheadBlock,
    _In_reads_bytes_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK))PUCHAR PostLookaheadBlock)
/*++
Routine Description:

    This routine steps through each entry in the lookahead and postlookahead memory blocks and sets up their
    relationship by linking pointing the lookahead's MDL and NET_BUFFER_SHARED_MEM next fields to the postlookahead.

Arguments:

    EntryCount              - Number of entris to step through in the memory block
    LookaheadBlock          - Lookahead tracking memory block
    PostLookaheadBlock      - Postlookahead tracking memory block

Return Value:

    VOID

--*/
{
    ULONG BlockIndex;
    ASSERT(LookaheadBlock);
    for(BlockIndex = 0; BlockIndex < EntryCount; ++BlockIndex)
    {
        PMP_ADAPTER_SHARED_MEMORY_BLOCK CurrentLookaheadBlock = (PMP_ADAPTER_SHARED_MEMORY_BLOCK)((LookaheadBlock) + (BlockIndex*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)));
        PMP_ADAPTER_SHARED_MEMORY_BLOCK CurrentPostLookaheadBlock = (PMP_ADAPTER_SHARED_MEMORY_BLOCK)((PostLookaheadBlock) + (BlockIndex*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)));

        //
        // link MDLs
        //
        CurrentLookaheadBlock->Mdl->Next = CurrentPostLookaheadBlock->Mdl;

        //
        // link shared memory info so that lookahead points to the postlookahead block
        //
        NET_BUFFER_SHARED_MEM_NEXT_SEGMENT(&CurrentLookaheadBlock->BufferSharedMemoryData) = &CurrentPostLookaheadBlock->BufferSharedMemoryData;
    }
}

_Success_(return == NDIS_STATUS_SUCCESS)
NDIS_STATUS
    AllocateSharedMemory(
        _In_ PMP_ADAPTER Adapter,
        NDIS_RECEIVE_QUEUE_ID QueueId,
        NDIS_SHARED_MEMORY_USAGE Usage,
        ULONG BlockSize,
        _Out_ PNDIS_HANDLE AllocationHandle,
        _Out_ PNDIS_HANDLE MemoryHandle,
        _Outptr_result_bytebuffer_(BlockSize) PUCHAR *SharedBuffer
        )
/*++
Routine Description:

    This routine allocates shared memory for the specified QueueId (the default queue is not valid here,
    since no shared memory is used). This should be called through AllocateRxSharedMemory, to get failure recovery.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                 - Pointer to our adapter
    QueueId                 - QueueId owning the shared memory
    Usage                   - Usage of the shared memory (lookahead, postlookahead, receive)
    BlockSize               - Size of the block to allocate
    AllocationHandle        - Receives the Allocation handle returned from NdisAllocateSharedMemory
    MemoryHandle            - Receives the Memory handle returned from NdisAllocateSharedMemory
    SharedBuffer            - Receives the allocated shared memory

Return Value:

    NDIS_STATUS

--*/
{

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_SHARED_MEMORY_PARAMETERS MemoryParams;
    PSCATTER_GATHER_LIST SGListBuffer = NULL;
    ULONG SGListSize, SGListNumElements;

    DEBUGP(MP_TRACE, "[%p] ---> AllocateSharedMemory\n", Adapter);

    *MemoryHandle = NULL;
	*SharedBuffer = NULL;

    do
    {
        //
        // Build the scatter gather list
        //

        //
        // Calculate the number of pages that would be necessary for the specified block size.
        //
        SGListNumElements = BYTES_TO_PAGES(BlockSize);
        //
        // Allocate gather list elements to hold information for each of these pages.
        //
        SGListSize = sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT) * SGListNumElements;
        SGListBuffer = NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
                                                         SGListSize,
                                                         NIC_TAG_QUEUE_SG_LIST,
                                                         NormalPoolPriority);
        if(SGListBuffer == NULL)
        {
            DEBUGP(MP_ERROR, "Failed to allocate scatter gather memory.\n");
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        SGListBuffer->NumberOfElements = SGListNumElements;

        //
        // Prepare the shared memory parameters to be passed in to the NDIS allocation function
        //
        NdisZeroMemory(&MemoryParams, sizeof(NDIS_SHARED_MEMORY_PARAMETERS));
        MemoryParams.Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
        MemoryParams.Header.Revision    = NDIS_SHARED_MEMORY_PARAMETERS_REVISION_1;
        MemoryParams.Header.Size        = NDIS_SIZEOF_SHARED_MEMORY_PARAMETERS_REVISION_1;

        //
        // Memory characteristics
        //
        MemoryParams.Length  = BlockSize;
        MemoryParams.QueueId = QueueId;
        MemoryParams.Usage = Usage;

        //
        // Scatter gather buffer
        //
        MemoryParams.SGListBufferLength = SGListSize;
        MemoryParams.SGListBuffer = SGListBuffer;

        //
        // Allocate the memory
        //
        Status = NdisAllocateSharedMemory(Adapter->AdapterHandle,
                                  &MemoryParams,
                                  AllocationHandle);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] NdisAllocateSharedMemory failed Status 0x%x.\n", Adapter, Status);
            break;
        }

        //
        // Set the buffer to return
        //
        *SharedBuffer = MemoryParams.VirtualAddress;
        *MemoryHandle = MemoryParams.SharedMemoryHandle;

        NdisZeroMemory(*SharedBuffer, BlockSize);

    } while(FALSE);

    if(SGListBuffer)
    {
        //
        // This virtual miniport does not actually do DMA, so no need to preserve the SG information
        //
        NdisFreeMemoryWithTagPriority(NdisDriverHandle, SGListBuffer, NIC_TAG_QUEUE_SG_LIST);
    }

    DEBUGP(MP_TRACE, "[%p] <--- AllocateSharedMemory. Status=0x%x\n", Adapter, Status);

    return Status;
}

NDIS_STATUS
    AllocateRxSharedMemory(
        _In_ PMP_ADAPTER Adapter,
        NDIS_RECEIVE_QUEUE_ID QueueId,
        NDIS_SHARED_MEMORY_USAGE Usage,
        _Inout_ ULONG *EntryCount,
        ULONG EntrySize,
        ULONG OffsetSize,
        _In_ PLIST_ENTRY AllocationList,
        _Outptr_result_bytebuffer_(*EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR *MemoryBlock
        )
/*++
Routine Description:

    This routine wraps the call to AllocateSharedMemory providing failover if the allocation fails. If the original
    requested number of entries fails to allocate, we try to allocate the same buffer size in smaller chunks.

    Runs at IRQL APC_PASSIVE

Arguments:

    Adapter                 - Pointer to our adapter
    QueueId                 - QueueId owning the shared memory
    Usage                   - Usage of the shared memory (lookahead, postlookahead, receive)
    EntryCount              - Number of Entries worth of shared memory to allocate
    EntrySize               - Size of each entry
    AllocationList          - LIST_ENTRY to receive all blocks allocated (MP_ADAPTER_SHARED_MEMORY)
    MemoryBlock             - Receives allocated and populated MP_ADAPTER_SHARED_MEMORY_BLOCK buffer with bookkeeping data on shared memory

Return Value:

    NDIS_STATUS

    On failure, caller needs to call FreeRxQueueReceiveMemory to free any allocations made during the function

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(MP_TRACE, "[%p] ---> AllocateRxSharedMemory.\n", Adapter);

    do
    {
        ULONG AllocatedEntryCount = 0, CurAllocationTarget = *EntryCount;
        //
        // Make sure the entry size results in cache aligned segments
        //
        ULONG AlignedEntrySize =  ALIGN_UP_BY(EntrySize, NdisGetSharedDataAlignment());
        PMP_ADAPTER_SHARED_MEMORY CurMemory = NULL;

        while(AllocatedEntryCount < *EntryCount)
        {
            //
            // Entry size will be cache aligned, allocate enough memory so that we can be sure the entries start at
            // a cache boundary
            //
            ULONG TotalAllocationSize = (CurAllocationTarget*AlignedEntrySize) +  NdisGetSharedDataAlignment();

            if(!CurMemory)
            {
                //
                // Allocate MP_ADAPTER_SHARED_MEMORY we use to track this particular allocation
                //
                CurMemory = NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
                                                              sizeof(MP_ADAPTER_SHARED_MEMORY),
                                                              NIC_TAG_QUEUE_SHARED_MEM,
                                                              NormalPoolPriority);
                if(!CurMemory)
                {
                    break;
                }
                NdisZeroMemory(CurMemory, sizeof(MP_ADAPTER_SHARED_MEMORY));
            }

            //
            // Attempt to allocate shared memory block of (EntryCount * EntrySize) size
            //
            Status = AllocateSharedMemory(Adapter,
                        QueueId,
                        Usage,
                        TotalAllocationSize,
                        &CurMemory->AllocationHandle,
                        &CurMemory->MemoryHandle,
                        &CurMemory->Buffer);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                //
                // Allocation failed, lets try multiple smaller allocation
                //
                CurAllocationTarget /= 2;
                if(CurAllocationTarget >= NIC_MIN_RECV_ENTRY_ALLOCATION_COUNT)
                {
                    DEBUGP(MP_ERROR, "[%p] AllocateSharedMemory failed, will attempt to allocate %i units of memory.\n", Adapter, CurAllocationTarget);
                }
                else
                {
                    DEBUGP(MP_ERROR, "[%p] AllocateSharedMemory failed, reached minimum allocation size. Failing allocation. \n", Adapter);
                    NdisFreeMemory(CurMemory, sizeof(MP_ADAPTER_SHARED_MEMORY), 0);
                    CurMemory = NULL;
                    break;
                }
            }
            else
            {
                //
                // Move base buffer pointer up to a cache aligned boundary
                //
                CurMemory->Buffer = ALIGN_UP_POINTER_BY(CurMemory->Buffer, NdisGetSharedDataAlignment());

                //
                // Store away successful allocation
                //
                CurMemory->EntrySize = AlignedEntrySize;
                CurMemory->NumberOfEntries = CurAllocationTarget;
                AllocatedEntryCount += CurAllocationTarget;
                InsertTailList(AllocationList, &CurMemory->Entry);
                CurMemory = NULL;
            }
        }

        //
        // Verify that we were at least able to allocate up to our minimum number of receive blocks. If not, then we fail the
        // allocation altogether.
        //
        if(AllocatedEntryCount<NIC_MIN_BUSY_RECVS)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Now that the shared memory buffers have been allocated, we need to split them into
        // individual MP_ADAPTER_SHARED_MEMORY_BLOCK items used for tracking the memory in NB
        //

        //
        // Allocate the necessary MP_ADAPTER_SHARED_MEMORY_BLOCK items to track the shared memory
        //
        *MemoryBlock = NdisAllocateMemoryWithTagPriority(
                            NdisDriverHandle,
                            AllocatedEntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK),
                            NIC_TAG_QUEUE_SHARED_MEM_BLOCK,
                            NormalPoolPriority);
        if(!*MemoryBlock)
        {
            DEBUGP(MP_ERROR, "[%p] NdisAllocateMemoryWithTagPriority for shared memory block failed.\n", Adapter);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        NdisZeroMemory(*MemoryBlock, AllocatedEntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK));

        //
        // Associate each ADAPTER_SHARED_MEMOR_BLOCK entry with their corresponding ADAPTER_SHARED_MEMORY buffer segment.
        //
        Status = AssociateSharedMemoryAndBlocks(
            Adapter,
            *EntryCount,
            EntrySize,
            OffsetSize,
            AllocationList,
            *MemoryBlock);

    }while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- AllocateRxSharedMemory. Status=0x%x\n", Adapter, Status);

    return Status;

}

VOID
    AssociateSharedMemoryAndRCBs(
    _In_ ULONG QueueId,
    _In_reads_bytes_opt_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR LookaheadBlock,
    _In_reads_bytes_(EntryCount*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)) PUCHAR PostLookaheadBlock,
    _In_reads_bytes_(EntryCount*sizeof(RCB)) PUCHAR RcbMemoryBlock,
    ULONG EntryCount)
/*++
Routine Description:

    This routine associates populates the fields in each RCB with its corresponding shared memory information
    contained in the lookahead and postlookahead memory block. Once this function runs, each RCB is ready to
    be used for receives (all that needs to happen on receive is to copy the receive data to the lookahead and
    postlookahead memory locations for each RCB).

Arguments:

    QueueId             - ID of the queue which owns the various pieces of memory
    LookaheadBlock     - Lookahead shared memory tracking blocks (can be NULL if no lookahead)
    PostLookaheadBlock - Postlookahead shared memory tracking blocks
    RcbMemoryBlock      - RCB memory blocks

Return Value:

    VOID

--*/
{

    ULONG BlockIndex;
    for(BlockIndex = 0; BlockIndex < EntryCount; ++BlockIndex)
    {
        //
        // Populate the RCB NBLs with the queue's shared memory.
        //
        PRCB Rcb = (PRCB)(RcbMemoryBlock+(BlockIndex*sizeof(RCB)));
        PNET_BUFFER NetBuffer = NET_BUFFER_LIST_FIRST_NB(Rcb->Nbl);
        PMP_ADAPTER_SHARED_MEMORY_BLOCK CurrentPostLookaheadBlock = (PMP_ADAPTER_SHARED_MEMORY_BLOCK)((PostLookaheadBlock) + (BlockIndex*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)));
        Rcb->Data = CurrentPostLookaheadBlock;
        if(LookaheadBlock)
        {
            PMP_ADAPTER_SHARED_MEMORY_BLOCK CurrentLookaheadBlock = (PMP_ADAPTER_SHARED_MEMORY_BLOCK)((LookaheadBlock) + (BlockIndex*sizeof(MP_ADAPTER_SHARED_MEMORY_BLOCK)));
            Rcb->LookaheadData = CurrentLookaheadBlock;
            //
            // Set the shared memory and MDL information
            //
            NET_BUFFER_FIRST_SHARED_MEM_INFO(NetBuffer) = &CurrentLookaheadBlock->BufferSharedMemoryData;
            NET_BUFFER_FIRST_MDL(NetBuffer) = CurrentLookaheadBlock->Mdl;
        }
        else
        {
            //
            // Set the shared memory and MDL information
            //
            NET_BUFFER_FIRST_SHARED_MEM_INFO(NetBuffer) = &CurrentPostLookaheadBlock->BufferSharedMemoryData;
            NET_BUFFER_FIRST_MDL(NetBuffer) = CurrentPostLookaheadBlock->Mdl;
        }

        //
        // Set the remaining NBL fields
        //
        NET_BUFFER_DATA_OFFSET(NetBuffer) = 0;
        NET_BUFFER_CURRENT_MDL(NetBuffer) = NET_BUFFER_FIRST_MDL(NetBuffer);
        NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer) = 0;
        NET_BUFFER_LIST_RECEIVE_QUEUE_ID(Rcb->Nbl) = (USHORT)QueueId;
        NET_BUFFER_LIST_RECEIVE_FILTER_ID(Rcb->Nbl) = 0;
    }

}

NDIS_STATUS
    AllocateRxQueueReceiveMemory(
        _Inout_ struct _MP_ADAPTER *Adapter,
        PMP_ADAPTER_QUEUE QueueInfo)
/*++
Routine Description:

    This routine allocates the necessary receive memory for the specified Queue.

    Runs at IRQL APC_PASSIVE

Arguments:

    Adapter         - Pointer to our adapter
    Queue           - Queue to allocate

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    do
    {
        NDIS_SHARED_MEMORY_USAGE postMemoryUsage = NdisSharedMemoryUsageReceive;

        //
        // Queue should already be initialized
        //
        if(!QUEUE_INITIALIZED(QueueInfo))
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }


        if(QueueInfo->NumSuggestedReceiveBuffers)
        {
            //
            // If the number of receive buffers has been passed down for the queue, then we will allocate that suggested amount (as long as it's not below our minimum required amount of receive buffers),
            // otherwise we use our minimum.
            //
            QueueInfo->NumReceiveBuffers = max(QueueInfo->NumSuggestedReceiveBuffers, NIC_MIN_BUSY_RECVS);
        }
        else
        {
            //
            // No suggested receive buffer count was set for the queue, so we our preferred maximum amount
            //
            QueueInfo->NumReceiveBuffers = NIC_MAX_BUSY_RECVS;
        }

        //
        // Initialize the shared memory allocation tracking lists
        //
        InitializeListHead(&QueueInfo->LookaheadSharedMemoryList);
        InitializeListHead(&QueueInfo->PostLookaheadSharedMemoryList);

        //
        // Initialize list and lock of the free Rcb list
        //
        NdisInitializeListHead(&QueueInfo->FreeRcbList);
        NdisAllocateSpinLock(&QueueInfo->FreeRcbListLock);


        //
        // In our implementation receive memory is only necessary for non-default queues. For default queue
        // we reuse the Frame copy made during send.
        //
        if(QueueInfo->QueueId)
        {
            ULONG LookaheadSize=0, PostLookaheadSize=HW_MAX_FRAME_SIZE;

            if(LOOKAHEAD_SPLIT_REQUIRED(QueueInfo))
            {
                ASSERT(QueueInfo->LookaheadSize);
                LookaheadSize = QueueInfo->LookaheadSize;
            }

            //
            // Allocate the shared memory that will be used to receive on this non-default queue.
            //
            if(LookaheadSize)
            {
                //
                // Lookahead memory
                //
                Status = AllocateRxSharedMemory(
                    Adapter,
                    QueueInfo->QueueId,
                    NdisSharedMemoryUsageReceiveLookahead,
                    &QueueInfo->NumReceiveBuffers,
                    LookaheadSize,
                    0,
                    &QueueInfo->LookaheadSharedMemoryList,
                    &QueueInfo->LookaheadBlocks);

                if(Status != NDIS_STATUS_SUCCESS)
                {
                    break;
                }

                QueueInfo->NumLookaheadBlocks = QueueInfo->NumReceiveBuffers;

                //
                // We're doing lookahead split, so the usage for the Postlookahead needs to be
                // explicitly postlookahead.
                //
                postMemoryUsage = NdisSharedMemoryUsageReceivePostLookahead;
            }

            //
            // Allocate postlookahead memory
            //
            Status = AllocateRxSharedMemory(
                Adapter,
                QueueInfo->QueueId,
                postMemoryUsage,
                &QueueInfo->NumReceiveBuffers,
                PostLookaheadSize,
                LookaheadSize,
                &QueueInfo->PostLookaheadSharedMemoryList,
                &QueueInfo->PostLookaheadBlocks);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }

            QueueInfo->NumPostLookaheadBlocks = QueueInfo->NumReceiveBuffers;

            if(LookaheadSize)
            {
                //
                // Now that we have both lookahead and post-lookahead allocated we need to chain
                // a few of their fields together
                //
                ChainMemoryBlocks(
                    QueueInfo->NumReceiveBuffers,
                    QueueInfo->LookaheadBlocks,
                    QueueInfo->PostLookaheadBlocks);

            }

        }

        //
        // To finalize the allocation, we need to allocate the RCB & NBL blocks used for receive, and populated them
        // with the appropriate shared memory data (if non-default queue).
        //

        //
        // Ready to allocate the RCB blocks
        //
        Status = NICAllocRCBData(
                    Adapter,
                    QueueInfo->NumReceiveBuffers,
                    &QueueInfo->RcbMemoryBlock,
                    &QueueInfo->FreeRcbList,
                    &QueueInfo->FreeRcbListLock,
                    &QueueInfo->RecvNblPoolHandle
                    );

        if(QueueInfo->QueueId && Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Populate the RCB NBLs with the queue's shared memory.
            //
            AssociateSharedMemoryAndRCBs(
                QueueInfo->QueueId,
                QueueInfo->LookaheadBlocks,
                QueueInfo->PostLookaheadBlocks,
                QueueInfo->RcbMemoryBlock,
                QueueInfo->NumReceiveBuffers);
        }

        //
        // At this point, all that is left to do on receive is for the shared memory to get populated with real frame data (e.g. DMA), and set the frame length.
        // This avoids any expensive allocations on receive.
        //

    }while(FALSE);

    if(Status != NDIS_STATUS_SUCCESS && QUEUE_INITIALIZED(QueueInfo))
    {
        //
        // Cleanup any left over allocated receive resources from this function
        //
        FreeRxQueueReceiveMemory(Adapter, QueueInfo);
    }

    DEBUGP(MP_TRACE, "[%p] <--- AllocateRxQueueReceiveMemory. Status=0x%x\n", Adapter, Status);

    return Status;
}


NDIS_STATUS
CompleteAllocationRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY CompleteArray
    )
/*++
Routine Description:

    This routine will complete the process of queue allocation, allocating shared mmemory and
    flagging queues as complete. Once a Queue is complete, receive indications can begin on it.

    Runs at IRQL APC_PASSIVE

Arguments:

    Adapter               - Pointer to our Adapter
    CompleteArray         - Pointer to the complete allocation parameters

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_PARAMETERS CompleteParamArray = (PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_PARAMETERS)((PUCHAR)CompleteArray + CompleteArray->FirstElementOffset);
    ULONG index;
    LOCK_STATE_EX LockState;

    DEBUGP(MP_TRACE, "[%p] ---> CompleteAllocationRxQueue\n", Adapter);

    //
    // Iterate through each complete entry in the parameter array
    //
    for (index=0; index<CompleteArray->NumElements; index++)
    {
        PMP_ADAPTER_QUEUE CurrentQueue;

        if(CompleteParamArray[index].QueueId >= NIC_SUPPORTED_NUM_QUEUES)
        {
            //
            // Queue out of range
            //
            CompleteParamArray[index].CompletionStatus = NDIS_STATUS_INVALID_PARAMETER;
            continue;
        }

        //
        // Verify that the queue is initialized, and not already being completed (or complete)
        //

        //
        // Synchronize access to queue status
        //
        CurrentQueue = &VMQData->RxQueues[CompleteParamArray[index].QueueId];
        NdisAcquireRWLockWrite(CurrentQueue->QueueLock, &LockState, 0);
        if(!QUEUE_INITIALIZED(CurrentQueue) ||
                QUEUE_COMPLETING(CurrentQueue) ||
                QUEUE_FREEING(CurrentQueue))
        {
            //
            // Queue should have already been initialized, and should not be already completed or freed.
            //
            DEBUGP(MP_ERROR, "[%p] Queue %i does not have the correct status for completion (QueueInfoFlags: 0x%x).\n", Adapter, CompleteParamArray[index].QueueId, CurrentQueue->QueueInfoFlags);
            CompleteParamArray[index].CompletionStatus = NDIS_STATUS_INVALID_PARAMETER;
            NdisReleaseRWLock(CurrentQueue->QueueLock, &LockState);
            continue;
        }
        //
        // Mark that completion has started for the queue
        //
        QUEUE_SET_FLAG(CurrentQueue, fMPAQI_COMPLETION_STARTED);
        NdisReleaseRWLock(CurrentQueue->QueueLock, &LockState);

        Status = AllocateRxQueueReceiveMemory(Adapter, CurrentQueue);
        CompleteParamArray[index].CompletionStatus = Status;
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] Failed while allocating receive memory for Queue %i\n", Adapter, CompleteParamArray[index].QueueId);
            QUEUE_CLEAR_FLAG(CurrentQueue,fMPAQI_COMPLETION_STARTED);
            continue;
        }

        //
        // Completion done. Update queue status in a synchronized block.
        //
        NdisAcquireRWLockWrite(CurrentQueue->QueueLock, &LockState, 0);
        QUEUE_SET_FLAG(CurrentQueue,fMPAQI_COMPLETION_FINISHED);
        NdisReleaseRWLock(CurrentQueue->QueueLock, &LockState);
    }

    DEBUGP(MP_TRACE, "[%p] <--- CompleteAllocationRxQueue Status 0x%08x\n", Adapter, Status);

    return Status;
}

NDIS_STATUS
UpdateRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams
)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_RECEIVE_QUEUE_ID  QueueId = QueueParams->QueueId;
    PMP_ADAPTER_QUEUE QueueInfo;

    DEBUGP(MP_TRACE, "[%p] ---> UpdateRxQueue\n", Adapter);

    do
    {
        if(QueueId >= NIC_SUPPORTED_NUM_QUEUES)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;

        }

        QueueInfo = &Adapter->VMQData.RxQueues[QueueId];
        if(QueueParams->Flags&NDIS_RECEIVE_QUEUE_PARAMETERS_CHANGE_MASK)
        {
            if (QueueParams->Flags & NDIS_RECEIVE_QUEUE_PARAMETERS_PROCESSOR_AFFINITY_CHANGED)
            {
                //
                // Change the owner DPC
                //
                PMP_ADAPTER_RECEIVE_DPC NewDpc;
                PROCESSOR_NUMBER ProcNumber = {0};

                if(!ValidGroupAffinity(&QueueParams->ProcessorAffinity))
                {
                    DEBUGP(MP_ERROR, "[%p] Requested group affinity failed validation\n", Adapter);
                    Status = NDIS_STATUS_INVALID_PARAMETER;
                    break;
                }

                ProcNumber.Group = QueueParams->ProcessorAffinity.Group;
                ProcNumber.Number = RtlFindLeastSignificantBit((ULONGLONG)QueueParams->ProcessorAffinity.Mask);
                ASSERT(ProcNumber.Number != -1);

                DEBUGP(MP_INFO, "[%p] Updating Queue Affinity Group: 0x%x, Number: 0x%x\n", Adapter, ProcNumber.Group, ProcNumber.Number);

                //
                // Get the new DPC with different affinity (if one already exists, will reuse it)
                //
                NewDpc = NICAllocReceiveDpc(Adapter, ProcNumber.Number, ProcNumber.Group, QueueId);
                if(!NewDpc)
                {
                    //
                    // Could not allocate DPC
                    //
                    Status = NDIS_STATUS_RESOURCES;
                    DEBUGP(MP_ERROR, "[%p] Could not allocate receive DPC for Queue %i, will keep original DPC\n", Adapter, QueueId);
                    break;
                }

                if(NewDpc != QueueInfo->ReceiveDpc)
                {
                    //
                    // This is a new DPC for the queue, update the existing owner DPC and replace
                    //
                    NICReceiveDpcRemoveOwnership(QueueInfo->ReceiveDpc, QueueId);

                    //
                    // Store new DPC
                    //
                    QueueInfo->ReceiveDpc = NewDpc;

                    //
                    // Check whether there are any pending receives on the queue, if there are, then schedule the
                    // new DPC to make sure they are not lost
                    //
                    if(!IsListEmpty(&Adapter->ReceiveBlock[QueueId].ReceiveList))
                    {
                        DEBUGP(MP_INFO, "[%p] Receive Block %i: Receives were pending, queued new DPC.\n", Adapter, QueueId);
                        KeInsertQueueDpc(&QueueInfo->ReceiveDpc->Dpc, QueueInfo->ReceiveDpc, NULL);
                    }
                }

            }

            if (QueueParams->Flags & NDIS_RECEIVE_QUEUE_PARAMETERS_FLAGS_CHANGED)
            {
                ULONG Flags = QueueParams->Flags & ~NDIS_RECEIVE_QUEUE_PARAMETERS_CHANGE_MASK;
                DEBUGP(MP_INFO, "[%p] Updating Queue flags: 0x%x\n", Adapter, Flags);
                if(Flags&NDIS_RECEIVE_QUEUE_PARAMETERS_LOOKAHEAD_SPLIT_REQUIRED
                    && !LOOKAHEAD_SPLIT_REQUIRED(QueueInfo)
                    && QUEUE_COMPLETE(QueueInfo))
                {
                    //
                    // Our implementation does not support enabling lookahead after the queue is complete
                    // since our complete handler would not have allocated the lookahead memory.
                    //
                    DEBUGP(MP_ERROR, "[%p] Queue was already completed without lookahead split. Did not update flags. \n", Adapter);
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
                else
                {
                    QueueInfo->NdisFlags = Flags;
                }
            }

            if (QueueParams->Flags & NDIS_RECEIVE_QUEUE_PARAMETERS_SUGGESTED_RECV_BUFFER_NUMBERS_CHANGED)
            {
                DEBUGP(MP_INFO, "[%p] Updating Queue number of suggested receive buffers: %i\n", Adapter, QueueParams->NumSuggestedReceiveBuffers);
                QueueInfo->NumSuggestedReceiveBuffers = QueueParams->NumSuggestedReceiveBuffers;
            }
        }
    }while(FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- UpdateRxQueue Status 0x%08x\n", Adapter, Status);
    return Status;
}


NDIS_STATUS
FreeAdapterQueueInfo(
    _In_ PMP_ADAPTER Adapter,
    USHORT QueueId,
    _In_opt_ PNDIS_OID_REQUEST   NdisSetRequest)
/*++
Routine Description:

    This routine will clean up the memory for the specified queue, and reset it to uninitialized. If the
    queue still has active references on it (e.g. - still in use), the function does not clean up the queue
    and instead returns NDIS_STATUS_PENDING. Callers should handle attempting the free at a later time.

    Runs at IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                 - Pointer to our adapter
    QueueId                 - Queue to free
    NdisSetRequest      - The OID for the free request (NULL if the call is not due to an OID request)

Return Value:

    Void

--*/
{
    LOCK_STATE_EX QueueLockState;
    PMP_ADAPTER_QUEUE QueueInfo = &Adapter->VMQData.RxQueues[QueueId];
    LONG ReferenceCount = 0;
    BOOLEAN DmaInProgress;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PLIST_ENTRY Entry;

    do
    {
        //
        // Check (and possibly update) queue status in synchronized block
        //
        NdisAcquireRWLockWrite(QueueInfo->QueueLock, &QueueLockState, 0);
        if(!QueueInfo->QueueInfoFlags)
        {
            //
            // Queue has already been freed, no need to do anything.
            //
            Status = NDIS_STATUS_SUCCESS;
            NdisReleaseRWLock(QueueInfo->QueueLock, &QueueLockState);
            break;
        }

        //
        // Reset the flags to "freeing". This way we will not accumulate any more receives on the queue.
        //
        QUEUE_SET_FLAG(QueueInfo,fMPAQI_FREEING);

        //
        // Make sure the freeing state set operation happens before the DMA in progress check
        //
        KeMemoryBarrier();

        //
        // Check whether the queue has a DMA in progress. This value check has to occur *after* we've updated
        // the queue state to freeing. The receive code depends on this ordering of operations to avoid taking locks
        // on its "DMA" path (see CopyFrameToRxQueue).
        //
        DmaInProgress = QUEUE_DMA_IN_PROGRESS(QueueInfo);
        if(!DmaInProgress)
        {
            //
            // There are no DMA's in progress. Update the flags of the queue to DMA stopped to avoid pending receive code
            // from indicating DMA stopped state later. The indication to NDIS of this change happens outside of our synchronized
            // block to avoid holding the lock for too long.
            //
            QUEUE_SET_FLAG(QueueInfo,fMPAQI_DMA_STOPPED);
        }

        //
        // Get the reference count
        //
        ReferenceCount = RECEIVE_BLOCK_REFERENCE_COUNT(Adapter, QueueId);
        if(ReferenceCount != 0)
        {
            //
            // Still have outstanding usage of the queue, so we must pend the free request.
            //
            if(NdisSetRequest)
            {
                //
                // Store the OID so that we can complete it later
                //
                SetPendingRxQueueFree(Adapter, NdisSetRequest);
            }
            else
            {
                //
                // The queue free should only pend on OID request. If this free call came from halt or other operation,
                // the queue should have been ready to free already, so check that we have a pending request.
                //
                ASSERT(Adapter->PendingRequest);
            }

        }

        NdisReleaseRWLock(QueueInfo->QueueLock, &QueueLockState);

        if(!DmaInProgress)
        {
            //
            // There were no DMA's in progress when we updated that state to freeing, and no more DMA's will occur, so it's safe to indicate that queue DMA is stopped.
            // Any pending receives that have already completed their copies will still be indicated.
            //
            IndicateRxQueue(
               Adapter,
               QueueId,
               NdisReceiveQueueOperationalStateDmaStopped);
        }

        if(ReferenceCount != 0)
        {
            //
            // Bail out of the free path. Once the ref count reaches zero on the queue and there are no in-progress DMA's, the queue will be freed and the OID will complete.
            //
            DEBUGP(MP_LOUD, "[%p] Queue %i has pending references (RefCount %i), returning NDIS_STATUS_PENDING for the free request.\n", Adapter, QueueId, ReferenceCount);
            Status = NDIS_STATUS_PENDING;
            break;
        }

        DEBUGP(MP_LOUD, "[%p] Queue %i has no outstanding references, queue memory has been freed.\n", Adapter, QueueId);

        NICReceiveDpcRemoveOwnership(QueueInfo->ReceiveDpc, QueueId);
        QueueInfo->ReceiveDpc = NULL;

        //
        // Free the RCB & NBL data
        //
        if(QueueInfo->FreeRcbList.Flink)
        {
            while (NULL != (Entry = NdisInterlockedRemoveHeadList(
                    &QueueInfo->FreeRcbList,
                    &QueueInfo->FreeRcbListLock)))
            {
                PRCB Rcb = CONTAINING_RECORD(Entry, RCB, RcbLink);
                NdisFreeNetBufferList(Rcb->Nbl);
            }
            NdisFreeSpinLock(&QueueInfo->FreeRcbListLock);
        }

        if (QueueInfo->RecvNblPoolHandle)
        {
            NdisFreeNetBufferListPool(QueueInfo->RecvNblPoolHandle);
            QueueInfo->RecvNblPoolHandle = NULL;
        }

        if (QueueInfo->RcbMemoryBlock)
        {
            NdisFreeMemory(
                    QueueInfo->RcbMemoryBlock,
                    sizeof(RCB)*QueueInfo->NumReceiveBuffers,
                    0);
            QueueInfo->RcbMemoryBlock = NULL;
        }

        if(QUEUE_COMPLETE(QueueInfo))
        {
            FreeRxQueueReceiveMemory(
                    Adapter,
                    QueueInfo);
        }

        //
        // Completed free, clear flags altogether
        //
        QueueInfo->QueueInfoFlags = 0;

        if(Adapter->PendingRequest)
        {
            PNDIS_OID_REQUEST Request = Adapter->PendingRequest;
            Adapter->PendingRequest = NULL;
            NdisMOidRequestComplete(Adapter->AdapterHandle,
                            Request,
                            Status);
        }

    } while(FALSE);

    return Status;
}

NDIS_STATUS
FreeRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_FREE_PARAMETERS QueueFreeParams,
    _In_opt_ PNDIS_OID_REQUEST   NdisSetRequest
    )
/*++
Routine Description:

    This routine will clean up the memory for the specified queue. Queue should already been indicated as DMA stopped
    using IndicateRxQueue. Callers should handle attempting the free at a later time if NDIS_STATUS_PENDING is returned.

    Runs at IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                 - Pointer to our adapter
    QueueFreeParams         - Pointer to the complete allocation parameters
    NdisSetRequest      - The OID for the free request (NULL if the call is not due to an OID request)

Return Value:

    NDIS_STATUS

--*/

{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(MP_TRACE, "[%p] ---> FreeRxQueue\n", Adapter);

    //
    // Verify within valid range
    //
    if(QueueFreeParams->QueueId>=NIC_SUPPORTED_NUM_QUEUES)
    {
        Status = NDIS_STATUS_INVALID_PARAMETER;
    }
    else
    {
        //
        // Attempt to free the queue (pending will be returned if queue still in use)
        //
        Status = FreeAdapterQueueInfo(Adapter, (USHORT)QueueFreeParams->QueueId, NdisSetRequest);
    }

    DEBUGP(MP_TRACE, "[%p] <--- FreeRxQueue Status 0x%08x\n", Adapter, Status);

    return Status;
}

_Use_decl_annotations_
VOID
FreeRxQueuesWorkItem(
    PVOID  WorkItemContext,
    NDIS_HANDLE  NdisIoWorkItemHandle
    )
/*++
Routine Description:

    This routine retrieves the pending free request (there can only be one at a time), and frees the queue. The work item is used
    when we pend a queue free OID (because the reference count is non-zero), and when the reference count is finally reduced to
    zero we are not at PASSIVE_LEVEL.

    Runs at IRQL = PASSIVE_LEVEL

Arguments:

    WorkItemContext       - Context for work item (PMP_ADAPTER)
    NdisIoWorkItemHandle  - Work item handle

Return Value:

    None

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER Adapter;
    PNDIS_OID_REQUEST Request;

	Adapter = MP_ADAPTER_FROM_CONTEXT(WorkItemContext);
	ASSERT(Adapter != NULL);
	_Analysis_assume_(Adapter != NULL);

    DEBUGP(MP_TRACE, "[%p] ---> FreeRxQueuesWorkItem\n", Adapter);

	Request = Adapter->PendingRequest;
    if(Request != NULL)
    {
        switch(Request->DATA.SET_INFORMATION.Oid)
        {
            case OID_RECEIVE_FILTER_FREE_QUEUE:
                Status = FreeRxQueue(
                            Adapter,
                            (PNDIS_RECEIVE_QUEUE_FREE_PARAMETERS)Request->DATA.SET_INFORMATION.InformationBuffer,
                            NULL //OID should already be held in pending OID member of adapter
                            );
                //
                // Should only be called once reference count has reached zero, it should succeed
                //
                ASSERT(Status == NDIS_STATUS_SUCCESS);
                break;
            default:
                ASSERT(!"Unsupported Set OID added to pending list");
                Status = NDIS_STATUS_INVALID_PARAMETER;
                break;
        }

    }

    NdisFreeIoWorkItem(NdisIoWorkItemHandle);

    DEBUGP(MP_TRACE, "[%p] <-- FreeRxQueuesWorkItem. Status=0x%x\n", Adapter, Status);
}

VOID
FreeVMQData(
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will clean up the memory for all the queues in the passed in VMQData. It should only be called
    when all pending receives have stopped (e.g. - miniport halt).

    Runs at IRQL = PASSIVE_LEVEL

Arguments:

    Adapter     - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    USHORT index;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    for(index=0; index<NIC_SUPPORTED_NUM_QUEUES; index++)
    {
        if(VMQData->RxQueues[index].QueueLock != NULL)
        {
            #pragma warning( suppress: 28931 ) // unused assignment of variable Status OK in Fre build
            Status = FreeAdapterQueueInfo(Adapter, index, NULL);
            ASSERT(Status == NDIS_STATUS_SUCCESS);
            NdisFreeRWLock(VMQData->RxQueues[index].QueueLock);
        }
    }

}

VOID
IndicateRxQueue(
           _In_ PMP_ADAPTER Adapter,
           NDIS_RECEIVE_QUEUE_ID QueueId,
           NDIS_RECEIVE_QUEUE_OPERATIONAL_STATE State)
/*++
Routine Description:

    This routine will indicate to NDIS the state of the queue.

Arguments:

    Adapter                - Pointer to our adapter
    QueueId                - Queue to indicate
    State                  - Current state of Queue

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS_INDICATION Status;
    NDIS_RECEIVE_QUEUE_STATE QueueState;

    DEBUGP(MP_TRACE, "[%p] ---> IndicateRxQueue ID: %i, State: 0x%x\n", Adapter, QueueId, State);

    NdisZeroMemory(&Status, sizeof(Status));
    NdisZeroMemory(&QueueState, sizeof(QueueState));

    QueueState.Header.Revision = NDIS_RECEIVE_QUEUE_STATE_REVISION_1;
    QueueState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    QueueState.Header.Size = NDIS_SIZEOF_NDIS_RECEIVE_QUEUE_STATE_REVISION_1;
    QueueState.QueueId = QueueId;
    QueueState.QueueState = State;

    Status.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    Status.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
    Status.Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    Status.StatusCode = NDIS_STATUS_RECEIVE_QUEUE_STATE;
    Status.StatusBuffer = &QueueState;
    Status.StatusBufferSize = QueueState.Header.Size;

    NdisMIndicateStatusEx(
        Adapter->AdapterHandle,
        &Status);

    DEBUGP(MP_TRACE, "[%p] <--- IndicateRxQueue\n", Adapter);

}

BOOLEAN
IsSupportedRxFilter(
    _In_ PMP_ADAPTER Adapter,
    _In_ PNDIS_RECEIVE_FILTER_PARAMETERS FilterParams,
    _In_ PNDIS_RECEIVE_FILTER_FIELD_PARAMETERS FilterCriteria)
/*++
Routine Description:

    This routine will verify whether the filter is well formed and supported by this adapter.

Arguments:

    Adapter                - Pointer to our adapter
    FilterParams           - Filter Parameters
    FilterCriteria         - Filter Field Parameters

Return Value:

    TRUE - filter is supported
    FALSE - filter is not not

--*/
{

    BOOLEAN Supported = TRUE;
    ULONG index;
    BOOLEAN VlanFilterPresent = FALSE, VlanUntaggedOrZeroFlagPresent = FALSE;

    DEBUGP(MP_TRACE, "[%p] ---> IsSupportedRxFilter. FilterParams: QueueId: %i, FilterId: %i, Flags: 0x%x\n",
             Adapter, FilterParams->QueueId, FilterParams->FilterId, FilterParams->Flags);

    //
    // Check all criteria for the filter
    //
    for(index = 0; index < FilterParams->FieldParametersArrayNumElements && Supported; index++)
    {
        DEBUGP(MP_TRACE, "[%p] FilterCriteris[%i]: FrameHeader: %i, ReceiveFilterTest: %i, MacHeaderField: %i\n",
                 Adapter,
                 index, FilterCriteria[index].FrameHeader, FilterCriteria[index].ReceiveFilterTest,
                 FilterCriteria[index].HeaderField.MacHeaderField);

        //
        // Verify frame header
        //
        if(FilterCriteria[index].FrameHeader != NdisFrameHeaderMac)
        {
            DEBUGP(MP_ERROR, "[%p] Unknown filter FrameHeader type %x\n", Adapter, FilterCriteria[index].FrameHeader);
            Supported = FALSE;
            break;
        }

        //
        // Verify that filter test type is supported
        //
        switch(FilterCriteria[index].ReceiveFilterTest)
        {
            case NdisReceiveFilterTestEqual:
                Supported = TRUE;
                break;
            default:
                Supported=FALSE;
                DEBUGP(MP_ERROR, "[%p] ReceiveFilterTest 0x%x not supported.\n", Adapter, FilterCriteria[index].ReceiveFilterTest);
                break;
        };

        if(!Supported)
        {
            break;
        }

        //
        // Verify header field
        //
        switch(FilterCriteria[index].HeaderField.MacHeaderField)
        {
            case NdisMacHeaderFieldDestinationAddress:
                VlanUntaggedOrZeroFlagPresent = FilterCriteria->Flags&NDIS_RECEIVE_FILTER_FIELD_MAC_HEADER_VLAN_UNTAGGED_OR_ZERO;
                Supported = TRUE;
                break;
            case NdisMacHeaderFieldVlanId:
                VlanFilterPresent = TRUE;
                Supported = TRUE;
                break;
            default:
                Supported = FALSE;
                DEBUGP(MP_ERROR, "[%p] MacHeaderField 0x%x not supported.\n", Adapter, FilterCriteria[index].HeaderField.MacHeaderField);
                break;
        };
    }

    if(Supported)
    {
        //
        // NDIS_RECEIVE_FILTER_FIELD_MAC_HEADER_VLAN_UNTAGGED_OR_ZERO should not be set if VLAN filter is specified,
        // and should be set if no VLAN filter is specified.
        //
        if(VlanFilterPresent == VlanUntaggedOrZeroFlagPresent)
        {
            //
            // Both are specified, this is not correct.
            //
            DEBUGP(MP_ERROR, "[%p] Invalid VMQ filter configuration. NDIS_RECEIVE_FILTER_FIELD_MAC_HEADER_VLAN_UNTAGGED_OR_ZERO (%s), Vlan ID filter present (%s). Filter not accepted.\n",
                    Adapter,
                    VlanFilterPresent?"TRUE":"FALSE",
                    VlanUntaggedOrZeroFlagPresent?"TRUE":"FALSE");
            Supported = FALSE;
        }
        else if(!VLAN_FILTER_ENABLED(Adapter) && VlanFilterPresent)
        {
            //
            // VLAN based filtering not configured on the adapter
            //
            DEBUGP(MP_ERROR, "[%p] Invalid VMQ filter configuration. *VMQVlanFiltering is not set, and Vlan ID filter was specified.\n", Adapter);
            Supported = FALSE;
        }
    }

    DEBUGP(MP_TRACE, "[%p] <--- IsSupportedRxFilter %s\n", Adapter, Supported?"TRUE":"FALSE");
    return Supported;
}

NDIS_STATUS
SetRxFilter(
        _Inout_ struct _MP_ADAPTER *Adapter,
        _In_ PNDIS_RECEIVE_FILTER_PARAMETERS FilterParams
        )
/*++
Routine Description:

    This routine will assign a filter to the specified queue, after verifying that the filter is valid.

Arguments:

    Adapter                - Pointer to our adapter
    FilterParams           - Filter data

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    PNDIS_RECEIVE_FILTER_FIELD_PARAMETERS FilterCriteria = (PNDIS_RECEIVE_FILTER_FIELD_PARAMETERS)((PUCHAR)FilterParams + FilterParams->FieldParametersArrayOffset);
    UINT FilterIndex = MP_ADAPTER_FILTER_INDEX(FilterParams->FilterId);
    UINT CriteriaIndex;

    DEBUGP(MP_TRACE, "[%p] ---> SetRxFilter\n", Adapter);

    do
    {
        //
        // Verify the requested queue is within supported range and is initialized
        //
        if(FilterParams->QueueId>=NIC_SUPPORTED_NUM_QUEUES
            ||
            FilterIndex>=NIC_MAX_HEADER_FILTERS
            ||
            !QUEUE_INITIALIZED(&VMQData->RxQueues[FilterParams->QueueId]))
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Verify that the filter itself is valid, and that it hasn't already been set
        //
        if(!IsSupportedRxFilter(Adapter, FilterParams, FilterCriteria)
            ||
            VMQData->RxFilters[FilterIndex].Valid)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Make sure filter data is all zero at the outset
        //
        NdisZeroMemory(&VMQData->RxFilters[FilterIndex], sizeof(MP_ADAPTER_FILTER));

        //
        // Copy filter information. IsSupportedRxFilter will have already verified that this filter definition
        // only contains MAC and VLAN ID definitions, and equals match criteria.
        //
        for(CriteriaIndex = 0; CriteriaIndex < FilterParams->FieldParametersArrayNumElements; CriteriaIndex++)
        {
            if(FilterCriteria[CriteriaIndex].HeaderField.MacHeaderField == NdisMacHeaderFieldDestinationAddress)
            {
                //
                // MAC field
                //
                memcpy(&VMQData->RxFilters[FilterIndex].MacAddress, FilterCriteria[CriteriaIndex].FieldValue.FieldByteArrayValue, NIC_MACADDR_SIZE);
                DEBUGP(MP_TRACE, "[%p] Filter Destination: %02x-%02x-%02x-%02x\n",
                    Adapter,
                    VMQData->RxFilters[FilterIndex].MacAddress[0], VMQData->RxFilters[FilterIndex].MacAddress[1],
                    VMQData->RxFilters[FilterIndex].MacAddress[2], VMQData->RxFilters[FilterIndex].MacAddress[3]);
            }
            else
            {
                //
                // Vlan ID field
                //
                VMQData->RxFilters[FilterIndex].VlanId = FilterCriteria[CriteriaIndex].FieldValue.FieldShortValue;
                DEBUGP(MP_TRACE, "[%p] Filter VlanId: %i\n", Adapter, VMQData->RxFilters[FilterIndex].VlanId);
            }

            if(FilterCriteria[CriteriaIndex].Flags&NDIS_RECEIVE_FILTER_FIELD_MAC_HEADER_VLAN_UNTAGGED_OR_ZERO)
            {
                VMQData->RxFilters[FilterIndex].VlanUntaggedOrZero = TRUE;
                DEBUGP(MP_TRACE, "[%p] Filter VlanUntaggedOrZero is TRUE\n", Adapter);
            }
        }

        //
        // Store the QueueId
        //
        VMQData->RxFilters[FilterIndex].QueueId = (USHORT)FilterParams->QueueId;

        //
        // Set to valid
        //
        VMQData->RxFilters[FilterIndex].Valid = TRUE;


    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- SetRxFilter Status 0x%08x\n", Adapter, Status);

    return Status;
}

NDIS_STATUS
ClearRxFilter(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS FilterParams
)
/*++
Routine Description:

    This routine will remove the specified filter from a particular queue.

Arguments:

    Adapter                - Pointer to our adapter
    FilterParams           - Parameters for freeing filter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER_VMQ_DATA VMQData = &Adapter->VMQData;
    UINT FilterIndex = MP_ADAPTER_FILTER_INDEX(FilterParams->FilterId);

    DEBUGP(MP_TRACE, "[%p] ---> ClearRxFilter. FilterParams: QueueId: %i, FilterId: %i\n",
             Adapter, FilterParams->QueueId, FilterParams->FilterId);

    do
    {

        //
        // Make sure the filter id is within the valid range
        //
        if(FilterParams->QueueId>=NIC_SUPPORTED_NUM_QUEUES
            ||
            FilterIndex>=NIC_MAX_HEADER_FILTERS)
        {
            DEBUGP(MP_ERROR, "[%p] Filter Clear request out of valid range. QueueId: %i, FilterId: %i\n",
                                 Adapter, FilterParams->QueueId, FilterParams->FilterId);
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        if(!VMQData->RxFilters[FilterIndex].Valid)
        {
            //
            // The filter is not valid.
            //
            DEBUGP(MP_ERROR, "[%p] Filter was not set. QueueId: %i, FilterId: %i\n",
                     Adapter, FilterParams->QueueId, FilterParams->FilterId);
            Status = NDIS_STATUS_INVALID_PARAMETER;
        }
        else
        {
            //
            // Reset the filter to invalid
            //
            VMQData->RxFilters[FilterIndex].Valid = FALSE;
        }

    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- ClearRxFilter Status 0x%08x\n", Adapter, Status);

    return Status;
}

BOOLEAN
MatchRxFilter(
    _In_ PMP_ADAPTER Adapter,
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR DestAddress,
    _In_ PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_ PMP_ADAPTER_FILTER Filter)
/*++
Routine Description:

    This routine will check whether a particular filter matches the specified destination address
    and VLAN ID.

Arguments:

    DestAddress            - Destination MAC address
    Nbl1QInfo              - VLAN information
    Filter                 - Filter to check against

Return Value:

    TRUE  - Data matches filter
    FALSE - Data did not match filter

--*/
{
    BOOLEAN MatchResult;

    DEBUGP(MP_TRACE, "[%p] ---> MatchRxFilter\n", Adapter);


    DEBUGP(MP_TRACE, "[%p] NBL Destination: %02x-%02x-%02x-%02x, Filter Destination: %02x-%02x-%02x-%02x\n",
                    Adapter,
                    DestAddress[0],DestAddress[1],DestAddress[2],DestAddress[3],
                    Filter->MacAddress[0], Filter->MacAddress[1],
                    Filter->MacAddress[2], Filter->MacAddress[3]);

    //
    // Match MAC address
    //
    MatchResult = NIC_ADDR_EQUAL(Filter->MacAddress, DestAddress);


    if(MatchResult)
    {
        if(Filter->VlanUntaggedOrZero)
        {
            //
            // VLAN ID should be zero or untagged for match
            //
            DEBUGP(MP_TRACE, "[%p] Destination Matched. Making sure VLAN is untagged or zero.\n", Adapter);
            if(Nbl1QInfo->Value)
            {
                MatchResult = (Nbl1QInfo->Value == 0 || Nbl1QInfo->TagHeader.VlanId == 0);
                if(Nbl1QInfo->Value)
                {
                    DEBUGP(MP_TRACE, "[%p] NBL Vlan ID: %i\n", Adapter, Nbl1QInfo->TagHeader.VlanId);
                }
            }
        }
        else
        {
            //
            // Match VLAN ID
            //
            if(Nbl1QInfo->Value)
            {
                MatchResult = (Nbl1QInfo->TagHeader.VlanId == Filter->VlanId);
                DEBUGP(MP_TRACE, "[%p] NBL Vlan ID: %i, Filter Vlan ID: %i\n",
                    Adapter,
                    Nbl1QInfo->TagHeader.VlanId, Filter->VlanId);
            }
            else
            {
                MatchResult = FALSE;
                DEBUGP(MP_TRACE, "[%p] NBL without VLAN TAG. Filter Vlan ID: %i\n",
                        Adapter,
                        Filter->VlanId);
            }
        }
    }

    DEBUGP(MP_TRACE, "[%p] <--- MatchRxFilter MatchResult %s\n", Adapter, MatchResult?"TRUE":"FALSE");

    return MatchResult;
}

BOOLEAN
FindRxQueueRecipient(
        _In_ struct _MP_ADAPTER *Adapter,
        _In_ struct _FRAME *Frame,
        _In_ PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
        _Out_ USHORT *QueueId)
/*++
Routine Description:

    This routine will return the matching QueueId and FilterId for a particular Frame and its 802.1Q data.

Arguments:

    Adapter                - Pointer to our adapter
    Frame                  - Receive frame
    Nbl1QInfo              - Receive VLAN information
    QueueId                - Is assigned the matching QueueId (NDIS_DEFAULT_RECEIVE_QUEUE_GROUP_ID if no queue matches)

Return Value:

    TRUE  - Matched a specific queue (even default queue, if filters present on it)
    FALSE - No queue matched

--*/
{
    USHORT index;
    BOOLEAN Matched=FALSE;

    DEBUGP(MP_TRACE, "[%p] ---> FindRxQueueRecipient\n", Adapter);

    //
    // Iterate through all the filters and check whether a match exists
    //
    *QueueId = NDIS_DEFAULT_RECEIVE_QUEUE_ID;
    for(index = 0; index < NIC_MAX_HEADER_FILTERS; index++)
    {
        //
        // Only attempt to match a filter when it is valid and its queue has been completed
        //
        if(Adapter->VMQData.RxFilters[index].Valid
            &&
            QUEUE_COMPLETE(&Adapter->VMQData.RxQueues[Adapter->VMQData.RxFilters[index].QueueId])
            &&
            MatchRxFilter(Adapter, ((PNIC_FRAME_HEADER)Frame->Data)->DestAddress, Nbl1QInfo, &Adapter->VMQData.RxFilters[index]))
        {
            DEBUGP(MP_TRACE, "[%p] Match Found.\n", Adapter);
            Matched = TRUE;
            *QueueId = Adapter->VMQData.RxFilters[index].QueueId;
            break;
        }
    }

    if(!Matched)
    {
        UCHAR         DestAddress[NIC_MACADDR_SIZE];
        ULONG         FrameType;
        //
        // No queue matched, if the packet is destinaed specifically for this NIC we indicate it on the default queue
        //
        GET_DESTINATION_OF_FRAME(DestAddress, Frame->Data);
        FrameType = NICGetFrameTypeFromDestination(DestAddress);
        Matched = HWIsFrameAcceptedByPacketFilter(Adapter, DestAddress, FrameType);
    }

    DEBUGP(MP_TRACE, "[%p] <-- FindRxQueueRecipient QueueId: %i, Matched: %i\n", Adapter, *QueueId, Matched);

    return Matched;

}

BOOLEAN
AcquireRxQueueReference(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) USHORT QueueId,
    BOOLEAN RequireComplete
    )
/*++
Routine Description:

    This routine will increase the reference count on a queue, as long as it is still valid.

Arguments:

    Adapter                - Pointer to our adapter
    QueueId                - The Queue to take reference
    RequireComplete        - If TRUE, the queue needs to be Completed to be considered valid, otherwise
                             Initialized is enough

Return Value:

    TRUE  - Queue was valid, ref count increased
    FALSE - Queue was not valid, ref count not changed

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PMP_ADAPTER_QUEUE QueueInfo = &Adapter->VMQData.RxQueues[QueueId];
    LOCK_STATE_EX LockState;

    DEBUGP(MP_TRACE, "[%p] ---> AcquireRxQueueReference. QueueId: %i\n", Adapter, QueueId);
    ASSERT(QueueId <  NIC_SUPPORTED_NUM_QUEUES);

    //
    // Acquire read lock on queue to verify that it is valid
    //
    NdisAcquireRWLockRead(QueueInfo->QueueLock, &LockState, 0);
    do
    {
        //
        // Verify queue is initialized and optionally completed before taking reference
        //
        if(!QUEUE_INITIALIZED(QueueInfo)
            ||
            (RequireComplete && !QUEUE_COMPLETE(QueueInfo)))
        {
             DEBUGP(MP_LOUD, "[%p] Queue not in correct state to take reference. QueueInfoFlags: 0x%x.\n", Adapter, QueueInfo->QueueInfoFlags);
             break;
        }

        //
        // Queue is ready for use. Increment the reference count for the receive block.
        //
        Status = NICReferenceReceiveBlock(Adapter, QueueInfo->QueueId);

    } while(FALSE);

    NdisReleaseRWLock(QueueInfo->QueueLock, &LockState);

    DEBUGP(MP_TRACE, "[%p] <--- AcquireRxQueueReference.\n", Adapter);

    return Status == NDIS_STATUS_SUCCESS;
}


VOID
ReleaseRxQueueReference(
        _Inout_ struct _MP_ADAPTER *Adapter,
        _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) USHORT QueueId)
/*++
Routine Description:

    This routine will decrease the reference count on a queue

Arguments:

    Adapter                - Pointer to our adapter
    QueueId                - The Queue to take reference

Return Value:

    None

--*/
{
    ULONG RefCount = 0;
    PMP_ADAPTER_QUEUE QueueInfo;
    LOCK_STATE_EX LockState;
    BOOLEAN PerformFree;

    DEBUGP(MP_TRACE, "[%p] ---> ReleaseRxQueueReference. QueueId: %i\n", Adapter, QueueId);
    ASSERT(QueueId <  NIC_SUPPORTED_NUM_QUEUES);

    QueueInfo = &Adapter->VMQData.RxQueues[QueueId];

    //
    // Acquire read lock on queue to synchronize our check whether the queue free was pended because reference count was not at zero
    //
    NdisAcquireRWLockRead(QueueInfo->QueueLock, &LockState, 0);

    //
    // Reduce the reference count
    //
    NICDereferenceReceiveBlock(Adapter, QueueId, &RefCount);

    //
    // If the reference count is zero, and the queue is tagged as freeing (which was pended earlier)
    // then execute the free.
    //
    PerformFree = (RefCount == 0 && QUEUE_FREEING(QueueInfo));

    //
    // Done dereferencing and checking queue state
    //
    NdisReleaseRWLock(QueueInfo->QueueLock, &LockState);

    if(PerformFree)
    {
        DEBUGP(MP_LOUD, "[%p] RefCount has reached zero and the queue is marked for free. Freeing Queue %i.\n", Adapter, QueueId);
        //
        // We've already gotten the call to free this queue, but references were still
        // active on it. Call the free function now that we're at zero references.
        //
        if(NDIS_CURRENT_IRQL() < DISPATCH_LEVEL)
        {
            NDIS_STATUS Status = FreeAdapterQueueInfo(Adapter, QueueId, NULL);
            ASSERT(Status == NDIS_STATUS_SUCCESS);
            UNREFERENCED_PARAMETER(Status);
        }
        else
        {
            NDIS_HANDLE WorkItemHandle = NdisAllocateIoWorkItem(Adapter->AdapterHandle);
            DEBUGP(MP_LOUD, "[%p] Currently at DISPATCH, queueing work item to free Queue %i.\n", Adapter, QueueId);
            if(WorkItemHandle)
            {
                NdisQueueIoWorkItem(WorkItemHandle, FreeRxQueuesWorkItem, Adapter);
            }
            else
            {
                DEBUGP(MP_ERROR, "[%p] Could not allocate IoWorkItem to free queue.\n", Adapter);
            }
        }
    }

    DEBUGP(MP_TRACE, "[%p] <--- ReleaseRxQueueReference.\n", Adapter);

}

VOID
SetPendingRxQueueFree(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_OID_REQUEST NdisSetRequest
    )
/*++
Routine Description:

    This routine stores the pending queue free OID request, so that it can be completed
    at a later time.

Arguments:

    Adapter                - Pointer to our adapter
    QueueId                - The Queue to take reference

Return Value:

    None

--*/
{
    DEBUGP(MP_TRACE, "[%p] ---> SetPendingRxQueueFree\n", Adapter);

    ASSERT(!Adapter->PendingRequest);
    Adapter->PendingRequest = NdisSetRequest;

    DEBUGP(MP_TRACE, "[%p] <--- SetPendingRxQueueFree\n", Adapter);
}

VOID
GetRcbForRxQueue(
    _In_  struct _MP_ADAPTER *Adapter,
    _In_  struct _FRAME *Frame,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _Outptr_result_maybenull_ PRCB *Rcb)
/*++
Routine Description:

    This routine returns an RCB for the passed in Frame & VLAN info. The RCB will originate from the
    matching queue.

Arguments:

    Adapter                - Pointer to our adapter
    Frame                  - Data Frame
    Nbl1QInfo              - VLAN information for Frame

Return Value:

    RCB pointer if matched
    NULL if not matched

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    USHORT QueueId = 0;

    DEBUGP(MP_TRACE, "[%p] ---> GetRcbForRxQueue. Frame: 0x%p.\n", Adapter, Frame);

    *Rcb = NULL;

    //
    // Find the matching queue for the Frame
    //
    if(FindRxQueueRecipient(Adapter, Frame, Nbl1QInfo, &QueueId))
    {
        //
        // Get a reference to the Queue to make sure it is kept valid throughout the receive
        //
        if(AcquireRxQueueReference(Adapter, QueueId, TRUE))
        {
            PMP_ADAPTER_QUEUE Queue = &Adapter->VMQData.RxQueues[QueueId];
            //
            // Get RCB from Queue
            //
            PLIST_ENTRY pEntry = NdisInterlockedRemoveHeadList(
                                    &Queue->FreeRcbList,
                                    &Queue->FreeRcbListLock);
            if (pEntry)
            {
                *Rcb = CONTAINING_RECORD(pEntry, RCB, RcbLink);
            }
            else
            {
                DEBUGP(MP_ERROR, "[%p] Queue %i has run out of available RCBs for receive.\n", Adapter, QueueId);
                //
                // release the acquired reference on the queue
                //
                ReleaseRxQueueReference(Adapter, QueueId);
                Status = NDIS_STATUS_RESOURCES;
            }
        }
    }
    else
    {
        DEBUGP(MP_LOUD, "[%p] No queue matched Frame: 0x%p.\n", Adapter, Frame);
    }

    DEBUGP(MP_TRACE, "[%p] <--- GetRcbForRxQueue. RCB: %p\n", Adapter, Rcb);

    return;
}

NDIS_STATUS
CopyFrameToRxQueueRcb(
    _In_  struct _MP_ADAPTER *Adapter,
    _In_  struct _FRAME *Frame,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _Inout_ struct _RCB *Rcb,
    _Out_ BOOLEAN *Copied)
/*++
Routine Description:

    This routine copies the Frame and VLAN information to the passed in RCB. On a real NIC this copy would
    occur through a HW DMA.

Arguments:

    Adapter                - Pointer to our adapter
    Frame                  - Data Frame
    Nbl1QInfo              - VLAN information for Frame
    Rcb                    - RCB to receive the data
    Copied                 - TRUE if the frame was copied to shared memory

Return Value:

    NDIS_STATUS_SUCCESS if copy succeeded.
    NDIS_STATUS_ADAPTER_NOT_READY if we can no longer copy data to the RCB due to the queue being in freeing state (DMA stopped)

--*/
{
    USHORT QueueId = NET_BUFFER_LIST_RECEIVE_QUEUE_ID(Rcb->Nbl);
    PMP_ADAPTER_QUEUE Queue = &Adapter->VMQData.RxQueues[QueueId];
    PNET_BUFFER NetBuffer = NET_BUFFER_LIST_FIRST_NB(Rcb->Nbl);
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    BOOLEAN DMAEnabled = TRUE;
    *Copied = FALSE;

    DEBUGP(MP_TRACE, "[%p] ---> CopyFrameToRxQueueRcb. RCB: %p\n", Adapter, Rcb);

    //
    // Mark the queue as having an in progress DMA. This avoids a queue free OID request from causing the queue to
    // enter DMA stopped state prematurely.
    //
    QUEUE_SET_FLAG(Queue,fMPAQI_DMA_IN_PROGRESS);

    //
    // Make sure the DMA progress set operation happends before the queue freeing check
    //
    KeMemoryBarrier();

    //
    // Verify that the queue is in a state where we can modify receive memory. Please note we set the DMA in progress
    // first, then check freeing state. The freeing code sets freeing state first, then checks DMA in progress flag. With this
    // approach, it's safe to check freeing state without synchronization and perform DMA if it's not yet set. If it gets updated
    // to the pending freeing state after the below check, the freeing code (FreeAdapterQueueInfo) will see the DMA in progress flag
    // and not set the DMA stopped state.
    //
    if(QUEUE_FREEING(Queue))
    {
        LOCK_STATE_EX LockState;
        BOOLEAN InDMAStoppedState;
        //
        // A free OID was issued on the queue, which is now pending until reference count reaches zero (we must have
        // at least one reference to have reached this point). Verify whether we should update the queue to DMA stopped state
        // (a DMA in progress may have prevented the free code from updating the state).
        //
        NdisAcquireRWLockWrite(Queue->QueueLock, &LockState, 0);
        InDMAStoppedState = QUEUE_DMA_STOPPED(Queue);
        if(!InDMAStoppedState)
        {
            // Set the DMA stopped sate. The indication to NDIS of this change happens outside of our synchronized
            // block to avoid holding the lock for too long.
            QUEUE_SET_FLAG(Queue,fMPAQI_DMA_STOPPED);
        }
        NdisReleaseRWLock(Queue->QueueLock, &LockState);

        if(!InDMAStoppedState)
        {
            //
            // The queue was marked for free, but a pending DMA prevented it from entering DMA stopped. Enter
            // DMA stopped state now, as we will not perform any more copies.
            //
            IndicateRxQueue(
               Adapter,
               QueueId,
               NdisReceiveQueueOperationalStateDmaStopped);
        }

        //
        // Once we've entered the queue free state, we should not perform any more copies to the receive buffers
        //
        DMAEnabled = FALSE;

    }

    if(DMAEnabled)
    {
        if(QueueId)
        {
            PMP_ADAPTER_SHARED_MEMORY_BLOCK PostLookaheadBlock = Rcb->Data, LookaheadBlock = Rcb->LookaheadData;
            ULONG LookaheadSize = 0;

            if(LOOKAHEAD_SPLIT_REQUIRED(Queue))
            {
                LookaheadSize = Queue->LookaheadSize;
            }

            if(LookaheadSize < Frame->ulSize)
            {

                //
                // Copy the PostLookahead data
                //
                memcpy(((PUCHAR)PostLookaheadBlock->Buffer) + LookaheadSize, ((PUCHAR)Frame->Data) + LookaheadSize, Frame->ulSize - LookaheadSize);
                //
                // Update the MDL to reflect the amount of data present
                //
                NdisAdjustMdlLength(PostLookaheadBlock->Mdl, Frame->ulSize - LookaheadSize);
            }

            if(LookaheadSize)
            {
                //
                // Copy the Lookahead
                //
                ULONG DataSize = min(LookaheadSize,Frame->ulSize);
                memcpy(LookaheadBlock->Buffer, Frame->Data, DataSize);
                //
                // Update MDL to reflect the amount of data present
                //
                NdisAdjustMdlLength(LookaheadBlock->Mdl, DataSize);
            }

            *Copied = TRUE;

            NET_BUFFER_DATA_LENGTH(NetBuffer) = Frame->ulSize;

        }

        //
        // Set the OOB VLAN Information
        //
        if(Nbl1QInfo->Value)
        {
            NET_BUFFER_LIST_INFO(Rcb->Nbl, Ieee8021QNetBufferListInfo) = Nbl1QInfo->Value;
        }

    }
    else
    {
        //
        // We are not in a state where we can copy data to receive buffers
        //
        Status = NDIS_STATUS_ADAPTER_NOT_READY;
    }

    //
    // Clear the DMA in progress flag
    //
    QUEUE_CLEAR_FLAG(Queue,fMPAQI_DMA_IN_PROGRESS);


    DEBUGP(MP_TRACE, "[%p] <--- CopyFrameToRxQueueRcb. Copied: %i, Status 0x%08x\n", Adapter, *Copied, Status);

    return Status;

}

VOID
RecoverRxQueueRcb(
    _In_ struct _MP_ADAPTER *Adapter,
    _In_ struct _RCB *Rcb)
/*++
Routine Description:

    This routine returns an RCB to its owner VMQ Queue, releasing any references held.

Arguments:

    Adapter                - Pointer to our adapter
    Rcb                    - RCB to recover

Return Value:

    None

--*/
{
    //
    // Return RCB to owner Queue
    //
    PMP_ADAPTER_QUEUE Queue = NULL;
    USHORT QueueId = NET_BUFFER_LIST_RECEIVE_QUEUE_ID(Rcb->Nbl);

    DEBUGP(MP_TRACE, "[%p] ---> RecoverRxQueueRcb. RCB: %p\n", Adapter, Rcb);

    if(QueueId>=NIC_SUPPORTED_NUM_QUEUES)
    {
        DEBUGP(MP_ERROR, "[%p] RCB could not be recovered. The NBL QueueId was out of range: %i\n", Adapter, QueueId);
        ASSERT(FALSE);
        return;
    }

    Queue = &Adapter->VMQData.RxQueues[QueueId];
    if(!QueueId)
    {
        //
        // Default queue uses the send frame, rather than copying the data. Release
        // the frame reference.
        //
        HWFrameRelease((PFRAME)Rcb->Data);
        Rcb->Data = NULL;
    }

    //
    // Add RCB back to free pool
    //
    NdisInterlockedInsertTailList(
            &Queue->FreeRcbList,
            &Rcb->RcbLink,
            &Queue->FreeRcbListLock);

    //
    // Release reference to queue that we took when indicating the NBL
    //
    ReleaseRxQueueReference(Adapter, QueueId);

    DEBUGP(MP_TRACE, "[%p] <--- RecoverRxQueueRcb\n", Adapter);

}

VOID
AddPendingRcbToRxQueue(
    _In_ struct _MP_ADAPTER *Adapter,
    _In_ struct _RCB *Rcb)
/*++
Routine Description:

    This routine adds an RCB to the pending receive list for its owner VMQ queue.

Arguments:

    Adapter                - Pointer to our adapter
    Rcb                    - RCB to queue for recieve

Return Value:

    None

--*/
{
    NDIS_RECEIVE_QUEUE_ID QueueId = NET_BUFFER_LIST_RECEIVE_QUEUE_ID(Rcb->Nbl);
    DEBUGP(MP_TRACE, "[%p] ---> AddPendingRcbToRxQueue. RCB: %p\n", Adapter, Rcb);
    NdisInterlockedInsertTailList(
        &Adapter->ReceiveBlock[QueueId].ReceiveList,
        &Rcb->RcbLink,
        &Adapter->ReceiveBlock[QueueId].ReceiveListLock);
    DEBUGP(MP_TRACE, "[%p] <--- AddPendingRcbToRxQueue\n", Adapter);
}

