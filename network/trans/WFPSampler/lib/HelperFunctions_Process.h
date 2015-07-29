////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Process.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to 
//         processes.
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

#ifndef HELPERFUNCTIONS_PROCESS_H
#define HELPERFUNCTIONS_PROCESS_H


_Success_(return == NO_ERROR)
UINT32 HlprProcessGetID(_In_ PCWSTR pProcessName,
                        _Inout_ UINT32* pPID);

#endif /// HELPERFUNCTIONS_PROCESS_H