////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmLayer.cpp
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_LAYER objects.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add basic support for WinBlue Fast layers
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_FWPM_LAYER_H
#define HELPERFUNCTIONS_FWPM_LAYER_H

_At_(*pppFilterConditionArray, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppFilterConditionArray, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pppFilterConditionArray, _Post_ _Maybenull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmLayerGetFilterConditionArrayByKey(_In_ const GUID* pLayerKey,
                                                 _Outptr_result_buffer_maybenull_(*pConditionArrayCount) GUID*** pppFilterConditionArray,
                                                 _Out_ UINT16* pConditionArrayCount);

_Success_(return < FWPS_BUILTIN_LAYER_MAX)
UINT8 HlprFwpmLayerGetIDByKey(_In_ const GUID* pLayerKey);

_Success_(return < FWPS_BUILTIN_LAYER_MAX)
UINT8 HlprFwpmLayerGetIDByString(_In_ PCWSTR pLayerString);

_Success_(return != 0)
const GUID* HlprFwpmLayerGetByString(_In_ PCWSTR pLayerString);

_Success_(return != 0)
const GUID* HlprFwpmLayerGetByID(_In_ const UINT32 pLayerID);

BOOLEAN HlprFwpmLayerIsUserMode(_In_ const GUID* pLayerKey);

BOOLEAN HlprFwpmLayerIsKernelMode(_In_ const GUID* pLayerKey);

BOOLEAN HlprFwpmLayerIsIPv4(_In_ const GUID* pLayerKey);

BOOLEAN HlprFwpmLayerIsIPv6(_In_ const GUID* pLayerKey);

#endif /// HELPERFUNCTIONS_FWPM_LAYER_H