////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmFilter.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_FILTER objects.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add HlprFwpmFilterConditionPrune
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_FWPM_FILTER_H
#define HELPERFUNCTIONS_FWPM_FILTER_H

VOID HlprFwpmFilterConditionPrune(_In_reads_(numFilterConditions) const FWPM_FILTER_CONDITION* pFilterConditions,
                                  _Inout_ UINT32 numFilterConditions,
                                  _Inout_updates_all_(*pNewNumFilterConditions) FWPM_FILTER_CONDITION** ppNewFilterConditions,
                                  _Inout_ UINT32* pNewNumFilterConditions);

_Success_(pFilterCondition->conditionValue.type == FWP_EMPTY &&
          pFilterCondition->conditionValue.uint64 == 0)
VOID HlprFwpmFilterConditionPurge(_Inout_ FWPM_FILTER_CONDITION* pFilterCondition);

_At_(*ppFilterConditions, _Post_ _Null_)
_Success_(*ppFilterConditions == 0)
VOID HlprFwpmFilterConditionDestroy(_Inout_updates_all_(numFilterConditions) FWPM_FILTER_CONDITION** ppFilterConditions,
                                    _In_ const UINT32 numFilterConditions = 1);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterConditionCreate(_Inout_updates_all_(numFilterConditions) FWPM_FILTER_CONDITION** ppFilterConditions,
                                     _In_ const UINT32 numFilterConditions = 1);

BOOLEAN HlprFwpmFilterConditionIsValidForLayer(_In_ const GUID* pLayerKey,
                                               _In_ const GUID* pConditionKey);

_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterEnum(_In_ const HANDLE engineHandle,
                          _In_ const HANDLE enumHandle,
                          _In_ const UINT32 numEntriesRequested,
                          _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_FILTER*** pppEntries,
                          _Out_ UINT32* pNumEntriesReturned);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                       _Inout_ HANDLE* pEnumHandle);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterCreateEnumHandle(_In_ const HANDLE engineHandle,
                                      _In_opt_ const FWPM_FILTER_ENUM_TEMPLATE* pEnumTemplate,
                                      _Out_ HANDLE* pEnumHandle);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterDeleteByKey(_In_ const HANDLE engineHandle,
                                 _In_ const GUID* pFilterKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterAdd(_In_ const HANDLE engineHandle,
                         _Inout_ FWPM_FILTER* pFilter);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                               _In_opt_ const GUID* pProviderKey);

VOID HlprFwpmFilterPurge(_Inout_ FWPM_FILTER* pFilter);

_At_(*ppFilters, _Post_ _Null_)
_Success_(*ppFilters == 0)
VOID HlprFwpmFilterDestroy(_Inout_updates_all_(numFilters) FWPM_FILTER** ppFilters,
                           _In_ const UINT32 numFilters = 1);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterCreate(_Inout_updates_all_(numFilters) FWPM_FILTER** ppFilters,
                            _In_ const UINT32 numFilters = 1);

#endif /// HELPERFUNCTIONS_FWPM_FILTER_H
