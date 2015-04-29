/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmimg.cpp

Abstract:

   Color managed image implementation. The CColorManagedImage class is responsible
   for managing the image resource for a stored bitmap. This implements
   the IResWriter interface so that the font can be added to the resource
   cache.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "cmimg.h"
#include "streamcnv.h"

using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

/*++

Routine Name:

    CColorManagedImage::CColorManagedImage

Routine Description:

    Constructor for the CColorManagedImage class which registers internally the
    supplied resource URI and a pointer to the profile manager which will supply
    suitable color transforms to apply to that resource

Arguments:

    bstrResURI   - String containing the URI to the resource to be handled
    pProfManager - Pointer to a profile manager which will supply suitable
                   transforms to apply to the supplied resource

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CColorManagedImage::CColorManagedImage(
    _In_ BSTR             bstrResURI,
    _In_ CProfileManager* pProfManager,
    _In_ IFixedPage*      pFixedPage,
    _In_ ResDeleteMap*    pResDel
    ) :
    m_pProfManager(pProfManager),
    m_pFixedPage(pFixedPage),
    m_bstrSrcProfileURI(NULL),
    m_pResDel(pResDel)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pProfManager, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pFixedPage, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pResDel, E_POINTER)))
    {
        if (SysStringLen(bstrResURI) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrAttribute(bstrResURI);

        if (bstrAttribute.Length() == 0)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            try
            {
                //
                // Process the mark-up looking for the bitmap URI and any associated profiles
                //
                CStringXDW cstrAttribute(bstrResURI);
                CStringXDW cstrColConvBMP(L"{ColorConvertedBitmap ");

                cstrAttribute.Trim();
                INT cchFind = cstrAttribute.Find(cstrColConvBMP);

                if (cchFind == -1)
                {
                    //
                    // The mark-up is the bitmap URI
                    //
                    cstrAttribute.Trim();
                    m_bstrBitmapURI.Empty();
                    m_bstrBitmapURI.Attach(cstrAttribute.AllocSysString());
                }
                else
                {
                    //
                    // The mark-up is specifying a profile associated with the image.
                    // Extract the path and set this in the profile manager. The markup takes
                    // the form "{ColorConvertedBitmap image.ext profile.icc}".
                    //

                    //
                    // Delete leading string "{ColorConvertedBitmap " and the trailing "}"
                    //
                    cstrAttribute.Delete(0, cstrColConvBMP.GetLength());
                    cstrAttribute.Delete(cstrAttribute.GetLength() - 1, 1);
                    cstrAttribute.Trim();

                    //
                    // Find the seperating space
                    //
                    cchFind = cstrAttribute.Find(L" ");

                    //
                    // Construct the bitmap URI
                    //
                    m_bstrBitmapURI.Empty();
                    m_bstrBitmapURI.Attach(cstrAttribute.Left(cchFind).AllocSysString());

                    //
                    // Delete the bitmap URI and space leaving the profile URI
                    //
                    cstrAttribute.Delete(0, cchFind+1);

                    m_bstrSrcProfileURI.Empty();
                    m_bstrSrcProfileURI.Attach(cstrAttribute.AllocSysString());
                }
            }
            catch (CXDException& e)
            {
                hr = e;
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

    CColorManagedImage::~CColorManagedImage

Routine Description:

    Default destructor for the CColorManagedImage class

Arguments:

    None

Return Value:

    None

--*/
CColorManagedImage::~CColorManagedImage()
{
}

