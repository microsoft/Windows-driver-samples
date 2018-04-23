/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    RSSv2.h

Abstract:

   This module declares the NDIS-RSSv2 related data types, flags, macros, and functions.

Revision History:

--*/

#pragma once

#if (NDIS_SUPPORT_NDIS680)

#define MAX_NIC_SWITCH_VPORTS   64

#define MAX_NUMBER_OF_INDIRECTION_TABLE_ENTRIES                     \
        (NDIS_RSS_INDIRECTION_TABLE_MAX_SIZE_REVISION_3 /           \
         sizeof(PROCESSOR_NUMBER))

//
// This define controls if QueueMap's functionality of the rssv2lib will
// be compiled-in.
//
#define RSSV2_MAX_NUMBER_OF_PROCESSORS_IN_RSS_TABLE 64

typedef struct _RSSV2_QUEUE_MAP* PRSSV2_QUEUE_MAP;
typedef struct _MP_ADAPTER_VPORT
{
    //
    // CREATE_VPORT OID was issues for this VPort
    //
    BOOLEAN Created;

    //
    // VPort is activated
    //
    BOOLEAN Active;

    //
    // If HwRSS (VMMQ) is enabled for this VPort
    //
    BOOLEAN RssEnabled;

    //
    // RSSv2 parameters
    //
    NDIS_RECEIVE_SCALE_PARAMETERS_V2 RssV2Params;

    //
    // Hashing key
    //
    UCHAR RssV2Key[NDIS_RSS_HASH_SECRET_KEY_MAX_SIZE_REVISION_2];

    //
    // Processor numbers for steering parameters
    //
    PROCESSOR_NUMBER    PrimaryProcessorNumber;
    PROCESSOR_NUMBER    DefaultProcessorNumber;
    PROCESSOR_NUMBER    RssV2Table[MAX_NUMBER_OF_INDIRECTION_TABLE_ENTRIES];

    //
    // Local indexes (relative to RSS table) for steering parameters
    //
    UINT8               PrimaryProcessorIndex;
    UINT8               DefaultProcessorIndex;
    UINT8               RssV2IndexTable[MAX_NUMBER_OF_INDIRECTION_TABLE_ENTRIES];

    //
    // NQ ("number of queues") enforcement helper object.
    // Tracks how many entries reference each RSS processor
    //
    PRSSV2_QUEUE_MAP    QueueMap;

} MP_ADAPTER_VPORT, *PMP_ADAPTER_VPORT;

//
// The MP_ADAPTER_RSS_DATA structure is used to track RSS state
//
typedef struct _MP_ADAPTER_RSS_DATA
{
    //
    // Flag determines if miniport is in Native RSS or NicSwitch mode
    //
    BOOLEAN                     IsNicSwitchCreated;

    //
    // RSS processor information from NDIS
    //
    PNDIS_RSS_PROCESSOR_INFO    RssProcessorInfo;
    PNDIS_RSS_PROCESSOR         RssProcessorArray;

    //
    // RSS state for a single "VPort" in NativeRSS mode
    //
    MP_ADAPTER_VPORT            NativeVPort;

    //
    // RSS state for VPorts in NicSwitch mode
    //
    MP_ADAPTER_VPORT            VPort[MAX_NIC_SWITCH_VPORTS];

} MP_ADAPTER_RSS_DATA, *PMP_ADAPTER_RSS_DATA;


_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
InitializeRSSConfig(
    _Inout_ struct _MP_ADAPTER* Adapter
    );

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
NICSetRSSv2Parameters(
    _In_ struct _MP_ADAPTER*    Adapter,
    _In_ PNDIS_OID_REQUEST  NdisRequest);

_IRQL_requires_(DISPATCH_LEVEL)
NDIS_STATUS
NICSetRSSv2IndirectionTableEntries(
    _In_ struct _MP_ADAPTER*    Adapter,
    _In_ PNDIS_OID_REQUEST  NdisRequest);


#endif  // NDIS_SUPPORT_NDIS680
