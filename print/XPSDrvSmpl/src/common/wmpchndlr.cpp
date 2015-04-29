/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmpchndlr.cpp

Abstract:

   Page watermark PrintCapabilities handling implementation. The watermark PC handler
   is used to set Page Watermark settings in a PrintCapabilities.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdexcept.h"
#include "wmpchndlr.h"
#include "privatedefs.h"

using XDPrintSchema::PRINTCAPABILITIES_NAME;

using XDPrintSchema::PageWatermark::TextWatermark;
using XDPrintSchema::PageWatermark::BitmapWatermark;
using XDPrintSchema::PageWatermark::VectorWatermark;
using XDPrintSchema::PageWatermark::ECommonWatermarkProps;
using XDPrintSchema::PageWatermark::ECommonWatermarkPropsMin;
using XDPrintSchema::PageWatermark::ECommonWatermarkPropsMax;
using XDPrintSchema::PageWatermark::ETextWatermarkProps;
using XDPrintSchema::PageWatermark::ETextWatermarkPropsMin;
using XDPrintSchema::PageWatermark::ETextWatermarkPropsMax;
using XDPrintSchema::PageWatermark::EVectBmpWatermarkProps;
using XDPrintSchema::PageWatermark::EVectBmpWatermarkPropsMin;
using XDPrintSchema::PageWatermark::EVectBmpWatermarkPropsMax;
using XDPrintSchema::PageWatermark::WATERMARK_FEATURE;
using XDPrintSchema::PageWatermark::WATERMARK_OPTIONS;
using XDPrintSchema::PageWatermark::CMN_WATERMARK_PROPS;
using XDPrintSchema::PageWatermark::TXT_WATERMARK_PROPS;
using XDPrintSchema::PageWatermark::VECTBMP_WATERMARK_PROPS;

using XDPrintSchema::PageWatermark::Layering::ELayeringOption;
using XDPrintSchema::PageWatermark::Layering::ELayeringOptionMin;
using XDPrintSchema::PageWatermark::Layering::ELayeringOptionMax;
using XDPrintSchema::PageWatermark::Layering::LAYERING_FEATURE;
using XDPrintSchema::PageWatermark::Layering::LAYERING_OPTIONS;

