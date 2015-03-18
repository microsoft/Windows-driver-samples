/*++

Copyright (c) 1990-2003 Microsoft Corporation
All Rights Reserved

Module Name:

    debug.c

Abstract:

    This module contains all debugging routines

[Environment:]

    NT Windows - Common Printer Driver UI DLL.

[Notes:]

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG

BOOL DoCPSUIWarn = TRUE;

VOID
cdecl
CPSUIDbgPrint
(
    LPCSTR   pszFormat,
    ...
)
/*++

Routine Description:

    This fucntion output the debug informat to the debugger

Arguments:

    pszFormat   - format string

    ...         - variable data

Return Value:

    VOID

--*/
{
    va_list         vaList;
    static TCHAR    OutBuf[768];
#ifdef UNICODE
    static WCHAR    FormatBuf[256];
#endif
    //
    // We assume that UNICODE flag is turn on for the compilation, bug the
    // format string passed to here is ASCII version, so we need to convert
    // it to LPWSTR before the wvsprintf()
    //

    va_start(vaList, pszFormat);

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, pszFormat, -1, FormatBuf, COUNT_ARRAY(FormatBuf));
    StringCchVPrintf(OutBuf, COUNT_ARRAY(OutBuf), FormatBuf, vaList);
#else
    StringCchVPrintf(OutBuf, COUNT_ARRAY(OutBuf), pszFormat, vaList);
#endif
    va_end(vaList);

    OutputDebugString((LPTSTR)OutBuf);
    OutputDebugString(TEXT("\n"));
}

VOID
CPSUIDbgType
(
    INT    Type
)
/*++

Routine Description:

    this function output the ERROR/WARNING message

Arguments:

    Type

Return Value:

--*/
{
    static TCHAR DebugDLLName[] = TEXT("SurPtrUI");

    if (Type < 0)
    {
        OutputDebugString(TEXT("ERROR) "));
    }
    else if (Type > 0)
    {
        OutputDebugString(TEXT("WARNING: "));
    }
    OutputDebugString(DebugDLLName);
    OutputDebugString(TEXT("!"));
}

#endif  // DBG
