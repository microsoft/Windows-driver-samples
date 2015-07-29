////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_Include.h
//
//   Abstract:
//      This module contains include headers for central exportation of the Scenarios_* modules.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add ADVANCED_PACKET_INJECTION, FLOW_ASSOCIATION, and
//                                              PEND_ENDPOINT_CLOSURE scenarios
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCENARIOS_INCLUDE_H
#define SCENARIOS_INCLUDE_H

#include "Scenarios_AdvancedPacketInjection.h" /// .

#if(NTDDI_VERSION >= NTDDI_WIN8)

#include "Scenarios_AppContainers.h"           /// .

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

#include "Scenarios_BasicAction.h"             /// .
#include "Scenarios_BasicPacketExamination.h"  /// .
#include "Scenarios_BasicPacketInjection.h"    /// .
#include "Scenarios_BasicPacketModification.h" /// .
#include "Scenarios_BasicStreamInjection.h"    /// .
#include "Scenarios_FastPacketInjection.h"     /// .
#include "Scenarios_FastStreamInjection.h"     /// .
#include "Scenarios_FlowAssociation.h"         /// .
#include "Scenarios_PendAuthorization.h"       /// .
#include "Scenarios_PendEndpointClosure.h"     /// .
#include "Scenarios_Proxy.h"                   /// .

#endif /// SCENARIOS_INCLUDE_H