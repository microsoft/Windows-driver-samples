/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmimg.cpp

Abstract:

   Watermark image implementation. The CWatermarkImage class is responsible
   for managing the image resource for a raster watermark. This implements
   the IResWriter interface so that the font can be added to the resource
   cache.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmimg.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::BitmapWatermark;

/*++

Routine Name:

    CWatermarkImage::CWatermarkImage

Routine Description:

    Constructor for the CWatermarkFont bitmap management class which
    sets the member variables to sensible defaults and ensures the
    current watermark which uses this bitmap is a bitmap based watermark

Arguments:

    wmProps    - Object containing the PrintTicket settings
    resourceID - Resource ID for the watermark
Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWatermarkImage::CWatermarkImage(
    _In_ CONST CWMPTProperties& wmProps,
    _In_ CONST INT              resourceID
    ) :
    m_WMProps(wmProps),
    m_resourceID(resourceID),
    m_pPNGData(NULL),
    m_pPNGStream(NULL),
    m_hPNGRes(NULL),
    m_cbPNGData(0)
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

    CWatermarkImage::~CWatermarkImage

Routine Description:

    Default destructor for the font management class

Arguments:

    None

Return Value:

    None

--*/
CWatermarkImage::~CWatermarkImage()
{
    if (m_hPNGRes != NULL)
    {
        FreeResource(m_hPNGRes);
        m_hPNGRes = NULL;
    }
}

/*++

Routine Name:

    CWatermarkImage::GetImageDimensions

Routine Description:

    Method to obtain the width and height for the watermark bitmap

Arguments:

    pDimensions - Pointer to a structure which will hold the bitmap dimensions

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::GetImageDimensions(
    _Out_ SizeF* pDimensions
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDimensions, E_POINTER)) &&
        SUCCEEDED(hr = CreatePNGStream()))
    {
        pDimensions->Width = 0;
        pDimensions->Height = 0;

        Bitmap image(m_pPNGStream);
        Status gdiPStat = image.GetLastStatus();

        if (gdiPStat == Ok)
        {
            pDimensions->Width  = (REAL)image.GetWidth();
            pDimensions->Height = (REAL)image.GetHeight();

            REAL horzRes = image.GetHorizontalResolution();
            REAL vertRes = image.GetVerticalResolution();

            if (horzRes > 0 &&
                vertRes > 0)
            {
                pDimensions->Width  *= 96.0f / horzRes;
                pDimensions->Height *= 96.0f / vertRes;
            }
        }
        else
        {
            hr = GetGDIStatusErrorAsHResult(gdiPStat);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkImage::WriteData

Routine Description:

    Method for writing out the bitmap to the container

Arguments:

    pStream - Pointer to the stream to write the bitmap out to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::WriteData(
    _In_ IPartBase*         pResource,
    _In_ IPrintWriteStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pResource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)))
    {
        if (SUCCEEDED(hr = LoadPNGResource()))
        {
            ULONG cbWritten = 0;

            hr = pStream->WriteBytes(m_pPNGData, m_cbPNGData, &cbWritten);

            ASSERTMSG(m_cbPNGData == cbWritten, "Failed to write all data.\n");

            //
            // Set the content type of the image part
            //
            CComQIPtr<IPartImage> pImage = pResource;
            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = CHECK_POINTER(pImage, E_NOINTERFACE)))
            {
                hr = pImage->SetImageContent(CComBSTR(L"image/png"));
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkImage::GetKeyName

Routine Description:

    Method to obtain a unique key for the stored bitmap based on the resource name

Arguments:

    pbstrKeyName - Pointer to the string to contain the generated key name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::GetKeyName(
    _Outptr_ BSTR* pbstrKeyName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrKeyName, E_POINTER)))
    {
        try
        {
            //
            // The id of the resource is a suitable key
            //

            CStringXDW cstrKeyName;
            cstrKeyName.Format(L"%d", m_resourceID);

            *pbstrKeyName = cstrKeyName.AllocSysString();
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

    CWatermarkImage::GetResURI

Routine Description:

    Method to obtain the URI to the stored bitmap

Arguments:

    pbstrResURI - Pointer to the string to contain the bitmap resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::GetResURI(
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
            // Create a unique name for the watermark bitmap for this print session
            //
            CStringXDW cstrURI;
            cstrURI.Format(L"/WM_%d_%u.png", m_resourceID, GetUniqueNumber());

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

/*++

Routine Name:

    CWatermarkImage::CheckResID

Routine Description:

    Method to check that the bitmap resource exists

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::CheckResID(
    VOID
    )
{
    HRESULT hr = S_OK;

    HRSRC hrSrc = FindResourceEx(g_hInstance,
                                 RT_RCDATA,
                                 MAKEINTRESOURCE(m_resourceID),
                                 MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));

    if (hrSrc == NULL)
    {
        hr = GetLastErrorAsHResult();
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkImage::CreatePNGStream

Routine Description:

    Method to write out the PNG bitmap to the container

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::CreatePNGStream(
    VOID
    )
{
    HRESULT hr = S_OK;

    //
    // If a stream hasn't already been created, load the PNG image resource
    // and create a new stream
    //
    if (m_pPNGStream == NULL &&
        SUCCEEDED(hr = LoadPNGResource()) &&
        SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pPNGStream)))
    {
        ULONG cbWritten = 0;

        //
        // Write the data to the stream
        //
        hr = m_pPNGStream->Write(m_pPNGData, m_cbPNGData, &cbWritten);
    }

    //
    // Make sure the stream is pointing back at the start of the data
    //
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER cbMoveFromStart = {0};

        hr = m_pPNGStream->Seek(cbMoveFromStart, STREAM_SEEK_SET, NULL);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkImage::LoadPNGResource

Routine Description:

    Method to load the PNG bitmap resource

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkImage::LoadPNGResource(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (m_pPNGData == NULL)
    {
        HRSRC hrSrc = FindResourceEx(g_hInstance,
                                     RT_RCDATA,
                                     MAKEINTRESOURCE(m_resourceID),
                                     MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
        if (hrSrc != NULL)
        {
            //
            // Load the resource
            //
            m_hPNGRes = LoadResource(g_hInstance, hrSrc);

            if (m_hPNGRes != NULL)
            {
                //
                // Retrieve the PNG data and the size of the data
                //
                m_pPNGData = LockResource(m_hPNGRes);
                m_cbPNGData = SizeofResource(g_hInstance, hrSrc);

                if (m_pPNGData == NULL ||
                    m_cbPNGData <= 0)
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = GetLastErrorAsHResult();
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

