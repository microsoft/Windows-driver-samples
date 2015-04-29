/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scaniter.cpp

Abstract:

   CScanIterator class implementation. The scan iterator class provides a convenient
   interface for iterating over WIC data and retrieving scanline data approriate for
   consumption in WCS/ICM. For example, the WIC pixel formats do not have alpha channel
   positions that correspond with the WCS/ICM BMFORMAT types so this class is responsible
   for presenting bitmap data without the alpha channel and for copying alpha data from
   source to destination when scnaline changes are commited to the underlying WIC bitmap.

--*/

#include "precomp.h"
#include "debug.h"
#include "xdexcept.h"
#include "globals.h"
#include "scaniter.h"

/*++

Routine Name:

    CScanIterator::CScanIterator

Routine Description:

    CScanIterator constructor

Arguments:


    bmpConv - Bitmap converter class encapsulating the underlying WIC bitmap
    pRect   - The rectangular area of the bitmap over which we want to iterate scanlines

Return Value:

    None
    Throws an exception on error.

--*/
CScanIterator::CScanIterator(
    _In_     CONST CBmpConverter& bmpConv,
    _In_opt_ WICRect*             pRect
    ) :
    CBmpConverter(bmpConv),
    m_bSrcLine(TRUE),
    m_cbWICStride(0),
    m_cWICWidth(0),
    m_cWICHeight(0),
    m_cbWICData(0),
    m_pbWICData(NULL)
{
    HRESULT hr = S_OK;

    UINT cWidth  = 0;
    UINT cHeight = 0;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pBitmap, E_POINTER)) &&
        SUCCEEDED(hr = m_pBitmap->GetSize(&cWidth, &cHeight)))
    {
        if (pRect == NULL)
        {
            //
            // We want to iterate over the entire surface
            //
            m_rectTotal.X = 0;
            m_rectTotal.Y = 0;
            m_rectTotal.Width  = static_cast<INT>(cWidth);
            m_rectTotal.Height = static_cast<INT>(cHeight);
        }
        else
        {
            if (pRect->X >= 0 &&
                pRect->Y >= 0 &&
                pRect->Width  <= static_cast<INT>(cWidth) &&
                pRect->Height <= static_cast<INT>(cHeight))
            {
                m_rectTotal = *pRect;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CScanIterator::~CScanIterator

Routine Description:

    CScanIterator destructor

Arguments:

    None

Return Value:

    None

--*/
CScanIterator::~CScanIterator()
{
    UnlockSurface();
}

/*++

Routine Name:

    CScanIterator::Initialize

Routine Description:

    Initialize the iterator. Here we are going to:

        1. Convert to a suitable WIC format for processing
        2. Allocate an intermediate color buffer (if required) for processing
        3. Allocate an intermediate alpha buffer (if required) for processing
        4. Reset and lock the iterator rect ready for processing

Arguments:

    bSrcLine - true if this is a source scanline (i.e. read only)

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CScanIterator::Initialize(
    _In_ CONST BOOL& bSrcLine
    )
{
    HRESULT hr = S_OK;
    m_bSrcLine = bSrcLine;

    //
    // Look up the pixel format to convert to so that the bitmap is appropriate
    // for consumption by WCS/ICM.
    //
    // Note: we are not dealing with the limitations of ICM downlevel. Downlevel
    // we need to convert floating and fixed point values to values appropriate
    // to ICM then back again before writing out so the underlying bitmap type is
    // unmodified.
    //
    EWICPixelFormat eConversionFormat = kWICPixelFormatDontCare;
    if (m_ePixelFormat > 0 &&
        m_ePixelFormat < kWICPixelFormatMax)
    {
        eConversionFormat = g_lutWICToBMFormat[m_ePixelFormat].m_pixFormTarget;
    }
    else
    {
        RIP("Unrecognised pixel format.\n");
        hr = E_FAIL;
    }

    //
    // Apply the conversion if required
    //
    BOOL bCanConvert = FALSE;
    if (eConversionFormat != m_ePixelFormat &&
        SUCCEEDED(hr) &&
        SUCCEEDED(hr = Convert(eConversionFormat, &bCanConvert)))
    {
        if (!bCanConvert)
        {
            RIP("Cannot convert to target type.\n");
            hr = E_FAIL;
        }
    }

    //
    // Set the current iteration data ready for processing
    //
    if (SUCCEEDED(hr))
    {
        //
        // Ensure the iterator is reset to the start of the area to be processed. This
        // ensures that the current rect lock is valid so that we can initialise the
        // first scanline to process
        //
        Reset();

        //
        // Initialise the WIC <-> BM scan line converter
        //
        if (SUCCEEDED(hr = m_currScan.Initialize(g_lutWICToBMFormat[m_ePixelFormat])))
        {
            hr = SetCurrentIterationData();
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CScanIterator::Reset

Routine Description:

    Resets the current rect to the first scanline in the buffer.

Arguments:

    None

Return Value:

    None

--*/
VOID
CScanIterator::Reset(
    VOID
    )
{
    m_rectCurrLock.X = m_rectTotal.X;
    m_rectCurrLock.Y = m_rectTotal.Y;
    m_rectCurrLock.Width = m_rectTotal.Width;
    m_rectCurrLock.Height = 1;
}

/*++

Routine Name:

    CScanIterator::operator++

Routine Description:

    Iterate to the next scan line.

Arguments:

    None

Return Value:

    Reference to this iterator

--*/
CScanIterator&
CScanIterator::operator++(INT)
{
    HRESULT hr = S_OK;

    m_rectCurrLock.Y++;
    if (!Finished())
    {
        if (FAILED(hr = SetCurrentIterationData()))
        {
            throw CXDException(hr);
        }
    }

    return *this;
}

/*++

Routine Name:

    CScanIterator::GetScanBuffer

Routine Description:

    Retrieves the scanline buffer appropriate for WCS/ICM consumption

Arguments:

    ppData    - Pointer to pointer that recieves the address of the data buffer
                Note: the buffer is only valid for the lifetime of the CScanIterator object.
    pBmFormat - Pointer to a BMFORMAT enumeration that recieves the format
    pcWidth   - Pointer to storage that recieves the pixel width
    pcHeight  - Pointer to storage that recieves the pixel height
    pcbStride - Pointer to storage that recieves the stride

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CScanIterator::GetScanBuffer(
    _Outptr_result_buffer_(*pcbStride) PBYTE*    ppData,
    _Out_                      BMFORMAT* pBmFormat,
    _Out_                      UINT*     pcWidth,
    _Out_                      UINT*     pcHeight,
    _Out_                      UINT*     pcbStride
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pBmFormat, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcWidth, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcHeight, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbStride, E_POINTER)))
    {
        *ppData    = NULL;
        *pcWidth   = 0;
        *pcHeight  = 1;
        *pcbStride = 0;
        *pBmFormat = BM_RGBTRIPLETS;

        //
        // Get the data from the WIC <-> BMFORMAT scanline converter
        //
        hr = m_currScan.GetData(ppData, pBmFormat, pcWidth, pcbStride);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CScanIterator::Commit

Routine Description:

    Commits the current color buffer to the surface if required and applies an optional alpha
    channel passed in from a source iterator

Arguments:

    alphaSource - Scan iterator instance with any potential alpha data to copy

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CScanIterator::Commit(
    _In_ CONST CScanIterator& alphaSource
    )
{
    HRESULT hr = S_OK;

    COLORDATATYPE srcDataType = COLOR_BYTE;
    COLORDATATYPE dstDataType = COLOR_BYTE;

    if (m_bSrcLine)
    {
        RIP("Cannot commit to readonly surface.\n");

        hr = E_FAIL;
    }
    else if (alphaSource.m_ePixelFormat >= kWICPixelFormatMax ||
             alphaSource.m_ePixelFormat <  kWICPixelFormatMin ||
             m_ePixelFormat >= kWICPixelFormatMax ||
             m_ePixelFormat <  kWICPixelFormatMin)
    {
        RIP("Invalid pixel format.\n");

        hr = E_FAIL;
    }
    else
    {
        srcDataType = g_lutWICToBMFormat[alphaSource.m_ePixelFormat].m_colDataType;
        dstDataType = g_lutWICToBMFormat[m_ePixelFormat].m_colDataType;

        if (srcDataType > COLOR_S2DOT13FIXED ||
            srcDataType < COLOR_BYTE ||
            dstDataType > COLOR_S2DOT13FIXED ||
            dstDataType < COLOR_BYTE)
        {
            RIP("Invalid data type.\n");

            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr) &&
        alphaSource.HasAlphaChannel() &&
        HasAlphaChannel())
    {
        //
        // Convert and copy alpha data into destination
        //
        PBYTE  pDst = m_pbWICData;
        size_t cbDstChannel     = g_lutColorDataSize[dstDataType];
        size_t cbDstAlphaOffset = g_lutWICToBMFormat[m_ePixelFormat].m_cAlphaOffset * cbDstChannel;
        UINT   cDstChannels     = g_lutWICToBMFormat[m_ePixelFormat].m_cChannels;

        PBYTE  pSrc = alphaSource.m_pbWICData;
        size_t cbSrcChannel     = g_lutColorDataSize[srcDataType];
        size_t cbSrcAlphaOffset = g_lutWICToBMFormat[alphaSource.m_ePixelFormat].m_cAlphaOffset * cbSrcChannel;
        UINT   cSrcChannels     = g_lutWICToBMFormat[alphaSource.m_ePixelFormat].m_cChannels;

        if (m_cWICWidth * cDstChannels * cbDstChannel <=  m_cbWICStride &&
            alphaSource.m_cWICWidth * cSrcChannels * cbSrcChannel <=  alphaSource.m_cbWICStride)
        {
            //
            // Move the source and destination to the first alpha channel
            //
            pDst += cbDstAlphaOffset;
            pSrc += cbSrcAlphaOffset;

            //
            // Call the appropriate convert copy function by casting the src pointer
            // to the underlying data type
            //
            switch (srcDataType)
            {
                case COLOR_BYTE:
                    hr = ConvertCopyAlphaChannels(reinterpret_cast<PBYTE>(pSrc),
                                                  m_cWICWidth,
                                                  cSrcChannels,
                                                  dstDataType,
                                                  pDst,
                                                  m_cWICWidth,
                                                  cDstChannels);
                break;

                case COLOR_WORD:
                    hr = ConvertCopyAlphaChannels(reinterpret_cast<PWORD>(pSrc),
                                                  m_cWICWidth,
                                                  cSrcChannels,
                                                  dstDataType,
                                                  pDst,
                                                  m_cWICWidth,
                                                  cDstChannels);
                break;

                case COLOR_FLOAT:
                    hr = ConvertCopyAlphaChannels(reinterpret_cast<PFLOAT>(pSrc),
                                                  m_cWICWidth,
                                                  cSrcChannels,
                                                  dstDataType,
                                                  pDst,
                                                  m_cWICWidth,
                                                  cDstChannels);
                break;

                case COLOR_S2DOT13FIXED:
                    hr = ConvertCopyAlphaChannels(reinterpret_cast<PS2DOT13FIXED>(pSrc),
                                                  m_cWICWidth,
                                                  cSrcChannels,
                                                  dstDataType,
                                                  pDst,
                                                  m_cWICWidth,
                                                  cDstChannels);
                break;

                default:
                {
                    RIP("Unrecognized source format.\n");

                    hr = E_FAIL;
                }
                break;
            }
        }
        else
        {
            RIP("Insufficient buffer sizes.\n");

            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    //
    // Commit the scanline
    //
    if (SUCCEEDED(hr))
    {
        hr = m_currScan.Commit(m_pbWICData, m_cbWICStride);
    }

    //
    // Release the lock
    //
    UnlockSurface();

    m_cbWICStride = 0;
    m_cWICWidth   = 0;
    m_cWICHeight  = 0;
    m_cbWICData   = 0;
    m_pbWICData   = NULL;

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CScanIterator::Finished

Routine Description:

    We are done once all scanlines have been processed.

Arguments:

    None

Return Value:

    TRUE  - We have iterated over all requested scanlines
    FALSE - There are scanlines remaining

--*/
BOOL
CScanIterator::Finished(
    VOID
    )
{
    return m_rectCurrLock.Y >= (m_rectTotal.Y + m_rectTotal.Height);
}

/*++

Routine Name:

    CScanIterator::SetCurrentIterationData

Routine Description:

    Sets up the current iteration data by locking the relevant area of the WIC bitmap
    source and getting the WIC to BMFORMAT converter class to apply any conversion required

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CScanIterator::SetCurrentIterationData(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_cbWICStride = 0;
    m_cWICWidth   = 0;
    m_cWICHeight  = 0;
    m_cbWICData   = 0;
    m_pbWICData   = NULL;

    if (SUCCEEDED(hr = LockSurface(&m_rectCurrLock,
                                   m_bSrcLine,
                                   &m_cbWICStride,
                                   &m_cWICWidth,
                                   &m_cWICHeight,
                                   &m_cbWICData,
                                   &m_pbWICData)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pbWICData, E_FAIL)))
    {
        hr = m_currScan.SetData(m_bSrcLine, m_pbWICData, m_cbWICStride, m_cWICWidth);
    }

    ERR_ON_HR(hr);
    return hr;
}

