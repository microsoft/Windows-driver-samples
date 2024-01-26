/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    RSSv2.c

Abstract:

    This module implements the NDIS RSSv2-related functionality for the adapter.

--*/

#include "netvmin6.h"
#include "rssv2.tmh"

//
// Constant defines the maximum number of processors in the RSS processor set
// the hardware can support.
//
#define RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE 64
#include "rssv2lib.h"

VOID
MiniportApplyMoveITECommandToHW(
    _Inout_ PMP_ADAPTER Adapter,
    _Inout_ PMP_ADAPTER_VPORT VPort,
    _In_ PNDIS_RSS_SET_INDIRECTION_ENTRY Command,
    _In_ UINT8 NewLocalCpuIndex
    )
/*++
Routine Description:

    This routine propagates changes as specified by the move command to the HW.

    NOTE: This routine cannot fail. All failures should have been handled and
    reported before calling this function.

    If there is an underlying transient condition (for example in FIRWMARE),
    it is miniport's responsibility to retry to clear transient error.

Arguments:

    Adapter                 - Pointer to our adapter

    Vport                   - Pointer to the VPort

    Command                 - Move command which identifies the steering 
                              parameter and the target processor.

    NewLocalCpuIndex        - Local index of the target processor the steering 
                              parameter is being pointed to.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(VPort);
    UNREFERENCED_PARAMETER(Command);
    UNREFERENCED_PARAMETER(NewLocalCpuIndex);
}

VOID
MiniportApplyConfigurationToHW(
    _Inout_ PMP_ADAPTER Adapter,
    _Inout_ PMP_ADAPTER_VPORT VPort,
    _In_ BOOLEAN IsRssEnabled,
    _In_ USHORT NewITCount, 
    _In_ ULONG NewNumberOfQueues
    )
/*++
Routine Description:

    This routine propagates changes as specified by the configuration OID to 
    the HW.

    All failures cases are already handled and before calling this function.

Arguments:

    Adapter                 - Pointer to our adapter

    Vport                   - Pointer to the VPort

    IsRssEnabled            - New RSS state

    NewITCount              - New size of the indirection table

    NewNumberOfQueues       - New number of HW queues allocated to the VPort

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(VPort);
    UNREFERENCED_PARAMETER(IsRssEnabled);
    UNREFERENCED_PARAMETER(NewITCount);
    UNREFERENCED_PARAMETER(NewNumberOfQueues);
}
    

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
InitializeRSSConfig(
    _Inout_ PMP_ADAPTER Adapter
    )
/*++
Routine Description:

    This routine will initialize RSS for the miniport.

Arguments:

    Adapter               - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    SIZE_T RssInfoSize;
    NDIS_STATUS Status;

    DEBUGP(MP_TRACE, "[%p] ---> InitializeRSSConfig\n", Adapter);

    PAGED_CODE();

    RssInfoSize = 0;
    Status = NdisGetRssProcessorInformation(Adapter->AdapterHandle,
                                            NULL,
                                            &RssInfoSize);

    if (Status != NDIS_STATUS_BUFFER_TOO_SHORT)
    {
        DEBUGP(MP_ERROR, "%s: Unabled to get rss information size\n", __FUNCTION__);
        goto Cleanup;
    }

    //
    // Allocate the neccessary memory for reading the available processors
    //
    Adapter->RSSData.RssProcessorInfo = 
        (PNDIS_RSS_PROCESSOR_INFO)ExAllocatePool2(POOL_FLAG_NON_PAGED, 
                                                  RssInfoSize,
                                                  'IRMT');
    if (Adapter->RSSData.RssProcessorInfo == NULL)
    {
        DEBUGP(MP_ERROR, "%s: Failed to allocate memory for rss information\n", __FUNCTION__);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = NdisGetRssProcessorInformation(Adapter->AdapterHandle,
                                            Adapter->RSSData.RssProcessorInfo,
                                            &RssInfoSize);
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(MP_ERROR, "%s: Unabled to get rss information\n", __FUNCTION__);
        goto Cleanup;
    }

    Adapter->RSSData.RssProcessorArray = (PNDIS_RSS_PROCESSOR)
        ((PUCHAR)Adapter->RSSData.RssProcessorInfo + 
         Adapter->RSSData.RssProcessorInfo->RssProcessorArrayOffset);

    Status = NDIS_STATUS_SUCCESS;

Cleanup:

    DEBUGP(MP_TRACE, "<--- [%p] InitializeRSSSConfig Status 0x%08x\n", Adapter, Status);

    return Status;
}


UINT8
NICSetRSSv2FindProcessorInRssSet(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PROCESSOR_NUMBER   ProcessorNumber
    )
/*++
Routine Description:

    This routine finds processor in RSS table and returns its local index
    (relative to RSS table).

Arguments:

    Adapter               - Pointer to our adapter

    ProcessorNumber       - Processor number to lookup

Return Value:

    Local index

--*/
{
    UINT8 index;
    PNDIS_RSS_PROCESSOR processor;

    for (index = 0; 
         index < Adapter->RSSData.RssProcessorInfo->RssProcessorCount; 
         index++)
    {
        processor = &Adapter->RSSData.RssProcessorArray[index];

        if ((processor->ProcNum.Group == ProcessorNumber.Group) && 
            (processor->ProcNum.Number == ProcessorNumber.Number))
        {
            return index;
        }
    }

    return 0xFF;
}


