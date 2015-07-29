////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_AdvancedPacketInjectionCallouts.h
//
//   Abstract:
//      This module contains prototypes for WFP Classify functions that inject packets back into the
//         data path using the allocate / block / inject method.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLASSIFY_ADVANCED_PACKET_INJECTION_H
#define CLASSIFY_ADVANCED_PACKET_INJECTION_H

#if DBG

extern INJECTION_COUNTERS g_apiTotalClassifies;
extern INJECTION_COUNTERS g_apiTotalBlockedAndAbsorbed;
extern INJECTION_COUNTERS g_apiTotalPermitted;
extern INJECTION_COUNTERS g_apiTotalSuccessfulInjectionCalls;
extern INJECTION_COUNTERS g_apiTotalFailedInjectionCalls;
extern INJECTION_COUNTERS g_apiOutstandingNewNBLs;

VOID AdvancedPacketInjectionCountersIncrement(_In_ HANDLE injectionHandle,
                                              _Inout_ INJECTION_COUNTERS* pCounters);

#endif /// DBG

_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID AdvancedPacketInjectionDeferredProcedureCall(_In_ KDPC* pDPC,
                                                  _In_opt_ PVOID pContext,
                                                  _In_opt_ PVOID pDPCData,
                                                  _In_opt_ PVOID pArg2);

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyAdvancedPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
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
VOID NTAPI ClassifyAdvancedPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                           _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                           _Inout_opt_ VOID* pLayerData,
                                           _In_ const FWPS_FILTER* pFilter,
                                           _In_ UINT64 flowContext,
                                           _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_ADVANCED_PACKET_INJECTION_H
