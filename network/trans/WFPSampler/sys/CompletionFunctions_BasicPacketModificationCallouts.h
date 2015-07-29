////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_BasicPacketModificationCallouts.cpp
//
//   Abstract:
//      This module contains prototypes for WFP Completion functions for injecting modified packets 
//         back into the data path using the clone / block / inject method.
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

#ifndef COMPLETION_BASIC_PACKET_MODIFICATION_H
#define COMPLETION_BASIC_PACKET_MODIFICATION_H

typedef struct BASIC_PACKET_MODIFICATION_COMPLETION_DATA_
{
   KSPIN_LOCK                  spinLock;
   INT32                       refCount;
   BOOLEAN                     performedInline;
   CLASSIFY_DATA*              pClassifyData;
   INJECTION_DATA*             pInjectionData;
   FWPS_TRANSPORT_SEND_PARAMS* pSendParams;
}BASIC_PACKET_MODIFICATION_COMPLETION_DATA, *PBASIC_PACKET_MODIFICATION_COMPLETION_DATA;

_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID BasicPacketModificationCompletionDataDestroy(_Inout_ BASIC_PACKET_MODIFICATION_COMPLETION_DATA** ppCompletionData,
                                                  _In_ BOOLEAN override = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteBasicPacketModification(_In_ VOID* pContext,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ BOOLEAN dispatchLevel);

#endif /// COMPLETION_BASIC_PACKET_MODIFICATION_H