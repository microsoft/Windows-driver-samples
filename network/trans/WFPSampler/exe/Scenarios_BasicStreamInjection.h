////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicStreamInjection.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified BASIC_STREAM_INJECTION
//         scenario.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      June      15,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCENARIOS_BASIC_STREAM_INJECTION_H
#define SCENARIOS_BASIC_STREAM_INJECTION_H

_Success_(return == NO_ERROR)
UINT32 BasicStreamInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                           _In_ const UINT32 stringCount);

VOID BasicStreamInjectionScenarioLogHelp();

#endif /// SCENARIOS_BASIC_STREAM_INJECTION_H
