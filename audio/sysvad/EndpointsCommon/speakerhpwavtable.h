/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhpwavtable.h

Abstract:

    Declaration of wave miniport tables for the speaker (external: headphone).

--*/

#ifndef _SYSVAD_SPEAKERHPWAVTABLE_H_
#define _SYSVAD_SPEAKERHPWAVTABLE_H_

#include "AudioModule0.h"
#include "AudioModule1.h"
#include "AudioModule2.h"

// To keep the code simple assume device supports only 48KHz, 16-bit, stereo (PCM and NON-PCM)

#define SPEAKERHP_DEVICE_MAX_CHANNELS                   2       // Max Channels.

#define SPEAKERHP_HOST_MAX_CHANNELS                     2       // Max Channels.
#define SPEAKERHP_HOST_MIN_BITS_PER_SAMPLE              16      // Min Bits Per Sample
#define SPEAKERHP_HOST_MAX_BITS_PER_SAMPLE              16      // Max Bits Per Sample
#define SPEAKERHP_HOST_MIN_SAMPLE_RATE                  24000   // Min Sample Rate
#define SPEAKERHP_HOST_MAX_SAMPLE_RATE                  96000   // Max Sample Rate

#define SPEAKERHP_OFFLOAD_MAX_CHANNELS                  2       // Max Channels.
#define SPEAKERHP_OFFLOAD_MIN_BITS_PER_SAMPLE           16      // Min Bits Per Sample
#define SPEAKERHP_OFFLOAD_MAX_BITS_PER_SAMPLE           16      // Max Bits Per Sample
#define SPEAKERHP_OFFLOAD_MIN_SAMPLE_RATE               44100   // Min Sample Rate
#define SPEAKERHP_OFFLOAD_MAX_SAMPLE_RATE               48000   // Max Sample Rate

#define SPEAKERHP_LOOPBACK_MAX_CHANNELS                 SPEAKERHP_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define SPEAKERHP_LOOPBACK_MIN_BITS_PER_SAMPLE          SPEAKERHP_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define SPEAKERHP_LOOPBACK_MAX_BITS_PER_SAMPLE          SPEAKERHP_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define SPEAKERHP_LOOPBACK_MIN_SAMPLE_RATE              SPEAKERHP_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define SPEAKERHP_LOOPBACK_MAX_SAMPLE_RATE              SPEAKERHP_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define SPEAKERHP_DOLBY_DIGITAL_MAX_CHANNELS            2       // Max Channels.
#define SPEAKERHP_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE     16      // Min Bits Per Sample
#define SPEAKERHP_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE     16      // Max Bits Per Sample
#define SPEAKERHP_DOLBY_DIGITAL_MIN_SAMPLE_RATE         44100   // Min Sample Rate
#define SPEAKERHP_DOLBY_DIGITAL_MAX_SAMPLE_RATE         44100   // Max Sample Rate

//
// Max # of pin instances.
//
#define SPEAKERHP_MAX_INPUT_SYSTEM_STREAMS              6
#define SPEAKERHP_MAX_INPUT_OFFLOAD_STREAMS             MAX_INPUT_OFFLOAD_STREAMS
#define SPEAKERHP_MAX_OUTPUT_LOOPBACK_STREAMS           MAX_OUTPUT_LOOPBACK_STREAMS

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHpAudioEngineSupportedDeviceFormats[] =
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHpHostPinSupportedDeviceFormats[] =
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
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpeakerHpOffloadPinSupportedDeviceFormats[] =
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
MODE_AND_DEFAULT_FORMAT SpeakerHpHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &SpeakerHpHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerHpHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerHpHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MOVIE,
        &SpeakerHpHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &SpeakerHpHostPinSupportedDeviceFormats[0].DataFormat // 24KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION,
        &SpeakerHpHostPinSupportedDeviceFormats[3].DataFormat // 48KHz
    }
};

