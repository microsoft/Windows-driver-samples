////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Strings.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to strings.
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

#ifndef HELPERFUNCTIONS_STRINGS_H
#define HELPERFUNCTIONS_STRINGS_H

BOOLEAN HlprStringsAreEqual(_In_reads_(stringSizeAlpha) PCWSTR pStringAlpha,
                            _In_ SIZE_T stringSizeAlpha,
                            _In_reads_(stringSizeOmega) PCWSTR pStringOmega,
                            _In_ SIZE_T stringSizeOmega,
                            _In_ BOOLEAN isCaseSensitive = FALSE);
BOOLEAN HlprStringsAreEqual(_In_opt_ PCWSTR pStringAlpha,
                            _In_opt_ PCWSTR pStringOmega,
                            _In_ BOOLEAN isCaseSensitive = FALSE);
BOOLEAN HlprStringsAreEqual(_In_ const UNICODE_STRING* pUnicodeStringAlpha,
                            _In_ const UNICODE_STRING* pUnicodeStringOmega,
                            _In_ BOOLEAN isCaseSensitive = FALSE);

#endif /// HELPERFUNCTIONS_STRINGS_H