PROCESSOR_NUMBER
NICSetRSSv2RssGetBaseProcessor(
    _In_ PMP_ADAPTER        Adapter
    )
/*++
Routine Description:

    This routine returns base RSS processor for the miniport.

Arguments:

    Adapter               - Pointer to our adapter

Return Value:

    Processor Number.

--*/
{
    PROCESSOR_NUMBER baseProcessor;

    //
    // Mark processor as invalid.
    //
    baseProcessor.Reserved = 1;

    if (Adapter->RSSData.RssProcessorInfo != NULL)
    {
        baseProcessor = Adapter->RSSData.RssProcessorInfo->RssBaseProcessor;
    }

    return baseProcessor;
}


BOOLEAN
NICSetRSSv2ValidateRssProcessor(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PROCESSOR_NUMBER   ProcessorNumber,
    _Out_ UINT8*            LocalCpuIndex
    )
/*++
Routine Description:

    This routine validates processor against minport's RSS settings, and if it 
    is valid, returns local index.

Arguments:

    Adapter               - Pointer to our adapter

    ProcessorNumber       - Processor number to validate

    LocalCpuIndex         - Poitner to local index if processor is valid

Return Value:

    TRUE if processor is valid, FALSE otherwise.

--*/
{
    UINT8 CpuIndex;
    PNDIS_RSS_PROCESSOR_INFO RssProcessorInfo;

    *LocalCpuIndex = (UINT8)(-1);
    RssProcessorInfo = Adapter->RSSData.RssProcessorInfo;

    if (ProcessorNumber.Group < (USHORT)RssProcessorInfo->RssBaseProcessor.Group)
    {
        DEBUGP(MP_ERROR, "RssValidateProcessor: Invalid Proc Group %d:%d, below RssBaseProcGroup\n",
                     ProcessorNumber.Group, ProcessorNumber.Number);
        return FALSE;
    }

    if ((ProcessorNumber.Group == (USHORT)RssProcessorInfo->RssBaseProcessor.Group) &&
        (ProcessorNumber.Number < (UCHAR)RssProcessorInfo->RssBaseProcessor.Number))
    {
        DEBUGP(MP_ERROR, "RssValidateProcessor: Invalid Proc Number %d:%d, below RssBaseProcNumber\n",
                     ProcessorNumber.Group, ProcessorNumber.Number);
        return FALSE;
    }

    if (ProcessorNumber.Group > (USHORT)RssProcessorInfo->RssMaxProcessor.Group)
    {
        DEBUGP(MP_ERROR, "RssValidateProcessor: Invalid Proc Group %d:%d, above RssMaxProcGroup\n",
                     ProcessorNumber.Group, ProcessorNumber.Number);
        return FALSE;
    }

    if ((ProcessorNumber.Group == (USHORT)RssProcessorInfo->RssMaxProcessor.Group) &&
        (ProcessorNumber.Number > (UCHAR)RssProcessorInfo->RssMaxProcessor.Number))
    {
        DEBUGP(MP_ERROR, "RssValidateProcessor: Invalid Proc Number %d:%d, above RssMaxProcNumber\n", 
                     ProcessorNumber.Group, ProcessorNumber.Number);
        return FALSE;
    }

    CpuIndex = NICSetRSSv2FindProcessorInRssSet(Adapter, ProcessorNumber);
    if (CpuIndex == 0xFF)
    {
        DEBUGP(MP_ERROR, "RssValidateProcessor: Invalid Proc Number %d:%d, outside of RSS table\n",
                     ProcessorNumber.Group, ProcessorNumber.Number);
        return FALSE;
    }

    *LocalCpuIndex = CpuIndex;

    return TRUE;
}


NDIS_STATUS
NICSetRSSv2InitializeVPortRSS (
    _In_ PMP_ADAPTER            Adapter,
    _Inout_ PMP_ADAPTER_VPORT   VPort,
    _In_ PROCESSOR_NUMBER       PrimaryProcessor
    )
/*++
Routine Description:

    This routine initializes RSS state for a given VPort.

Arguments:

    Adapter               - Pointer to our adapter

    VPort                 - Pointer to the VPort which contains RSS state

    PrimaryProcessor      - Primary processor to use

Return Value:

    NDIS_STATUS

--*/
{
    SIZE_T Size;
    NDIS_STATUS Status;
    UINT8 PrimaryProcessorIndex;

    VPort->RssEnabled = FALSE;

    if (!NICSetRSSv2ValidateRssProcessor(Adapter,
                                         PrimaryProcessor, 
                                         &PrimaryProcessorIndex))
    {
        DEBUGP(MP_TRACE, "Primary is not a valid RSS processor.\n");
        Status = NDIS_STATUS_INVALID_DATA;
        goto Cleanup;
    }

    Size = RssV2NQEnforcerGetQueueMapSize(RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE);

    VPort->QueueMap = (PRSSV2_QUEUE_MAP)ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                        Size,
                                                        'QNMT');
    if (VPort->QueueMap == NULL)
    {
        DEBUGP(MP_TRACE, "Inisufficient memory for QueueMap processor.\n");
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    RssV2NQEnforcerInitialize(VPort->QueueMap, 
                              RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE);

    RtlFillMemory(&VPort->RssV2IndexTable, sizeof(VPort->RssV2IndexTable), 0xFF);

    VPort->DefaultProcessorNumber = PrimaryProcessor;
    VPort->PrimaryProcessorNumber = PrimaryProcessor;
    VPort->DefaultProcessorIndex = PrimaryProcessorIndex;
    VPort->PrimaryProcessorIndex = PrimaryProcessorIndex;

    VPort->RssV2Params.NumberOfIndirectionTableEntries = 1;
    VPort->RssV2Table[0] = PrimaryProcessor;
    VPort->RssV2IndexTable[0] = PrimaryProcessorIndex;

    //
    // When RSS is OFF, only the primary is active and holds a queue reference.
    //
    RssV2NQEnforcerReference(VPort->QueueMap, VPort->PrimaryProcessorIndex);

    Status = NDIS_STATUS_SUCCESS;

Cleanup:

    return Status;
}


