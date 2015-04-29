/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wcsapiconv.h

Abstract:

   Provides a wrapper to the WCS API's

--*/

#pragma once

class CWCSApiConv;
extern CWCSApiConv g_WCSApiConv;

typedef BOOL (CALLBACK* WCSASSOCIATECOLORPROFILEWITHDEVICE)(WCS_PROFILE_MANAGEMENT_SCOPE, PCWSTR, PCWSTR);
typedef BOOL (CALLBACK* WCSDISASSOCIATECOLORPROFILEFROMDEVICE)(WCS_PROFILE_MANAGEMENT_SCOPE, PCWSTR, PCWSTR);
typedef BOOL (CALLBACK* WCSENUMCOLORPROFILESSIZE)(WCS_PROFILE_MANAGEMENT_SCOPE, PENUMTYPEW, PDWORD);
typedef BOOL (CALLBACK* WCSGETDEFAULTCOLORPROFILESIZE)(WCS_PROFILE_MANAGEMENT_SCOPE, PCWSTR, COLORPROFILETYPE, COLORPROFILESUBTYPE, DWORD, PDWORD);
typedef BOOL (CALLBACK* WCSGETDEFAULTCOLORPROFILE)(WCS_PROFILE_MANAGEMENT_SCOPE, PCWSTR, COLORPROFILETYPE, COLORPROFILESUBTYPE, DWORD, DWORD, LPWSTR);
typedef BOOL (CALLBACK* WCSSETDEFAULTCOLORPROFILE)(WCS_PROFILE_MANAGEMENT_SCOPE, PCWSTR, COLORPROFILETYPE, COLORPROFILESUBTYPE, DWORD, LPCWSTR);
typedef BOOL (CALLBACK* WCSSETDEFAULTRENDERINGINTENT)(WCS_PROFILE_MANAGEMENT_SCOPE, DWORD);
typedef BOOL (CALLBACK* WCSGETUSEPERUSERPROFILES)(LPCWSTR, DWORD, PBOOL);
typedef BOOL (CALLBACK* WCSSETUSEPERUSERPROFILES)(LPCWSTR, DWORD, BOOL);
typedef BOOL (CALLBACK* WCSTRANSLATECOLORS)(HTRANSFORM, DWORD, DWORD, COLORDATATYPE, DWORD, PVOID, DWORD, COLORDATATYPE, DWORD, PVOID);
typedef BOOL (CALLBACK* WCSCHECKCOLORS)(HTRANSFORM, DWORD, DWORD, COLORDATATYPE, DWORD, PVOID, PBYTE);
typedef HPROFILE (CALLBACK* WCSOPENCOLORPROFILE)(PPROFILE, PPROFILE, PPROFILE, DWORD, DWORD, DWORD, DWORD);
typedef HPROFILE (CALLBACK* WCSCREATEICCPROFILE)(HPROFILE, DWORD);

#define WcsAssociateColorProfileWithDeviceXD    g_WCSApiConv.WcsAssociateColorProfileWithDevice
#define WcsDisassociateColorProfileFromDeviceXD g_WCSApiConv.WcsDisassociateColorProfileFromDevice
#define WcsEnumColorProfilesSizeXD              g_WCSApiConv.WcsEnumColorProfilesSize
#define WcsGetDefaultColorProfileSizeXD         g_WCSApiConv.WcsGetDefaultColorProfileSize
#define WcsGetDefaultColorProfileXD             g_WCSApiConv.WcsGetDefaultColorProfile
#define WcsSetDefaultColorProfileXD             g_WCSApiConv.WcsSetDefaultColorProfile
#define WcsSetDefaultRenderingIntentXD          g_WCSApiConv.WcsSetDefaultRenderingIntent
#define WcsGetUsePerUserProfilesXD              g_WCSApiConv.WcsGetUsePerUserProfiles
#define WcsSetUsePerUserProfilesXD              g_WCSApiConv.WcsSetUsePerUserProfiles
#define WcsTranslateColorsXD                    g_WCSApiConv.WcsTranslateColors
#define WCSCheckColorsXD                        g_WCSApiConv.WCSCheckColors
#define WcsOpenColorProfileWXD                  g_WCSApiConv.WcsOpenColorProfileW
#define WcsOpenColorProfileAXD                  g_WCSApiConv.WcsOpenColorProfileA
#define WcsCreateIccProfileXD                   g_WCSApiConv.WcsCreateIccProfile

#ifdef _UNICODE
#define WcsOpenColorProfileXD WcsOpenColorProfileWXD
#else
#define WcsOpenColorProfileXD WcsOpenColorProfileAXD
#endif

template <typename _T>
class CEncodedFuncPtr
{
public:
    CEncodedFuncPtr() :
        m_pFunc(NULL)
    {
    }

    ~CEncodedFuncPtr(){}

    CEncodedFuncPtr<_T>&
    operator=(
         _In_ FARPROC pFunc
         )
    {
        m_pFunc = EncodePointer(pFunc);
        return *this;
    }

    BOOL
    operator==(
        _In_opt_ PVOID pv
        ) const
    {
        return m_pFunc == pv;
    }

    BOOL
    operator!=(
        _In_opt_ PVOID pv
        ) const
    {
        return !operator==(pv);
    }

    _T
    GetFunc(
        VOID
        )
    {
        return reinterpret_cast<_T>(DecodePointer(m_pFunc));
    }

private:
    __field_encoded_pointer PVOID m_pFunc;
};

class CWCSApiConv
{
public:
    CWCSApiConv();

    ~CWCSApiConv();

    BOOL
    WcsAssociateColorProfileWithDevice(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_ PCWSTR pProfileName,
        _In_ PCWSTR pDeviceName
        );

