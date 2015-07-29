////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmProviderContext.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_PROVIDER_CONTEXT objects.
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

#ifndef HELPERFUNCTIONS_FWPM_PROVIDER_CONTEXT_H
#define HELPERFUNCTIONS_FWPM_PROVIDER_CONTEXT_H

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextDeleteByKey(_In_ const HANDLE engineHandle,
                                          _In_ const GUID* pProviderContextKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextAdd(_In_ const HANDLE engineHandle,
                                  _Inout_ FWPM_PROVIDER_CONTEXT* pProviderContext);

_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextEnum(_In_ const HANDLE engineHandle,
                                   _In_ const HANDLE enumHandle,
                                   _In_ const UINT32 numEntriesRequested,
                                   _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_PROVIDER_CONTEXT*** pppEntries,
                                   _Out_ UINT32* pNumEntriesReturned);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                                _Inout_ HANDLE* pEnumHandle);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextCreateEnumHandle(_In_ const HANDLE engineHandle,
                                               _In_opt_ const FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE* pEnumTemplate,
                                               _Out_ HANDLE* pEnumHandle);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                                        _In_opt_ const GUID* pProviderContextKey);

#endif /// HELPERFUNCTIONS_FWPM_PROVIDER_CONTEXT_H