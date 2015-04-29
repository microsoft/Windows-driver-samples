/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmbase.cpp

Abstract:

   Base watermark class implementation. The base watermark class provides
   common functionality required between different watermarks (Text, RasterGraphic
   and VectorGraphic. This includes methods for converting a GDI matrix object
   into the appropriate XPS matrix mark-up and intialising the matrix according to
   the watermark options.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmbase.h"

using XDPrintSchema::PageWatermark::Layering::ELayeringOption;
using XDPrintSchema::PageWatermark::Layering::Overlay;
using XDPrintSchema::PageWatermark::Layering::Underlay;

/*++

Routine Name:

    CWatermark::CWatermark

Routine Description:

    Constructor for the base watermark class

Arguments:

    wmProps - Watermark PrintTicket properties class

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWatermark::CWatermark(
    _In_ CONST CWMPTProperties& wmProps
    ) :
    m_pDOMDoc(NULL),
    m_WMProps(wmProps)
{
    //
    // Create the DOM document so that sub-classes have access ASAP
    //
    HRESULT hr = m_pDOMDoc.CoCreateInstance(CLSID_DOMDocument60);

    if (FAILED(hr))
    {
        ERR("Failed to create watermark DOM document.\n");
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CWatermark::~CWatermark

Routine Description:

    Default destructor for the watermarks base class

Arguments:

    None

Return Value:

    None

--*/
CWatermark::~CWatermark()
{
}

/*++

Routine Name:

    CWatermark::MatrixToXML

Routine Description:

    Method to create XML markup representing the
    supplied transformation matrix

Arguments:

    pMatrix          - Pointer to the transformation matrix to convert to XML
    pbstrMatrixXForm - Pointer to the string which will containg the matrix markup

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermark::MatrixToXML(
    _In_        CONST Matrix* pMatrix,
    _Outptr_ BSTR*         pbstrMatrixXForm
    )
{
    //
    // Construct the matric mark-up from a GDI+ matrix
    //
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrMatrixXForm, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pMatrix, E_POINTER)))
    {
        REAL matElems[6];
        if (Ok == pMatrix->GetElements(matElems))
        {
            try
            {
                CStringXDW cstrMatrix;
                cstrMatrix.Format(L"%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
                                  matElems[0],
                                  matElems[1],
                                  matElems[2],
                                  matElems[3],
                                  matElems[4],
                                  matElems[5]);

                *pbstrMatrixXForm = cstrMatrix.AllocSysString();
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }
        else
        {
            *pbstrMatrixXForm = NULL;
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermark::GetXML

Routine Description:

    Method to get the XML containing the watermark text

Arguments:

    pbstrXML - Pointer to the string to hold the watermark XML text

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermark::GetXML(
    _Outptr_ BSTR* pbstrXML
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrXML, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pWMElem, E_PENDING)))
    {
        hr = m_pWMElem->get_xml(pbstrXML);

        if (SUCCEEDED(hr))
        {
            _Analysis_assume_nullterminated_(*pbstrXML);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermark::CreateWMTransform

Routine Description:

    Method to create a transformation matrix which will scale, translate and rotate
    the watermark to correctly fit onto the page. This overload perform scaling to fit
    the watermark content to the requested bounds and is used when creating the bitmap
    or vector watermark.

Arguments:

    wmBounds         - Rectangular area to contain the watermark
    pbstrMatrixXForm - Pointer to the string to contain the watermark transformation matrix

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermark::CreateWMTransform(
    _In_        RectF wmBounds,
    _Outptr_ BSTR* pbstrMatrixXForm
    )
{
    ASSERTMSG(wmBounds.Width > 0, "Zero width watermark found whilst creating transform\n");
    ASSERTMSG(wmBounds.Height > 0, "Zero height watermark found whilst creating transform\n");

    HRESULT hr = S_OK;

    if (wmBounds.Width <= 0 || wmBounds.Height <= 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrMatrixXForm, E_POINTER)))
    {
        *pbstrMatrixXForm = NULL;

        RectF targetBounds;
        REAL  angle = 0;

        if (SUCCEEDED(hr = m_WMProps.GetBounds(&targetBounds)) &&
            SUCCEEDED(hr = m_WMProps.GetAngle(&angle)))
        {
            ASSERTMSG(targetBounds.Width > 0, "Zero width target found whilst creating transform\n");
            ASSERTMSG(targetBounds.Height > 0, "Zero height target found whilst creating transform\n");

            //
            // Start with the identity matrix
            //
            Matrix xForm;

            //
            // Offset to the target bounds
            //
            PointF offset(targetBounds.X - wmBounds.X, targetBounds.Y - wmBounds.Y);

            //
            // Apply the transforms to the matrix
            //
            xForm.Scale(targetBounds.Width/wmBounds.Width, targetBounds.Height/wmBounds.Height, MatrixOrderAppend);
            xForm.Rotate(angle, MatrixOrderAppend);
            xForm.Translate(offset.X, offset.Y, MatrixOrderAppend);

            //
            // Retrieve the matrix string
            //
            hr = MatrixToXML(&xForm, pbstrMatrixXForm);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermark::CreateWMTransform

Routine Description:

    Method to create a transformation matrix which will translate and rotate. This overload
    does not perform any scaling to fit the watermark content and is used when creating the
    text watermark.

Arguments:

    wmOrigin         - Point defining the watermark origin
    pbstrMatrixXForm - Pointer to the string to contain the watermark transformation matrix

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermark::CreateWMTransform(
    _In_        PointF wmOrigin,
    _Outptr_ BSTR*  pbstrMatrixXForm
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrMatrixXForm, E_POINTER)))
    {
        *pbstrMatrixXForm = NULL;
        REAL  angle = 0;

        if (SUCCEEDED(hr = m_WMProps.GetAngle(&angle)))
        {
            //
            // Start with the identity matrix
            //
            Matrix xForm;

            //
            // Apply the transforms to the matrix
            //
            xForm.Rotate(angle, MatrixOrderAppend);
            xForm.Translate(wmOrigin.X, wmOrigin.Y, MatrixOrderAppend);

            //
            // Retrieve the matrix string
            //
            hr = MatrixToXML(&xForm, pbstrMatrixXForm);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermark::InsertStart

Routine Description:

    Method to check whether the watermark should be inserted at the start of the
    page to create an underlay effect

Arguments:

    None

Return Value:

    BOOL
    TRUE  - The watermark mark-up needs to be inserted at the start of the fixed page
    FALSE - The watermark mark-up needs to be inserted at the end of the fixed page

--*/
BOOL
CWatermark::InsertStart(
    VOID
    )
{
    ELayeringOption wmLayering = Overlay;

    m_WMProps.GetLayering(&wmLayering);

    return wmLayering == Underlay;
}

/*++

Routine Name:

    CWatermark::InsertEnd

Routine Description:

    Method to check whether the watermark should be inserted at the
    end of the page to create an overlay effect

Arguments:

    None

Return Value:

    BOOL
    TRUE  - The watermark mark-up needs to be inserted at the end of the fixed page
    FALSE - The watermark mark-up needs to be inserted at the start of the fixed page

--*/
BOOL
CWatermark::InsertEnd(
    VOID
    )
{
    return !InsertStart();
}

