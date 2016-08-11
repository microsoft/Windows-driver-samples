/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        Formats.h

    Abstract:

        A list of available Image and Video formats.

    History:

        created 2/12/2016

**************************************************************************/

#pragma once

#include "capture.h"

#define DEFINE_DATARANGE_VIDEO(_RANGE_NAME_, _Subtype_, _X_, _Y_, _Rate_, _Planes_, _BitCount_, _Compression_) \
const                                                                                           \
KS_DATARANGE_VIDEO                                                                              \
_RANGE_NAME_ =                                                                                  \
{                                                                                               \
                                                                                                \
    /*                                                                                          \
    // KSDATARANGE                                                                              \
    */                                                                                          \
    {                                                                                           \
        sizeof( KS_DATARANGE_VIDEO ),               /* FormatSize                      */       \
        0,                                          /* Flags                           */       \
        ULONG((ULONGLONG(_X_) *_Y_ * _BitCount_)/8),/* SampleSize                      */       \
        0,                                          /* Reserved                        */       \
                                                                                                \
        STATICGUIDOF( KSDATAFORMAT_TYPE_VIDEO ),    /* aka. MEDIATYPE_Video            */       \
        _Subtype_,                                  /* aka. MEDIASUBTYPE_RGB24,        */       \
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) /* aka. FORMAT_VideoInfo      */       \
    },                                                                                          \
                                                                                                \
    TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)           */       \
    FALSE,              /* BOOL,  bTemporalCompression (all I frames?)                 */       \
    0,                  /* Reserved (was StreamDescriptionFlags)                       */       \
    0,                  /* Reserved (was MemoryAllocationFlags                         */       \
                        /*           (KS_VIDEO_ALLOC_*))                               */       \
                        /*                                                             */       \
                        /* _KS_VIDEO_STREAM_CONFIG_CAPS                                */       \
                        /*                                                             */       \
    {                                                                                           \
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), /* GUID                      */       \
        KS_AnalogVideo_None,                            /* AnalogVideoStandard         */       \
        _X_,_Y_,        /* InputSize, (the inherent size of the incoming signal        */       \
                        /*             with every digitized pixel unique)              */       \
        _X_,_Y_,        /* MinCroppingSize, smallest rcSrc cropping rect allowed       */       \
        _X_,_Y_,        /* MaxCroppingSize, largest  rcSrc cropping rect allowed       */       \
        8,              /* CropGranularityX, granularity of cropping size              */       \
        1,              /* CropGranularityY                                            */       \
        8,              /* CropAlignX, alignment of cropping rect                      */       \
        1,              /* CropAlignY;                                                 */       \
        _X_, _Y_,       /* MinOutputSize, smallest bitmap stream can produce           */       \
        _X_, _Y_,       /* MaxOutputSize, largest  bitmap stream can produce           */       \
        8,              /* OutputGranularityX, granularity of output bitmap size       */       \
        1,              /* OutputGranularityY;                                         */       \
        0,              /* StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)        */       \
        0,              /* StretchTapsY                                                */       \
        0,              /* ShrinkTapsX                                                 */       \
        0,              /* ShrinkTapsY                                                 */       \
        (ONESECOND / _Rate_),         /* MinFrameInterval, 100 nS units                */       \
        640000000,      /* MaxFrameInterval, 100 nS units                              */       \
        ULONG(min(ULONG_MAX,(ULONGLONG(_X_) *_Y_ * _BitCount_)*_Rate_)),/* MinBitsPerSecond;*/  \
        ULONG(min(ULONG_MAX,(ULONGLONG(_X_) *_Y_ * _BitCount_)*_Rate_)) /* MaxBitsPerSecond;*/  \
    },                                                                                          \
                                                                                                \
    /*                                                                                          \
    // KS_VIDEOINFOHEADER (default format)                                                      \
    */                                                                                          \
    {                                                                                           \
        0,0,0,0,                            /* RECT  rcSource;                        */        \
        0,0,0,0,                            /* RECT  rcTarget;                        */        \
        ULONG(min(ULONG_MAX,(ULONGLONG(_X_) *_Y_ * _BitCount_)*_Rate_)),/* DWORD dwBitRate;*/   \
        0L,                                 /* DWORD dwBitErrorRate;                  */        \
        (ONESECOND / _Rate_),               /* REFERENCE_TIME  AvgTimePerFrame;       */        \
        sizeof( KS_BITMAPINFOHEADER ),      /* DWORD biSize;                          */        \
        _X_,                                /* LONG  biWidth;                         */        \
        _Y_,                                /* LONG  biHeight;                        */        \
        _Planes_,                           /* WORD  biPlanes;                        */        \
        _BitCount_,                         /* WORD  biBitCount;                      */        \
        _Compression_,                      /* DWORD biCompression;                   */        \
        ULONG((ULONGLONG(_X_) *_Y_ * _BitCount_)/8),/* DWORD biSizeImage;             */        \
        0,                                  /* LONG  biXPelsPerMeter;                 */        \
        0,                                  /* LONG  biYPelsPerMeter;                 */        \
        0,                                  /* DWORD biClrUsed;                       */        \
        0                                   /* DWORD biClrImportant;                  */        \
    }                                                                                           \
};                                                                                              \


extern const KS_DATARANGE_VIDEO FormatYUY2Image_Capture;
extern const KS_DATARANGE_VIDEO FormatNV12Image_Capture;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureQVGA;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureQVGAOverscan;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGAOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureQVGA;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureQVGAOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGAOverscan;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_60fps;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_60fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_60fps;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_60fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_90fps;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_90fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_90fps;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_90fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_120fps;
extern const KS_DATARANGE_VIDEO FormatYUY2_CaptureVGA_120fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_120fps;
extern const KS_DATARANGE_VIDEO FormatRGB24Bpp_CaptureVGA_120fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGAOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_60fps;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_60fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_90fps;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_90fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_120fps;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureVGA_120fpsOverscan;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureQVGA;
extern const KS_DATARANGE_VIDEO FormatRGB32Bpp_CaptureQVGAOverscan;

extern const PKSDATARANGE VideoCapturePinDataRanges[VIDEO_CAPTURE_PIN_DATA_RANGE_COUNT];
extern const PKSDATARANGE VideoPreviewPinDataRanges[VIDEO_PREVIEW_PIN_DATA_RANGE_COUNT];
extern const PKSDATARANGE ImageCapturePinDataRanges[IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT];
