/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pagescale.cpp

Abstract:

   Page Scaling class implementation. The Page Scaling class provides
   functionality required to perform page Scaling. This includes methods for converting a GDI matrix object
   into the appropriate XPS matrix mark-up and intialising the matrix according to
   the Page Scaling options.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "pagescale.h"

using XDPrintSchema::PageScaling::EScaleOption;
using XDPrintSchema::PageScaling::Custom;
using XDPrintSchema::PageScaling::CustomSquare;
using XDPrintSchema::PageScaling::FitBleedToImageable;
using XDPrintSchema::PageScaling::FitContentToImageable;
using XDPrintSchema::PageScaling::FitMediaToImageable;
using XDPrintSchema::PageScaling::FitMediaToMedia;

using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOption;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomLeft;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomRight;
using XDPrintSchema::PageScaling::OffsetAlignment::Center;
using XDPrintSchema::PageScaling::OffsetAlignment::LeftCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::RightCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::TopCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::TopLeft;
using XDPrintSchema::PageScaling::OffsetAlignment::TopRight;

/*++

Routine Name:

    CPageScaling::CPageScaling

Routine Description:

    CPageScaling class constructor

Arguments:

    pgscProps - Reference to the page scaling PrintTicket properties.

Return Value:

    None

--*/
CPageScaling::CPageScaling(
    _In_ CONST CPGSCPTProperties& pgscProps
    ) :
    m_pageDimensions(0.0f, 0.0f),
    m_PGSCProps(pgscProps),
    m_bAddCanvas(FALSE),
    m_contentBox(0.0f, 0.0f, 0.0f, 0.0f),
    m_bleedBox(0.0f, 0.0f, 0.0f, 0.0f)
{
}

