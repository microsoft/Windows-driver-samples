/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerwavtable.h

Abstract:

    Declaration of wave miniport tables for the render endpoints.

--*/

#ifndef _SYSVAD_SPEAKERWAVTABLE_H_
#define _SYSVAD_SPEAKERWAVTABLE_H_

#include "SysVadShared.h"
#include "AudioModule0.h"
#include "AudioModule1.h"

// To keep the code simple assume device supports only 48KHz, 16-bit, stereo (PCM and NON-PCM)

#define SPEAKER_DEVICE_MAX_CHANNELS                 2       // Max Channels.

#define SPEAKER_HOST_MAX_CHANNELS                   2       // Max Channels.
#define SPEAKER_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define SPEAKER_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define SPEAKER_HOST_MIN_SAMPLE_RATE                24000   // Min Sample Rate
#define SPEAKER_HOST_MAX_SAMPLE_RATE                96000   // Max Sample Rate

#define SPEAKER_OFFLOAD_MAX_CHANNELS                2       // Max Channels.
#define SPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE         16      // Min Bits Per Sample
#define SPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define SPEAKER_OFFLOAD_MIN_SAMPLE_RATE             44100   // Min Sample Rate
#define SPEAKER_OFFLOAD_MAX_SAMPLE_RATE             48000   // Max Sample Rate

#define SPEAKER_LOOPBACK_MAX_CHANNELS               SPEAKER_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define SPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE        SPEAKER_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define SPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE        SPEAKER_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define SPEAKER_LOOPBACK_MIN_SAMPLE_RATE            SPEAKER_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define SPEAKER_LOOPBACK_MAX_SAMPLE_RATE            SPEAKER_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define SPEAKER_DOLBY_DIGITAL_MAX_CHANNELS          2       // Max Channels.
#define SPEAKER_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   16      // Min Bits Per Sample
#define SPEAKER_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define SPEAKER_DOLBY_DIGITAL_MIN_SAMPLE_RATE       44100   // Min Sample Rate
#define SPEAKER_DOLBY_DIGITAL_MAX_SAMPLE_RATE       44100   // Max Sample Rate

//
// Max # of pin instances.
//
#define SPEAKER_MAX_INPUT_SYSTEM_STREAMS            6
#define SPEAKER_MAX_INPUT_OFFLOAD_STREAMS           MAX_INPUT_OFFLOAD_STREAMS
#define SPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS         MAX_OUTPUT_LOOPBACK_STREAMS

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerAudioEngineSupportedDeviceFormats[] =
{
    { // 0 : First entry in this table is the default format for the audio engine
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHostPinSupportedDeviceFormats[] =
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
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerOffloadPinSupportedDeviceFormats[] =
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
MODE_AND_DEFAULT_FORMAT SpeakerHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &SpeakerHostPinSupportedDeviceFormats[3].DataFormat  // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerHostPinSupportedDeviceFormats[3].DataFormat  // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerHostPinSupportedDeviceFormats[3].DataFormat  // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MOVIE,
        &SpeakerHostPinSupportedDeviceFormats[3].DataFormat  // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &SpeakerHostPinSupportedDeviceFormats[0].DataFormat  // 24KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION,
        &SpeakerHostPinSupportedDeviceFormats[3].DataFormat  // 48KHz
    }
};

static
MODE_AND_DEFAULT_FORMAT SpeakerOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES SpeakerPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        SpeakerHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHostPinSupportedDeviceFormats),
        SpeakerHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        SpeakerOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerOffloadPinSupportedDeviceFormats),
        SpeakerOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        SpeakerHostPinSupportedDeviceFormats,  // Must support all the formats supported by host pin
        SIZEOF_ARRAY(SpeakerHostPinSupportedDeviceFormats),
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
        SpeakerAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO SpeakerPinDataRangesStream[] =
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
        SPEAKER_HOST_MAX_CHANNELS,           
        SPEAKER_HOST_MIN_BITS_PER_SAMPLE,    
        SPEAKER_HOST_MAX_BITS_PER_SAMPLE,    
        SPEAKER_HOST_MIN_SAMPLE_RATE,            
        SPEAKER_HOST_MAX_SAMPLE_RATE             
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
        SPEAKER_OFFLOAD_MAX_CHANNELS,           
        SPEAKER_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        SPEAKER_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        SPEAKER_OFFLOAD_MIN_SAMPLE_RATE,
        SPEAKER_OFFLOAD_MAX_SAMPLE_RATE
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
        SPEAKER_LOOPBACK_MAX_CHANNELS,           
        SPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        SPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        SPEAKER_LOOPBACK_MIN_SAMPLE_RATE,
        SPEAKER_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE SpeakerPinDataRangePointersStream[] =
{
    PKSDATARANGE(&SpeakerPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
};

static
PKSDATARANGE SpeakerPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&SpeakerPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList),

};

