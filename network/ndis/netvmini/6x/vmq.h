/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Vmq.h

Abstract:

   This module declares the VMQ related data types, flags, macros, and functions. 

Revision History:

Notes:

--*/


struct _FRAME;
struct _RCB;
    
#if (NDIS_SUPPORT_NDIS620)


//
// VMQ queue structures
//

//
// Tracks the shared memory segment allocated for indications
//
typedef struct _MP_ADAPTER_SHARED_MEMORY_BLOCK
{
    //
    // MDL for the shared memory segment
    //
    PMDL Mdl;
    //
    // Pointer to shared memory for this block
    //
    PUCHAR                      Buffer;
    //
    // Description of the shared memory block used for the NetBuffer usage
    //
    NET_BUFFER_SHARED_MEMORY    BufferSharedMemoryData;
} MP_ADAPTER_SHARED_MEMORY_BLOCK, *PMP_ADAPTER_SHARED_MEMORY_BLOCK;

//
// Tracks a shared memory buffer allocation, which is then subdivided into individual blocks for indications
//
typedef struct _MP_ADAPTER_SHARED_MEMORY
{
    LIST_ENTRY                      Entry;
    NDIS_HANDLE                     AllocationHandle;
    NDIS_HANDLE                     MemoryHandle;
    PUCHAR                          Buffer;
    ULONG                           EntrySize;
    ULONG                           NumberOfEntries;
} MP_ADAPTER_SHARED_MEMORY, *PMP_ADAPTER_SHARED_MEMORY;

//
// The minimum number of shared memory blocks we require. Used when recovering from 
// shared memory allocation failures. 
//
#define NIC_MIN_RECV_ENTRY_ALLOCATION_COUNT 32

//
// Flags set to individual queues. Any code changing the flag value should acquire the corresponding
// lock.
//
//
// The queue is initialized, but not yet completed. Receives disabled. 
//
#define fMPAQI_INITIALIZED              0x1
//
// Completion of the queue has started. Receives disabled. 
//
#define fMPAQI_COMPLETION_STARTED       0x2
//
// Queue completed. Receives enabled. 
//
#define fMPAQI_COMPLETION_FINISHED      0x4
//
// Queue is being freed (pending RefCount). Receives disabled. 
// 
#define fMPAQI_FREEING                  0x8
//
// DMA is being performed for queue
//
#define fMPAQI_DMA_IN_PROGRESS          0x10
//
// The queue has entered DMA stopped state. No copies should occur to receive memory for this queue.
//
#define fMPAQI_DMA_STOPPED              0x20

#define QUEUE_SET_FLAG(_QueueInfo, _Flag)\
    ((_QueueInfo)->QueueInfoFlags |= _Flag)

#define QUEUE_CLEAR_FLAG(_QueueInfo, _Flag)\
    ((_QueueInfo)->QueueInfoFlags &= ~(_Flag))

#define QUEUE_INITIALIZED(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_INITIALIZED)

#define QUEUE_COMPLETING(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_COMPLETION_STARTED)

#define QUEUE_COMPLETE(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_COMPLETION_FINISHED)

#define QUEUE_FREEING(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_FREEING)

#define QUEUE_DMA_IN_PROGRESS(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_DMA_IN_PROGRESS)

#define QUEUE_DMA_STOPPED(_QueueInfo)\
    ((_QueueInfo)->QueueInfoFlags & fMPAQI_DMA_STOPPED)

