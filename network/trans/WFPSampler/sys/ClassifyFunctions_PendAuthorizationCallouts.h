////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_PendAuthorizationCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Classify functions for pending and completing 
//         authorizations
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

#ifndef CLASSIFY_PEND_AUTHORIZATION_H
#define CLASSIFY_PEND_AUTHORIZATION_H

_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID PendAuthorizationDeferredProcedureCall(_In_ KDPC* pDPC,
                                            _In_opt_ PVOID pContext,
                                            _In_opt_ PVOID pArg1,
                                            _In_opt_ PVOID pArg2);

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyPendAuthorization(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
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
VOID NTAPI ClassifyPendAuthorization(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pLayerData,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_PEND_AUTHORIZATION_H
