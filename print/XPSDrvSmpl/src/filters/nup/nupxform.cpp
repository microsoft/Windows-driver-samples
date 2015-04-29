/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupxform.cpp

Abstract:

   NUp transform implementation. The NUp transform class is responsible
   for calculating the appropriate matrix transform for a given page in
   an NUp sequence.

Known Issues:

    The NUp implementation does not yet account for reverse orientations
    (ReverseLandscape and ReversePortrait).

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "nupxform.h"

using XDPrintSchema::NUp::NUpData;
using XDPrintSchema::NUp::JobNUpAllDocumentsContiguously;

using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMin;
using XDPrintSchema::NUp::PresentationDirection::RightBottom;
using XDPrintSchema::NUp::PresentationDirection::BottomRight;
using XDPrintSchema::NUp::PresentationDirection::LeftBottom;
using XDPrintSchema::NUp::PresentationDirection::BottomLeft;
using XDPrintSchema::NUp::PresentationDirection::RightTop;
using XDPrintSchema::NUp::PresentationDirection::TopRight;
using XDPrintSchema::NUp::PresentationDirection::LeftTop;
using XDPrintSchema::NUp::PresentationDirection::TopLeft;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMax;

using XDPrintSchema::Binding::EBindingOption;
using XDPrintSchema::Binding::None;
using XDPrintSchema::Binding::BindTop;
using XDPrintSchema::Binding::BindBottom;

using XDPrintSchema::PageOrientation::EOrientationOption;
using XDPrintSchema::PageOrientation::Landscape;
using XDPrintSchema::PageOrientation::ReverseLandscape;
using XDPrintSchema::PageOrientation::Portrait;

static CONST REAL kLetterWidth96thsInch  = 816.0f;
static CONST REAL kLetterHeight96thsInch = 1056.0f;

/*++

Routine Name:

    CNUpTransform::CNUpTransform

Routine Description:

    Constructor for the CNUpTransform class which initialises members
    to sensible values based on the supplied PrintTicket object

Arguments:

    pNUpPTProps - PrintTicket properties object used for initialising the
                  CNUpTransform object

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CNUpTransform::CNUpTransform(
    _In_ CNUpPTProperties* pNUpPTProps
    ) :
    m_bRotatePage(FALSE),
    m_cCurrPageIndex(0)
{
    ASSERTMSG(pNUpPTProps != NULL, "NULL PrintTicket passed to page transform handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpPTProps, E_POINTER)))
    {
        if (SUCCEEDED(hr) &&
            FAILED(hr = pNUpPTProps->GetCount(&m_CanvasCount)))
        {
            m_CanvasCount = 1;
            ERR("Failed to retrieve NUp count\n");
        }

        if (SUCCEEDED(hr) &&
            FAILED(hr = pNUpPTProps->GetPresentationDirection(&m_NUpPresentDirection)))
        {
            m_NUpPresentDirection = RightBottom;
            ERR("Failed to retrieve NUp order\n");
        }

        if (SUCCEEDED(hr) &&
            FAILED(hr = pNUpPTProps->GetPageSize(&m_sizeTargetPage)))
        {
            m_sizeTargetPage.Width  = kLetterWidth96thsInch;
            m_sizeTargetPage.Height = kLetterHeight96thsInch;
            ERR("Failed to retrieve target page size\n");
        }

        EOrientationOption pgOrient = Portrait;
        if (SUCCEEDED(hr) &&
            FAILED(hr = pNUpPTProps->GetPageOrientation(&pgOrient)))
        {
            ERR("Failed to retrieve target page orientation\n");
        }

        //
        // Initialise canvas count
        //
        switch (m_CanvasCount)
        {
            case 1:
            {
                m_CanvasXCount = 1;
                m_CanvasYCount = 1;
            }
            break;

            case 2:
            {
                m_CanvasXCount = 1;
                m_CanvasYCount = 2;
                m_bRotatePage = TRUE;

                //
                // Booklet printing is treated as a special case of 2Up.
                // Unlike regular 2Up which always rotates pages, booklet
                // only requires pages to be rotated when binding left to right
                // or right to left.
                //
                EBindingOption bindingOption;
                if(SUCCEEDED(hr = pNUpPTProps->GetBindingOption(&bindingOption)))
                {
                    if(bindingOption == BindBottom ||
                       bindingOption == BindTop)
                    {
                        m_bRotatePage = FALSE;
                    }

                    if (pgOrient == Landscape)
                    {
                        m_CanvasXCount = 2;
                        m_CanvasYCount = 1;
                    }
                }
            }
            break;

            case 4:
            {
                m_CanvasXCount = 2;
                m_CanvasYCount = 2;
            }
            break;

            case 6:
            {
                m_CanvasXCount = 2;
                m_CanvasYCount = 3;
                m_bRotatePage = TRUE;
            }
            break;

            case 8:
            {
                m_CanvasXCount = 2;
                m_CanvasYCount = 4;
                m_bRotatePage = TRUE;
            }
            break;

            case 9:
            {
                m_CanvasXCount = 3;
                m_CanvasYCount = 3;
            }
            break;

            case 16:
            {
                m_CanvasXCount = 4;
                m_CanvasYCount = 4;
            }
            break;

            default:
            {
                m_CanvasXCount = 1;
                m_CanvasYCount = 1;
            }
            break;
        }

        if (pgOrient == Landscape ||
            pgOrient == ReverseLandscape)
        {
            INT countSwap = m_CanvasXCount;
            m_CanvasXCount = m_CanvasYCount;
            m_CanvasYCount = countSwap;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = InitTransformMap();
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}


/*++

Routine Name:

    CNUpTransform::~CNUpTransform

Routine Description:

    Default destructor for the CNUpTransform class

Arguments:

    None

Return Value:

    None

--*/
CNUpTransform::~CNUpTransform()
{
}

