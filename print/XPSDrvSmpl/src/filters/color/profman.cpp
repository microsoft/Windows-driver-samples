/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   profman.cpp

Abstract:

   Color profile class implementation. The CProfileManager class represents
   a color profile and provides a transform from the system default profile
   to the supplied profile.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "profman.h"

using XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData;
using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

using XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData;
using XDPrintSchema::PageICMRenderingIntent::AbsoluteColorimetric;
using XDPrintSchema::PageICMRenderingIntent::RelativeColorimetric;
using XDPrintSchema::PageICMRenderingIntent::Photographs;
using XDPrintSchema::PageICMRenderingIntent::BusinessGraphics;


/*++

Routine Name:

    CProfileManager::CProfileManager

Routine Description:

    Constructor for the CProfileManager class which records internally the device name,
    color profile structure, color intents data and initialises the CProfileManager
    ready to supply suitable color transforms

Arguments:

    pszDeviceName - Pointer to a string containing the device name
    cmProfData    - Structure containing color profile settings from the PrintTicket
    cmIntData     - Structure containing color intents settings from the PrintTicket

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CProfileManager::CProfileManager(
    _In_ LPCWSTR                               pszDeviceName,
    _In_ PageSourceColorProfileData cmProfData,
    _In_ PageICMRenderingIntentData            cmIntData,
    _In_ IFixedPage*                           pFP
    ) :
    m_strDeviceName(pszDeviceName),
    m_cmProfData(cmProfData),
    m_cmIntData(cmIntData),
    m_pFixedPage(pFP)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pszDeviceName, E_POINTER)) ||
        SUCCEEDED(hr = CHECK_POINTER(m_pFixedPage, E_POINTER)))
    {
        if(m_strDeviceName.GetLength() <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(E_INVALIDARG);
    }
}

/*++

Routine Name:

    CProfileManager::~CProfileManager

Routine Description:

    Default destructor for the CProfileManager class which performs and clean up

Arguments:

    None

Return Value:

    None

--*/
CProfileManager::~CProfileManager()
{
}

