////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmSubLayer.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         FWPM_SUBLAYER objects.
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

#ifndef HELPERFUNCTIONS_FWPM_SUBLAYER_H
#define HELPERFUNCTIONS_FWPM_SUBLAYER_H

_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerEnum(_In_ const HANDLE engineHandle,
                            _In_ const HANDLE enumHandle,
                            _In_ const UINT32 numEntriesRequested,
                            _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_SUBLAYER*** pppEntries,
                            _Out_ UINT32* pNumEntriesReturned);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                         _Inout_ HANDLE* pEnumHandle);

_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerCreateEnumHandle(_In_ const HANDLE engineHandle,
                                        _In_opt_ const FWPM_SUBLAYER_ENUM_TEMPLATE* pEnumTemplate,
                                        _Out_ HANDLE* pEnumHandle);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDeleteByKey(_In_ const HANDLE engineHandle,
                                   _In_ const GUID* pSubLayerKey);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerAdd(_In_ HANDLE engineHandle,
                           _In_ const GUID* pSubLayerKey,
                           _In_ PCWSTR pSubLayerName,
                           _In_ const GUID* pProviderKey,
                           _In_ UINT16 weight = 0x7FFF,
                           _In_ UINT32 flags = FWPM_SUBLAYER_FLAG_PERSISTENT);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDelete(_In_opt_ HANDLE* pEngineHandle,
                              _In_ const GUID* pSubLayerKey);

#endif /// HELPERFUNCTIONS_FWPM_SUBLAYER_H