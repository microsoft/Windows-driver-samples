/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    spdifwavtable.h

Abstract:

    Declaration of wave miniport tables for the SPDIF endpoint.

--*/

#ifndef _SYSVAD_SPDIFWAVTABLE_H_
#define _SYSVAD_SPDIFWAVTABLE_H_


//=============================================================================
// Defines
//=============================================================================


#define SPDIF_DEVICE_MAX_CHANNELS                 2       // Max Channels.

#define SPDIF_HOST_MAX_CHANNELS                   2       // Max Channels.
#define SPDIF_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define SPDIF_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define SPDIF_HOST_MIN_SAMPLE_RATE                44100   // Min Sample Rate
#define SPDIF_HOST_MAX_SAMPLE_RATE                96000   // Max Sample Rate

#define SPDIF_OFFLOAD_MAX_CHANNELS                2       // Max Channels.
#define SPDIF_OFFLOAD_MIN_BITS_PER_SAMPLE         16      // Min Bits Per Sample
#define SPDIF_OFFLOAD_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define SPDIF_OFFLOAD_MIN_SAMPLE_RATE             44100   // Min Sample Rate
#define SPDIF_OFFLOAD_MAX_SAMPLE_RATE             96000   // Max Sample Rate

#define SPDIF_LOOPBACK_MAX_CHANNELS               2       // Max Channels.
#define SPDIF_LOOPBACK_MIN_BITS_PER_SAMPLE        16      // Min Bits Per Sample
#define SPDIF_LOOPBACK_MAX_BITS_PER_SAMPLE        16      // Max Bits Per Sample
#define SPDIF_LOOPBACK_MIN_SAMPLE_RATE            44100   // Min Sample Rate
#define SPDIF_LOOPBACK_MAX_SAMPLE_RATE            48000   // Max Sample Rate

#define SPDIF_DOLBY_DIGITAL_MAX_CHANNELS          2       // Max Channels.
#define SPDIF_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   16      // Min Bits Per Sample
#define SPDIF_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define SPDIF_DOLBY_DIGITAL_MIN_SAMPLE_RATE       44100   // Min Sample Rate
#define SPDIF_DOLBY_DIGITAL_MAX_SAMPLE_RATE       48000   // Max Sample Rate

//
// Max # of pin instances.
//
#define SPDIF_MAX_INPUT_SYSTEM_STREAMS            2       // Raw + Default streams
#define SPDIF_MAX_INPUT_OFFLOAD_STREAMS           MAX_INPUT_OFFLOAD_STREAMS
#define SPDIF_MAX_OUTPUT_LOOPBACK_STREAMS         MAX_OUTPUT_LOOPBACK_STREAMS


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpdifAudioEngineSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpdifHostPinSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpdifOffloadPinSupportedDeviceFormats[] =
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
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE SpdifLoopbackPinSupportedDeviceFormats[] =
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
MODE_AND_DEFAULT_FORMAT SpdifHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        NULL, // just an example of no default format for this endpoint/mode   
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        NULL, // just an example of no default format for this endpoint/mode   
    }
};

