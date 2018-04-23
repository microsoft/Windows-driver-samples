/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Adapter.H

Abstract:

    This module contains structure definitons and function prototypes.

Revision History:

Notes:

--*/




//
// Utility macros
// -----------------------------------------------------------------------------
//

#define MP_SET_FLAG(_M, _F)             ((_M)->Flags |= (_F))
#define MP_CLEAR_FLAG(_M, _F)           ((_M)->Flags &= ~(_F))
#define MP_TEST_FLAG(_M, _F)            (((_M)->Flags & (_F)) != 0)
#define MP_TEST_FLAGS(_M, _F)           (((_M)->Flags & (_F)) == (_F))

#define MP_IS_READY(_M)        (((_M)->Flags &                             \
                                 (fMP_DISCONNECTED                         \
                                    | fMP_RESET_IN_PROGRESS                \
                                    | fMP_ADAPTER_HALT_IN_PROGRESS         \
                                    | fMP_ADAPTER_PAUSE_IN_PROGRESS        \
                                    | fMP_ADAPTER_PAUSED                   \
                                    | fMP_ADAPTER_LOW_POWER                \
                                    )) == 0)

//
// Each receive DPC is tracked by this structure, kept in a list in the MP_ADAPTER structure.
// It tracks the DPC object, the processor affinity of the DPC, and which MP_ADAPTER_RECEIVE_BLOCK
// indexes the DPC should consume for receive NBLs.
//
typedef struct _MP_ADAPTER_RECEIVE_DPC
{
    LIST_ENTRY Entry;
    //
    // Kernel DPC used for recieve
    //
    KDPC Dpc;
    USHORT ProcessorGroup;
    ULONG ProcessorNumber;

    //
    // Tracks which receive blocks need to be recieved on this DPC.
    //
    BOOLEAN RecvBlock[NIC_SUPPORTED_NUM_QUEUES];
    volatile LONG RecvBlockCount;

    //
    // Sets up the maximum amount of NBLs that can be indicated by a single
    // receive block. This is initially NIC_MAX_RECVS_PER_INDICATE.
    //
    ULONG MaxNblCountPerIndicate;

    //
    // Work item used if we need to avoid DPC timeout
    //
    NDIS_HANDLE WorkItem;
    volatile LONG WorkItemQueued;

    //
    // Pointer back to owner Adapter structure (accesed within work item)
    //
    struct _MP_ADAPTER *Adapter;

} MP_ADAPTER_RECEIVE_DPC, * PMP_ADAPTER_RECEIVE_DPC;

//
// This structure is used to track pending receives on the adpater (consumed by receive DPCs).
// One receive block maintained for each VMQ queue (if enabled), otherwise
// a single structure is used to track receives on the adapter.
//
typedef struct DECLSPEC_CACHEALIGN _MP_ADAPTER_RECEIVE_BLOCK
{
    //
    // List of pending RCB blocks that need to be indicated up to NDIS
    //
    LIST_ENTRY ReceiveList;
    NDIS_SPIN_LOCK ReceiveListLock;
    volatile LONG PendingReceives;
} MP_ADAPTER_RECEIVE_BLOCK, * PMP_ADAPTER_RECEIVE_BLOCK;

