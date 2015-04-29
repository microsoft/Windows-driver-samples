/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmptprop.cpp

Abstract:

   Watermark properties class implementation. The Watermark properties class
   is responsible for holding and controling Watermark properties.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmptprop.h"

using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::NoWatermark;
using XDPrintSchema::PageWatermark::TextWatermark;
using XDPrintSchema::PageWatermark::BitmapWatermark;
using XDPrintSchema::PageWatermark::VectorWatermark;
using XDPrintSchema::PageWatermark::Layering::ELayeringOption;

/*++

Routine Name:

    CWMPTProperties::CWMPTProperties

Routine Description:

    Constructor for the CWMPTProperties PrintTicket properties class

Arguments:

    wmData - Structure containing watermark properties read from the PrintTicket

Return Value:

    None

--*/
CWMPTProperties::CWMPTProperties(
    _In_ CONST WatermarkData& wmData
    ) :
    m_wmData(wmData)
{
}

/*++

Routine Name:

    CWMPTProperties::~CWMPTProperties

Routine Description:

    Default destructor for the CWMPTProperties class

Arguments:

    None

Return Value:

    None

--*/
CWMPTProperties::~CWMPTProperties()
{
}

/*++

Routine Name:

    CWMPTProperties::GetType

Routine Description:

    Method to obtain the watermark type to identify as vector, text or bitmap

Arguments:

    pType - Enumerated type to indicate the watermark type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetType(
    _Out_ EWatermarkOption* pType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pType, E_POINTER)))
    {
        *pType = m_wmData.type;

        //
        // If we are a bitmap or vector watermark and the extents are zero, report no watermark
        //
        if (m_wmData.type == BitmapWatermark ||
            m_wmData.type == VectorWatermark)
        {
            if (m_wmData.widthExtent == 0 ||
                m_wmData.heightExtent == 0)
            {
                *pType = NoWatermark;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetLayering

Routine Description:

    Method to obtain the watermark layering type

Arguments:

    pLayering - Enumerated type to indicate the watermark layering type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetLayering(
    _Out_ ELayeringOption* pLayering
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pLayering, E_POINTER)))
    {
        *pLayering = m_wmData.layering;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetBounds

Routine Description:

    Method to obtain the watermark bounding area

Arguments:

    pBounds - Variable which is set to the watermark bounding area

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetBounds(
    _Out_ RectF* pBounds
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBounds, E_POINTER)))
    {
        //
        // Convert from microns to 96ths of an inch
        //
        pBounds->X = static_cast<REAL>(m_wmData.widthOrigin)/k96thInchAsMicrons;
        pBounds->Y = static_cast<REAL>(m_wmData.heightOrigin)/k96thInchAsMicrons;
        pBounds->Width  = static_cast<REAL>(m_wmData.widthExtent)/k96thInchAsMicrons;
        pBounds->Height = static_cast<REAL>(m_wmData.heightExtent)/k96thInchAsMicrons;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetOrigin

Routine Description:

    Method to obtain the watermark origin

Arguments:

    pOrigin - Variable which is set to the watermark origin

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetOrigin(
    _Out_ PointF* pOrigin
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOrigin, E_POINTER)))
    {
        //
        // Convert from microns to 96ths of an inch
        //
        pOrigin->X = static_cast<REAL>(m_wmData.widthOrigin)/k96thInchAsMicrons;
        pOrigin->Y = static_cast<REAL>(m_wmData.heightOrigin)/k96thInchAsMicrons;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetAngle

Routine Description:

    Method to obtain the watermark angle

Arguments:

    pAngle - Variable which is set to the watermark angle

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetAngle(
    _Out_ REAL* pAngle
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pAngle, E_POINTER)))
    {
        *pAngle = static_cast<REAL>(m_wmData.angle);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetTransparency

Routine Description:

    Method to obtain the watermark transparency value

Arguments:

    pTransparency - Variable which is set to the watermark transparency value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetTransparency(
    _Out_ INT* pTransparency
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pTransparency, E_POINTER)))
    {
        *pTransparency = m_wmData.transparency;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetOpacity

Routine Description:

    Method to obtain the watermark opacity value

Arguments:

    pOpacity - Variable which is set to the watermark opacity value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetOpacity(
    _Out_ REAL* pOpacity
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOpacity, E_POINTER)))
    {
        *pOpacity = 1.0f - (m_wmData.transparency / 100.0f);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetTransparency

Routine Description:

    Method to obtain the watermark transparency value as a string

Arguments:

    pbstrTransparency - String which is set to contain the watermark transparency value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetTransparency(
    _Outptr_ BSTR* pbstrTransparency
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrTransparency, E_POINTER)))
    {
        *pbstrTransparency = NULL;
        INT transparency = 0;
        if (SUCCEEDED(hr = GetTransparency(&transparency)))
        {
            try
            {
                CStringXDW szTransparency;
                szTransparency.Format(L"%i", transparency);

                *pbstrTransparency = szTransparency.AllocSysString();
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

    CWMPTProperties::GetOpacity

Routine Description:

    Method to obtain the watermark opacity value as a string

Arguments:

    pbstrOpacity - String which is set to contain the watermark opacity value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetOpacity(
    _Outptr_ BSTR* pbstrOpacity
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrOpacity, E_POINTER)))
    {
        *pbstrOpacity = NULL;
        REAL opacity = 0;
        if (SUCCEEDED(hr = GetOpacity(&opacity)))
        {
            try
            {
                CStringXDW szOpacity;
                szOpacity.Format(L"%.2f", opacity);

                *pbstrOpacity = szOpacity.AllocSysString();
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

    CWMPTProperties::GetFontColor

Routine Description:

    Method to obtain the watermark font color as a string

Arguments:

    pbstrColor - String which is set to contain the watermark font color. Any BSTR
    this points to will be freed.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetFontColor(
    _Inout_ _At_(*pbstrColor, _Pre_maybenull_ _Post_valid_) BSTR* pbstrColor
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrColor, E_POINTER)))
    {
        SysFreeString(*pbstrColor);
        *pbstrColor = NULL;
        if (m_wmData.type == TextWatermark)
        {
            if (SUCCEEDED(hr = m_wmData.txtData.bstrFontColor.CopyTo(pbstrColor)) &&
                !*pbstrColor)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetText

Routine Description:

    Method to obtain the watermark text as a string

Arguments:

    pbstrText - String which is set to contain the watermark text

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetText(
    _Inout_ _At_(*pbstrText, _Pre_maybenull_ _Post_valid_) BSTR* pbstrText
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrText, E_POINTER)))
    {
        SysFreeString(*pbstrText);
        *pbstrText = NULL;
        if (m_wmData.type == TextWatermark)
        {
            if (SUCCEEDED(hr = m_wmData.txtData.bstrText.CopyTo(pbstrText)) &&
                !*pbstrText)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetFontSize

Routine Description:

    Method to obtain the watermark font size

Arguments:

    pSize - Variable which is set to contain the watermark font size

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetFontSize(
    _Out_ INT* pSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSize, E_POINTER)))
    {
        if (m_wmData.type == TextWatermark)
        {
            *pSize = m_wmData.txtData.fontSize;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWMPTProperties::GetFontEmSize

Routine Description:

    Method to obtain the watermark font EM size as a string

Arguments:

    pbstrSize - String which is set to contain the watermark EM font size

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWMPTProperties::GetFontEmSize(
    _Outptr_ BSTR* pbstrSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrSize, E_POINTER)))
    {
        *pbstrSize = NULL;

        if (m_wmData.type != TextWatermark)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    if (SUCCEEDED(hr))
    {
        INT size = 0;

        if (SUCCEEDED(hr = GetFontSize(&size)))
        {
            //
            // Convert from 72nds to 96ths of an inch
            //
            size = MulDiv(size, 96, 72);

            try
            {
                CStringXDW szFontSize;
                szFontSize.Format(L"%d", size);

                *pbstrSize = szFontSize.AllocSysString();
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

