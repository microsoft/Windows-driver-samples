//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

#define MBB_DEFAULT_SESSION_ID 0

typedef struct _DSS_PACKET
{
    WDFMEMORY Data;
    PMDL Mdl;
} DSS_PACKET, *PDSS_PACKET;

typedef struct _MBB_PORT MBB_PORT, *PMBB_PORT;

typedef struct _MINIPORT_DRIVER_CONTEXT
{
    WDFDRIVER hDriver;
} MINIPORT_DRIVER_CONTEXT, *PMINIPORT_DRIVER_CONTEXT;

typedef struct _MBB_RECEIVE_QUEUE
{
    //
    // Queue State
    //
    BOOLEAN LookasideList;
    NDIS_SPIN_LOCK Lock;
    KEVENT QueueEmptyEvent;
    //
    // Track Receives
    //
    LIST_ENTRY ReceivedQueue;
    //
    // Resources
    //
    NDIS_HANDLE NblPool;
    NPAGED_LOOKASIDE_LIST ReceiveLookasideList;

} MBB_RECEIVE_QUEUE, *PMBB_RECEIVE_QUEUE;

typedef struct _MBB_NDIS_RECEIVE_CONTEXT
{
    //
    // Resources
    //

    PMDL Mdl;
    PUCHAR ReceiveNtbBuffer;
    WDFLOOKASIDE ReceiveLookasideList;
    PMBB_RECEIVE_QUEUE RecvQueue;
    MBB_BUS_HANDLE BusHandle;
    MBB_RECEIVE_CONTEXT BusContext;
    PWMBCLASS_DEVICE_CONTEXT WmbDeviceContext;
    //
    // Parse state
    //
    ULONG NtbSequence;
    WDFMEMORY ReceiveLookasideBufferMemory;

    LONG TotalNdpCount;
    LONG CompletedNdpCount;
} MBB_NDIS_RECEIVE_CONTEXT, *PMBB_NDIS_RECEIVE_CONTEXT;

typedef struct _MBB_RECEIVE_NDP_CONTEXT
{
    LIST_ENTRY ReceiveNdpNode;
    PMBB_NDIS_RECEIVE_CONTEXT ReceiveContext;
    WDFMEMORY ReceiveNdpLookasideBufferMemory;
    PVOID Nth;
    PVOID Ndp;
    ULONG CurrentDatagramIndex;
    ULONG IndicatedPackets;
    PWMBCLASS_NETADAPTER_CONTEXT NetAdapterContext;
} MBB_RECEIVE_NDP_CONTEXT, *PMBB_RECEIVE_NDP_CONTEXT;

extern MINIPORT_DRIVER_CONTEXT GlobalControl;

typedef enum
{
    MbbNdpTypeNoCrc = 0,
    MbbNdpTypeCrc,
    MbbNdpTypeIps,
    MbbNdpTypeVendor_1, // Vendor with session id X (need not be 1)
    MbbNdpTypeVendor_2, // Vendor with session id Y (need not be X + 1)
    MbbNdpTypeVendor_3,
    MbbNdpTypeVendor_Max, // Max 4 vendor sessions in one NTB
    MbbNdpTypeMax

} MBB_NDP_TYPE;

typedef struct _MBB_PACKET_CONTEXT
{
    PMDL DataStartMdl;
    PMDL DataEndMdl;
    PMDL PaddingMdl;
    PMDL ModifiedMdl;
    MDL OriginalMdl;

} MBB_PACKET_CONTEXT, *PMBB_PACKET_CONTEXT;

typedef struct _MBB_NDP_HEADER_ENTRY
{
    MBB_NDP_TYPE NdpType;
    ULONG SessionId;
    ULONG DatagramOffset;
    ULONG DatagramLength;
    ULONG NextEntryIndex;
    PNET_BUFFER NetBuffer;
    PNET_BUFFER_LIST NetBufferList;

    union
    {
        NET_PACKET* NetPacket;
        PDSS_PACKET DssPacket;
    };
    MBB_PACKET_CONTEXT NetPacketContext;
} MBB_NDP_HEADER_ENTRY, *PMBB_NDP_HEADER_ENTRY;

typedef struct _MBB_NTB_BUILD_CONTEXT
{
    LIST_ENTRY NtbQLink;
    //
    // Read-only values
    //
    BOOLEAN IsNtb32Bit;
    ULONG NtbHeaderSize;
    ULONG NdpHeaderFixedSize;
    ULONG NdpDatagramEntrySize;
    ULONG NtbOutMaxSize;
    USHORT NtbOutMaxDatagrams;
    USHORT NdpOutDivisor;
    USHORT NdpOutPayloadRemainder;
    USHORT NdpOutAlignment;
    PVOID PaddingBuffer;
    WDFLOOKASIDE NtbLookasideList;
    //
    // Network Transfer Header(NTH)
    //
    union
    {
        NCM_NTH16 Nth16;
        NCM_NTH32 Nth32;
    };
    PMDL NthMdl;
    //
    // NDP Header
    //
    PMDL NdpMdl;
    ULONG NdpSize;
    PVOID NdpBuffer;
    //
    // NDP Datagrams
    //
    ULONG DatagramCount;
    ULONG DatagramLength;
    PMDL DatagramLastMdl;

    WDFMEMORY NtbLookasideBufferMemory;
    WDFMEMORY NdpBufferMemory;
    NETPACKETQUEUE NetTxQueue;
    NETADAPTER NetAdapter;
    NET_RING_COLLECTION const* NetDatapathDescriptor;

#if DBG
    //
    // Testing
    //
    PCHAR ScratchBuffer;
    ULONG ScratchLength;
#endif

    //
    // NDP Headers. Varialble length array is limited
    // to NdpMaxDatagrams. NdpFirstDatagramEntry of -1
    // is invalid.
    //
    ULONG NdpFirstDatagramEntry[MbbNdpTypeMax];
    MBB_NDP_HEADER_ENTRY NdpDatagramEntries[ANYSIZE_ARRAY];

} MBB_NTB_BUILD_CONTEXT, *PMBB_NTB_BUILD_CONTEXT;
