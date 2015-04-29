/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscpthndlr.cpp

Abstract:

   PageScaling PrintTicket handler implementation. Derived from CPTHandler,
   this provides PageScaling specific Get and Set methods acting on the
   PrintTicket (as a DOM document) passed.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "pgscpthndlr.h"

using XDPrintSchema::PRINTTICKET_NAME;
using XDPrintSchema::NAME_ATTRIBUTE_NAME;

using XDPrintSchema::PageScaling::PageScalingData;
using XDPrintSchema::PageScaling::EScaleOption;
using XDPrintSchema::PageScaling::EScaleOptionMin;
using XDPrintSchema::PageScaling::Custom;
using XDPrintSchema::PageScaling::CustomSquare;
using XDPrintSchema::PageScaling::FitBleedToImageable;
using XDPrintSchema::PageScaling::FitContentToImageable;
using XDPrintSchema::PageScaling::FitMediaToImageable;
using XDPrintSchema::PageScaling::FitMediaToMedia;
using XDPrintSchema::PageScaling::None;
using XDPrintSchema::PageScaling::EScaleOptionMax;
using XDPrintSchema::PageScaling::ECustomScaleProps;
using XDPrintSchema::PageScaling::ECustomScalePropsMin;
using XDPrintSchema::PageScaling::CstOffsetWidth;
using XDPrintSchema::PageScaling::CstOffsetHeight;
using XDPrintSchema::PageScaling::CstScaleWidth;
using XDPrintSchema::PageScaling::CstScaleHeight;
using XDPrintSchema::PageScaling::ECustomScalePropsMax;
using XDPrintSchema::PageScaling::ECustomSquareScaleProps;
using XDPrintSchema::PageScaling::ECustomSquareScalePropsMin;
using XDPrintSchema::PageScaling::CstSqOffsetWidth;
using XDPrintSchema::PageScaling::CstSqOffsetHeight;
using XDPrintSchema::PageScaling::CstSqScale;
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

    CPageScalingPTHandler::CPageScalingPTHandler

Routine Description:

    CPageScalingPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CPageScalingPTHandler::CPageScalingPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CPageScalingPTHandler::~CPageScalingPTHandler

Routine Description:

    CPageScalingPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingPTHandler::~CPageScalingPTHandler()
{
}