/*++

Routine Name:

    CNUpTransform::SetCurrentPage

Routine Description:

    Sets the page member of the object

Arguments:

    cPage - Current page count around the nup page count.

Return Value:

    None

--*/
VOID
CNUpTransform::SetCurrentPage(
    _In_ UINT cPage
    )
{
    m_cCurrPageIndex = cPage;
}

/*++

Routine Name:

    CNUpTransform::GetPageTransform

Routine Description:

    Method for obtaining a page transformation matrix based on the count of the
    current canvas and the dimensions of the source and target pages

Arguments:

    sizePage   - Size of the source page being transformed
    pTransform - Pointer to the resulting transformation matrix

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpTransform::GetPageTransform(
    _In_  SizeF   sizePage,
    _Out_ Matrix* pTransform
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pTransform, E_POINTER)))
    {
        SizeF  sizeCanvas(m_sizeTargetPage.Width / m_CanvasXCount, m_sizeTargetPage.Height / m_CanvasYCount);
        PointF canvasCentre(sizeCanvas.Width/2.0f, sizeCanvas.Height/2.0f);
        PointF pageCentre(sizePage.Width/2.0f, sizePage.Height/2.0f);

        REAL scaleX = 1.0f;
        REAL scaleY = 1.0f;
        if (m_bRotatePage)
        {
            //
            // Calculate the dimension scale accounting for page rotation
            //
            scaleX = sizeCanvas.Width / sizePage.Height;
            scaleY = sizeCanvas.Height / sizePage.Width;

            //
            // Apply the rotation
            //
            pTransform->Rotate(90.0f, MatrixOrderAppend);
        }
        else
        {
            //
            // Calculate the dimension scale
            //
            scaleX = sizeCanvas.Width / sizePage.Width;
            scaleY = sizeCanvas.Height / sizePage.Height;
        }

        //
        // The minimum scale of x and y gurantees both
        // dimensions fit the target
        //
        REAL scale  = min(scaleX, scaleY);

        //
        // Apply the scaling factor
        //
        pTransform->Scale(scale, scale, MatrixOrderAppend);

        //
        // Find the new page centre
        //
        pTransform->TransformPoints(&pageCentre, 1);

        //
        // Apply a translate to the canvas
        //
        UINT cPage = 0;
        BOOL bDone = FALSE;

        for (UINT cIndexY = 0; cIndexY < m_CanvasYCount && !bDone; cIndexY++)
        {
            for (UINT cIndexX = 0; cIndexX < m_CanvasXCount && !bDone; cIndexX++)
            {
                if (m_pageNumVect[cPage] == m_cCurrPageIndex)
                {
                    //
                    // Offset multipliers from loop indices
                    //
                    REAL yMult = static_cast<REAL>(cIndexY);
                    REAL xMult = static_cast<REAL>(cIndexX);

                    //
                    // Use a matrix to find the centre of the translated canvas
                    //
                    Matrix canvasXForm;

                    //
                    // Translate canvas to destination
                    //
                    canvasXForm.Translate(sizeCanvas.Width * xMult, sizeCanvas.Height * yMult, MatrixOrderAppend);

                    //
                    // Apply the canvas transform to the canvas centre
                    //
                    canvasXForm.TransformPoints(&canvasCentre, 1);

                    bDone = TRUE;
                }
                cPage++;
            }
        }

        //
        // Calculate the offset of the centre of page to the centre
        // of the canvas and apply the translation
        //
        PointF offset(canvasCentre - pageCentre);
        pTransform->Translate(offset.X, offset.Y, MatrixOrderAppend);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpTransform::MatrixToXML

Routine Description:

    Method to create xml markup representing a transformation matrix

Arguments:

    pMatrix          - Pointer to a transformation matrix to convert to markup
    pbstrMatrixXForm - Pointer to a string to contain the matrix markup

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpTransform::MatrixToXML(
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
        *pbstrMatrixXForm = NULL;
        REAL matElems[6];

        if (Ok == pMatrix->GetElements(matElems))
        {
            CStringXDW cstrMatrix;

            try
            {
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

    CNUpTransform::InitTransformMap

Routine Description:

    Method to initialise the map of transforms which depends
    on the nup page count and presentation direction

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpTransform::InitTransformMap(
    VOID
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the map of page numbers.
    // The transforms are calculated left to right, top to bottom on
    // the physical page so rather than special case each presentation
    // order, we simply apply the same transform in the same order but
    // to a different ordering of pages
    //
    try
    {
        //
        // Certain NUp page counts cause a rotation to be applied to the page.
        // When this occurs, the presentation order should also be rotated.
        //
        ENUpDirectionOption rotatedOrder[ENUpDirectionOptionMax] = {
            BottomLeft,
            LeftBottom,
            TopLeft,
            LeftTop,
            BottomRight,
            RightBottom,
            TopRight,
            RightTop
        };

        if (m_bRotatePage)
        {
            if (m_NUpPresentDirection >= ENUpDirectionOptionMin &&
                m_NUpPresentDirection <  ENUpDirectionOptionMax)
            {
                m_NUpPresentDirection = rotatedOrder[m_NUpPresentDirection];
            }
            else
            {
                ERR("Invalid presentation direction.\n");

                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            m_pageNumVect.clear();
            for (UINT cIndexY = 0; cIndexY < m_CanvasYCount; cIndexY++)
            {
                for (UINT cIndexX = 0; cIndexX < m_CanvasXCount; cIndexX++)
                {
                    switch (m_NUpPresentDirection)
                    {
                        case RightTop:
                        {
                            m_pageNumVect.push_back((m_CanvasXCount * (m_CanvasYCount-1)) - (m_CanvasXCount * cIndexY) + cIndexX);
                        }
                        break;

                        case LeftBottom:
                        {
                            m_pageNumVect.push_back(cIndexY * m_CanvasXCount + (m_CanvasXCount - cIndexX) - 1);
                        }
                        break;

                        case LeftTop:
                        {
                            m_pageNumVect.push_back((m_CanvasXCount * m_CanvasYCount) - cIndexX - (cIndexY*m_CanvasXCount) - 1);
                        }
                        break;

                        case BottomRight:
                        {
                            m_pageNumVect.push_back(m_CanvasYCount * cIndexX + cIndexY);
                        }
                        break;

                        case TopRight:
                        {
                            m_pageNumVect.push_back((m_CanvasYCount - 1 - cIndexY) + (cIndexX * m_CanvasYCount));
                        }
                        break;

                        case BottomLeft:
                        {
                            m_pageNumVect.push_back(m_CanvasYCount * (m_CanvasXCount - 1 - cIndexX) + cIndexY);
                        }
                        break;

                        case TopLeft:
                        {
                            m_pageNumVect.push_back(((m_CanvasXCount-cIndexX) * m_CanvasYCount) - 1 - cIndexY);
                        }
                        break;

                        case RightBottom:
                        default:
                        {
                            m_pageNumVect.push_back(cIndexY * m_CanvasXCount + cIndexX);
                        }
                        break;
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

