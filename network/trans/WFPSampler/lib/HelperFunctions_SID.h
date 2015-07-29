////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_SID.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to security
//         identifiers (SIDs).
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

#ifndef HELPERFUNCTIONS_SID_H
#define HELPERFUNCTIONS_SID_H

_At_(*ppSID, _Pre_ _Maybenull_)
_At_(*ppSID, _Post_ _Null_)
_Success_(*ppSID == 0)
VOID HlprSIDDestroy(_Inout_ SID** ppSID);

_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDCreate(_Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                     _Out_ SIZE_T* pSIDSize,
                     _In_opt_ PCWSTR pAccountName = 0,
                     _In_ UINT32 type = WinNullSid);

_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDGetForCurrentUser(_Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                                _Inout_ SIZE_T* pSIDSize);

_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDGetWellKnown(_In_ WELL_KNOWN_SID_TYPE sidType,
                           _Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                           _Inout_ UINT32* pSIDSize);

#endif /// HELPERFUNCTIONS_SID_H