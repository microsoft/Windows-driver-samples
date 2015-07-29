////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_AdvancedPacketInjectionCallouts.cpp
//
//   Abstract:
//      This module contains prototypes for WFP Completion functions for injecting packets back into
//         the data path using the allocate / block / inject method.
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

#ifndef COMPLETION_ADVANCED_PACKET_INJECTION_H
#define COMPLETION_ADVANCED_PACKET_INJECTION_H

typedef struct ADVANCED_PACKET_INJECTION_COMPLETION_DATA_
{
   KSPIN_LOCK                  spinLock;
   INT32                       refCount;
   BOOLEAN                     performedInline;
   CLASSIFY_DATA*              pClassifyData;
   INJECTION_DATA*             pInjectionData;
   FWPS_TRANSPORT_SEND_PARAMS* pSendParams;
   BYTE*                       pAllocatedBuffer;
   PMDL                        pAllocatedMDL;
}ADVANCED_PACKET_INJECTION_COMPLETION_DATA, *PADVANCED_PACKET_INJECTION_COMPLETION_DATA;

#if DBG

extern INJECTION_COUNTERS g_apiTotalCompletions;

VOID AdvancedPacketInjectionCountersDecrement(_In_ HANDLE injectionHandle,
                                              _Inout_ INJECTION_COUNTERS* pCounters);

#endif /// DBG

_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID AdvancedPacketInjectionCompletionDataDestroy(_Inout_ ADVANCED_PACKET_INJECTION_COMPLETION_DATA** ppCompletionData,
                                                  _In_ BOOLEAN override = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteAdvancedPacketInjection(_In_ VOID* pContext,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ BOOLEAN dispatchLevel);

#endif /// COMPLETION_ADVANCED_PACKET_INJECTION_H
