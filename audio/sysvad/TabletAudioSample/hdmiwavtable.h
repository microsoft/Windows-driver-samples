/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    hdmiwavtable.h

Abstract:

    Declaration of wave miniport tables for the hdmi endpoint.

    Note: the HDMI in this sample shows how to do h/w loopback 
          for non-offloaded devices.

--*/

#ifndef _SYSVAD_HDMIWAVTABLE_H_
#define _SYSVAD_HDMIWAVTABLE_H_


//=============================================================================
// Defines
//=============================================================================


#define HDMI_DEVICE_MAX_CHANNELS                 2       // Max Channels.

#define HDMI_HOST_MAX_CHANNELS                   2       // Max Channels.
#define HDMI_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define HDMI_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define HDMI_HOST_MIN_SAMPLE_RATE                44100   // Min Sample Rate
#define HDMI_HOST_MAX_SAMPLE_RATE                96000   // Max Sample Rate

#define HDMI_LOOPBACK_MAX_CHANNELS               HDMI_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define HDMI_LOOPBACK_MIN_BITS_PER_SAMPLE        HDMI_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define HDMI_LOOPBACK_MAX_BITS_PER_SAMPLE        HDMI_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define HDMI_LOOPBACK_MIN_SAMPLE_RATE            HDMI_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define HDMI_LOOPBACK_MAX_SAMPLE_RATE            HDMI_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define HDMI_DOLBY_DIGITAL_MAX_CHANNELS          2       // Max Channels.
#define HDMI_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   16      // Min Bits Per Sample
#define HDMI_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define HDMI_DOLBY_DIGITAL_MIN_SAMPLE_RATE       44100   // Min Sample Rate
#define HDMI_DOLBY_DIGITAL_MAX_SAMPLE_RATE       48000   // Max Sample Rate

#define HDMI_DOLBY_MAT_MAX_CHANNELS              8       // Max Channels.
#define HDMI_DOLBY_MAT_MIN_BITS_PER_SAMPLE       16      // Min Bits Per Sample
#define HDMI_DOLBY_MAT_MAX_BITS_PER_SAMPLE       16      // Max Bits Per Sample
#define HDMI_DOLBY_MAT_MIN_SAMPLE_RATE           192000  // Min Sample Rate
#define HDMI_DOLBY_MAT_MAX_SAMPLE_RATE           192000  // Max Sample Rate

#define HDMI_DTS_MAX_CHANNELS                    2       // Max Channels.
#define HDMI_DTS_MIN_BITS_PER_SAMPLE             16      // Min Bits Per Sample
#define HDMI_DTS_MAX_BITS_PER_SAMPLE             16      // Max Bits Per Sample
#define HDMI_DTS_MIN_SAMPLE_RATE                 48000   // Min Sample Rate
#define HDMI_DTS_MAX_SAMPLE_RATE                 48000   // Max Sample Rate

#define HDMI_DTS_HD_MAX_CHANNELS                 8       // Max Channels.
#define HDMI_DTS_HD_MIN_BITS_PER_SAMPLE          16      // Min Bits Per Sample
#define HDMI_DTS_HD_MAX_BITS_PER_SAMPLE          16      // Max Bits Per Sample
#define HDMI_DTS_HD_MIN_SAMPLE_RATE              192000  // Min Sample Rate
#define HDMI_DTS_HD_MAX_SAMPLE_RATE              192000  // Max Sample Rate

//
// Max # of pin instances.
//
#define HDMI_MAX_INPUT_SYSTEM_STREAMS            2
#define HDMI_MAX_OUTPUT_LOOPBACK_STREAMS         MAX_OUTPUT_LOOPBACK_STREAMS


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE HdmiHostPinSupportedDeviceFormats[] =
{
    { // 0
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                44100,
                176400,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_STEREO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 1
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                48000,
                192000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_STEREO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 2
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                88200,
                352800,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_STEREO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 3
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                96000,
                384000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_STEREO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 4
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                44100,
                176400,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_5POINT1_SURROUND,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)
        }
    },
    { // 5
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                48000,
                192000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_5POINT1_SURROUND,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)
        }
    },
    { // 6
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)
        }
    },
    { // 7
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1_SURROUND,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)
        }
    },
    { // 8
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                2,
                48000,
                192000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_5POINT1_SURROUND,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS)
        }
    },
    { // 9
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)
        }
    },
    { // 10
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E1),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E1)
        }
    },
    { // 11
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E2),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E2)
        }
    },
    { // 12
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT20),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT20)
        }
    },
    { // 13
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT21),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                8,
                192000,
                3072000,
                16,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_7POINT1,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT21)
        }
    }
};

