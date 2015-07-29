////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Service.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         services.
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

#ifndef HELPERFUNCTIONS_SERVICE_H
#define HELPERFUNCTIONS_SERVICE_H

typedef enum _HLPR_SERVICE_COMMAND_
{
   HLPR_SERVICE_COMMAND_START,
   HLPR_SERVICE_COMMAND_STOP,
   HLPR_SERVICE_COMMAND_MAX
} HLPR_SERVICE_COMMAND;

_When_(return != NO_ERROR, _At_(*pSCMHandle, _Post_ _Null_))
_When_(return != NO_ERROR, _At_(*pSvcHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pSCMHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pSvcHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprServiceNotificationStateChangeRegister(_In_ PCWSTR pServiceName,
                                                  _In_ SERVICE_NOTIFY* pSvcNotify,
                                                  _In_ UINT32 notifyMask,
                                                  _Out_ SC_HANDLE* pSCMHandle,
                                                  _Out_ SC_HANDLE* pSvcHandle);

_Success_(return != 0)
UINT32 HlprServiceQueryState(_In_ PCWSTR pServiceName);

_Success_(return == NO_ERROR)
UINT32 HlprServiceStart(_In_ PCWSTR pServiceName);


_Success_(return == NO_ERROR)
UINT32 HlprServiceStop(_In_ PCWSTR pServiceName);

#endif /// HELPERFUNCTIONS_SERVICE_H