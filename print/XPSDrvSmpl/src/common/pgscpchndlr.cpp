/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscpchndlr.cpp

Abstract:

   Pagescale PrintCapabilities handling implementation. The pagescale PC handler
   is used to set page scaling settings in a PrintCapabilities.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "pgscpchndlr.h"
#include "privatedefs.h"

using XDPrintSchema::PRINTCAPABILITIES_NAME;

using XDPrintSchema::PageScaling::EScaleOption;
using XDPrintSchema::PageScaling::FitBleedToImageable;
using XDPrintSchema::PageScaling::FitMediaToMedia;
using XDPrintSchema::PageScaling::Custom;
using XDPrintSchema::PageScaling::CustomSquare;
using XDPrintSchema::PageScaling::ECustomScaleProps;
using XDPrintSchema::PageScaling::ECustomScalePropsMin;
using XDPrintSchema::PageScaling::ECustomScalePropsMax;
using XDPrintSchema::PageScaling::ECustomSquareScaleProps;
using XDPrintSchema::PageScaling::ECustomSquareScalePropsMin;
using XDPrintSchema::PageScaling::ECustomSquareScalePropsMax;
using XDPrintSchema::PageScaling::SCALE_FEATURE;
using XDPrintSchema::PageScaling::SCALE_OPTIONS;
using XDPrintSchema::PageScaling::CUST_SCALE_PROPS;
using XDPrintSchema::PageScaling::CUST_SQR_SCALE_PROPS;

using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOption;
using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOptionMin;
using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOptionMax;
using XDPrintSchema::PageScaling::OffsetAlignment::SCALE_OFFSET_FEATURE;
using XDPrintSchema::PageScaling::OffsetAlignment::SCALE_OFFSET_OPTIONS;

