/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmtext.cpp

Abstract:

   Text watermark class implamentation. CTextWatermark is the
   text implementation of the CWatermark class. This implements methods
   for creating the page mark-up and adding the watermark resource to the
   resource cache.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "wmtext.h"
#include "wmptprop.h"
#include "rescache.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::TextWatermark;

/*++

Routine Name:

    CTextWatermark::CTextWatermark

Routine Description:

    Constructor for the text watermark class

Arguments:

    wmProps - Watermark PrintTicket properties class

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CTextWatermark::CTextWatermark(
    _In_ CONST CWMPTProperties& wmProps
    ) :
    CWatermark(wmProps),
    m_wmFont(wmProps)
{
    HRESULT hr = S_OK;

    EWatermarkOption wmType;

    if (SUCCEEDED(hr = m_WMProps.GetType(&wmType)))
    {
        if (wmType != TextWatermark)
        {
            hr = E_INVALIDARG;
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CTextWatermark::~CTextWatermark

Routine Description:

    Default destructor for the watermarks base class

Arguments:

    None

Return Value:

    None

--*/
CTextWatermark::~CTextWatermark()
{
}

/*++

Routine Name:

    CTextWatermark::CreateXMLElement

Routine Description:

    Method to create the XML markup which describes the transform and properties used
    to present the watermark correctly on the page

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTextWatermark::CreateXMLElement(
    VOID
    )
{
    ASSERTMSG(m_pDOMDoc != NULL, "NULL DOM document detected whilst creating text watermark\n");
    ASSERTMSG(m_bstrFontURI.Length() > 0, "Invalid font URI detected whilst creating text watermark\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pDOMDoc, E_PENDING)))
    {
        if (m_bstrFontURI.Length() == 0)
        {
            hr = E_PENDING;
        }
    }

    PointF stringOrigin;

    CComBSTR bstrWMText;
    CComBSTR bstrWMOpacity;
    CComBSTR bstrWMFontSize;
    CComBSTR bstrWMFontColor;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_WMProps.GetText(&bstrWMText)) &&
        SUCCEEDED(hr = m_WMProps.GetFontColor(&bstrWMFontColor)) &&
        SUCCEEDED(hr = m_WMProps.GetFontEmSize(&bstrWMFontSize)) &&
        SUCCEEDED(hr = m_WMProps.GetOpacity(&bstrWMOpacity)) &&
        SUCCEEDED(hr = m_WMProps.GetOrigin(&stringOrigin)))
    {
        //
        // The markup will look like this (square bracketed values "[]"
        // describes content):
        //
        // <Glyphs
        //     Fill="[color ref]"
        //     Opacity="[float value between 0 and 1]"
        //     RenderTransform="[matrix transform from PT angle and offset]"
        //     FontURI="[font URI]"
        //     FontRenderingEmSize="[em size]"
        //     OriginX="0"
        //     OriginY="0"
        //     UnicodeString="[the watermark text]"
        // />
        //
        CComBSTR bstrMatrixXForm;

        CComBSTR bstrFontName(L"/");
        hr = bstrFontName.Append(m_bstrFontURI);

        //
        // Create the transform and element and add attributes
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CreateWMTransform(stringOrigin, &bstrMatrixXForm)) &&
            SUCCEEDED(hr = m_pDOMDoc->createElement(CComBSTR(L"Glyphs"), &m_pWMElem)) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"Fill"), CComVariant(bstrWMFontColor))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"Opacity"), CComVariant(bstrWMOpacity))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"RenderTransform"), CComVariant(bstrMatrixXForm))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"FontUri"), CComVariant(bstrFontName))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"FontRenderingEmSize"), CComVariant(bstrWMFontSize))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"OriginX"), CComVariant(L"0"))) &&
            SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"OriginY"), CComVariant(L"0"))))
        {
            hr = m_pWMElem->setAttribute(CComBSTR(L"UnicodeString"), CComVariant(bstrWMText));
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CTextWatermark::AddParts

Routine Description:

    Method to add the watermark font resource to the cache

Arguments:

    pXpsConsumer - Pointer to the writer used when writing the resource back out to the pipeline
    pFixedPage   - Pointer to the fixed page associated with the font
    pResCache    - Pointer to a resource cache object which manages the writing of the font

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTextWatermark::AddParts(
    _In_ IXpsDocumentConsumer* pXpsConsumer,
    _In_ IFixedPage*           pFixedPage,
    _In_ CFileResourceCache*   pResCache
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pXpsConsumer, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFixedPage, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pResCache, E_POINTER)))
    {
        //
        // Write the font resource to the cache
        //
        m_bstrFontURI.Empty();
        CComBSTR bstrKey;
        if (SUCCEEDED(hr = pResCache->WriteResource<IPartFont>(pXpsConsumer, pFixedPage, &m_wmFont)) &&
            SUCCEEDED(hr = m_wmFont.GetKeyName(&bstrKey)))
        {
            hr = pResCache->GetURI(bstrKey, &m_bstrFontURI);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

