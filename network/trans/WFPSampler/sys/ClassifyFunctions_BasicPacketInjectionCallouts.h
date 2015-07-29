////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicPacketInjectionCallouts.h
//
//   Abstract:
//      This module contains prototypes for WFP Classify functions that inject packets back into the
//         data path using the clone / block / inject method.
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

#ifndef CLASSIFY_BASIC_PACKET_INJECTION_H
#define CLASSIFY_BASIC_PACKET_INJECTION_H

#if DBG

extern INJECTION_COUNTERS g_bpiTotalClassifies;
extern INJECTION_COUNTERS g_bpiTotalBlockedAndAbsorbed;
extern INJECTION_COUNTERS g_bpiTotalPermitted;
extern INJECTION_COUNTERS g_bpiTotalSuccessfulInjectionCalls;
extern INJECTION_COUNTERS g_bpiTotalFailedInjectionCalls;
extern INJECTION_COUNTERS g_bpiOutstandingNBLClones;

VOID BasicPacketInjectionCountersIncrement(_In_ HANDLE injectionHandle,
                                           _Inout_ INJECTION_COUNTERS* pCounters);

#endif /// DBG

_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID BasicPacketInjectionDeferredProcedureCall(_In_ KDPC* pDPC,
                                               _In_opt_ PVOID pContext,
                                               _In_opt_ PVOID pDPCData,
                                               _In_opt_ PVOID pArg2);

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
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
VOID NTAPI ClassifyBasicPacketInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_opt_ VOID* pLayerData,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_BASIC_PACKET_INJECTION_H