/*++

Routine Name:

    CProfileManager::GetColorTransform

Routine Description:

    Method which supplies a color transform based on the settings in the PrintTicket

Arguments:

    phColorTrans - Pointer to a color transform handle which will contain
                   a transform based off the settings in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetColorTransform(
    _Out_ HTRANSFORM* phColorTrans,
    _Out_ BOOL*       pbUseWCS
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbUseWCS, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_HANDLE(phColorTrans, E_POINTER)) &&
        SUCCEEDED(hr = m_dstProfile.SetProfile(m_cmProfData.cmProfileName)))
    {
        *pbUseWCS = IsVista();

        try
        {
            //
            // Construct and populate a profile list for the transform object
            //
            ProfileList profileList;

            //
            // Check the source profile has been set. GetProfileHandle will return an error
            // if it has not been.
            //
            HPROFILE hProfile = NULL;
            if (SUCCEEDED(hr = m_srcProfile.GetProfileHandle(&hProfile)))
            {
                profileList.push_back(&m_srcProfile);
                profileList.push_back(&m_dstProfile);

                //
                // Map the PrintTicket intents option to the ICM option
                //
                DWORD intents = INTENT_ABSOLUTE_COLORIMETRIC;
                switch (m_cmIntData.cmOption)
                {
                    case AbsoluteColorimetric:
                    {
                        intents = INTENT_ABSOLUTE_COLORIMETRIC;
                    }
                    break;

                    case RelativeColorimetric:
                    {
                        intents = INTENT_RELATIVE_COLORIMETRIC;
                    }
                    break;

                    case Photographs:
                    {
                        intents = INTENT_PERCEPTUAL;
                    }
                    break;

                    case BusinessGraphics:
                    default:
                    {
                        intents = INTENT_SATURATION;
                    }
                    break;
                }

                DWORD flRender = BEST_MODE;

                //
                // Check if the source and destination profiles are compatible with WCS
                //
                if (*pbUseWCS &&
                    SUCCEEDED(hr = m_srcProfile.IsWCSCompatible(pbUseWCS)))
                {
                    if (*pbUseWCS)
                    {
                        hr = m_dstProfile.IsWCSCompatible(pbUseWCS);
                    }
                }

                if (SUCCEEDED(hr) &&
                    *pbUseWCS)
                {
                    //
                    // Everything is in place for using WCS
                    //
                    flRender |= WCS_ALWAYS;
                }

                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = m_colorTrans.CreateTransform(&profileList, intents, flRender)))
                {
                    hr = m_colorTrans.GetTransformHandle(phColorTrans);
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::GetDstProfileType

Routine Description:

    Method to return the color profile type as read from the PrintTicket

Arguments:

    pType - Pointer to the profile type to be filled in

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetDstProfileType(
    _Out_ EProfileOption* pType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pType, E_POINTER)))
    {
        *pType = m_cmProfData.cmProfile;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::GetDstProfileName

Routine Description:

    Method which returns the systems default colour profile name

Arguments:

    pbstrProfileName - Pointer to string to hold the profile name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetDstProfileName(
    _Inout_ BSTR* pbstrProfileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrProfileName, E_POINTER)))
    {
        try
        {
            CStringXDW cstrURI;

            if (m_cmProfData.cmProfile == RGB ||
                m_cmProfData.cmProfile == CMYK)
            {
                if (SUCCEEDED(hr = GetColDir(&cstrURI)))
                {
                    cstrURI += L"\\";
                    cstrURI += m_cmProfData.cmProfileName;
                }
            }
            else
            {
                hr = E_NOTIMPL;
            }
            SysFreeString(*pbstrProfileName);
            *pbstrProfileName = cstrURI.AllocSysString();
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

    CProfileManager::SetSrcProfileFromContainer

Routine Description:

    Method which sets the source colour profile from a profile within the
    XPS container

Arguments:

    pszProfileURI - Pointer to string holding the profile URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::SetSrcProfileFromContainer(
    _In_ LPWSTR szProfileURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szProfileURI, E_POINTER)))
    {
        hr = m_srcProfile.SetProfile(m_pFixedPage, szProfileURI);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::SetSrcProfileFromColDir

Routine Description:

    Method which sets the source colour profile from the color directory

Arguments:

    pszProfile - Pointer to string holding the profile name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::SetSrcProfileFromColDir(
    _In_ LPWSTR szProfile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szProfile, E_POINTER)))
    {
        hr = SetProfileFromColDir(&m_srcProfile, szProfile);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::SetSrcProfileFromBuffer

Routine Description:

    Method which sets the source colour profile from a buffer

Arguments:

    pszProfile - Pointer to string holding the profile name
    pBuffer    - Pointer to buffer holding the profile data
    cbBuffer   - Count of bytes in the profile buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::SetSrcProfileFromBuffer(
    _In_                  LPWSTR szProfile,
    _In_reads_bytes_(cbBuffer) PBYTE  pBuffer,
    _In_                  UINT   cbBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szProfile, E_POINTER)) ||
        SUCCEEDED(hr = CHECK_POINTER(pBuffer, E_POINTER)))
    {
        hr = m_srcProfile.SetProfile(szProfile, pBuffer, cbBuffer);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::GetProfileOption

Routine Description:

    This method retrieves the profile option set in the PrintTicket

Arguments:

    pProfileOption - Pointer to a profile option enumeration type to recieve the option

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetProfileOption(
    _Out_ EProfileOption* pProfileOption
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pProfileOption, E_POINTER)))
    {
        *pProfileOption = m_cmProfData.cmProfile;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::WriteData

Routine Description:

    This method handles writing the destination profile to the container

Arguments:

    pStream - Pointer to a stream to write the resource out to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::WriteData(
    _In_ IPartBase*         pResource,
    _In_ IPrintWriteStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pResource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)))
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;

        //
        // Open the destination profile file from disk. We could use the
        // handle however we do not know whether we will get the ICC or DMP.
        //
        CComBSTR bstrProfile;
        if (SUCCEEDED(hr = GetDstProfileName(&bstrProfile)))
        {
            hFile = CreateFile(bstrProfile,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                hr = GetLastErrorAsHResult();
            }
        }

        if (SUCCEEDED(hr))
        {
            //
            // Write profile data to stream
            //
            PBYTE pBuff = new(std::nothrow) BYTE[CB_COPY_BUFFER];
            hr = CHECK_POINTER(pBuff, E_OUTOFMEMORY);

            DWORD cbRead = 0;

            while (SUCCEEDED(hr))
            {
                if (ReadFile(hFile, pBuff, CB_COPY_BUFFER, &cbRead, NULL))
                {
                    if (cbRead > 0)
                    {
                        ULONG cbWritten = 0;
                        hr = pStream->WriteBytes(reinterpret_cast<LPVOID>(pBuff), cbRead, &cbWritten);

                        if (cbRead != cbWritten)
                        {
                            RIP("Failed to write all profile data.\n");

                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    hr = GetLastErrorAsHResult();
                }
            }

            delete[] pBuff;
            pBuff = NULL;
        }

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::GetKeyName

Routine Description:

    Method to obtain a unique key for the resource being handled

Arguments:

    pbstrKeyName - Pointer to a string to hold the generated key

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetKeyName(
    _Inout_ _At_(*pbstrKeyName, _Pre_maybenull_ _Post_valid_) BSTR* pbstrKeyName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrKeyName, E_POINTER)))
    {
        SysFreeString(*pbstrKeyName);

        if (SUCCEEDED(hr = m_cmProfData.cmProfileName.CopyTo(pbstrKeyName)) &&
            !*pbstrKeyName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CProfileManager::GetResURI

Routine Description:

    Method to obtain the URI of the resource being handled

Arguments:

    pbstrResURI - Pointer to a string to hold the resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetResURI(
    _Outptr_ BSTR* pbstrResURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrResURI, E_POINTER)))
    {
        try
        {
            CStringXDW cstrFileName(m_cmProfData.cmProfileName);
            CStringXDW cstrFileExt(PathFindExtension(cstrFileName));

            INT indFileExt = cstrFileName.Find(cstrFileExt);

            if (indFileExt > -1)
            {
                cstrFileName.Delete(indFileExt, cstrFileExt.GetLength());
            }

            //
            // Create a unique name for the profile for this print session
            //
            CStringXDW cstrURI;
            cstrURI.Format(L"/%s_%u%s", cstrFileName, GetUniqueNumber(), cstrFileExt);

            *pbstrResURI = cstrURI.AllocSysString();
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

    CProfileManager::SetProfileFromColDir

Routine Description:

    Set the named profile from the color directory. The method appends the specified
    file name to the color directory path and instructs the profile class to open the
    profile

Arguments:

    pProfile  - Pointer to the profile class that actually opens the profile
    szProfile - The profile file name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::SetProfileFromColDir(
    _In_ CProfile* pProfile,
    _In_ LPWSTR    szProfile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pProfile, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(szProfile, E_POINTER)))
    {
        try
        {
            CStringXDW cstrProfile;

            if (SUCCEEDED(hr = GetColDir(&cstrProfile)))
            {
                cstrProfile += L"\\";
                cstrProfile += szProfile;

                hr = pProfile->SetProfile(cstrProfile.GetBuffer());
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

    CProfileManager::GetColDir

Routine Description:

    Retrieves the color directory path

Arguments:

    pcstrColDir - Pointer to a string class that accepts the color directory path

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CProfileManager::GetColDir(
    _Out_ CStringXDW* pcstrColDir
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcstrColDir, E_POINTER)))
    {
        try
        {
            pcstrColDir->Empty();
            DWORD cbColDir = 0;
            if (!GetColorDirectory(NULL, NULL, &cbColDir))
            {
                pcstrColDir->Preallocate(cbColDir/sizeof(WCHAR));
                if (!GetColorDirectory(NULL, pcstrColDir->GetBuffer(), &cbColDir))
                {
                    hr = GetLastErrorAsHResult();
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

