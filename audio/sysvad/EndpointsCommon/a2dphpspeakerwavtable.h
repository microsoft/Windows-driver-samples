/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    s2dphppeakerwavtable.h

Abstract:

    Declaration of wave miniport tables for the A2DP Headphone speaker (external).

--*/

#ifndef _SYSVAD_A2DPHPSPEAKERWAVTABLE_H_
#define _SYSVAD_A2DPHPSPEAKERWAVTABLE_H_

//
// Function prototypes.
//
NTSTATUS PropertyHandler_A2dpHpWaveFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);



#define A2DPHPSPEAKER_DEVICE_MAX_CHANNELS                 2       // Max Channels.
                                                         
#define A2DPHPSPEAKER_HOST_MAX_CHANNELS                   2       // Max Channels.
#define A2DPHPSPEAKER_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define A2DPHPSPEAKER_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define A2DPHPSPEAKER_HOST_MIN_SAMPLE_RATE                24000   // Min Sample Rate
#define A2DPHPSPEAKER_HOST_MAX_SAMPLE_RATE                96000   // Max Sample Rate
                                                         
#define A2DPHPSPEAKER_OFFLOAD_MAX_CHANNELS                2       // Max Channels.
#define A2DPHPSPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE         16      // Min Bits Per Sample
#define A2DPHPSPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define A2DPHPSPEAKER_OFFLOAD_MIN_SAMPLE_RATE             44100   // Min Sample Rate
#define A2DPHPSPEAKER_OFFLOAD_MAX_SAMPLE_RATE             48000   // Max Sample Rate
                                                         
#define A2DPHPSPEAKER_LOOPBACK_MAX_CHANNELS               A2DPHPSPEAKER_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define A2DPHPSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE        A2DPHPSPEAKER_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define A2DPHPSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE        A2DPHPSPEAKER_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define A2DPHPSPEAKER_LOOPBACK_MIN_SAMPLE_RATE            A2DPHPSPEAKER_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define A2DPHPSPEAKER_LOOPBACK_MAX_SAMPLE_RATE            A2DPHPSPEAKER_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define A2DPHPSPEAKER_DOLBY_DIGITAL_MAX_CHANNELS          2       // Max Channels.
#define A2DPHPSPEAKER_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   16      // Min Bits Per Sample
#define A2DPHPSPEAKER_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define A2DPHPSPEAKER_DOLBY_DIGITAL_MIN_SAMPLE_RATE       44100   // Min Sample Rate
#define A2DPHPSPEAKER_DOLBY_DIGITAL_MAX_SAMPLE_RATE       44100   // Max Sample Rate

//
// Max # of pin instances.
//
#define A2DPHPSPEAKER_MAX_INPUT_SYSTEM_STREAMS             6
#define A2DPHPSPEAKER_MAX_INPUT_OFFLOAD_STREAMS            MAX_INPUT_OFFLOAD_STREAMS
#define A2DPHPSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS          MAX_OUTPUT_LOOPBACK_STREAMS

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE A2dpHpSpeakerAudioEngineSupportedDeviceFormats[] =
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE A2dpHpSpeakerHostPinSupportedDeviceFormats[] =
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE A2dpHpSpeakerOffloadPinSupportedDeviceFormats[] =
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

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT A2dpHpSpeakerHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MOVIE,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[0].DataFormat // 24KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION,
        &A2dpHpSpeakerHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    }
};

static
MODE_AND_DEFAULT_FORMAT A2dpHpSpeakerOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &A2dpHpSpeakerOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &A2dpHpSpeakerOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES A2dpHpSpeakerPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        A2dpHpSpeakerHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(A2dpHpSpeakerHostPinSupportedDeviceFormats),
        A2dpHpSpeakerHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(A2dpHpSpeakerHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        A2dpHpSpeakerOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(A2dpHpSpeakerOffloadPinSupportedDeviceFormats),
        A2dpHpSpeakerOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(A2dpHpSpeakerOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        A2dpHpSpeakerHostPinSupportedDeviceFormats,   // Must support all the formats supported by host pin
        SIZEOF_ARRAY(A2dpHpSpeakerHostPinSupportedDeviceFormats),
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
        A2dpHpSpeakerAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(A2dpHpSpeakerAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO A2dpHpSpeakerPinDataRangesStream[] =
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
        A2DPHPSPEAKER_HOST_MAX_CHANNELS,           
        A2DPHPSPEAKER_HOST_MIN_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_HOST_MAX_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_HOST_MIN_SAMPLE_RATE,            
        A2DPHPSPEAKER_HOST_MAX_SAMPLE_RATE             
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
        A2DPHPSPEAKER_OFFLOAD_MAX_CHANNELS,           
        A2DPHPSPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_OFFLOAD_MIN_SAMPLE_RATE,
        A2DPHPSPEAKER_OFFLOAD_MAX_SAMPLE_RATE
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
        A2DPHPSPEAKER_LOOPBACK_MAX_CHANNELS,           
        A2DPHPSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        A2DPHPSPEAKER_LOOPBACK_MIN_SAMPLE_RATE,
        A2DPHPSPEAKER_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE A2dpHpSpeakerPinDataRangePointersStream[] =
{
    PKSDATARANGE(&A2dpHpSpeakerPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE A2dpHpSpeakerPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&A2dpHpSpeakerPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE A2dpHpSpeakerPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&A2dpHpSpeakerPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE A2dpHpSpeakerPinDataRangesBridge[] =
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
PKSDATARANGE A2dpHpSpeakerPinDataRangePointersBridge[] =
{
    &A2dpHpSpeakerPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesA2dpHpSpeakerOffloadPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationA2dpHpSpeakerOffloadPin, PropertiesA2dpHpSpeakerOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR A2dpHpSpeakerWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_SYSTEM
    {
        A2DPHPSPEAKER_MAX_INPUT_SYSTEM_STREAMS,
        A2DPHPSPEAKER_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(A2dpHpSpeakerPinDataRangePointersStream),
            A2dpHpSpeakerPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        A2DPHPSPEAKER_MAX_INPUT_OFFLOAD_STREAMS,
        A2DPHPSPEAKER_MAX_INPUT_OFFLOAD_STREAMS, 
        0,
        &AutomationA2dpHpSpeakerOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(A2dpHpSpeakerPinDataRangePointersOffloadStream),
            A2dpHpSpeakerPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_LOOPBACK
    {
        A2DPHPSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS,
        A2DPHPSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(A2dpHpSpeakerPinDataRangePointersLoopbackStream),
            A2dpHpSpeakerPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(A2dpHpSpeakerPinDataRangePointersBridge),
            A2dpHpSpeakerPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR A2dpHpSpeakerWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR A2dpHpSpeakerWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesA2dpHpSpeakerWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpWaveFilter
    },
    {
        &KSPROPSETID_AudioEffectsDiscovery,
        KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpWaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationA2dpHpSpeakerWaveFilter, PropertiesA2dpHpSpeakerWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR A2dpHpSpeakerWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationA2dpHpSpeakerWaveFilter,             // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(A2dpHpSpeakerWaveMiniportPins),    // PinCount
    A2dpHpSpeakerWaveMiniportPins,                  // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(A2dpHpSpeakerWaveMiniportNodes),   // NodeCount
    A2dpHpSpeakerWaveMiniportNodes,                 // Nodes
    SIZEOF_ARRAY(A2dpHpSpeakerWaveMiniportConnections),// ConnectionCount
    A2dpHpSpeakerWaveMiniportConnections,           // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_A2DPHPSPEAKERWAVTABLE_H_

