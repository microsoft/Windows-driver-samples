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































// Define custom test resource properties
namespace WEX {
    namespace TestExecution {
        namespace TestResourceProperty
        {
            static const wchar_t c_szMode[] = L"Mode";
            static const wchar_t c_szPin[] = L"Pin";
        }
    }
}

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