//
// Supported modes (only on streaming pins).
// Note: This pin does not support KSPROPERTY_PIN_PROPOSEDATAFORMAT2 
//       since none of the modes on this endpoint have a default format.
//
static
MODE_AND_DEFAULT_FORMAT HdmiHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        NULL, // just an example of no default format for this endpoint/mode   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES HdmiPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        HdmiHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(HdmiHostPinSupportedDeviceFormats),
        HdmiHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(HdmiHostPinSupportedDeviceModes)
    },
    {
        RenderLoopbackPin,
        HdmiHostPinSupportedDeviceFormats,   // Must support all the formats supported by host pin
        SIZEOF_ARRAY(HdmiHostPinSupportedDeviceFormats),
        NULL,   // loopback doesn't support modes.
        0
    },
    {
        BridgePin,
        NULL,
        0,
        NULL,
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO HdmiPinDataRangesStream[] =
{
    { // 0 - PCM host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_HOST_MAX_CHANNELS,           
        HDMI_HOST_MIN_BITS_PER_SAMPLE,    
        HDMI_HOST_MAX_BITS_PER_SAMPLE,    
        HDMI_HOST_MIN_SAMPLE_RATE,            
        HDMI_HOST_MAX_SAMPLE_RATE             
    },
    { // 1 - PCM loopback
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_LOOPBACK_MAX_CHANNELS,           
        HDMI_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        HDMI_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        HDMI_LOOPBACK_MIN_SAMPLE_RATE,
        HDMI_LOOPBACK_MAX_SAMPLE_RATE
    },
    { // 2 - DOLBY-DIGITAL host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DOLBY_DIGITAL_MAX_CHANNELS,
        HDMI_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE,
        HDMI_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE,
        HDMI_DOLBY_DIGITAL_MIN_SAMPLE_RATE,
        HDMI_DOLBY_DIGITAL_MAX_SAMPLE_RATE
    },
    { // 3 - DOLBY-MLP host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DOLBY_MAT_MAX_CHANNELS,
        HDMI_DOLBY_MAT_MIN_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MAX_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MIN_SAMPLE_RATE,
        HDMI_DOLBY_MAT_MAX_SAMPLE_RATE
    },
    { // 4 - DTS host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DTS_MAX_CHANNELS,
        HDMI_DTS_MIN_BITS_PER_SAMPLE,
        HDMI_DTS_MAX_BITS_PER_SAMPLE,
        HDMI_DTS_MIN_SAMPLE_RATE,
        HDMI_DTS_MAX_SAMPLE_RATE
    },
    { // 5 - DTS-HD host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DTS_HD_MAX_CHANNELS,
        HDMI_DTS_HD_MIN_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MAX_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MIN_SAMPLE_RATE,
        HDMI_DTS_HD_MAX_SAMPLE_RATE
    },
    { // 6 - DTS:X E1 host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E1),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DTS_HD_MAX_CHANNELS,
        HDMI_DTS_HD_MIN_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MAX_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MIN_SAMPLE_RATE,
        HDMI_DTS_HD_MAX_SAMPLE_RATE
    },
    { // 7 - DTS:X E2 host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E2),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DTS_HD_MAX_CHANNELS,
        HDMI_DTS_HD_MIN_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MAX_BITS_PER_SAMPLE,
        HDMI_DTS_HD_MIN_SAMPLE_RATE,
        HDMI_DTS_HD_MAX_SAMPLE_RATE
    },
    { // 8 - DOLBY-MAT20 host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT20),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DOLBY_MAT_MAX_CHANNELS,
        HDMI_DOLBY_MAT_MIN_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MAX_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MIN_SAMPLE_RATE,
        HDMI_DOLBY_MAT_MAX_SAMPLE_RATE
    },
    { // 9 - DOLBY-MAT21 host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT21),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HDMI_DOLBY_MAT_MAX_CHANNELS,
        HDMI_DOLBY_MAT_MIN_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MAX_BITS_PER_SAMPLE,
        HDMI_DOLBY_MAT_MIN_SAMPLE_RATE,
        HDMI_DOLBY_MAT_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE HdmiPinDataRangePointersStream[] =
{
    PKSDATARANGE(&HdmiPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[2]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[3]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[4]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[5]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[6]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[7]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[8]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&HdmiPinDataRangesStream[9]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE HdmiPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&HdmiPinDataRangesStream[1])
};

//=============================================================================
static
KSDATARANGE HdmiPinDataRangesBridge[] =
{
    { // 0
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 1
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 2
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 3
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 4
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 5
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E1),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 6
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DTSX_E2),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 7
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT20),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    { // 8
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MAT21),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE HdmiPinDataRangePointersBridge[] =
{
    &HdmiPinDataRangesBridge[0],
    &HdmiPinDataRangesBridge[1],
    &HdmiPinDataRangesBridge[2],
    &HdmiPinDataRangesBridge[3],
    &HdmiPinDataRangesBridge[4],
    &HdmiPinDataRangesBridge[5],
    &HdmiPinDataRangesBridge[6],
    &HdmiPinDataRangesBridge[7],
    &HdmiPinDataRangesBridge[8]
};


//=============================================================================
static
PCPIN_DESCRIPTOR HdmiWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_SYSTEM
    {
        HDMI_MAX_INPUT_SYSTEM_STREAMS,
        HDMI_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(HdmiPinDataRangePointersStream),
            HdmiPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_LOOPBACK
    {
        HDMI_MAX_OUTPUT_LOOPBACK_STREAMS,
        HDMI_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(HdmiPinDataRangePointersLoopbackStream),
            HdmiPinDataRangePointersLoopbackStream,
            KSPIN_DATAFLOW_OUT,              
            KSPIN_COMMUNICATION_SINK,
            &KSNODETYPE_AUDIO_LOOPBACK,
            NULL,
            0
        }
    },
    // Wave Out Bridge Pin (Renderer) KSPIN_WAVE_RENDER2_SOURCE
    {
        0,
        0,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(HdmiPinDataRangePointersBridge),
            HdmiPinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
};

//=============================================================================
static
PCPROPERTY_ITEM HdmiPropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHdmiVolume, HdmiPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM HdmiPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHdmiMute, HdmiPropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM HdmiPropertiesPeakMeter[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_PEAKMETER2,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  },
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_CPU_RESOURCES,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHdmiPeakMeter, HdmiPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR HdmiWaveMiniportNodes[] =
{
    // KSNODE_WAVE_SUM
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_SUM,            // Type
        NULL                        // Name
    },
    // KSNODE_WAVE_VOLUME
    {
      0,                            // Flags
      &AutomationHdmiVolume,        // AutomationTable
      &KSNODETYPE_VOLUME,           // Type
      &KSAUDFNAME_WAVE_VOLUME       // Name
    },
    // KSNODE_WAVE_MUTE
    {
      0,                            // Flags
      &AutomationHdmiMute,          // AutomationTable
      &KSNODETYPE_MUTE,             // Type
      &KSAUDFNAME_WAVE_MUTE         // Name
    },
    // KSNODE_WAVE_PEAKMETER
    {
      0,                            // Flags
      &AutomationHdmiPeakMeter,     // AutomationTable
      &KSNODETYPE_PEAKMETER,        // Type
      &KSAUDFNAME_PEAKMETER         // Name
    }
};

C_ASSERT( KSNODE_WAVE_SUM == 0 );
C_ASSERT( KSNODE_WAVE_VOLUME  == 1 );
C_ASSERT( KSNODE_WAVE_MUTE    == 2 );
C_ASSERT( KSNODE_WAVE_PEAKMETER == 3 );


//=============================================================================
//
// Note: 
// 1) A sum node can be used to 'sum' multiple streams associated to different
//    instances of the same pin.  
// 2) Any node output has an implicit 'tee' node associated with it. Thus in the 
//    scenario below it is ok for the peak meter node to have two identical outputs.
//
//                       ----------------------------------------------------------
//                       |                                                        |      
//    System Pin 0       |    |-----|    |--------|   |------|   |-----------|    |--> 1 Loopback Pin
//   [instances:0...n]-->| -->| Sum |--->| Volume |-->| Mute |-->| PeakMeter |--> |       
//                       |    |-----|    |--------|   |------|   |-----------|    |--> 2 KSPIN_WAVE_RENDER2_SOURCE
//                       |                                                        | 
//                       ----------------------------------------------------------

static
PCCONNECTION_DESCRIPTOR HdmiWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER2_SINK_SYSTEM, KSNODE_WAVE_SUM,            1 },
    { KSNODE_WAVE_SUM,          0,                              KSNODE_WAVE_VOLUME,         1 },
    { KSNODE_WAVE_VOLUME,       0,                              KSNODE_WAVE_MUTE,           1 },
    { KSNODE_WAVE_MUTE,         0,                              KSNODE_WAVE_PEAKMETER,      1 },
    { KSNODE_WAVE_PEAKMETER,    2,                              PCFILTER_NODE,              KSPIN_WAVE_RENDER2_SINK_LOOPBACK },
    { KSNODE_WAVE_PEAKMETER,    0,                              PCFILTER_NODE,              KSPIN_WAVE_RENDER2_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesHdmiWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHdmiWaveFilter, PropertiesHdmiWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR HdmiWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationHdmiWaveFilter,                      // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(HdmiWaveMiniportPins),             // PinCount
    HdmiWaveMiniportPins,                           // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(HdmiWaveMiniportNodes),            // NodeCount
    HdmiWaveMiniportNodes,                          // Nodes
    SIZEOF_ARRAY(HdmiWaveMiniportConnections),      // ConnectionCount
    HdmiWaveMiniportConnections,                    // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_HDMIWAVTABLE_H_

