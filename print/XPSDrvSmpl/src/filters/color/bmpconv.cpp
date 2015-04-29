/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bmpconv.cpp

Abstract:

   WIC bitmap conversion class implementation. This class provides a wrapper to a bitmap
   stream that uses WIC to access bitmap data and provide conversion functionality.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "bmpconv.h"

/*++

Routine Name:

    CBmpConverter::CBmpConverter

Routine Description:

    CBmpConverter default constructor

Arguments:

    None

Return Value:

    None
    Throws an exception on error.

--*/
CBmpConverter::CBmpConverter() :
    m_pImagingFactory(NULL),
    m_pBitmap(NULL),
    m_pColorContext(NULL),
    m_pCurrentLock(NULL),
    m_ePixelFormat(kWICPixelFormatDontCare)
{
    //
    // Make sure we have an imaging factory to work with
    //
    HRESULT hr = CreateImagingFactory();
    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CBmpConverter::CBmpConverter

Routine Description:

    CBmpConverter constructor

Arguments:

    ePixFormat - Bitmap pixel format
    cWidth     - Bitmap width
    cHeight    - Bitmap height
    dpiX       - Bitmap horizontal resolution
    dpiY       - Bitmap vertical resolution

Return Value:

    None
    Throws an exception on error.

--*/
CBmpConverter::CBmpConverter(
    _In_ CONST EWICPixelFormat& ePixFormat,
    _In_ CONST UINT&            cWidth,
    _In_ CONST UINT&            cHeight,
    _In_ CONST DOUBLE&          dpiX,
    _In_ CONST DOUBLE&          dpiY
    ) :
    m_pImagingFactory(NULL),
    m_pBitmap(NULL),
    m_pColorContext(NULL),
    m_pCurrentLock(NULL),
    m_ePixelFormat(ePixFormat)
{
    HRESULT hr = S_OK;

    //
    // Make sure we have an imaging factory to work with...
    //
    if (SUCCEEDED(hr = CreateImagingFactory()))
    {
        //
        // ...and that the underlying bitmap is created and intialized.
        //
        hr = Initialize(m_ePixelFormat, cWidth, cHeight, dpiX, dpiY);
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CBmpConverter::CBmpConverter

Routine Description:

    CBmpConverter constructor

Arguments:

    pStream - Stream containing the bitmap container data

Return Value:

    None
    Throws an exception on error.

--*/
CBmpConverter::CBmpConverter(
    _In_ IStream* pStream
    ) :
    m_pImagingFactory(NULL),
    m_pBitmap(NULL),
    m_pColorContext(NULL),
    m_pCurrentLock(NULL),
    m_ePixelFormat(kWICPixelFormatDontCare)
{
    HRESULT hr = S_OK;

    //
    // Make sure we have an imaging factory to work with...
    //
    if (SUCCEEDED(hr = CreateImagingFactory()))
    {
        //
        // ...and that the underlying bitmap is created and intialized from the input stream.
        //
        hr = Initialize(pStream);
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CBmpConverter::CBmpConverter

Routine Description:

    CBmpConverter constructor

Arguments:

    converter - Converter class to construct from

Return Value:

    None
    Throws an exception on error.

--*/
CBmpConverter::CBmpConverter(
    _In_ CONST CBmpConverter& converter
    ) :
    m_pImagingFactory(converter.m_pImagingFactory),
    m_pBitmap(converter.m_pBitmap),
    m_pColorContext(converter.m_pColorContext),
    m_pCurrentLock(converter.m_pCurrentLock),
    m_ePixelFormat(converter.m_ePixelFormat)
{
    ASSERTMSG(m_pCurrentLock == NULL, "Copying locked bitmap can lead to a deadlock when writing out the bitmap.\n");
}

/*++

Routine Name:

    CBmpConverter::~CBmpConverter

Routine Description:

    CBmpConverter destructor

Arguments:

    None

Return Value:

    None

--*/
CBmpConverter::~CBmpConverter()
{
}

/*++

Routine Name:

    CBmpConverter::Initialize

Routine Description:

    Initializes the bitmap given format, dimensions and resolution

Arguments:

    ePixFormat - Bitmap pixel format
    cWidth     - Bitmap width
    cHeight    - Bitmap height
    dpiX       - Bitmap horizontal resolution
    dpiY       - Bitmap vertical resolution

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::Initialize(
    _In_ CONST EWICPixelFormat& ePixFormat,
    _In_ CONST UINT&            cWidth,
    _In_ CONST UINT&            cHeight,
    _In_ CONST DOUBLE&          dpiX,
    _In_ CONST DOUBLE&          dpiY
    )
{
    HRESULT hr = S_OK;

    if (ePixFormat <= kWICPixelFormatMin ||
        ePixFormat >= kWICPixelFormatMax ||
        cWidth == 0 ||
        cHeight == 0 ||
        dpiX <= 0 ||
        dpiY <= 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)))
    {
        //
        // Create the bitmap
        //
        m_pBitmap = NULL;
        m_ePixelFormat = ePixFormat;
        if (SUCCEEDED(hr = m_pImagingFactory->CreateBitmap(cWidth,
                                                           cHeight,
                                                           g_lutPixFrmtGuid[m_ePixelFormat],
                                                           WICBitmapCacheOnLoad,
                                                           &m_pBitmap)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_FAIL)))
        {
            //
            // Make sure the resolution is preserved - this can lead to strange scaling
            // in an XPS document if the resoution changes
            //
            hr = m_pBitmap->SetResolution(dpiX, dpiY);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::Initialize

Routine Description:

    Initializes the bitmap from a stream containing the container information

Arguments:

    pStream - Streamed container information

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::Initialize(
    _In_ IStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)))
    {
        CComPtr<IWICBitmapDecoder> pDecoder(NULL);
        CComPtr<IWICBitmapFrameDecode> pBitmapSrcFrame(NULL);
        LARGE_INTEGER cbMoveFromStart = {0};

        //
        // Ensure we have released any current color contexts
        //
        m_pColorContext = NULL;

        //
        // Create a decoder from the input stream and retrieve the first frame. Create a color context
        // to recieve any potential embedded color profiles.
        //
        if (SUCCEEDED(hr = pStream->Seek(cbMoveFromStart, STREAM_SEEK_SET, NULL)) &&
            SUCCEEDED(hr = m_pImagingFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder)) &&
            SUCCEEDED(hr = CHECK_POINTER(pDecoder, E_FAIL)) &&
            SUCCEEDED(hr = pDecoder->GetFrame(0, &pBitmapSrcFrame)) &&
            SUCCEEDED(hr = CHECK_POINTER(pDecoder, E_FAIL)) &&
            SUCCEEDED(hr = m_pImagingFactory->CreateColorContext(&m_pColorContext)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pColorContext, E_FAIL)))
        {
            //
            // Check for an embedded color context
            //
            UINT cColContexts = 0;
            if (SUCCEEDED(hr = pBitmapSrcFrame->GetColorContexts(1, &(m_pColorContext.p), &cColContexts)))
            {
                //
                // If there are no color contexts, ensure the color context is released
                //
                if (cColContexts == 0)
                {
                    m_pColorContext = NULL;
                }

                hr = Initialize(pBitmapSrcFrame);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::Initialize

Routine Description:

    Intializes the bitmap from a WIC bitmap source

Arguments:

    pSource - pointer to the WIC bitmap source interface to intialize from

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::Initialize(
    _In_ IWICBitmapSource* pSource
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)))
    {
        //
        // Make sure we have released any current bitmaps
        //
        m_pBitmap = NULL;

        //
        // Create the bitmap from the source and set the pixel format enumeration from the GUID
        //
        WICPixelFormatGUID guidPixFormat;
        if (SUCCEEDED(hr = m_pImagingFactory->CreateBitmapFromSource(pSource, WICBitmapCacheOnDemand, &m_pBitmap)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_FAIL)) &&
            SUCCEEDED(hr = m_pBitmap->GetPixelFormat(&guidPixFormat)))
        {
            hr = PixelFormatFromGUID(guidPixFormat, &m_ePixelFormat);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::Write

Routine Description:

    Writes the bitmap to a stream using the requested container format

Arguments:

    guidContainerFormat - Requested container format to write out
    pStream             - The destination stream to write to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::Write(
    _In_    REFGUID  guidContainerFormat,
    _Inout_ IStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)))
    {
        CComPtr<IWICBitmapEncoder>     pEncoder(NULL);
        CComPtr<IWICBitmapFrameEncode> pFrame(NULL);
        CComPtr<IPropertyBag2>         pPropertyBag(NULL);

        UINT srcWidth = 0;
        UINT srcHeight = 0;

        DOUBLE xRes = 0.0;
        DOUBLE yRes = 0.0;

        WICPixelFormatGUID srcFormat;

        //
        // Create the appropriate encoder, set the stream to recieve the data and retrieve a frame.
        // Initialize the frame from the current bitmap data.
        //
        if (SUCCEEDED(hr = m_pImagingFactory->CreateEncoder(guidContainerFormat, NULL, &pEncoder)) &&
            SUCCEEDED(hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache)) &&
            SUCCEEDED(hr = pEncoder->CreateNewFrame(&pFrame, &pPropertyBag)) &&
            SUCCEEDED(hr = CHECK_POINTER(pFrame, E_FAIL)) &&
            SUCCEEDED(hr = CHECK_POINTER(pPropertyBag, E_FAIL)) &&
            SUCCEEDED(hr = pFrame->Initialize(pPropertyBag)) &&
            SUCCEEDED(hr = m_pBitmap->GetSize(&srcWidth, &srcHeight)) &&
            SUCCEEDED(hr = m_pBitmap->GetResolution(&xRes, &yRes)) &&
            SUCCEEDED(hr = m_pBitmap->GetPixelFormat(&srcFormat)))
        {
            WICRect sizeSrc = {0};
            sizeSrc.Width  = static_cast<INT>(srcWidth);
            sizeSrc.Height = static_cast<INT>(srcHeight);

            if (SUCCEEDED(hr = pFrame->SetSize(srcWidth, srcHeight)) &&
                SUCCEEDED(hr = pFrame->SetResolution(xRes, yRes)) &&
                SUCCEEDED(hr = pFrame->SetPixelFormat(&srcFormat)))
            {
                if (HasColorContext())
                {
                    hr = pFrame->SetColorContexts(1, &m_pColorContext.p);
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Write the bitmap data to the frame.
                    //
                    hr = pFrame->WriteSource(m_pBitmap, &sizeSrc);
                }
            }
        }

        //
        // Commit the frame and the encoder - this sets the bitmap data and encodes the
        // bitmap into a container
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = pFrame->Commit()))
        {
            hr = pEncoder->Commit();
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::Convert

Routine Description:

    Uses WIC to convert from the current pixel format to the requested pixel format

Arguments:

    ePixFormat   - the requested pixel format
    pbCanConvert - variable that recieves whether the conversion is possible

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::Convert(
    _In_  EWICPixelFormat ePixFormat,
    _Out_ BOOL*           pbCanConvert
    )
{
    HRESULT hr = S_OK;

    if (ePixFormat > kWICPixelFormatDontCare &&
        ePixFormat < kWICPixelFormatMax)
    {
        if (SUCCEEDED(hr = CHECK_POINTER(pbCanConvert, E_POINTER)))
        {
            *pbCanConvert = TRUE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    //
    // If the requested format differs from the original we need to do some work.
    //
    if (SUCCEEDED(hr) &&
        ePixFormat != m_ePixelFormat)
    {
        CComPtr<IWICFormatConverter> pConverter(NULL);

        WICPixelFormatGUID dstFormat = g_lutPixFrmtGuid[ePixFormat];
        WICPixelFormatGUID srcFormat;

        //
        // Retrieve a format converter, intialize from the current bitmap the use the converter
        // to reset the bitmap to the appropriate type.
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_PENDING)) &&
            SUCCEEDED(hr = m_pImagingFactory->CreateFormatConverter(&pConverter)) &&
            SUCCEEDED(hr = CHECK_POINTER(pConverter, E_FAIL)) &&
            SUCCEEDED(hr = m_pBitmap->GetPixelFormat(&srcFormat)) &&
            SUCCEEDED(hr = pConverter->CanConvert(srcFormat, dstFormat, pbCanConvert)) &&
            *pbCanConvert &&
            SUCCEEDED(hr = pConverter->Initialize(m_pBitmap, dstFormat, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom)))
        {
            hr = Initialize(pConverter);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::LockSurface

Routine Description:

    Locks the requested rectangle for reading and writing of the pixel data directly

Arguments:

    prcLock   - The rectangle area of the bitmap to lock
    bReadOnly - Is this a read only lock
    pcbStride - Pointer to variable that recieves the stride between scanlines
    pcWidth   - Pointer to variable that recieves the bitmap pixel width
    pcHeight  - Pointer to variable that recieves the bitmap pixel height
    pcbData   - Pointer to variable that recieves the size of the pixel data buffer
    ppbData   - Pointer to a BYTE pointer that recieves the data pointer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::LockSurface(
    _In_            WICRect*    prcLock,
    _In_            CONST BOOL& bReadOnly,
    _Out_           UINT*       pcbStride,
    _Out_           UINT*       pcWidth,
    _Out_           UINT*       pcHeight,
    _Inout_         UINT*       pcbData,
    _Outptr_result_bytebuffer_maybenull_(*pcbData)
                    PBYTE*      ppbData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(prcLock, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbStride, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcWidth, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcHeight, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppbData, E_POINTER)))
    {
        *pcbStride = 0;
        *pcWidth = 0;
        *pcHeight = 0;
        *pcbData = 0;
        *ppbData = NULL;

        //
        // Free any current locks and acquire a new one. Get the stride, size and data from the lock.
        //
        m_pCurrentLock = NULL;
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = m_pBitmap->Lock(prcLock, bReadOnly ? WICBitmapLockRead : WICBitmapLockWrite, &m_pCurrentLock)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pCurrentLock, E_FAIL)) &&
            SUCCEEDED(hr = m_pCurrentLock->GetStride(pcbStride)) &&
            SUCCEEDED(hr = m_pCurrentLock->GetSize(pcWidth, pcHeight)))
        {
            hr = m_pCurrentLock->GetDataPointer(pcbData, ppbData);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::UnlockSurface

Routine Description:

    Unlocks any current lock on the bitmap surface

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::UnlockSurface(
    VOID
    )
{
    m_pCurrentLock = NULL;

    return S_OK;
}

/*++

Routine Name:

    CBmpConverter::GetColorContext

Routine Description:

    Retrieves any color context associated with the bitmap

Arguments:

    ppColorContext - Pointer to a color context interface pointer to recieve the context

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - On no context present
    E_*                 - On error

--*/
HRESULT
CBmpConverter::GetColorContext(
    _Outptr_ IWICColorContext** ppColorContext
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppColorContext, E_POINTER)))
    {
        *ppColorContext = NULL;

        if (HasColorContext())
        {
            *ppColorContext = m_pColorContext;
        }
        else
        {
            hr = E_ELEMENT_NOT_FOUND;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::HasAlphaChannel

Routine Description:

    Indicates if the bitmap has an alpha channel

Arguments:

    None

Return Value:

    TRUE  - The bitmap has an alpha channel
    FALSE - There is no alpha channel

--*/
BOOL
CBmpConverter::HasAlphaChannel(
    VOID
    ) CONST
{
    BOOL bHasAlpha = FALSE;

    switch (m_ePixelFormat)
    {
        case kWICPixelFormat32bppBGRA:
        case kWICPixelFormat32bppPBGRA:
        case kWICPixelFormat64bppRGBA:
        case kWICPixelFormat64bppPRGBA:
        case kWICPixelFormat128bppRGBAFloat:
        case kWICPixelFormat128bppPRGBAFloat:
        case kWICPixelFormat64bppRGBAFixedPoint:
        case kWICPixelFormat128bppRGBAFixedPoint:
        case kWICPixelFormat64bppRGBAHalf:
        case kWICPixelFormat40bppCMYKAlpha:
        case kWICPixelFormat80bppCMYKAlpha:
        case kWICPixelFormat32bpp3ChannelsAlpha:
        case kWICPixelFormat40bpp4ChannelsAlpha:
        case kWICPixelFormat48bpp5ChannelsAlpha:
        case kWICPixelFormat56bpp6ChannelsAlpha:
        case kWICPixelFormat64bpp7ChannelsAlpha:
        case kWICPixelFormat72bpp8ChannelsAlpha:
        case kWICPixelFormat64bpp3ChannelsAlpha:
        case kWICPixelFormat80bpp4ChannelsAlpha:
        case kWICPixelFormat96bpp5ChannelsAlpha:
        case kWICPixelFormat112bpp6ChannelsAlpha:
        case kWICPixelFormat128bpp7ChannelsAlpha:
        case kWICPixelFormat144bpp8ChannelsAlpha:
        {
            bHasAlpha = TRUE;
        }
        break;

        default:
        {
        }
        break;
    }

    return bHasAlpha;
}

/*++

Routine Name:

    CBmpConverter::HasColorContext

Routine Description:

    Indicates whether the bitmap has a color context

Arguments:

    None

Return Value:

    TRUE  - The bitmap has a color context
    FALSE - There is no color context associated with the bitmap

--*/
BOOL
CBmpConverter::HasColorContext(
    VOID
    ) CONST
{
    return m_pColorContext != NULL;
}

/*++

Routine Name:

    CBmpConverter::HasColorProfile

Routine Description:

    Indicates whether the color context (if present) contains a color profile

Arguments:

    None

Return Value:

    TRUE  - There is a color context and it contains a color profile
    FALSE - There is either no color context or the context does not contain a color profile

--*/
BOOL
CBmpConverter::HasColorProfile(
    VOID
    ) CONST
{
    BOOL bHasProfile = FALSE;

    if (HasColorContext())
    {
        WICColorContextType wicContextType = WICColorContextUninitialized;
        if (m_pColorContext != NULL &&
            SUCCEEDED(m_pColorContext->GetType(&wicContextType)))
        {
            bHasProfile = wicContextType == WICColorContextProfile;
        }
    }

    return bHasProfile;
}

/*++

Routine Name:

    CBmpConverter::GetPixelFormat

Routine Description:

    Retrieves the underlying pixel format

Arguments:

    None

Return Value:

    Enumerated value identifying the current pixel format of the bitmap

--*/
EWICPixelFormat
CBmpConverter::GetPixelFormat(
    VOID
    )
{
    return m_ePixelFormat;
}

/*++

Routine Name:

    CBmpConverter::GetSize

Routine Description:

    Retrieves the dimensions of the bitmap

Arguments:

    pcWidth  - Pointer to a  variable that recieves the pixel width
    pcHeight - Pointer to a  variable that recieves the pixel height

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::GetSize(
    _Out_ UINT* pcWidth,
    _Out_ UINT* pcHeight
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcWidth, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcHeight, E_POINTER)))
    {
        *pcWidth = 0;
        *pcHeight = 0;

        hr = m_pBitmap->GetSize(pcWidth, pcHeight);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::GetResolution

Routine Description:

    Retrieves the resolution of the underlying bitmap

Arguments:

    pDpiX - Pointer to a variable that recieves the horizontal resolution
    pDpiY - Pointer to a variable that recieves the vertical resolution

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::GetResolution(
    _Out_ DOUBLE* pDpiX,
    _Out_ DOUBLE* pDpiY
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDpiX, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDpiY, E_POINTER)))
    {
        *pDpiX = 0.0;
        *pDpiY = 0.0;

        hr = m_pBitmap->GetResolution(pDpiX, pDpiY);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::SetProfile

Routine Description:

    This method sets creates a color context based of the supplied profile filename and
    associates that cotext with the bitmap

Arguments:

    szProfile - The profile filename

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::SetProfile(
    _In_z_ LPWSTR szProfile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szProfile, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pImagingFactory, E_PENDING)))
    {
        m_pColorContext = NULL;
        if (SUCCEEDED(hr = m_pImagingFactory->CreateColorContext(&m_pColorContext)))
        {
            hr = m_pColorContext->InitializeFromFilename(szProfile);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::operator=

Routine Description:

    Assignment operator

Arguments:

    converter - Instance of a CBmpConverter to be assigned to

Return Value:

    Reference to the newly assigned CBmpConverter instance

--*/
CBmpConverter&
CBmpConverter::operator=(
    _In_ CONST CBmpConverter& converter
    )
{
    //
    // Assign all the member pointers. The COM pointer takes care of reference counting.
    //
    m_pImagingFactory = converter.m_pImagingFactory;
    m_pBitmap = converter.m_pBitmap;
    m_pColorContext = converter.m_pColorContext;
    m_pCurrentLock = converter.m_pCurrentLock;
    m_ePixelFormat = converter.m_ePixelFormat;

    ASSERTMSG(m_pCurrentLock == NULL, "Copying locked bitmap can lead to a deadlock.\n");

    return *this;
}

/*++

Routine Name:

    CBmpConverter::CreateImagingFactory

Routine Description:

    Creates the WIC imaging factory

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::CreateImagingFactory(
    VOID
    )
{
    m_pImagingFactory = NULL;
    HRESULT hr = m_pImagingFactory.CoCreateInstance(CLSID_WICImagingFactory);
    if (SUCCEEDED(hr))
    {
        hr = CHECK_POINTER(m_pImagingFactory, E_FAIL);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBmpConverter::PixelFormatFromGUID

Routine Description:

    Converts the WIC pixel format GUID to the matching pixel format enumeration

Arguments:

    pixelFormat - GUID defining the pixel format
    pPixFormat  - Pointer to the pixel format to be set

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBmpConverter::PixelFormatFromGUID(
    _In_  REFGUID          pixelFormat,
    _Out_ EWICPixelFormat* pPixFormat
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPixFormat, E_POINTER)))
    {
        *pPixFormat = kWICPixelFormatDontCare;
        BOOL bFound = 0;

        //
        // Loop all the potential formats
        //
        for (UINT cFormat = 0;
             cFormat < kWICPixelFormatMax;
             cFormat++)
        {
            //
            // Try to match the guid
            //
            if (pixelFormat == g_lutPixFrmtGuid[cFormat])
            {
                //
                // The index provides the corresponding pixel format enumerated value
                //
                bFound = TRUE;
                *pPixFormat = static_cast<EWICPixelFormat>(cFormat);
                break;
            }
        }

        if (!bFound)
        {
            RIP("Could not match format GUID\n");
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