//
// Each adapter managed by this driver has a MP_ADAPTER struct.
//
typedef struct _MP_ADAPTER
{
    LIST_ENTRY              List;

    //
    // Keep track of various device objects.
    //
    PDEVICE_OBJECT          Pdo;
    PDEVICE_OBJECT          Fdo;
    PDEVICE_OBJECT          NextDeviceObject;

    NDIS_HANDLE             AdapterHandle;


    //
    // Status flags
    //

#define fMP_RESET_IN_PROGRESS               0x00000001
#define fMP_DISCONNECTED                    0x00000002
#define fMP_ADAPTER_HALT_IN_PROGRESS        0x00000004
#define fMP_ADAPTER_PAUSE_IN_PROGRESS       0x00000010
#define fMP_ADAPTER_PAUSED                  0x00000020
#define fMP_ADAPTER_SURPRISE_REMOVED        0x00000100
#define fMP_ADAPTER_LOW_POWER               0x00000200

    ULONG                   Flags;


    UCHAR                   PermanentAddress[NIC_MACADDR_SIZE];
    UCHAR                   CurrentAddress[NIC_MACADDR_SIZE];

    //
    // Send tracking
    // -------------------------------------------------------------------------
    //

    // Pool of unused TCBs
    PVOID                   TcbMemoryBlock;

    // List of unused TCBs (sliced out of TcbMemoryBlock)
    LIST_ENTRY              FreeTcbList;
    NDIS_SPIN_LOCK          FreeTcbListLock;

    // List of net buffers to send that are waiting for a free TCB
    LIST_ENTRY              SendWaitList;
    NDIS_SPIN_LOCK          SendWaitListLock;

    // List of TCBs that are being read by the NIC hardware
    LIST_ENTRY              BusyTcbList;
    NDIS_SPIN_LOCK          BusyTcbListLock;

    // A DPC that simulates interrupt processing for send completes
    NDIS_HANDLE             SendCompleteTimer;

    //
    // Work item used if we need to avoid DPC timeout
    //
    NDIS_HANDLE             SendCompleteWorkItem;
    volatile LONG           SendCompleteWorkItemQueued;
    volatile BOOLEAN        SendCompleteWorkItemRunning;


    // Number of transmit NBLs from the protocol that we still have
    volatile LONG           nBusySend;

    // Spin lock to ensure only one CPU is sending at a time
    KSPIN_LOCK              SendPathSpinLock;


    //
    // Receive tracking
    // -------------------------------------------------------------------------
    //

    // Pool of unused RCBs
    PVOID                   RcbMemoryBlock;

    // List of unused RCBs (sliced out of RcbMemoryBlock)
    LIST_ENTRY              FreeRcbList;
    NDIS_SPIN_LOCK          FreeRcbListLock;

    NDIS_HANDLE             RecvNblPoolHandle;

    //
    // List of receive DPCs allocated for various ProcessorAffinity values (if only
    // one needed, then only default is present in the list
    //
    LIST_ENTRY               RecvDpcList;
    NDIS_SPIN_LOCK           RecvDpcListLock;
    PMP_ADAPTER_RECEIVE_DPC  DefaultRecvDpc;

    //
    // Async pause and reset tracking
    // -------------------------------------------------------------------------
    //
    NDIS_HANDLE             AsyncBusyCheckTimer;
    LONG                    AsyncBusyCheckCount;


    //
    // NIC configuration
    // -------------------------------------------------------------------------
    //
    ULONG                   PacketFilter;
    ULONG                   ulLookahead;
    ULONG64                 ulLinkSendSpeed;
    ULONG64                 ulLinkRecvSpeed;
    ULONG                   ulMaxBusySends;
    ULONG                   ulMaxBusyRecvs;

    // multicast list
    ULONG                   ulMCListSize;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][NIC_MACADDR_SIZE];


    //
    // Statistics
    // -------------------------------------------------------------------------
    //

    // Packet counts
    ULONG64                 FramesRxDirected;
    ULONG64                 FramesRxMulticast;
    ULONG64                 FramesRxBroadcast;
    ULONG64                 FramesTxDirected;
    ULONG64                 FramesTxMulticast;
    ULONG64                 FramesTxBroadcast;

    // Byte counts
    ULONG64                 BytesRxDirected;
    ULONG64                 BytesRxMulticast;
    ULONG64                 BytesRxBroadcast;
    ULONG64                 BytesTxDirected;
    ULONG64                 BytesTxMulticast;
    ULONG64                 BytesTxBroadcast;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;
    ULONG                   TransmitFailuresOther;

    // Count of receive errors
    ULONG                   RxCrcErrors;
    ULONG                   RxAlignmentErrors;
    ULONG                   RxResourceErrors;
    ULONG                   RxDmaOverrunErrors;
    ULONG                   RxCdtFrames;
    ULONG                   RxRuntErrors;

    //
    // Reference to the allocated root of MP_ADAPTER memory, which may not be cache aligned.
    // When allocating, the pointer returned will be UnalignedBuffer + an offset that will make
    // the base pointer cache aligned.
    //
    PVOID                   UnalignedAdapterBuffer;
    ULONG                   UnalignedAdapterBufferSize;

    //
    // Tracks any pending NBLs for the particular receiver (either
    // 0 for non-VMQ scenarios, or the corresponding VMQ queue). These
    // are consumed by the receive DPCs.
    //
    MP_ADAPTER_RECEIVE_BLOCK ReceiveBlock[NIC_SUPPORTED_NUM_QUEUES];

    //
    // An OID request that could not be fulfulled at the time of the call. These OIDs are serialized
    // so we will not receive new queue management OID's until this one is complete.
    // Currently this is used only for freeing a Queue (which may still have outstanding references)
    //
    PNDIS_OID_REQUEST PendingRequest;

    NDIS_DEVICE_POWER_STATE CurrentPowerState;

