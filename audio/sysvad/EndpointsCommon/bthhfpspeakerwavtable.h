/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpspeakerwavtable.h

Abstract:

    Declaration of wave miniport tables for the Bluetooth Hands-Free Profile speaker (external).

--*/

#ifndef _SYSVAD_BTHHFPSPEAKERWAVTABLE_H_
#define _SYSVAD_BTHHFPSPEAKERWAVTABLE_H_

//
// Function prototypes.
//
NTSTATUS PropertyHandler_BthHfpWaveFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);



#define BTHHFPSPEAKER_DEVICE_MAX_CHANNELS                 1       // Max Channels.

#define BTHHFPSPEAKER_HOST_MAX_CHANNELS                   1       // Max Channels.
#define BTHHFPSPEAKER_HOST_MIN_BITS_PER_SAMPLE            8       // Min Bits Per Sample
#define BTHHFPSPEAKER_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define BTHHFPSPEAKER_HOST_MIN_SAMPLE_RATE                8000    // Min Sample Rate
#define BTHHFPSPEAKER_HOST_MAX_SAMPLE_RATE                8000    // Max Sample Rate

#define BTHHFPSPEAKER_OFFLOAD_MAX_CHANNELS                1       // Max Channels.
#define BTHHFPSPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE         8       // Min Bits Per Sample
#define BTHHFPSPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define BTHHFPSPEAKER_OFFLOAD_MIN_SAMPLE_RATE             8000    // Min Sample Rate
#define BTHHFPSPEAKER_OFFLOAD_MAX_SAMPLE_RATE             8000    // Max Sample Rate

#define BTHHFPSPEAKER_LOOPBACK_MAX_CHANNELS               1       // Max Channels.
#define BTHHFPSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE        8       // Min Bits Per Sample
#define BTHHFPSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE        16      // Max Bits Per Sample
#define BTHHFPSPEAKER_LOOPBACK_MIN_SAMPLE_RATE            8000    // Min Sample Rate
#define BTHHFPSPEAKER_LOOPBACK_MAX_SAMPLE_RATE            8000    // Max Sample Rate

#define BTHHFPSPEAKER_DOLBY_DIGITAL_MAX_CHANNELS          1       // Max Channels.
#define BTHHFPSPEAKER_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   8       // Min Bits Per Sample
#define BTHHFPSPEAKER_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define BTHHFPSPEAKER_DOLBY_DIGITAL_MIN_SAMPLE_RATE       8000    // Min Sample Rate
#define BTHHFPSPEAKER_DOLBY_DIGITAL_MAX_SAMPLE_RATE       8000    // Max Sample Rate


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerAudioEngineSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerHostPinSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerOffloadPinSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpSpeakerLoopbackPinSupportedDeviceFormats[] =
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
    }
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT BthHfpSpeakerHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpSpeakerHostPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpSpeakerHostPinSupportedDeviceFormats)-1].DataFormat   
    }
};

static
MODE_AND_DEFAULT_FORMAT BthHfpSpeakerOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpSpeakerHostPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpSpeakerHostPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES BthHfpSpeakerPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        BthHfpSpeakerHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerHostPinSupportedDeviceFormats),
        BthHfpSpeakerHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(BthHfpSpeakerHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        BthHfpSpeakerOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerOffloadPinSupportedDeviceFormats),
        BthHfpSpeakerOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(BthHfpSpeakerOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        BthHfpSpeakerLoopbackPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerLoopbackPinSupportedDeviceFormats),
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
        BthHfpSpeakerAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpSpeakerAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO BthHfpSpeakerPinDataRangesStream[] =
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
        BTHHFPSPEAKER_HOST_MAX_CHANNELS,           
        BTHHFPSPEAKER_HOST_MIN_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_HOST_MAX_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_HOST_MIN_SAMPLE_RATE,            
        BTHHFPSPEAKER_HOST_MAX_SAMPLE_RATE             
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
        BTHHFPSPEAKER_OFFLOAD_MAX_CHANNELS,           
        BTHHFPSPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_OFFLOAD_MIN_SAMPLE_RATE,
        BTHHFPSPEAKER_OFFLOAD_MAX_SAMPLE_RATE
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
        BTHHFPSPEAKER_LOOPBACK_MAX_CHANNELS,           
        BTHHFPSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        BTHHFPSPEAKER_LOOPBACK_MIN_SAMPLE_RATE,
        BTHHFPSPEAKER_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE BthHfpSpeakerPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE BthHfpSpeakerPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE BthHfpSpeakerPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE BthHfpSpeakerPinDataRangesBridge[] =
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
PKSDATARANGE BthHfpSpeakerPinDataRangePointersBridge[] =
{
    &BthHfpSpeakerPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesBthHfpSpeakerOffloadPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpSpeakerOffloadPin, PropertiesBthHfpSpeakerOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR BthHfpSpeakerWaveMiniportPins[] =
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
            SIZEOF_ARRAY(BthHfpSpeakerPinDataRangePointersStream),
            BthHfpSpeakerPinDataRangePointersStream,
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
        &AutomationBthHfpSpeakerOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(BthHfpSpeakerPinDataRangePointersOffloadStream),
            BthHfpSpeakerPinDataRangePointersOffloadStream,
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
            SIZEOF_ARRAY(BthHfpSpeakerPinDataRangePointersLoopbackStream),
            BthHfpSpeakerPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(BthHfpSpeakerPinDataRangePointersBridge),
            BthHfpSpeakerPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR BthHfpSpeakerWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR BthHfpSpeakerWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesBthHfpSpeakerWaveFilter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpSpeakerWaveFilter, PropertiesBthHfpSpeakerWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR BthHfpSpeakerWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationBthHfpSpeakerWaveFilter,             // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(BthHfpSpeakerWaveMiniportPins),    // PinCount
    BthHfpSpeakerWaveMiniportPins,                  // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(BthHfpSpeakerWaveMiniportNodes),   // NodeCount
    BthHfpSpeakerWaveMiniportNodes,                 // Nodes
    SIZEOF_ARRAY(BthHfpSpeakerWaveMiniportConnections),// ConnectionCount
    BthHfpSpeakerWaveMiniportConnections,           // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_BTHHFPSPEAKERWAVTABLE_H_

