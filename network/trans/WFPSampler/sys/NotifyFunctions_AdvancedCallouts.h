////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_AdvancedCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Notify functions for the simpler callouts.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       NotifyAdvancedNotification
//
//       <Module>
//          Notify               -       Function is an FWPS_CALLOUT_NOTIFY_FN
//       <Scenario>
//          AdvancedNotification -       Function demonstates use of the simple notifications
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

#ifndef NOTIFY_ADVANCED_NOTIFICATION_H
#define NOTIFY_ADVANCED_NOTIFICATION_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS NTAPI NotifyAdvancedNotification(_In_ FWPS_CALLOUT_NOTIFY_TYPE notificationType,
                                          _In_ const GUID* pFilterKey,
                                          _Inout_ FWPS_FILTER* pFilter);

#endif /// NOTIFY_ADVANCED_NOTIFICATION_H