/*++

Routine Name:

    CWMPCHandler::CWMPCHandler

Routine Description:

    CWMPCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CWMPCHandler::CWMPCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CWMPCHandler::~CWMPCHandler

Routine Description:

    CWMPCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CWMPCHandler::~CWMPCHandler()
{
}

/*++

Routine Name:

    CWMPCHandler::SetCapabilities

Routine Description:

    This routine sets watermark capabilities in the PrintCapabilities passed to the
    class constructor.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPCHandler::SetCapabilities(
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

            if (SUCCEEDED(hr = CreateFeatureSelection(CComBSTR(WATERMARK_FEATURE), NULL, &pFeatureElement)))
            {
                CComPtr<IXMLDOMElement> pTextOption(NULL);

                //
                // Create the Text Watermark Options
                //
                if (SUCCEEDED(hr = CreateOption(CComBSTR(WATERMARK_OPTIONS[TextWatermark]), NULL, &pTextOption)))
                {
                    PTDOMElementVector propertList;

                    //
                    // Create the common scored property list
                    //
                    for (ECommonWatermarkProps cmnProps = ECommonWatermarkPropsMin;
                         cmnProps < ECommonWatermarkPropsMax && SUCCEEDED(hr);
                         cmnProps = static_cast<ECommonWatermarkProps>(cmnProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CMN_WATERMARK_PROPS[cmnProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);

                            if (wcscmp(CMN_WATERMARK_PROPS[cmnProps], L"Angle") == 0)
                            {
                                hr = bstrPRefName.Append(L"Text");
                            }

                            bstrPRefName += CMN_WATERMARK_PROPS[cmnProps];

                            if (SUCCEEDED(hr) &&
                                SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Create the text specific scored property list
                    //
                    for (ETextWatermarkProps txtProps = ETextWatermarkPropsMin;
                         txtProps < ETextWatermarkPropsMax && SUCCEEDED(hr);
                         txtProps = static_cast<ETextWatermarkProps>(txtProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(TXT_WATERMARK_PROPS[txtProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);
                            bstrPRefName += TXT_WATERMARK_PROPS[txtProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Add the properties to the text option
                    //
                    PTDOMElementVector::iterator iterPropertList = propertList.begin();

                    for (;iterPropertList != propertList.end() && SUCCEEDED(hr); iterPropertList++)
                    {
                        hr = pTextOption->appendChild(*iterPropertList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pTextOption, NULL);
                    }
                }

                CComPtr<IXMLDOMElement> pVectorOption(NULL);

                //
                // Create the Vector Watermark Options
                //
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = CreateOption(CComBSTR(WATERMARK_OPTIONS[VectorWatermark]), NULL, &pVectorOption)))
                {
                    PTDOMElementVector propertList;

                    //
                    // Create the common scored property list
                    //
                    for (ECommonWatermarkProps cmnProps = ECommonWatermarkPropsMin;
                         cmnProps < ECommonWatermarkPropsMax && SUCCEEDED(hr);
                         cmnProps = static_cast<ECommonWatermarkProps>(cmnProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CMN_WATERMARK_PROPS[cmnProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);

                            if (wcscmp(CMN_WATERMARK_PROPS[cmnProps], L"Angle") == 0)
                            {
                                bstrPRefName += WATERMARK_OPTIONS[TextWatermark];
                            }

                            bstrPRefName += CMN_WATERMARK_PROPS[cmnProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Create the vector specific property list
                    //
                    for (EVectBmpWatermarkProps vectProps = EVectBmpWatermarkPropsMin;
                         vectProps < EVectBmpWatermarkPropsMax && SUCCEEDED(hr);
                         vectProps = static_cast<EVectBmpWatermarkProps>(vectProps + 1))
                    {
                        //
                        // Create the scored property element
                        //
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(VECTBMP_WATERMARK_PROPS[vectProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);
                            bstrPRefName += VECTBMP_WATERMARK_PROPS[vectProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Add the properties to the vector option
                    //
                    PTDOMElementVector::iterator iterPropertList = propertList.begin();

                    for (;iterPropertList != propertList.end() && SUCCEEDED(hr); iterPropertList++)
                    {
                        hr = pVectorOption->appendChild(*iterPropertList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pVectorOption, NULL);
                    }
                }

                CComPtr<IXMLDOMElement> pBitmapOption(NULL);

                //
                // Create the Bitmap Watermark Options
                //
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = CreateOption(CComBSTR(WATERMARK_OPTIONS[BitmapWatermark]), NULL, &pBitmapOption)))
                {
                    PTDOMElementVector propertList;

                    //
                    // Create the common scored property list
                    //
                    for (ECommonWatermarkProps cmnProps = ECommonWatermarkPropsMin;
                         cmnProps < ECommonWatermarkPropsMax && SUCCEEDED(hr);
                         cmnProps = static_cast<ECommonWatermarkProps>(cmnProps + 1))
                    {
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CMN_WATERMARK_PROPS[cmnProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);

                            if (wcscmp(CMN_WATERMARK_PROPS[cmnProps], L"Angle") == 0)
                            {
                                hr = bstrPRefName.Append(L"Text");
                            }

                            bstrPRefName += CMN_WATERMARK_PROPS[cmnProps];

                            if (SUCCEEDED(hr) &&
                                SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Create the bitmap specific property list
                    //
                    for (EVectBmpWatermarkProps bmpProps = EVectBmpWatermarkPropsMin;
                         bmpProps < EVectBmpWatermarkPropsMax && SUCCEEDED(hr);
                         bmpProps = static_cast<EVectBmpWatermarkProps>(bmpProps + 1))
                    {
                        //
                        // Create the scored property element
                        //
                        CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                        if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(VECTBMP_WATERMARK_PROPS[bmpProps]), &pScoredProperty)))
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);

                            //
                            // Construct the parameter reference elements
                            //
                            CComBSTR bstrPRefName(WATERMARK_FEATURE);
                            bstrPRefName += VECTBMP_WATERMARK_PROPS[bmpProps];

                            if (SUCCEEDED(hr = CreateParameterRef(bstrPRefName, &pParamRef)))
                            {
                                hr = pScoredProperty->appendChild(pParamRef, NULL);
                            }

                            propertList.push_back(pScoredProperty);
                        }
                    }

                    //
                    // Add the properties to the bitmap option
                    //
                    PTDOMElementVector::iterator iterPropertList = propertList.begin();

                    for (;iterPropertList != propertList.end() && SUCCEEDED(hr); iterPropertList++)
                    {
                        hr = pBitmapOption->appendChild(*iterPropertList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pBitmapOption, NULL);
                    }
                }

                CComPtr<IXMLDOMElement> pLayeringFeature(NULL);

                //
                // Create the layering feature options
                //
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = CreateFeature(CComBSTR(LAYERING_FEATURE), NULL, &pLayeringFeature)))
                {
                    PTDOMElementVector optionList;

                    //
                    // Create the layering option list
                    //
                    for (ELayeringOption wmLayering = ELayeringOptionMin;
                         wmLayering < ELayeringOptionMax && SUCCEEDED(hr);
                         wmLayering = static_cast<ELayeringOption>(wmLayering + 1))
                    {
                        CComPtr<IXMLDOMElement> pOptionProperty(NULL);

                        if (SUCCEEDED(hr = CreateOption(CComBSTR(LAYERING_OPTIONS[wmLayering]), NULL, &pOptionProperty)))
                        {
                            optionList.push_back(pOptionProperty);
                        }
                    }

                    //
                    // Add the options to the layering feature
                    //
                    PTDOMElementVector::iterator iterOptionList = optionList.begin();

                    for (;iterOptionList != optionList.end() && SUCCEEDED(hr); iterOptionList++)
                    {
                        hr = pLayeringFeature->appendChild(*iterOptionList, NULL);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pFeatureElement->appendChild(pLayeringFeature, NULL);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = pPTRoot->appendChild(pFeatureElement, NULL);
                }
            }

            //
            // Create the integer parameter defs
            //
            for (UINT cIndex = 0; SUCCEEDED(hr) && cIndex < numof(wmParamDefIntegers); cIndex++)
            {
                CComPtr<IXMLDOMElement> pParameterDef(NULL);

                //
                // PageWatermarkTextColor has no associated multiple value
                //
                INT multiple;
                if (wcscmp(CComBSTR(wmParamDefIntegers[cIndex].property_name), L"PageWatermarkTextColor") == 0)
                {
                    //
                    // convert integer representation into a string parameter in the 
                    // PrintCapabilities. This is a special case because it's treated 
                    // as an int in the DEVMODE & a string in the PrintTicket
                    //
                    CComBSTR bstrDefault(L"#AARRGGBB");
                    hr = StringCchPrintf(bstrDefault, SysStringLen(bstrDefault)+1, TEXT("#%.8X"), wmParamDefIntegers[cIndex].default_value);

                    if (SUCCEEDED(hr))
                    {
                        hr = CreateStringParameterDef(CComBSTR(wmParamDefIntegers[cIndex].property_name),  // Paramater Name
                                                      TRUE,                                                // Is Print Schema keyword?
                                                      CComBSTR(wmParamDefIntegers[cIndex].display_name),   // Display Text
                                                      bstrDefault,                                         // Default
                                                      9,                                                   // Min Length:
                                                                                                           //   table contains min value, which is different
                                                      9,                                                   // Max Length:
                                                                                                           //   table contains min value, which is different
                                                      CComBSTR(wmParamDefIntegers[cIndex].unit_type),      // Unit Type
                                                      &pParameterDef);                                     // Parameter Def
                    }
                }
                else
                {
                    multiple = wmParamDefIntegers[cIndex].multiple;

                    hr = CreateIntParameterDef(CComBSTR(wmParamDefIntegers[cIndex].property_name),  // Paramater Name
                                               wmParamDefIntegers[cIndex].is_public,                // Is Print Schema keyword?
                                               CComBSTR(wmParamDefIntegers[cIndex].display_name),   // Display Text
                                               wmParamDefIntegers[cIndex].default_value,            // Default
                                               wmParamDefIntegers[cIndex].min_length,               // Min value
                                               wmParamDefIntegers[cIndex].max_length,               // Max value
                                               multiple,                                            // Multiple
                                               CComBSTR(wmParamDefIntegers[cIndex].unit_type),      // Unit Type
                                               &pParameterDef);                                     // Parameter Def
                }

                if (SUCCEEDED(hr))
                {
                    hr = pPTRoot->appendChild(pParameterDef, NULL);
                }
            }

            //
            // Create the string parameter defs
            //
            for (UINT cIndex = 0; SUCCEEDED(hr) && cIndex < numof(wmParamDefStrings); cIndex++)
            {
                CComPtr<IXMLDOMElement> pParameterDef(NULL);

                if (SUCCEEDED(hr = CreateStringParameterDef(CComBSTR(wmParamDefStrings[cIndex].property_name),  // Paramater Name
                                                            TRUE,                                               // Is Print Schema keyword?
                                                            CComBSTR(wmParamDefStrings[cIndex].display_name),   // Display Text
                                                            CComBSTR(wmParamDefStrings[cIndex].default_value),  // Default
                                                            wmParamDefStrings[cIndex].min_length,               // Min Length
                                                            wmParamDefStrings[cIndex].max_length,               // Max Length
                                                            CComBSTR(wmParamDefStrings[cIndex].unit_type),      // Unit Type
                                                            &pParameterDef)))                                   // Parameter Def
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