//
// The MP_ADAPTER_QUEUE structure is used to track a specific VMQ queue
//
typedef struct DECLSPEC_CACHEALIGN _MP_ADAPTER_QUEUE
{
    //
    // Lock for Read/Write access to QueueInfoFlags and RefCount.
    //
    PNDIS_RW_LOCK_EX QueueLock;
    //
    // Flags used to track status of Queue (fMPAQI_* flags)
    //
    ULONG QueueInfoFlags;

    //
    // DPC used for receives for this queue
    //
    struct _MP_ADAPTER_RECEIVE_DPC *ReceiveDpc;

    //
    // List of unused RCBs (sliced out of RcbMemoryBlock)
    //
    LIST_ENTRY              FreeRcbList;
    NDIS_SPIN_LOCK          FreeRcbListLock;

    //
    // RCB & NBL memory information
    //
    PUCHAR                  RcbMemoryBlock;
    NDIS_HANDLE             RecvNblPoolHandle;
    
    //
    // Shared memory information (MP_ADAPTER_SHARED_MEMORY)
    //
    ULONG                   NumReceiveBuffers;
    //
    // List of MP_ADAPTER_SHARED_MEMORY, large shared memory buffers subdivided for use when receiving
    //
    LIST_ENTRY              LookaheadSharedMemoryList;
    LIST_ENTRY              PostLookaheadSharedMemoryList;
    //
    // MP_ADAPTER_SHARED_MEMORY_BLOCK buffers to hold book-keeping info on subdivided shared memory buffers 
    //
    PUCHAR                  LookaheadBlocks;
    ULONG                   NumLookaheadBlocks;
    PUCHAR                  PostLookaheadBlocks;     
    ULONG                   NumPostLookaheadBlocks;    

    //
    // Data passed in through the VMQ Queue configuration related OIDs
    //
    NDIS_RECEIVE_QUEUE_ID QueueId;
    ULONG NumSuggestedReceiveBuffers;
    ULONG LookaheadSize;
    ULONG NdisFlags;
    GROUP_AFFINITY ProcessorAffinity;
} MP_ADAPTER_QUEUE, *PMP_ADAPTER_QUEUE;

//
// Structure used to store receive filter data
//
typedef struct _MP_ADAPTER_FILTER
{
    //
    // Whether the receive filter should be used 
    //
    BOOLEAN Valid;
    //
    // Filter matching fields
    //
    BOOLEAN VlanUntaggedOrZero;
    USHORT QueueId;
    USHORT VlanId;
    UCHAR MacAddress[NIC_MACADDR_SIZE];
} MP_ADAPTER_FILTER, *PMP_ADAPTER_FILTER;

#define MP_ADAPTER_FILTER_INDEX(_FilterId_)\
    ((_FilterId_)-1)

//
// Global VMQ configuration structures
//

//
// Flags tracking global VMQ state
//
//
// VMQ is enabled on the adapter.
//
#define fMPVMQD_FILTERING_ENABLED       0x0001
//
// Lookahead split in VMQ indication is enabled on the adapter. 
//
#define fMPVMQD_LOOKAHEAD_ENABLED       0x0002
//
// VLAN filtering in VMQ is enabled on the adapter. 
//
#define fMPVMQD_VLANFILTER_ENABLED      0x0004

#define VMQ_SET_FLAG(_Adapter, _Flag) \
    ((_Adapter)->VMQData.Flags |= (_Flag))

#define VMQ_ENABLED(_Adapter) \
        ((_Adapter)->VMQData.Flags & fMPVMQD_FILTERING_ENABLED)
#define LOOKAHEAD_SPLIT_ENABLED(_Adapter)\
        ((_Adapter)->VMQData.Flags & fMPVMQD_LOOKAHEAD_ENABLED)
#define VLAN_FILTER_ENABLED(_Adapter)\
        ((_Adapter)->VMQData.Flags & fMPVMQD_VLANFILTER_ENABLED)

#define LOOKAHEAD_SPLIT_REQUIRED(_QueueInfo)\
    ((_QueueInfo)->NdisFlags & NDIS_RECEIVE_QUEUE_PARAMETERS_LOOKAHEAD_SPLIT_REQUIRED)

//
// The MP_ADAPTER_VMQ_DATA structure is used to track the global VMQ configuration for an adapter
//        
typedef struct _MP_ADAPTER_VMQ_DATA
{
    //
    // Tracks global VMQ state (fMPVMQD_* flags)
    //
    ULONG Flags;
    //
    // Individual Queues. The MP_ADAPTER_QUEUE array is not dynamically allocated to reduce
    // pointer dereferencing during receives, which can affect performance. 
    //
    MP_ADAPTER_QUEUE RxQueues[NIC_SUPPORTED_NUM_QUEUES];
    //
    // Filters used to match packets to Queues
    //
    MP_ADAPTER_FILTER RxFilters[NIC_MAX_HEADER_FILTERS];
} MP_ADAPTER_VMQ_DATA, *PMP_ADAPTER_VMQ_DATA;

