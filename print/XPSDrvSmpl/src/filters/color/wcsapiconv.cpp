/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wcsapiconv.cpp

Abstract:

   Implementation of the wrapper to the WCS API's

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "wcsapiconv.h"

CWCSApiConv g_WCSApiConv;

/*++

Routine Name:

    CWCSApiConv::CWCSApiConv

Routine Description:

    CWCSApiConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CWCSApiConv::CWCSApiConv()
{
   m_dllHandle = LoadLibrary(L"mscms.dll");

    //
    // If running on Vista, record the proc address for each WCS API
    //
    if (m_dllHandle != NULL &&
        IsVista())
    {
        m_WcsAssociateColorProfileWithDevice    = GetProcAddress(m_dllHandle,"WcsAssociateColorProfileWithDevice");
        m_WcsDisassociateColorProfileFromDevice = GetProcAddress(m_dllHandle, "WcsDisassociateColorProfileFromDevice");
        m_WcsEnumColorProfilesSize              = GetProcAddress(m_dllHandle, "WcsEnumColorProfilesSize");
        m_WcsGetDefaultColorProfileSize         = GetProcAddress(m_dllHandle, "WcsGetDefaultColorProfileSize");
        m_WcsGetDefaultColorProfile             = GetProcAddress(m_dllHandle, "WcsGetDefaultColorProfile");
        m_WcsSetDefaultColorProfile             = GetProcAddress(m_dllHandle, "WcsSetDefaultColorProfile");
        m_WcsSetDefaultRenderingIntent          = GetProcAddress(m_dllHandle, "WcsSetDefaultRenderingIntent");
        m_WcsGetUsePerUserProfiles              = GetProcAddress(m_dllHandle, "WcsGetUsePerUserProfiles");
        m_WcsSetUsePerUserProfiles              = GetProcAddress(m_dllHandle, "WcsSetUsePerUserProfiles");
        m_WcsTranslateColors                    = GetProcAddress(m_dllHandle, "WcsTranslateColors");
        m_WcsCheckColors                        = GetProcAddress(m_dllHandle, "WcsCheckColors");
        m_WcsOpenColorProfileA                  = GetProcAddress(m_dllHandle, "WcsOpenColorProfileA");
        m_WcsOpenColorProfileW                  = GetProcAddress(m_dllHandle, "WcsOpenColorProfileW");
        m_WcsCreateIccProfile                   = GetProcAddress(m_dllHandle, "WcsCreateIccProfile");

        ASSERTMSG(m_WcsAssociateColorProfileWithDevice    != NULL &&
                  m_WcsDisassociateColorProfileFromDevice != NULL &&
                  m_WcsEnumColorProfilesSize              != NULL &&
                  m_WcsGetDefaultColorProfileSize         != NULL &&
                  m_WcsGetDefaultColorProfile             != NULL &&
                  m_WcsSetDefaultColorProfile             != NULL &&
                  m_WcsSetDefaultRenderingIntent          != NULL &&
                  m_WcsGetUsePerUserProfiles              != NULL &&
                  m_WcsSetUsePerUserProfiles              != NULL &&
                  m_WcsTranslateColors                    != NULL &&
                  m_WcsCheckColors                        != NULL &&
                  m_WcsOpenColorProfileA                  != NULL &&
                  m_WcsOpenColorProfileW                  != NULL &&
                  m_WcsCreateIccProfile                   != NULL,
                  "Failed to load WCS APIs correctly.\n");
    }
}

/*++

Routine Name:

    CWCSApiConv::~CWCSApiConv

Routine Description:

    CWCSApiConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CWCSApiConv::~CWCSApiConv()
{
    FreeLibrary(m_dllHandle);
}

/*++

Routine Name:

    CWCSApiConv::WcsAssociateColorProfileWithDevice

Routine Description:


Arguments:

    scope - Profile management scope for this operation, which could be system wide or for current user.

    pProfileName - Points to the file name of the profile to associate.

    pDeviceName - Points to the name of the device to associate.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsAssociateColorProfileWithDevice(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_ PCWSTR pProfileName,
    _In_ PCWSTR pDeviceName
    )
{
    BOOL bResult = FALSE;

    if (m_WcsAssociateColorProfileWithDevice != NULL)
    {
        bResult = m_WcsAssociateColorProfileWithDevice.GetFunc()(scope, pProfileName, pDeviceName);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsDisassociateColorProfileFromDevice

Routine Description:


Arguments:

    scope - Profile management scope for this operation, which could be system wide or for current user.

    pProfileName - Pointer to the file name of the profile to disassociate.

    pDeviceName - Pointer to the name of the device to disassociate.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsDisassociateColorProfileFromDevice(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_ PCWSTR pProfileName,
    _In_ PCWSTR pDeviceName
    )
{
    BOOL bResult = FALSE;

    if (m_WcsDisassociateColorProfileFromDevice != NULL)
    {
        bResult = m_WcsDisassociateColorProfileFromDevice.GetFunc()(scope, pProfileName, pDeviceName);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsEnumColorProfilesSize

Routine Description:


Arguments:

    scope - management scope for this operation, which could be system wide or for current user.

    pEnumRecord - Pointer to the structure specifying the enumeration criteria.

    pdwSize - Returns the size in bytes required for the buffer to receive the set of profile names in WcsEnumColorProfiles.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
_Success_(return)
BOOL
#pragma warning(suppress: 6001 6101) // PREFast can't see through templated forwarder function call 
CWCSApiConv::WcsEnumColorProfilesSize(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_ PENUMTYPEW pEnumRecord,
    _Out_ PDWORD pdwSize
    )
{
    BOOL bResult = FALSE;

    if (m_WcsEnumColorProfilesSize != NULL)
    {
        bResult = m_WcsEnumColorProfilesSize.GetFunc()(scope, pEnumRecord, pdwSize);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsGetDefaultColorProfileSize

Routine Description:


Arguments:

    scope - Profile management scope for this operation, which could be system wide or for current user.

    pDeviceName - Pointer to the name of the device to get the default profile for. NULL implies device-independent default.

    cptColorProfileType - Specifies the color profile type value.

    cpstColorProfileSubType - Specifies the color profile subtype value.

    dwProfileID - Specifies the ID of the color space that the profile represents.

    pcbProfileName - the size in bytes for receiving the default profile in WcsGetDefaultColorProfile.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
_Success_(return)
BOOL
#pragma warning(suppress: 6001 6101) // PREFast can't see through templated forwarder function call 
CWCSApiConv::WcsGetDefaultColorProfileSize(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_opt_ PCWSTR pDeviceName,
    _In_ COLORPROFILETYPE cptColorProfileType,
    _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
    _In_ DWORD dwProfileID,
    _Out_ PDWORD pcbProfileName
    )
{
    BOOL bResult = FALSE;

    if (m_WcsGetDefaultColorProfileSize != NULL)
    {
        bResult = m_WcsGetDefaultColorProfileSize.GetFunc()(scope,
                                                            pDeviceName,
                                                            cptColorProfileType,
                                                            cpstColorProfileSubType,
                                                            dwProfileID,
                                                            pcbProfileName);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsGetDefaultColorProfile

Routine Description:


Arguments:

    scope - Profile management scope for this operation, which could be system wide or for current user.

    pDeviceName - Pointer to the name of the device to get the default profile for. NULL implies device-independent default.

    cptColorProfileType - Specifies the color profile type value.

    cpstColorProfileSubType - Specifies the color profile subtype value.

    dwProfileID - Specifies the ID of the color space that the profile represents.

    cbProfileName - The size in bytes of the buffer pointed to by pProfileName.

    pProfileName - Pointer to the buffer in which the name of the profile is to be placed.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
#pragma warning(suppress: 6001 6054 6101) // PREFast can't see through templated forwarder function call 
_Success_(return)
BOOL
CWCSApiConv::WcsGetDefaultColorProfile(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_opt_ PCWSTR pDeviceName,
    _In_ COLORPROFILETYPE cptColorProfileType,
    _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
    _In_ DWORD dwProfileID,
    _In_ DWORD cbProfileName,
    _Out_writes_bytes_(cbProfileName) LPWSTR pProfileName
    )
{
    BOOL bResult = FALSE;

    if (m_WcsGetDefaultColorProfile != NULL)
    {
        bResult = m_WcsGetDefaultColorProfile.GetFunc()(scope,
                                                       pDeviceName,
                                                       cptColorProfileType,
                                                       cpstColorProfileSubType,
                                                       dwProfileID,
                                                       cbProfileName,
                                                       pProfileName);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsSetDefaultColorProfile

Routine Description:


Arguments:

    scope - Profile management scope for this operation, which could be system wide or for current user.

    pDeviceName - Pointer to the name of the device to set the default profile for. NULL implies device-independent default.

    cptColorProfileType - Specifies the color profile type value.

    cpstColorProfileSubType - Specifies the color profile subtype value.

    dwProfileID - Specifies the ID of the color space that the profile represents.

    pProfileName - Pointer to the buffer in which contains the name of the profile.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsSetDefaultColorProfile(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_opt_ PCWSTR pDeviceName,
    _In_ COLORPROFILETYPE cptColorProfileType,
    _In_ COLORPROFILESUBTYPE cpstColorProfileSubType,
    _In_ DWORD dwProfileID,
    _In_opt_ LPCWSTR pProfileName
    )
{
    BOOL bResult = FALSE;

    if (m_WcsSetDefaultColorProfile != NULL)
    {
        bResult = m_WcsSetDefaultColorProfile.GetFunc()(scope,
                                                        pDeviceName,
                                                        cptColorProfileType,
                                                        cpstColorProfileSubType,
                                                        dwProfileID,
                                                        pProfileName);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsSetDefaultRenderingIntent

Routine Description:


Arguments:

    scope - management scope for this operation, which could be system wide or for current user.

    dwRenderingIntent - The rendering intent to set.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsSetDefaultRenderingIntent(
    _In_ WCS_PROFILE_MANAGEMENT_SCOPE scope,
    _In_ DWORD dwRenderingIntent
    )
{
    BOOL bResult = FALSE;

    if (m_WcsSetDefaultRenderingIntent != NULL)
    {
        bResult = m_WcsSetDefaultRenderingIntent.GetFunc()(scope, dwRenderingIntent);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsGetUsePerUserProfiles

Routine Description:


Arguments:

    pDeviceName - The friendly name of the device.

    dwDeviceClass - The class of the device: CLASS_SCANNER for a capture device,
                    CLASS_MONITOR for a display device, or CLASS_PRINTER for a printer.

    pUsePerUserProfiles - Pointer to a location to receive the result.
                          This location receives TRUE if the user has chosen to use a
                          per-user profile association list for the specified device; otherwise FALSE.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
_Success_(return)
BOOL
#pragma warning(suppress: 6001 6101) // PREFast can't see through templated forwarder function call 
CWCSApiConv::WcsGetUsePerUserProfiles(
    _In_ LPCWSTR pDeviceName,
    _In_ DWORD dwDeviceClass,
    _Out_ PBOOL pUsePerUserProfiles
    )
{
    BOOL bResult = FALSE;

    if (m_WcsGetUsePerUserProfiles != NULL)
    {
        bResult = m_WcsGetUsePerUserProfiles.GetFunc()(pDeviceName, dwDeviceClass, pUsePerUserProfiles);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsSetUsePerUserProfiles

Routine Description:


Arguments:

    pDeviceName - The friendly name of the device.

    dwDeviceClass - The class of the device: CLASS_SCANNER for a capture device,
                    CLASS_MONITOR for a display device, or CLASS_PRINTER for a printer.

    usePerUserProfiles - TRUE is the user wishes to use a per-user profile association
                         list for the specified device; otherwise FALSE.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsSetUsePerUserProfiles(
    _In_ LPCWSTR pDeviceName,
    _In_ DWORD dwDeviceClass,
    _In_ BOOL usePerUserProfiles
    )
{
    BOOL bResult = FALSE;

    if (m_WcsSetUsePerUserProfiles != NULL)
    {
        bResult = m_WcsSetUsePerUserProfiles.GetFunc()(pDeviceName, dwDeviceClass, usePerUserProfiles);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsTranslateColors

Routine Description:


Arguments:

    hColorTransform - Identifies the WCS color transform to use.

    nColors - Contains the number of elements in the arrays pointed to by pInputData and pOutputData.

    nInputChannels - Contains the number of channels per element in the array pointed to by pInputData

    cdtInput - Specifies the input COLORDATATYPTE.

    cbInput - Contains buffer size of pInputData.

    pInputData - Pointer to array of input colors.

    nOutputChannels - Contains the number of channels per element in the array pointed to by pOutputData.

    cdtOutput - Specifies the output COLORDATATYPTE.

    cbOutput - Contains buffer size of pOutputData.

    pOutputData - Pointer to array of output colors.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WcsTranslateColors(
    _In_ HTRANSFORM hColorTransform,
    _In_ DWORD nColors,
    _In_ DWORD nInputChannels,
    _In_ COLORDATATYPE cdtInput,
    _In_ DWORD cbInput,
    _In_reads_bytes_(cbInput) PVOID pInputData,
    _In_ DWORD nOutputChannels,
    _In_ COLORDATATYPE cdtOutput,
    _In_ DWORD cbOutput,
    _Out_writes_bytes_(cbOutput) PVOID pOutputData
    )
{
    BOOL bResult = FALSE;

    if (m_WcsTranslateColors != NULL)
    {
        bResult = m_WcsTranslateColors.GetFunc()(hColorTransform,
                                                 nColors,
                                                 nInputChannels,
                                                 cdtInput,
                                                 cbInput,
                                                 pInputData,
                                                 nOutputChannels,
                                                 cdtOutput,
                                                 cbOutput,
                                                 pOutputData);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WCSCheckColors

Routine Description:


Arguments:

    hColorTransform - Identifies the WCS color transform to use.

    nColors - Contains the number of elements in the arrays pointed to by pInputData and pResult.

    nInputChannels - Contains the number of channels per element in the array pointed to by pInputData

    cdtInput - Specifies the input COLORDATATYPTE.

    cbInput - Contains buffer size of pInputData.

    pInputData - Pointer to array of input colors.

    pResult - Pointer to array of nColor results of the test.

Return Value:

    BOOL
    If this function succeeds, the return value is TRUE.

--*/
BOOL
CWCSApiConv::WCSCheckColors(
    _In_ HTRANSFORM hColorTransform,
    _In_ DWORD nColors,
    _In_ DWORD nInputChannels,
    _In_ COLORDATATYPE cdtInput,
    _In_ DWORD cbInput,
    _In_reads_bytes_(cbInput) PVOID pInputData,
    _Out_writes_bytes_(nColors)PBYTE pResult
)
{
    BOOL bResult = FALSE;

    if (m_WcsCheckColors != NULL)
    {
        bResult = m_WcsCheckColors.GetFunc()(hColorTransform,
                                             nColors,
                                             nInputChannels,
                                             cdtInput,
                                             cbInput,
                                             pInputData,
                                             pResult);
    }

    return bResult;
}

