/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:
	Stream Edit Callout Driver Sample.

	This sample demonstrates finding and replacing a string pattern from a
	live TCP stream via the WFP stream API.

Environment:
	Kernel mode

--*/

#ifndef _STREAM_EDIT_H
#define _STREAM_EDIT_H

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include <ntifs.h>
#include <wdf.h>

#include <ntddk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union
#include <fwpsk.h>
#pragma warning(pop)

#pragma warning(push)
#pragma prefast(disable: 26061) // warning 26061: Range postcondition violation
#pragma prefast(disable: 28196) // RtlUnicodeStringInitWorker restriction
#include <ntstrsafe.h>
#pragma warning(pop)

#include <fwpmk.h>
#include "LwQueue.h"

//
// Pool Tags used for allocations
//
#define STMEDIT_TAG_NDIS_OBJ        'oNeS'
#define STMEDIT_TAG_NBL_POOL        'pNeS'
#define STMEDIT_TAG_FLAT_BUFFER     'bSeS'
#define STMEDIT_TAG_FLOWCTX         'cFeS'
#define STMEDIT_TAG_TASK_ENTRY      'eTeS'
#define STMEDIT_TAG_MDL_DATA        'dMeS'

#define CFG_LOCAL_PORT              8888
#define STR_MAX_SIZE                 128
#define NUM_WORKITEM_QUEUES            2
#define INVALID_PROC_NUMBER           -1

#pragma warning(disable: 4127)  // conditional expression is constant -- for do-while(true/false) loops!

//
// Inline editing states of a stream
//
typedef enum _INLINE_EDIT_STATE
{
    INLINE_EDIT_IDLE = 0,
    INLINE_EDIT_SKIPPING,
    INLINE_EDIT_MODIFYING,
    INLINE_EDIT_SCANNING
} INLINE_EDIT_STATE;

//
// Out-Of-Band editing states of a stream
//
typedef enum _OOB_EDIT_STATE
{
    OOB_EDIT_IDLE = 0,
    OOB_EDIT_PROCESSING,
    OOB_EDIT_BUSY,
    OOB_EDIT_ERROR
} OOB_EDIT_STATE;

typedef struct _OUTGOING_STREAM_DATA
{
    // Link for placement on Queue.
    LIST_ENTRY Link;

    // NetBuffer List containing data
    NET_BUFFER_LIST* NetBufferList;

    // If the NetBufferList is a clone or did we allocate it
    BOOLEAN isClone;

    // Valid data length 
    size_t DataLength;

    // Stream Flags to be used when injecting this segment of data
    DWORD StreamFlags;

    MDL* Mdl;
} OUTGOING_STREAM_DATA, *POUTGOING_STREAM_DATA;

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union
#pragma warning(disable:4214)       // nonstandard extension used : bit field types other than int