static
MODE_AND_DEFAULT_FORMAT SpdifOffloadPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        NULL, // just an example of no default format for this endpoint/mode   
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        NULL, // just an example of no default format for this endpoint/mode   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES SpdifPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        SpdifHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpdifHostPinSupportedDeviceFormats),
        SpdifHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpdifHostPinSupportedDeviceModes)
    },
    {
        OffloadRenderPin,
        SpdifOffloadPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpdifOffloadPinSupportedDeviceFormats),
        SpdifOffloadPinSupportedDeviceModes,
        SIZEOF_ARRAY(SpdifOffloadPinSupportedDeviceModes),
    },
    {
        RenderLoopbackPin,
        SpdifLoopbackPinSupportedDeviceFormats,
        SIZEOF_ARRAY(SpdifLoopbackPinSupportedDeviceFormats),
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
        SpdifAudioEngineSupportedDeviceFormats,
        SIZEOF_ARRAY(SpdifAudioEngineSupportedDeviceFormats),
        NULL,       // no modes for this entry.
        0
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO SpdifPinDataRangesStream[] =
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
        SPDIF_HOST_MAX_CHANNELS,           
        SPDIF_HOST_MIN_BITS_PER_SAMPLE,    
        SPDIF_HOST_MAX_BITS_PER_SAMPLE,    
        SPDIF_HOST_MIN_SAMPLE_RATE,            
        SPDIF_HOST_MAX_SAMPLE_RATE             
    },
    { // 1 - PCM offload
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPDIF_OFFLOAD_MAX_CHANNELS,           
        SPDIF_OFFLOAD_MIN_BITS_PER_SAMPLE,    
        SPDIF_OFFLOAD_MAX_BITS_PER_SAMPLE,    
        SPDIF_OFFLOAD_MIN_SAMPLE_RATE,
        SPDIF_OFFLOAD_MAX_SAMPLE_RATE
    },
    { // 2 - PCM loopback
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPDIF_LOOPBACK_MAX_CHANNELS,           
        SPDIF_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        SPDIF_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        SPDIF_LOOPBACK_MIN_SAMPLE_RATE,
        SPDIF_LOOPBACK_MAX_SAMPLE_RATE
    },
    { // 3 - DOLBY-DIGITAL host
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        SPDIF_DOLBY_DIGITAL_MAX_CHANNELS,           
        SPDIF_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE,    
        SPDIF_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE,    
        SPDIF_DOLBY_DIGITAL_MIN_SAMPLE_RATE,
        SPDIF_DOLBY_DIGITAL_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE SpdifPinDataRangePointersStream[] =
{
    PKSDATARANGE(&SpdifPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&SpdifPinDataRangesStream[3]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE SpdifPinDataRangePointersOffloadStream[] =
{
    PKSDATARANGE(&SpdifPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList),

};

static
PKSDATARANGE SpdifPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&SpdifPinDataRangesStream[2])
};

//=============================================================================
static
KSDATARANGE SpdifPinDataRangesBridge[] =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE SpdifPinDataRangePointersBridge[] =
{
    &SpdifPinDataRangesBridge[0],
    &SpdifPinDataRangesBridge[1]    
};

//=============================================================================

static
PCPROPERTY_ITEM PropertiesSpdifOffloadPin[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpdifOffloadPin, PropertiesSpdifOffloadPin);


//=============================================================================
static
PCPIN_DESCRIPTOR SpdifWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_SYSTEM
    {
        SPDIF_MAX_INPUT_SYSTEM_STREAMS,
        SPDIF_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpdifPinDataRangePointersStream),
            SpdifPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER_SINK_OFFLOAD
    {
        SPDIF_MAX_INPUT_OFFLOAD_STREAMS,
        SPDIF_MAX_INPUT_OFFLOAD_STREAMS, 
        0,
        &AutomationSpdifOffloadPin,     // AutomationTable
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpdifPinDataRangePointersOffloadStream),
            SpdifPinDataRangePointersOffloadStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_LOOPBACK
    {
        SPDIF_MAX_OUTPUT_LOOPBACK_STREAMS,
        SPDIF_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(SpdifPinDataRangePointersLoopbackStream),
            SpdifPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(SpdifPinDataRangePointersBridge),
            SpdifPinDataRangePointersBridge,
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
PCNODE_DESCRIPTOR SpdifWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR SpdifWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_SYSTEM,     KSNODE_WAVE_AUDIO_ENGINE,   1 },
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER_SINK_OFFLOAD,    KSNODE_WAVE_AUDIO_ENGINE,   2 },
    { KSNODE_WAVE_AUDIO_ENGINE, 3,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SINK_LOOPBACK },
    { KSNODE_WAVE_AUDIO_ENGINE, 0,                                 PCFILTER_NODE,              KSPIN_WAVE_RENDER_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpdifWaveFilter[] =
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
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpdifWaveFilter, PropertiesSpdifWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpdifWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationSpdifWaveFilter,                     // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(SpdifWaveMiniportPins),            // PinCount
    SpdifWaveMiniportPins,                          // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(SpdifWaveMiniportNodes),           // NodeCount
    SpdifWaveMiniportNodes,                         // Nodes
    SIZEOF_ARRAY(SpdifWaveMiniportConnections),     // ConnectionCount
    SpdifWaveMiniportConnections,                   // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_SPDIFWAVTABLE_H_

