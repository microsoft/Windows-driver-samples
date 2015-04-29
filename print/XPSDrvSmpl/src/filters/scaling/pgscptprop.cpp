/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscptprop.cpp

Abstract:

   Page Scaling properties class implementation. The Page Scaling properties class
   is responsible for holding and controling Page Scaling properties.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "pgscptprop.h"

using XDPrintSchema::PageScaling::PageScalingData;
using XDPrintSchema::PageScaling::EScaleOption;
using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOption;

using XDPrintSchema::PageOrientation::PageOrientationData;
using XDPrintSchema::PageOrientation::Landscape;
using XDPrintSchema::PageOrientation::ReverseLandscape;

using XDPrintSchema::PageMediaSize::PageMediaSizeData;

using XDPrintSchema::PageImageableSize::PageImageableData;

/*++

Routine Name:

    CPGSCPTProperties::CPGSCPTProperties

Routine Description:

    CPGSCPTProperties class constructor

Arguments:

    pgscData - reference to the Page Scaling data.
    pSizeData - reference to the Page Size data.
    pImageableData - reference to the Page Imageable Size data.

Return Value:

    None

--*/
CPGSCPTProperties::CPGSCPTProperties(
    _In_ CONST PageScalingData& pgscData,
    _In_ CONST PageMediaSizeData& pSizeData,
    _In_ CONST PageImageableData& pImageableData,
    _In_ CONST PageOrientationData& pageOrientData
    ) :
    m_pgscData(pgscData),
    m_pageMediaSizeData(pSizeData),
    m_pageImageableData(pImageableData),
    m_pageOrientData(pageOrientData)
{
}

/*++

Routine Name:

    CPGSCPTProperties::~CPGSCPTProperties

Routine Description:

    CPGSCPTProperties class destructor

Arguments:

    None

Return Value:

    None

--*/
CPGSCPTProperties::~CPGSCPTProperties()
{
}

