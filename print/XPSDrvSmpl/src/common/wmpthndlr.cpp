/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmpthndlr.cpp

Abstract:

   PageWatermark PrintTicket handler implementation. Derived from CPTHandler,
   this provides PageWatermark specific Get and Set methods acting on the
   PrintTicket passed (as a DOM document).

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "wmpthndlr.h"

using XDPrintSchema::PRINTTICKET_NAME;
using XDPrintSchema::NAME_ATTRIBUTE_NAME;
using XDPrintSchema::SCHEMA_DECIMAL;
using XDPrintSchema::SCHEMA_INTEGER;
using XDPrintSchema::SCHEMA_STRING;

using XDPrintSchema::PageWatermark::WatermarkData;

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::EWatermarkOptionMin;
using XDPrintSchema::PageWatermark::NoWatermark;
using XDPrintSchema::PageWatermark::TextWatermark;
using XDPrintSchema::PageWatermark::BitmapWatermark;
using XDPrintSchema::PageWatermark::VectorWatermark;
using XDPrintSchema::PageWatermark::EWatermarkOptionMax;

using XDPrintSchema::PageWatermark::ECommonWatermarkProps;
using XDPrintSchema::PageWatermark::ECommonWatermarkPropsMin;
using XDPrintSchema::PageWatermark::WidthOrigin;
using XDPrintSchema::PageWatermark::HeightOrigin;
using XDPrintSchema::PageWatermark::Transparency;
using XDPrintSchema::PageWatermark::Angle;
using XDPrintSchema::PageWatermark::ECommonWatermarkPropsMax;

using XDPrintSchema::PageWatermark::ETextWatermarkProps;
using XDPrintSchema::PageWatermark::ETextWatermarkPropsMin;
using XDPrintSchema::PageWatermark::FontColor;
using XDPrintSchema::PageWatermark::FontSize;
using XDPrintSchema::PageWatermark::Text;
using XDPrintSchema::PageWatermark::ETextWatermarkPropsMax;

using XDPrintSchema::PageWatermark::EVectBmpWatermarkProps;
using XDPrintSchema::PageWatermark::EVectBmpWatermarkPropsMin;
using XDPrintSchema::PageWatermark::WidthExtent;
using XDPrintSchema::PageWatermark::HeightExtent;
using XDPrintSchema::PageWatermark::EVectBmpWatermarkPropsMax;

using XDPrintSchema::PageWatermark::WATERMARK_FEATURE;
using XDPrintSchema::PageWatermark::WATERMARK_OPTIONS;
using XDPrintSchema::PageWatermark::CMN_WATERMARK_PROPS;
using XDPrintSchema::PageWatermark::TXT_WATERMARK_PROPS;
using XDPrintSchema::PageWatermark::VECTBMP_WATERMARK_PROPS;

using XDPrintSchema::PageWatermark::Layering::ELayeringOption;

using XDPrintSchema::PageWatermark::Layering::ELayeringOptionMin;
using XDPrintSchema::PageWatermark::Layering::Overlay;
using XDPrintSchema::PageWatermark::Layering::Underlay;
using XDPrintSchema::PageWatermark::Layering::ELayeringOptionMax;

using XDPrintSchema::PageWatermark::Layering::LAYERING_FEATURE;
using XDPrintSchema::PageWatermark::Layering::LAYERING_OPTIONS;

