/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  Title: FileConv.cpp
*
*  Description: This file contains implementation of utility functions for
*               image file format conversions used by the sample driver.
*
***************************************************************************/

#include "stdafx.h"

//
// The GDI+ codec name used for the DIB file format:
//
#define GDIPLUS_BMP_ENCODER L"image/bmp"

/**************************************************************************\
*
* Inline function to convert a GDI+ result code to a COM HRESULT,
* similar to the HRESULT_FOM_WIN32 macro.
*
* Parameters:
*
*    status - GDI+ return code value
*
* Return Value:
*
*    HRESULT describing the GDI+ status, E_FAIL if conversion is possible
*
\**************************************************************************/

inline HRESULT
GDISTATUS_TO_HRESULT(
    Gdiplus::Status status)
{
    HRESULT hr = E_FAIL;
    DWORD dwErr = NO_ERROR;

    switch (status)
    {
        case Gdiplus::Ok:
            hr = S_OK;
            break;

        case Gdiplus::InvalidParameter:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::InvalidParameter"));
            hr = E_INVALIDARG;
            break;

        case Gdiplus::OutOfMemory:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::OutOfMemory"));
            hr = E_OUTOFMEMORY;
            break;

        case Gdiplus::InsufficientBuffer:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::InsufficientBuffer"));
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            break;

        case Gdiplus::Aborted:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::Aborted"));
            hr = E_ABORT;
            break;

        case Gdiplus::ObjectBusy:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::ObjectBusy"));
            hr = E_PENDING;
            break;

        case Gdiplus::FileNotFound:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::FileNotFound"));
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;

        case Gdiplus::AccessDenied:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::AccessDenied"));
            hr = E_ACCESSDENIED;
            break;

        case Gdiplus::UnknownImageFormat:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::UnknownImageFormat"));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PIXEL_FORMAT);
            break;

        case Gdiplus::NotImplemented:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::NotImplemented"));
            hr = E_NOTIMPL;
            break;

        case Gdiplus::Win32Error:
            dwErr = GetLastError();
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::Win32Error, last error: 0x%08X", dwErr));
            hr = HRESULT_FROM_WIN32(dwErr);
            break;

        case Gdiplus::ValueOverflow:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::ValueOverflow"));
            hr = E_FAIL;
            break;

        case Gdiplus::FontFamilyNotFound:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::FontFamilyNotFound"));
            hr = E_FAIL;
            break;

        case Gdiplus::FontStyleNotFound:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::FontFamilyNotFound"));
            hr = E_FAIL;
            break;

        case Gdiplus::NotTrueTypeFont:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::NotTrueTypeFont"));
            hr = E_FAIL;
            break;

        case Gdiplus::UnsupportedGdiplusVersion:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::UnsupportedGdiplusVersion"));
            hr = E_FAIL;
            break;

        case Gdiplus::GdiplusNotInitialized:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::GdiplusNotInitialized"));
            hr = E_FAIL;
            break;

        case Gdiplus::WrongState:
            WIAEX_ERROR((g_hInst, "GDI+: Gdiplus::WrongState"));
            hr = E_FAIL;
            break;

        default:
            WIAEX_ERROR((g_hInst, "GDI+: unknown Gdiplus status code (%u)", (ULONG)status));
            hr = E_FAIL;
    }

    return hr;
}

/**************************************************************************\
*
* Executes GdiplusStartup to initialize the GDI+ engine. Must be called
* before calling ConvertImageToDIB. A matching ShutdownGDIPlus call
* must be always made to stop the GDI+ engine started by this function.
*
* Parameters:
*
*    ppToken - token returned by GDI+, must be used when calling ShutdownGDIPlus
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
InitializeGDIPlus(
    _Out_ ULONG_PTR *ppToken)
{
    HRESULT hr = S_OK;
    GdiplusStartupInput gdiplusStartupInput;

    if (!ppToken)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Start the GDI+ engine:
    //
    if (SUCCEEDED(hr))
    {
        hr = GDISTATUS_TO_HRESULT(GdiplusStartup(ppToken, &gdiplusStartupInput, NULL));
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Gdiplus::GdiplusStartup failed, hr = 0x%08X", hr));
        }
    }

    return hr;
}

/**************************************************************************\
*
* Executes GdiplusShutdown to shutdown the GDI+ engine.
*
* Parameters:
*
*    pToken - token obtained from a previous InitializeGDIPlus call
*
* Return Value:
*
*    E_INVALIDARG if called with a NULL pToken parameter or S_OK
*    (GDI+ does not return a result code for GdiplusShutdown)
*
\**************************************************************************/

