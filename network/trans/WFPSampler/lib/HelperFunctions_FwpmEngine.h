////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmEngine.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to BFE 
//         engine objects.
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

#ifndef HELPERFUNCTIONS_FWPM_ENGINE_H
#define HELPERFUNCTIONS_FWPM_ENGINE_H

#define FWPM_SESSION_FLAG_NONDYNAMIC 0

_When_(return == NO_ERROR, _At_(*pEngineHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmEngineClose(_Inout_ HANDLE* pEngineHandle);

_At_(*pEngineHandle, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pEngineHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEngineHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmEngineOpen(_Out_ HANDLE* pEngineHandle,
                          _In_ const UINT32 sessionFlags = FWPM_SESSION_FLAG_NONDYNAMIC);

#endif /// HELPERFUNCTIONS_FWPM_ENGINE_H