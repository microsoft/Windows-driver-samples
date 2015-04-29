/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   globals.cpp

Abstract:

   Stores function implementations and variable instances global to the filter
   module. This is shared between all filters.

--*/

#include "precomp.h"
#include <VersionHelpers.h>

//
// Module's Instance handle from DLLEntry of process.
//
HINSTANCE g_hInstance = NULL;

//
// Server lock count
//
LONG g_cServerLocks = 0;

/*++

Routine Name:

    GetLastErrorAsHResult

Routine Description:

    Converts the Win32 last error to an HRESULT

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
GetLastErrorAsHResult(
    void
    )
{
    DWORD error = GetLastError();

    return HRESULT_FROM_WIN32(error);
}

/*++

Routine Name:

    GetLastErrorAsHResult

Routine Description:

    Converts a GDI status error to an HRESULT

Arguments:

    gdiPStatus - The GDI plus status value to be converted to an HRESULT

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
GetGDIStatusErrorAsHResult(
    _In_ Status gdiPStatus
    )
{
    HRESULT hr = E_FAIL;

    switch (gdiPStatus)
    {
        case Ok:
        {
            hr = S_OK;
        }
        break;

        case GenericError:
        {
            hr = E_FAIL;
        }
        break;

        case InvalidParameter:
        {
            hr = E_INVALIDARG;
        }
        break;

        case OutOfMemory:
        {
            hr = E_OUTOFMEMORY;
        }
        break;

        case ObjectBusy:
        {
            hr = E_PENDING;
        }
        break;

        case InsufficientBuffer:
        {
            hr = E_OUTOFMEMORY;
        }
        break;

        case NotImplemented:
        {
            hr = E_NOTIMPL;
        }
        break;

        case Win32Error:
        {
            hr = GetLastErrorAsHResult();
        }
        break;

        case FileNotFound:
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        break;

        case AccessDenied:
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
        break;

        //
        // Default to E_FAIL
        //
        case UnknownImageFormat:
        case FontFamilyNotFound:
        case FontStyleNotFound:
        case NotTrueTypeFont:
        case UnsupportedGdiplusVersion:
        case GdiplusNotInitialized:
        case PropertyNotFound:
        case PropertyNotSupported:
        case ValueOverflow:
        case Aborted:
        case WrongState:
        default:
            break;
    };

    return hr;
}

/*++

Routine Name:

    IsVista

Routine Description:

    Checks if we are running under Vista

Arguments:

    None

Return Value:

    TRUE if we are under Vista
    FALSE otherwise

--*/

BOOL
IsVista(
    VOID
    )
{
    BOOL bIsVista = FALSE;
    bIsVista = IsWindowsVistaOrGreater();
    return bIsVista;
}

/*++

Routine Name:

    GetUniqueNumber

Routine Description:

    Returns a unique number.

Arguments:

    None

Return Value:

    A unique DWORD.

--*/

DWORD
GetUniqueNumber(
    VOID
    )
{
    static volatile DWORD number = 0;

    return InterlockedIncrement(&number);
}
