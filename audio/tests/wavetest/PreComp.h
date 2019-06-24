// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// File Name:
//
//  PreComp.h
//
// Abstract:
//
//  Precompiled common headers
//
// -------------------------------------------------------------------------------

#include <basiclog.h>
#include <BasicLogHelper.h>
#include <mmsystem.h>
#include <atlbase.h>
#include <atlcom.h>
#include <initguid.h>

#include <AudioEngineEndpoint.h>
#include <AudioEngineEndpointP.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmdeviceapip.h>
#include <devicetopology.h>
#include <devicetopologyp.h> 
#include <propvarutil.h>

#include <wil\com.h>

#include <winioctl.h>
#include <KsLib.h>
#include <Waveutil.h>
#include <aecommon.h>
#include <ilog.h>
#include <shlflags.h>
#include <basicprintf.h>
#include <wextestclass.h>
#include <resourcelist.h>

// ------------------------------------------------------------------------------
// Log
#define LOG(log, ...) BasicLogPrintf( log, XMSG, 1, __VA_ARGS__ )
#define SKIP(log, ...) BasicLogPrintf( log, XSKIP, 1, __VA_ARGS__ )
#define WARN(log, ...) BasicLogPrintf( log, XWARN, 1, __VA_ARGS__ )
#define BLOCK(log, ...) BasicLogPrintf( log, XBLOCK, 1, __VA_ARGS__ )
#define ERR(log, ...) BasicLogPrintf( log, XFAIL, 1, __VA_ARGS__ )

// ------------------------------------------------------------------------------
// Data flow
enum STACKWISE_DATAFLOW { render, capture };

// ------------------------------------------------------------------------------
// Structs used to hold results of format records
typedef struct
{
    WAVEFORMATEXTENSIBLE wfxEx;
    UINT32 fundamentalPeriodInFrames;
    UINT32 defaultPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;
    UINT32 maxPeriodInFramesExtended;
} FORMAT_RECORD, *PFORMAT_RECORD;

// ----------------------------------------------------------
// Parameters for test sine tone
#define TEST_AMPLITUDE 1.0f
#define TEST_FREQUENCY 200.0f

// ----------------------------------------------------------
// Predefined format list for offload streaming
#define COUNT_FORMATS             4

static WAVEFORMATEXTENSIBLE ListofFormats[COUNT_FORMATS] =
{
    // 1
    {
        // Format
        {
            WAVE_FORMAT_EXTENSIBLE, // wFormatTag
            2, // nChannels
            44100, // nSamplesPerSec
            176400, // nAvgBytesPerSec
            4, // nBlockAlign
            16, // wBitsPerSample
            22 // cbSize
        },
        // Samples
        {
            16 // wValidBitsPerSample
        },
        KSAUDIO_SPEAKER_STEREO, // dwChannelMask
        KSDATAFORMAT_SUBTYPE_PCM // SubFormat
    },
    // 2
    {
        // Format
        {
            WAVE_FORMAT_EXTENSIBLE, // wFormatTag
            2, // nChannels
            48000, // nSamplesPerSec
            192000, // nAvgBytesPerSec
            4, // nBlockAlign
            16, // wBitsPerSample
            22 // cbSize
        },
        // Samples
        {
            16 // wValidBitsPerSample
        },
        KSAUDIO_SPEAKER_STEREO, // dwChannelMask
        KSDATAFORMAT_SUBTYPE_PCM // SubFormat
    },
    // 3
    {
        // Format
        {
            WAVE_FORMAT_EXTENSIBLE, // wFormatTag
            2, // nChannels
            88200, // nSamplesPerSec
            352800, // nAvgBytesPerSec
            4, // nBlockAlign
            16, // wBitsPerSample
            22 // cbSize
        },
        // Samples
        {
            16 // wValidBitsPerSample
        },
        KSAUDIO_SPEAKER_STEREO, // dwChannelMask
        KSDATAFORMAT_SUBTYPE_PCM // SubFormat
    },
    // 4
    {
        // Format
        {
            WAVE_FORMAT_EXTENSIBLE, // wFormatTag
            2, // nChannels
            96000, // nSamplesPerSec
            384000, // nAvgBytesPerSec
            4, // nBlockAlign
            16, // wBitsPerSample
            22 // cbSize
        },
        // Samples
        {
            16 // wValidBitsPerSample
        },
        KSAUDIO_SPEAKER_STEREO, // dwChannelMask
        KSDATAFORMAT_SUBTYPE_PCM // SubFormat
    },
};