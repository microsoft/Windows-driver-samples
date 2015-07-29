////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_GUID.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to GUIDs.
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

#ifndef HELPERFUNCTIONS_GUID_H
#define HELPERFUNCTIONS_GUID_H

VOID HlprGUIDPurge(_Inout_ GUID* pGUID);

_At_(*ppGUID, _Post_ _Null_)
VOID HlprGUIDDestroy(_Inout_ GUID** ppGUID);

_Success_(return == NO_ERROR)
UINT32 HlprGUIDPopulate(_Inout_ GUID* pGUID);

_At_(*ppGUID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppGUID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppGUID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprGUIDCreate(_Outptr_ GUID** ppGUID);

_When_(return != NO_ERROR, _At_(*ppGUIDString, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*ppGUIDString, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprGUIDDestroyString(_Inout_ PWSTR* ppGUIDString);

_Success_(return != 0)
PWSTR HlprGUIDCreateString(_In_ const GUID* pGUID);

BOOLEAN HlprGUIDsAreEqual(_In_ const GUID* pGUIDAlpha,
                          _In_ const GUID* pGUIDOmega);

BOOLEAN HlprGUIDIsNull(_In_ const GUID* pGUID);

#endif /// HELPERFUNCTIONS_GUID_H