static
MODE_AND_DEFAULT_FORMAT SpeakerHpOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &SpeakerHpOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_MEDIA,
        &SpeakerHpOffloadPinSupportedDeviceFormats[1].DataFormat // 48KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES SpeakerHpPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        SpeakerHpHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHpHostPinSupportedDeviceFormats),
        SpeakerHpHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerHpHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        SpeakerHpOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHpOffloadPinSupportedDeviceFormats),
        SpeakerHpOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpeakerHpOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        SpeakerHpHostPinSupportedDeviceFormats,   // Must support all the formats supported by host pin
        SIZEOF_ARRAY(SpeakerHpHostPinSupportedDeviceFormats),
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
        SpeakerHpAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(SpeakerHpAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO SpeakerHpPinDataRangesStream[] =
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
        SPEAKERHP_HOST_MAX_CHANNELS,           
        SPEAKERHP_HOST_MIN_BITS_PER_SAMPLE,    
        SPEAKERHP_HOST_MAX_BITS_PER_SAMPLE,    
        SPEAKERHP_HOST_MIN_SAMPLE_RATE,            
        SPEAKERHP_HOST_MAX_SAMPLE_RATE             
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
        SPEAKERHP_OFFLOAD_MAX_CHANNELS,           
        SPEAKERHP_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        SPEAKERHP_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        SPEAKERHP_OFFLOAD_MIN_SAMPLE_RATE,
        SPEAKERHP_OFFLOAD_MAX_SAMPLE_RATE
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
        SPEAKERHP_LOOPBACK_MAX_CHANNELS,           
        SPEAKERHP_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        SPEAKERHP_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        SPEAKERHP_LOOPBACK_MIN_SAMPLE_RATE,
        SPEAKERHP_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE SpeakerHpPinDataRangePointersStream[] =
{
    PKSDATARANGE(&SpeakerHpPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE SpeakerHpPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&SpeakerHpPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE SpeakerHpPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&SpeakerHpPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE SpeakerHpPinDataRangesBridge[] =
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
PKSDATARANGE SpeakerHpPinDataRangePointersBridge[] =
{
    &SpeakerHpPinDataRangesBridge[0]
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpeakerHpHostPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHpHostPin, PropertiesSpeakerHpHostPin);

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpeakerHpOffloadPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHpOffloadPin, PropertiesSpeakerHpOffloadPin);

//=============================================================================
static
PCPIN_DESCRIPTOR SpeakerHpWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (renderer) KSPIN_WAVE_RENDER_SINK_SYSTEM
    {
        SPEAKERHP_MAX_INPUT_SYSTEM_STREAMS,
        SPEAKERHP_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        &AutomationSpeakerHpHostPin,        // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHpPinDataRangePointersStream),
            SpeakerHpPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        SPEAKERHP_MAX_INPUT_OFFLOAD_STREAMS,
        SPEAKERHP_MAX_INPUT_OFFLOAD_STREAMS, 
        0,
        &AutomationSpeakerHpOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHpPinDataRangePointersOffloadStream),
            SpeakerHpPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_LOOPBACK
    {
        SPEAKERHP_MAX_OUTPUT_LOOPBACK_STREAMS,
        SPEAKERHP_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpeakerHpPinDataRangePointersLoopbackStream),
            SpeakerHpPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(SpeakerHpPinDataRangePointersBridge),
            SpeakerHpPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR SpeakerHpWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR SpeakerHpWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerHpWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
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
AUDIOMODULE_DESCRIPTOR SpeakerHpModulesWaveFilter[] = 
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
        L"Speaker HP endpoint, EQ effect module, Movie mode", 
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
        &AudioModule2Id,    // class id.
        &AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        L"Speaker HP endpoint, Echo Cancellation effect module, Default mode", 
        AUDIOMODULE_INSTANCE_ID(0,0),
        AUDIOMODULE2_MAJOR,
        AUDIOMODULE2_MINOR,
        AUDIOMODULE_DESCRIPTOR_FLAG_NONE,
        sizeof(AUDIOMODULE2_CONTEXT),
        AudioModule2_InitClass,
        AudioModule2_InitInstance,
        AudioModule2_Cleanup,
        AudioModule2_Handler
    }
};

//
// Audio module notification device id.
// NOTE: do not use this guid for real driver, generate a new one.
//
const GUID SpeakerHpModuleNotificationDeviceId = 
{0x3A66BE17,0x5440,0x48E7,0x83,0x76,0xB7,0x5A,0x0B,0x1A,0x92,0x2D};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHpWaveFilter, PropertiesSpeakerHpWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpeakerHpWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationSpeakerHpWaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(SpeakerHpWaveMiniportPins),        // PinCount
    SpeakerHpWaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(SpeakerHpWaveMiniportNodes),       // NodeCount
    SpeakerHpWaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(SpeakerHpWaveMiniportConnections), // ConnectionCount
    SpeakerHpWaveMiniportConnections,               // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_SPEAKERHPWAVTABLE_H_