//
// Flow Context associated with a flow
//
typedef struct _STREAM_FLOW_CONTEXT
{
    // for putting on the global FlowContest list
    LIST_ENTRY Link;

    union
    {
        // Inline data editting state
        struct
        {
            // State of inline stream edit processing
            INLINE_EDIT_STATE InlineEditState;

            // Current Processor on which inline classify function is invoked!
            volatile LONG CurrentProcessor;
        };

        struct
        {
            // Internal Stream State for OoB Processing
            OOB_EDIT_STATE EditState;

            // Spin lock to sync editing
            KSPIN_LOCK EditLock;

            // Processed data ready to be injected back into the stream.
            LIST_ENTRY OutgoingDataQueue;

            // Index to LW Queue, assigned to the flow.
            ULONG QueueNumber;

            // Length of data classified, but not yet processed.
            volatile size_t PendedDataLength;

            // Number of data processing tasks still queued
            volatile ULONG PendingTasks;

            // Flags that specify characteristics of the data stream.
            //
            // A callout driver should specify the same stream flags that
            // were set in the 'flags' member of the _STREAM_DATA structure
            // that the filter engine passed to the callout driver's
            // classifyFn callout function when the callout deferred the
            // data stream.
            //
            UINT32 StreamFlags;

        } OobInfo;
    };

    // Ref count - # of unprocessed task
    volatile ULONG  RefCount;

    // IVP4 or V6 ? For troubleshooting only
    UINT16 IpProto;

    // Stream Flags for partially matching data.
    UINT32 PartialSFlags;

    //
    // For copying stream data to a (flat)buffer for inspection
    //
    PVOID  ScratchBuffer;
    size_t ScratchBufferSize;
    size_t ScratchDataOffset;
    size_t ScratchDataLength;

    // 
    // Flow information for data (re-)injection.
    //

    UINT64 FlowHandle;
    UINT16 LayerId;
    UINT32 CalloutId;
    
    volatile char  bFlowActive;     // If the Flow is still active. true @FlowEstablished, false @FlowDelete

    BOOLEAN bFlowTerminating : 1;   // FIN/RST has been received.
    BOOLEAN bNoMoreData : 1;        // classifyOut->flags has FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA bit set
    BOOLEAN bEntryRemoved : 1;      // True when the context is not on the 'global context list'.
    BOOLEAN bEditInline : 1;
    BOOLEAN bUnused : 4;

} STREAM_FLOW_CONTEXT, *PSTREAM_FLOW_CONTEXT;

#pragma warning(pop)


// Task Entry structure
// For dispatching tasks to threads!
//
typedef struct _TASK_ENTRY
{
    // Link for placement on Queues
    LW_ENTRY LwQLink;

    // Target Flow Context
    PSTREAM_FLOW_CONTEXT FlowCtx;

    // NetBufferList Chain to be processed
    NET_BUFFER_LIST *NetBufferList;

    // Length of NBL Chain
    size_t DataLength;

    // Flags..
    DWORD StreamFlags;

} TASK_ENTRY, *PTASK_ENTRY;

//
// Stream Editor Globals block 
//
typedef struct _STMEDIT_GLOBALS
{
    // Lookaside list for Task entries and OutgoingData structs.
    // On 64-bit platforms, this structure must be 16-byte aligned.
    //
    DECLSPEC_CACHEALIGN LOOKASIDE_LIST_EX LookasideList;


    // ... For Callout #1
    //
    // IPV4 callout for FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 layer
    UINT32 FlowEstablishedV4Callout1;

    // IPV4 callout for FWPM_LAYER_STREAM_V4 layer
    UINT32 StreamLayerV4Callout1;

    // IPV4 callout for FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6 layer
    UINT32 FlowEstablishedV6Callout1;

    // IPV4 callout for FWPM_LAYER_STREAM_V6 layer
    UINT32 StreamLayerV6Callout1;

    //
    // ...For Callout #2
    //
    // IPV4 callout for FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 layer
    UINT32 FlowEstablishedV4Callout2;

    // IPV4 callout for FWPM_LAYER_STREAM_V4 layer
    UINT32 StreamLayerV4Callout2;

    // IPV4 callout for FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6 layer
    UINT32 FlowEstablishedV6Callout2;

    // IPV4 callout for FWPM_LAYER_STREAM_V6 layer
    UINT32 StreamLayerV6Callout2;

    // WFP injection handles
    HANDLE InjectionHandle;

    // WFP Engine handle
    HANDLE EngineHandle;

    // NDIS uses a generic object to manage resources that are allocated
    // by a component that does not otherwise have an NDIS handle.
    NDIS_GENERIC_OBJECT* NdisGenericObj;

    // To allocate NetBuffer and NetBufferLists for injection
    NDIS_HANDLE NetBufferListPool;

    // MDL chain used to initialize the preallocated NET_BUFFER
    // structure for injecting replacement string.
    MDL* StringToReplaceMdl;
    MDL* StringXMdl;

    // Number of flow-context structures allocated (mostly for troubleshooting)
    ULONG FlowContextCount;

    // A linked list of flow-context structures
    LIST_ENTRY FlowContextList;

    // Spin lock for synchronizing FlowContextList access
    KSPIN_LOCK FlowContextListLock;

    // Event to signal if Flow Context count is zero!
    KEVENT ZeroFlowCountEvent;

    DEVICE_OBJECT* WdmDevice;   // Device object for StreamEdit

    // 
    // Configurable parameters block
    //

    // Register two sets of callouts ?
    BOOLEAN MultipleCallouts;

    // Local port for inspecting network traffic
    USHORT  InspectionLocalPort;

    // Remote port for inspecting network traffic
    USHORT  InspectionRemotePort;

    // What traffic direction to inspect
    UCHAR   InspectionDirection;

    // Busy Threshold (to Defer/Resume stream)
    size_t  BusyThreshold;

    // String to search for in the stream
    CHAR StringToFind[STR_MAX_SIZE];

    // Replacement string!
    CHAR StringX[STR_MAX_SIZE];

    // Replacement string!
    CHAR StringToReplace[STR_MAX_SIZE];

    // Length of StringToFind
    size_t StringToFindLength;

    // Length of StringToReplace
    size_t StringXLength;

    // Length of StringToReplace
    size_t StringToReplaceLength;

    // True if the driver is unloading/shutting down
    volatile char DriverUnloading;

    // To generate workitem queue array index to be associated to a flow.
    volatile ULONG QueueIndex;

    // Queues for processing task workitems
    LW_QUEUE ProcessingQueues[NUM_WORKITEM_QUEUES];

    // True if TaskEntry look aside list is successfully initialized.
    BOOLEAN LookasideCreated;

} STMEDIT_GLOBALS;


