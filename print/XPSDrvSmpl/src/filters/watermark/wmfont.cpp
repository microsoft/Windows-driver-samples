/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmfont.cpp

Abstract:

   Watermark font implementation. The CWatermarkFont class is responsible
   for managing the font resource for a text watermark. This implements
   the IResWriter interface so that the font can be added to the resource
   cache.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmfont.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::TextWatermark;

/*++

Routine Name:

    CWatermarkFont::CWatermarkFont

Routine Description:

    Constructor for the CWatermarkFont font management class which
    sets the member variables to sensible defaults and ensures the
    current watermark which uses this font is a text based watermark

Arguments:

    wmProps - Object containing the PrintTicket settings

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWatermarkFont::CWatermarkFont(
    _In_ CONST CWMPTProperties& wmProps
     ) :
    m_hDC(CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL)),
    m_WMProps(wmProps),
    m_hFont(NULL),
    m_hOldFont(NULL),
    m_bstrFaceName(L"Arial")
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_hDC != NULL, "NULL DC passed.\n");

    EWatermarkOption wmType;

    if (SUCCEEDED(hr = m_WMProps.GetType(&wmType)))
    {
        if (wmType == TextWatermark)
        {
            hr = SetFont();
        }
        else
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

    CWatermarkFont::~CWatermarkFont

Routine Description:

    Default destructor for the font management class

Arguments:

    None

Return Value:

    None

--*/
CWatermarkFont::~CWatermarkFont()
{
    UnsetFont();

    if (SUCCEEDED(CHECK_HANDLE(m_hDC, E_PENDING)))
    {
        DeleteDC(m_hDC);
    }
}

/*++

Routine Name:

    CWatermarkFont::WriteData

Routine Description:

    Method for writing out the font to the container

Arguments:

    pStream - Pointer to the stream to write the font out to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkFont::WriteData(
    _In_ IPartBase*         pResource,
    _In_ IPrintWriteStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pResource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)))
    {
        //
        // Find the size of the font data
        //
        DWORD cbFontData = GetFontData(m_hDC, 0, 0, 0, 0);

        if (cbFontData != GDI_ERROR)
        {
            PBYTE pFontData = new(std::nothrow) BYTE[cbFontData];

            if (SUCCEEDED(hr = CHECK_POINTER(pFontData, E_OUTOFMEMORY)))
            {
                //
                // Retrieve the font data
                //
                if (GDI_ERROR != GetFontData(m_hDC,
                                             0,
                                             0,
                                             reinterpret_cast<LPVOID>(pFontData),
                                             cbFontData))
                {
                    //
                    // SetFontOptions(Font_Obfusticate) sets the appropriate content type
                    // The pipeline takes care of XORing the font data with the URI GUID
                    //
                    CComQIPtr<IPartFont> pFont = pResource;
                    if (SUCCEEDED(hr = CHECK_POINTER(pFont, E_NOINTERFACE)) &&
                        SUCCEEDED(hr = pFont->SetFontOptions(Font_Obfusticate)))
                    {
                        //
                        // Write font data to stream
                        //
                        ULONG cbWritten = 0;
                        hr = pStream->WriteBytes(reinterpret_cast<LPVOID>(pFontData),
                                                 cbFontData,
                                                 &cbWritten);

                        ASSERTMSG(cbFontData == cbWritten, "Failed to write all font data.\n");
                    }
                }
                else
                {
                    hr = GetLastErrorAsHResult();
                }

                delete[] pFontData;
                pFontData = NULL;
            }
        }
        else
        {
            hr = GetLastErrorAsHResult();
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkFont::SetFont

Routine Description:

    Method to select a font into the device context

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkFont::SetFont(
    VOID
    )
{
    ASSERTMSG(m_hFont == NULL, "Non NULL font handle when setting new font. Possible resource leak.\n");
    ASSERTMSG(m_hOldFont == NULL, "Non NULL old font handle when setting new font. Possible resource leak.\n");

    HRESULT hr = S_OK;

    INT      fontSize;

    if (SUCCEEDED(hr = m_WMProps.GetFontSize(&fontSize)))
    {
        LOGFONTW logfont;

        logfont.lfHeight = -MulDiv(fontSize, GetDeviceCaps(m_hDC, LOGPIXELSY), 72);
        logfont.lfWidth = 0;
        logfont.lfEscapement = 0;
        logfont.lfOrientation = 0;
        logfont.lfWeight = FW_NORMAL;
        logfont.lfItalic = FALSE;
        logfont.lfUnderline = FALSE;
        logfont.lfStrikeOut = FALSE;
        logfont.lfCharSet = ANSI_CHARSET;
        logfont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
        logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        logfont.lfQuality = CLEARTYPE_QUALITY;
        logfont.lfPitchAndFamily = FF_DONTCARE;

        size_t cchSrc = 0;
        if (SUCCEEDED(hr = StringCchLength(m_bstrFaceName, LF_FACESIZE, &cchSrc)) &&
            SUCCEEDED(hr = StringCchCopyN(logfont.lfFaceName, LF_FACESIZE, m_bstrFaceName, cchSrc)))
        {
            if (m_hFont != NULL)
            {
                UnsetFont();
            }

            m_hFont = CreateFontIndirect(&logfont);

            if (m_hFont != NULL)
            {
                m_hOldFont = SelectFont(m_hDC, m_hFont);
            }
            else
            {
                hr = GetLastErrorAsHResult();
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkFont::UnsetFont

Routine Description:

    Method de-select a font from the current device context

Arguments:

    None

Return Value:

    None

--*/
VOID
CWatermarkFont::UnsetFont(
    VOID
    )
{
    ASSERTMSG(m_hFont != NULL, "Attempting to deselect invalid font.\n");
    ASSERTMSG(m_hOldFont != NULL, "Attempting to select invalid font.\n");

    SelectFont(m_hDC, m_hOldFont);
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

/*++

Routine Name:

    CWatermarkFont::GetKeyName

Routine Description:

    Method to obtain a unique key for the stored font based on the resource name

Arguments:

    pbstrKeyName - Pointer to the string to contain the generated key name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkFont::GetKeyName(
    _Outptr_ BSTR* pbstrKeyName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrKeyName, E_POINTER)))
    {
        //
        // The name of the resource is a suitable key
        //
        if (SUCCEEDED(hr = m_bstrFaceName.CopyTo(pbstrKeyName)) &&
            !*pbstrKeyName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkFont::GetResURI

Routine Description:

    Method to obtain the URI to the stored font

Arguments:

    pbstrResURI - Pointer to the string to contain the font resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkFont::GetResURI(
    _Outptr_ BSTR* pbstrResURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrResURI, E_POINTER)))
    {
        *pbstrResURI = NULL;

        try
        {
            //
            // Create an obfuscated font name using our font GUID
            //
            CStringXDW cstrURI(L"Resources/Fonts/78F47176-ADD7-0E49-AB3A-C59F137240AC.odttf");
            *pbstrResURI = cstrURI.AllocSysString();
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

