////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_AppContainers.h
//
//   Abstract:
//      This module contains prototypes for functions which prepares and sends data for the 
//      APPLICATION_CONTAINER scenario implementation.
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

#ifndef SCENARIOS_APP_CONTAINER_H
#define SCENARIOS_APP_CONTAINER_H

#if(NTDDI_VERSION >= NTDDI_WIN8)

_Success_(return == NO_ERROR)
UINT32 AppContainerScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                   _In_ UINT32 stringCount);

VOID AppContainerScenarioLogHelp();

#endif // (NTDDI_VERSION >= NTDDI_WIN8)

#endif /// SCENARIOS_APP_CONTAINER_H