extern STMEDIT_GLOBALS      Globals;

typedef
VOID
WFP_CLASSIFY_FUNCTION(
    _In_ const FWPS_INCOMING_VALUES*,
    _In_ const FWPS_INCOMING_METADATA_VALUES*,
    _In_ PVOID,
#if(NTDDI_VERSION >= NTDDI_WIN7)
    _In_ const void*,
#endif  
    _In_ const FWPS_FILTER*,
    _In_ UINT64,
    _Inout_ FWPS_CLASSIFY_OUT*
);


IO_WORKITEM_ROUTINE StreamEditOobPoolWorker;

NTSTATUS
StreamEditInitializeWorkitemPool(
    );

NTSTATUS
StreamOobFlushOutgoingData(
    _Inout_ PSTREAM_FLOW_CONTEXT
    );

NTSTATUS
StreamEditRemoveFlowCtx(
    _In_ STREAM_FLOW_CONTEXT*
    );

VOID
StreamEditSignalShutdown(
    );

BOOLEAN
StreamEditCopyDataForInspection(
    _In_ PSTREAM_FLOW_CONTEXT,
    _In_ const FWPS_STREAM_DATA*,
    _In_ SIZE_T BytesToCopy
    );

VOID
NTAPI
StreamEditInjectCompletionFn(
    _In_ VOID*,
    _Inout_ NET_BUFFER_LIST*,
    _In_ BOOLEAN
    );

VOID
NTAPI
StreamOobCloneInjectCompletionFn(
    _In_ VOID*,
    _Inout_ NET_BUFFER_LIST*,
    _In_ BOOLEAN
    );

NTSTATUS
OobEditCreateThread(
    _Inout_ PSTREAM_FLOW_CONTEXT
    );

VOID
InlineEditClassify(
    _In_ const FWPS_INCOMING_VALUES*,
    _In_ const FWPS_INCOMING_METADATA_VALUES*,
    _Inout_ PVOID,
    _In_ const FWPS_FILTER*,
    _In_ UINT64,
    _Inout_ FWPS_CLASSIFY_OUT*
    );

