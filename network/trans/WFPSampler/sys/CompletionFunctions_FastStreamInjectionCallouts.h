////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_FastStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains prototypes for WFP Completion functions for data injected back into
//         the stream path using the clone / block / inject method.
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

#ifndef COMPLETION_FAST_STREAM_INJECTION_H
#define COMPLETION_FAST_STREAM_INJECTION_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteFastStreamInjection(_In_ VOID* pContext,
                                       _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                       _In_ BOOLEAN dispatchLevel);

#endif /// COMPLETION_FAST_STREAM_INJECTION_H
