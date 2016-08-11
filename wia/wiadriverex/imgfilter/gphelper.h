using namespace Gdiplus;

#define BUFFER_SIZE  (128 * 1024)

/*****************************************************************************
 *
 *  @func Gdiplus::Status | GetEncoderGUIDFromImage | Retrieves the encoder for a Bitmap
 *
 *  @parm   Bitmap | pOriginalBitmap |
 *          The Bitmap for which to get its encoder
 *
 *  @parm   CLSID | pFormatEncoder |
 *          On successful return this contains the GUID of the encoder for
 *          pOriginalBitmaps image type
 *
 *  @comm
 *  This function is used to return the GDI+ encoder guid for a Bitmap.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXXXXX    |
 *              Failure to retrieve image format
 *
 *****************************************************************************/
static Status GetEncoderGUIDFromImage(
    _In_	IN  Bitmap      *pOriginalBitmap,
    _Out_ OUT CLSID       *pFormatEncoder)
{
    Status          status;
    CLSID           imageFormat;
    UINT            num = 0;          // number of image encoders
    UINT            size = 0;         // size of the image encoder array in bytes
    ImageCodecInfo* pImageCodecInfo = NULL;
    BOOL            bFound = FALSE;

    status = (pOriginalBitmap && pFormatEncoder) ? Ok : InvalidParameter;

    if (status == Ok)
    {
        status = pOriginalBitmap->GetRawFormat(&imageFormat);
    }

    if (status == Ok)
    {
        status = GetImageEncodersSize(&num, &size);

        if ((status == Ok) && (size == 0))
        {
            status = GenericError;
        }
    }

    if (status == Ok)
    {
        pImageCodecInfo = (ImageCodecInfo*)(malloc(size));

        status = pImageCodecInfo ? Ok : OutOfMemory;
    }

    if (status == Ok)
    {
        status = GetImageEncoders(num, size, pImageCodecInfo);
    }

    if (status == Ok)
    {
        for(UINT j = 0; (j < num) && !bFound ; ++j)
        {
            _Analysis_assume_(size >= (num * sizeof(ImageCodecInfo))); 
            if( pImageCodecInfo[j].FormatID == imageFormat )
            {
                *pFormatEncoder = pImageCodecInfo[j].Clsid;
                bFound = TRUE;
            }
        }
    }

    if (status == Ok)
    {
        status = bFound ? Ok : UnknownImageFormat;
    }

    if (pImageCodecInfo)
    {
        free(pImageCodecInfo);
    }

    return status;
}


/*****************************************************************************
 *
 *  @func HRESULT | GetUpperLimitSize | Returns an estimate of the maximum size of a BMP
 *          image. The result of this function should be used in a subsequent call to
 *          IStream::SetSize to ensure that the stream does not have to do any reallocations
 *          of memory, which can be very expensive
 *
 *  @parm   ULONG | uWidth |
 *          Image width in pixels
 *
 *  @parm   ULONG | uHeight |
 *          Image height in pixels
 *
 *  @parm   ULONG | uBitsPerPixel |
 *          Number of bits per pixel
 * *
 *  @rvalue ULONG    |
 *              Estimated upper limit of image size.
 *
 *****************************************************************************/
static inline ULONG GetUpperLimitSize( ULONG uWidth, ULONG uHeight, ULONG uBitsPerPixel )
{
       return ( /*Safety factor of 1.33 = 8/6*/ uWidth * uHeight * uBitsPerPixel / 6 ) + /*Safety amount for overhead*/ 2048;
}


static inline HRESULT GDISTATUS_TO_HRESULT(Gdiplus::Status status)
{
    //
    // Default to turning GDI+ errors into generic failures
    //
    HRESULT hr = E_FAIL;

    switch( status )
    {
        case Gdiplus::Ok:
            hr = S_OK;
            break;

        case Gdiplus::InvalidParameter:
            hr = E_INVALIDARG;
            break;

        case Gdiplus::OutOfMemory:
            hr = E_OUTOFMEMORY;
            break;

        case Gdiplus::InsufficientBuffer:
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            break;

        case Gdiplus::Aborted:
            hr = E_ABORT;
            break;

        case Gdiplus::ObjectBusy:
            hr = E_PENDING;
            break;

        case Gdiplus::FileNotFound:
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;

        case Gdiplus::AccessDenied:
            hr = E_ACCESSDENIED;
            break;

        case Gdiplus::UnknownImageFormat:
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PIXEL_FORMAT);
            break;

        case Gdiplus::NotImplemented:
            hr = E_NOTIMPL;
            break;

        case Gdiplus::Win32Error:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        case Gdiplus::ValueOverflow:
        case Gdiplus::FontFamilyNotFound:
        case Gdiplus::FontStyleNotFound:
        case Gdiplus::NotTrueTypeFont:
        case Gdiplus::UnsupportedGdiplusVersion:
        case Gdiplus::GdiplusNotInitialized:
        case Gdiplus::WrongState:
            break;
    }
    return hr;
}

