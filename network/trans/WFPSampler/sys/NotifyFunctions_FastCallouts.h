////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_FastCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Notify functions for the performance callouts.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       NotifyFastNotification
//
//       <Module>
//          Notify            -       Function is an FWPS_CALLOUT_NOTIFY_FN
//       <Scenario>
//          FastNotification  -       Function demonstates use of the performance notifications
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

#ifndef NOTIFY_FAST_NOTIFICATION_H
#define NOTIFY_FAST_NOTIFICATION_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS NTAPI NotifyFastNotification(_In_ FWPS_CALLOUT_NOTIFY_TYPE notificationType,
                                      _In_ const GUID* pFilterKey,
                                      _Inout_ FWPS_FILTER* pFilter);

#endif /// NOTIFY_FAST_NOTIFICATION_H