#if (NDIS_SUPPORT_NDIS620)

    //
    // VMQ related data
    //
    MP_ADAPTER_VMQ_DATA     VMQData;

#endif

#if (NDIS_SUPPORT_NDIS630)

    //
    // NDIS QoS related data
    //
    MP_ADAPTER_QOS_DATA     QOSData;

#endif

#if (NDIS_SUPPORT_NDIS680)

    //
    // NDIS RSSv2-related data
    //
    MP_ADAPTER_RSS_DATA     RSSData;

#endif

} MP_ADAPTER, *PMP_ADAPTER;

#define MP_ADAPTER_FROM_CONTEXT(_ctx_) ((PMP_ADAPTER)(_ctx_))

PMP_ADAPTER_RECEIVE_DPC
NICAllocReceiveDpc(
    _In_ PMP_ADAPTER      Adapter,
    ULONG ProcessorNumber,
    USHORT  ProcessorGroup,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

VOID
NICReceiveDpcRemoveOwnership(
    _In_ PMP_ADAPTER_RECEIVE_DPC ReceiveDpc,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

PMP_ADAPTER_RECEIVE_DPC
NICGetDefaultReceiveDpc(
    _In_ PMP_ADAPTER      Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

NDIS_STATUS
NICAllocRCBData(
    _In_ PMP_ADAPTER Adapter,
    ULONG NumberOfRcbs,
    _Outptr_result_bytebuffer_(NumberOfRcbs * sizeof(RCB)) PVOID *RcbMemoryBlock,
    _Inout_ PLIST_ENTRY FreeRcbList,
    _Inout_ PNDIS_SPIN_LOCK FreeRcbListLock,
    _Inout_ PNDIS_HANDLE RecvNblPoolHandle);

NDIS_STATUS
NICInitializeReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

VOID
NICFlushReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

NDIS_STATUS
NICReferenceReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId);

VOID
NICDereferenceReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId,
    _Out_opt_ ULONG *RefCount);


BOOLEAN
NICIsBusy(
    _In_  PMP_ADAPTER  Adapter);


#define RECEIVE_BLOCK_REFERENCE_COUNT(Adapter, BlockIndex) Adapter->ReceiveBlock[BlockIndex].PendingReceives
#define RECEIVE_BLOCK_IS_BUSY(Adapter, BlockIndex) Adapter->ReceiveBlock[BlockIndex].PendingReceives!=0

// Prototypes for standard NDIS miniport entry points
MINIPORT_INITIALIZE                 MPInitializeEx;
MINIPORT_HALT                       MPHaltEx;
MINIPORT_UNLOAD                     DriverUnload;
MINIPORT_PAUSE                      MPPause;
MINIPORT_RESTART                    MPRestart;
MINIPORT_SEND_NET_BUFFER_LISTS      MPSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS    MPReturnNetBufferLists;
MINIPORT_CANCEL_SEND                MPCancelSend;
MINIPORT_CHECK_FOR_HANG             MPCheckForHangEx;
MINIPORT_RESET                      MPResetEx;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY    MPDevicePnpEventNotify;
MINIPORT_SHUTDOWN                   MPShutdownEx;
MINIPORT_CANCEL_OID_REQUEST         MPCancelOidRequest;

