////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Log.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to logging.
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

#ifndef HELPERFUNCTIONS_LOG_H
#define HELPERFUNCTIONS_LOG_H

VOID HlprLogError(_In_ PCWSTR pMessage,
                  ...);

VOID HlprLogInfo(_In_ PCWSTR pMessage,
                 ...);

#endif /// HELPERFUNCTIONS_LOG_H