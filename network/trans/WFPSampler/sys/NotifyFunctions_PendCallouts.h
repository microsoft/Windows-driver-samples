////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_PendCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Notify functions for the pend callouts.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       NotifyPendNotification
//
//       <Module>
//          Notify               -     Function is located in sys\NotifyFunctions\
//       <Scenario>
//          PendNotification     -     Function demonstates use of notifications for callouts 
//                                        pending authorization or endpoint closure  requests.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation 
//                                          (replaces NotifyFuncitons_PendAuthorizationCallouts.h)
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NOTIFY_PEND_NOTIFICATION_H
#define NOTIFY_PEND_NOTIFICATION_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS NTAPI NotifyPendNotification(_In_ FWPS_CALLOUT_NOTIFY_TYPE notificationType,
                                      _In_ const GUID* pFilterKey,
                                      _Inout_ FWPS_FILTER* pFilter);

#endif /// NOTIFY_PEND_NOTIFICATION_H