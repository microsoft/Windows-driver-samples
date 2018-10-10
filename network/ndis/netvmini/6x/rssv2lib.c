/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    RSSv2Lib.c

Abstract:

    This module implements RSSv2 helper library.

--*/
#pragma warning(push)
#define  PRAGMA_ZERO_SIZED_ARRAY        4200
#define  PRAGMA_NAMELESS_STRUCT_UNION   4201
#define  PRAGMA_BIT_FIELD_NOT_INT       4214
#define  PRAGMA_STRUCTURE_PADDED        4324
#define  PRAGMA_NO_RETTYPE_FOR_FUNC     4508
#pragma warning(disable: PRAGMA_NO_RETTYPE_FOR_FUNC)
#pragma warning(disable: PRAGMA_ZERO_SIZED_ARRAY) 
#pragma warning(disable: PRAGMA_NAMELESS_STRUCT_UNION)
#pragma warning(disable: PRAGMA_STRUCTURE_PADDED)
#include <ndis.h>
#pragma warning(pop)

#define RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE 64
#include "rssv2lib.h"

extern
FORCEINLINE
VOID
RssV2InitializeParsingContext (
    _Out_ PRSSV2_PARSING_CONTEXT Context,
    _In_ PNDIS_RSS_SET_INDIRECTION_ENTRIES RssV2Oid,
    _In_ BOOLEAN IsNativeRss
    )
/*++
Routine Description:

    This routine initializes RSSv2 command parsing context from the OID.

Arguments:

    Context - parsing context

    RssV2Oid - OID that needs parsing

    IsNativeRSS - Boolean which tells if the miniport is in NativeRSS mode

Return Value:

    None.

--*/
{
    RtlZeroMemory(Context, sizeof(*Context));
    Context->IsNativeRss = IsNativeRss;
    Context->RssV2Oid = RssV2Oid;
    Context->MaxIndex = RssV2Oid->NumberOfRssEntries;
    Context->LimitIndex = 0; 
    Context->StartIndex = 0;
    Context->LastStartIndex = 0;
}


_Success_(return != FALSE)
BOOLEAN
RssV2GetNextCommandRange (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _Out_ ULONG* SwitchId,
    _Out_ ULONG* VPortId
    )
/*++
Routine Description:

    This routine finds a group of move commands in the OID, which target the
    same SwitchID & VPortID. It updates the Context to facilitate iteration
    over the group later.

Arguments:

    Context - parsing context

    SwitchId - pointer to the variable which receives SwitchId

    VPortId - pointer to the variable which receives VPortId

Return Value:

    TRUE if a new group of commands is found, FALSE otherwise.

--*/
{
    PNDIS_RSS_SET_INDIRECTION_ENTRY command;
    ULONG currIndex;
    BOOLEAN isRangePresent;
    ULONG switchId;
    ULONG vPortId;

    isRangePresent = (BOOLEAN)(Context->LimitIndex < Context->MaxIndex);

    if (isRangePresent)
    {
        Context->StartIndex = Context->LimitIndex;
        Context->LastStartIndex = Context->StartIndex;

        if (Context->IsNativeRss)
        {
            switchId = NDIS_INVALID_SWITCH_ID;
            vPortId = NDIS_INVALID_VPORT_ID;
            currIndex = Context->MaxIndex;
        }
        else
        {
            currIndex = Context->LimitIndex;
            command = RSSV2_GET_COMMAND(Context->RssV2Oid, currIndex);
            switchId = command->SwitchId;
            vPortId = command->VPortId;

            for (; currIndex < Context->MaxIndex; currIndex++)
            {
                command = RSSV2_GET_COMMAND(Context->RssV2Oid, currIndex);

                if ((command->SwitchId != switchId) ||
                    (command->VPortId != vPortId))
                {
                    break;
                }
            }
        }

        Context->LimitIndex = currIndex;
        *SwitchId = switchId;
        *VPortId = vPortId;
    }

    return isRangePresent;
}


VOID
RssV2RestartRangeIterator (
    _Inout_ PRSSV2_PARSING_CONTEXT Context
    )
/*++
Routine Description:

    This routine allows to reset the context with respect to iterations
    over the command groups.

    This is used by multi-pass OID handlers.

Arguments:

    Context - parsing context

Return Value:

    None.

--*/
{
    Context->LimitIndex = 0;
    Context->StartIndex = 0;
    Context->LastStartIndex = 0;
}




PNDIS_RSS_SET_INDIRECTION_ENTRY
RssV2GetNextCommand (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _In_ BOOLEAN SkipProcessedCommands
    )
/*++
Routine Description:

    This routine returns next command from the current command group (which 
    target the same SwitchId & VPortId). 

Arguments:

    Context - parsing context

    SkipProcessedCommands - TRUE, if user wants to commands which already 
                            have EntryStatus changed from NDIS_STATUS_PENDING.
                            FALSE, if user wants to iterate over all commands.

Return Value:

    Pointer to the next command, NULL - if there are no more commands left.

--*/
{
    PNDIS_RSS_SET_INDIRECTION_ENTRY command;

    do
    {
        if (Context->StartIndex < Context->LimitIndex)
        {
            command = RSSV2_GET_COMMAND(Context->RssV2Oid, 
                                        Context->StartIndex++);
        }
        else
        {
            command = NULL;
        }
    } while (SkipProcessedCommands && 
             (command != NULL) &&
             (command->EntryStatus != NDIS_STATUS_PENDING));

    return command;
}



