////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmProvider.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_PROVIDER objects.
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

#ifndef HELPERFUNCTIONS_FWPM_PROVIDER_H
#define HELPERFUNCTIONS_FWPM_PROVIDER_H

_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderEnum(_In_ const HANDLE engineHandle,
                            _In_ const HANDLE enumHandle,
                            _In_ const UINT32 numEntriesRequested,
                            _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_PROVIDER*** pppEntries,
                            _Out_ UINT32* pNumEntriesReturned);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                         _Inout_ HANDLE* pEnumHandle);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderCreateEnumHandle(_In_ const HANDLE engineHandle,
                                        _In_opt_ const FWPM_PROVIDER_ENUM_TEMPLATE* pEnumTemplate,
                                        _Out_ HANDLE* pEnumHandle);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDeleteByKey(_In_ const HANDLE engineHandle,
                                   _In_ const GUID* pProviderKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDelete(_In_opt_ HANDLE* pEngineHandle,
                              _In_ const GUID* pProviderKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderAdd(_In_ HANDLE engineHandle,
                           _In_ const GUID* pProviderKey,
                           _In_ PCWSTR pCompanyName,
                           _In_ PCWSTR pBinaryDescription,
                           _In_ PCWSTR pServiceName,
                           _In_ UINT32 flags);

#endif /// HELPERFUNCTIONS_FWPM_PROVIDER_H