/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        imagecapture.cpp

    Abstract:

        An implementation of CImageCapturePin

    History:

        created 3/8/2001

**************************************************************************/

#include "Common.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CImageCapturePin::
CImageCapturePin(
    _In_    PKSPIN Pin
)   : CCapturePin (Pin)
{
    PAGED_CODE();
}

NTSTATUS
CImageCapturePin::
DispatchCreate(
    _In_    PKSPIN Pin,
    _In_    PIRP Irp
)
/*++

Routine Description:

    Static member function called when a pin is created.  Instantiate a 
    CImageCapturePin and attach it to our filter object.

Arguments:

    Pin - 
        The KSPIN to wrap.

    Irp -
        The IRP assocated with this request.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    DBG_ENTER("(Pin=%d)", Pin->Id);

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter* pFilter = reinterpret_cast <CCaptureFilter*>(KsPinGetParentFilter(Pin)->Context);
    CImageCapturePin *CapPin = new (NonPagedPoolNx) CImageCapturePin (Pin);

    if( !CapPin )
    {
        // Fail if we couldn't create the pin.
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //  Query the filter for the PhotoMode so we can figure out how many frames should be allocated.
        CExtendedPhotoMode  Mode;
        Mode.PinId = Pin->Id;       // Use Pin ID from KS.
        Status = pFilter->GetPhotoMode( &Mode );

        //  CSensor::GetPhotoMode could fail if the framework gives us a Pin ID that doesn't match our descriptors...
        if( NT_SUCCESS(Status) )
        {
            //  Override the number of frames.
            CapPin->SetDesiredFrames( 
                ( (Mode.Flags & KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE) ? 
                    Mode.RequestedHistoryFrames() : 1)
                        + IMAGE_CAPTURE_PIN_MINIMUM_FRAMES );

            Status = 
                CapPin->Initialize();
        }
    }

    if (NT_SUCCESS (Status)) 
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


NTSTATUS
CImageCapturePin::
Close(
    _In_    PIRP Irp
)
{
    PAGED_CODE();

    Reset();

    return STATUS_SUCCESS;
}

// ImageCapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
// just a copy of CapturePinDispatch for now.
//
DEFINE_CAMERA_KSPIN_DISPATCH( ImageCapturePinDispatch, CImageCapturePin );


//
// ImagePinAllocatorFraming:
//
// This is the simple framing structure for the capture pin.  Note that this
// will be modified via KsEdit when the actual capture format is determined.
// just a copy of CapturePinAllocatorFraming for now.
//
DECLARE_SIMPLE_FRAMING_EX (
    ImageCapturePinAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    4,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);

const
KS_DATARANGE_VIDEO
FormatYUY2Image_Capture =
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

const
KS_DATARANGE_VIDEO
FormatNV12Image_Capture =
{

    //
    // KSDATARANGE
    //
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        0,                                      // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x3231564E, 0x0000, 0x0010, 0x80, 0x00,
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_NV12,
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
        12 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        12 * 30 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    },

    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource;
        0, 0, 0, 0,                         // RECT  rcTarget;
        DMAX_X *DMAX_Y * 12 * 30,           // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        12,                                 // WORD  biBitCount;
        FOURCC_NV12,                        // DWORD biCompression;
        DWORD(DMAX_X *DMAX_Y * 1.5),        // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


//
// CapturePinDataRanges:
//
// This is the list of data ranges supported on the capture pin.  We support
// two: one RGB24, and one YUY2.
//
const
PKSDATARANGE
ImageCapturePinDataRanges [IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT] =
{
    (PKSDATARANGE) &FormatYUY2Image_Capture,
    (PKSDATARANGE) &FormatNV12Image_Capture
};