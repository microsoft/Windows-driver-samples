/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nuppchndlr.cpp

Abstract:

   NUp PrintCapabilities handling implementation. The NUp PC handler
   is used to set NUp settings in a PrintCapabilities.

--*/

#include "precomp.h"
#include "debug.h"
#include "xdexcept.h"
#include "nupchndlr.h"
#include "ptquerybld.h"

using XDPrintSchema::NUp::NUpData;
using XDPrintSchema::NUp::ENUpFeature;
using XDPrintSchema::NUp::ENUpFeatureMin;
using XDPrintSchema::NUp::DocumentNUp;
using XDPrintSchema::NUp::JobNUpAllDocumentsContiguously;
using XDPrintSchema::NUp::ENUpFeatureMax;
using XDPrintSchema::NUp::NUP_FEATURES;
using XDPrintSchema::NUp::NUP_PROP;

using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMin;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMax;
using XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_FEATURE;
using XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_OPTIONS;

/*++

Routine Name:

    CNUpPCHandler::CNUpPCHandler

Routine Description:

    CNUpPCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CNUpPCHandler::CNUpPCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CNUpPCHandler::~CNUpPCHandler

Routine Description:

    CNUpPCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CNUpPCHandler::~CNUpPCHandler()
{
}

/*++

Routine Name:

    CNUpPCHandler::SetCapabilities

Routine Description:

    This routine sets NUp capabilities in the PrintCapabilities passed to the
    class constructor.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPCHandler::SetCapabilities(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        for (ENUpFeature nupFeatures = ENUpFeatureMin;
             nupFeatures < ENUpFeatureMax && SUCCEEDED(hr);
             nupFeatures = static_cast<ENUpFeature>(nupFeatures + 1))
        {
            //
            // Find the existing NUp feature node
            //
            CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
            CComPtr<IXMLDOMNode> pFeatureNode(NULL);
            CComBSTR bstrFeatureQuery;

            if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, CComBSTR(NUP_FEATURES[nupFeatures]))) &&
                SUCCEEDED(hr = propertyQuery.GetQuery(&bstrFeatureQuery)) &&
                SUCCEEDED(hr = GetNode(bstrFeatureQuery, &pFeatureNode)) &&
                hr != S_FALSE)
            {
                CComPtr<IXMLDOMElement> pPresentationFeature(NULL);

                //
                // Create the presentation feature options
                //
                if (SUCCEEDED(hr = CreateFeature(CComBSTR(NUP_DIRECTION_FEATURE),
                                                 NULL,
                                                 &pPresentationFeature)))
                {
                    PTDOMElementVector optionList;

                    for (ENUpDirectionOption presentationOption = ENUpDirectionOptionMin;
                         presentationOption < ENUpDirectionOptionMax && SUCCEEDED(hr);
                         presentationOption = static_cast<ENUpDirectionOption>(presentationOption + 1))
                    {
                        //
                        // Create the presentation options property element
                        //
                        CComPtr<IXMLDOMElement> pOptionProperty(NULL);

                        if (SUCCEEDED(hr = CreateOption(CComBSTR(NUP_DIRECTION_OPTIONS[presentationOption]), NULL, &pOptionProperty)))
                        {
                            optionList.push_back(pOptionProperty);
                        }
                    }

                    //
                    // Add the options into the presentation feature
                    //
                    PTDOMElementVector::iterator iterOptionList = optionList.begin();

                    for (;iterOptionList != optionList.end() && SUCCEEDED(hr); iterOptionList++)
                    {
                        hr = pPresentationFeature->appendChild(*iterOptionList, NULL);
                    }

                    //
                    // Add the presentation feature into the existing NUp feature
                    //
                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureNode->appendChild(pPresentationFeature, NULL);
                    }
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

