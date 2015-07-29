////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_Include.h
//
//   Abstract:
//      This module contains a central repository of headers which contain prototypes for all of the
//         notification functions.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add NotifyFunctions_AdvancedCallouts.h,
//                                              NotifyFunctions_FlowDelete.h, and replace
//                                              NotifyFunctions_PendAuthorizationCallouts.h, with
//                                              NotifyFunctions_PendCallouts.h
//                             
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NOTIFY_INCLUDE_H
#define NOTIFY_INCLUDE_H

#include "NotifyFunctions_AdvancedCallouts.h" /// .
#include "NotifyFunctions_BasicCallouts.h"    /// .
#include "NotifyFunctions_FastCallouts.h"     /// .
#include "NotifyFunctions_FlowDelete.h"       /// .
#include "NotifyFunctions_PendCallouts.h"     /// .
#include "NotifyFunctions_ProxyCallouts.h"    /// .

#endif /// NOTIFY_INCLUDE_H