/*++

Routine Name:

    CPGSCPTProperties::GetOption

Routine Description:

    Used to obtain the current page scaling option.

Arguments:

    pOption - Pointer to the page scaling option to filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetOption(
    _Out_ EScaleOption* pOption
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOption, E_POINTER)))
    {
        *pOption = m_pgscData.pgscOption;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetOffsetOption

Routine Description:

    Used to obtain the current page scaling offset option.

Arguments:

    pOffsetOption - Pointer to the page scaling offset option to filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetOffsetOption(
    _Out_ EScaleOffsetOption* pOffsetOption
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOffsetOption, E_POINTER)))
    {
        *pOffsetOption = m_pgscData.offsetOption;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetWidthOffset

Routine Description:

    Used to obtain the current width offset (Only applies to the custom scaling options).
    Units are in 1/96 Inch.

Arguments:

    pWidthOffset - Pointer to the width offset data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetWidthOffset(
    _Out_ REAL* pWidthOffset
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWidthOffset, E_POINTER)))
    {
        *pWidthOffset = static_cast<REAL>(m_pgscData.offWidth)/k96thInchAsMicrons;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetHeightOffset

Routine Description:

    Used to obtain the current height offset (Only applies to the custom scaling options).
    Units are in 1/96 Inch.

Arguments:

    pHeightOffset - Pointer to the height offset data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetHeightOffset(
    _Out_ REAL* pHeightOffset
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pHeightOffset, E_POINTER)))
    {
        *pHeightOffset = static_cast<REAL>(m_pgscData.offHeight)/k96thInchAsMicrons;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetWidthScale

Routine Description:

    Used to obtain the current width scale percentage
    (Only applies to the custom scaling options).

Arguments:

    pWidthScale - Pointer to the width scale data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetWidthScale(
    _Out_ REAL* pWidthScale
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWidthScale, E_POINTER)))
    {
        *pWidthScale = static_cast<REAL>(m_pgscData.scaleWidth)/100.00f;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetHeightScale

Routine Description:

    Used to obtain the current height scale percentage
    (Only applies to the custom scaling options).

Arguments:

    pHeightScale - Pointer to the height scale data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetHeightScale(
    _Out_ REAL* pHeightScale
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pHeightScale, E_POINTER)))
    {
        *pHeightScale = static_cast<REAL>(m_pgscData.scaleHeight)/100.00f;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetPageSize

Routine Description:

    Used to obtain the current page size. Units are in 1/96 Inch.

    Note: This returns the page dimensions accounting for the orientation and media size
    expressed by the PrintTicket as PageOrientation and PageMediaSize. The PageMediaSize
    expresses the dimensions using the convention that the page is oriented as portrait.
    To get the dimensions for scaling we need to swap width for height when in landscape
    so that we can scale the FixedPage content appropriately.

Arguments:

    pSizePage - Pointer to the page size data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetPageSize(
    _Out_ SizeF* pSizePage
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSizePage, E_POINTER)))
    {
        //
        // Convert microns to 96th of an inch
        //
        pSizePage->Width  = static_cast<REAL>(m_pageMediaSizeData.pageWidth)/k96thInchAsMicrons;
        pSizePage->Height = static_cast<REAL>(m_pageMediaSizeData.pageHeight)/k96thInchAsMicrons;

        //
        // Swap dimensions if the orientation is a landscape orientation
        //
        if (m_pageOrientData.orientation == Landscape ||
            m_pageOrientData.orientation == ReverseLandscape)
        {
            REAL sizeSwap = pSizePage->Width;
            pSizePage->Width = pSizePage->Height;
            pSizePage->Height = sizeSwap;
        }

        if (pSizePage->Width <= 0.0f ||
            pSizePage->Height <= 0.0f)
        {
            //
            // The page media size is incorrect
            //
            ERR("Could not acquire a valid media page size\n");

            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPGSCPTProperties::GetImageableRect

Routine Description:

    Used to obtain the current page imageable area and offset.
    Units are in 1/96 Inch.

Arguments:

    pImageableRect - Pointer to the page imageable data to be filled out.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPGSCPTProperties::GetImageableRect(
    _Out_ RectF* pImageableRect
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pImageableRect, E_POINTER)))
    {
        pImageableRect->X = 0;
        pImageableRect->Y = 0;
        pImageableRect->Width  = 0;
        pImageableRect->Height = 0;

        //
        // Get the size of the area - convert microns to 96th of an inch
        //
        SizeF sizeImage;
        if (m_pageImageableData.imageableSizeWidth  > 0 &&
            m_pageImageableData.imageableSizeHeight > 0)
        {
            sizeImage.Width  = static_cast<REAL>(m_pageImageableData.imageableSizeWidth)/k96thInchAsMicrons;
            sizeImage.Height = static_cast<REAL>(m_pageImageableData.imageableSizeHeight)/k96thInchAsMicrons;
        }
        else
        {
            //
            // Page imageable size was not set - use the page media size
            //
            sizeImage.Width  = static_cast<REAL>(m_pageMediaSizeData.pageWidth)/k96thInchAsMicrons;
            sizeImage.Height = static_cast<REAL>(m_pageMediaSizeData.pageHeight)/k96thInchAsMicrons;
        }

        //
        // Get the offset of the area - convert microns to 96th of an inch
        //
        PointF offsetImage(static_cast<REAL>(m_pageImageableData.originWidth)/k96thInchAsMicrons,
                           static_cast<REAL>(m_pageImageableData.originHeight)/k96thInchAsMicrons);

        //
        // Swap dimensions if the orientation is a landscape orientation
        //
        if (m_pageOrientData.orientation == Landscape ||
            m_pageOrientData.orientation == ReverseLandscape)
        {
            pImageableRect->Width  = sizeImage.Height;
            pImageableRect->Height = sizeImage.Width;

            pImageableRect->X = offsetImage.Y;
            pImageableRect->Y = offsetImage.X;
        }
        else
        {
            pImageableRect->Width  = sizeImage.Width;
            pImageableRect->Height = sizeImage.Height;

            pImageableRect->X = offsetImage.X;
            pImageableRect->Y = offsetImage.Y;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

