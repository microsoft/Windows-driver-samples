////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_CommandLine.h
//
//   Abstract:
//      This module contains functions which assist in parsing information from the command prompt.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_COMMAND_LINE_H
#define HELPERFUNCTIONS_COMMAND_LINE_H

VOID HlprCommandLineParseForScenarioRemoval(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                            _In_ UINT32 stringCount,
                                            _Inout_ BOOLEAN* pRemoveScenario);

VOID HlprCommandLineParseForBootTime(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                     _In_ UINT32 stringCount,
                                     _Inout_ FWPM_FILTER* pFilter);

VOID HlprCommandLineParseForCalloutUse(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter);

VOID HlprCommandLineParseForVolatility(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter);

_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForLayerKey(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter);

_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForSubLayerKey(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount,
                                          _Inout_ FWPM_FILTER* pFilter);

_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForFilterConditions(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                               _In_ UINT32 stringCount,
                                               _Inout_ FWPM_FILTER* pFilter,
                                               _In_ BOOLEAN forEnum = FALSE);

_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForFilterInfo(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                         _In_ UINT32 stringCount,
                                         _Inout_ FWPM_FILTER* pFilter,
                                         _In_ BOOLEAN forEnum = FALSE);

#endif /// HELPERFUNCTIONS_COMMAND_LINE_H