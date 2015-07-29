////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_ProxyCallouts.cpp
//
//   Abstract:
//      This module contains prototypes for WFP Classify functions for proxying connections and 
//         sockets.
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

#ifndef CLASSIFY_PROXY_H
#define CLASSIFY_PROXY_H

_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID ProxyUsingInjectionMethodDeferredProcedureCall(_In_ KDPC* pDPC,
                                                    _In_opt_ PVOID pContext,
                                                    _In_opt_ PVOID pArg1,
                                                    _In_opt_ PVOID pArg2);

#if(NTDDI_VERSION >= NTDDI_WIN7)


_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID ProxyByALERedirectDeferredProcedureCall(_In_ KDPC* pDPC,
                                             _In_opt_ PVOID pContext,
                                             _In_opt_ PVOID pArg1,
                                             _In_opt_ PVOID pArg2);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByALERedirect(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                      _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                      _Inout_opt_ VOID* pNetBufferList,
                                      _In_opt_ const VOID* pClassifyContext,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ UINT64 flowContext,
                                      _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pNetBufferList,
                                    _In_opt_ const VOID* pClassifyContext,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#else

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByALERedirect(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                      _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                      _Inout_opt_ VOID* pNetBufferList,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ UINT64 flowContext,
                                      _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pNetBufferList,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_PROXY_H