/*++

Routine Name:

    CPageScaling::~CPageScaling

Routine Description:

    CPageScaling class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageScaling::~CPageScaling()
{
}

/*++

Routine Name:

    CPageScaling::GetFixedPageWidth

Routine Description:

    Creates a string containing the page width in 1/96 Inch
    (Not applicable to Custom Scaling options).

Arguments:

    pbstrWidth - Address of a pointer that will be modified to point to a string
                 that is filled out with the fixed page width.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::GetFixedPageWidth(
    _Outptr_ BSTR* pbstrWidth
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrWidth, E_POINTER)))
    {
        try
        {
            SizeF targetPage;
            if (SUCCEEDED(hr = m_PGSCProps.GetPageSize(&targetPage)))
            {
                CStringXDW cstrValue;
                cstrValue.Format(L"%.2f", targetPage.Width);
                *pbstrWidth = cstrValue.AllocSysString();
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

    CPageScaling::GetFixedPageHeight

Routine Description:

    Creates a string containing the page height in 1/96 Inch
    (Not applicable to Custom Scaling options).

Arguments:

    pbstrHeight - Address of a pointer that will be modified to point to a string
                  that is filled out with the fixed page height.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::GetFixedPageHeight(
    _Outptr_ BSTR* pbstrHeight
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrHeight, E_POINTER)))
    {
        try
        {
            SizeF targetPage;
            if (SUCCEEDED(hr = m_PGSCProps.GetPageSize(&targetPage)))
            {
                CStringXDW cstrValue;
                cstrValue.Format(L"%.2f", targetPage.Height);
                *pbstrHeight = cstrValue.AllocSysString();
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

    CPageScaling::MatrixToXML

Routine Description:

    Creates a string representation of a Matrix class object.
    The string is suitable for use in XPS markup with the 'RenderTransform' keyword.

Arguments:

    pMatrix - Pointer to a Matrix class object.
    pbstrMatrixXForm - Address of a pointer that will be modified to point to a string
                       that is filled out with the matrix.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::MatrixToXML(
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

    CPageScaling::SetPageDimensions

Routine Description:

    Used to set the XPS page width and height in the page scaling interface.

Arguments:

    width - Width of the XPS fixed page in 1/96 Inch.
    height - Height of the XPS fixed page in 1/96 Inch.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::SetPageDimensions(
    _In_ CONST REAL width,
    _In_ CONST REAL height
    )
{
    m_pageDimensions.Width  = width;
    m_pageDimensions.Height = height;

    return S_OK;
}

/*++

Routine Name:

    CPageScaling::SetBleedBox

Routine Description:

    Used to set the XPS BleedBox in the page scaling interface.

Arguments:

    pBleedBox - pointer to a rectangle defining the BleedBox of the XPS page in 1/96 Inch.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::SetBleedBox(
    _In_ RectF*       pBleedBox
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBleedBox, E_POINTER)))
    {
        m_bleedBox = *pBleedBox;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScaling::SetContentBox

Routine Description:

    Used to set the XPS ContentBox in the page scaling interface.

Arguments:

    pContentBox - pointer to a rectangle defining the ContentBox of the XPS page in 1/96 Inch.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::SetContentBox(
    _In_ RectF*       pContentBox
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pContentBox, E_POINTER)))
    {
        m_contentBox = *pContentBox;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScaling::CalculateMatrix

Routine Description:

    Calculates a matrix transform from the current page scaling properties.

Arguments:

    pMatrix - Pointer to a Matrix class object that is modified to reflect the current transform.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::CalculateMatrix(
    _Inout_ Matrix* pMatrix
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pMatrix, E_POINTER)))
    {
        REAL xScale = 1.0f;
        REAL yScale = 1.0f;
        REAL widthOffset = 0.0f;
        REAL heightOffset = 0.0f;

        EScaleOption pgscOption;

        if (SUCCEEDED(hr = m_PGSCProps.GetOption(&pgscOption)))
        {
            switch (pgscOption)
            {
                case Custom:
                case CustomSquare:
                {
                    //
                    // The PrintTicket property handler for Custom and CustomSquare
                    // already handle mapping of x and y.
                    //
                    if (SUCCEEDED(hr = m_PGSCProps.GetWidthScale(&xScale)) &&
                        SUCCEEDED(hr = m_PGSCProps.GetHeightScale(&yScale)) &&
                        SUCCEEDED(hr = m_PGSCProps.GetWidthOffset(&widthOffset)))
                    {
                        hr = m_PGSCProps.GetHeightOffset(&heightOffset);
                    }
                }
                break;

                case FitBleedToImageable:   // ImageableSize <- BleedBox
                case FitContentToImageable: // ImageableSize <- ContentBox
                case FitMediaToImageable:   // ImageableSize <- FixedPage
                case FitMediaToMedia:       // Mediasize     <- FixedPage
                {
                    EScaleOffsetOption offsetOption;

                    if (SUCCEEDED(hr = m_PGSCProps.GetOffsetOption(&offsetOption)))
                    {
                        RectF targetPage;
                        RectF sourcePage;

                        switch (pgscOption)
                        {
                            case FitBleedToImageable:
                            {
                                m_PGSCProps.GetImageableRect(&targetPage);

                                sourcePage = m_bleedBox;
                            }
                            break;
                            case FitContentToImageable:
                            {
                                m_PGSCProps.GetImageableRect(&targetPage);

                                sourcePage = m_contentBox;
                            }
                            break;
                            case FitMediaToImageable:
                            {
                                m_PGSCProps.GetImageableRect(&targetPage);

                                sourcePage = RectF(0, 0, m_pageDimensions.Width, m_pageDimensions.Height);
                            }
                            break;
                            case FitMediaToMedia:
                            {
                                SizeF pageSize;
                                m_PGSCProps.GetPageSize(&pageSize);
                                targetPage = RectF(0, 0, pageSize.Width, pageSize.Height);

                                sourcePage = RectF(0, 0, m_pageDimensions.Width, m_pageDimensions.Height);
                            }
                            break;
                        }

                        //
                        // Calculate scaling factors
                        //
                        xScale = static_cast<REAL>(targetPage.Width / sourcePage.Width);
                        yScale = static_cast<REAL>(targetPage.Height / sourcePage.Height);

                        //
                        // Best Fit, always maintain aspect ratio
                        // Select the smallest of the two scale factors,
                        // This will ensure that the image always fits on the page.
                        //
                        if (xScale < yScale)
                        {
                            yScale = xScale;
                        }
                        else
                        {
                            xScale = yScale;
                        }

                        //
                        // Calculate the offset to meet the offset alignment setting
                        //
                        RectF scaledPage(xScale*sourcePage.X, yScale*sourcePage.Y, xScale*sourcePage.Width, yScale*sourcePage.Height);
                        switch (offsetOption)
                        {
                            case BottomCenter:
                            {
                                widthOffset  = (targetPage.X + (targetPage.Width/2.0f)) - (scaledPage.X + (scaledPage.Width/2.0f));
                                heightOffset = (targetPage.Y + targetPage.Height) - (scaledPage.Y + scaledPage.Height);
                            }
                            break;
                            case BottomLeft:
                            {
                                widthOffset  = targetPage.X - scaledPage.X;
                                heightOffset = targetPage.Y + targetPage.Height - (scaledPage.Y + scaledPage.Height);
                            }
                            break;
                            case BottomRight:
                            {
                                widthOffset  = (targetPage.X + targetPage.Width) - (scaledPage.X + scaledPage.Width);
                                heightOffset = (targetPage.Y + targetPage.Height) - (scaledPage.Y + scaledPage.Height);
                            }
                            break;
                            case Center:
                            {
                                widthOffset  = (targetPage.X + (targetPage.Width/2.0f)) - (scaledPage.X + (scaledPage.Width/2.0f));
                                heightOffset = (targetPage.Y + (targetPage.Height/2.0f)) - (scaledPage.Y + (scaledPage.Height/2.0f));
                            }
                            break;
                            case LeftCenter:
                            {
                                widthOffset  = targetPage.X - scaledPage.X;
                                heightOffset = (targetPage.Y + (targetPage.Height/2.0f)) - (scaledPage.Y + (scaledPage.Height/2.0f));
                            }
                            break;
                            case RightCenter:
                            {
                                widthOffset  = (targetPage.X + targetPage.Width) - (scaledPage.X + scaledPage.Width);
                                heightOffset = (targetPage.Y + (targetPage.Height/2.0f)) - (scaledPage.Y + (scaledPage.Height/2.0f));
                            }
                            break;
                            case TopCenter:
                            {
                                widthOffset  = (targetPage.X + (targetPage.Width/2.0f)) - (scaledPage.X + (scaledPage.Width/2.0f));
                                heightOffset = targetPage.Y - scaledPage.Y;
                            }
                            break;
                            default:
                            case TopLeft:
                            {
                                //
                                // Default Top Left
                                //
                                widthOffset  = targetPage.X - scaledPage.X;
                                heightOffset = targetPage.Y - scaledPage.Y;
                            }
                            break;
                            case TopRight:
                            {
                                widthOffset  = (targetPage.X + targetPage.Width) - (scaledPage.X + scaledPage.Width);
                                heightOffset = targetPage.Y - scaledPage.Y;
                            }
                            break;
                        }
                    }
                }
                break;

                default:
                {
                    //
                    // No Scaling required.
                    //
                }
                break;
            }
        }

        //
        // Apply the transforms to the matrix
        //
        Status gdiPlusStatus = pMatrix->Scale(xScale, yScale, MatrixOrderAppend);

        if (gdiPlusStatus == Ok)
        {
            gdiPlusStatus = pMatrix->Translate(widthOffset, heightOffset, MatrixOrderAppend);
        }

        if (gdiPlusStatus != Ok)
        {
            hr = GetGDIStatusErrorAsHResult(gdiPlusStatus);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CPageScaling::CreateTransform

Routine Description:

    Creates a string representation of the current page scaling matrix.
    The string is suitable for use in XPS markup with the 'RenderTransform' keyword.

Arguments:

    pbstrMatrixXForm - Address of a pointer that will be modified to point to a string
                       that is filled out with the matrix.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::CreateTransform(
    _Outptr_ BSTR* pbstrMatrixXForm
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrMatrixXForm, E_POINTER)))
    {
        *pbstrMatrixXForm = NULL;

        //
        // Start with the identity matrix
        //

        Matrix xForm;

        if (SUCCEEDED(hr = CalculateMatrix(&xForm)))
        {
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

    CPageScaling::GetOpenTagXML

Routine Description:

    Creates a string that contains the open tag XPS markup required to
    scale the page content. Page Scaling is achieved by wrapping the page content
    in a canvas and applying a RenderTransform which reflects the current page scaling properties.

Arguments:

    pbstrXML - Address of a pointer that will be modified to point to a string
               that is filled out with the page scaling close tag XPS markup.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::GetOpenTagXML(
    _Outptr_ BSTR* pbstrXML
    )
{
    HRESULT hr = S_OK;

    //
    // Create the Canvas with Render Transform
    //
    try
    {
        CComBSTR bstrMatrixXForm;

        if (SUCCEEDED(hr = CreateTransform(&bstrMatrixXForm)))
        {
            CStringXDW cstrCanvas;
            cstrCanvas.Format(L"<Canvas RenderTransform=\"%s\"", bstrMatrixXForm);
            *pbstrXML = cstrCanvas.AllocSysString();
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScaling::GetCloseTagXML

Routine Description:

    Creates a string that contains the close tag XPS markup.
    This is achived by closing the canvas tag.

Arguments:

    pbstrXML - Address of a pointer that will be modified to point to a string
               that is filled out with the page scaling close tag XPS markup.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScaling::GetCloseTagXML(
    _Outptr_ BSTR* pbstrXML
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrXML, E_POINTER)))
    {
        try
        {
            CStringXDW cstrCanvas(L"</Canvas>\n");
            *pbstrXML = cstrCanvas.AllocSysString();
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

