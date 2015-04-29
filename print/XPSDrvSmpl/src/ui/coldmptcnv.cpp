/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   coldmptcnv.cpp

Abstract:

   PageSourceColorProfile devmode <-> PrintTicket conversion class implementation.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "cmprofpchndlr.h"
#include "coldmptcnv.h"

using XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData;
using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

PCWSTR g_pszCMYKProfileName = L"xdCMYKPrinter.icc";
PCWSTR g_pszRGBProfileName  = L"xdwscRGB.icc";
PCSTR g_pszColProfFeature = "PageSourceColorProfile";
static GPDStringToOption<EProfileOption> g_colProfTypeOption[] = {
    {"CMYK",       CMYK},
    {"scRGB",      RGB},
};
UINT g_cColProfOption = sizeof(g_colProfTypeOption)/sizeof(GPDStringToOption<EProfileOption>);

/*++

Routine Name:

    CColorProfileDMPTConv::CColorProfileDMPTConv

Routine Description:

    CColorProfileDMPTConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CColorProfileDMPTConv::CColorProfileDMPTConv()
{
}

/*++

Routine Name:

    CColorProfileDMPTConv::~CColorProfileDMPTConv

Routine Description:

    CColorProfileDMPTConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorProfileDMPTConv::~CColorProfileDMPTConv()
{
}

/*++

Routine Name:

    CColorProfileDMPTConv::GetPTDataSettingsFromDM

Routine Description:

    Populates the color profile data structure from the Devmode passed in.

Arguments:

    pDevmode - pointer to input devmode buffer.
    cbDevmode - size in bytes of full input devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.
    pDataSettings - Pointer to color profile data structure to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorProfileDMPTConv::GetPTDataSettingsFromDM(
    _In_    PDEVMODE         pDevmode,
    _In_    ULONG            cbDevmode,
    _In_    PVOID            pPrivateDevmode,
    _In_    ULONG            cbDrvPrivateSize,
    _Out_   PageSourceColorProfileData* pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Defer setting up the profile file names until we write to the PT
    // as they only have meaning at that point
    //
    if (SUCCEEDED(hr))
    {
        GetOptionFromGPDString<EProfileOption>(pDevmode,
                                               cbDevmode,
                                               g_pszColProfFeature,
                                               g_colProfTypeOption,
                                               g_cColProfOption,
                                               pDataSettings->cmProfile);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorProfileDMPTConv::MergePTDataSettingsWithPT

Routine Description:

    This method updates the color profile data structure from a PrintTicket description.

Arguments:

    pPrintTicket  - Pointer to the input PrintTicket.
    pDataSettings - Pointer to the color profile data structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorProfileDMPTConv::MergePTDataSettingsWithPT(
    _In_    IXMLDOMDocument2* pPrintTicket,
    _Inout_ PageSourceColorProfileData*  pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        try
        {
            PageSourceColorProfileData profData;
            CColorManageProfilePTHandler  profPTHndlr(pPrintTicket);

            if (SUCCEEDED(hr = profPTHndlr.GetData(&profData)))
            {
                pDataSettings->cmProfile = profData.cmProfile;
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // The property is not in the PT. This is not an error - reset hresult
                // and continue
                //
                hr = S_OK;
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

    CColorProfileDMPTConv::SetPTDataInDM

Routine Description:

    This method updates the color profile options in the devmode from the UI Settings.

Arguments:

    dataSettings - Reference to color profile data settings to be updated.
    pDevmode - pointer to devmode to be updated.
    cbDevmode - size in bytes of full devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorProfileDMPTConv::SetPTDataInDM(
    _In_    CONST PageSourceColorProfileData&           dataSettings,
    _Inout_ PDEVMODE                                    pDevmode,
    _In_    ULONG                                       cbDevmode,
    _Inout_ PVOID                                       pPrivateDevmode,
    _In_    ULONG                                       cbDrvPrivateSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGPDStringFromOption<EProfileOption>(pDevmode,
                                                    cbDevmode,
                                                    g_pszColProfFeature,
                                                    g_colProfTypeOption,
                                                    g_cColProfOption,
                                                    dataSettings.cmProfile);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorProfileDMPTConv::SetPTDataInPT

Routine Description:

    This method updates the watemark PrintTicket description from color profile data structure.

Arguments:

    dataSettings - Reference to color profile data structure to update from.
    pPrintTicket - Pointer to the PrintTicket to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorProfileDMPTConv::SetPTDataInPT(
    _In_    CONST PageSourceColorProfileData&   dataSettings,
    _Inout_ IXMLDOMDocument2*                   pPrintTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        try
        {
            CColorManageProfilePTHandler cmProfPTHndlr(pPrintTicket);
            PageSourceColorProfileData profData;
            profData.cmProfile = dataSettings.cmProfile;

            //
            // Ensure profile file names are set from the enumerated type
            //
            if (profData.cmProfile == CMYK)
            {
                profData.cmProfileName = g_pszCMYKProfileName;
            }
            else
            {
                profData.cmProfileName = g_pszRGBProfileName;
            }

            hr = cmProfPTHndlr.SetData(&profData);
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorProfileDMPTConv::CompletePrintCapabilities

Routine Description:

    Unidrv calls this routine with an input Device Capabilities Document
    that is partially populated with Device capabilities information
    filled in by Unidrv for features that it understands. The plug-in
    needs to read any private features in the input PrintTicket, delete
    them and add them back under Printschema namespace so that higher
    level applications can understand them and make use of them.

Arguments:

    pPrintTicket - pointer to input PrintTicket
    pCapabilities - pointer to Device Capabilities Document.

Return Value:

    HRESULT
    S_OK - Always

--*/
HRESULT STDMETHODCALLTYPE
CColorProfileDMPTConv::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2*,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            CColorManageProfilePCHandler cmProfPCHandler(pPrintCapabilities);
            cmProfPCHandler.SetCapabilities();
        }
        catch(CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
