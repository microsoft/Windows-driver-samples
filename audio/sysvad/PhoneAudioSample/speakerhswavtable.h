/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhswavtable.h

Abstract:

    Declaration of wave miniport tables for the speaker (external: headset).

--*/

#ifndef _SYSVAD_SPEAKERHSWAVTABLE_H_
#define _SYSVAD_SPEAKERHSWAVTABLE_H_


// To keep the code simple assume device supports only 48KHz, 16-bit, stereo (PCM and NON-PCM)

#define SPEAKERHS_DEVICE_MAX_CHANNELS                   2       // Max Channels.

#define SPEAKERHS_HOST_MAX_CHANNELS                     2       // Max Channels.
#define SPEAKERHS_HOST_MIN_BITS_PER_SAMPLE              16      // Min Bits Per Sample
#define SPEAKERHS_HOST_MAX_BITS_PER_SAMPLE              16      // Max Bits Per Sample
#define SPEAKERHS_HOST_MIN_SAMPLE_RATE                  24000   // Min Sample Rate
#define SPEAKERHS_HOST_MAX_SAMPLE_RATE                  96000   // Max Sample Rate

#define SPEAKERHS_OFFLOAD_MAX_CHANNELS                  2       // Max Channels.
#define SPEAKERHS_OFFLOAD_MIN_BITS_PER_SAMPLE           16      // Min Bits Per Sample
#define SPEAKERHS_OFFLOAD_MAX_BITS_PER_SAMPLE           16      // Max Bits Per Sample
#define SPEAKERHS_OFFLOAD_MIN_SAMPLE_RATE               44100   // Min Sample Rate
#define SPEAKERHS_OFFLOAD_MAX_SAMPLE_RATE               48000   // Max Sample Rate

#define SPEAKERHS_LOOPBACK_MAX_CHANNELS                 2       // Max Channels.
#define SPEAKERHS_LOOPBACK_MIN_BITS_PER_SAMPLE          16      // Min Bits Per Sample
#define SPEAKERHS_LOOPBACK_MAX_BITS_PER_SAMPLE          16      // Max Bits Per Sample
#define SPEAKERHS_LOOPBACK_MIN_SAMPLE_RATE              24000   // Min Sample Rate
#define SPEAKERHS_LOOPBACK_MAX_SAMPLE_RATE              48000   // Max Sample Rate

#define SPEAKERHS_DOLBY_DIGITAL_MAX_CHANNELS            2       // Max Channels.
#define SPEAKERHS_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE     16      // Min Bits Per Sample
#define SPEAKERHS_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE     16      // Max Bits Per Sample
#define SPEAKERHS_DOLBY_DIGITAL_MIN_SAMPLE_RATE         44100   // Min Sample Rate
#define SPEAKERHS_DOLBY_DIGITAL_MAX_SAMPLE_RATE         44100   // Max Sample Rate

