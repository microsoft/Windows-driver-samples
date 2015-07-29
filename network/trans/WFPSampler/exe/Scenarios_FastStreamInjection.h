////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_FastStreamInjection.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified FAST_STREAM_INJECTION
//         scenario.
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

#ifndef SCENARIOS_FAST_STREAM_INJECTION_H
#define SCENARIOS_FAST_STREAM_INJECTION_H

_Success_(return == NO_ERROR)
UINT32 FastStreamInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ const UINT32 stringCount);

VOID FastStreamInjectionScenarioLogHelp();

#endif /// SCENARIOS_FAST_STREAM_INJECTION_H