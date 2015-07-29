////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_Proxy.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified PROXY scenario.
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

#ifndef SCENARIOS_PROXY_H
#define SCENARIOS_PROXY_H

_Success_(return == NO_ERROR)
UINT32 ProxyScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                            _In_ UINT32 stringCount);

VOID ProxyScenarioLogHelp();

#endif /// SCENARIOS_PROXY_H
