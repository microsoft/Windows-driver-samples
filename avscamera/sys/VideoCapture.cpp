/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        videocapture.cpp

    Abstract:

        Video Capture Pin implementation.  

        Handles construction and defines pin data ranges.

    History:

        created 3/8/2001

**************************************************************************/

#include "Common.h"
#include "ntintsafe.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CVideoCapturePin::CVideoCapturePin(IN PKSPIN Pin) :
    CCapturePin (Pin)
{
    PAGED_CODE();

}


/*************************************************

Routine Description:

    Create a new capture pin.  This is the creation dispatch for
    the video capture pin.

Arguments:

    Pin -
        The pin being created

    Irp -
        The creation Irp

Return Value:

    Success / Failure

**************************************************/

NTSTATUS 
CVideoCapturePin::
DispatchCreate(
    IN PKSPIN Pin, 
    IN PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("(Pin=%d)", Pin->Id);

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter* pFilter = reinterpret_cast <CCaptureFilter*>(KsPinGetParentFilter(Pin)->Context);
    CVideoCapturePin *CapPin = new (NonPagedPoolNx) CVideoCapturePin (Pin);

    if( !CapPin )
    {
        // Fail if we couldn't create the pin.
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = 
            CapPin->Initialize();
    }

    if( NT_SUCCESS (Status) )
    {
        //
        // Adjust the stream header size.  The video packets have extended
        // header info (KS_FRAME_INFO).
        //
        pFilter->setPin(CapPin, Pin->Id);
    }
    else
    {
        //  Clean up.
        delete CapPin;
    }

    DBG_LEAVE("(Pin=%d)=0x%08X", Pin->Id, Status);
    return Status;
}

/**************************************************************************

    DISPATCH AND DESCRIPTOR LAYOUT

**************************************************************************/

//
// FormatRGB24Bpp_Capture:
//
// This is the data range description of the RGB24 capture format we support.
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureQVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X *D_Y * 3,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X,D_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X,D_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        D_X, D_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 3 * 30 * D_X *D_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        D_X *D_Y * 3 * 8 * 30,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X *D_Y * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 3,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 3 * 30 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 3 * 8 * 30,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                                // LONG  biWidth;
        DMAX_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
// FormatYUY2_Capture:
//
// This is the data range description of the YUY2 format we support.
//
const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureQVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        D_X *D_Y * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X, D_Y,       // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X, D_Y,       // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        D_X, D_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 30 * D_X * D_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        D_X *D_Y * 2 * 8 * 30,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        D_X *D_Y * 2,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X *DMAX_Y * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X, DMAX_Y, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X, DMAX_Y, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 30 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X *DMAX_Y * 2 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X *DMAX_Y * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
//60 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_60fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 3,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        166833,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 60 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 3 * 60 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 3 * 8 * 60,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        166833,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 3,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_60fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X *DMAX_Y * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X, DMAX_Y, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X, DMAX_Y, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        166833,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 60 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 60 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X *DMAX_Y * 2 * 8 * 60,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        166833,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X *DMAX_Y * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
//90 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_90fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 3,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                     // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        111111,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 90 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 3 * 90 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 3 * 8 * 90,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        111111,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 3,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_90fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X *DMAX_Y * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X, DMAX_Y, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X, DMAX_Y, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        111111,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 90 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 90 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X *DMAX_Y * 2 * 8 * 90,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        111111,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X *DMAX_Y * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
//120 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_120fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 3,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,          // InputSize, (the inherent size of the incoming signal
                                //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,          // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,          // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        83333,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 120 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 3 * 120 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 3 * 8 * 120,       // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        83333,                              // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 3,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_120fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X *DMAX_Y * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X, DMAX_Y, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X, DMAX_Y, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        83333,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 120 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 120 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X *DMAX_Y * 2 * 8 * 120,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        83333,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X *DMAX_Y * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureQVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X *D_Y * 4,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X,D_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X,D_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        D_X, D_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 30, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 4 * 30 * D_X *D_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        D_X *D_Y * 4 * 8 * 30,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 30,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X *D_Y * 4,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 30, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 30 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 4 * 30 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 4 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 30,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                                // LONG  biWidth;
        DMAX_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 4,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
