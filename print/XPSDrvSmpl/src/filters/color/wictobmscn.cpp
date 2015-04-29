/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wictobmscn.cpp

Abstract:

   WIC pixel format to BMFORMAT conversion class implementation. This class provides methods
   for converting between a source WIC scanline to a target destination BMFORMAT scanline.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "wictobmscn.h"

/*++

Routine Name:

    CWICToBMFormatScan::CWICToBMFormatScan

Routine Description:

    CWICToBMFormatScan constructor

Arguments:

    None

Return Value:

    None

--*/
CWICToBMFormatScan::CWICToBMFormatScan() :
    m_pScanBuffer(NULL),
    m_cbScanBuffer(0),
    m_cWidth(0),
    m_pData(NULL),
    m_cbData(0),
    m_bInitialized(FALSE)
{
}

/*++

Routine Name:

    CWICToBMFormatScan::~CWICToBMFormatScan

Routine Description:

    CWICToBMFormatScan destructor

Arguments:

    None

Return Value:

    None

--*/
CWICToBMFormatScan::~CWICToBMFormatScan()
{
    FreeScanBuffer();
}

/*++

Routine Name:

    CWICToBMFormatScan::Initialize

Routine Description:

    Initializes the WIC to BMFORMAT transform data from the WICToBMFORMAT structure

Arguments:

    WICToBM - Structure containing WIC pixel format to BMFORMAT conversion information

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWICToBMFormatScan::Initialize(
    _In_ CONST WICToBMFORMAT& WICToBM
    )
{
    HRESULT hr = S_OK;

    m_convData = WICToBM;

    if (m_convData.m_pixFormTarget <= kWICPixelFormatMin ||
        m_convData.m_pixFormTarget >= kWICPixelFormatMax ||
        m_convData.m_bmFormTarget  <  kICMPixelFormatMin ||
        m_convData.m_bmFormTarget  >= kICMPixelFormatMax)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) &&
        !IsVista())
    {
        //
        // When processing color data down-level from Vista, we cannot use fixed or float
        // BMFORMAT types. In these circumstances we need to convert to a 16 bpc equivalent,
        // then back again.
        //
        if (m_convData.m_bmFormTarget == kBM_32b_scRGB ||
            m_convData.m_bmFormTarget == kBM_32b_scARGB ||
            m_convData.m_bmFormTarget == kBM_S2DOT13FIXED_scRGB ||
            m_convData.m_bmFormTarget == kBM_S2DOT13FIXED_scARGB)
        {
            m_convData.m_bNeedsScanBuffer = TRUE;
            m_convData.m_bmFormTarget = kBM_16b_RGB;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_bInitialized = TRUE;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWICToBMFormatScan::SetData

Routine Description:

    Set the current WIC data. This method takes the WIC scanline data and
    applies any necessary conversion to achieve the required BMFORMAT

Arguments:

    bIsSrc      - Indicates if this is a source scanline (i.e. is the conversion required as the scanline is input)
    pWicPxData  - Pointer to the WIC data
    cbWicPxData - Count of bytes in the WIC data buffer
    cWidth      - Count of pixels inthe scanline

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWICToBMFormatScan::SetData(
    _In_                     CONST BOOL bIsSrc,
    _In_reads_bytes_(cbWicPxData) PBYTE      pWicPxData,
    _In_                     CONST UINT cbWicPxData,
    _In_                     CONST UINT cWidth
    )
{
    HRESULT hr = m_bInitialized ? S_OK : E_PENDING;

    COLORDATATYPE   srcColType  = COLOR_BYTE;
    EICMPixelFormat eICMFormSrc = kBM_RGBTRIPLETS;

    COLORDATATYPE   dstColType = COLOR_BYTE;
    EICMPixelFormat eICMFormDst = kBM_RGBTRIPLETS;

    //
    // Validate input
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pWicPxData, E_POINTER)))
    {
        if (m_convData.m_pixFormTarget >= kWICPixelFormatMin &&
            m_convData.m_pixFormTarget <  kWICPixelFormatMax)
        {
            //
            // Set up the destination and source formats
            //
            eICMFormSrc = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_bmFormTarget;
            srcColType  = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_colDataType;

            if (m_convData.m_bmFormTarget <  kICMPixelFormatMax &&
                m_convData.m_bmFormTarget >= kICMPixelFormatMin)
            {
                dstColType  = g_lutBMFormatData[m_convData.m_bmFormTarget].m_colDataType;
                eICMFormDst = m_convData.m_bmFormTarget;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    //
    // Validate source and destination formats
    //
    if (SUCCEEDED(hr))
    {
        if (eICMFormSrc <  kICMPixelFormatMin ||
            eICMFormSrc >= kICMPixelFormatMax ||
            eICMFormDst <  kICMPixelFormatMin ||
            eICMFormDst >= kICMPixelFormatMax)
        {
            RIP("Invalid pixel format.\n");

            hr = E_FAIL;
        }
        else if (srcColType < COLOR_BYTE ||
                 srcColType > COLOR_S2DOT13FIXED ||
                 dstColType < COLOR_BYTE ||
                 dstColType > COLOR_S2DOT13FIXED)
        {
            RIP("Invalid color data type.\n");

            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_cWidth = cWidth;
        if (m_convData.m_bNeedsScanBuffer)
        {
            //
            // We need to convert in one way or another - create a buffer to
            // hold the intermediate scanline data
            //
            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = CreateScanBuffer(m_cWidth,
                                                g_lutColorDataSize[dstColType],
                                                g_lutBMFormatData[eICMFormDst].m_cChannels)))
            {
                if (bIsSrc)
                {
                    //
                    // We need to initialise the buffer from the WIC buffer passed in. Here we
                    // are doing one of two things:
                    //
                    // 1. Copying data directly as we have parity between the WIC and
                    //     ICM pixel formats
                    // 2. Convert from the WIC format to the ICM format and copy.
                    //

                    //
                    // Set up the source and destination pointers
                    //
                    PBYTE  pSrc = pWicPxData;
                    UINT   cSrcChannels   = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_cChannels;
                    size_t cbSrcBytesPerPixel = g_lutColorDataSize[srcColType] * cSrcChannels;

                    PBYTE  pDst = m_pScanBuffer;
                    UINT   cDstChannels   = g_lutBMFormatData[eICMFormDst].m_cChannels;
                    size_t cbDstBytesPerPixel = g_lutColorDataSize[dstColType] * cDstChannels;

                    PBYTE pSrcEnd = pSrc + cbWicPxData;
                    PBYTE pDstEnd = pDst + m_cbScanBuffer;

                    if (dstColType == m_convData.m_colDataType)
                    {
                        while (pSrc < pSrcEnd &&
                               pDst < pDstEnd)
                        {
                            if (pSrc + cbDstBytesPerPixel < pSrcEnd)
                            {
                                CopyMemory(pDst, pSrc, cbDstBytesPerPixel);
                            }
                            pSrc += cbSrcBytesPerPixel;
                            pDst += cbDstBytesPerPixel;
                        }
                    }
                    else
                    {
                        //
                        // We should only ever be required to convert from floating point and fixed point
                        // scRGB formats to 16 bpc RGB
                        //
                        if (eICMFormDst == kBM_16b_RGB &&
                            (eICMFormSrc == kBM_32b_scRGB ||
                             eICMFormSrc == kBM_32b_scARGB))
                        {
                            FLOAT* pSrcData = reinterpret_cast<FLOAT*>(pSrc);
                            WORD*  pDstData = reinterpret_cast<WORD*>(pDst);

                            while (pSrcData <= reinterpret_cast<FLOAT*>(pSrcEnd) - cSrcChannels &&
                                   pDstData <= reinterpret_cast<WORD*>(pDstEnd) - cDstChannels)
                            {
                                for (UINT cChan = 0;
                                     cChan < cDstChannels && cChan < cSrcChannels;
                                     cChan++)
                                {
                                    //
                                    // Note: We are only converting from FLOAT and not applying gamma modification
                                    // as the transform should account for this given the input scRGB ICC source profile.
                                    //
                                    if (pSrcData[cChan] <= -2.0f)
                                    {
                                        pDstData[cChan] = 0;
                                    }
                                    else if (pSrcData[cChan] >= 2.0f)
                                    {
                                        pDstData[cChan] = 0xFFFF;
                                    }
                                    else
                                    {
                                        pDstData[cChan] = static_cast<WORD>(kMaxWordAsFloat * (pSrcData[cChan] + 2.0) / 4.0f);
                                    }
                                }

                                pSrcData += cSrcChannels;
                                pDstData += cDstChannels;
                            }
                        }
                        else if (eICMFormDst == kBM_16b_RGB &&
                                 (eICMFormSrc == kBM_S2DOT13FIXED_scRGB ||
                                  eICMFormSrc == kBM_S2DOT13FIXED_scARGB))
                        {
                            WORD* pSrcData = reinterpret_cast<WORD*>(pSrc);
                            WORD* pDstData = reinterpret_cast<WORD*>(pDst);

                            while (pSrcData <= reinterpret_cast<WORD*>(pSrcEnd) - cSrcChannels &&
                                   pDstData <= reinterpret_cast<WORD*>(pDstEnd) - cDstChannels)
                            {
                                for (UINT cChan = 0;
                                     cChan < cDstChannels && cChan < cSrcChannels;
                                     cChan++)
                                {
                                    //
                                    // Note: We are only converting from S2DOT13FIXED and not applying gamma modification
                                    // as the transform should account for this given the input scRGB ICC source profile.
                                    //
                                    pDstData[cChan] = pSrcData[cChan] & kS2Dot13Neg ? pSrcData[cChan] ^ 0xFFFF : pSrcData[cChan] | kS2Dot13Neg;
                                }

                                pSrcData += cSrcChannels;
                                pDstData += cDstChannels;
                            }
                        }
                        else
                        {
                            hr = E_NOTIMPL;
                        }
                    }
                }

                //
                // Set the out buffer pointer and size to the copy buffer
                //
                if (SUCCEEDED(hr))
                {
                    m_pData  = m_pScanBuffer;
                    m_cbData = m_cbScanBuffer;
                }
            }
        }
        else
        {
            //
            // Set the out buffer pointer and size to the WIC pixel data
            //
            m_pData  = pWicPxData;
            m_cbData = cbWicPxData;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWICToBMFormatScan::Commit

Routine Description:

    Commits the internal scanline data to the WIC data buffer

Arguments:

    pWicPxData  - Pointer to the WIC data
    cbWicPxData - Count of bytes in the WIC data buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWICToBMFormatScan::Commit(
    _In_reads_bytes_(cbWicPxData) PBYTE      pWicPxData,
    _In_                     CONST UINT cbWicPxData
    )
{
    HRESULT hr = m_bInitialized ? S_OK : E_PENDING;

    COLORDATATYPE   srcColType  = COLOR_BYTE;
    EICMPixelFormat eICMFormSrc = kBM_RGBTRIPLETS;

    COLORDATATYPE   dstColType = COLOR_BYTE;
    EICMPixelFormat eICMFormDst = kBM_RGBTRIPLETS;

    //
    // Validate input
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pWicPxData, E_POINTER)))
    {
        if (m_convData.m_pixFormTarget >= kWICPixelFormatMin &&
            m_convData.m_pixFormTarget <  kWICPixelFormatMax)
        {
            //
            // Set up the destination and source formats
            //
            eICMFormDst = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_bmFormTarget;
            dstColType  = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_colDataType;

            if (m_convData.m_bmFormTarget <  kICMPixelFormatMax &&
                m_convData.m_bmFormTarget >= kICMPixelFormatMin)
            {
                eICMFormSrc = m_convData.m_bmFormTarget;
                srcColType = g_lutBMFormatData[eICMFormSrc].m_colDataType;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    //
    // Validate source and destination formats
    //
    if (SUCCEEDED(hr))
    {
        if (eICMFormSrc <  kICMPixelFormatMin ||
            eICMFormSrc >= kICMPixelFormatMax ||
            eICMFormDst <  kICMPixelFormatMin ||
            eICMFormDst >= kICMPixelFormatMax)
        {
            RIP("Invalid pixel format.\n");

            hr = E_FAIL;
        }
        else if (srcColType < COLOR_BYTE ||
                 srcColType > COLOR_S2DOT13FIXED ||
                 dstColType < COLOR_BYTE ||
                 dstColType > COLOR_S2DOT13FIXED)
        {
            RIP("Invalid color data type.\n");

            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pWicPxData != m_pData)
        {
            //
            // We need to convert the scan buffer and write back into the WIC buffer
            //
            PBYTE  pDst = pWicPxData;
            UINT   cDstChannels   = g_lutWICToBMFormat[m_convData.m_pixFormTarget].m_cChannels;
            size_t cbDstBytesPerPixel = g_lutColorDataSize[dstColType] * cDstChannels;

            PBYTE  pSrc = m_pScanBuffer;
            UINT   cSrcChannels   = g_lutBMFormatData[eICMFormSrc].m_cChannels;
            size_t cbSrcBytesPerPixel = g_lutColorDataSize[srcColType] * cSrcChannels;

            PBYTE pSrcEnd = pSrc + m_cbScanBuffer;
            PBYTE pDstEnd = pDst + cbWicPxData;

            if (srcColType == m_convData.m_colDataType)
            {
                while (pSrc < pSrcEnd &&
                       pDst < pDstEnd)
                {
                    if (pDst + cbSrcBytesPerPixel < pDstEnd)
                    {
                        CopyMemory(pDst, pSrc, cbSrcBytesPerPixel);
                    }

                    pSrc += cbSrcBytesPerPixel;
                    pDst += cbDstBytesPerPixel;
                }
            }
            else
            {
                //
                // We should only ever be required to convert from 16 bpc RGB to floating point
                // and fixed point scRGB formats
                //
                if (eICMFormSrc == kBM_16b_RGB &&
                    (eICMFormDst == kBM_32b_scRGB ||
                     eICMFormDst == kBM_32b_scARGB))
                {
                    WORD*  pSrcData = reinterpret_cast<WORD*>(pSrc);
                    FLOAT* pDstData = reinterpret_cast<FLOAT*>(pDst);
                    UINT cAlpha = eICMFormDst == kBM_32b_scARGB ? 1 : 0;

                    while (pSrcData <= reinterpret_cast<WORD*>(pSrcEnd) - cSrcChannels &&
                           pDstData <= reinterpret_cast<FLOAT*>(pDstEnd) - cDstChannels)
                    {
                        pDstData += cAlpha;

                        for (UINT cChan = 0;
                             cChan < (cDstChannels - cAlpha) && cChan < cSrcChannels;
                             cChan++)
                        {
                            //
                            // Note: We are only converting to FLOAT and not applying gamma modification
                            // as the transform should account for this given the input scRGB ICC source profile.
                            //
                            pDstData[cChan] = (4.0f * static_cast<FLOAT>(pSrcData[cChan]) / kMaxWordAsFloat) - 2.0f;
                        }

                        pSrcData += cSrcChannels;
                        pDstData += cDstChannels - cAlpha;
                    }
                }
                else if (eICMFormSrc == kBM_16b_RGB &&
                         (eICMFormDst == kBM_S2DOT13FIXED_scRGB ||
                          eICMFormDst == kBM_S2DOT13FIXED_scARGB))
                {
                    WORD* pSrcData = reinterpret_cast<WORD*>(pSrc);
                    WORD* pDstData = reinterpret_cast<WORD*>(pDst);
                    UINT cAlpha = eICMFormDst == kBM_S2DOT13FIXED_scARGB ? 1 : 0;

                    while (pSrcData <= reinterpret_cast<WORD*>(pSrcEnd) - cSrcChannels &&
                           pDstData <= reinterpret_cast<WORD*>(pDstEnd) - cDstChannels)
                    {
                        pDstData += cAlpha;

                        for (UINT cChan = 0;
                             cChan < (cDstChannels - cAlpha) && cChan < cSrcChannels;
                             cChan++)
                        {
                            //
                            // Note: We are only converting to S2DOT13FIXED and not applying gamma modification
                            // as the transform should account for this given the input scRGB ICC source profile.
                            //
                            pDstData[cChan] = pSrcData[cChan] & kS2Dot13Neg ?
                                pSrcData[cChan] ^ kS2Dot13Neg : pSrcData[cChan] ^ 0xFFFF;
                        }

                        pSrcData += cSrcChannels;
                        pDstData += cDstChannels - cAlpha;
                    }
                }
                else
                {
                    hr = E_NOTIMPL;
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWICToBMFormatScan::GetData

Routine Description:

    Retrieves the BMFORMAT buffer ready for processing

Arguments:

    ppData    - Pointer to a pointer that recieves the address of the data buffer.
                Note: the buffer is only valid for the lifetime of the CWICToBMFormatScan object.
    pBmFormat - Pointer to a BMFORMAT enum that recieves the type
    pcWidth   - Pointer to storage that recieves the pixel width
    pcbStride - Pointer to storage that recieves the scanline stride

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWICToBMFormatScan::GetData(
    _Outptr_result_bytebuffer_(*pcbStride) PBYTE*    ppData,
    _Out_                          BMFORMAT* pBmFormat,
    _Out_                          UINT*     pcWidth,
    _Out_                          UINT*     pcbStride
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pBmFormat, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcWidth, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbStride, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pData, E_PENDING)))
    {
        if (m_convData.m_bmFormTarget >= kICMPixelFormatMin &&
            m_convData.m_bmFormTarget <  kICMPixelFormatMax)
        {
            *pBmFormat = g_lutBMFormatData[m_convData.m_bmFormTarget].m_bmFormat;
            *pcWidth   = m_cWidth;
            *ppData    = m_pData;
            *pcbStride = static_cast<UINT>(m_cbData);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWICToBMFormatScan::CreateScanBuffer

Routine Description:

    Creates the intermediate scanline buffer

Arguments:

    cWidth - Pixel width of the scanline

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWICToBMFormatScan::CreateScanBuffer(
    _In_ CONST UINT   cWidth,
    _In_ CONST SIZE_T cbDataType,
    _In_ CONST UINT   cChannels
    )
{
    HRESULT hr = S_OK;

    if (cWidth     > MAX_PIXELWIDTH_COUNT ||
        cbDataType > MAX_COLDATATYPE_SIZE ||
        cChannels  > MAX_COLCHANNEL_COUNT)
    {
        hr = E_INVALIDARG;
    }

    size_t cbScanBuffer = 0;

    if (SUCCEEDED(hr))
    {
        if (FAILED(SizeTMult(cWidth, cbDataType, &cbScanBuffer)) ||
            FAILED(SizeTMult(cbScanBuffer, cChannels, &cbScanBuffer)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }

    }

    if (SUCCEEDED(hr))
    {
        if (cbScanBuffer != m_cbScanBuffer)
        {
            FreeScanBuffer();
            m_pScanBuffer = new(std::nothrow) BYTE[cbScanBuffer];
            if (SUCCEEDED(hr = CHECK_POINTER(m_pScanBuffer, E_OUTOFMEMORY)))
            {
                m_cbScanBuffer = cbScanBuffer;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWICToBMFormatScan::FreeScanBuffer

Routine Description:

    Free the intermediate scanline buffer

Arguments:

    None

Return Value:

    None

--*/
VOID
CWICToBMFormatScan::FreeScanBuffer(
    VOID
    )
{
    if (m_pScanBuffer != NULL)
    {
        delete[] m_pScanBuffer;
        m_pScanBuffer = NULL;
    }

    m_cbScanBuffer = 0;
}

