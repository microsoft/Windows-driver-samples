////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_RedirectData.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with REDIRECT_DATA.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance support for ALE redirection.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_REDIRECT_DATA_H
#define HELPERFUNCTIONS_REDIRECT_DATA_H

typedef struct REDIRECTION_HANDLE_DATA_
{
   HANDLE* pConnectRedirectionHandle;
   HANDLE* pBindRedirectionHandle;
}REDIRECTION_HANDLE_DATA,*PREDIRECTION_HANDLE_DATA;

typedef struct REDIRECT_DATA_
{
   UINT64             classifyHandle;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   HANDLE             redirectHandle;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

   VOID*              pWritableLayerData; /// FWPS_BIND_REQUEST or FWPS_CONNECT_REQUEST
   PC_PROXY_DATA*     pProxyData;
   FWPS_CLASSIFY_OUT* pClassifyOut;
   BOOLEAN            isPended;
   BYTE               pReserved[7];
}REDIRECT_DATA, *PREDIRECT_DATA;

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprRedirectDataPurge(_Inout_ REDIRECT_DATA* pRedirectData);

_At_(*ppRedirectData, _Pre_ _Notnull_)
_At_(*ppRedirectData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppRedirectData == 0)
VOID KrnlHlprRedirectDataDestroy(_Inout_ REDIRECT_DATA** ppRedirectData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprRedirectDataPopulate(_Inout_ REDIRECT_DATA* pRedirectData,
                                      _In_ const VOID* pClassifyContext,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                      _In_opt_ HANDLE redirectHandle);

_At_(*ppRedirectData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppRedirectData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppRedirectData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprRedirectDataCreate(_Outptr_ REDIRECT_DATA** ppRedirectData,
                                    _In_ const VOID* pClassifyContext,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                    _In_opt_ HANDLE redirectHandle);

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// HELPERFUNCTIONS_REDIRECT_DATA_H