static inline void CalculateBrightnessAndContrastParams( INT iBrightness, INT iContrast, _Out_ float *scale, _Out_ float *translate )
{
    //
    // force values to be at least 1, to avoid undesired effects
    //
    if (iBrightness < 1)
    {
        iBrightness = 1;
    }
    if (iContrast < 1)
    {
        iContrast = 1;
    }

    //
    // get current brightness as a percentage of full scale
    //
    float fBrightness = (float)( 1000 - iBrightness ) / 1000.0f;
    if (fBrightness > 0.95f)
    {
        fBrightness = 0.95f; /* clamp */
    }

    //
    // get current contrast as a percentage of full scale
    //
    float fContrast = (float) iContrast / 1000.0f;
    if (fContrast > 1.0f)
    {
        fContrast = 1.0;    /* limit to 1.0    */
    }

    //
    // convert contrast to a scale value
    //
    if (fContrast <= 0.5f)
    {
        *scale = fContrast / 0.5f;    /* 0 -> 0, .5 -> 1.0 */
    }
    else
    {
        if (fContrast == 1.0f)
        {
                fContrast = 0.9999f;
        }
        *scale = 0.5f / (1.0f - fContrast); /* .5 -> 1.0, 1.0 -> inf */
    }

    *translate = 0.5f - *scale * fBrightness;
}

/*****************************************************************************
 *
 *  @func HRESULT | GetBitmapHeaderFromBitmapData | Fills in BITMAPINFOHEADER from BitmapData object
 *
 *  @parm   BitmapData* | pGDIPlusBitmapData |
 *          Pointer to a GDI+ BitmapData object
 *
 *
 *  @parm   BITMAPINFOHEADER* | pBitmapInfoHeader |
 *          Pointer to a BITMAPINFOHEADER structure
 *
 *  @comm
 *  This function populates a BITMAPINFOHEADER structure
 *  using data contained in a Gdiplus::BitmapData object.
 *  This function only works with 24-bit data.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXXXXX    |
 *              The function failed
 *
 *****************************************************************************/