/*++

Routine Name:

    CWCSApiConv::WcsOpenColorProfileA

Routine Description:


Arguments:

    pCDMPProfile - Pointer to a WCS DMP or an ICC color profile structure specifying the profile.

    pCAMPProfile - Pointer to a WCS CAMP color profile structure specifying the profile.
                   If this parameter is NULL, then the standard default CAMP is used.

    pGMMPProfile - Pointer to a WCS GMMP color profile structure specifying the profile.
                   If this parameter is NULL, then the standard default GMMP is used.

    dwDesireAccess - Specifies how to access the given profile. This parameter must take one the following constant values.

    dwShareMode - Specifies how the profile should be shared, if the profile is contained in a file.
                  A value of zero prevents the profile from being shared at all.

    dwCreationMode - Specifies which actions to take on the profile while opening it, if it is contained in a file.


Return Value:

    HPROFILE - Handle to the opened color profile

--*/
HPROFILE
CWCSApiConv::WcsOpenColorProfileA(
    _In_        PPROFILE pCDMPProfile,
    _In_opt_    PPROFILE pCAMPProfile,
    _In_opt_    PPROFILE pGMMPProfile,
    _In_        DWORD    dwDesireAccess,
    _In_        DWORD    dwShareMode,
    _In_        DWORD    dwCreationMode,
    _In_        DWORD    dwFlags
    )
{
    HPROFILE hProfile = NULL;

    if (m_WcsOpenColorProfileA != NULL)
    {
        hProfile = m_WcsOpenColorProfileA.GetFunc()(pCDMPProfile,
                                                    pCAMPProfile,
                                                    pGMMPProfile,
                                                    dwDesireAccess,
                                                    dwShareMode,
                                                    dwCreationMode,
                                                    dwFlags);
    }

    return hProfile;
}

