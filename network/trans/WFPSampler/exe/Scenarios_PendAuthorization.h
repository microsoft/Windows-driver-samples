////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_PendAuthorization.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified PEND_AUTHORIZATION 
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

#ifndef SCENARIOS_PEND_AUTHORIZATION_H
#define SCENARIOS_PEND_AUTHORIZATION_H

_Success_(return == NO_ERROR)
UINT32 PendAuthorizationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount);

VOID PendAuthorizationScenarioLogHelp();

#endif /// SCENARIOS_PEND_AUTHORIZATION_H