////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_Include.h
//
//   Abstract:
//      This module contains a central repository of headers which contain prototypes for all of the
//         classify functions.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add ClassifyFunctions_AdvancedPacketInjection.h,
//                                              ClassifyFunctions_FlowAssociation.h, and
//                                              ClassifyFunctions_EndpointClosure.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLASSIFY_INCLUDE_H
#define CLASSIFY_INCLUDE_H

#include "ClassifyFunctions_AdvancedPacketInjectionCallouts.h" /// .
#include "ClassifyFunctions_BasicActionCallouts.h"             /// .
#include "ClassifyFunctions_BasicPacketExaminationCallouts.h"  /// .
#include "ClassifyFunctions_BasicPacketInjectionCallouts.h"    /// .
#include "ClassifyFunctions_BasicPacketModificationCallouts.h" /// .
#include "ClassifyFunctions_BasicStreamInjectionCallouts.h"    /// .
#include "ClassifyFunctions_FastPacketInjectionCallouts.h"     /// .
#include "ClassifyFunctions_FastStreamInjectionCallouts.h"     /// .
#include "ClassifyFunctions_FlowAssociationCallouts.h"         /// .
#include "ClassifyFunctions_PendAuthorizationCallouts.h"       /// .
#include "ClassifyFunctions_PendEndpointClosureCallouts.h"     /// .
#include "ClassifyFunctions_ProxyCallouts.h"                   /// .

#endif /// CLASSIFY_INCLUDE_H