VOID
OobEditClassify(
    _In_ const FWPS_INCOMING_VALUES*,
    _In_ const FWPS_INCOMING_METADATA_VALUES*,
    _Inout_ PVOID,
    _In_ const FWPS_FILTER*,
    _In_ UINT64,
    _Inout_ FWPS_CLASSIFY_OUT*
    );

WFP_CLASSIFY_FUNCTION StreamEditFlowEstablishedClassify;
WFP_CLASSIFY_FUNCTION StreamEditCommonStreamClassify;

NTSTATUS
InlineEditFlushData(
    _In_ PSTREAM_FLOW_CONTEXT,
    _In_ ULONG,
    _In_ UINT
    );

FORCEINLINE
VOID
StreamEditFreeFlowCtxCommon(
    _Inout_ PSTREAM_FLOW_CONTEXT
    );

VOID
StmEditReferenceFlow(
    _Inout_ PSTREAM_FLOW_CONTEXT,
    _In_    char,
    _In_    UINT
    );

VOID
StmEditDeReferenceFlow(
    _Inout_ PSTREAM_FLOW_CONTEXT,
    _In_    char,
    _In_    UINT
    );

FORCEINLINE
ULONG NetBufferListLength(PNET_BUFFER_LIST Nbl)
{
    ULONG Length = 0;
    PNET_BUFFER nb = NET_BUFFER_LIST_FIRST_NB(Nbl);

    while (nb)
    {
        Length += NET_BUFFER_DATA_LENGTH(nb);
        nb = NET_BUFFER_NEXT_NB(nb);
    }

    return Length;
}

// 
// Callout driver keys
//

/* 524F4849-5420-5241-494E-41205A9EEF0B */
__declspec (selectany) const GUID STREAM_EDITOR_SUBLAYER_1 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0A }
};

/* 524F4849-5420-5241-494E-41205A9EEF0B */
__declspec (selectany) const GUID STREAM_EDITOR_SUBLAYER_2 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0B }
};


/* 524F4849-5420-5241-494E-41205A9EEF0C */
__declspec (selectany) const GUID STREAM_EDITOR_STREAM_CALLOUT_V4 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0C }
};

/* 524F4849-5420-5241-494E-41205A9EEF0D */
__declspec (selectany) const GUID STREAM_EDITOR_STREAM_CALLOUT_V6 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0D }
};

/* 524F4849-5420-5241-494E-41205A9EEF0E */
__declspec (selectany) const GUID STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V4 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0E }
};

/* 524F4849-5420-5241-494E-41205A9EEF0F */
__declspec (selectany) const GUID STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V6 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x0F }
};

/* 524F4849-5420-5241-494E-41205A9EEF01 */
__declspec (selectany) const GUID STREAM_EDITOR_STREAM_CALLOUT_V4_2 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x01 }
};

/* 524F4849-5420-5241-494E-41205A9EEF02 */
__declspec (selectany) const GUID STREAM_EDITOR_STREAM_CALLOUT_V6_2 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x02 }
};

/* 524F4849-5420-5241-494E-41205A9EEF03 */
__declspec (selectany) const GUID STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V4_2 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x03 }
};

/* 524F4849-5420-5241-494E-41205A9EEF04 */
__declspec (selectany) const GUID STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V6_2 = {
    0x524F4849,
    0x5420,
    0x5241,
    { 0x49, 0x4E, 0x41, 0x20, 0x5A, 0x9E, 0xEF, 0x04 }
};

// {C1EA91DC-A37F-453E-BFD5-EF68E36EEDAA}
__declspec (selectany) const GUID WFP_DRIVER_CLASS_GUID =
{ 0xc1ea91dc, 0xa37f, 0x453e,{ 0xbf, 0xd5, 0xef, 0x68, 0xe3, 0x6e, 0xed, 0xaa } };


#endif // _STREAM_EDIT_H
