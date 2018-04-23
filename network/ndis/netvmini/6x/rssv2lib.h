/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    RSSv2Lib.h

Abstract:

   This module declares the NDIS RSSv2 helper data types, flags, macros, and functions.

Revision History:

--*/

#pragma once

//
// Temporary workaround until new definition reaches WDK.
//
#if !defined(NDIS_STATUS_NO_QUEUES)
#define NDIS_STATUS_NO_QUEUES       0xC0230031L
#endif 

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _RSSV2_PARSING_CONTEXT
{
    PNDIS_RSS_SET_INDIRECTION_ENTRIES RssV2Oid;
    ULONG MaxIndex;
    ULONG StartIndex;
    ULONG LimitIndex;
    ULONG LastStartIndex;
    BOOLEAN IsNativeRss;

} RSSV2_PARSING_CONTEXT, *PRSSV2_PARSING_CONTEXT;


#define RSSV2_GET_COMMAND(_RssV2Oid_,_Index_) \
        (PNDIS_RSS_SET_INDIRECTION_ENTRY)   \
        ((PUCHAR)(_RssV2Oid_) + (_RssV2Oid_)->RssEntryTableOffset + \
                    (_Index_) * (_RssV2Oid_)->RssEntrySize)

extern
FORCEINLINE
VOID
RssV2InitializeParsingContext (
    _Out_ PRSSV2_PARSING_CONTEXT Context,
    _In_ PNDIS_RSS_SET_INDIRECTION_ENTRIES RssV2Oid,
    _In_ BOOLEAN IsNativeRss
    );

_Success_(return != FALSE)
BOOLEAN
RssV2GetNextCommandRange (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _Out_ ULONG* SwitchId,
    _Out_ ULONG* VPortId
    );

VOID
RssV2RestartRangeIterator (
    _Inout_ PRSSV2_PARSING_CONTEXT Context
    );

PNDIS_RSS_SET_INDIRECTION_ENTRY
RssV2GetNextCommand (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _In_ BOOLEAN SkipProcessedCommands
    );

VOID
RssV2RestartCommandIterator (
    _Inout_ PRSSV2_PARSING_CONTEXT Context
    );

VOID
RssV2SetCommandRangeStatus (
    _Inout_ PRSSV2_PARSING_CONTEXT Context,
    _In_ NDIS_STATUS Status
    );

//
// Conditionally include support for example RSSV2_QUEUE_MAP implementation.
// Miniports don't have to use this implementation (as they may have different
// locking schema)
//
#if defined(RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE) && (RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE != 0)

//
// Queue map for VPort (or for adapter's in NativeRSS mode)
// 
#define BITS_PER_WORD                           (sizeof(ULONG_PTR) * 8)
#define RSSV2_BITFIELD_OFFSET(_PROC_INDEX_)     ((_PROC_INDEX_) / BITS_PER_WORD)
#define RSSV2_BITFIELD_SIZE(_MAX_PROC_) \
            sizeof(ULONG_PTR) * (1 + RSSV2_BITFIELD_OFFSET((_MAX_PROC_) - 1))

#define RSSV2_REFERENCE_OFFSET(_PROC_INDEX_)    ((_PROC_INDEX_) * sizeof(UINT8))
#define RSSV2_REFERENCE_SIZE(_MAX_PROC_)        ((_MAX_PROC_) * sizeof(UINT8))

typedef struct _RSSV2_QUEUE_MAP
{
    //
    // Members to help enforce "NQ-violation" (per-VPort limit on number of queues)
    //
    KSPIN_LOCK  SpinLock;    

    //
    // Maximum number of processors in adapter's RSS table.
    //
    ULONG MaxProcessors;
    ULONG ReservedField;

    //
    // Two variable-size fields follow this structure:
    //
    //  - Bitmask of referenced processors
    //  - Array with reference counts for each RSS processor 
    //    (indexed by a local CPU index, which is relative to RSS table).
    //
    //
    // ULONG_PTR ReferenceBitmask[];
    // UINT8 ReferenceCount[];

} RSSV2_QUEUE_MAP, *PRSSV2_QUEUE_MAP;


#define DECLARE_RSSV2_QUEUE_MAP_ON_STACK(_LOCAL_NAME_, _MAX_PROCS_) \
            __pragma(warning(suppress: 6255)) \
            PRSSV2_QUEUE_MAP _LOCAL_NAME_ = \
            (PRSSV2_QUEUE_MAP)_alloca(RssV2NQEnforcerGetQueueMapSize(_MAX_PROCS_))

FORCEINLINE
ULONG 
RssV2NQEnforcerGetQueueMapSize (
    _In_ ULONG MaxNumberOfProcessorsInRssTable
    )
{
    return sizeof(RSSV2_QUEUE_MAP) + 
            RSSV2_BITFIELD_SIZE(MaxNumberOfProcessorsInRssTable) +
            RSSV2_REFERENCE_SIZE(MaxNumberOfProcessorsInRssTable);
}

VOID
RssV2NQEnforcerReset (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap
    );

VOID
RssV2NQEnforcerInitialize (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG MaxNumberOfProcessorsInRssTable
    );

VOID
RssV2NQEnforcerReference (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    );

VOID
RssV2NQEnforcerDereference (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ ULONG LocalCpuIndex
    );

VOID
RssV2NQEnforcerUpdate (
    _Inout_ PRSSV2_QUEUE_MAP QueueMap,
    _In_ UINT8 OldCpuIndex, 
    _In_ UINT8 NewCpuIndex
    );

ULONG
RssV2NQEnforcerGetNumberOfProcs (
    _In_ PRSSV2_QUEUE_MAP QueueMap
    );

VOID
RssV2NQEnforcerEnter(
    _Inout_ PRSSV2_QUEUE_MAP GlobalQueueMap,
    _Inout_ PRSSV2_QUEUE_MAP LocalQueueMap
    );

NDIS_STATUS
RssV2NQEnforcerLeave (
    _Inout_ PRSSV2_QUEUE_MAP GlobalQueueMap,
    _In_ PRSSV2_QUEUE_MAP LocalQueueMap,
    _In_ ULONG QueueLimit
    );

#endif // defined(RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE) && (RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE != 0)

#ifdef __cplusplus
}
#endif
