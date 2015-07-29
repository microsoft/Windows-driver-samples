////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicAction.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified BASIC_ACTION scenario.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCENARIOS_BASIC_ACTION_H
#define SCENARIOS_BASIC_ACTION_H

_Success_(return == NO_ERROR)
UINT32 BasicActionBlockScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount);

_Success_(return == NO_ERROR)
UINT32 BasicActionContinueScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount);

_Success_(return == NO_ERROR)
UINT32 BasicActionPermitScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount);

_Success_(return == NO_ERROR)
UINT32 BasicActionRandomScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount);

VOID BasicActionScenarioLogHelp(_In_ const UINT32 scenario);

#endif /// SCENARIOS_BASIC_ACTION_H