////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_AdvancedPacketInjection.h
//
//   Abstract:
//      This module contains prototypes for functions which run the specified 
//         ADVANCED_PACKET_INJECTION scenario.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December   13,   2013  -     1.1  -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCENARIOS_ADVANCED_PACKET_INJECTION_H
#define SCENARIOS_ADVANCED_PACKET_INJECTION_H

_Success_(return == NO_ERROR)
UINT32 AdvancedPacketInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                              _In_ const UINT32 stringCount);

VOID AdvancedPacketInjectionScenarioLogHelp();

#endif /// SCENARIOS_ADVANCED_PACKET_INJECTION_H