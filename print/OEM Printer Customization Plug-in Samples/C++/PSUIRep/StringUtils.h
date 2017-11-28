//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   StringUtils.h
//
//  PURPOSE:  Header file for string utility routines.
//
#pragma once

////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////
_Success_(return >= 0 && *pwCount > 0) HRESULT MakeStrPtrList(HANDLE hHeap, PCSTR pmszMultiSz, _Outptr_result_buffer_(*pwCount) PCSTR **pppszList, _Out_ PWORD pwCount);
WORD mstrcount(PCSTR pmszMultiSz);
PWSTR MakeUnicodeString(HANDLE hHeap, PCSTR pszAnsi);
PWSTR MakeStringCopy(HANDLE hHeap, PCWSTR pszSource);
PSTR MakeStringCopy(HANDLE hHeap, PCSTR pszSource);
void FreeStringList(HANDLE hHeap, _In_reads_(wCount) PWSTR *ppszList, WORD wCount);
HRESULT GetStringResource(HANDLE hHeap, HMODULE hModule, UINT uResource, _Outptr_result_maybenull_ PWSTR *ppszString);