HRESULT
ShutdownGDIPlus(
    _In_ ULONG_PTR pToken)
{
    HRESULT hr = S_OK;

    if (!pToken)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Shutdown the GDI+ engine:
    //
    if (SUCCEEDED(hr))
    {
        GdiplusShutdown(pToken);
    }

    return hr;
}

/**************************************************************************\
*
* Internal helper for ConvertImageToDIB. Enumerates available GDI+ image
* format encoders and returns the CLSID of the specified encoder, if available.
*
* Parameters:
*
*    wszFormat - GDI+ format name, e.g. "image/bmp" for DIB
*    pClisid   - returns the CLSID of the GDI+ encoder
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
_GetEncoderCLSID(
    _In_  LPCWSTR wszFormat,
    _Out_ CLSID  *pClsid)
{
    HRESULT hr = S_OK;
    UINT nNumEncoders = 0;
    UINT cbEncoderSize = 0;
    ImageCodecInfo *pImageCodecInfo = NULL;

    if ((!wszFormat) || (!pClsid))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        hr = GDISTATUS_TO_HRESULT(GetImageEncodersSize(&nNumEncoders, &cbEncoderSize));
        if (SUCCEEDED(hr) && (!cbEncoderSize))
        {
            hr = E_FAIL;
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Gdiplus::GetImageEncodersSize(%ws) failed, hr = 0x%08X", wszFormat, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        pImageCodecInfo = (ImageCodecInfo*)new BYTE[cbEncoderSize];
        if (!pImageCodecInfo)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for the encoder info, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = GDISTATUS_TO_HRESULT(GetImageEncoders(nNumEncoders, cbEncoderSize, pImageCodecInfo));
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Gdiplus::GetImageEncoders(%u encoders) failed, hr = 0x%08X", nNumEncoders, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;

        for (UINT j = 0; j < nNumEncoders; ++j)
        {
            _Analysis_assume_(cbEncoderSize >= nNumEncoders * sizeof(ImageCodecInfo));
            _Analysis_assume_(wcslen(wszFormat) < (cbEncoderSize / sizeof(WCHAR)));

            if (!wcscmp(pImageCodecInfo[j].MimeType, wszFormat))
            {
                *pClsid = pImageCodecInfo[j].Clsid;
                hr = S_OK;
                break;
            }
        }

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "No %ws encoder found in %u available GDI+ encoders, hr = 0x%08X", wszFormat, nNumEncoders, hr));
        }
    }

    if (pImageCodecInfo)
    {
        delete[] pImageCodecInfo;
    }

    return hr;
}

/**************************************************************************\
*
* Converts a GDI+ compatible image to a Windows DIB.
*
* Parameters:
*
*    pInputImage         - GDI+ Image object containing the image to be converted
*    ppOutputStream      - returns a new global memory IStream containing the
*                          converted DIB image file (must be released by caller)
*    plImageWidth        - (optional) returns the width of the image, in pixels
*    plImageHeight       - (optional) returns the height of the image, in pixels
*    plOutputImageBPL    - (optional) returns the estimated number of bytes
*                          per line for the converted DIB image
*
* Remarks:
*
*    The caller is responsible to release the returned IStream object to
*    free the memory after successful execution of this function.
*
*    The caller must execute InitializeGDIPlus before calling this function.
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
ConvertImageToDIB(
    _In_        Image    *pInputImage,
    _Outptr_ IStream **ppOutputStream,
    _Out_opt_   LONG     *plImageWidth,
    _Out_opt_   LONG     *plImageHeight,
    _Out_opt_   LONG     *plOutputImageBPL)
{
    HRESULT hr = S_OK;
    CLSID clsidBmpEncoder = {};
    LONG lImageWidth = 0;
    LONG lImageHeight = 0;
    LONG lOutputBPL = 0;
    LONG lBitDepth = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pInputImage) || (!ppOutputStream))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Read the dimensions of the input image:
    //
    if (SUCCEEDED(hr))
    {
        *ppOutputStream = NULL;

        lImageWidth = pInputImage->GetWidth();
        lImageHeight = pInputImage->GetHeight();
        lBitDepth = GetPixelFormatSize(pInputImage->GetPixelFormat());

        if ((lImageWidth > 0) && (lImageHeight > 0) && (lBitDepth > 0))
        {
            lOutputBPL = BytesPerLine(lImageWidth, lBitDepth);

            WIAS_TRACE((g_hInst, "Input image is %u x %u pixels, %u bpp, %u BPL (estimated) on output",
                lImageWidth, lImageHeight, lBitDepth, lOutputBPL));
        }
        else
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Incorrect image dimensions reported by GDI+ (%d x %d pixels, %d bpp), hr = 0x%08X",
                lImageWidth, lImageHeight, lBitDepth, hr));
        }
    }

    //
    // Retrieve the GDI+ encoder necessary to convert the input image to a DIB:
    //
    if (SUCCEEDED(hr))
    {
        hr = _GetEncoderCLSID(GDIPLUS_BMP_ENCODER, &clsidBmpEncoder);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "The GDI+ %ws encoder appears to be missing or improperly installed, hr = 0x%08X",
                GDIPLUS_BMP_ENCODER, hr));
        }
    }

    //
    // Create the output stream in global memory:
    //
    if (SUCCEEDED(hr))
    {
        hr = CreateStreamOnHGlobal(NULL, TRUE, ppOutputStream);
        if (SUCCEEDED(hr) && (!(*ppOutputStream)))
        {
            hr = E_FAIL;
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CreateStreamOnHGlobal failed, hr = 0x%08X", hr));
        }
    }

    //
    // Convert and save the image stored in the Image object to the output stream as a DIB:
    //
    if (SUCCEEDED(hr))
    {
        hr = GDISTATUS_TO_HRESULT(pInputImage->Save(*ppOutputStream, &clsidBmpEncoder));
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Image::Save(%ws) failed, hr = 0x%08X", GDIPLUS_BMP_ENCODER, hr));
        }
    }

    if (FAILED(hr))
    {
        if (ppOutputStream && *ppOutputStream)
        {
            (*ppOutputStream)->Release();
            *ppOutputStream = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (plImageWidth)
        {
            *plImageWidth = lImageWidth;
        }
        if (plImageHeight)
        {
            *plImageHeight = lImageHeight;
        }
        if (plOutputImageBPL)
        {
            *plOutputImageBPL = lOutputBPL;
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Converts a 8-bpp grayscale or 24-bpp RGB color DIB to a WIA Raw image.
*
* Parameters:
*
*    ppInputStream       - input stream containing the DIB to be converted
*    ppOutputStream      - returns a new global memory IStream containing the
*                          converted Raw image file (must be released by caller)
*
* Remarks:
*
*    The current form of this function for simplicty supports only 8-bpp
*    Grayscale and 24-bpp RGB color images. Palettes are not supported.
*
*    The caller is responsible to release the returned IStream object to
*    free the memory after successful execution of this function.
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
ConvertDibToRaw(
    _In_  IStream  *pInputStream,
    _Out_ IStream **ppOutputStream)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER liOffset = {};
    WIA_RAW_HEADER wiaRawHeader = {};
    BYTE bGrayPalette[256] = {};
    BITMAPINFOHEADER bih = {};
    ULONG ulDataSize = 0;
    ULONG ulDataWritten = 0;
    ULONG ulPaletteSize = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pInputStream) || (!ppOutputStream))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08.8X", hr));
    }
    else
    {
        *ppOutputStream = NULL;
    }

    liOffset.LowPart = sizeof(BITMAPFILEHEADER);
    hr = pInputStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
    if (S_OK != hr)
    {
        WIAEX_ERROR((g_hInst, "IStream::Seek(%u, STREAM_SEEK_SET, NULL) failed, hr = 0x%08X", liOffset.LowPart, hr));
    }

    if (S_OK == hr)
    {
        //
        // Extract the DIB header from the input stream:
        //
        hr = pInputStream->Read((void *)&bih, sizeof(bih), &ulDataSize);
        if ((S_OK == hr) && (ulDataSize != sizeof(bih)))
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Expected to read %u bytes for the DIB header, got %u bytes, hr = 0x%08X",
                sizeof(bih), ulDataSize, hr));
        }

        //
        // Validate the input DIB and fill in the output Raw header:
        //
        if (S_OK == hr)
        {
            if ((bih.biBitCount != 24) && (bih.biBitCount != 8))
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Test image must be either 8-bpp Grayscale or 24-bpp RGB Color, hr = 0x%08X",
                    sizeof(bih), ulDataSize, hr));
            }
            else
            {
                const char szRawSignature[] = "WRAW";
                memcpy(&wiaRawHeader.Tag, szRawSignature, sizeof(DWORD));

                wiaRawHeader.Version = 0x00010000;
                wiaRawHeader.HeaderSize = sizeof(wiaRawHeader);

                wiaRawHeader.XExtent = bih.biWidth;
                wiaRawHeader.YExtent = bih.biHeight;
                wiaRawHeader.LineOrder = (bih.biHeight < 0) ? WIA_LINE_ORDER_TOP_TO_BOTTOM : WIA_LINE_ORDER_BOTTOM_TO_TOP;
                wiaRawHeader.BitsPerPixel = bih.biBitCount;
                wiaRawHeader.BytesPerLine = BytesPerLine(bih.biWidth, bih.biBitCount);
                wiaRawHeader.ChannelsPerPixel = (8 == bih.biBitCount) ? 1 : 3;
                wiaRawHeader.DataType = (8 == bih.biBitCount) ?  WIA_DATA_GRAYSCALE : WIA_DATA_RAW_BGR;
                wiaRawHeader.BitsPerChannel[0] = 8;
                wiaRawHeader.BitsPerChannel[1] = (8 == bih.biBitCount) ?  0 : 8;
                wiaRawHeader.BitsPerChannel[2] = (8 == bih.biBitCount) ?  0 : 8;
                wiaRawHeader.Compression = WIA_COMPRESSION_NONE;
                wiaRawHeader.PhotometricInterp = WIA_PHOTO_WHITE_1;

                //
                // The scan resolution is fixed for this sample driver:
                //
                wiaRawHeader.XRes = OPTICAL_RESOLUTION;
                wiaRawHeader.YRes = OPTICAL_RESOLUTION;

                ulDataSize = wiaRawHeader.BytesPerLine * wiaRawHeader.YExtent;
                ulPaletteSize = (8 == bih.biBitCount) ? (256 * sizeof(BYTE)) : 0;

                wiaRawHeader.RawDataSize = ulDataSize;
                wiaRawHeader.RawDataOffset = ulPaletteSize;
                wiaRawHeader.PaletteOffset = 0;
                wiaRawHeader.PaletteSize = ulPaletteSize;

                //
                // For 8-bpp Grayscale data prepare a standard grayscale palette with entries sorted
                // in increasing or decreasing order depending on the photometric interpretation:
                //
                if (8 == bih.biBitCount)
                {
                    for (UINT i = 0; i < 256; i++)
                    {
                        bGrayPalette[i] = (WIA_PHOTO_WHITE_1 == wiaRawHeader.PhotometricInterp) ? (BYTE)i : (255 - (BYTE)i);
                    }
                }

                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.Version = 0x%08X", wiaRawHeader.Version));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.HeaderSize = %u bytes", wiaRawHeader.HeaderSize));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.XExtent = %u pixels", wiaRawHeader.XExtent));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.YExtent = %u pixels", wiaRawHeader.YExtent));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.LineOrder = %u", wiaRawHeader.LineOrder));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.BitsPerPixel = %u bpp", wiaRawHeader.BitsPerPixel));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.BytesPerLine = %u BPL", wiaRawHeader.BytesPerLine));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.ChannelsPerPixel = %u", wiaRawHeader.ChannelsPerPixel));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.DataType = %u", wiaRawHeader.DataType));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.BitsPerChannel[0] = %u bps", wiaRawHeader.BitsPerChannel[0]));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.BitsPerChannel[1] = %u bps", wiaRawHeader.BitsPerChannel[1]));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.BitsPerChannel[2] = %u bps", wiaRawHeader.BitsPerChannel[2]));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.Compression = %u", wiaRawHeader.Compression));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.PhotometricInterp = %u", wiaRawHeader.PhotometricInterp));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.XRes = %u DPI", wiaRawHeader.XRes));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.YRes = %u DPI", wiaRawHeader.YRes));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.RawDataOffset = %u bytes", wiaRawHeader.RawDataOffset));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.RawDataSize = %u bytes", wiaRawHeader.RawDataSize));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.PaletteOffset = %u bytes", wiaRawHeader.PaletteOffset));
                WIAS_TRACE((g_hInst, "WIA_RAW_HEADER.PaletteSize = %u bytes", wiaRawHeader.PaletteSize));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to read the DIB header for the test image, hr = 0x%08X", hr));
        }

        //
        // If there is a grayscale palette, jump the input stream pointer past it:
        //
        if ((S_OK == hr) && (8 == bih.biBitCount))
        {
            liOffset.LowPart = 256 * sizeof(RGBQUAD);
            hr = pInputStream->Seek(liOffset, STREAM_SEEK_CUR, NULL);
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IStream::Seek(%u, STREAM_SEEK_SET, NULL) failed, hr = 0x%08X", liOffset.LowPart, hr));
            }
        }

        //
        // Create the output stream in memory:
        //
        if (S_OK == hr)
        {
            hr = CreateStreamOnHGlobal(NULL, TRUE, ppOutputStream);
            if (SUCCEEDED(hr) && (!(*ppOutputStream)))
            {
                hr = E_FAIL;
            }
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "CreateStreamOnHGlobal failed, hr = 0x%08X", hr));
            }
        }

        //
        // Write the Raw header to the output stream:
        //
        if (S_OK == hr)
        {
            hr = (*ppOutputStream)->Write(&wiaRawHeader, sizeof(wiaRawHeader), &ulDataWritten);
            if (SUCCEEDED(hr) && (sizeof(wiaRawHeader) != ulDataWritten))
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "Expected to write %u bytes for the raw header, wrote %u bytes, hr = 0x%08X",
                    sizeof(wiaRawHeader), ulDataWritten, hr));
            }
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IStream::Write(%u bytes) failed, hr = 0x%08X", sizeof(wiaRawHeader), hr));
            }
        }

        //
        // Write the palette (if one) to the output stream:
        //
        if ((S_OK == hr) && (8 == bih.biBitCount))
        {
            hr = (*ppOutputStream)->Write(&bGrayPalette[0], ulPaletteSize, &ulDataWritten);
            if (SUCCEEDED(hr) && (ulPaletteSize != ulDataWritten))
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "Expected to write %u bytes for the raw palette, wrote %u bytes, hr = 0x%08X",
                    ulPaletteSize, ulDataWritten, hr));
            }
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IStream::Write(%u bytes) failed, hr = 0x%08X", ulPaletteSize, hr));
            }
        }

        //
        // Write the Raw image data to the output stream:
        //
        if (S_OK == hr)
        {
            ULARGE_INTEGER uliDataSize = {}, uliRead = {}, uliWritten = {};

            uliDataSize.LowPart = ulDataSize;

            hr = pInputStream->CopyTo(*ppOutputStream, uliDataSize, &uliRead, &uliWritten);
            if ((S_OK == hr) && ((ulDataSize != uliRead.LowPart) || (ulDataSize != uliWritten.LowPart)))
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "Expected to stream copy %u bytes for the raw image data, copied %u bytes, hr = 0x%08X",
                    ulDataSize, uliWritten.LowPart, hr));
            }
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IStream::CopyTo(%u bytes) failed, hr = 0x%08X", ulDataSize, hr));
            }
        }

        //
        // Reset the output stream seek pointer at the beginning of the stream:
        //
        if (S_OK == hr)
        {
            liOffset.LowPart = 0;
            hr = (*ppOutputStream)->Seek(liOffset, STREAM_SEEK_SET, NULL);
            if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IStream::Seek(0, STREAM_SEEK_SET, NULL) failed, hr = 0x%08X", hr));
            }
        }
    }

    if ((S_OK != hr) && ppOutputStream && *ppOutputStream)
    {
        (*ppOutputStream)->Release();
        *ppOutputStream = NULL;
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}
