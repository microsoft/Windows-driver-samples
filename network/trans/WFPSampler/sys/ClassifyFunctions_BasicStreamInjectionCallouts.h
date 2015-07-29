////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains prototypes for WFP Classify functions that inject data back into TCP's 
//         stream using the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyBasicStreamInjection
//
//       <Module>
//          Classify             -       Function is an FWPS_CALLOUT_CLASSIFY_FN
//       <Scenario>
//          BasicStreamInjection -       Function demonstates use clone / block / inject model
//
//      <Action><Scenario><Modifier>
//
//      i.e.
//
//       TriggerBasicStreamInjectionOutOfBand
//
//       <Action>
//        {
//                               -
//          Trigger              -       Initiates the desired scenario
//          Perform              -       Executes the desired scenario
//        }
//       <Scenario>
//          BasicStreamInjection -       Function demonstates use clone / block / inject model
//       <Modifier>
//          WorkItemRoutine      -       WorkItem Routine for Out of Band Injection.
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

#ifndef CLASSIFY_BASIC_STREAM_INJECTION_H
#define CLASSIFY_BASIC_STREAM_INJECTION_H

typedef struct BASIC_STREAM_INJECTION_LIST_ITEM_
{
   LIST_ENTRY      entry;
   CLASSIFY_DATA*  pClassifyData;
   INJECTION_DATA* pInjectionData;
}BASIC_STREAM_INJECTION_LIST_ITEM, *PBASIC_STREAM_INJECTION_LIST_ITEM;


_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID BasicStreamInjectionDeferredProcedureCall(_In_ KDPC* pDPC,
                                               _In_opt_ PVOID pContext,
                                               _In_opt_ PVOID pArg1,
                                               _In_opt_ PVOID pArg2);

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                        _In_opt_ const VOID* pClassifyContext,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#else

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_BASIC_STREAM_INJECTION_H
