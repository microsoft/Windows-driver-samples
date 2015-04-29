/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   profile.cpp

Abstract:

   Profile class implementation. The profile class is responsible for loading and maintaining
   colo profile resources. The class provides methods for loading profiles from filename or
   URI and is responsible for limited caching based off the file name and the (WCS)OpenProfile
   options.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "profile.h"
#include "wcsapiconv.h"

DWORD g_tagWCSProfile = 'MS00';

/*++

Routine Name:

    CProfile::CProfile

Routine Description:

    CProfile constructor

Arguments:

    None

Return Value:

    None

--*/
CProfile::CProfile() :
    m_hProfile(NULL),
    m_dwDesiredAccess(PROFILE_READ),
    m_dwShareMode(FILE_SHARE_READ),
    m_dwCreationMode(OPEN_EXISTING),
    m_dwWCSFlags(0)
{
}

/*++

Routine Name:

    CProfile::~CProfile

Routine Description:

    CProfile destructor

Arguments:

    None

Return Value:

    None

--*/
CProfile::~CProfile()
{
    FreeProfile();
}

/*++

Routine Name:

    CProfile::SetProfile

Routine Description:

    Sets the current profile from the fixed page using the specified resoure URI

Arguments:

    pFP   - Pointer to the fixed page interface
    szURI - URI to the ICC profile to load

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::SetProfile(
    _In_ IFixedPage* pFP,
    _In_ LPCWSTR     szURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(szURI, E_POINTER)))
    {
        try
        {
            CStringXDW cstrKey;

            if (SUCCEEDED(hr = CreateProfileKey(szURI, NULL, NULL, &cstrKey)) &&
                (m_hProfile == NULL ||
                 m_cstrProfileKey != cstrKey))
            {
                FreeProfile();

                //
                // Get the read stream for the URI
                //
                CComPtr<IUnknown>          pPart(NULL);
                CComPtr<IPartColorProfile> pProfilePart(NULL);
                CComPtr<IPrintReadStream>  pRead(NULL);
                CComBSTR                   bstrProfileURI(szURI);

                ULONGLONG cbEnd = 0;

                //
                // Get the profile part and a read stream. Seek to the end to find the
                // size of the profile data
                //
                if (SUCCEEDED(hr = pFP->GetPagePart(bstrProfileURI, &pPart)) &&
                    SUCCEEDED(hr = pPart.QueryInterface(&pProfilePart)) &&
                    SUCCEEDED(hr = pProfilePart->GetStream(&pRead)) &&
                    SUCCEEDED(hr = pRead->Seek(0, STREAM_SEEK_END, &cbEnd)))
                {
                    //
                    // Allocate the buffer and copy
                    //
                    UINT  cbProfileData = static_cast<UINT>(cbEnd);
                    PBYTE pProfileData  = new(std::nothrow) BYTE[cbProfileData];

                    ULONG cbRead;
                    BOOL  bEOF = FALSE;
                    if (SUCCEEDED(hr = CHECK_POINTER(pProfileData, E_OUTOFMEMORY)) &&
                        SUCCEEDED(hr = pRead->Seek(0, STREAM_SEEK_SET, &cbEnd)) &&
                        SUCCEEDED(hr = pRead->ReadBytes(pProfileData, cbProfileData, &cbRead, &bEOF)))
                    {
                        if (cbProfileData != cbRead)
                        {
                            RIP("Failed to read all profile data\n");

                            hr = E_FAIL;
                        }
                    }

                    //
                    // Create the transform from the buffer
                    //
                    if (SUCCEEDED(hr))
                    {
                        PROFILE profile = {
                            PROFILE_MEMBUFFER,
                            pProfileData,
                            cbProfileData
                            };

                        if (SUCCEEDED(hr = OpenProfile(&profile, &m_hProfile)))
                        {
                            m_cstrProfileKey = cstrKey;
                        }
                    }

                    if (pProfileData != NULL)
                    {
                        delete[] pProfileData;
                        pProfileData = NULL;
                    }
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfile::SetProfile

Routine Description:

    Sets a profile from the specified file name

Arguments:

    szFileName - File name of the profile to open

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::SetProfile(
    _In_ LPCWSTR szFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)))
    {
        try
        {
            CStringXDW cstrKey;

            if (SUCCEEDED(hr = CreateProfileKey(szFileName, NULL, NULL, &cstrKey)) &&
                (m_hProfile == NULL ||
                 m_cstrProfileKey != cstrKey))
            {
                FreeProfile();

                CStringXDW profileFileName(szFileName);
                PROFILE profile = {
                    PROFILE_FILENAME,
                    profileFileName.GetBuffer(),
                    profileFileName.GetLength() * sizeof(WCHAR)
                    };

                if (SUCCEEDED(hr = OpenProfile(&profile, &m_hProfile)))
                {
                    m_cstrProfileKey = cstrKey;
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfile::SetProfile

Routine Description:

    Sets a color profile from memory

Arguments:

    szProfile - Name of the profile. This is used for caching puproses
    pBuffer   - Buffer containing the profile data
    cbBuffer  - Size of the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::SetProfile(
    _In_                  LPWSTR szProfile,
    _In_reads_bytes_(cbBuffer) PBYTE  pBuffer,
    _In_                  UINT   cbBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szProfile, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pBuffer, E_POINTER)))
    {
        try
        {
            CStringXDW cstrKey;

            if (SUCCEEDED(hr = CreateProfileKey(szProfile, NULL, NULL, &cstrKey)) &&
                (m_hProfile == NULL ||
                 m_cstrProfileKey != cstrKey))
            {
                FreeProfile();

                PROFILE profile = {
                    PROFILE_MEMBUFFER,
                    pBuffer,
                    cbBuffer
                    };

                if (SUCCEEDED(hr = OpenProfile(&profile, &m_hProfile)))
                {
                    m_cstrProfileKey = cstrKey;
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfile::GetProfileHandle

Routine Description:

    Retrieves the current profile handle

Arguments:

    phProfile - Pointer to a HPROFILE that accepts the current profile handle

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::GetProfileHandle(
    _Out_ HPROFILE* phProfile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(phProfile, E_POINTER)))
    {
        *phProfile = NULL;
        if (SUCCEEDED(hr = CHECK_HANDLE(m_hProfile, E_PENDING)))
        {
            *phProfile = m_hProfile;
        }
    }

    ERR_ON_HR_EXC(hr, E_PENDING);
    return hr;
}

/*++

Routine Name:

    CProfile::IsWCSCompatible

Routine Description:

    Checks if the current profile is suitable for use with WCS

Arguments:

    pbWCSCompat - Pointer to BOOL the accepts the result

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::IsWCSCompatible(
    _Out_ BOOL* pbWCSCompat
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbWCSCompat, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_HANDLE(m_hProfile, E_PENDING)))
    {
        //
        // Assume true as the GetColorProfileHeader documentation states: "This function
        // does not support Windows Color System (WCS) profiles CAMP, DMP, and GMMP.", therefore
        // if the profile is loaded and it is not ICC (i.e. no header) it should be WCS compatible
        //
        *pbWCSCompat = TRUE;

        PROFILEHEADER profileHeader = {0};
        if (GetColorProfileHeader(m_hProfile, &profileHeader))
        {
            if (profileHeader.phClass == CLASS_LINK ||
                profileHeader.phClass == CLASS_NAMED)
            {
                *pbWCSCompat = FALSE;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CProfile::GetProfileKey

Routine Description:

    Retrieves a key identifying the current profile. This is composed of the profile name
    and the options used to create the profile

Arguments:

    pcstrProfileKey - Pointer to a CStringXDW object that recieves the key

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::GetProfileKey(
    _Out_ CStringXDW* pcstrProfileKey
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcstrProfileKey, E_POINTER)))
    {
        try
        {
            *pcstrProfileKey = m_cstrProfileKey;
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfile::operator==

Routine Description:

    CProfile equality operator

Arguments:

    cstrProfileKey - the key to compare against

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
BOOL
CProfile::operator==(
    _In_ CONST CStringXDW& cstrProfileKey
    ) CONST
{
    BOOL bEqual = TRUE;

    try
    {
        if (m_cstrProfileKey != cstrProfileKey)
        {
            bEqual = FALSE;
        }
    }
    catch (CXDException&)
    {
        bEqual = FALSE;
    }

    return bEqual;
}

/*++

Routine Name:

    CProfile::operator!=

Routine Description:

    CProfile inequality operator

Arguments:

    cstrProfileKey - profile key to compare against

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
BOOL
CProfile::operator!=(
    _In_ CONST CStringXDW& cstrProfileKey
    ) CONST
{
    return !operator==(cstrProfileKey);
}

/*++

Routine Name:

    CProfile::FreeProfile

Routine Description:

    Frees the current profile

Arguments:

    None

Return Value:

    None

--*/
VOID
CProfile::FreeProfile(
    VOID
    )
{
    try
    {
        m_cstrProfileKey.Empty();
    }
    catch (CXDException&)
    {
    }

    if (m_hProfile != NULL)
    {
        CloseColorProfile(m_hProfile);
        m_hProfile = NULL;
    }
}

