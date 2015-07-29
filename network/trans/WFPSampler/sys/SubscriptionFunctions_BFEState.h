////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      SubscriptionFunctions_BFEState.h
//
//   Abstract:
//      This module contains protptyes for WFP callback functions for changes in the BFE state.
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

#ifndef SUBSCRIBE_BFE_STATE_H
#define SUBSCRIBE_BFE_STATE_H

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID CALLBACK SubscriptionBFEStateChangeCallback(_Inout_ VOID* pContext,
                                                 _In_ FWPM_SERVICE_STATE bfeState);

#endif /// SUBSCRIBE_BFE_STATE_H