//60 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_60fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 60, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 60 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 4 * 60 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 4 * 8 * 60,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 60,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

//
//90 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_90fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 90, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 90 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 4 * 90 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 4 * 8 * 90,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 90,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

//
//120 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_120fps =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X *DMAX_Y * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X,DMAX_Y,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X,DMAX_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X,DMAX_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X, DMAX_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 120,// MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 120 * DMAX_X * DMAX_Y,  // MinBitsPerSecond;
        8 * 4 * 120 * DMAX_X *DMAX_Y    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        DMAX_X *DMAX_Y * 4 * 8 * 120,       // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 120,                    // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X *DMAX_Y * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

// Start of Overscan Dataranges. These are related to the above mediatypes, but take into account the increased sizes required.
// For simulation purposes, we are assuming an additional 20 % on the Width and 20 % on the Height
//
// FormatRGB24Bpp_Capture:
//
// This is the data range description of the RGB24 capture format we support.
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureQVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X_OS *D_Y_OS * 3,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X_OS, D_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X_OS, D_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X_OS, D_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X_OS, D_Y_OS, // MinOutputSize, smallest bitmap stream can produce
        D_X_OS, D_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * D_X_OS * D_Y_OS,  // MinBitsPerSecond;
        8 * 3 * 30 * D_X_OS *D_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        D_X_OS *D_Y_OS * 3 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X_OS,                                // LONG  biWidth;
        D_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X_OS *D_Y_OS * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 3,                   // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 3 * 30 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 3 * 8 * 30,     // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                          // LONG  biWidth;
        DMAX_Y_OS,                          // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 3,           // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
// FormatYUY2_Capture:
//
// This is the data range description of the YUY2 format we support.
//
const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureQVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        D_X_OS *D_Y_OS * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X_OS, D_Y_OS, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X_OS, D_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X_OS, D_Y_OS, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X_OS, D_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        D_X_OS, D_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * D_X_OS * D_Y_OS,  // MinBitsPerSecond;
        8 * 2 * 30 * D_X_OS * D_Y_OS,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        D_X_OS *D_Y_OS * 2 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X_OS,                             // LONG  biWidth;
        D_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        D_X_OS *D_Y_OS * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X_OS *DMAX_Y_OS * 2,               // SampleSize
        0,                                      // Reserved
        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS, // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS, // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 2 * 30 * DMAX_X_OS * DMAX_Y_OS,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 2 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
//60 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_60fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 3,                   // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        166833,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 60 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 3 * 60 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 3 * 8 * 60,     // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        166833,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                                // LONG  biWidth;
        DMAX_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_60fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X_OS *DMAX_Y_OS * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        166833,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 60 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 2 * 60 * DMAX_X_OS * DMAX_Y_OS,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 2 * 8 * 60,  // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        166833,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                          // LONG  biWidth;
        DMAX_Y_OS,                          // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 2,           // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
//90 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_90fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 3,                   // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        111111,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 90 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 3 * 90 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 3 * 8 * 90,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        111111,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                                // LONG  biWidth;
        DMAX_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_90fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X_OS *DMAX_Y_OS * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        111111,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 90 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 2 * 90 * DMAX_X_OS * DMAX_Y_OS,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 2 * 8 * 90,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        111111,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
//120 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB24Bpp_CaptureVGA_120fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 3,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        83333,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 120 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 3 * 120 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 3 * 8 * 120,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        83333,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                                // LONG  biWidth;
        DMAX_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 3,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatYUY2_CaptureVGA_120fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X_OS *DMAX_Y_OS * 2,                     // SampleSize
        0,                                      // Reserved
        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS, // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        83333,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 120 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 2 * 120 * DMAX_X_OS * DMAX_Y_OS,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 2 * 8 * 120,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        83333,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 2,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureQVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X_OS *D_Y_OS * 4,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X_OS, D_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        D_X_OS, D_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X_OS, D_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        D_X_OS, D_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        D_X_OS, D_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 30, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 30 * D_X_OS * D_Y_OS,  // MinBitsPerSecond;
        8 * 4 * 30 * D_X_OS *D_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        D_X_OS *D_Y_OS * 4 * 8 * 30,              // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 30,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X_OS,                                // LONG  biWidth;
        D_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X_OS *D_Y_OS * 4,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGAOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 30, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 30 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 4 * 30 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 4 * 8 * 30,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 30,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                                // LONG  biWidth;
        DMAX_Y_OS,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 4,                       // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};