/*++

Routine Name:

    CPageScalingPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with page scaling data
    retrieved from the PrintTicket passed to the class constructor.

Arguments:

    pPageScaleData - Pointer to the page scaling data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CPageScalingPTHandler::GetData(
    _Out_ PageScalingData* pPageScaleData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageScaleData, E_POINTER)))
    {
        CComBSTR bstrPgScOption;

        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(SCALE_FEATURE), &bstrPgScOption)))
        {
            if (bstrPgScOption == SCALE_OPTIONS[Custom])
            {
                pPageScaleData->pgscOption = Custom;

                if (SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SCALE_PROPS[CstOffsetWidth]),
                                                          &pPageScaleData->offWidth)) &&
                    SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SCALE_PROPS[CstOffsetHeight]),
                                                          &pPageScaleData->offHeight)) &&
                    SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SCALE_PROPS[CstScaleWidth]),
                                                          &pPageScaleData->scaleWidth)))
                {
                    hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                CComBSTR(CUST_SCALE_PROPS[CstScaleHeight]),
                                                &pPageScaleData->scaleHeight);
                }
            }
            else if (bstrPgScOption == SCALE_OPTIONS[CustomSquare])
            {
                pPageScaleData->pgscOption = CustomSquare;

                if (SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SQR_SCALE_PROPS[CstSqOffsetWidth]),
                                                          &pPageScaleData->offWidth)) &&
                    SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SQR_SCALE_PROPS[CstSqOffsetHeight]),
                                                          &pPageScaleData->offHeight)) &&
                    SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(SCALE_FEATURE),
                                                          CComBSTR(CUST_SQR_SCALE_PROPS[CstSqScale]),
                                                          &pPageScaleData->scaleWidth)))
                {
                    pPageScaleData->scaleHeight = pPageScaleData->scaleWidth;
                }
            }
            else if (bstrPgScOption == SCALE_OPTIONS[None])
            {
                //
                // No scaling - return as if no element is present
                //
                hr = E_ELEMENT_NOT_FOUND;
            }
            else
            {
                //
                // Identify the option
                //
                for (EScaleOption pgScOption = FitBleedToImageable;
                     pgScOption < EScaleOptionMax;
                     pgScOption = static_cast<EScaleOption>(pgScOption + 1))
                {
                    if (bstrPgScOption == SCALE_OPTIONS[pgScOption])
                    {
                        pPageScaleData->pgscOption = pgScOption;

                        //
                        // Get the offset alignment
                        //
                        CComBSTR bstrOffsetAlignment;

                        hr = GetSubFeatureOption(CComBSTR(SCALE_FEATURE),
                                                 CComBSTR(SCALE_OFFSET_FEATURE),
                                                 &bstrOffsetAlignment);

                        //
                        // Identify the Alignment
                        //
                        for (EScaleOffsetOption offOption = EScaleOffsetOptionMin;
                             offOption < EScaleOffsetOptionMax;
                             offOption = static_cast<EScaleOffsetOption>(offOption + 1))
                        {
                            if (bstrOffsetAlignment == SCALE_OFFSET_OPTIONS[offOption])
                            {
                                pPageScaleData->offsetOption = offOption;

                                break;
                            }
                        }

                        break;
                    }
                }
            }
        }
    }

    //
    // Validate the data
    //
    if (SUCCEEDED(hr))
    {
        if (pPageScaleData->pgscOption <  EScaleOptionMin ||
            pPageScaleData->pgscOption >= EScaleOptionMax)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPageScalingPTHandler::SetData

Routine Description:

    This routine sets the page scaling data in the PrintTicket passed to the
    class constructor.

Arguments:

    pPageScaleData - Pointer to the page scale data to be set in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingPTHandler::SetData(
    _In_ CONST PageScalingData* pPageScaleData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageScaleData, E_POINTER)))
    {
        if (pPageScaleData->pgscOption   <  EScaleOptionMin ||
            pPageScaleData->pgscOption   >= EScaleOptionMax ||
            pPageScaleData->offsetOption <  EScaleOffsetOptionMin ||
            pPageScaleData->offsetOption >= EScaleOffsetOptionMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Remove any existing page scaling feature node
        //
        CComPtr<IXMLDOMElement> pFeature(NULL);
        CComPtr<IXMLDOMElement> pOption(NULL);

        CComBSTR bstrFeature(SCALE_FEATURE);
        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);

        if (SUCCEEDED(hr = DeleteFeature(bstrFeature)) &&
            pPageScaleData->pgscOption != None &&
            SUCCEEDED(hr = CreateFeatureOptionPair(bstrFeature, &pFeature, &pOption)))
        {
            //
            // Retrieve the PrintTicket root
            //
            CComPtr<IXMLDOMNode> pPTRoot(NULL);

            CComBSTR bstrPTQuery(m_bstrFrameworkPrefix);
            bstrPTQuery += PRINTTICKET_NAME;

            if (SUCCEEDED(hr = GetNode(bstrPTQuery, &pPTRoot)))
            {
                //
                // Append the feature node to the PrintTicket root element
                //
                hr = pPTRoot->appendChild(pFeature, NULL);
            }

            //
            // Set the option value
            //
            bstrAttribName += SCALE_OPTIONS[pPageScaleData->pgscOption];

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = CreateXMLAttribute(pOption, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName )))
            {
                switch (pPageScaleData->pgscOption)
                {
                    case Custom:
                    {
                        //
                        // Create the width offset, height offset, width scale and height scale
                        // parameter ref/init pairs. Create the relevant scored properties passing the
                        // paramater refs. Append the scored properties to the option element and the
                        // paramter init values to the PrintTicket element
                        //
                        CONST INT* pValue = &pPageScaleData->offWidth;
                        for (ECustomScaleProps props = ECustomScalePropsMin;
                             props < ECustomScalePropsMax && SUCCEEDED(hr);
                             props = static_cast<ECustomScaleProps>(props + 1), pValue++)
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);
                            CComPtr<IXMLDOMElement> pParamInit(NULL);
                            CComPtr<IXMLDOMElement> pScoredProp(NULL);

                            CComBSTR bstrParamRefName(SCALE_FEATURE);
                            bstrParamRefName += CUST_SCALE_PROPS[props];

                            if (SUCCEEDED(hr = CreateParamRefInitPair(bstrParamRefName,
                                                                      *pValue,
                                                                      &pParamRef,&pParamInit)) &&
                                SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CUST_SCALE_PROPS[props]),
                                                                    pParamRef,
                                                                    &pScoredProp)) &&
                                SUCCEEDED(hr = pOption->appendChild(pScoredProp, NULL)))
                            {
                                hr = pPTRoot->appendChild(pParamInit, NULL);
                            }
                        }
                    }
                    break;

                    case CustomSquare:
                    {
                        //
                        // Create the width offset, height offset and scale parameter ref/init pairs.
                        // Create the relevant scored properties passing the paramater refs. Append
                        // the scored properties to the option element and the paramter init values
                        // to the PrintTicket element.
                        //
                        CONST INT* pValue = &pPageScaleData->offWidth;
                        for (ECustomSquareScaleProps props = ECustomSquareScalePropsMin;
                             props < ECustomSquareScalePropsMax && SUCCEEDED(hr);
                             props = static_cast<ECustomSquareScaleProps>(props + 1), pValue++)
                        {
                            CComPtr<IXMLDOMElement> pParamRef(NULL);
                            CComPtr<IXMLDOMElement> pParamInit(NULL);
                            CComPtr<IXMLDOMElement> pScoredProp(NULL);

                            CComBSTR bstrParamRefName(SCALE_FEATURE);
                            bstrParamRefName += CUST_SQR_SCALE_PROPS[props];

                            if (SUCCEEDED(hr = CreateParamRefInitPair(bstrParamRefName,
                                                                      *pValue,
                                                                      &pParamRef,
                                                                      &pParamInit)) &&
                                SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CUST_SQR_SCALE_PROPS[props]),
                                                                    pParamRef,
                                                                    &pScoredProp)) &&
                                SUCCEEDED(hr = pOption->appendChild(pScoredProp, NULL)))
                            {
                                hr = pPTRoot->appendChild(pParamInit, NULL);
                            }
                        }
                    }
                    break;

                    case FitBleedToImageable:
                    case FitContentToImageable:
                    case FitMediaToImageable:
                    case FitMediaToMedia:
                    {
                        //
                        // Create the ScaleOffsetAlignement feature and append to the option node
                        //
                        CComPtr<IXMLDOMElement> pSubFeature(NULL);
                        CComPtr<IXMLDOMElement> pSubOption(NULL);

                        CComBSTR bstrOffsetFeature(SCALE_OFFSET_FEATURE);
                        CComBSTR bstrOffsetOption(SCALE_OFFSET_OPTIONS[pPageScaleData->offsetOption]);

                        if (SUCCEEDED(hr = CreateFeatureOptionPair(bstrOffsetFeature,
                                                                   bstrOffsetOption,
                                                                   &pSubFeature,
                                                                   &pSubOption)))
                        {
                            hr = pFeature->appendChild(pSubFeature, NULL);
                        }
                    }
                    break;

                    case None:
                    default:
                    {
                    }
                    break;
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScalingPTHandler::Delete

Routine Description:

    This routine deletes the page scaling feature from the PrintTicket passed to the
    class constructor

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingPTHandler::Delete(
    VOID
    )
{
    //
    // Remove any existing page scaling feature node
    //
    HRESULT hr = DeleteFeature(CComBSTR(SCALE_FEATURE));

    ERR_ON_HR(hr);
    return hr;
}