static
PKSDATARANGE SpeakerPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&SpeakerPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE SpeakerPinDataRangesBridge[] =
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
PKSDATARANGE SpeakerPinDataRangePointersBridge[] =
{
    &SpeakerPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpeakerHostPin[] =
{
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_DESCRIPTORS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_COMMAND,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHostPin, PropertiesSpeakerHostPin);

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpeakerOffloadPin[] =
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
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_DESCRIPTORS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_COMMAND,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_GenericPin
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerOffloadPin, PropertiesSpeakerOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR SpeakerWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_SYSTEM
    {
        SPEAKER_MAX_INPUT_SYSTEM_STREAMS,
        SPEAKER_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        &AutomationSpeakerHostPin,        // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerPinDataRangePointersStream),
            SpeakerPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        SPEAKER_MAX_INPUT_OFFLOAD_STREAMS,
        SPEAKER_MAX_INPUT_OFFLOAD_STREAMS, 
        0,
        &AutomationSpeakerOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerPinDataRangePointersOffloadStream),
            SpeakerPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_LOOPBACK
    {
        SPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS,
        SPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerPinDataRangePointersLoopbackStream),
            SpeakerPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(SpeakerPinDataRangePointersBridge),
            SpeakerPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR SpeakerWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR SpeakerWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_SysVAD,
        KSPROPERTY_SYSVAD_DEFAULTSTREAMEFFECTS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_DESCRIPTORS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_COMMAND,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_AudioModule,
        KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
};

//
// Endpoint audio module list.
//
static
AUDIOMODULE_DESCRIPTOR SpeakerModulesWaveFilter[] = 
{
    { // 0
        &AudioModule0Id,    // class id. 
        &NULL_GUID,         // generic, mode independent.
        L"Generic system module",
        AUDIOMODULE_INSTANCE_ID(0,0),
        AUDIOMODULE0_MAJOR,
        AUDIOMODULE0_MINOR,
        AUDIOMODULE_DESCRIPTOR_FLAG_NONE,
        sizeof(AUDIOMODULE0_CONTEXT),
        AudioModule0_InitClass,
        AudioModule0_InitInstance,
        AudioModule0_Cleanup,
        AudioModule0_Handler
    },    
    { // 1
        &AudioModule1Id,    // class id. 
        &AUDIO_SIGNALPROCESSINGMODE_MOVIE, 
        L"Speaker endpoint, EQ effect module, Movie mode", 
        AUDIOMODULE_INSTANCE_ID(0,0),
        AUDIOMODULE1_MAJOR,
        AUDIOMODULE1_MINOR,
        AUDIOMODULE_DESCRIPTOR_FLAG_NONE,
        sizeof(AUDIOMODULE1_CONTEXT),
        AudioModule1_InitClass,
        AudioModule1_InitInstance,
        AudioModule1_Cleanup,
        AudioModule1_Handler
    },
    { // 2
        &AudioModule1Id,    // class id. 
        &AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        L"Speaker endpoint, EQ effect module, Default mode", 
        AUDIOMODULE_INSTANCE_ID(1,0),
        AUDIOMODULE1_MAJOR,
        AUDIOMODULE1_MINOR,
        AUDIOMODULE_DESCRIPTOR_FLAG_NONE,
        sizeof(AUDIOMODULE1_CONTEXT),
        AudioModule1_InitClass,
        AudioModule1_InitInstance,
        AudioModule1_Cleanup,
        AudioModule1_Handler
    }
};

//
// Audio module notification device id.
// NOTE: do not use this guid for real driver, generate a new one.
//
const GUID SpeakerModuleNotificationDeviceId = 
{0xC01C987A,0x30A9,0x4DB7,0x8D,0x69,0x69,0x6C,0xFF,0x41,0xE4,0xB9};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerWaveFilter, PropertiesSpeakerWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpeakerWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationSpeakerWaveFilter,                   // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(SpeakerWaveMiniportPins),          // PinCount
    SpeakerWaveMiniportPins,                        // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(SpeakerWaveMiniportNodes),         // NodeCount
    SpeakerWaveMiniportNodes,                       // Nodes
    SIZEOF_ARRAY(SpeakerWaveMiniportConnections),   // ConnectionCount
    SpeakerWaveMiniportConnections,                 // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_SPEAKERWAVTABLE_H_