VOID
RssV2RestartCommandIterator (
    _Inout_ PRSSV2_PARSING_CONTEXT Context
    )
/*++
Routine Description:

    This routine allows to reset the context with respect to iterations
    over the commands inside the same command group.

    This is used by multi-pass OID handlers.

Arguments:

    Context - parsing context

Return Value:

    None.

--*/
{
    Context->StartIndex = Context->LastStartIndex;
}


VOID
RssV2SetCommandRangeStatus (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _In_ NDIS_STATUS Status
    )
/*++
Routine Description:

    This routine sets EntryStatus for all commands of the current command group

Arguments:

    Context - parsing context

    Status - NDIS_STATUS to set

Return Value:

    None.

--*/
{
    ULONG index;
    PNDIS_RSS_SET_INDIRECTION_ENTRY command;

    for (index = Context->LastStartIndex; index < Context->LimitIndex; index++)
    {
        command = RSSV2_GET_COMMAND(Context->RssV2Oid, index);
        command->EntryStatus = Status;
    }
}


FORCEINLINE
PULONG_PTR
RssV2NQEnforcerGetBitfield (
    _In_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    )
/*++
Routine Description:

    Routine finds a pointer to the bitfield for specified 
    (RSS-)local processor index.

Arguments:

    QueueMap - Pointer to the queue map

    LocalCpuIndex - Local index of the processor

Return Value:

    Pointer to the bitfield.

--*/
{
    ASSERT(LocalCpuIndex < QueueMap->MaxProcessors);

    return (PULONG_PTR)((PUINT8)QueueMap + 
                        sizeof(RSSV2_QUEUE_MAP) + 
                        RSSV2_BITFIELD_OFFSET(LocalCpuIndex));
}

FORCEINLINE
PUINT8
RssV2NQEnforceGetReference (
    _In_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    )
/*++
Routine Description:

    Routine finds a pointer to the reference counter for the specified 
    (RSS-)local processor index.

Arguments:

    QueueMap - Pointer to the queue map

    LocalCpuIndex - Local index of the processor

Return Value:

    Pointer to the reference coutner.

--*/
{
    ASSERT(LocalCpuIndex < QueueMap->MaxProcessors);

    return (PUINT8)((PUINT8)QueueMap + 
                    sizeof(RSSV2_QUEUE_MAP) + 
                    RSSV2_BITFIELD_SIZE(QueueMap->MaxProcessors) +
                    RSSV2_REFERENCE_OFFSET(LocalCpuIndex));
}


VOID
RssV2NQEnforcerReset (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap
    )
/*++
Routine Description:

    Routine resets the already initialized QueueMap.

    Queue Map has zero references by any processors.

Arguments:

    QueueMap - Pointer to the queue map

Return Value:

    None.

--*/
{
    RtlZeroMemory(RssV2NQEnforcerGetBitfield(QueueMap, 0),
                  RSSV2_BITFIELD_SIZE(QueueMap->MaxProcessors));

    RtlZeroMemory(RssV2NQEnforceGetReference(QueueMap, 0),
                  RSSV2_REFERENCE_SIZE(QueueMap->MaxProcessors));
}


VOID
RssV2NQEnforcerInitialize (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG MaxNumberOfProcessorsInRssTable
    )
/*++
Routine Description:

    Routine initializes the QueueMap.

Arguments:

    QueueMap - Pointer to the queue map

    MaxNumberOfProcessorsInRssTable - Maximum number of processors which 
                                      RSS table can ever contain.

Return Value:

    None.

--*/
{
    QueueMap->MaxProcessors = MaxNumberOfProcessorsInRssTable;
    KeInitializeSpinLock(&QueueMap->SpinLock);
    RssV2NQEnforcerReset(QueueMap);
}


VOID
RssV2NQEnforcerReference (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    )
/*++
Routine Description:

    Routine marks processor as holding a reference. Corresponding bit in 
    the bitfiled is set to 1, and reference count is incremented.

Arguments:

    QueueMap - Pointer to the queue map

    LocalCpuIndex - Local index of the processor

Return Value:

    None.

--*/
{
    PUINT8 reference;
    PULONG_PTR bitfield;

    if (LocalCpuIndex < QueueMap->MaxProcessors)
    {
        reference = RssV2NQEnforceGetReference(QueueMap, LocalCpuIndex);
        *reference += 1;

        bitfield =  RssV2NQEnforcerGetBitfield(QueueMap, LocalCpuIndex);
        *bitfield |= 1UI64 << (LocalCpuIndex % BITS_PER_WORD);
    }
}