//
//60 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_60fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 4,                               // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 60, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 60 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 4 * 60 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 4 * 8 * 60,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 60,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

//
//90 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_90fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 90, // MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 90 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 4 * 90 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 4 * 8 * 90,        // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 90,                     // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

//
//120 fps capture
//
const
KS_DATARANGE_VIDEO
FormatRGB32Bpp_CaptureVGA_120fpsOverscan =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        DMAX_X_OS *DMAX_Y_OS * 4,                         // SampleSize
        0,                                          // Reserved

        STATICGUIDOF(KSDATAFORMAT_TYPE_VIDEO),             // aka. MEDIATYPE_Video
        0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20,
        0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB32
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)     // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS
    //
    {
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X_OS, DMAX_Y_OS,        // InputSize, (the inherent size of the incoming signal
        //             with every digitized pixel unique)
        DMAX_X_OS, DMAX_Y_OS,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X_OS, DMAX_Y_OS,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect
        1,              // CropAlignY;
        DMAX_X_OS, DMAX_Y_OS,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X_OS, DMAX_Y_OS,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX
        0,              // ShrinkTapsY
        ONESECOND / 120,// MinFrameInterval, 100 nS units
        ONESECOND,      // MaxFrameInterval, 100 nS units
        8 * 4 * 120 * DMAX_X_OS * DMAX_Y_OS,  // MinBitsPerSecond;
        8 * 4 * 120 * DMAX_X_OS *DMAX_Y_OS    // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                            // RECT  rcSource;
        0, 0, 0, 0,                            // RECT  rcTarget;
        DMAX_X_OS *DMAX_Y_OS * 4 * 8 * 120,       // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        ONESECOND / 120,                    // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X_OS,                             // LONG  biWidth;
        DMAX_Y_OS,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        32,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        DMAX_X_OS *DMAX_Y_OS * 4,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
// CapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
//
DEFINE_CAMERA_KSPIN_DISPATCH( VideoCapturePinDispatch, CVideoCapturePin );

//
// CapturePinAllocatorFraming:
//
// This is the simple framing structure for the capture pin.  Note that this
// will be modified via KsEdit when the actual capture format is determined.
//
DECLARE_SIMPLE_FRAMING_EX (
    VideoCapturePinAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    2,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);


//
// CapturePinDataRanges:
//
// This is the list of data ranges supported on the capture pin.  We support
// RGB24, YUY2, and RGB32
//
const
PKSDATARANGE
VideoCapturePinDataRanges [VIDEO_CAPTURE_PIN_DATA_RANGE_COUNT] =
{
    (PKSDATARANGE) &FormatYUY2_CaptureQVGA,
    (PKSDATARANGE) &FormatYUY2_CaptureQVGAOverscan,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA,
    (PKSDATARANGE) &FormatYUY2_CaptureVGAOverscan,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureQVGA,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureQVGAOverscan,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGAOverscan,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_60fpsOverscan,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_60fpsOverscan,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_90fpsOverscan,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_90fpsOverscan,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_120fpsOverscan,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_120fpsOverscan,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGAOverscan,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_60fpsOverscan,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_90fpsOverscan,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_120fpsOverscan,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureQVGA,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureQVGAOverscan,
};

const
PKSDATARANGE
VideoPreviewPinDataRanges[VIDEO_PREVIEW_PIN_DATA_RANGE_COUNT] =
{
    (PKSDATARANGE) &FormatYUY2_CaptureQVGA,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureQVGA,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatYUY2_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatRGB24Bpp_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_60fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_90fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureVGA_120fps,
    (PKSDATARANGE) &FormatRGB32Bpp_CaptureQVGA,
};