/*++

Routine Name:

    CWCSApiConv::WcsOpenColorProfileW

Routine Description:


Arguments:

    pCDMPProfile - Pointer to a WCS DMP or an ICC color profile structure specifying the profile.

    pCAMPProfile - Pointer to a WCS CAMP color profile structure specifying the profile.
                   If this parameter is NULL, then the standard default CAMP is used.

    pGMMPProfile - Pointer to a WCS GMMP color profile structure specifying the profile.
                   If this parameter is NULL, then the standard default GMMP is used.

    dwDesireAccess - Specifies how to access the given profile. This parameter must take one the following constant values.

    dwShareMode - Specifies how the profile should be shared, if the profile is contained in a file.
                  A value of zero prevents the profile from being shared at all.

    dwCreationMode - Specifies which actions to take on the profile while opening it, if it is contained in a file.


Return Value:

    HPROFILE - Handle to the opened color profile

--*/
HPROFILE
CWCSApiConv::WcsOpenColorProfileW(
    _In_        PPROFILE pCDMPProfile,
    _In_opt_    PPROFILE pCAMPProfile,
    _In_opt_    PPROFILE pGMMPProfile,
    _In_        DWORD    dwDesireAccess,
    _In_        DWORD    dwShareMode,
    _In_        DWORD    dwCreationMode,
    _In_        DWORD    dwFlags
    )
{
    HPROFILE hProfile = NULL;

    if (m_WcsOpenColorProfileW != NULL)
    {
        hProfile = m_WcsOpenColorProfileW.GetFunc()(pCDMPProfile,
                                                    pCAMPProfile,
                                                    pGMMPProfile,
                                                    dwDesireAccess,
                                                    dwShareMode,
                                                    dwCreationMode,
                                                    dwFlags);
    }

    return hProfile;
}

/*++

Routine Name:

    CWCSApiConv::WcsCreateIccProfile

Routine Description:


Arguments:

    hWcsProfile - handle to a  WCS profile which contains a combination
                  of a WCS DMP, and optionally a CAMP and/or GMMP.

    dwOptions   - Options flag values from WCS_DEFAULT and WCS_ICCONLY

Return Value:

    HPROFILE - Handle to the newly created color profile

--*/
HPROFILE
CWCSApiConv::WcsCreateIccProfile(
    _In_ HPROFILE hWcsProfile,
    _In_ DWORD dwOptions
    )
{
    HPROFILE hProfile = NULL;

    if (m_WcsCreateIccProfile != NULL)
    {
        hProfile = m_WcsCreateIccProfile.GetFunc()(hWcsProfile, dwOptions);
    }

    return hProfile;
}

