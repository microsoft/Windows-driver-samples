/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   profile.h

Abstract:

   Profile class definition. The profile class is responsible for loading and maintaining
   colo profile resources. The class provides methods for loading profiles from filename or
   URI and is responsible for limited caching based off the file name and the (WCS)OpenProfile
   options.

--*/

#pragma once

class CProfile
{
public:
    CProfile();

    ~CProfile();

    HRESULT
    SetProfile(
        _In_ IFixedPage* pFP,
        _In_ LPCWSTR     szURI
        );

    HRESULT
    SetProfile(
        _In_ LPCWSTR szURI
        );

    HRESULT
    SetProfile(
        _In_                  LPWSTR szProfile,
        _In_reads_bytes_(cbBuffer) PBYTE  pBuffer,
        _In_                  UINT   cbBuffer
        );

    HRESULT
    GetProfileHandle(
        _Out_ HPROFILE* phProfile
        );

    HRESULT
    IsWCSCompatible(
        _Out_ BOOL* pbWCSCompat
        );

    HRESULT
    GetProfileKey(
        _Out_ CStringXDW* pcstrProfileURI
        );

    BOOL
    operator==(
        _In_ CONST CStringXDW& cstrProfileKey
        ) CONST;

    BOOL
    operator!=(
        _In_ CONST CStringXDW& cstrProfileKey
        ) CONST;

private:
    HRESULT
    CreateProfileBuffer(
        VOID
        );

    VOID
    FreeProfile(
        VOID
        );

    HRESULT
    OpenProfile(
        _In_  PROFILE*  pProfile,
        _Out_ HPROFILE* phProfile
        );

    HRESULT
    CreateProfileKey(
        _In_     LPCWSTR pszCDMP,
        _In_opt_ LPCWSTR pszCAMP,
        _In_opt_ LPCWSTR pszGMMP,
        _Out_    CStringXDW* pcstrKey
        );

private:
    HPROFILE   m_hProfile;

    DWORD      m_dwDesiredAccess;

    DWORD      m_dwShareMode;

    DWORD      m_dwCreationMode;

    DWORD      m_dwWCSFlags;

    //
    // We define a key for a profile as the URI(s) plus any options used when
    // creating the profile. This allows anything that uses this profile in
    // combination with other profiles to identify the constituents of the
    // transform and hence cache that transform without needing to store the
    // profile handles.
    //
    // Note: Do not cache against the handle as handles can be re-used
    //
    CStringXDW m_cstrProfileKey;
};

