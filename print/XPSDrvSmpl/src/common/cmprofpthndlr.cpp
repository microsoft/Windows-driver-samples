/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofpthndlr.cpp

Abstract:

   PageSourceColorProfile PrintTicket handling implementation.
   The PageSourceColorProfile PT handler is used to extract
   PageSourceColorProfile settings from a PrintTicket and populate
   the PageSourceColorProfile data structure with the retrieved
   settings. The class also defines a method for setting the feature in
   the PrintTicket given the data structure.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "ptquerybld.h"
#include "cmprofpthndlr.h"

using XDPrintSchema::SCHEMA_STRING;
using XDPrintSchema::PRINTTICKET_NAME;

using XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData;
using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::EProfileOptionMin;
using XDPrintSchema::PageSourceColorProfile::EProfileOptionMax;
using XDPrintSchema::PageSourceColorProfile::PROFILE_FEATURE;
using XDPrintSchema::PageSourceColorProfile::PROFILE_OPTIONS;
using XDPrintSchema::PageSourceColorProfile::PROFILE_URI_PROP;
using XDPrintSchema::PageSourceColorProfile::PROFILE_URI_REF;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

/*++

Routine Name:

    CColorManageProfilePTHandler::CColorManageProfilePTHandler

Routine Description:

    CColorManageProfilePTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CColorManageProfilePTHandler::CColorManageProfilePTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CColorManageProfilePTHandler::~CColorManageProfilePTHandler

Routine Description:

    CColorManageProfilePTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorManageProfilePTHandler::~CColorManageProfilePTHandler()
{
}

/*++

Routine Name:

    CColorManageProfilePTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with color profile data
    retrieved from the PrintTicket passed to the class constructor.

Arguments:

    pCmData - Pointer to the color profile data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CColorManageProfilePTHandler::GetData(
    _Inout_ PageSourceColorProfileData* pCmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pCmData, E_POINTER)))
    {
        CComBSTR option;
        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(PROFILE_FEATURE), &option)) &&
            SUCCEEDED(GetScoredPropertyValue(CComBSTR(PROFILE_FEATURE), CComBSTR(PROFILE_URI_PROP), &pCmData->cmProfileName))
            )
        {
            pCmData->cmProfile = CMYK;   // default to CMYK

            for (EProfileOption cmOption = EProfileOptionMin;
                     cmOption < EProfileOptionMax;
                     cmOption = static_cast<EProfileOption>(cmOption + 1))
            {
                if (option == CComBSTR(PROFILE_OPTIONS[cmOption]))
                {
                    pCmData->cmProfile = cmOption;
                }
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CColorManageProfilePTHandler::SetData

Routine Description:

    This routine sets the color profile data in the PrintTicket
    passed to the class constructor.

Arguments:

    pCmData - Pointer to the color profile data to be set in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManageProfilePTHandler::SetData(
    _In_ CONST PageSourceColorProfileData* pCmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pCmData, E_POINTER)))
    {
        CComPtr<IXMLDOMElement> pFeature(NULL);
        CComPtr<IXMLDOMElement> pOption(NULL);
        CComPtr<IXMLDOMElement> pUriProperty(NULL);
        CComPtr<IXMLDOMElement> pRef(NULL);
        CComPtr<IXMLDOMElement> pInit(NULL);


        CComBSTR bstrFeature(PROFILE_FEATURE);

        if (SUCCEEDED(hr = DeleteFeature(bstrFeature)) &&
            SUCCEEDED(hr = CreateFeatureOptionPair(bstrFeature, CComBSTR(PROFILE_OPTIONS[pCmData->cmProfile]), &pFeature, &pOption)) &&
            SUCCEEDED(hr = CreateParamRefInitPair(CComBSTR(PROFILE_URI_REF), CComBSTR(SCHEMA_STRING), pCmData->cmProfileName, &pRef, &pInit)) &&
            SUCCEEDED(hr = CreateScoredProperty(CComBSTR(PROFILE_URI_PROP), pRef, &pUriProperty)) &&
            SUCCEEDED(hr = pOption->appendChild(pUriProperty, NULL)) &&
            SUCCEEDED(hr = AppendToElement(CComBSTR(PRINTTICKET_NAME), pInit)))
        {
            SUCCEEDED(hr = AppendToElement(CComBSTR(PRINTTICKET_NAME), pFeature));
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

