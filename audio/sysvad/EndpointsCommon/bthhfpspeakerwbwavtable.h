/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpspeakerwbwavtable.h

Abstract:

    Declaration of wave miniport tables for the Bluetooth Hands-Free Profile speaker (external),
    for Bluetooth connections that support Wideband Speech.

--*/

#ifndef _SYSVAD_BTHHFPSPEAKERWBWAVTABLE_H_
#define _SYSVAD_BTHHFPSPEAKERWBWAVTABLE_H_

//
// Function prototypes.
//
NTSTATUS PropertyHandler_BthHfpWaveFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);



#define BTHHFPSPEAKERWB_DEVICE_MAX_CHANNELS                 1       // Max Channels.

#define BTHHFPSPEAKERWB_HOST_MAX_CHANNELS                   1       // Max Channels.
#define BTHHFPSPEAKERWB_HOST_MIN_BITS_PER_SAMPLE            8       // Min Bits Per Sample
#define BTHHFPSPEAKERWB_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define BTHHFPSPEAKERWB_HOST_MIN_SAMPLE_RATE                8000    // Min Sample Rate
#define BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE                16000   // Max Sample Rate

#define BTHHFPSPEAKERWB_OFFLOAD_MAX_CHANNELS                1       // Max Channels.
#define BTHHFPSPEAKERWB_OFFLOAD_MIN_BITS_PER_SAMPLE         8       // Min Bits Per Sample
#define BTHHFPSPEAKERWB_OFFLOAD_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define BTHHFPSPEAKERWB_OFFLOAD_MIN_SAMPLE_RATE             8000    // Min Sample Rate
#define BTHHFPSPEAKERWB_OFFLOAD_MAX_SAMPLE_RATE             16000   // Max Sample Rate

#define BTHHFPSPEAKERWB_LOOPBACK_MAX_CHANNELS               BTHHFPSPEAKERWB_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define BTHHFPSPEAKERWB_LOOPBACK_MIN_BITS_PER_SAMPLE        BTHHFPSPEAKERWB_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define BTHHFPSPEAKERWB_LOOPBACK_MAX_BITS_PER_SAMPLE        BTHHFPSPEAKERWB_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define BTHHFPSPEAKERWB_LOOPBACK_MIN_SAMPLE_RATE            BTHHFPSPEAKERWB_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define BTHHFPSPEAKERWB_LOOPBACK_MAX_SAMPLE_RATE            BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define BTHHFPSPEAKERWB_DOLBY_DIGITAL_MAX_CHANNELS          1       // Max Channels.
#define BTHHFPSPEAKERWB_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   8       // Min Bits Per Sample
#define BTHHFPSPEAKERWB_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define BTHHFPSPEAKERWB_DOLBY_DIGITAL_MIN_SAMPLE_RATE       8000    // Min Sample Rate
#define BTHHFPSPEAKERWB_DOLBY_DIGITAL_MAX_SAMPLE_RATE       16000   // Max Sample Rate


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerWBAudioEngineSupportedDeviceFormats[] =
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
                1,
                8000,
                8000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                8000,
                16000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                16000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                32000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerWBHostPinSupportedDeviceFormats[] =
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
                1,
                8000,
                8000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                8000,
                16000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                16000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                32000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerWBOffloadPinSupportedDeviceFormats[] =
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
                1,
                8000,
                8000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                8000,
                16000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                16000,
                1,
                8,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            8,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                16000,
                32000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    }
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT BthHfpSpeakerWBHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpSpeakerWBHostPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpSpeakerWBHostPinSupportedDeviceFormats)-1].DataFormat   
    }
};

