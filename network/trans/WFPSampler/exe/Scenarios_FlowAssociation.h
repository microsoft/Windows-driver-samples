////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_FastStreamInjection.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified FLOW_ASSOCIATION
//         scenario.
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

#ifndef SCENARIOS_FLOW_ASSOCIATION_H
#define SCENARIOS_FLOW_ASSOCIATION_H

_Success_(return == NO_ERROR)
UINT32 FlowAssociationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                      _In_ const UINT32 stringCount);

VOID FlowAssociationScenarioLogHelp();

#endif /// SCENARIOS_FLOW_ASSOCIATION_H
