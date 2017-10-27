//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    StringUtils.cpp
//
//  PURPOSE:  Implementation wrapper class for WinXP PS Driver Features and Options.
//


#include "precomp.h"
#include "debug.h"
#include "devmode.h"
#include "globals.h"
#include "helper.h"
#include "features.h"
#include "oemui.h"
#include "stringutils.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

// Create a list of pointers to the strings
// in a multi-sz.
_Use_decl_annotations_
HRESULT MakeStrPtrList(HANDLE hHeap, PCSTR pmszMultiSz, PCSTR **pppszList, PWORD pwCount)
{
    PCSTR   *ppszList   = NULL;
    HRESULT hrResult    = S_OK;


    // Validate parameters
    if( (NULL == hHeap)
        ||
        (NULL == pmszMultiSz)
        ||
        (NULL == pppszList)
        ||
        (NULL == pwCount)
      )
    {
        return E_INVALIDARG;
    }

    // Get the count of strings in the multi-sz.
    *pwCount = mstrcount(pmszMultiSz);
    if(0 == *pwCount)
    {
        WARNING(DLLTEXT("MakeStrPtrList() pmszMultiSz contains no strings.\r\n"));

        *pppszList = NULL;

        goto Exit;
    }

    // Allocate pointer list.
    *pppszList = (PCSTR *) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (*pwCount) * sizeof(PCSTR));
    if(NULL == *pppszList)
    {
        ERR(ERRORTEXT("MakeStrPtrList() failed to allote array of PCSTR.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }
    ppszList = *pppszList;

    // Walk multi-sz mapping string pointers.
    for(WORD wIndex = 0; wIndex < *pwCount; ++wIndex)
    {
        ppszList[wIndex] = pmszMultiSz;
        pmszMultiSz += strlen(pmszMultiSz) + 1;
    }


Exit:

    return hrResult;
}


// Determine how many strings are in the multi-sz.
WORD mstrcount(PCSTR pmszMultiSz)
{
    WORD    wCount = 0;


    // NULL string pointers have no strings.
    if(NULL == pmszMultiSz)
    {
        return 0;
    }

    // Walk list of strings counting them.
    while(pmszMultiSz[0] != '\0')
    {
        ++wCount;
        pmszMultiSz += strlen(pmszMultiSz) + 1;
    }

    return wCount;
}

// Allocates and converts ANSI string to Unicode.
PWSTR MakeUnicodeString(HANDLE hHeap, PCSTR pszAnsi)
{
    int     nSize       = 0;
    PWSTR   pszUnicode  = NULL;


    // Validate parameters
    if( (NULL == hHeap)
        ||
        (NULL == pszAnsi)
      )
    {
        return NULL;
    }

    // Get the size needed for UNICODE string.
    nSize = MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, NULL, 0);
    if(0 != nSize)
    {
        // Allocate unicode string.
        pszUnicode = (PWSTR) HeapAlloc(hHeap, 0, nSize * sizeof(WCHAR));
        if(NULL != pszUnicode)
        {
            // Convert ANSI to Unicode
            nSize = MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, pszUnicode, nSize);
            if(0 == nSize)
            {
                // INVARIANT:  failed to convert.

                // free buffer and return NULL.
                HeapFree(hHeap, 0, pszUnicode);
                pszUnicode = NULL;
            }
        }
    }

    return pszUnicode;
}