/*++

Routine Name:

    CColorManagedImage::WriteData

Routine Description:

    This method handles the decoding of a bitmap, the colour
    translation applied to the bitmap and the re-encoding of the bitmap

Arguments:

    pStream - Pointer to a stream to write the resource out to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManagedImage::WriteData(
    _In_ IPartBase*         pResource,
    _In_ IPrintWriteStream* pStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pResource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pStream, E_POINTER)))
    {
        try
        {
            CComPtr<IUnknown>         pRead(NULL);
            CComPtr<IPartImage>       pImagePart(NULL);
            CComPtr<IPrintReadStream> pImageStream(NULL);

            //
            // Load the bitmap into memory and create a stream to read the bitmap from
            //
            if (SUCCEEDED(hr = m_pFixedPage->GetPagePart(m_bstrBitmapURI, &pRead)) &&
                SUCCEEDED(hr = pRead.QueryInterface(&pImagePart)) &&
                SUCCEEDED(hr = pImagePart->GetStream(&pImageStream)))
            {
                BOOL bApplyTransform = FALSE;
                CComPtr<IStream> pInputImageStream(NULL);

                EWICPixelFormat eDstPixFormat = kWICPixelFormatDontCare;
                UINT            cDstWidth  = 0;
                UINT            cDstHeight = 0;
                DOUBLE          dpiDstX = 0.0;
                DOUBLE          dpiDstY = 0.0;

                EProfileOption eProfileOption = RGB;

                //
                // Set up the source and destination bitmaps so they are ready for use by TranslateBitmapBits
                //
                pInputImageStream.Attach(new(std::nothrow) CPrintReadStreamToIStream(pImageStream));
                if (SUCCEEDED(hr = CHECK_POINTER(pInputImageStream, E_OUTOFMEMORY)) &&
                    SUCCEEDED(hr = m_srcBmp.Initialize(pInputImageStream)) &&
                    SUCCEEDED(hr = m_srcBmp.GetSize(&cDstWidth, &cDstHeight)) &&
                    SUCCEEDED(hr = m_srcBmp.GetResolution(&dpiDstX, &dpiDstY)) &&
                    SUCCEEDED(hr = m_pProfManager->GetProfileOption(&eProfileOption)))
                {
                    //
                    // The destination bitmap format can be one of:
                    //
                    //    kWICPixelFormat128bppRGBFloat  - scRGB output
                    //    kWICPixelFormat128bppRGBAFloat - scRGB output with a source alpha channel
                    //    kWICPixelFormat64bppCMYK       - CMYK output
                    //    kWICPixelFormat80bppCMYKAlpha  - CMYK output with a source alpha channel
                    //
                    BOOL bScRGBOut = FALSE;
                    switch (eProfileOption)
                    {
                        case RGB:
                        {
                            eDstPixFormat = m_srcBmp.HasAlphaChannel() ? kWICPixelFormat128bppRGBAFloat : kWICPixelFormat128bppRGBFloat;
                            bScRGBOut = TRUE;
                        }
                        break;

                        case CMYK:
                        {
                            eDstPixFormat = m_srcBmp.HasAlphaChannel() ? kWICPixelFormat40bppCMYKAlpha : kWICPixelFormat32bppCMYK;
                        }
                        break;

                        default:
                        {
                            RIP("Unrecognised destination profile option.\n");

                            hr = E_FAIL;
                        }
                        break;
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // If the input is scRGB and the output is scRGB just pass through the bitmap
                        // Note: This should be optimised we do not write out a new bitmap but use the original
                        //
                        if (eDstPixFormat == m_srcBmp.GetPixelFormat() &&
                            bScRGBOut)
                        {
                            m_dstBmp = m_srcBmp;
                        }
                        else if (SUCCEEDED(hr = m_dstBmp.Initialize(eDstPixFormat, cDstWidth, cDstHeight, dpiDstX, dpiDstY)))
                        {
                            bApplyTransform = TRUE;
                        }
                    }
                }

                //
                // Process the source and destination bitmaps a scanline at a time if the output image
                // is still not set
                //
                if (SUCCEEDED(hr) &&
                    bApplyTransform)
                {
                    //
                    // Create scanline iterator objects from the source and destination bitmaps
                    //
                    CScanIterator srcIter(m_srcBmp, NULL);
                    CScanIterator dstIter(m_dstBmp, NULL);

                    //
                    // Initialize the iterators, set the source profile and transform the scanlines
                    //
                    if (SUCCEEDED(hr = srcIter.Initialize(TRUE)) &&
                        SUCCEEDED(hr = dstIter.Initialize(FALSE)) &&
                        SUCCEEDED(hr = SetSrcProfile(&srcIter)) &&
                        SUCCEEDED(hr = TransformScanLines(&srcIter, &dstIter)))
                    {
                        //
                        // CMYK output requires an embedded profile
                        //
                        CComBSTR bstrProfile;
                        if (eProfileOption == CMYK &&
                            SUCCEEDED(hr = m_pProfManager->GetDstProfileName(&bstrProfile)))
                        {
                            hr = dstIter.SetProfile(bstrProfile);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        m_dstBmp = dstIter;
                    }

                    //
                    // If there's a problem loading a suitable source profile, just pass
                    // the bitmap through unmodified
                    //
                    if (hr == HRESULT_FROM_WIN32(ERROR_PROFILE_NOT_FOUND))
                    {
                        bApplyTransform = FALSE;
                        m_dstBmp = m_srcBmp;
                        hr = S_OK;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Create a write stream to accept the converted bitmap, fill from the output bitmap
                    // and write to the output stream (after ensuring the stream is pointing to the start).
                    //
                    CComPtr<IStream> pOutputImageStream(NULL);

                    LARGE_INTEGER cbMoveFromStart = {0};

                    PBYTE pBuff = new(std::nothrow) BYTE[CB_COPY_BUFFER];

                    if (SUCCEEDED(hr = CHECK_POINTER(pBuff, E_OUTOFMEMORY)) &&
                        SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pOutputImageStream)) &&
                        SUCCEEDED(hr = m_dstBmp.Write(GUID_ContainerFormatWmp, pOutputImageStream)) &&
                        SUCCEEDED(hr = pOutputImageStream->Seek(cbMoveFromStart, STREAM_SEEK_SET, NULL)))
                    {
                        ULONG cbRead = 0;
                        ULONG cbWritten = 0;

                        while (SUCCEEDED(hr) &&
                               SUCCEEDED(hr = pOutputImageStream->Read(pBuff, CB_COPY_BUFFER, &cbRead)) &&
                               cbRead > 0)
                        {
                            hr = pStream->WriteBytes(pBuff, cbRead, &cbWritten);
                            ASSERTMSG(cbRead == cbWritten, "Failed to write all data.\n");
                        }
                    }

                    if (pBuff != NULL)
                    {
                        delete[] pBuff;
                        pBuff = NULL;
                    }
                }

                //
                // Set the content type of the image part
                //
                CComQIPtr<IPartImage> pImage = pResource;
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = CHECK_POINTER(pImage, E_NOINTERFACE)))
                {
                    hr = pImage->SetImageContent(CComBSTR(L"image/vnd.ms-photo"));
                }

                //
                // If all is well mark the replaced bitmap and profile for deletion
                //
                if (SUCCEEDED(hr))
                {
                    (*m_pResDel)[m_bstrBitmapURI.m_str] = TRUE;

                    if (m_bstrSrcProfileURI.Length() > 0)
                    {
                        (*m_pResDel)[m_bstrSrcProfileURI.m_str] = TRUE;
                    }
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorManagedImage::GetKeyName

Routine Description:

    Method to obtain a unique key for the resource being handled

Arguments:

    pbstrKeyName - Pointer to a string to hold the generated key

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManagedImage::GetKeyName(
    _Outptr_ BSTR* pbstrKeyName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrKeyName, E_POINTER)))
    {
        if (m_bstrBitmapURI.Length() > 0)
        {
            *pbstrKeyName = NULL;

            //
            // The full URI to the bitmap resource concatenated with any associated
            // profile is a suitable key
            //
            try
            {
                CStringXDW cstrKey(m_bstrBitmapURI);
                cstrKey += m_bstrSrcProfileURI;

                *pbstrKeyName = cstrKey.AllocSysString();
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }
        else
        {
            hr = E_PENDING;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorManagedImage::GetResURI

Routine Description:

    Method to obtain the URI of the resource being handled

Arguments:

    pbstrResURI - Pointer to a string to hold the resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManagedImage::GetResURI(
    _Outptr_ BSTR* pbstrResURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrResURI, E_POINTER)))
    {
        *pbstrResURI = NULL;

        if (m_bstrBitmapURI.Length() > 0)
        {
            try
            {
                //
                // Create a URI from the original bitmap URI and the current tick count
                //
                CStringXDW cstrFileName(m_bstrBitmapURI);
                CStringXDW cstrFileExt(PathFindExtension(cstrFileName));

                INT indFileExt = cstrFileName.Find(cstrFileExt);

                if (indFileExt > -1)
                {
                    cstrFileName.Delete(indFileExt, cstrFileExt.GetLength());
                }

                //
                // Create a unique name for the bitmap for this print session
                //
                CStringXDW cstrURI;
                cstrURI.Format(L"%s_%u.wdp", cstrFileName, GetUniqueNumber());

                SysFreeString(*pbstrResURI);
                *pbstrResURI = cstrURI.AllocSysString();
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }
        else
        {
            hr = E_PENDING;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorManagedImage::SetSrcProfile

Routine Description:

    Method to set the source color profile given a particular source bitmap

Arguments:

    pSrcBmp - Pointer to the source bitmap

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManagedImage::SetSrcProfile(
    _In_ CBmpConverter* pSrcBmp
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcBmp, E_POINTER)))
    {
        //
        // Update the source profile based on the source bitmap. The source profile
        // is set according to the following rules:
        //
        //    If the profile is specified in the ImageSource mark-up
        //        Use the profile in the container
        //    Else If the profile is embeded
        //        Use the embedded profile
        //    Else
        //        If the profile is RGB <= 16 bpc
        //            Use sRGB profile as source
        //        Else if the profile is RGB > 16 bpc
        //            Use scRGB profile as source
        //        Else if the profile is CMYK
        //            Use SWOP profile as source
        //
        if (m_bstrSrcProfileURI.Length() > 0)
        {
            //
            // The mark-up references a profile in the XPS document - get the profile manager
            // to extract it and set as the source profile
            //
            hr = m_pProfManager->SetSrcProfileFromContainer(m_bstrSrcProfileURI);
        }
        else if (pSrcBmp->HasColorProfile())
        {
            IWICColorContext* pSrcContext = NULL;

            UINT  cbBuffer = 0;
            PBYTE pBuffer = NULL;
            UINT  cbActual = 0;

            //
            // We have a profile embedded in the bitmap - extract and set as the source profile
            //
            if (SUCCEEDED(hr = pSrcBmp->GetColorContext(&pSrcContext)) &&
                SUCCEEDED(hr = pSrcContext->GetProfileBytes(cbBuffer, pBuffer, &cbActual)))
            {
                if (cbActual > 0)
                {
                    pBuffer  = new(std::nothrow) BYTE[cbActual];
                    cbBuffer = cbActual;

                    //
                    // The annotation on IWICColorContext::GetProfileBytes requires that the buffer
                    // be initialized.
                    //

					if (SUCCEEDED(hr = CHECK_POINTER(pBuffer, E_OUTOFMEMORY)))
					{
						::memset(pBuffer, 0, cbBuffer);
					}

                    if (SUCCEEDED(hr) &&
                        SUCCEEDED(hr = pSrcContext->GetProfileBytes(cbBuffer, pBuffer, &cbActual)))
                    {
                        //
                        // Set the source profile in the profile manager. Note: in this instance the profile name is
                        // no used to load the profile, merely to cache the profile. The bitmap URI uniquely idenitifies
                        // the bitmap and hence the embedded profile and is appropriate as a cache name
                        //
                        hr = m_pProfManager->SetSrcProfileFromBuffer(m_bstrBitmapURI, pBuffer, cbBuffer);
                    }

                    if (pBuffer != NULL)
                    {
                        delete[] pBuffer;
                        pBuffer = NULL;
                    }
                }
                else
                {
                    RIP("Zero length profile.\n");
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            //
            // Deduce the source profile from the source bitmap format
            //
            switch (pSrcBmp->GetPixelFormat())
            {
                case kWICPixelFormatDontCare:
                case kWICPixelFormat1bppIndexed:
                case kWICPixelFormat2bppIndexed:
                case kWICPixelFormat4bppIndexed:
                case kWICPixelFormat8bppIndexed:
                case kWICPixelFormatBlackWhite:
                case kWICPixelFormat2bppGray:
                case kWICPixelFormat4bppGray:
                case kWICPixelFormat8bppGray:
                case kWICPixelFormat16bppBGR555:
                case kWICPixelFormat16bppBGR565:
                case kWICPixelFormat16bppGray:
                case kWICPixelFormat24bppBGR:
                case kWICPixelFormat24bppRGB:
                case kWICPixelFormat32bppBGR:
                case kWICPixelFormat32bppBGRA:
                case kWICPixelFormat32bppPBGRA:
                case kWICPixelFormat32bppBGR101010:
                case kWICPixelFormat48bppRGB:
                case kWICPixelFormat64bppRGBA:
                case kWICPixelFormat64bppPRGBA:
                {
                    hr = m_pProfManager->SetSrcProfileFromColDir(L"sRGB Color Space Profile.icm");
                }
                break;

                case kWICPixelFormat48bppRGBFixedPoint:
                case kWICPixelFormat96bppRGBFixedPoint:
                case kWICPixelFormat128bppRGBAFloat:
                case kWICPixelFormat128bppPRGBAFloat:
                case kWICPixelFormat128bppRGBFloat:
                case kWICPixelFormat64bppRGBAFixedPoint:
                case kWICPixelFormat64bppRGBFixedPoint:
                case kWICPixelFormat128bppRGBAFixedPoint:
                case kWICPixelFormat128bppRGBFixedPoint:
                case kWICPixelFormat64bppRGBAHalf:
                case kWICPixelFormat64bppRGBHalf:
                case kWICPixelFormat48bppRGBHalf:
                case kWICPixelFormat32bppRGBE:
                case kWICPixelFormat16bppGrayHalf:
                case kWICPixelFormat32bppGrayFloat:
                case kWICPixelFormat32bppGrayFixedPoint:
                case kWICPixelFormat16bppGrayFixedPoint:
                {
                    hr = m_pProfManager->SetSrcProfileFromColDir(L"xdwscRGB.icc");
                }
                break;

                case kWICPixelFormat32bppCMYK:
                case kWICPixelFormat64bppCMYK:
                case kWICPixelFormat40bppCMYKAlpha:
                case kWICPixelFormat80bppCMYKAlpha:
                {
                    hr = m_pProfManager->SetSrcProfileFromColDir(L"xdCMYKPrinter.icc");
                }
                break;

                default:
                {
                    RIP("No acceptable default profile.\n");

                    hr = HRESULT_FROM_WIN32(ERROR_PROFILE_NOT_FOUND);
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CColorManagedImage::TransformScanLines

Routine Description:

    Given source and destination scanline iterators, this method applies the
    requiresite color transform

Arguments:

    pSrcScans - Pointer to the source scanline iterator
    pDstScans - Pointer to the destination scanline iterator

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorManagedImage::TransformScanLines(
    _In_ CScanIterator* pSrcScans,
    _In_ CScanIterator* pDstScans
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcScans, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDstScans, E_POINTER)))
    {
        try
        {
            //
            // Apply the transform
            //
            HTRANSFORM hTransform = NULL;
            BOOL       bCanUseWCS = FALSE;

            if (SUCCEEDED(hr = m_pProfManager->GetColorTransform(&hTransform, &bCanUseWCS)))
            {
                PBYTE    pDstData = NULL;
                BMFORMAT bmSrcFormat;
                UINT     cSrcWidth = 0;
                UINT     cSrcHeight = 0;
                UINT     cbSrcStride = 0;

                PBYTE    pSrcData = NULL;
                BMFORMAT bmDstFormat;
                UINT     cDstWidth = 0;
                UINT     cDstHeight = 0;
                UINT     cbDstStride = 0;

                //
                // While the source andd destination have scanlines remaining to process...
                //
                while (!pDstScans->Finished() &&
                       !pSrcScans->Finished() &&
                       SUCCEEDED(hr))
                {
                    //
                    // ...retrieve the scan buffers (these may have been processed to remove alpha data)...
                    //
                    if (SUCCEEDED(hr = pSrcScans->GetScanBuffer(&pSrcData, &bmSrcFormat, &cSrcWidth, &cSrcHeight, &cbSrcStride)) &&
                        SUCCEEDED(hr = pDstScans->GetScanBuffer(&pDstData, &bmDstFormat, &cDstWidth, &cDstHeight, &cbDstStride)))
                    {
                        //
                        // ...translate the scanline data...
                        //
                        if (TranslateBitmapBits(hTransform,
                                                pSrcData,
                                                bmSrcFormat,
                                                cSrcWidth,
                                                cSrcHeight,
                                                cbSrcStride,
                                                pDstData,
                                                bmDstFormat,
                                                cbDstStride,
                                                NULL,
                                                0))
                        {
                            //
                            // ...and commit the data to the destination bitmap before incrementing to the next scanline.
                            //
                            hr = pDstScans->Commit(*pSrcScans);
                            (*pSrcScans)++;
                            (*pDstScans)++;
                        }
                        else
                        {
                            RIP("Translate bitmap bits failed.\n");
                            hr = GetLastErrorAsHResult();
                        }
                    }
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