//
// Max # of pin instances.
//
#define SPEAKERHS_MAX_INPUT_SYSTEM_STREAMS              2       // Raw + Default streams
#define SPEAKERHS_MAX_INPUT_OFFLOAD_STREAMS             MAX_INPUT_OFFLOAD_STREAMS
#define SPEAKERHS_MAX_OUTPUT_LOOPBACK_STREAMS           MAX_OUTPUT_LOOPBACK_STREAMS

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHsAudioEngineSupportedDeviceFormats[] =
{
    { // 0 : First entry in htis table is the default format for the audio engine
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
                24000,
                96000,
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
    { // 4
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHsHostPinSupportedDeviceFormats[] =
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
                24000,
                96000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
                32000,
                128000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
    { // 4
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
    { // 5
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHsOffloadPinSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHsLoopbackPinSupportedDeviceFormats[] =
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
                24000,
                96000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
                32000,
                128000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
    }
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT SpeakerHsHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &SpeakerHsHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerHsHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerHsHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MOVIE,
        &SpeakerHsHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &SpeakerHsHostPinSupportedDeviceFormats[0].DataFormat // 24KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION,
        &SpeakerHsHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    }
};

static
MODE_AND_DEFAULT_FORMAT SpeakerHsOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerHsOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerHsOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES SpeakerHsPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        SpeakerHsHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHsHostPinSupportedDeviceFormats),
        SpeakerHsHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerHsHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        SpeakerHsOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHsOffloadPinSupportedDeviceFormats),
        SpeakerHsOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerHsOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        SpeakerHsLoopbackPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHsLoopbackPinSupportedDeviceFormats),
        NULL,   // loopback doesn't support modes.
        0
    },
    {
        BridgePin,
        NULL,
        0,
        NULL,
        0
    },
    {
        NoPin,      // For convenience, offload engine device formats appended here
        SpeakerHsAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHsAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO SpeakerHsPinDataRangesStream[] =
{
    { // 0
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPEAKERHS_HOST_MAX_CHANNELS,           
        SPEAKERHS_HOST_MIN_BITS_PER_SAMPLE,    
        SPEAKERHS_HOST_MAX_BITS_PER_SAMPLE,    
        SPEAKERHS_HOST_MIN_SAMPLE_RATE,            
        SPEAKERHS_HOST_MAX_SAMPLE_RATE             
    },
    { // 1
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPEAKERHS_OFFLOAD_MAX_CHANNELS,           
        SPEAKERHS_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        SPEAKERHS_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        SPEAKERHS_OFFLOAD_MIN_SAMPLE_RATE,
        SPEAKERHS_OFFLOAD_MAX_SAMPLE_RATE
    },
    { // 2
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPEAKERHS_LOOPBACK_MAX_CHANNELS,           
        SPEAKERHS_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        SPEAKERHS_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        SPEAKERHS_LOOPBACK_MIN_SAMPLE_RATE,
        SPEAKERHS_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE SpeakerHsPinDataRangePointersStream[] =
{
    PKSDATARANGE(&SpeakerHsPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE SpeakerHsPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&SpeakerHsPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE SpeakerHsPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&SpeakerHsPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE SpeakerHsPinDataRangesBridge[] =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE SpeakerHsPinDataRangePointersBridge[] =
{
    &SpeakerHsPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpeakerHsOffloadPin[] =
{
    {
        &KSPROPSETID_OffloadPin,  // define new property set
        KSPROPERTY_OFFLOAD_PIN_GET_STREAM_OBJECT_POINTER, // define properties
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_OffloadPin
    },
    {
        &KSPROPSETID_OffloadPin,  // define new property set
        KSPROPERTY_OFFLOAD_PIN_VERIFY_STREAM_OBJECT_POINTER, // define properties
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_OffloadPin
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHsOffloadPin, PropertiesSpeakerHsOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR SpeakerHsWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (renderer) KSPIN_WAVE_RENDER_SINK_SYSTEM
    {
        SPEAKERHS_MAX_INPUT_SYSTEM_STREAMS,
        SPEAKERHS_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHsPinDataRangePointersStream),
            SpeakerHsPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        SPEAKERHS_MAX_INPUT_OFFLOAD_STREAMS,
        SPEAKERHS_MAX_INPUT_OFFLOAD_STREAMS, 
        0,
        &AutomationSpeakerHsOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHsPinDataRangePointersOffloadStream),
            SpeakerHsPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_LOOPBACK
    {
        SPEAKERHS_MAX_OUTPUT_LOOPBACK_STREAMS,
        SPEAKERHS_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHsPinDataRangePointersLoopbackStream),
            SpeakerHsPinDataRangePointersLoopbackStream,
            KSPIN_DATAFLOW_OUT,              
            KSPIN_COMMUNICATION_SINK,
            &KSNODETYPE_AUDIO_LOOPBACK,
            NULL,
            0
        }
    },
    // Wave Out Bridge Pin (Renderer) KSPIN_WAVE_RENDER_SOURCE
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
            SIZEOF_ARRAY(SpeakerHsPinDataRangePointersBridge),
            SpeakerHsPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR SpeakerHsWaveMiniportNodes[] =
{
    // KSNODE_WAVE_AUDIO_ENGINE
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_AUDIO_ENGINE,   // Type  KSNODETYPE_AUDIO_ENGINE
        NULL                        // Name
    }
};
//=============================================================================
//
//                   ----------------------------      
//                   |                          |      
//  System Pin   0-->|                          |--> 2 Loopback Pin
//                   |   HW Audio Engine node   |      
//  Offload Pin  1-->|                          |--> 3 KSPIN_WAVE_RENDER_SOURCE
//                   |                          |      
//                   ----------------------------       
static
PCCONNECTION_DESCRIPTOR SpeakerHsWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerHsWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHsWaveFilter, PropertiesSpeakerHsWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpeakerHsWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationSpeakerHsWaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(SpeakerHsWaveMiniportPins),        // PinCount
    SpeakerHsWaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(SpeakerHsWaveMiniportNodes),       // NodeCount
    SpeakerHsWaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(SpeakerHsWaveMiniportConnections), // ConnectionCount
    SpeakerHsWaveMiniportConnections,               // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_SPEAKERHSWAVTABLE_H_

