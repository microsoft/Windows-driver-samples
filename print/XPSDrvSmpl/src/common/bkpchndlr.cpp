/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkpchndlr.cpp

Abstract:

   Booklet PrintCapabilities handling implementation. The booklet PC handler
   is used to set booklet settings in a PrintCapabilities document.

--*/

#include "precomp.h"
#include "debug.h"
#include "xdexcept.h"
#include "bkpchndlr.h"
#include "globals.h"
#include "privatedefs.h"

using XDPrintSchema::PRINTCAPABILITIES_NAME;

using XDPrintSchema::Binding::EBinding;
using XDPrintSchema::Binding::EBindingMin;
using XDPrintSchema::Binding::EBindingMax;
using XDPrintSchema::Binding::EBindingOption;
using XDPrintSchema::Binding::EBindingOptionMin;
using XDPrintSchema::Binding::EBindingOptionMax;
using XDPrintSchema::Binding::BIND_FEATURES;
using XDPrintSchema::Binding::BIND_OPTIONS;

/*++

Routine Name:

    CBookPCHandler::CBookPCHandler

Routine Description:

    CBookPCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CBookPCHandler::CBookPCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CBookPCHandler::~CBookPCHandler

Routine Description:

    CBookPCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CBookPCHandler::~CBookPCHandler()
{
}

/*++

Routine Name:

    CBookPCHandler::SetCapabilities

Routine Description:

    This routine sets booklet capabilities in the PrintCapabilities passed to the
    class constructor.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookPCHandler::SetCapabilities(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        //
        // Retrieve the PrintTicket root
        //
        CComPtr<IXMLDOMNode> pPTRoot(NULL);

        CComBSTR bstrPTQuery(m_bstrFrameworkPrefix);
        bstrPTQuery += PRINTCAPABILITIES_NAME;

        if (SUCCEEDED(hr = GetNode(bstrPTQuery, &pPTRoot)))
        {
            for (EBinding bindFeatures = EBindingMin;
                     bindFeatures < EBindingMax && SUCCEEDED(hr);
                     bindFeatures = static_cast<EBinding>(bindFeatures + 1))
            {
                CComPtr<IXMLDOMElement> pFeatureElement(NULL);

                if (SUCCEEDED(hr = CreateFeatureSelection(CComBSTR(BIND_FEATURES[bindFeatures]), NULL, &pFeatureElement)))
                {
                    PTDOMElementVector optionList;

                    for (EBindingOption bindOption = EBindingOptionMin;
                         bindOption < EBindingOptionMax && SUCCEEDED(hr);
                         bindOption = static_cast<EBindingOption>(bindOption + 1))
                    {
                        //
                        // Create the booklet options
                        //
                        CComPtr<IXMLDOMElement> pOptionProperty(NULL);

                        if (SUCCEEDED(hr = CreateOption(CComBSTR(BIND_OPTIONS[bindOption]), NULL, &pOptionProperty)))
                        {
                            optionList.push_back(pOptionProperty);
                        }
                    }

                    //
                    // Add the options into the booklet feature
                    //
                    PTDOMElementVector::iterator iterOptionList = optionList.begin();

                    for (;iterOptionList != optionList.end() && SUCCEEDED(hr); iterOptionList++)
                    {
                        hr = pFeatureElement->appendChild(*iterOptionList, NULL);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = pPTRoot->appendChild(pFeatureElement, NULL);
                }
            }

            for (UINT cIndex = 0; cIndex < numof(bkParamDefIntegers); cIndex++)
            {
                CComPtr<IXMLDOMElement> pParameterDef(NULL);

                if (SUCCEEDED(hr = CreateIntParameterDef(CComBSTR(bkParamDefIntegers[cIndex].property_name),  // Paramater Name
                                                         bkParamDefIntegers[cIndex].is_public,                // Is Print Schema keyword?
                                                         CComBSTR(bkParamDefIntegers[cIndex].display_name),   // Display Text
                                                         bkParamDefIntegers[cIndex].default_value,            // Default
                                                         bkParamDefIntegers[cIndex].min_length,               // Min Length
                                                         bkParamDefIntegers[cIndex].max_length,               // Max Length
                                                         bkParamDefIntegers[cIndex].multiple,                 // Multiple
                                                         CComBSTR(bkParamDefIntegers[cIndex].unit_type),      // Unit Type
                                                         &pParameterDef)))                                    // Parameter Def
                {
                    hr = pPTRoot->appendChild(pParameterDef, NULL);
                }
            }
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

