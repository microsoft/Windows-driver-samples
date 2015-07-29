////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_ProxyCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Notify functions for the proxy callouts using WFP's 
//         REDIRECT layers.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       NotifyProxyByALERedirectNotification
//
//       <Module>
//          Notify                            -       Function is an FWPS_CALLOUT_NOTIFY_FN
//       <Scenario>
//          ProxyByALERedirectNotification    -       Function demonstates use of notifications for 
//                                                       callouts using WFP's REDIRECT layers
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

#ifndef NOTIFY_PROXY_H
#define NOTIFY_PROXY_H

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS NTAPI NotifyProxyByALERedirectNotification(_In_ FWPS_CALLOUT_NOTIFY_TYPE notificationType,
                                                    _In_ const GUID* pFilterKey,
                                                    _Inout_ FWPS_FILTER* pFilter);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// NOTIFY_PROXY_H