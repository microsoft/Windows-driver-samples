///////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_ProxyCallouts.cpp
//
//   Abstract:
//      This module contains prototypes of WFP Completion functions for proxied connections using 
//         the injection clone / drop / modify /inject model.
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

#ifndef COMPLETION_PROXY_CONNECTION_H
#define COMPLETION_PROXY_CONNECTION_H

typedef struct PROXY_COMPLETION_DATA_
{
   BOOLEAN                     performedInline;
   CLASSIFY_DATA*              pClassifyData;
   union
   {
      INJECTION_DATA*          pInjectionData;
      REDIRECT_DATA*           pRedirectData;
   };
   PC_PROXY_DATA*              pProxyData;
   FWPS_TRANSPORT_SEND_PARAMS* pSendParams;
}PROXY_COMPLETION_DATA, *PPROXY_COMPLETION_DATA;

_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID ProxyCompletionDataDestroy(_Inout_ PROXY_COMPLETION_DATA** ppCompletionData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteProxyInjection(_In_ VOID* pContext,
                                  _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                  _In_ BOOLEAN dispatchLevel);

#endif /// COMPLETION_PROXY_CONNECTION_H