NDIS_STATUS
AllocateVMQData(
    _Inout_ struct _MP_ADAPTER *Adapter);

VOID
FreeVMQData(
    _Inout_ struct _MP_ADAPTER *Adapter);

NDIS_STATUS 
ReadRxQueueConfig(
    _In_ NDIS_HANDLE ConfigurationHandle,
    _Inout_ struct _MP_ADAPTER *Adapter);

NDIS_STATUS
InitializeRxQueueMPConfig(
    _Inout_ struct _MP_ADAPTER *Adapter);

NDIS_STATUS
AllocateDefaultRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter);


NDIS_STATUS
AllocateRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams
    );

NDIS_STATUS
CompleteAllocationRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY CompleteArray
    );

NDIS_STATUS
UpdateRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams
);

NDIS_STATUS
FreeRxQueue(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_QUEUE_FREE_PARAMETERS QueueParams,
    _In_opt_ PNDIS_OID_REQUEST   NdisSetRequest
);

VOID
IndicateRxQueue(
           _In_ NDIS_HANDLE AdapterHandle,
           NDIS_RECEIVE_QUEUE_ID QueueId,
           NDIS_RECEIVE_QUEUE_OPERATIONAL_STATE State);

NDIS_STATUS
SetRxFilter(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_FILTER_PARAMETERS FilterParams
    );

NDIS_STATUS
ClearRxFilter( 
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS FilterParams
    );

BOOLEAN
AcquireRxQueueReference(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) USHORT QueueId,
    BOOLEAN RequireComplete
    );

VOID
ReleaseRxQueueReference(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) USHORT QueueId
   );

VOID
SetPendingRxQueueFree(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_OID_REQUEST NdisSetRequest
    );

VOID
GetRcbForRxQueue(
    _In_  struct _MP_ADAPTER *Adapter,
    _In_  struct _FRAME *Frame,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _Outptr_result_maybenull_ struct _RCB **Rcb);

NDIS_STATUS 
CopyFrameToRxQueueRcb(
    _In_  struct _MP_ADAPTER *Adapter,
    _In_  struct _FRAME *Frame,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _Inout_ struct _RCB *Rcb,
    _Out_ BOOLEAN *Copied);
    
VOID
RecoverRxQueueRcb(    
    _In_ struct _MP_ADAPTER *Adapter,
    _In_ struct _RCB *Rcb);

VOID
AddPendingRcbToRxQueue(
    _In_ struct _MP_ADAPTER *Adapter,
    _In_ struct _RCB *Rcb);

#define GetRxQueueDpc(Adapter, QueueId) (Adapter)->VMQData.RxQueues[(QueueId)].ReceiveDpc

#else

//
// In order to avoid excessible "#if defined(NDIS620_MINIPORT)" statements scattered 
// through the miniport implementation, NDIS60 miniports define 
// placeholder macros for the VMQ functions which cause the code to always proceed 
// as if VMQ were disabled on the adapter. 
//

#define VMQ_ENABLED(_Adapter) FALSE
#define LOOKAHEAD_SPLIT_ENABLED(_Adapter) FALSE
#define VLAN_FILTER_ENABLED(_Adapter) FALSE
#define LOOKAHEAD_SPLIT_REQUIRED(_QueueInfo) FALSE
#define AllocateDefaultRxQueue(Adapter) NDIS_STATUS_NOT_SUPPORTED
#define AddPendingRcbToRxQueue(Adapter, Rcb) 
#define GetRxQueueDpc(Adapter, QueueId) NULL
#define AllocateVMQData(Adapter) NDIS_STATUS_SUCCESS
#define FreeVMQData(Adapter)
#define ReadRxQueueConfig(ConfigurationHandle, Adapter) NDIS_STATUS_SUCCESS
#define InitializeRxQueueMPConfig(Adapter) NDIS_STATUS_SUCCESS
#define CopyFrameToRxQueueRcb(Adapter, Frame, Nbl1QInfo, Rcb, Copied) FALSE
#define GetRcbForRxQueue(Adapter, Frame, Nbl1QInfo, Rcb) NDIS_STATUS_NOT_SUPPORTED
#define RecoverRxQueueRcb(Adapter, Rcb) 

#endif
