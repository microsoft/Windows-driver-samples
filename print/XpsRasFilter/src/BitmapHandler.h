// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    BitmapHandler.h
//
// Abstract:
//
//    Abstract class that encapsulates the processing done to each 
//    individual band bitmap, as well as the concrete implemention
//    that streams the bands as Tiffs to the output stream.
//

#pragma once

namespace xpsrasfilter
{

class TiffStreamBitmapHandler
{
public:

    static
    TiffStreamBitmapHandler_t
    CreateTiffStreamBitmapHandler(
        const IPrintWriteStream_t &pStream
        );

    void
    ProcessBitmap(
        const IWICBitmap_t &bitmap
        );

    void
    WriteFooter();

private:
    IWICImagingFactory_t    m_pWICFactory;
    IPrintWriteStream_t     m_pWriter;        // output stream

    //
    // Members to keep track of where each Tiff
    // starts in the output stream
    //
    ULONGLONG               m_nextTiffStart;
    ULONGLONG               m_numTiffs;
    std::vector<ULONGLONG>  m_tiffStarts;

    //
    // Constructor is private; use CreateTiffStreamBitmapHandler
    // to create instances
    //
    TiffStreamBitmapHandler(
        const IWICImagingFactory_t  &pWICFactory,
        const IPrintWriteStream_t   &pStream
        );
};

} // namespace xpsrasfilter