VOID
NICSetRSSv2SetCurrentProcessor(
    _Inout_ PMP_ADAPTER_VPORT VPort,
    _In_ PNDIS_RSS_SET_INDIRECTION_ENTRY Command,
    _In_ PROCESSOR_NUMBER NewProcessor,
    _In_ UINT8 NewCpuIndex
    )
/*++
Routine Description:

    This routine updates tracking information for the steering parameter 
    (Primary, Default or ITE[n]), as selected by the Command.

    Routine is called after the "move command" has fully succeeded.

Arguments:

    VPort                   - Pointer to the VPort

    Command                   - Move command used to update steering parameter

    NewProcessor            - New processor number

    NewCpuIndex             - Corresponding local index

Return Value:

    None.

--*/
{
    if ((Command->Flags & NDIS_RSS_SET_INDIRECTION_ENTRY_FLAG_DEFAULT_PROCESSOR) != 0)
    {
        VPort->DefaultProcessorIndex = NewCpuIndex;
        VPort->DefaultProcessorNumber = NewProcessor;

        DEBUGP(MP_TRACE, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: OK: DEFAULT_PROCESSOR: VPortId %d, RssEnabled %d, TargetProc %d:%d \n",
                        Command->VPortId, VPort->RssEnabled, NewProcessor.Group, NewProcessor.Number);
    }
    else if ((Command->Flags & NDIS_RSS_SET_INDIRECTION_ENTRY_FLAG_PRIMARY_PROCESSOR) != 0)
    {
        VPort->PrimaryProcessorIndex = NewCpuIndex;
        VPort->PrimaryProcessorNumber = NewProcessor;

        DEBUGP(MP_TRACE, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: OK: PRIMARY_PROCESSOR: VPortId %d, RssEnabled %d, TargetProc %d:%d \n",
                        Command->VPortId, VPort->RssEnabled, NewProcessor.Group, NewProcessor.Number);

    }
    else if (Command->Flags == 0)
    {
        VPort->RssV2IndexTable[Command->IndirectionTableIndex] = NewCpuIndex;
        VPort->RssV2Table[Command->IndirectionTableIndex] = NewProcessor;

        DEBUGP(MP_TRACE, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: OK: ITE[%d] VPortId %d, RssEnabled %d, TargetProc %d:%d\n",
                    Command->IndirectionTableIndex, 
                    Command->VPortId, 
                    VPort->RssEnabled, 
                    NewProcessor.Group, 
                    NewProcessor.Number);
    }
}


_Success_(return != FALSE)
BOOLEAN
NICSetRSSv2ValidateCommandAndGetProcessor (
    _In_ PMP_ADAPTER_VPORT VPort,
    _In_ PNDIS_RSS_SET_INDIRECTION_ENTRY Command,
    _Out_ PPROCESSOR_NUMBER CurrentProcessor,
    _Out_ UINT8* CurrentCpuIndex,
    _Out_ BOOLEAN* IsActiveSteeringParameter
    )
/*++
Routine Description:

    This routine validates the move command, and returns information about 
    specified steering parameter (Primary, Default or ITE[n] if command is valid.

Arguments:

    VPort                   - Pointer to the VPort

    Command                   - Move command used to update steering parameter

    CurrentProcessor        - Pointer which receives processor where steering 
                              parameter currenty points to.

    CurrentCpuIndex         - Corresponding local index

    IsActiveSteeringParameter - Pointer to the boolean which say if steering
                                parameter is currently ACTIVE or not.

Return Value:

    TRUE if move command is valid, FALSE - otherwise.

--*/
{
    BOOLEAN isSucceeded;

    isSucceeded = FALSE;
    *CurrentCpuIndex = 0xFF;
    *IsActiveSteeringParameter = FALSE;

    if ((Command->Flags & NDIS_RSS_SET_INDIRECTION_ENTRY_FLAG_DEFAULT_PROCESSOR) != 0)
    {
        *CurrentProcessor = VPort->DefaultProcessorNumber;
        *CurrentCpuIndex = VPort->DefaultProcessorIndex;
        *IsActiveSteeringParameter = VPort->RssEnabled;
        isSucceeded = TRUE;
    }
    else if ((Command->Flags & NDIS_RSS_SET_INDIRECTION_ENTRY_FLAG_PRIMARY_PROCESSOR) != 0)
    {
        *CurrentProcessor = VPort->PrimaryProcessorNumber;
        *CurrentCpuIndex = VPort->PrimaryProcessorIndex;
        *IsActiveSteeringParameter = (BOOLEAN)(VPort->RssEnabled == FALSE);
        isSucceeded = TRUE;
    }
    else if (Command->Flags == 0)
    {
        if (Command->IndirectionTableIndex <
            VPort->RssV2Params.NumberOfIndirectionTableEntries)
        {
            *CurrentProcessor = VPort->RssV2Table[Command->IndirectionTableIndex];
            *CurrentCpuIndex = VPort->RssV2IndexTable[Command->IndirectionTableIndex];
            *IsActiveSteeringParameter = VPort->RssEnabled;
            isSucceeded = TRUE;
        }
        else
        {
            DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid IndirectionTableIndex(%d) >= %d\n", 
                        Command->IndirectionTableIndex, 
                        VPort->RssV2Params.NumberOfIndirectionTableEntries);
        }
    }
    else
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid flags=0x%x\n", Command->Flags);
    }

    return isSucceeded;
}


