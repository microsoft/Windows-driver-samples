/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   MpHAL.H

Abstract:

    This module declares the structures and functions that abstract the
    adapter and medium's (emulated) hardware capabilities.

--*/


#ifndef _MPHAL_H
#define _MPHAL_H


//
// A FRAME represents the physical bits as they are being transmitted on the
// wire.  Normally, a miniport wouldn't need to implement any such tracking,
// but we have to simulate our own Ethernet hub.
//
typedef struct _FRAME
{
    volatile LONG           Ref;
    PMDL                    Mdl;
    ULONG                   ulSize;
    UCHAR                   Data[NIC_BUFFER_SIZE];
} FRAME, *PFRAME;

ALLOCATE_FUNCTION HWFrameAllocate;
FREE_FUNCTION HWFrameFree;

struct _TCB;
struct _RCB;

//
// Structures and utility macros to retrieve and set VLAN tag related data
//
#define FRAME_8021Q_ETHER_TYPE      0x81
#define IS_FRAME_8021Q(_Frame)\
    (((PNIC_FRAME_HEADER)(_Frame)->Data)->EtherType[0] == FRAME_8021Q_ETHER_TYPE)

typedef struct _VLAN_TAG_HEADER
{
    UCHAR       TagInfo[2];    
} VLAN_TAG_HEADER, *PVLAN_TAG_HEADER;

#define GET_FRAME_VLAN_TAG_HEADER(_Frame)\
    ((VLAN_TAG_HEADER UNALIGNED *)((PNIC_FRAME_HEADER)(_Frame)->Data)->EtherType+1)

#define USER_PRIORITY_MASK          0xe0
#define CANONICAL_FORMAT_ID_MASK    0x10
#define HIGH_VLAN_ID_MASK           0x0F

#define COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(_Ieee8021qInfo, _pTagHeader)                                   \
{                                                                                                               \
    (_Ieee8021qInfo).TagHeader.UserPriority = ((_pTagHeader->TagInfo[0] & USER_PRIORITY_MASK) >> 5);              \
    (_Ieee8021qInfo).TagHeader.CanonicalFormatId = ((_pTagHeader->TagInfo[0] & CANONICAL_FORMAT_ID_MASK) >> 4);   \
    (_Ieee8021qInfo).TagHeader.VlanId = (((USHORT)(_pTagHeader->TagInfo[0] & HIGH_VLAN_ID_MASK) << 8)| (USHORT)(_pTagHeader->TagInfo[1]));                                                                \
}

VOID
HWFrameReference(
    _In_  PFRAME  Frame);

VOID
HWFrameRelease(
    _In_  PFRAME  Frame);

NDIS_STATUS
HWInitialize(
    _In_  PMP_ADAPTER Adapter,
    _In_  NDIS_HANDLE  WrapperConfigurationContext);

VOID
HWReadPermanentMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle,
    _Out_writes_bytes_(NIC_MACADDR_SIZE)  PUCHAR  PermanentMacAddress);

NDIS_STATUS
HWGetDestinationAddress(
    _In_  PNET_BUFFER  NetBuffer,
    _Out_writes_bytes_(NIC_MACADDR_SIZE) PUCHAR DestAddress);

BOOLEAN
HWIsFrameAcceptedByPacketFilter(
    _In_  PMP_ADAPTER  Adapter,
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    _In_  ULONG        FrameType);

NDIS_MEDIA_CONNECT_STATE
HWGetMediaConnectStatus(
    _In_  PMP_ADAPTER Adapter);

VOID
HWProgramDmaForSend(
    _In_  PMP_ADAPTER   Adapter,
    _In_  struct _TCB  *Tcb,
    _In_  PNET_BUFFER   NetBuffer,
    _In_  BOOLEAN       fAtDispatch);

_IRQL_requires_(DISPATCH_LEVEL)
ULONG
HWGetBytesSent(
    _In_  PMP_ADAPTER  Adapter,
    _In_  struct _TCB *Tcb);

NDIS_STATUS
HWBeginReceiveDma(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  struct _RCB *Rcb,
    _In_  PFRAME       Frame);

#endif // _MPHAL_H

