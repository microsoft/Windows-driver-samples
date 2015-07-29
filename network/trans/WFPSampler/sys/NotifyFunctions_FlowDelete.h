////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_FlowDelete.h
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
//       NotifyFlowDeleteNotification
//
//       <Module>
//          NotifyFlowDelete -       Function is an FWPS_CALLOUT_NOTIFY_FN
//       <Scenario>
//          Notification     -       Function demonstates use of the simple notifications
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

#ifndef NOTIFY_FLOW_DELETE_NOTIFICATION_H
#define NOTIFY_FLOW_DELETE_NOTIFICATION_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI NotifyFlowDeleteNotification(_In_ UINT16 layerID,
                                        _In_ UINT32 calloutID,
                                        _In_ UINT64 flowContext);

#endif /// NOTIFY_FLOW_DELETE_NOTIFICATION_H
