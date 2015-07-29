////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_PendEndpointClosure.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified PEND_ENDPOINT_CLOSURE 
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

#ifndef SCENARIOS_PEND_ENDPOINT_CLOSURE_H
#define SCENARIOS_PEND_ENDPOINT_CLOSURE_H

#if(NTDDI_VERSION >= NTDDI_WIN7)

_Success_(return == NO_ERROR)
UINT32 PendEndpointClosureScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount);

VOID PendEndpointClosureScenarioLogHelp();

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// SCENARIOS_PEND_ENDPOINT_CLOSURE_H
