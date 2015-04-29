/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   profman.h

Abstract:

   Color profile manager definition. The CProfileManager class is responsible
   for managing the color profile set in the driver.

--*/

#pragma once

#include "cmprofiledata.h"
#include "cmintentsdata.h"
#include "rescache.h"
#include "transform.h"

class CProfileManager : public IResWriter
{
public:
    CProfileManager(
        _In_ LPCWSTR                                                                                 pszDeviceName,
        _In_ XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData cmProfData,
        _In_ XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData                       cmIntData,
        _In_ IFixedPage*                                                                             pFP
        );

    virtual ~CProfileManager();

    HRESULT
    GetColorTransform(
        _Out_ HTRANSFORM* phColorTrans,
        _Out_ BOOL*       pbUseWCS
        );

    HRESULT
    GetDstProfileType(
        _Out_ XDPrintSchema::PageSourceColorProfile::EProfileOption* pType
        );

    HRESULT
    GetDstProfileName(
        _Inout_ BSTR* pbstrProfileName
        );

    HRESULT
    SetSrcProfileFromContainer(
        _In_ LPWSTR szProfileURI
        );

    HRESULT
    SetSrcProfileFromColDir(
        _In_ LPWSTR szProfile
        );

    HRESULT
    SetSrcProfileFromBuffer(
        _In_                  LPWSTR szProfile,
        _In_reads_bytes_(cbBuffer) PBYTE  pBuffer,
        _In_                  UINT   cbBuffer
        );

    HRESULT
    GetProfileOption(
        _Out_ XDPrintSchema::PageSourceColorProfile::EProfileOption* pProfileOption
        );

    //
    // IResWriter interface
    //
    HRESULT
    WriteData(
        _In_ IPartBase*         pResource,
        _In_ IPrintWriteStream* pStream
        );

    HRESULT
    GetKeyName(
        _Inout_ _At_(*pbstrKeyName, _Pre_maybenull_ _Post_valid_) BSTR* pbstrKeyName
        );

    HRESULT
    GetResURI(
        _Outptr_ BSTR* pbstrResURI
        );

private:
    HRESULT
    SetProfileFromColDir(
        _In_ CProfile* pProfile,
        _In_ LPWSTR    szProfile
        );

    HRESULT
    GetColDir(
        _Out_ CStringXDW* pcstrColDir
        );

private:
    CStringXDW                       m_strDeviceName;

    XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData m_cmProfData;

    XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData                       m_cmIntData;

    CProfile                         m_srcProfile;

    CProfile                         m_dstProfile;

    CTransform                       m_colorTrans;

    CComPtr<IFixedPage>              m_pFixedPage;
};