_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
NICSetRSSv2Parameters(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisRequest)
/*++
Routine Description:

    This routine handles OID_GEN_RECEIVE_SCALE_PARAMETERS_V2 set request. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisRequest  - The OID data for the request

Return Value:

    NDIS_STATUS   

--*/      
{
    USHORT EntryIndex;
    BOOLEAN IsHashInfoChanged;
    BOOLEAN IsHashKeyChanged;
    BOOLEAN IsNumITEsChanged;
    BOOLEAN IsNumQueuesChanged;
    BOOLEAN IsRssEnabled;
    UINT8 LocalCpuIndex;
    USHORT NewITCount;
    ULONG NewNumberOfQueues;
    ULONG NumProcs;
    USHORT OldITCount;
    PROCESSOR_NUMBER PrimaryProcessorNumber;
    PROCESSOR_NUMBER ProcessorNumber;
    PNDIS_RECEIVE_SCALE_PARAMETERS_V2 RssParams;
    struct _SET *Set;
    NDIS_STATUS Status;
    ULONG VPortId;
    PMP_ADAPTER_VPORT VPort;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICSetRSSv2Parameters\n", Adapter);

    Set = &NdisRequest->DATA.SET_INFORMATION;
    RssParams = (PNDIS_RECEIVE_SCALE_PARAMETERS_V2)Set->InformationBuffer;

    //
    // Validate the request
    //
    if (RssParams->HashSecretKeySize != 
        NDIS_RSS_HASH_SECRET_KEY_MAX_SIZE_REVISION_2)
    {
        DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid HashSecretKeySize\n");
        return NDIS_STATUS_INVALID_LENGTH;
    }

    if ((Set->InformationBufferLength < 
         (RssParams->HashSecretKeyOffset + RssParams->HashSecretKeySize))
            ||
        (RssParams->HashSecretKeyOffset <
         NDIS_SIZEOF_RECEIVE_SCALE_PARAMETERS_V2_REVISION_1))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid HashSecretKeyOffset\n");
        return NDIS_STATUS_INVALID_LENGTH;
    }

    if ((NdisRequest->Flags & NDIS_OID_REQUEST_FLAGS_VPORT_ID_VALID) != 0)
    {
        DEBUGP(MP_TRACE, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Issued for VPortId=%d, Flags=0x%x \n", 
                          NdisRequest->VPortId, RssParams->Flags);
        VPortId = NdisRequest->VPortId;
    }
    else
    {
        DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Issued for miniport (NativeRSS), Flags=0x%x\n", RssParams->Flags);
        VPortId = NDIS_INVALID_VPORT_ID;
    }

    if ((VPortId != NDIS_INVALID_VPORT_ID) && 
        (VPortId >= MAX_NIC_SWITCH_VPORTS))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid VPortId\n");
        return NDIS_STATUS_INVALID_PORT;
    }

    if (VPortId != NDIS_INVALID_VPORT_ID)
    {
        VPort = &Adapter->RSSData.VPort[VPortId];
    }
    else
    {
        VPort = &Adapter->RSSData.NativeVPort;
    }

    IsNumQueuesChanged = 
        ((RssParams->Flags & NDIS_RECEIVE_SCALE_PARAM_NUMBER_OF_QUEUES_CHANGED) != 0) &&
         (RssParams->NumberOfQueues != VPort->RssV2Params.NumberOfQueues);

    IsRssEnabled = 
        ((RssParams->Flags & NDIS_RECEIVE_SCALE_PARAM_ENABLE_RSS) != 0);

    IsNumITEsChanged = 
        ((RssParams->Flags & NDIS_RECEIVE_SCALE_PARAM_NUMBER_OF_ENTRIES_CHANGED) != 0) &&
         (RssParams->NumberOfIndirectionTableEntries != 
          VPort->RssV2Params.NumberOfIndirectionTableEntries);

    IsHashInfoChanged = 
        ((RssParams->Flags & NDIS_RECEIVE_SCALE_PARAM_HASH_INFO_CHANGED) != 0) &&
         (VPort->RssV2Params.HashInformation != RssParams->HashInformation);

    IsHashKeyChanged = 
        ((RssParams->Flags & NDIS_RECEIVE_SCALE_PARAM_HASH_KEY_CHANGED) != 0) &&
          !RtlEqualMemory(&VPort->RssV2Key,
                          (PUCHAR)RssParams + RssParams->HashSecretKeyOffset,
                          RssParams->HashSecretKeySize);

    NewNumberOfQueues = VPort->RssV2Params.NumberOfQueues;
    if (IsNumQueuesChanged)
    {
        NewNumberOfQueues = RssParams->NumberOfQueues;
        if (VPort->RssEnabled)
        {
            //
            // Check for NQ-violation during queue change.
            //
            // Since RSS is enabled, the VPort->QueueMap reflects up-to-date
            // queue-to-processor mapping.
            //
            // Get the number of current referenced processors.
            //
            NumProcs = RssV2NQEnforcerGetNumberOfProcs(VPort->QueueMap);
            if (NumProcs > NewNumberOfQueues)
            {
                DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: VPortId=%d, NQ-Violation (queue change): NQueues=%d < NProcs=%d\n",
                            NdisRequest->VPortId,
                            NewNumberOfQueues, 
                            NumProcs);
                Status = NDIS_STATUS_NO_QUEUES;
                goto Cleanup;
            }
        }
    }

    OldITCount = (USHORT)VPort->RssV2Params.NumberOfIndirectionTableEntries;
    NewITCount = OldITCount;

    //
    // IT size change: check for non-repeat violation during shrinking.
    //
    if (IsNumITEsChanged)
    {
        NewITCount = (USHORT)RssParams->NumberOfIndirectionTableEntries;
        if (OldITCount > NewITCount)
        {
            ASSERT((OldITCount % NewITCount) == 0);

            //
            // IT shrinking should fail if pattern is non-repeat.
            //
            for (EntryIndex = NewITCount; EntryIndex < OldITCount; EntryIndex++)
            {
                ProcessorNumber = VPort->RssV2Table[EntryIndex % NewITCount];
                if ((ProcessorNumber.Group != VPort->RssV2Table[EntryIndex].Group) ||
                    (ProcessorNumber.Number != VPort->RssV2Table[EntryIndex].Number))
                {
                    DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: VPortId %d, Cannot shrink IT size from %d to %d, mismatch at ITE[%d]\n", 
                        NdisRequest->VPortId, 
                        OldITCount, 
                        NewITCount,
                        EntryIndex);
                    Status = NDIS_STATUS_INVALID_DATA;
                    goto Cleanup;
                }
            }
        }
    }

    if (VPort->RssEnabled == FALSE && IsRssEnabled)
    {
        RssV2NQEnforcerReset(VPort->QueueMap);

        //
        // Check for processor numbers sanity in the default processor
        // and the indirection table.
        //
        if (!NICSetRSSv2ValidateRssProcessor(Adapter,
                                             VPort->DefaultProcessorNumber,
                                             &LocalCpuIndex))
        {
            DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid DedfaultProcessorNumebr: %d:%d \n",
                        VPort->DefaultProcessorNumber.Group, 
                        VPort->DefaultProcessorNumber.Number);

            Status = NDIS_STATUS_INVALID_DATA;
            goto Cleanup;
        }

        VPort->DefaultProcessorIndex = LocalCpuIndex;
        RssV2NQEnforcerReference(VPort->QueueMap, LocalCpuIndex);

        OldITCount = (USHORT)VPort->RssV2Params.NumberOfIndirectionTableEntries;

        for (EntryIndex = 0; EntryIndex < OldITCount; EntryIndex++)
        {
            ProcessorNumber = VPort->RssV2Table[EntryIndex];

            if (!NICSetRSSv2ValidateRssProcessor(Adapter,
                                                 ProcessorNumber, 
                                                 &LocalCpuIndex))
            {
                DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid ITE[%d]: %d:%d \n",
                            EntryIndex,
                            ProcessorNumber.Group, 
                            ProcessorNumber.Number);

                Status = NDIS_STATUS_INVALID_DATA;
                goto Cleanup;
            }

            VPort->RssV2IndexTable[EntryIndex] = LocalCpuIndex;
            RssV2NQEnforcerReference(VPort->QueueMap, LocalCpuIndex);
        }

        //
        // After VPort->QueueMap is built, check for NQ-violation during 
        // RSS enablement.
        //
        // Get number of queues after RSS enablement.
        //
        NumProcs = RssV2NQEnforcerGetNumberOfProcs(VPort->QueueMap);
        if (NumProcs > NewNumberOfQueues)
        {
            DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: NQ-Violation: NQueues=%d < NProcs=%d\n",
                        NewNumberOfQueues, 
                        NumProcs);
            Status = NDIS_STATUS_NO_QUEUES;
            goto Cleanup;
        }
    }

    if (VPort->RssEnabled && (IsRssEnabled == FALSE))
    {
        //
        // Check for primary processor number sanity during RSS=OFF
        //
        PrimaryProcessorNumber = VPort->PrimaryProcessorNumber;

        if (!NICSetRSSv2ValidateRssProcessor(Adapter,
                                             PrimaryProcessorNumber, 
                                             &LocalCpuIndex))
        {
            DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid PrimaryProcessorNumebr: %d:%d \n",
                        PrimaryProcessorNumber.Group, 
                        PrimaryProcessorNumber.Number);

            Status = NDIS_STATUS_INVALID_DATA;
            goto Cleanup;
        }

        RssV2NQEnforcerReset(VPort->QueueMap);
        VPort->PrimaryProcessorIndex = LocalCpuIndex;
        RssV2NQEnforcerReference(VPort->QueueMap, LocalCpuIndex);
    }

    //
    // COMMIT PASS: NO FAILURES AFTER THIS POINT!
    //
    if (IsHashKeyChanged)
    {
        VPort->RssV2Params.HashSecretKeySize = 
            RssParams->HashSecretKeySize;

        NdisMoveMemory(&VPort->RssV2Key,
                      (PUCHAR)RssParams + RssParams->HashSecretKeyOffset,
                      RssParams->HashSecretKeySize);
    }

    if (IsHashInfoChanged)
    {
        VPort->RssV2Params.HashInformation = RssParams->HashInformation;
    }

    //
    // IT size change: actual application (cannot fail)
    //
    if (IsNumITEsChanged)
    {
        if (OldITCount < NewITCount)
        {
            //
            // IT expansion
            //
            DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: VPortId %d, Expand IT size from %d to %d\n", 
                NdisRequest->VPortId, OldITCount, NewITCount);

            ASSERT((NewITCount % OldITCount) == 0);
            for (EntryIndex = OldITCount; EntryIndex < NewITCount; EntryIndex++)
            {
                ProcessorNumber = VPort->RssV2Table[EntryIndex % OldITCount];
                VPort->RssV2Table[EntryIndex] = ProcessorNumber;

                LocalCpuIndex = VPort->RssV2IndexTable[EntryIndex % OldITCount];
                VPort->RssV2IndexTable[EntryIndex] = LocalCpuIndex;
                RssV2NQEnforcerReference(VPort->QueueMap, LocalCpuIndex);
            }
        }
        else if (OldITCount > NewITCount)
        {
            //
            // IT contraction
            //
            DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: VPortId %d, Shrink IT size from %d to %d\n", 
                NdisRequest->VPortId, OldITCount, NewITCount);
            ASSERT((OldITCount % NewITCount) == 0);

            for (EntryIndex = NewITCount; EntryIndex < OldITCount; EntryIndex++)
            {
                ProcessorNumber = VPort->RssV2Table[EntryIndex % NewITCount];
                ASSERT(ProcessorNumber.Group == VPort->RssV2Table[EntryIndex].Group);
                ASSERT(ProcessorNumber.Number == VPort->RssV2Table[EntryIndex].Number);
                LocalCpuIndex = VPort->RssV2IndexTable[EntryIndex];
                VPort->RssV2IndexTable[EntryIndex] = 0xFF;
                RssV2NQEnforcerDereference(VPort->QueueMap, LocalCpuIndex);
            }
        }
    }

    //
    // Apply new configuration to HW (hash key and information is already in 
    // the VPort object).
    //
    MiniportApplyConfigurationToHW(Adapter, 
                                   VPort, 
                                   IsRssEnabled,
                                   NewITCount, 
                                   NewNumberOfQueues);

    VPort->RssV2Params.NumberOfIndirectionTableEntries = NewITCount;
    VPort->RssV2Params.NumberOfQueues = NewNumberOfQueues;
    VPort->RssEnabled = IsRssEnabled;

    DEBUGP(MP_TRACE, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: VPortId=%d, RssEnabled=%d, IT.size=%d\n", 
        NdisRequest->VPortId, IsRssEnabled, VPort->RssV2Params.NumberOfIndirectionTableEntries);

    Set->BytesNeeded = Set->InformationBufferLength;
    Status = NDIS_STATUS_SUCCESS;