static
MODE_AND_DEFAULT_FORMAT BthHfpSpeakerWBOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpSpeakerWBHostPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpSpeakerWBHostPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES BthHfpSpeakerWBPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        BthHfpSpeakerWBHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerWBHostPinSupportedDeviceFormats),
        BthHfpSpeakerWBHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(BthHfpSpeakerWBHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        BthHfpSpeakerWBOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerWBOffloadPinSupportedDeviceFormats),
        BthHfpSpeakerWBOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(BthHfpSpeakerWBOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        BthHfpSpeakerWBHostPinSupportedDeviceFormats,   // Must support all the formats supported by host pin
        SIZEOF_ARRAY(BthHfpSpeakerWBHostPinSupportedDeviceFormats),
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
        BthHfpSpeakerWBAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerWBAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO BthHfpSpeakerWBPinDataRangesStream[] =
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
        BTHHFPSPEAKERWB_HOST_MAX_CHANNELS,           
        BTHHFPSPEAKERWB_HOST_MIN_BITS_PER_SAMPLE,    
        BTHHFPSPEAKERWB_HOST_MAX_BITS_PER_SAMPLE,    
        BTHHFPSPEAKERWB_HOST_MIN_SAMPLE_RATE,            
        BTHHFPSPEAKERWB_HOST_MIN_SAMPLE_RATE   
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
        BTHHFPSPEAKERWB_HOST_MAX_CHANNELS,
        BTHHFPSPEAKERWB_HOST_MIN_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_HOST_MAX_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE,
        BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE
    },
    { // 2
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        BTHHFPSPEAKERWB_OFFLOAD_MAX_CHANNELS,
        BTHHFPSPEAKERWB_OFFLOAD_MIN_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_OFFLOAD_MAX_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_OFFLOAD_MIN_SAMPLE_RATE,
        BTHHFPSPEAKERWB_OFFLOAD_MIN_SAMPLE_RATE
    },
    { // 3
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        BTHHFPSPEAKERWB_OFFLOAD_MAX_CHANNELS,
        BTHHFPSPEAKERWB_OFFLOAD_MIN_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_OFFLOAD_MAX_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_OFFLOAD_MAX_SAMPLE_RATE,
        BTHHFPSPEAKERWB_OFFLOAD_MAX_SAMPLE_RATE
    },
    { // 4
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        BTHHFPSPEAKERWB_LOOPBACK_MAX_CHANNELS,
        BTHHFPSPEAKERWB_LOOPBACK_MIN_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_LOOPBACK_MAX_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_LOOPBACK_MIN_SAMPLE_RATE,
        BTHHFPSPEAKERWB_LOOPBACK_MIN_SAMPLE_RATE
    },
    { // 5
        {
            sizeof(KSDATARANGE_AUDIO),
                0,
                0,
                0,
                STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
                STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
                STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        BTHHFPSPEAKERWB_LOOPBACK_MAX_CHANNELS,
        BTHHFPSPEAKERWB_LOOPBACK_MIN_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_LOOPBACK_MAX_BITS_PER_SAMPLE,
        BTHHFPSPEAKERWB_LOOPBACK_MAX_SAMPLE_RATE,
        BTHHFPSPEAKERWB_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE BthHfpSpeakerWBPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE BthHfpSpeakerWBPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[2]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[3]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE BthHfpSpeakerWBPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[4]),
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[5])
};

//=============================================================================
static
KSDATARANGE BthHfpSpeakerWBPinDataRangesBridge[] =
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
PKSDATARANGE BthHfpSpeakerWBPinDataRangePointersBridge[] =
{
    &BthHfpSpeakerWBPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesBthHfpSpeakerWBOffloadPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpSpeakerWBOffloadPin, PropertiesBthHfpSpeakerWBOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR BthHfpSpeakerWBWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_SYSTEM
    {
        MAX_INPUT_SYSTEM_STREAMS,
        MAX_INPUT_SYSTEM_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(BthHfpSpeakerWBPinDataRangePointersStream),
            BthHfpSpeakerWBPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        MAX_INPUT_OFFLOAD_STREAMS,
        MAX_INPUT_OFFLOAD_STREAMS,
        0,
        &AutomationBthHfpSpeakerWBOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(BthHfpSpeakerWBPinDataRangePointersOffloadStream),
            BthHfpSpeakerWBPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_LOOPBACK
    {
        MAX_OUTPUT_LOOPBACK_STREAMS,
        MAX_OUTPUT_LOOPBACK_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(BthHfpSpeakerWBPinDataRangePointersLoopbackStream),
            BthHfpSpeakerWBPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(BthHfpSpeakerWBPinDataRangePointersBridge),
            BthHfpSpeakerWBPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR BthHfpSpeakerWBWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR BthHfpSpeakerWBWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesBthHfpSpeakerWBWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpWaveFilter
    },
    {
        &KSPROPSETID_AudioEffectsDiscovery,
        KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpWaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpSpeakerWBWaveFilter, PropertiesBthHfpSpeakerWBWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR BthHfpSpeakerWBWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationBthHfpSpeakerWBWaveFilter,             // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(BthHfpSpeakerWBWaveMiniportPins),    // PinCount
    BthHfpSpeakerWBWaveMiniportPins,                  // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(BthHfpSpeakerWBWaveMiniportNodes),   // NodeCount
    BthHfpSpeakerWBWaveMiniportNodes,                 // Nodes
    SIZEOF_ARRAY(BthHfpSpeakerWBWaveMiniportConnections),// ConnectionCount
    BthHfpSpeakerWBWaveMiniportConnections,           // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_BTHHFPSPEAKERWBWAVTABLE_H_