/*++

Routine Name:

    CWMPTHandler::CWMPTHandler

Routine Description:

    CWMPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CWMPTHandler::CWMPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CWMPTHandler::~CWMPTHandler

Routine Description:

    CWMPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CWMPTHandler::~CWMPTHandler()
{
}

/*++

Routine Name:

    CWMPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with watermark data retrieved from
    the PrintTicket passed to the class constructor.

Arguments:

    pWmData - Pointer to the watermark data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CWMPTHandler::GetData(
    _Out_ WatermarkData* pWmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)))
    {
        CComBSTR bstrWMOption;
        CComBSTR bstrLayerOption;

        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(WATERMARK_FEATURE), &bstrWMOption)) &&
            SUCCEEDED(hr = GetSubFeatureOption(CComBSTR(WATERMARK_FEATURE),
                                               CComBSTR(LAYERING_FEATURE),
                                               &bstrLayerOption)) &&
            SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                  CComBSTR(CMN_WATERMARK_PROPS[WidthOrigin]),
                                                  &pWmData->widthOrigin)) &&
            SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                  CComBSTR(CMN_WATERMARK_PROPS[HeightOrigin]),
                                                  &pWmData->heightOrigin)) &&
            SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                  CComBSTR(CMN_WATERMARK_PROPS[Transparency]),
                                                  &pWmData->transparency)) &&
            SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                  CComBSTR(CMN_WATERMARK_PROPS[Angle]),
                                                  &pWmData->angle)))
        {
            if (bstrWMOption == WATERMARK_OPTIONS[BitmapWatermark])
            {
                pWmData->type = BitmapWatermark;

                if (SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                          CComBSTR(VECTBMP_WATERMARK_PROPS[WidthExtent]),
                                                          &pWmData->widthExtent)))
                {
                    hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                CComBSTR(VECTBMP_WATERMARK_PROPS[HeightExtent]),
                                                &pWmData->heightExtent);
                }

            }
            else if (bstrWMOption == WATERMARK_OPTIONS[TextWatermark])
            {
                pWmData->type = TextWatermark;
                if (SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                          CComBSTR(TXT_WATERMARK_PROPS[FontColor]),
                                                          &pWmData->txtData.bstrFontColor)) &&
                    SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                          CComBSTR(TXT_WATERMARK_PROPS[FontSize]),
                                                          &pWmData->txtData.fontSize)))
                {
                    hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                CComBSTR(TXT_WATERMARK_PROPS[Text]),
                                                &pWmData->txtData.bstrText);
                }
            }
            else if (bstrWMOption == WATERMARK_OPTIONS[VectorWatermark])
            {
                pWmData->type = VectorWatermark;

                if (SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                          CComBSTR(VECTBMP_WATERMARK_PROPS[WidthExtent]),
                                                          &pWmData->widthExtent)))
                {
                    hr = GetScoredPropertyValue(CComBSTR(WATERMARK_FEATURE),
                                                CComBSTR(VECTBMP_WATERMARK_PROPS[HeightExtent]),
                                                &pWmData->heightExtent);
                }
            }
            else
            {
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                if (bstrLayerOption == LAYERING_OPTIONS[Underlay])
                {
                    pWmData->layering = Underlay;
                }
                else if (bstrLayerOption == LAYERING_OPTIONS[Overlay])
                {
                    pWmData->layering = Overlay;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }

        if (hr == E_ELEMENT_NOT_FOUND)
        {
            pWmData->type = NoWatermark;
        }
    }

    //
    // Validate the data
    //
    if (SUCCEEDED(hr))
    {
        if (pWmData->type     <  EWatermarkOptionMin ||
            pWmData->type     >= EWatermarkOptionMax ||
            pWmData->layering <  ELayeringOptionMin ||
            pWmData->layering >= ELayeringOptionMax)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::SetData

Routine Description:

    This routine sets the watermark data in the PrintTicket passed to the
    class constructor.

Arguments:

    pWmData - Pointer to the watermark data to be set in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::SetData(
    _In_ CONST WatermarkData* pWmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)))
    {
        try
        {
            //
            // Remove any existing watermark feature node
            //
            if (SUCCEEDED(hr = Delete()) &&
                pWmData->type != NoWatermark)
            {
                //
                // Construct the appropriate watermark element
                //
                CComPtr<IXMLDOMElement> pWMDataElem(NULL);
                PTDOMElementVector      paramInitList;

                switch (pWmData->type)
                {
                    case TextWatermark:
                    {
                        hr = CreateTextWMElements(pWmData, &pWMDataElem, &paramInitList);
                    }
                    break;

                    case BitmapWatermark:
                    {
                        hr = CreateBitmapWMElements(pWmData, &pWMDataElem, &paramInitList);
                    }
                    break;

                    case VectorWatermark:
                    {
                        hr = CreateVectorWMElements(pWmData, &pWMDataElem, &paramInitList);
                    }
                    break;

                    default:
                    {
                        hr = E_FAIL;
                    }
                    break;
                }

                //
                // Insert the watermark and paramater init nodes
                //
                CComPtr<IXMLDOMNode> pPTRoot(NULL);

                CComBSTR bstrPTQuery(m_bstrFrameworkPrefix);
                bstrPTQuery += PRINTTICKET_NAME;

                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = GetNode(bstrPTQuery, &pPTRoot)))
                {
                    hr = pPTRoot->appendChild(pWMDataElem, NULL);

                    PTDOMElementVector::iterator iterParamInit = paramInitList.begin();

                    for (;iterParamInit != paramInitList.end() && SUCCEEDED(hr); iterParamInit++)
                    {
                        hr = pPTRoot->appendChild(*iterParamInit, NULL);
                    }
                }
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::Delete

Routine Description:

    This routine deletes the watermark feature from the PrintTicket passed to the
    class constructor

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::Delete(
    VOID
    )
{
    //
    // Remove any existing watermark feature node
    //
    HRESULT hr = DeleteFeature(CComBSTR(WATERMARK_FEATURE));

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::CreateCommonWMElements

Routine Description:

    This routine creates all the DOM elements common to all Watermark types. Note that
    this routine also returns the option element seperately (so the caller can specify the
    watermark type) and a list of the ParameterInit elements (so the caller can append them
    to the root PrintTicket element)

Arguments:

    pWmData        - Pointer to the watermark data structure
    ppWMDataElem   - Pointer to an IXMLDOMElement pointer that recieves feature element
    ppOptionElem   - Pointer to an IXMLDOMElement pointer that recieves option element
    pParamInitList - Pointer to a vector of DOM element pointers that recieves the parameter init elements

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::CreateCommonWMElements(
    _In_        CONST WatermarkData* pWmData,
    _Outptr_ IXMLDOMElement**     ppWMDataElem,
    _Outptr_ IXMLDOMElement**     ppOptionElem,
    _Out_       PTDOMElementVector*  pParamInitList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppWMDataElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppOptionElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pParamInitList, E_POINTER)))
    {
        *ppWMDataElem = NULL;

        if (pWmData->layering <  ELayeringOptionMin ||
            pWmData->layering >= ELayeringOptionMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the feature option pair - CreateFeatureOptionPair does not
        // set the option name, this is done in the calling function
        //
        if (SUCCEEDED(hr = CreateFeatureOptionPair(CComBSTR(WATERMARK_FEATURE), ppWMDataElem, ppOptionElem)))
        {
            //
            // Over all common properties, create and insert the element
            //
            for (ECommonWatermarkProps cmnProps = ECommonWatermarkPropsMin;
                 cmnProps < ECommonWatermarkPropsMax && SUCCEEDED(hr);
                 cmnProps = static_cast<ECommonWatermarkProps>(cmnProps + 1))
            {
                //
                // Create the scored property element
                //
                CComPtr<IXMLDOMElement> pScoredProperty(NULL);

                if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(CMN_WATERMARK_PROPS[cmnProps]), &pScoredProperty)))
                {
                    //
                    // Construct the param ref and param init elements
                    //
                    CComBSTR bstrPRefName(WATERMARK_FEATURE);

                    if (wcscmp(CMN_WATERMARK_PROPS[cmnProps], L"Angle") == 0)
                    {
                        bstrPRefName += L"Text";
                    }

                    bstrPRefName += CMN_WATERMARK_PROPS[cmnProps];

                    CComPtr<IXMLDOMElement> pParamRef(NULL);
                    CComPtr<IXMLDOMElement> pParamInit(NULL);

                    CComBSTR bstrType;
                    CComBSTR bstrValue;

                    if (SUCCEEDED(hr = GetCmnPropTypeAndValue(pWmData, cmnProps, &bstrType, &bstrValue)) &&
                        SUCCEEDED(hr = CreateParamRefInitPair(bstrPRefName, bstrType, bstrValue, &pParamRef, &pParamInit)))
                    {
                        //
                        // Append the parameter ref element to the scored property, append the
                        // scored property to the option element and add the parameter init
                        // element to the vector
                        //
                        CComPtr<IXMLDOMNode> pPRInserted(NULL);
                        CComPtr<IXMLDOMNode> pSPInserted(NULL);

                        if (SUCCEEDED(hr = pScoredProperty->appendChild(pParamRef, &pPRInserted)) &&
                            SUCCEEDED(hr = (*ppOptionElem)->appendChild(pScoredProperty, &pSPInserted)))
                        {
                            try
                            {
                                pParamInitList->push_back(pParamInit);
                            }
                            catch (exception& DBG_ONLY(e))
                            {
                                ERR(e.what());
                                hr = E_FAIL;
                            }
                        }
                    }
                }
            }
        }

        //
        // Create the layering feature
        //
        CComPtr<IXMLDOMElement> pLayeringElement(NULL);
        CComPtr<IXMLDOMElement> pLayeringOptionElement(NULL);

        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += LAYERING_OPTIONS[pWmData->layering];

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CreateFeatureOptionPair(CComBSTR(LAYERING_FEATURE),
                                                   CComBSTR(LAYERING_OPTIONS[pWmData->layering]),
                                                   &pLayeringElement,
                                                   &pLayeringOptionElement)))
        {
            //
            // Append the layering feature to the watermark feature node
            //
            hr = (*ppWMDataElem)->appendChild(pLayeringElement, NULL);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::CreateCommonVectBmpWMElements

Routine Description:

    This routine creates all the DOM elements common to all the vector and bitmap Watermark types.

Arguments:

    pWmData        - Pointer to the watermark data structure
    pOptionElem    - Pointer to an IXMLDOMElement to append the elements to
    pParamInitList - Pointer to a vector of DOM element pointers that recieves the parameter init elements

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::CreateCommonVectBmpWMElements(
    _In_  CONST WatermarkData* pWmData,
    _In_  IXMLDOMElement*      pOptionElem,
    _Out_ PTDOMElementVector*  pParamInitList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOptionElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pParamInitList, E_POINTER)))
    {
        //
        // Over all common properties, create and insert the element
        //
        for (EVectBmpWatermarkProps vectBmpProps = EVectBmpWatermarkPropsMin;
             vectBmpProps < EVectBmpWatermarkPropsMax && SUCCEEDED(hr);
             vectBmpProps = static_cast<EVectBmpWatermarkProps>(vectBmpProps + 1))
        {
            //
            // Create the scored property element
            //
            CComPtr<IXMLDOMElement> pScoredProperty(NULL);

            if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(VECTBMP_WATERMARK_PROPS[vectBmpProps]), &pScoredProperty)))
            {
                //
                // Construct the param ref and param init elements
                //
                CComBSTR bstrPRefName(WATERMARK_FEATURE);
                bstrPRefName += VECTBMP_WATERMARK_PROPS[vectBmpProps];

                CComPtr<IXMLDOMElement> pParamRef(NULL);
                CComPtr<IXMLDOMElement> pParamInit(NULL);

                CComBSTR bstrType;
                CComBSTR bstrValue;

                if (SUCCEEDED(hr = GetCmnVectBmpPropTypeAndValue(pWmData, vectBmpProps, &bstrType, &bstrValue)) &&
                    SUCCEEDED(hr = CreateParamRefInitPair(bstrPRefName, bstrType, bstrValue, &pParamRef, &pParamInit)))
                {
                    //
                    // Append the parameter ref element to the scored property, append the
                    // scored property to the option element and add the parameter init
                    // element to the vector
                    //
                    CComPtr<IXMLDOMNode> pPRInserted(NULL);
                    CComPtr<IXMLDOMNode> pSPInserted(NULL);

                    if (SUCCEEDED(hr = pScoredProperty->appendChild(pParamRef, &pPRInserted)) &&
                        SUCCEEDED(hr = pOptionElem->appendChild(pScoredProperty, &pSPInserted)))
                    {
                        try
                        {
                            pParamInitList->push_back(pParamInit);
                        }
                        catch (exception& DBG_ONLY(e))
                        {
                            ERR(e.what());
                            hr = E_FAIL;
                        }
                    }
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::CreateTextWMElements

Routine Description:

    This routine creates a text watermark feature in the PrintTicket

Arguments:

    pWmData        - Pointer to the watermark data structure
    ppWMDataElem   - Pointer to an IXMLDOMElement pointer that recieves feature element
    pParamInitList - P:ointer to a vector of DOM element pointers that recieves the parameter init elements

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::CreateTextWMElements(
    _In_        CONST WatermarkData* pWmData,
    _Outptr_ IXMLDOMElement**     ppWMDataElem,
    _Out_       PTDOMElementVector*  pParamInitList
    )
{
    HRESULT hr = S_OK;

    CComPtr<IXMLDOMElement> pOptionElem(NULL);

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppWMDataElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pParamInitList, E_POINTER)) &&
        SUCCEEDED(hr = CreateCommonWMElements(pWmData, ppWMDataElem, &pOptionElem, pParamInitList)))
    {
        //
        // Set the option node name attribute
        //
        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += WATERMARK_OPTIONS[TextWatermark];

        hr = CreateXMLAttribute(pOptionElem, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
    }

    //
    // Write out the text specific options
    //
    if (SUCCEEDED(hr))
    {
        for (ETextWatermarkProps txtProps = ETextWatermarkPropsMin;
             txtProps < ETextWatermarkPropsMax && SUCCEEDED(hr);
             txtProps = static_cast<ETextWatermarkProps>(txtProps + 1))
        {
            //
            // Create the scored property element
            //
            CComPtr<IXMLDOMElement> pScoredProperty(NULL);

            if (SUCCEEDED(hr = CreateScoredProperty(CComBSTR(TXT_WATERMARK_PROPS[txtProps]), &pScoredProperty)))
            {
                //
                // Construct the param ref and param init elements
                //
                CComBSTR bstrPRefName(WATERMARK_FEATURE);
                bstrPRefName += TXT_WATERMARK_PROPS[txtProps];

                CComPtr<IXMLDOMElement> pParamRef(NULL);
                CComPtr<IXMLDOMElement> pParamInit(NULL);

                CComBSTR bstrType;
                CComBSTR bstrValue;

                if (SUCCEEDED(hr = GetTxtPropTypeAndValue(pWmData, txtProps, &bstrType, &bstrValue)) &&
                    SUCCEEDED(hr = CreateParamRefInitPair(bstrPRefName, bstrType, bstrValue, &pParamRef, &pParamInit)))
                {
                    //
                    // Append the parameter ref element to the scored property, append the
                    // scored property to the option element and add the parameter init
                    // element to the param init list
                    //
                    CComPtr<IXMLDOMNode> pPRInserted(NULL);
                    CComPtr<IXMLDOMNode> pSPInserted(NULL);

                    if (SUCCEEDED(hr = pScoredProperty->appendChild(pParamRef, &pPRInserted)) &&
                        SUCCEEDED(hr = pOptionElem->appendChild(pScoredProperty, &pSPInserted)))
                    {
                        try
                        {
                            pParamInitList->push_back(pParamInit);
                        }
                        catch (exception& DBG_ONLY(e))
                        {
                            ERR(e.what());
                            hr = E_FAIL;
                        }
                    }
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::CreateBitmapWMElements

Routine Description:

    This routine creates a bitmap watermark feature in the PrintTicket

Arguments:

    pWmData        - Pointer to the watermark data structure
    ppWMDataElem   - Pointer to an IXMLDOMElement pointer that recieves feature element
    pParamInitList - P:ointer to a vector of DOM element pointers that recieves the parameter init elements

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::CreateBitmapWMElements(
    _In_        CONST WatermarkData* pWmData,
    _Outptr_ IXMLDOMElement**     ppWMDataElem,
    _Out_       PTDOMElementVector*  pParamInitList
    )
{
    HRESULT hr = S_OK;

    CComPtr<IXMLDOMElement> pOptionElem(NULL);

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppWMDataElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pParamInitList, E_POINTER)) &&
        SUCCEEDED(hr = CreateCommonWMElements(pWmData, ppWMDataElem, &pOptionElem, pParamInitList)) &&
        SUCCEEDED(hr = CreateCommonVectBmpWMElements(pWmData, pOptionElem, pParamInitList)))
    {
        //
        // Set the option node name attribute
        //
        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += WATERMARK_OPTIONS[BitmapWatermark];

        hr = CreateXMLAttribute(pOptionElem, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::CreateVectorWMElements

Routine Description:

    This routine creates a vector watermark feature in the PrintTicket

Arguments:

    pWmData        - Pointer to the watermark data structure
    ppWMDataElem   - Pointer to an IXMLDOMElement pointer that recieves feature element
    pParamInitList - P:ointer to a vector of DOM element pointers that recieves the parameter init elements

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::CreateVectorWMElements(
    _In_        CONST WatermarkData* pWmData,
    _Outptr_ IXMLDOMElement**     ppWMDataElem,
    _Out_       PTDOMElementVector*  pParamInitList
    )
{
    HRESULT hr = S_OK;

    CComPtr<IXMLDOMElement> pOptionElem(NULL);

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppWMDataElem, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pParamInitList, E_POINTER)) &&
        SUCCEEDED(hr = CreateCommonWMElements(pWmData, ppWMDataElem, &pOptionElem, pParamInitList)) &&
        SUCCEEDED(hr = CreateCommonVectBmpWMElements(pWmData, pOptionElem, pParamInitList)))
    {
        //
        // Set the option node name attribute
        //
        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += WATERMARK_OPTIONS[VectorWatermark];

        hr = CreateXMLAttribute(pOptionElem, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTHandler::GetCmnPropTypeAndValue

Routine Description:

    This routine initalises the type and value strings for a given common watermark property

Arguments:

    pWmData    - Pointer to a watermark data structure with the watermark settings
    cmnProps   - The watermark setting to retrieve the type and value for
    pbstrType  - Pointer to a BSTR that recieves the type of the value
    pbstrValue - Pointer to a BSTR that recieves the value as a string

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::GetCmnPropTypeAndValue(
    _In_        CONST WatermarkData*        pWmData,
    _In_        CONST ECommonWatermarkProps cmnProps,
    _Outptr_ BSTR*                       pbstrType,
    _Outptr_ BSTR*                       pbstrValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrType, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        if (cmnProps < ECommonWatermarkPropsMin ||
            cmnProps >= ECommonWatermarkPropsMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrType;
            CStringXDW cstrValue;

            switch (cmnProps)
            {
                case WidthOrigin:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->widthOrigin);
                }
                break;

                case HeightOrigin:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->heightOrigin);
                }
                break;

                case Transparency:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->transparency);
                }
                break;

                case Angle:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->angle);
                }
                break;

                default:
                {
                    hr = E_FAIL;
                    ERR("Unknown common watermark property\n");
                }
                break;
            }

            if (SUCCEEDED(hr))
            {
                *pbstrType  = cstrType.AllocSysString();
                *pbstrValue = cstrValue.AllocSysString();
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

    CWMPTHandler::GetCmnVectBmpPropTypeAndValue

Routine Description:

    This routine initalises the type and value strings for a common vector and bitmap
    watermark properties

Arguments:

    pWmData    - Pointer to a watermark data structure with the watermark settings
    cmnProps   - The setting to retrieve the type and value for
    pbstrType  - Pointer to a BSTR that recieves the type of the value
    pbstrValue - Pointer to a BSTR that recieves the value as a string

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::GetCmnVectBmpPropTypeAndValue(
    _In_        CONST WatermarkData*         pWmData,
    _In_        CONST EVectBmpWatermarkProps cmnProps,
    _Outptr_ BSTR*                        pbstrType,
    _Outptr_ BSTR*                        pbstrValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrType, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        if (cmnProps < EVectBmpWatermarkPropsMin ||
            cmnProps >= EVectBmpWatermarkPropsMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrType;
            CStringXDW cstrValue;

            switch (cmnProps)
            {
                case WidthExtent:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->widthExtent);
                }
                break;

                case HeightExtent:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->heightExtent);
                }
                break;

                default:
                {
                    hr = E_FAIL;
                    ERR("Unknown common watermark property\n");
                }
                break;
            }

            if (SUCCEEDED(hr))
            {
                *pbstrType  = cstrType.AllocSysString();
                *pbstrValue = cstrValue.AllocSysString();
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

    CWMPTHandler::GetTxtPropTypeAndValue

Routine Description:

    This routine initalises the type and value strings for a given text watermark property

Arguments:

    pWmData    - Pointer to a watermark data structure with the watermark settings
    txtProps   - The text watermark setting to retrieve the type and value for
    pbstrType  - Pointer to a BSTR that recieves the type of the value
    pbstrValue - Pointer to a BSTR that recieves the value as a string

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTHandler::GetTxtPropTypeAndValue(
    _In_        CONST WatermarkData*      pWmData,
    _In_        CONST ETextWatermarkProps txtProps,
    _Outptr_ BSTR*                     pbstrType,
    _Outptr_ BSTR*                     pbstrValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWmData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrType, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        if (txtProps < ETextWatermarkPropsMin ||
            txtProps >= ETextWatermarkPropsMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrType;
            CStringXDW cstrValue;

            switch (txtProps)
            {
                case FontColor:
                {
                    cstrType = SCHEMA_STRING;
                    cstrValue.Format(L"%s", pWmData->txtData.bstrFontColor);
                }
                break;

                case FontSize:
                {
                    cstrType = SCHEMA_INTEGER;
                    cstrValue.Format(L"%i", pWmData->txtData.fontSize);
                }
                break;

                case Text:
                {
                    cstrType = SCHEMA_STRING;
                    cstrValue.Format(L"%s", pWmData->txtData.bstrText);
                }
                break;

                default:
                {
                    hr = E_FAIL;
                    ERR("Unknown text watermark property\n");
                }
                break;
            }

            if (SUCCEEDED(hr))
            {
                *pbstrType  = cstrType.AllocSysString();
                *pbstrValue = cstrValue.AllocSysString();
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