// Allocates and copies source string.
PWSTR MakeStringCopy(HANDLE hHeap, PCWSTR pszSource)
{
    PWSTR   pszCopy = NULL;
    DWORD   dwSize;


    // Validate parameters
    if( (NULL == hHeap)
        ||
        (NULL == pszSource)
      )
    {
        return NULL;
    }

    // Allocate memory for string duplication, and duplicate the string.
    dwSize = (DWORD)(wcslen(pszSource) + 1) * sizeof(WCHAR);
    pszCopy = (PWSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
    if(NULL != pszCopy)
    {
        HRESULT hResult;


        hResult = StringCbCopyW(pszCopy, dwSize, pszSource);
        if(FAILED(hResult))
        {
            HeapFree(hHeap, 0, pszCopy);
            pszCopy = NULL;
            SetLastError(hResult);
        }
    }

    return pszCopy;
}

// Allocates and copies source string.
PSTR MakeStringCopy(HANDLE hHeap, PCSTR pszSource)
{
    PSTR    pszCopy = NULL;
    size_t  dwSize;


    // Validate parameters
    if( (NULL == hHeap)
        ||
        (NULL == pszSource)
      )
    {
        return NULL;
    }

    // Allocate memory for string duplication, and duplicate the string.
    dwSize = (strlen(pszSource) + 1) * sizeof(CHAR);
    pszCopy = (PSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
    if(NULL != pszCopy)
    {
        HRESULT hResult;

        hResult = StringCbCopyA(pszCopy, dwSize, pszSource);
        if(FAILED(hResult))
        {
            HeapFree(hHeap, 0, pszCopy);
            pszCopy = NULL;
            SetLastError(hResult);
        }
    }

    return pszCopy;
}

// Frees list of strings.
// NOTE: don't use this for string list made with MakeStrPtrList(), since
//       the strings pointed to be list made with MakeStrPtrList() will
//       be freed when the multi-sz is freed.
void FreeStringList(HANDLE hHeap, _In_reads_(wCount) PWSTR *ppszList, WORD wCount)
{
    // Validate parameters.
    if( (NULL == hHeap)
        ||
        (NULL == ppszList)
      )
    {
        return;
    }

    // Free each of the strings in the list.
    for(WORD wIndex = 0; wIndex < wCount; ++wIndex)
    {
        if(NULL != ppszList[wIndex]) HeapFree(hHeap, 0, ppszList[wIndex]);
    }

    // Free list.
    HeapFree(hHeap, 0, ppszList);
}


//  Retrieves pointer to a String resource.
HRESULT GetStringResource(HANDLE hHeap, HMODULE hModule, UINT uResource, _Outptr_result_maybenull_ PWSTR *ppszString)
{
    int     nResult;
    DWORD   dwSize      = MAX_PATH;
    PWSTR   pszString   = NULL;
    HRESULT hrResult    = S_OK;


    VERBOSE(DLLTEXT("GetStringResource(%#x, %#x, %d) entered.\r\n"), hHeap, hModule, uResource);

    // Validate parameters.
    if( (NULL == hHeap)
        ||
        (NULL == ppszString)
      )
    {
        return E_INVALIDARG;
    }

    // Allocate buffer for string resource from heap; let the driver clean it up.
    pszString = (PWSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(WCHAR));
    if(NULL == pszString)
    {
        ERR(ERRORTEXT("GetStringResource() failed to allocate string buffer!\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Load string resource; resize after loading so as not to waste memory.
    nResult = LoadString(hModule, uResource, pszString, dwSize);
    if(nResult > 0)
    {
        PWSTR   pszTemp;


        VERBOSE(DLLTEXT("LoadString() returned %d!\r\n"), nResult);
        VERBOSE(DLLTEXT("String load was \"%s\".\r\n"), pszString);

        pszTemp = (PWSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pszString, (nResult + 1) * sizeof(WCHAR));
        if(NULL != pszTemp)
        {
            pszString = pszTemp;
        }
        else
        {
            WARNING(DLLTEXT("GetStringResource() HeapReAlloc() of string retrieved failed! (Last Error was %d)\r\n"), GetLastError());
        }
    }
    else
    {
        DWORD   dwError = GetLastError();


        ERR(ERRORTEXT("LoadString() returned %d! (Last Error was %d)\r\n"), nResult, GetLastError());
        ERR(ERRORTEXT("GetStringResource() failed to load string resource %d!\r\n"), uResource);

        HeapFree(hHeap, 0, pszString);
        pszString   = NULL;
        hrResult    = HRESULT_FROM_WIN32(dwError);
    }


Exit:

    // Save string pointer to caller.
    // NOTE: string pointer will be NULL on failure.
    *ppszString = pszString;

    return hrResult;
}

