////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicPacketModification.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified 
//         BASIC_PACKET_MODIFICATION scenario.
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

#ifndef SCENARIOS_BASIC_PACKET_MODIFICATION_H
#define SCENARIOS_BASIC_PACKET_MODIFICATION_H

_Success_(return == NO_ERROR)
UINT32 BasicPacketModificationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                              _In_ const UINT32 stringCount);

VOID BasicPacketModificationScenarioLogHelp();

#endif /// SCENARIOS_BASIC_PACKET_MODIFICATION_H