// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    BitmapHandler.cpp
//
// Abstract:
//
//    Abstract class that encapsulates the processing done to each 
//    individual band bitmap, as well as the concrete implemention
//    that streams the bands as Tiffs to the output stream.
//

#include "precomp.h"
#include "WppTrace.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "xpsrasfilter.h"
#include "OMConvertor.h"
#include "rasinterface.h"
#include "BitmapHandler.h"

#include "BitmapHandler.tmh"

namespace xpsrasfilter
{

//
//Routine Name:
//
//    TiffStreamBitmapHandler::CreateTiffStreamBitmapHandler
//
//Routine Description:
//
//    Static factory method that creates an instance of
//    TiffStreamBitmapHandler.
//
//Arguments:
//
//    pStream         - Filter output stream (IPrintWriteStream)
//
//Return Value:
//
//    TiffStreamBitmapHandler_t (smart ptr)
//    The new TiffStreamBitmapHandler.
//
TiffStreamBitmapHandler_t
TiffStreamBitmapHandler::CreateTiffStreamBitmapHandler(
    const IPrintWriteStream_t &pStream
    )
{
    IWICImagingFactory_t        pWICFactory;

    //
    // Create an instance of a WIC Imaging Factory
    //
    THROW_ON_FAILED_HRESULT(
        ::CoCreateInstance(
            CLSID_WICImagingFactory, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            __uuidof(IWICImagingFactory), 
            reinterpret_cast<LPVOID*>(&pWICFactory)
            )
        );

    //
    // Construct the TiffStreamBitmapHandler and return it
    //
    TiffStreamBitmapHandler_t toReturn(
                                new TiffStreamBitmapHandler(
                                        pWICFactory,
                                        pStream
                                        )
                                );

    return toReturn;
}

//
//Routine Name:
//
//    TiffStreamBitmapHandler::TiffStreamBitmapHandler
//
//Routine Description:
//
//    Construct the bitmap handler with the WIC factory
//    and filter output stream.
//
//Arguments:
//
//    pWICFactory - Windows Imaging Components object factory
//    pStream     - Output stream
//    pHG         - Encoder cache HGLOBAL
//
TiffStreamBitmapHandler::TiffStreamBitmapHandler(
    const IWICImagingFactory_t        &pWICFactory,
    const IPrintWriteStream_t         &pStream
    ) : m_pWICFactory(pWICFactory),
        m_pWriter(pStream),
        m_nextTiffStart(0),
        m_numTiffs(0),
        m_tiffStarts(0)
{
}

//
//Routine Name:
//
//    TiffStreamBitmapHandler::ProcessBitmap
//
//Routine Description:
//
//    Encode the bitmap as a TIFF and stream out of the filter.
//
//Arguments:
//
//    bitmap    - bitmap of a single band, to stream
//
void
TiffStreamBitmapHandler::ProcessBitmap(
    const IWICBitmap_t &bitmap
    )
{

    //
    // Create an empty HGLOBAL to hold the encode cache
    //
    SafeHGlobal_t pHG(
                    new SafeHGlobal(GMEM_SHARE | GMEM_MOVEABLE, 0)
                    );

    //
    // Create a stream to the encode buffer so that WIC can
    // encode the TIFF in-memory
    //
    IStream_t pIStream;

    THROW_ON_FAILED_HRESULT(
        ::CreateStreamOnHGlobal(
            *pHG,
            FALSE, // Do NOT Free the HGLOBAL on Release of the stream
            &pIStream
            )
        );

    //
    // Create a WIC TIFF Encoder on the stream
    //
    IWICBitmapEncoder_t pWICEncoder;
    THROW_ON_FAILED_HRESULT(
        m_pWICFactory->CreateEncoder(GUID_ContainerFormatTiff, NULL, &pWICEncoder)
        );
    THROW_ON_FAILED_HRESULT(
        pWICEncoder->Initialize(pIStream, WICBitmapEncoderNoCache)
        );

    //
    // Create a new frame for the band and configure it
    //
    IWICBitmapFrameEncode_t pWICFrame;
    IPropertyBag2_t pFramePropertyBag;

    THROW_ON_FAILED_HRESULT(
        pWICEncoder->CreateNewFrame(&pWICFrame, &pFramePropertyBag)
        );

    {
        //
        // Write the compression method to the frame's property bag
        //
        PROPBAG2 option = { 0 };
        option.pstrName = L"TiffCompressionMethod";
        
        VARIANT varValue;    
        VariantInit(&varValue);
        varValue.vt = VT_UI1;
        varValue.bVal = WICTiffCompressionLZW;      

        THROW_ON_FAILED_HRESULT(
            pFramePropertyBag->Write(
                1, // number of properties being set
                &option, 
                &varValue
                )
            );
    }

    THROW_ON_FAILED_HRESULT(
        pWICFrame->Initialize(pFramePropertyBag)
        );

    //
    // Set the frame's size
    //
    UINT bitmapWidth, bitmapHeight;
    THROW_ON_FAILED_HRESULT(
        bitmap->GetSize(&bitmapWidth, &bitmapHeight)
        );
    THROW_ON_FAILED_HRESULT(
        pWICFrame->SetSize(bitmapWidth, bitmapHeight)
        );

    //
    // Set the frame's resolution
    //
    DOUBLE xDPI, yDPI;
    THROW_ON_FAILED_HRESULT(
        bitmap->GetResolution(&xDPI, &yDPI)
        );
    THROW_ON_FAILED_HRESULT(
        pWICFrame->SetResolution(xDPI, yDPI)
        );

    //
    // Set the frame's pixel format
    //
    WICPixelFormatGUID format;
    THROW_ON_FAILED_HRESULT(
        bitmap->GetPixelFormat(&format)
        );
    THROW_ON_FAILED_HRESULT(
        pWICFrame->SetPixelFormat(&format)
        );

    //
    // Write the bitmap data to the frame
    //
    WICRect rect = {0, 0, 0, 0};
    rect.Width = bitmapWidth;
    rect.Height = bitmapHeight;
    THROW_ON_FAILED_HRESULT(
        pWICFrame->WriteSource(bitmap, &rect)
        );

    //
    // Commit the frame and encoder
    //
    THROW_ON_FAILED_HRESULT(
        pWICFrame->Commit()
        );
    THROW_ON_FAILED_HRESULT(
        pWICEncoder->Commit()
        );

    //
    // Get the size of the TIFF from the stream position
    //
    ULARGE_INTEGER tiffSize;
    LARGE_INTEGER zero;
    zero.QuadPart = 0;

    THROW_ON_FAILED_HRESULT(
        pIStream->Seek(zero, SEEK_CUR, &tiffSize)
        );

    ULONG cb;
    
    THROW_ON_FAILED_HRESULT(
        ::ULongLongToULong(tiffSize.QuadPart, &cb)
        );

    //
    // Update the list of Tiff locations so that it can be written to
    // the end of the Tiff stream.
    //
    m_tiffStarts.push_back(m_nextTiffStart);
    m_nextTiffStart += tiffSize.QuadPart;
    m_numTiffs++;

    {
        //
        // Get a pointer to the HGLOBAL memory
        //
        HGlobalLock_t lock = pHG->Lock();
        BYTE *pCache = lock->GetAddress();

        //
        // Write the encoded Tiff to the output stream
        //
        ULONG written;

        THROW_ON_FAILED_HRESULT(
            m_pWriter->WriteBytes(pCache, cb, &written)
            );
    }
}

//
//Routine Name:
//
//    TiffStreamBitmapHandler::WriteFooter
//
//Routine Description:
//
//    Write the footer to the stream, making it easier
//    to decode the individual TIFFs later. The footer
//    looks like this:
//
//       |<--8 bytes--->|
//
//       +--------------+
//       | Tiff 1 start |
//       +--------------+
//       | Tiff 2 start |
//       +--------------+
//       |     ...      |
//       +--------------+
//       | Tiff N start |
//       +--------------+
//       |      N       | 
//       +--------------+
//
void
TiffStreamBitmapHandler::WriteFooter()
{
    ULONG written;

    //
    // Write the vector of Tiff starts to the stream, if any Tiffs
    // have been written to the stream.
    //
    if (m_numTiffs > 0)
    {
        ULONG toWrite;

        THROW_ON_FAILED_HRESULT(
            SizeTToULong(
                sizeof(ULONGLONG) * m_tiffStarts.size(),
                &toWrite
                )
            );

        THROW_ON_FAILED_HRESULT(
            m_pWriter->WriteBytes(
                            reinterpret_cast<BYTE *>(&m_tiffStarts[0]),
                            toWrite, 
                            &written
                            )
            );
    }

    //
    // Write the number of Tiffs to the stream
    //
    THROW_ON_FAILED_HRESULT(
        m_pWriter->WriteBytes(
            reinterpret_cast<BYTE *>(&m_numTiffs),
            sizeof(m_numTiffs),
            &written
            )
        );
}

} // namespace xpsrasfilter