/*++

Routine Name:

    CPageScalingPCHandler::CPageScalingPCHandler

Routine Description:

    CPageScalingPCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CPageScalingPCHandler::CPageScalingPCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CPageScalingPCHandler::~CPageScalingPCHandler

Routine Description:

    CPageScalingPCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingPCHandler::~CPageScalingPCHandler()
{
}

/*++

Routine Name:

    CPageScalingPCHandler::SetCapabilities

Routine Description:

    This routine sets page scaling capabilities in the PrintCapabilities passed to the
    class constructor.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingPCHandler::SetCapabilities(
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
            CComPtr<IXMLDOMElement> pFeatureElement(NULL);

            if (SUCCEEDED(hr = CreateFeatureSelection(CComBSTR(SCALE_FEATURE), NULL, &pFeatureElement)))
            {
                CComPtr<IXMLDOMElement> pCustomOption(NULL);

                //
                // Create the Custom Options
                //
                if (SUCCEEDED(hr = CreateOption(CComBSTR(SCALE_OPTIONS[Custom]), NULL, &pCustomOption)))
                {
                    PTDOMElementVector propertList;

                    for (ECustomScaleProps customProps = ECustomScalePropsMin;
                         customProps < ECustomScalePropsMax && SUCCEEDED(hr);
                         customProps = static_cast<ECustomScaleProps>(customProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        //
                        // Create the scored property list
                        //
                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CUST_SCALE_PROPS[customProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(SCALE_FEATURE);
                            bstrPRefName += CUST_SCALE_PROPS[customProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Add the properties to the custom option
                    //
                    PTDOMElementVector::iterator iterPropertList = propertList.begin();

                    for (;iterPropertList != propertList.end() && SUCCEEDED(hr); iterPropertList++)
                    {
                        hr = pCustomOption->appendChild(*iterPropertList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pCustomOption, NULL);
                    }
                }

                CComPtr<IXMLDOMElement> pCustomSquareOption(NULL);

                //
                // Create the Custom Square
                //
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = CreateOption(CComBSTR(SCALE_OPTIONS[CustomSquare]), NULL, &pCustomSquareOption)))
                {
                    PTDOMElementVector propertList;

                    for (ECustomSquareScaleProps customSquareProps = ECustomSquareScalePropsMin;
                         customSquareProps < ECustomSquareScalePropsMax && SUCCEEDED(hr);
                         customSquareProps = static_cast<ECustomSquareScaleProps>(customSquareProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        //
                        // Create the scored property list
                        //
                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CUST_SQR_SCALE_PROPS[customSquareProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(SCALE_FEATURE);
                            bstrPRefName += CUST_SQR_SCALE_PROPS[customSquareProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Add the properties to the custom square option
                    //
                    PTDOMElementVector::iterator iterPropertList = propertList.begin();

                    for (;iterPropertList != propertList.end() && SUCCEEDED(hr); iterPropertList++)
                    {
                        hr = pCustomSquareOption->appendChild(*iterPropertList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pCustomSquareOption, NULL);
                    }
                }

                //
                // Create the remaining scaling options: Fit To ...
                //
                for (EScaleOption fitToOptions = FitBleedToImageable;
                     fitToOptions <= FitMediaToMedia && SUCCEEDED(hr);
                     fitToOptions = static_cast<EScaleOption>(fitToOptions + 1))
                {
                    CComPtr<IXMLDOMElement> pFitToOption(NULL);

                    //
                    // Create the fit to page option
                    //
                    if (SUCCEEDED(hr = CreateOption(CComBSTR(SCALE_OPTIONS[fitToOptions]), NULL, &pFitToOption)))
                    {
                        hr = pFeatureElement->appendChild(pFitToOption, NULL);
                    }
                }

                //
                // Create the offset options
                //
                if (SUCCEEDED(hr))
                {
                    CComPtr<IXMLDOMElement> pOffsetFeature(NULL);
                    if (SUCCEEDED(hr = CreateFeature(CComBSTR(SCALE_OFFSET_FEATURE),
                                                     NULL,
                                                     &pOffsetFeature)))
                    {
                        PTDOMElementVector optionList;

                        for (EScaleOffsetOption offsetOption = EScaleOffsetOptionMin;
                             offsetOption < EScaleOffsetOptionMax && SUCCEEDED(hr);
                             offsetOption = static_cast<EScaleOffsetOption>(offsetOption + 1))
                        {
                            CComPtr<IXMLDOMElement> pOptionProperty(NULL);

                            if (SUCCEEDED(hr = CreateOption(CComBSTR(SCALE_OFFSET_OPTIONS[offsetOption]), NULL, &pOptionProperty)))
                            {
                                optionList.push_back(pOptionProperty);
                            }
                        }

                        //
                        // Add the options to the offset feature
                        //
                        PTDOMElementVector::iterator iterOptionList = optionList.begin();

                        for (;iterOptionList != optionList.end() && SUCCEEDED(hr); iterOptionList++)
                        {
                            hr = pOffsetFeature->appendChild(*iterOptionList, NULL);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pOffsetFeature, NULL);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = pPTRoot->appendChild(pFeatureElement, NULL);
                }
            }

            for (UINT cIndex = 0; cIndex < numof(pgscParamDefIntegers); cIndex++)
            {
                CComPtr<IXMLDOMElement> pParameterDef(NULL);

                if (SUCCEEDED(hr = CreateIntParameterDef(CComBSTR(pgscParamDefIntegers[cIndex].property_name),  // Paramater Name
                                                         pgscParamDefIntegers[cIndex].is_public,                 // Is Print Schema Keyword?
                                                         CComBSTR(pgscParamDefIntegers[cIndex].display_name),   // Display Text
                                                         pgscParamDefIntegers[cIndex].default_value,            // Default
                                                         pgscParamDefIntegers[cIndex].min_length,               // Min Length
                                                         pgscParamDefIntegers[cIndex].max_length,               // Max Length
                                                         pgscParamDefIntegers[cIndex].multiple,                 // Multiple
                                                         CComBSTR(pgscParamDefIntegers[cIndex].unit_type),      // Unit Type
                                                         &pParameterDef)))                                      // Parameter Def
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