HRESULT GetBitmapHeaderFromBitmapData(
    _In_	Gdiplus::BitmapData     *pGDIPlusBitmapData,
    _Out_ BITMAPINFOHEADER          *pBitmapInfoHeader)
{
    HRESULT hr = E_INVALIDARG;
    if((pGDIPlusBitmapData) && (pBitmapInfoHeader) && (pGDIPlusBitmapData->PixelFormat == PixelFormat24bppRGB))
    {
        memset(pBitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
        pBitmapInfoHeader->biSize       = sizeof(BITMAPINFOHEADER);
        pBitmapInfoHeader->biPlanes     = 1;
        pBitmapInfoHeader->biWidth      = pGDIPlusBitmapData->Width;
        pBitmapInfoHeader->biHeight     = pGDIPlusBitmapData->Height;

        // We cannot use the stride to calculate the size, because if there is no
        // format conversion, we might get the original bits...
        // We need to calculate the size based on the width
        pBitmapInfoHeader->biSizeImage  = ((((pGDIPlusBitmapData->Width * 3) + 3) & ~3) * pGDIPlusBitmapData->Height);

        pBitmapInfoHeader->biBitCount   = 24;
        hr = S_OK;
    }

    return hr;
}

/*****************************************************************************
 *
 *  @func HRESULT | WriteBitmapToStream | WriteBitmapToStream writes the data from the Bitmap object pTargetBitmap into the IStream pOutputStream
 *
 *  @parm   Bitmap* | pTargetBitmap |
 *          Pointer to a GDI+ Bitmap object
 *
 *
 *  @parm   IStream* | pOutputStream |
 *          Pointer to IStream provided by application. We write the data from pTargetBitmap
 *          into this stream
 *
 *  @comm
 *  We use this function since the GDI+ method Bitmap::Save method does not work
 *  very well for images that an application displays band by band since it results
 *  in a large number of small Write calls. Instead we do a LockBits to read the bits
 *  from the bitmap and then write them to the application's stream.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXXXXX    |
 *              The function failed
 *
 *****************************************************************************/
HRESULT
WriteBitmapToStream(
    _In_    Gdiplus::Bitmap *pTargetBitmap,
    _In_    IStream         *pOutputStream,
    _Inout_ ULONG64         *pulBytesWrittenToOutputStream)

{
    HRESULT          hr = S_OK;

    Gdiplus::Rect    rFrame(0, 0, pTargetBitmap->GetWidth(), pTargetBitmap->GetHeight());
    BitmapData       bitmapData = {0};
    BITMAPINFOHEADER bmih = {0};
    BITMAPFILEHEADER bmfh = {0};
    BOOL             bBitsLocked = FALSE;
    DWORD            dwTotalBytes = 0;
    DWORD            dwTotalBytesRead = 0;
    DWORD            dwLinesRead = 0;
    BYTE             *pBitmapBits = NULL;
    ULONG            cbWritten = 0;
    INT              iScanline = 0;
    DWORD            dwNumLineBytesInBuffer = 0;
    DWORD            dwNumBytesLeftToRead = 0;
    BYTE             *pBuffer = NULL;

    if (!pTargetBitmap || !pOutputStream || !pulBytesWrittenToOutputStream)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        pBuffer = (BYTE*) LocalAlloc(LPTR, BUFFER_SIZE);

        hr = pBuffer ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = GDISTATUS_TO_HRESULT(pTargetBitmap->LockBits(&rFrame, ImageLockModeRead, PixelFormat24bppRGB, &bitmapData));
    }

    if (SUCCEEDED(hr))
    {
        bBitsLocked = TRUE;
        hr = GetBitmapHeaderFromBitmapData(&bitmapData,&bmih);
    }

    if (SUCCEEDED(hr))
    {
        pBitmapBits     = (BYTE*)bitmapData.Scan0;
        bmfh.bfType     = ((WORD) ('M' << 8) | 'B');
        bmfh.bfOffBits  = sizeof(bmfh) + sizeof(bmih);
        bmfh.bfSize     = bmfh.bfOffBits + bmih.biSizeImage;

        dwTotalBytes        = bmfh.bfSize;
        dwTotalBytesRead    = 0;
        dwLinesRead         = 0;
        //
        // iScanline contains the number of bytes to copy from each scanline
        //
        iScanline = ((bitmapData.Width * 3) + 3) & ~3;

        if (iScanline > BUFFER_SIZE)
        {
            //
            // We don't have enough space in our temporary scanline buffer
            //
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            //  Calculate number of bytes in whole scan lines.
            //
            dwNumLineBytesInBuffer    = (BUFFER_SIZE - (BUFFER_SIZE % iScanline));
        }
    }

    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER li = {0};
        hr = pOutputStream->Seek(li, STREAM_SEEK_END, NULL);
    }

    //
    // First write bitmap headers
    //
    if (SUCCEEDED(hr))
    {
        hr = pOutputStream->Write(&bmfh, sizeof(bmfh), &cbWritten);
        dwTotalBytesRead += sizeof(bmfh);
    }

    if (SUCCEEDED(hr))
    {
        hr = pOutputStream->Write(&bmih, sizeof(bmih), &cbWritten);
        dwTotalBytesRead += sizeof(bmih);
    }

    while (SUCCEEDED(hr) && (dwTotalBytesRead < dwTotalBytes))
    {
        dwNumBytesLeftToRead      = (dwTotalBytes - dwTotalBytesRead);

        //
        //  Set how many bytes we are going to read.  This is either the maxiumun
        //  nunmber of scan lines that will fit into the buffer, or it's the number
        //  of bytes left in the last chunk.
        //
        if(dwNumBytesLeftToRead < dwNumLineBytesInBuffer)
        {
            dwNumLineBytesInBuffer = dwNumBytesLeftToRead;
        }

        //
        //  Position buffer pointer to correct data location for this band.  We are copying
        //  in reverse scanline order so that the bitmap becomes topdown (it is currently
        //  upside-down in the source buffer).
        //
        BYTE *pBits = pBitmapBits + (bitmapData.Height * bitmapData.Stride);
        pBits -= (bitmapData.Stride * (1 + dwLinesRead));

        DWORD dwDestOffset  = 0;
        for (BYTE *pCurLine = pBits; dwDestOffset < dwNumLineBytesInBuffer; pCurLine -= bitmapData.Stride, dwLinesRead++)
        {
            _Analysis_assume_(dwDestOffset + iScanline <= BUFFER_SIZE);
            memcpy(pBuffer + dwDestOffset, pCurLine, iScanline);
            dwDestOffset   += iScanline;
        }

        hr = pOutputStream->Write(pBuffer, dwNumLineBytesInBuffer, &cbWritten);

        //
        // We should check cbWritten here!
        //

        dwTotalBytesRead+= dwNumLineBytesInBuffer;

    }

    //
    // Update the pulBytesWrittenToOutputStream even in failure case
    //
    if (pulBytesWrittenToOutputStream)
    {
        *pulBytesWrittenToOutputStream = (ULONG64)dwTotalBytesRead;
    }

    if (bBitsLocked)
    {
        //
        // Although we do not save the results we should log any errors
        // during UnlockBits
        //
        pTargetBitmap->UnlockBits(&bitmapData);
    }

    if (pBuffer)
    {
        LocalFree(pBuffer);
    }

    return hr;
}

