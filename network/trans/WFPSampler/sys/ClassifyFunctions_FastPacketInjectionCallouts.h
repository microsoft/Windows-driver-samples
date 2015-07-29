////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_FastPacketInjectionCallouts.h
//
//   Abstract:
//      This module contains prototypes for WFP Classify functions that inject packets back into the
//         data path using the clone / block / inject method.  This method is inline only, no 
//         modification, and uses as little validation and error checking as possible.  Certain 
//         scenarios will definitely fail injection, such as injection to loopback, or IPsec 
//         encrypted packets.  These functions are meant for test performance purposes only.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLASSIFY_FAST_PACKET_INJECTION_H
#define CLASSIFY_FAST_PACKET_INJECTION_H

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFastPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pLayerData,
                                       _In_opt_ const VOID* pClassifyContext,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#else

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFastPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pLayerData,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_FAST_PACKET_INJECTION_H
