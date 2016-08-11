/*****************************************************************************
 *
 *  WiaDevice.h
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION:
 *
 *  This class simulates a "real" device from which we can acquire image data and upload
 *  image data. It uses GDI+ internally to create the image. 
 *  
 *******************************************************************************/

#pragma once

using namespace Gdiplus;

extern HINSTANCE g_hInst;

class WiaDevice
{
public:
    WiaDevice() :
        m_dwTotalBytesToRead(0),
        m_dwTotalBytesRead(0),
        m_dwLinesRead(0),
        m_pBitmap(NULL),
        m_pBitmapData(NULL),
        m_pBitmapBits(NULL),
        m_ulHeaderSize(NULL),
        m_ulBytesPerLineBMP(0),
        m_ulBytesPerLineRAW(0)
    {
        memset(&m_bmfh, 0, sizeof(m_bmfh));
        memset(&m_bmih, 0, sizeof(m_bmih));
    };

    virtual ~WiaDevice()
    {
        UninitializeForDownload();
    };

    HRESULT InitializeForDownload(
        _In_ BYTE       *pWiasContext,
        _In_ HINSTANCE  hInstance,
        UINT            uiBitmapResourceID,
        const GUID  &guidFormatID)
    {
        HRESULT hr = E_INVALIDARG;

        memset(&m_RawHeader, 0, sizeof(m_RawHeader));

        if((pWiasContext)&&(hInstance))
        {
            HBITMAP hBitmap = static_cast<HBITMAP>(LoadImage(hInstance, MAKEINTRESOURCE(uiBitmapResourceID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));

            if (hBitmap)
            {
                m_pBitmap = Bitmap::FromHBITMAP(hBitmap, NULL);

                if(m_pBitmap)
                {
                    m_pBitmapData = new BitmapData;

                    if(m_pBitmapData)
                    {
                        hr = LockSelectionAreaOnBitmap(pWiasContext, m_pBitmap, m_pBitmapData, &m_bmih, &m_pBitmapBits);
                        if(SUCCEEDED(hr))
                        {
                            if(IsEqualGUID(guidFormatID, WiaImgFmt_RAW))
                            {
                                //
                                // Raw format (no color palette is used, just the header):
                                //
                                m_ulHeaderSize = sizeof(WIA_RAW_HEADER);
                            }
                            else
                            {
                                //
                                // Device Independent Bitmap (DIB):
                                //
                                m_ulHeaderSize = sizeof(m_bmfh) + sizeof(m_bmih);
                            }

                            //
                            // Initialize the remaining BITMAPINFOHEADER fields (use for both BMP and RAW transfers):
                            //
                            m_bmfh.bfType    = ((WORD) ('M' << 8) | 'B');
                            m_bmfh.bfOffBits = sizeof(m_bmfh) + sizeof(m_bmih); //m_ulHeaderSize;
                            m_bmfh.bfSize    = m_bmfh.bfOffBits + m_bmih.biSizeImage;

                            //
                            // We assume the sample source data is 24-bit RGB only:
                            //
                            m_ulBytesPerLineBMP = m_bmih.biWidth * 3; 

                            //
                            // The WIA raw format requires image lines to be DWORD aligned,
                            // in this case however the DIB data that we are using as the
                            // source is already DWORD aligned:
                            //
                            // m_ulBytesPerLineRAW = (m_ulBytesPerLineBMP + 3) & ~3;
                            //
                            m_ulBytesPerLineRAW = m_ulBytesPerLineBMP;

                            //
                            // m_dwTotalBytesToRead is used to measure the total number of bytes to read from the source DIB
                            // (in a real case for RAW this may be different than the actual number of bytes to be transferred,
                            // however in this particular case the two match because we accept in this sample only DIBs at input
                            // - with the exception of the DIB file header, see below..)
                            //
                            if(IsEqualGUID(guidFormatID, WiaImgFmt_RAW))
                            {
                                //
                                // For Raw this is just the size of the DIB data (no file header)
                                //
                                m_dwTotalBytesToRead = m_bmih.biSizeImage;

                                //
                                // The number of bytes in the raw data is described in this case by the number of bytes
                                // to be read from the DIB source (the data comes already DWORD aligned so the two numbers
                                // match in this particular case):
                                //
                                m_RawHeader.RawDataSize = m_dwTotalBytesToRead;
                                m_RawHeader.BytesPerLine = m_ulBytesPerLineRAW;
                            }
                            else
                            {
                                //
                                // For bitmap transfers the DIB file header is transferred too..
                                //
                                m_dwTotalBytesToRead = m_bmfh.bfSize;
                                m_RawHeader.RawDataSize = 0;
                            }

                            m_dwTotalBytesRead = 0;
                            m_dwLinesRead      = 0;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        WIAS_ERROR((g_hInst, "Failed to allocate memory for GDI+ bitmap data object, hr = 0x%lx",hr));
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAS_ERROR((g_hInst, "Failed to allocate memory for GDI+ bitmap object, hr = 0x%lx",hr));
                }

                DeleteObject(hBitmap);
            }
            else
            {
                DWORD dwError = GetLastError();

                hr = HRESULT_FROM_WIN32(dwError);
                WIAS_ERROR((g_hInst, "Failed to get HBITMAP for bitmap object, hr = 0x%lx", hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
        }
        return hr;
    }

    void UninitializeForDownload()
    {
        if (m_pBitmap && m_pBitmapData)
        {
            UnlockSelectionAreaOnBitmap(m_pBitmap, m_pBitmapData);
        }
        m_pBitmapBits = NULL;
        SAFE_DELETE(m_pBitmapData);
        SAFE_DELETE(m_pBitmap);
    }

    BOOL InitializedForDownload()
    {
        return (BOOL)(m_pBitmap && m_pBitmapData);
    }

    BitmapData* GetBitmapData()
    {
        return m_pBitmapData;
    }

    HRESULT GetNextBand(_Out_writes_bytes_to_(ulBufferSize, *pulBytesRead)  BYTE       *pBuffer,
                                                                        ULONG      ulBufferSize,
                        _Out_                                           ULONG      *pulBytesRead,
                        _Out_                                           LONG       *plPercentComplete,
                                                                        const GUID &guidFormatID)
    {
        HRESULT hr = S_OK;

        if (pBuffer && pulBytesRead && plPercentComplete && (ulBufferSize > m_ulHeaderSize))
        {
            //
            // iScanline contains the number of bytes to copy from each scanline
            //
            // Note: this logic works well considering that we are using only 
            // 24-bit RGB color sample images. For a real solution different
            // pixel formats and bit depths may have to be considered.
            //
            INT iScanline = ((m_pBitmapData->Width * 3) + 3) & ~3;

            *pulBytesRead       = 0;
            *plPercentComplete  = 0;

            if(m_dwTotalBytesRead < m_dwTotalBytesToRead)
            {
                //
                //  Check whether we should send the bitmap header or the data.
                //  The header is always sent first, unless this is a Raw transfer
                //  (when the raw header is individually sent before calling GetNextBand)
                //
                if((m_dwTotalBytesRead == 0) && (!IsEqualGUID(guidFormatID, WiaImgFmt_RAW)))
                {
                    if (ulBufferSize >= sizeof(m_bmfh) + sizeof(m_bmih))
                    {
                        //
                        // Read the header.
                        //
                        memcpy(pBuffer,&m_bmfh, sizeof(m_bmfh));
                        memcpy(pBuffer + sizeof(m_bmfh),&m_bmih, sizeof(m_bmih));
                        *pulBytesRead = m_ulHeaderSize;
                    }
                    else
                    {
                        //
                        // Insufficient Buffer
                        //
                        hr = E_FAIL;
                    }
                }
                else
                {
                    //
                    // For WIA raw transfers we do not have much to do in this case other than
                    // just copy the DIB data which already had DWORD line alignment, in the
                    // current line order the DIB provides (bottom to top usually) considering
                    // the raw header describes the current order (the WIA raw format supports
                    // both possible configurations). So we'll use the same code for both
                    // formats, WiaImgFmt_BMP and WiaImgFmt_RAW.
                    //    
                    
                    // Read a data band
                    //  First calculate number of bytes in whole scan lines.
                    DWORD dwNumLineBytesInBuffer    = (ulBufferSize - (ulBufferSize % iScanline));
                    DWORD dwNumBytesLeftToRead      = (m_dwTotalBytesToRead - m_dwTotalBytesRead);
                    //  Set how many bytes we are going to read.  This is either the maxiumun
                    //  nunmber of scan lines that will fit into the buffer, or it's the number
                    //  of bytes left in the last chunk.
                    if(dwNumBytesLeftToRead < dwNumLineBytesInBuffer)
                    {
                        dwNumLineBytesInBuffer = dwNumBytesLeftToRead;
                    }
                    //  Position buffer pointer to correct data location for this band.  We are copying 
                    //  in reverse scanline order so that the bitmap becomes topdown (it is currently
                    //  upside-down in the source buffer).
                    BYTE *pBits = m_pBitmapBits + (m_pBitmapData->Height * m_pBitmapData->Stride);
                    pBits -= (m_pBitmapData->Stride * (1 + m_dwLinesRead));

                    DWORD dwDestOffset  = 0;
                    for (BYTE *pCurLine = pBits; dwDestOffset < dwNumLineBytesInBuffer; pCurLine -= m_pBitmapData->Stride, m_dwLinesRead++)
                    {
                        if (ulBufferSize - dwDestOffset >= (ULONG) iScanline)
                        {
                            memcpy(pBuffer + dwDestOffset, pCurLine, iScanline);
                            dwDestOffset   += iScanline;
                        }
                        else
                        {
                            hr = E_FAIL;
                            break;
                        }
                    }
                    
                    *pulBytesRead = dwNumLineBytesInBuffer;
                }
                m_dwTotalBytesRead    += *pulBytesRead;
                
                if(IsEqualGUID(guidFormatID, WiaImgFmt_RAW))
                {
                    *plPercentComplete  = (LONG)((((float)(m_RawHeader.HeaderSize + m_dwTotalBytesRead) /
                        (float)(m_RawHeader.RawDataSize + m_RawHeader.HeaderSize + m_RawHeader.PaletteSize))) * 100.0f);
                }
                else
                {
                    *plPercentComplete     = (LONG)((((float)m_dwTotalBytesRead/(float)m_dwTotalBytesToRead)) * 100.0f);
                }
            }
            else
            {
                // We have no more data
                hr = WIA_STATUS_END_OF_MEDIA;
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Invalid parameters"));
            hr = E_INVALIDARG;
        }
        return hr;
    }

    HRESULT Upload(_In_         BSTR                        bstrItemName,
                                ULONG                       ulTotalBytes,
                   _In_         IStream                     *pSourceStream,
                   __callback   IWiaMiniDrvTransferCallback *pTransferCallback,
                   _Inout_      WiaTransferParams           *pParams,
                                const CBasicStringWide      &cswStoragePath)
    {
        // TBD: don't write to C:\TEMP\DATATRANSFERTEST, use actual storage item.
        HRESULT hr = S_OK;
        IStream             *pDestination   = NULL;
        CBasicStringWide   cswFileName     = cswStoragePath;
        cswFileName += L"\\";
        cswFileName += bstrItemName;

        // create stream on a file in the temporary directory (filename is bstrItemName)
        hr  = SHCreateStreamOnFile(cswFileName.String(),STGM_WRITE|STGM_CREATE,&pDestination);
        if(SUCCEEDED(hr))
        {
            // loop while reading data is availble from source stream
            BYTE *pBuffer = (BYTE*)CoTaskMemAlloc(DEFAULT_BUFFER_SIZE);
            if(pBuffer)
            {
                ULONG ulNumBytesRead        = 0;
                ULONG ulNumBytesWritten     = 0;
                ULONG ulTotalBytesWritten   = 0;

                //
                // Seek to the beginning of the stream before reading:
                //
                LARGE_INTEGER li = {0};
                hr = pSourceStream->Seek(li, STREAM_SEEK_SET, NULL);
                if (FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Could not seek to stream start before during upload, hr = 0x%lx", hr));
                }

                while (SUCCEEDED(hr) && SUCCEEDED(pSourceStream->Read(pBuffer,DEFAULT_BUFFER_SIZE,&ulNumBytesRead)) && ulNumBytesRead)
                {
                    //      write the chunk
                    hr = pDestination->Write(pBuffer,ulNumBytesRead,&ulNumBytesWritten);
                    if(FAILED(hr))
                    {
                        WIAS_ERROR((g_hInst, "Failed to write upload data to destination stream, hr = 0x%lx",hr));
                        break;
                    }

                    ulTotalBytesWritten += ulNumBytesWritten;

                    LONG lPercentComplete = -1;
                    if(ulTotalBytes > 0)
                    {
                        lPercentComplete = (LONG)((((float)ulTotalBytesWritten/(float)ulTotalBytes)) * 100.0f);
                    }
                    // make callback

                    pParams->lMessage           = WIA_TRANSFER_MSG_STATUS;
                    pParams->lPercentComplete   = lPercentComplete;
                    pParams->ulTransferredBytes = ulTotalBytesWritten;

                    hr = pTransferCallback->SendMessage(0,pParams);
                    if(SUCCEEDED(hr))
                    {
                        if(S_FALSE == hr)
                        {
                            WIAS_TRACE((g_hInst,"Application cancelled upload"));
                            break;
                        }
                        else if (S_OK != hr)
                        {
                            WIAS_ERROR((g_hInst, "SendMessage returned unknown Success value, hr = 0x%lx",hr));
                            hr = E_UNEXPECTED;
                            break;
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Failed to send status message to application. Upload aborted, hr = 0x%lx",hr));
                        break;
                    }
                }

                if(ulTotalBytesWritten == 0)
                {
                    hr = E_FAIL;
                    WIAS_ERROR((g_hInst, "No data was written during upload, hr = 0x%lx",hr));
                }

                CoTaskMemFree(pBuffer);
                pBuffer = NULL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate buffer for upload, hr = 0x%lx",hr));
            }

            // TBD: decide on exact behavior for notifying clients.

            pDestination->Release();
            pDestination = NULL;
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to create destination stream on file %ws, hr = 0x%lx",cswFileName.String(),hr));
        }
        return hr;
    }

public:
    DWORD               m_dwTotalBytesToRead;
    WIA_RAW_HEADER      m_RawHeader;

private:
    ULONG               m_ulHeaderSize;
    BITMAPFILEHEADER    m_bmfh;
    BITMAPINFOHEADER    m_bmih;
    DWORD               m_dwTotalBytesRead;
    DWORD               m_dwLinesRead;
    ULONG               m_ulBytesPerLineBMP;
    ULONG               m_ulBytesPerLineRAW;
    Bitmap              *m_pBitmap;
    BitmapData          *m_pBitmapData;
    BYTE                *m_pBitmapBits;
};