Cleanup:

    DEBUGP(MP_TRACE, "<--- [%p] NICSetRSSv2Parameters Status 0x%08x\n", Adapter, Status);

    return Status;
}


_IRQL_requires_(DISPATCH_LEVEL)
NDIS_STATUS
NICSetRSSv2IndirectionTableEntries(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisRequest)
/*++
Routine Description:

    This routine handles OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES method request. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisRequest  - The OID data for the request

Return Value:

    NDIS_STATUS   

--*/      
{
    PROCESSOR_NUMBER ActorProcessorNumber;
    PNDIS_RSS_SET_INDIRECTION_ENTRY Command;
    RSSV2_PARSING_CONTEXT Context;
    PROCESSOR_NUMBER CurrentProcessorNumber;
    PNDIS_RSS_SET_INDIRECTION_ENTRY EntryArray;
    BOOLEAN IsActive;
    BOOLEAN IsNativeRss;
    BOOLEAN IsValid;
    ULONG InputBufferLength;
    struct _METHOD *Method;
    UINT8 NewCpuIndex;
    ULONG NumCommandsToExecute;
    UINT8 OldCpuIndex;
    PNDIS_RSS_SET_INDIRECTION_ENTRIES RssEntries;
    NDIS_STATUS Status;
    ULONG SwitchId;
    PROCESSOR_NUMBER TargetProcessorNumber;
    ULONG VPortId;
    PMP_ADAPTER_VPORT VPort;

    //
    // Allocate a local queue map on stack, to hold temporary results during 
    // handling of each "move all" group.
    //
    DECLARE_RSSV2_QUEUE_MAP_ON_STACK(LocalQueueMap, 
                                     RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE);

    DEBUGP(MP_TRACE, "[%p] ---> NICSetRSSv2IndirectionTableEntries\n", Adapter);

    Method = &NdisRequest->DATA.METHOD_INFORMATION;
    RssEntries = (PNDIS_RSS_SET_INDIRECTION_ENTRIES)Method->InformationBuffer;
    InputBufferLength = Method->InputBufferLength;

    Method->BytesRead = 0;
    Method->BytesNeeded = 0;
    Method->BytesWritten = 0; 

    //
    // Validate the request
    //
    if (InputBufferLength < 
        (NDIS_SIZEOF_RSS_SET_INDIRECTION_ENTRIES_REVISION_1 +
         RssEntries->NumberOfRssEntries * RssEntries->RssEntrySize))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid InformationBufferLength \n");
        Status = NDIS_STATUS_INVALID_LENGTH;
        goto Cleanup;
    }

    //
    // RSSv2 spec requires up to 130 entries to be handled in a single batch.
    //
    if (RssEntries->NumberOfRssEntries > 
        (2 + MAX_NUMBER_OF_INDIRECTION_TABLE_ENTRIES))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid NumberOfRssEntries \n");
        Status = NDIS_STATUS_INVALID_DATA;
        goto Cleanup;
    }

    KeGetCurrentProcessorNumberEx(&ActorProcessorNumber);

    IsNativeRss = (BOOLEAN)(Adapter->RSSData.IsNicSwitchCreated == FALSE);

    RssV2InitializeParsingContext(&Context, RssEntries, IsNativeRss);

    EntryArray = (PNDIS_RSS_SET_INDIRECTION_ENTRY)
        ((PUCHAR)RssEntries + RssEntries->RssEntryTableOffset);

    DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Actor %d:%d, NumberOfRssEntries %d \n",
                ActorProcessorNumber.Group, 
                ActorProcessorNumber.Number, 
                RssEntries->NumberOfRssEntries);

    NumCommandsToExecute = 0;

    //
    // PASS 1: validation
    //
    while (RssV2GetNextCommandRange(&Context, &SwitchId, &VPortId))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: 1ST PASS FOR VPortId=%d\n",
                    VPortId);
        if (IsNativeRss)
        {
            ASSERT(VPortId == NDIS_INVALID_SWITCH_ID);
            ASSERT(VPortId == NDIS_INVALID_VPORT_ID);
            VPort = &Adapter->RSSData.NativeVPort;
        }
        else
        {
            if (SwitchId != NDIS_DEFAULT_SWITCH_ID)
            {
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid SwitchId (%d)\n", 
                            SwitchId);
                RssV2SetCommandRangeStatus(&Context, 
                                           NDIS_STATUS_INVALID_PARAMETER);
                continue; // while (RssV2FindNextCommandRange())
            }

            if (VPortId > MAX_NIC_SWITCH_VPORTS)
            {
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid VPortId (%d)\n", 
                            VPortId);
                RssV2SetCommandRangeStatus(&Context, 
                                           NDIS_STATUS_INVALID_PARAMETER);
                continue; // while (RssV2FindNextCommandRange())
            }

            VPort = &Adapter->RSSData.VPort[VPortId];

            if (VPort->Created == FALSE)
            {                
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: VPort %d is not created\n", 
                            VPortId);
                RssV2SetCommandRangeStatus(&Context, 
                                           NDIS_STATUS_INVALID_PARAMETER);
                continue; // while (RssV2FindNextCommandRange())
            }

            if (VPort->Active == FALSE)
            {                
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: VPort %d is not active\n", 
                            VPortId);
                RssV2SetCommandRangeStatus(&Context, 
                                           NDIS_STATUS_INVALID_PORT_STATE);
                continue; // while (RssV2FindNextCommandRange())
            }
        }

        ASSERT(VPort != NULL);

        //
        // Once valid VPort for the Command range is acquired, perform
        // validation of individual "move entry" commands.
        //
        while ((Command = RssV2GetNextCommand(&Context, FALSE)) != NULL)
        {
            if (!NICSetRSSv2ValidateCommandAndGetProcessor(
                    VPort, 
                    Command,
                    &CurrentProcessorNumber,
                    &OldCpuIndex,
                    &IsActive))
            {
                Command->EntryStatus = NDIS_STATUS_INVALID_PARAMETER;
                continue; // while (Command = RssV2GetNextCommand())
            }

            if ((ActorProcessorNumber.Group  != CurrentProcessorNumber.Group) ||
                (ActorProcessorNumber.Number != CurrentProcessorNumber.Number))
            {
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: VPortId=%d, Flags=0x%x, EntryIndex=%d: Invalid Actor %d:%d, expected %d:%d\n",
                            Command->VPortId, 
                            Command->Flags,
                            Command->IndirectionTableIndex, 
                            ActorProcessorNumber.Group,
                            ActorProcessorNumber.Number,
                            CurrentProcessorNumber.Group, 
                            CurrentProcessorNumber.Number);

                Command->EntryStatus = NDIS_STATUS_NOT_ACCEPTED;
                __debugbreak();
                continue; // while (Command = RssV2GetNextCommand())
            }

            TargetProcessorNumber = Command->TargetProcessorNumber;

            //
            // Treat no-op commands as "succeded".
            //
            if ((TargetProcessorNumber.Group == CurrentProcessorNumber.Group) &&
                (TargetProcessorNumber.Number == CurrentProcessorNumber.Number))
            {
                DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: OK1: VPortId=%d, Flags=0x%x, EntryIndex=%d: TargetProc=%d:%d\n",
                            Command->VPortId, 
                            Command->Flags,
                            Command->IndirectionTableIndex, 
                            TargetProcessorNumber.Group,
                            TargetProcessorNumber.Number);

                Command->EntryStatus = NDIS_STATUS_SUCCESS;
                continue; // while (Command = RssV2GetNextCommand())
            }

            if (IsActive == FALSE)
            {
                //
                // INACTIVE steering entities are only tracked and will be
                // enforced during RSS transition to ON/OFF.
                //
                NICSetRSSv2SetCurrentProcessor(VPort, 
                                               Command, 
                                               TargetProcessorNumber, 
                                               0xFF);

                DEBUGP(MP_ERROR, "NDIS_SET_INDIRECTION_TABLE_ENTRY: OK2: VPortId=%d, Flags=0x%x, EntryIndex=%d, TargetProc=%d:%d\n",
                            Command->VPortId, 
                            Command->Flags, 
                            Command->IndirectionTableIndex, 
                            TargetProcessorNumber.Group, 
                            TargetProcessorNumber.Number);

                Command->EntryStatus = NDIS_STATUS_SUCCESS;
                continue; // while (Command = RssV2GetNextCommand())
            }

            if (!NICSetRSSv2ValidateRssProcessor(Adapter,
                                                 TargetProcessorNumber, 
                                                 &NewCpuIndex))
            {
                Command->EntryStatus = NDIS_STATUS_INVALID_DATA;
                continue; // while (Command = RssV2GetNextCommand())
            }

            //
            // Marked commands as candidates for NQ-check and HW execution
            // if they:
            //
            //  - refer to ACTIVE steering entities
            //  - contain a validated target processor
            //  - target processor actualy changes
            //
            ASSERT(Command->EntryStatus == NDIS_STATUS_PENDING);
            NumCommandsToExecute++;
        }
    }

    if (NumCommandsToExecute == 0)
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: 2ND PASS EMPTY FOR VPortId=%d\n",
                    VPortId);
        Status = NDIS_STATUS_SUCCESS;
        goto Cleanup;
    }

    RssV2RestartRangeIterator(&Context);

    //
    // PASS 2: NQ-enforcement and execution for ACTIVE steering parameters only.
    //
    while (RssV2GetNextCommandRange(&Context, &SwitchId, &VPortId))
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: 2ND PASS FOR VPortId=%d\n",
                    VPortId);
        //
        // VPorts are already validated
        //
        VPort = IsNativeRss ? &Adapter->RSSData.NativeVPort : 
                              &Adapter->RSSData.VPort[VPortId];

        RssV2NQEnforcerEnter(VPort->QueueMap, LocalQueueMap);

        //
        // Iterate over non-processed commands and update LocalQueueMap
        //
        while ((Command = RssV2GetNextCommand(&Context, TRUE)) != NULL)
        {
            TargetProcessorNumber = Command->TargetProcessorNumber;
            IsValid = NICSetRSSv2ValidateRssProcessor(Adapter,
                                                      TargetProcessorNumber, 
                                                      &NewCpuIndex);
            ASSERT(IsValid);

            IsValid = NICSetRSSv2ValidateCommandAndGetProcessor(
                            VPort,
                            Command,
                            &CurrentProcessorNumber,
                            &OldCpuIndex,
                            &IsActive);
            ASSERT(IsValid && IsActive);
            RssV2NQEnforcerUpdate(LocalQueueMap, OldCpuIndex, NewCpuIndex);
        }

        Status = RssV2NQEnforcerLeave(VPort->QueueMap,
                                      LocalQueueMap, 
                                      VPort->RssV2Params.NumberOfQueues);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: VPortId=%d: NQ-violation: NQueues=%d < NProcs=%d\n",
                        VPortId, 
                        VPort->RssV2Params.NumberOfQueues,
                        RssV2NQEnforcerGetNumberOfProcs(LocalQueueMap));
            RssV2SetCommandRangeStatus(&Context, Status);
            continue; // while (RssV2GetNextCommandRange())
        }

        //
        // NO FAILURES AFTER THIS POINT (INCLUDING IN HW !!!)
        //

        //
        // Iterate over the same Command range to actually update the 
        // processor numbers/indices.
        //
        RssV2RestartCommandIterator(&Context);

        while ((Command = RssV2GetNextCommand(&Context, TRUE)) != NULL)
        {
            TargetProcessorNumber = Command->TargetProcessorNumber;
            IsValid = NICSetRSSv2ValidateRssProcessor(Adapter,
                                                      TargetProcessorNumber, 
                                                      &NewCpuIndex);
            ASSERT(IsValid);

            //
            // Apply "move ITE" command to hardware
            //
            MiniportApplyMoveITECommandToHW(Adapter, VPort, Command, NewCpuIndex);

            //
            // Reflect the change in software structure.
            //
            NICSetRSSv2SetCurrentProcessor(VPort, 
                                           Command, 
                                           TargetProcessorNumber, 
                                           NewCpuIndex);

            DEBUGP(MP_ERROR, "NDIS_SET_INDIRECTION_TABLE_ENTRY: OK3: VPortId=%d, Flags=0x%x, EntryIndex=%d, TargetProc=%d:%d\n",
                        Command->VPortId, 
                        Command->Flags, 
                        Command->IndirectionTableIndex, 
                        TargetProcessorNumber.Group, 
                        TargetProcessorNumber.Number);

            Command->EntryStatus = NDIS_STATUS_SUCCESS;
        }
    }


    Method->BytesRead = InputBufferLength;
    Method->BytesWritten = InputBufferLength;
    Status = NDIS_STATUS_SUCCESS;

Cleanup:

    DEBUGP(MP_TRACE, "<--- [%p] NICSetRSSv2IndirectionTableEntries Status 0x%08x\n", Adapter, Status);

    return Status;
}
