/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmrast.cpp

Abstract:

   RasterGraphic watermark class implamentation. CRasterWatermark is the
   raster implementation of the CWatermark class. This implements methods
   for creating the page mark-up and adding the watermark resource to the
   resource cache.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmrast.h"
#include "wmptprop.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::BitmapWatermark;

/*++

Routine Name:

    CRasterWatermark::CRasterWatermark

Routine Description:

    Constructor for the raster watermark class

Arguments:

    wmProps    - Watermark PrintTicket properties class
    resourceID - Resource ID for the watermark

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CRasterWatermark::CRasterWatermark(
    _In_ CONST CWMPTProperties& wmProps,
    _In_ CONST INT              resourceID
    ) :
    CWatermark(wmProps),
    m_wmBMP(wmProps, resourceID)
{
    HRESULT hr = S_OK;

    EWatermarkOption wmType;

    if (SUCCEEDED(hr = m_WMProps.GetType(&wmType)))
    {
        if (wmType != BitmapWatermark)
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

    CRasterWatermark::~CRasterWatermark

Routine Description:

    Default destructor for the watermarks base class

Arguments:

    None

Return Value:

    None

--*/
CRasterWatermark::~CRasterWatermark()
{
}

/*++

Routine Name:

    CRasterWatermark::CreateXMLElement

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
CRasterWatermark::CreateXMLElement(
    VOID
    )
{
    ASSERTMSG(m_pDOMDoc != NULL, "NULL DOM document detected whilst creating text watermark\n");
    ASSERTMSG(m_bstrImageURI.Length() > 0, "Invalid image URI detected whilst creating text watermark\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pDOMDoc, E_PENDING)))
    {
        if (m_bstrImageURI.Length() == 0)
        {
            hr = E_PENDING;
        }
    }

    //
    // We need to retrieve the bitmap bounds to calculate
    // the correct render transform for the watermark
    //
    SizeF bmpDims;
    CComBSTR bstrWMOpacity;
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_wmBMP.GetImageDimensions(&bmpDims)) &&
        SUCCEEDED(hr = m_WMProps.GetOpacity(&bstrWMOpacity)))
    {
        //
        // The markup will look like this (square bracketed values "[]"
        // describes content):
        //
        // <Path
        //     RenderTransform="[appropriate to scale, translate and rotate to the PT settings]"
        //     Data="M [corner coords of image] z">
        //     <Path.Fill>
        //         <ImageBrush
        //             ImageSource="[image URI from cache]"
        //             Opacity="[opacity value from PT]"
        //             ViewboxUnits="Absolute"
        //             Viewbox="[image bounds]"
        //             ViewportUnits="Absolute"
        //             Viewport="[image bounds]" />
        //     </Path.Fill>
        // </Path>
        //
        CComPtr<IXMLDOMElement> pPathFill(NULL);
        CComPtr<IXMLDOMElement> pImageBrush(NULL);
        CComPtr<IXMLDOMNode>    pInsertNode(NULL);

        CStringXDW strPathData;
        CStringXDW strViewbox;
        CStringXDW strViewport;

        CComBSTR bstrMatrixXForm;

        //
        // Create the transform and elements and add attributes
        //
        try
        {
            strPathData.Format(L"M 0,0 L 0,%.2f %.2f,%.2f %.2f,0 z", bmpDims.Height, bmpDims.Width, bmpDims.Height, bmpDims.Width);
            strViewbox.Format(L"0,0,%.2f,%.2f", bmpDims.Width, bmpDims.Height);
            strViewport.Format(L"0,0,%.2f,%.2f", bmpDims.Width, bmpDims.Height);

            if (SUCCEEDED(hr = CreateWMTransform(RectF(0, 0, bmpDims.Width, bmpDims.Height), &bstrMatrixXForm)) &&
                SUCCEEDED(hr = m_pDOMDoc->createElement(CComBSTR(L"Path"), &m_pWMElem)) &&
                SUCCEEDED(hr = m_pDOMDoc->createElement(CComBSTR(L"Path.Fill"), &pPathFill)) &&
                SUCCEEDED(hr = m_pDOMDoc->createElement(CComBSTR(L"ImageBrush"), &pImageBrush)) &&
                SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"RenderTransform"), CComVariant(bstrMatrixXForm))) &&
                SUCCEEDED(hr = m_pWMElem->setAttribute(CComBSTR(L"Data"), CComVariant(strPathData.GetBuffer()))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"ImageSource"), CComVariant(m_bstrImageURI))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"Opacity"), CComVariant(bstrWMOpacity))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"ViewboxUnits"), CComVariant(L"Absolute"))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"Viewbox"), CComVariant(strViewbox.GetBuffer()))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"ViewportUnits"), CComVariant(L"Absolute"))) &&
                SUCCEEDED(hr = pImageBrush->setAttribute(CComBSTR(L"Viewport"), CComVariant(strViewport.GetBuffer()))) &&
                SUCCEEDED(hr = pPathFill->appendChild(pImageBrush, &pInsertNode)) &&
                SUCCEEDED(hr = m_pWMElem->appendChild(pPathFill, &pInsertNode)))
            {
                hr = m_pWMElem->appendChild(pPathFill, &pInsertNode);
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

    CRasterWatermark::AddParts

Routine Description:

    Method to add the watermark bitmap resource to the cache

Arguments:

    pXpsConsumer - Pointer to the writer used when writing the resource back out to the pipeline
    pFixedPage   - Pointer to the fixed page associated with the bitmap
    pResCache    - Pointer to a resource cache object which manages the writing of the bitmap

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRasterWatermark::AddParts(
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
        m_bstrImageURI.Empty();
        CComBSTR bstrKey;
        if (SUCCEEDED(hr = m_wmBMP.CheckResID()) &&
            SUCCEEDED(hr = pResCache->WriteResource<IPartImage>(pXpsConsumer, pFixedPage, &m_wmBMP)) &&
            SUCCEEDED(hr = m_wmBMP.GetKeyName(&bstrKey)))
        {
            hr = pResCache->GetURI(bstrKey, &m_bstrImageURI);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