    BOOL
    WcsDisassociateColorProfileFromDevice(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_ PCWSTR pProfileName,
        _In_ PCWSTR pDeviceName
        );

    _Success_(return)
    BOOL
    WcsEnumColorProfilesSize(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_ PENUMTYPEW pEnumRecord,
        _Out_ PDWORD pdwSize
        );

    _Success_(return)
    BOOL
    WcsGetDefaultColorProfileSize(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_opt_ PCWSTR pDeviceName,
        _In_ COLORPROFILETYPE cptColorProfileType,
        _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
        _In_ DWORD dwProfileID,
        _Out_ PDWORD pcbProfileName
        );

    _Success_(return)
    BOOL
    WcsGetDefaultColorProfile(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_opt_ PCWSTR pDeviceName,
        _In_ COLORPROFILETYPE cptColorProfileType,
        _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
        _In_ DWORD dwProfileID,
        _In_ DWORD cbProfileName,
        _Out_writes_bytes_(cbProfileName) LPWSTR pProfileName
        );

    BOOL
    WcsSetDefaultColorProfile(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_opt_ PCWSTR pDeviceName,
        _In_ COLORPROFILETYPE cptColorProfileType,
        _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
        _In_ DWORD dwProfileID,
        _In_opt_ LPCWSTR pProfileName
        );

    BOOL
    WcsSetDefaultRenderingIntent(
        _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
        _In_ DWORD dwRenderingIntent
        );

    _Success_(return)
    BOOL
    WcsGetUsePerUserProfiles(
        _In_ LPCWSTR pDeviceName,
        _In_ DWORD dwDeviceClass,
        _Out_ PBOOL pUsePerUserProfiles
        );

    BOOL
    WcsSetUsePerUserProfiles(
        _In_ LPCWSTR pDeviceName,
        _In_ DWORD dwDeviceClass,
        _In_ BOOL usePerUserProfiles
        );

    BOOL
    WcsTranslateColors(
        _In_ HTRANSFORM hColorTransform,
        _In_ DWORD nColors,
        _In_ DWORD nInputChannels,
        _In_ COLORDATATYPE cdtInput,
        _In_ DWORD cbInput,
        _In_reads_bytes_(cbInput) PVOID pInputData,
        _In_ DWORD nOutputChannels,
        _In_ COLORDATATYPE cdtOutput,
        _In_ DWORD cbOutput,
        _Out_writes_bytes_(cbOutput)PVOID pOutputData
        );

    BOOL
    WCSCheckColors(
        _In_ HTRANSFORM hColorTransform,
        _In_ DWORD nColors,
        _In_ DWORD nInputChannels,
        _In_ COLORDATATYPE cdtInput,
        _In_ DWORD cbInput,
        _In_reads_bytes_(cbInput) PVOID pInputData,
        _Out_writes_bytes_(nColors)PBYTE pResult
    );

    HPROFILE WINAPI
    WcsOpenColorProfileA(
        _In_        PPROFILE pCDMPProfile,
        _In_opt_    PPROFILE pCAMPProfile,
        _In_opt_    PPROFILE pGMMPProfile,
        _In_        DWORD    dwDesireAccess,
        _In_        DWORD    dwShareMode,
        _In_        DWORD    dwCreationMode,
        _In_        DWORD    dwFlags
        );

    HPROFILE WINAPI
    WcsOpenColorProfileW(
        _In_        PPROFILE pCDMPProfile,
        _In_opt_    PPROFILE pCAMPProfile,
        _In_opt_    PPROFILE pGMMPProfile,
        _In_        DWORD    dwDesireAccess,
        _In_        DWORD    dwShareMode,
        _In_        DWORD    dwCreationMode,
        _In_        DWORD    dwFlags
        );

    HPROFILE
    WcsCreateIccProfile(
        _In_ HPROFILE hWcsProfile,
        _In_ DWORD dwOptions
        );

private:
    HINSTANCE m_dllHandle;

    //
    // WCS API
    //
    CEncodedFuncPtr<WCSASSOCIATECOLORPROFILEWITHDEVICE>    m_WcsAssociateColorProfileWithDevice;

    CEncodedFuncPtr<WCSDISASSOCIATECOLORPROFILEFROMDEVICE> m_WcsDisassociateColorProfileFromDevice;

    CEncodedFuncPtr<WCSENUMCOLORPROFILESSIZE>              m_WcsEnumColorProfilesSize;

    CEncodedFuncPtr<WCSGETDEFAULTCOLORPROFILESIZE>         m_WcsGetDefaultColorProfileSize;

    CEncodedFuncPtr<WCSGETDEFAULTCOLORPROFILE>             m_WcsGetDefaultColorProfile;

    CEncodedFuncPtr<WCSSETDEFAULTCOLORPROFILE>             m_WcsSetDefaultColorProfile;

    CEncodedFuncPtr<WCSSETDEFAULTRENDERINGINTENT>          m_WcsSetDefaultRenderingIntent;

    CEncodedFuncPtr<WCSGETUSEPERUSERPROFILES>              m_WcsGetUsePerUserProfiles;

    CEncodedFuncPtr<WCSSETUSEPERUSERPROFILES>              m_WcsSetUsePerUserProfiles;

    CEncodedFuncPtr<WCSTRANSLATECOLORS>                    m_WcsTranslateColors;

    CEncodedFuncPtr<WCSCHECKCOLORS>                        m_WcsCheckColors;

    CEncodedFuncPtr<WCSOPENCOLORPROFILE>                   m_WcsOpenColorProfileA;

    CEncodedFuncPtr<WCSOPENCOLORPROFILE>                   m_WcsOpenColorProfileW;

    CEncodedFuncPtr<WCSCREATEICCPROFILE>                   m_WcsCreateIccProfile;
};