/*++

Routine Name:

    CProfile::OpenProfile

Routine Description:

    Opens the current profile based off the current OS. Vista uses WCSOpenProfile while
    earlier Windows use OpenProfile

Arguments:

    pProfile  - Pointer to the profile structure used to open the profile
    phProfile - Pointer to a HPROFILE to recieve the opened profile handle

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::OpenProfile(
    _In_  PROFILE*  pProfile,
    _Out_ HPROFILE* phProfile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pProfile, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(phProfile, E_POINTER)))
    {
        //
        // Get a handle to the color profile using WCS in Vista or the XP API in earlier operating systems
        //
        if (IsVista())
        {
            *phProfile = WcsOpenColorProfileXD(pProfile, NULL, NULL, m_dwDesiredAccess, m_dwShareMode, m_dwCreationMode, m_dwWCSFlags);
        }
        else
        {
            *phProfile = OpenColorProfile(pProfile, m_dwDesiredAccess, m_dwShareMode, m_dwCreationMode);
        }

        if (FAILED(CHECK_POINTER(*phProfile, E_POINTER)))
        {
            hr = GetLastErrorAsHResult();
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfile::CreateProfileKey

Routine Description:

    Creates a profile key based on the possible profile names used to construct the
    profile and the settings used to open the profile

Arguments:


    pszCDMP  - Device model profile name
    pszCAMP  - Color adjustment model profile name
    pszGMMP  - Gamut model profile name
    pcstrKey - Pointer to a CStringXDW that recieves the key

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfile::CreateProfileKey(
    _In_     LPCWSTR     pszCDMP,
    _In_opt_ LPCWSTR     pszCAMP,
    _In_opt_ LPCWSTR     pszGMMP,
    _Out_    CStringXDW* pcstrKey
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pszCDMP, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcstrKey, E_POINTER)))
    {
        try
        {
            pcstrKey->Empty();
            pcstrKey->Append(pszCDMP);

            if (pszCAMP != NULL)
            {
                pcstrKey->Append(pszCAMP);
            }

            if (pszGMMP != NULL)
            {
                pcstrKey->Append(pszGMMP);
            }

            CStringXDW cstrOpenOptions;
            cstrOpenOptions.Format(L"%x%x%x%x", m_dwDesiredAccess, m_dwShareMode, m_dwCreationMode, m_dwWCSFlags);

            pcstrKey->Append(cstrOpenOptions);
            pcstrKey->MakeLower();
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

