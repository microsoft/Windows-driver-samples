/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        Formats.cpp

    Abstract:

        A list of available Image and Video formats.  

    History:

        created 2/12/2016

**************************************************************************/

#include "Common.h"
#include "ntintsafe.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//  Define 720p resolution.
#define D720P_W 1280
#define D720P_H 720

//  Add 20% for overscan.
#define D720P_W_OS (1280+(1280/5))
#define D720P_H_OS (720+(720/5))


/**************************************************************************

    DISPATCH AND DESCRIPTOR LAYOUT

**************************************************************************/

DEFINE_DATARANGE_VIDEO(
    FormatNV12_CaptureVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_NV12),
    DMAX_X, DMAX_Y,
    30,
    1,
    12,
    FOURCC_NV12)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureQVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    D_X, D_Y,
    30,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureQVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    D_X_OS, D_Y_OS,
    30,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X, DMAX_Y,
    30,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X_OS, DMAX_Y_OS,
    30,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureQVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    D_X, D_Y,
    30,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureQVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    D_X_OS, D_Y_OS,
    30,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X, DMAX_Y,
    30,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X_OS, DMAX_Y_OS,
    30,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_60fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X, DMAX_Y,
    60,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_60fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X_OS, DMAX_Y_OS,
    60,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_60fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X, DMAX_Y,
    60,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_60fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X_OS, DMAX_Y_OS,
    60,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_90fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X, DMAX_Y,
    90,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_90fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X_OS, DMAX_Y_OS,
    90,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_90fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X, DMAX_Y,
    90,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_90fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X_OS, DMAX_Y_OS,
    90,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_120fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X, DMAX_Y,
    120,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatYUY2_CaptureVGA_120fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_YUY2),
    DMAX_X_OS, DMAX_Y_OS,
    120,
    1,
    16,
    FOURCC_YUY2)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_120fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X, DMAX_Y,
    120,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB24Bpp_CaptureVGA_120fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB24),
    DMAX_X_OS, DMAX_Y_OS,
    120,
    1,
    24,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X, DMAX_Y,
    30,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X_OS, DMAX_Y_OS,
    30,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_60fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X, DMAX_Y,
    60,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_60fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X_OS, DMAX_Y_OS,
    60,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_90fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X, DMAX_Y,
    90,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_90fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X_OS, DMAX_Y_OS,
    90,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_120fps,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X, DMAX_Y,
    120,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureVGA_120fpsOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    DMAX_X_OS, DMAX_Y_OS,
    120,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureQVGA,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    D_X, D_Y,
    30,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatRGB32Bpp_CaptureQVGAOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RGB32),
    D_X_OS, D_Y_OS,
    30,
    1,
    32,
    KS_BI_RGB)

DEFINE_DATARANGE_VIDEO(
    FormatNV12_720p,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_NV12),
    D720P_W, D720P_H,
    30,
    1,
    12,
    FOURCC_NV12)

DEFINE_DATARANGE_VIDEO(
    FormatNV12_720pOverscan,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_NV12),
    D720P_W_OS, D720P_H_OS,
    30,
    1,
    12,
    FOURCC_NV12)

//
// CapturePinDataRanges:
//
// This is the list of data ranges supported on the capture pin.  We support
// two: one RGB24, and one YUY2.
//
const
PKSDATARANGE
ImageCapturePinDataRanges[ IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT ] =
{
    (PKSDATARANGE) &FormatYUY2_CaptureVGA,
    (PKSDATARANGE) &FormatNV12_CaptureVGA 
};

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
    (PKSDATARANGE) &FormatNV12_720p,
    (PKSDATARANGE) &FormatNV12_720pOverscan
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
    (PKSDATARANGE) &FormatNV12_720p
};
