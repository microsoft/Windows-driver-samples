////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmCallout.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_CALLOUT objects.
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

#ifndef HELPERFUNCTIONS_FWPM_CALLOUT_H
#define HELPERFUNCTIONS_FWPM_CALLOUT_H

_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutDeleteByKey(_In_ const HANDLE engineHandle,
                                  _In_ const GUID* pCalloutKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutAdd(_In_ const HANDLE engineHandle,
                          _Inout_ FWPM_CALLOUT* pCallout);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                                _In_ const GUID* pProviderKey);

_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutEnum(_In_ const HANDLE engineHandle,
                           _In_ const HANDLE enumHandle,
                           _In_ const UINT32 numEntriesRequested,
                           _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_CALLOUT*** pppEntries,
                           _Out_ UINT32* pNumEntriesReturned);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                        _Inout_ HANDLE* pEnumHandle);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutCreateEnumHandle(_In_ const HANDLE engineHandle,
                                       _In_opt_ const FWPM_CALLOUT_ENUM_TEMPLATE* pEnumTemplate,
                                       _Out_ HANDLE* pEnumHandle);

#endif /// HELPERFUNCTIONS_FWPM_CALLOUT_H