VOID
RssV2NQEnforcerDereference (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    )
/*++
Routine Description:

    Routine removes one reference cause by the processor. Reference count
    is decremented, and if it becomes zero, a corresponding bit in 
    the bitfiled is cleared to 0.

Arguments:

    QueueMap - Pointer to the queue map

    LocalCpuIndex - Local index of the processor

Return Value:

    None.

--*/
{
    PUINT8 reference;
    PULONG_PTR bitfield;

    if (LocalCpuIndex < QueueMap->MaxProcessors)
    {
        reference = RssV2NQEnforceGetReference(QueueMap, LocalCpuIndex);
        *reference -= 1;
        if (*reference == 0)
        {
            bitfield =  RssV2NQEnforcerGetBitfield(QueueMap, LocalCpuIndex);
            *bitfield &= ~(1UI64 << (LocalCpuIndex % BITS_PER_WORD));
        }
    }
}


VOID
RssV2NQEnforcerUpdate (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ UINT8 OldCpuIndex, 
    _In_ UINT8 NewCpuIndex
    )
/*++
Routine Description:

    Routine updates references for steering parameter (when it is re-points
    from one processor to another).

Arguments:

    QueueMap - Pointer to the queue map

    OldCpuIndex - Old local index of the processor used by steering parameter

    NewCpuIndex - New Local index of the processor for the steering parameter

Return Value:

    None.

--*/
{
    if (OldCpuIndex != NewCpuIndex)
    {
        RssV2NQEnforcerReference(QueueMap, NewCpuIndex);
        RssV2NQEnforcerDereference(QueueMap, OldCpuIndex);
    }
}


ULONG
RssV2NQEnforcerGetNumberOfProcs (
    _In_ PRSSV2_QUEUE_MAP QueueMap
    )
/*++
Routine Description:

    Routine counts number of bits set to "1" in the bitfield, this telling
    how many processors are used.

Arguments:

    QueueMap - Pointer to the queue map


Return Value:

    Number of processors used.

--*/
{
    ULONG localCpuIndex;
    ULONG numberOfProcessors;
    PULONG_PTR bitfield;

    numberOfProcessors = 0;

    for (localCpuIndex = 0; 
         localCpuIndex < QueueMap->MaxProcessors; 
         localCpuIndex += BITS_PER_WORD)
    {
        bitfield = RssV2NQEnforcerGetBitfield(QueueMap, localCpuIndex);
        numberOfProcessors += RtlNumberOfSetBitsUlongPtr(*bitfield);
    }

    return numberOfProcessors;
}


VOID
RssV2NQEnforcerEnter(
    _Inout_ PRSSV2_QUEUE_MAP GlobalQueueMap,
    _Inout_ PRSSV2_QUEUE_MAP LocalQueueMap
    )
/*++
Routine Description:

    Routine takes a spinlock and copies Queue Map locally.

Arguments:

    GlobalQueueMap - Pointer to the global queue map (e.g. VPort's) visible by
                     many processors.

    LocalQueueMap - Pointer to the queue map on stack.

Return Value:

    None.

--*/
{
    KeAcquireSpinLockAtDpcLevel(&GlobalQueueMap->SpinLock);
    
    RtlMoveMemory(LocalQueueMap, 
                  GlobalQueueMap, 
                  RssV2NQEnforcerGetQueueMapSize(GlobalQueueMap->MaxProcessors));
}


NDIS_STATUS
RssV2NQEnforcerLeave (
    _Inout_ PRSSV2_QUEUE_MAP GlobalQueueMap,
    _In_ PRSSV2_QUEUE_MAP LocalQueueMap,
    _In_ ULONG QueueLimit
    )
/*++
Routine Description:

    Routine performs "Number of Queues" (NQ-)violation check.

    Usage pattern:

        RssV2NQEnforcerEnter(qm);

            for_all_commands_in_the_current_group()
            {
                RssV2NQEnforcerUpdate(qm, oldCpuIndex, newCpuIndex);
            }

        status = RssV2NQEnforcerLeave(qm);
        if (FAILED(status)
        {
            RssV2SetCommandRangeStatus(&context, status);
        }

        //
        // Proceed to apply to HW.
        //

Arguments:

    GlobalQueueMap - Pointer to the global queue map (e.g. VPort's) visible by
                     many processors. 

    LocalQueueMap - Pointer to the queue map on stack. If NQ-check succeeds, 
                    the global queue map will be updated from the local copy.
                    If NQ-check fails, local copy will be discarded and global
                    copy is unchanged.

    QueueLimit  - Configured maximum number of queues for scaling entity (VPort).

Return Value:

    NDIS_STATUS_SUCCESS - if the accumulated local changes lead to valid 
                         configuration.
    
    NDIS_STATUS_NO_QUEUES - if the accumulated local configuration exceeds 
                            QueueLimit.

--*/
{
    NDIS_STATUS status;

    if (RssV2NQEnforcerGetNumberOfProcs(LocalQueueMap) <= QueueLimit)
    {
        RtlMoveMemory(GlobalQueueMap, 
                      LocalQueueMap, 
                      RssV2NQEnforcerGetQueueMapSize(GlobalQueueMap->MaxProcessors));

        status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        status = NDIS_STATUS_NO_QUEUES;
    }

    KeReleaseSpinLockFromDpcLevel(&GlobalQueueMap->SpinLock);

    return status;
}

