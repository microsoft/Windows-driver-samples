/*++

Copyright (c) 2008 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofpchndlr.cpp

Abstract:

   PageSourceColorProfile PrintCapabilities handling implementation.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "ptquerybld.h"
#include "cmprofpchndlr.h"

using XDPrintSchema::PRINTCAPABILITIES_NAME;

using XDPrintSchema::PageSourceColorProfile::PROFILE_FEATURE;
using XDPrintSchema::PageSourceColorProfile::PROFILE_OPTIONS;
using XDPrintSchema::PageSourceColorProfile::PROFILE_URI_PROP;
using XDPrintSchema::PageSourceColorProfile::PROFILE_URI_REF;
using XDPrintSchema::PageSourceColorProfile::PROFILE_PARAM_DEF;
using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::EProfileOptionMax;
using XDPrintSchema::PageSourceColorProfile::EProfileOptionMin;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

/*++

Routine Name:

    CColorManageProfilePCHandler::CColorManageProfilePCHandler

Routine Description:

    CColorManageProfilePCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CColorManageProfilePCHandler::CColorManageProfilePCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CColorManageProfilePCHandler::~CColorManageProfilePCHandler

Routine Description:

    CColorManageProfilePCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorManageProfilePCHandler::~CColorManageProfilePCHandler()
{
}

/*++

Routine Name:

    CColorManageProfilePCHandler::SetCapabilities

Routine Description:

    This routine sets color profile capabilities in the PrintCapabilities passed to the
    class constructor.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManageProfilePCHandler::SetCapabilities(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        //
        // Retrieve the PrintTicket root
        //
        CComPtr<IXMLDOMNode> pPCRoot(NULL);
        CComPtr<IXMLDOMElement> pFeatureElement(NULL);

        CComBSTR bstrPTQuery(m_bstrFrameworkPrefix);
        bstrPTQuery += PRINTCAPABILITIES_NAME;

        if (SUCCEEDED(hr = GetNode(bstrPTQuery, &pPCRoot)) &&
            SUCCEEDED(hr = CreateFeatureSelection(CComBSTR(PROFILE_FEATURE), NULL, &pFeatureElement)))
        {
            for (   EProfileOption option = EProfileOptionMin;
                    option < EProfileOptionMax && SUCCEEDED(hr);
                    option = static_cast<EProfileOption>(option + 1))
            {
                CComPtr<IXMLDOMElement> pOptionElement(NULL);
                CComPtr<IXMLDOMElement> pScoredPropElement(NULL);
                CComPtr<IXMLDOMElement> pPropRefElement(NULL);

                if (SUCCEEDED(hr = CreateOption(CComBSTR(PROFILE_OPTIONS[option]), NULL, &pOptionElement)) &&
                    SUCCEEDED(hr = CreateScoredProperty(CComBSTR(PROFILE_URI_PROP), &pScoredPropElement)) &&
                    SUCCEEDED(hr = CreateParameterRef(CComBSTR(PROFILE_URI_REF), &pPropRefElement)) &&
                    SUCCEEDED(hr = pScoredPropElement->appendChild(pPropRefElement, NULL)) &&
                    SUCCEEDED(hr = pOptionElement->appendChild(pScoredPropElement, NULL)))
                {
                    hr = pFeatureElement->appendChild(pOptionElement, NULL);
                }
            }

            CComPtr<IXMLDOMElement> pParameterDef(NULL);

            if (SUCCEEDED(hr = pPCRoot->appendChild(pFeatureElement, NULL)) &&
                SUCCEEDED(hr = CreateStringParameterDef(    CComBSTR(PROFILE_PARAM_DEF.property_name),  // Paramater Name
                                                            TRUE,                                       // Is Print Schema Keyword?
                                                            CComBSTR(PROFILE_PARAM_DEF.display_name),   // Display Text
                                                            CComBSTR(PROFILE_PARAM_DEF.default_value),  // Default
                                                            PROFILE_PARAM_DEF.min_length,               // Min Length
                                                            PROFILE_PARAM_DEF.max_length,               // Max Length
                                                            CComBSTR(PROFILE_PARAM_DEF.unit_type),      // Unit Type
                                                            &pParameterDef)))                           // Parameter Def
            {
                hr = pPCRoot->appendChild(pParameterDef, NULL);
            }
        }


    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);

    return S_OK;
}
