/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmvect.cpp

Abstract:

   VectorGraphic watermark class implamentation. CVectorWatermark is the
   vecotr implementation of the CWatermark class. This implements methods
   for creating the page mark-up and adding the watermark resource to the
   markup.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmvect.h"
#include "wmptprop.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::VectorWatermark;

/*++

Routine Name:

    CVectorWatermark::CVectorWatermark

Routine Description:

    Constructor for the vector watermark class

Arguments:

    wmProps    - Watermark PrintTicket properties class
    resourceID - Resource ID for the watermark

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CVectorWatermark::CVectorWatermark(
    _In_ CONST CWMPTProperties& wmProps,
    _In_ CONST INT              resourceID
    ) :
    CWatermark(wmProps),
    m_wmMarkup(wmProps, resourceID)
{
    HRESULT hr = S_OK;

    EWatermarkOption wmType;

    if (SUCCEEDED(hr = m_WMProps.GetType(&wmType)))
    {
        if (wmType != VectorWatermark)
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

    CVectorWatermark::~CVectorWatermark

Routine Description:

    Default destructor for the watermarks base class

Arguments:

    None

Return Value:

    None

--*/
CVectorWatermark::~CVectorWatermark()
{
}

/*++

Routine Name:

    CVectorWatermark::CreateXMLElement

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
CVectorWatermark::CreateXMLElement(
    VOID
    )
{
    ASSERTMSG(m_pDOMDoc != NULL, "NULL DOM document detected whilst creating text watermark\n");

    HRESULT hr = S_OK;

    //
    // We need to retrieve the RAW Markup bounds to calculate
    // the correct render transform for the watermark
    //
    SizeF markupDims;
    CComBSTR bstrWMOpacity;
    if (SUCCEEDED(hr = CHECK_POINTER(m_pDOMDoc, E_PENDING)) &&
        SUCCEEDED(hr = m_wmMarkup.GetImageDimensions(&markupDims)) &&
        SUCCEEDED(hr = m_WMProps.GetOpacity(&bstrWMOpacity)))
    {
        //
        // The markup will look like this (square bracketed values "[]"
        // describes content):
        //
        // <Canvas
        //     Opacity="[appropriate transparency value]"
        //     RenderTransform="[appropriate to scale, translate and rotate to the PT settings]"
        //     [Raw Markup]
        // </Canvas>
        //
        CComBSTR bstrMatrixXForm;

        if (SUCCEEDED(hr = CreateWMTransform(RectF(0, 0, markupDims.Width, markupDims.Height), &bstrMatrixXForm)))
        {
            try
            {
                CStringXDW strOpenCanvas;
                strOpenCanvas.Format(L"<Canvas Opacity=\"%s\" RenderTransform=\"%s\">", bstrWMOpacity, bstrMatrixXForm);

                IStream* pStream;
                CComBSTR bstrContent;

                if (SUCCEEDED(hr = m_wmMarkup.GetStream(&pStream)) &&
                    SUCCEEDED(hr = bstrContent.ReadFromStream(pStream)))
                {
                    m_bstrMarkup.Empty();

                    if (SUCCEEDED(hr = m_bstrMarkup.Append(strOpenCanvas)) &&
                        SUCCEEDED(hr = m_bstrMarkup.Append(bstrContent)))
                    {
                        hr = m_bstrMarkup.Append(L"</Canvas>\n");
                    }
                }
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CVectorWatermark::GetXML

Routine Description:

    Method for copying the XML markup which describes the watermark text

Arguments:

    pbstrXML - Pointer to the string to contain the XML vector markup. Any BSTR
    this points to will be freed.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CVectorWatermark::GetXML(
    _Inout_ _At_(*pbstrXML, _Pre_maybenull_ _Post_valid_) BSTR* pbstrXML
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrXML, E_POINTER)))
    {
        SysFreeString(*pbstrXML);

        if (m_bstrMarkup.Length() > 0)
        {
            if (SUCCEEDED(hr = m_bstrMarkup.CopyTo(pbstrXML)) &&
                !*pbstrXML)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_PENDING;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CVectorWatermark::AddParts

Routine Description:

    Method to add any required resource to the cache.
    This isn't required for vector based watermarks.

Arguments:

    pXpsConsumer - Pointer to the writer used when writing the resource back out to the pipeline
    pFixedPage   - Pointer to the fixed page associated with the resource
    pResCache    - Pointer to a resource cache object which manages the writing of the resource

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CVectorWatermark::AddParts(
    _In_ IXpsDocumentConsumer*,
    _In_ IFixedPage*,
    _In_ CFileResourceCache*
    )
{
    //
    // No Implementation required.
    //

    return S_OK;
}

