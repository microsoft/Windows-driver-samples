/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micinwavtable.h

Abstract:

    Declaration of wave miniport tables for the mic (external: headphone).

--*/

#ifndef _SYSVAD_MICINWAVTABLE_H_
#define _SYSVAD_MICINWAVTABLE_H_

//
// Mic in (external: headphone) range.
//
#define MICIN_DEVICE_MAX_CHANNELS           1       // Max Channels.
#define MICIN_MIN_BITS_PER_SAMPLE_PCM       16      // Min Bits Per Sample
#define MICIN_MAX_BITS_PER_SAMPLE_PCM       16      // Max Bits Per Sample
#define MICIN_MIN_SAMPLE_RATE               8000    // Min Sample Rate
#define MICIN_MAX_SAMPLE_RATE               48000   // Max Sample Rate

//
// Max # of pin instances.
//
#define MICIN_MAX_INPUT_STREAMS             4

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE MicInPinSupportedDeviceFormats[] =
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
                11025,
                22050,
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
                32000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
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
                22050,
                44100,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                24000,
                48000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
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
                1,
                32000,
                64000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 6
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
                44100,
                88200,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    { // 7
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
                48000,
                96000,
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
MODE_AND_DEFAULT_FORMAT MicInPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &MicInPinSupportedDeviceFormats[SIZEOF_ARRAY(MicInPinSupportedDeviceFormats) - 1].DataFormat,
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &MicInPinSupportedDeviceFormats[SIZEOF_ARRAY(MicInPinSupportedDeviceFormats) - 1].DataFormat,
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_SPEECH,
        &MicInPinSupportedDeviceFormats[2].DataFormat, // 16KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &MicInPinSupportedDeviceFormats[4].DataFormat, // 24KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
static 
PIN_DEVICE_FORMATS_AND_MODES MicInPinDeviceFormatsAndModes[] = 
{
    { 
        BridgePin,
        NULL,
        0,
        NULL,
        0
    },
    {
        SystemCapturePin,
        MicInPinSupportedDeviceFormats,
        SIZEOF_ARRAY(MicInPinSupportedDeviceFormats),
        MicInPinSupportedDeviceModes,
        SIZEOF_ARRAY(MicInPinSupportedDeviceModes)
    }
};

//=============================================================================
static
KSDATARANGE MicInPinDataRangesBridge[] =
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
PKSDATARANGE MicInPinDataRangePointersBridge[] =
{
    &MicInPinDataRangesBridge[0]
};

//=============================================================================
static
KSDATARANGE_AUDIO MicInPinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        MICIN_DEVICE_MAX_CHANNELS,           
        MICIN_MIN_BITS_PER_SAMPLE_PCM,    
        MICIN_MAX_BITS_PER_SAMPLE_PCM,    
        MICIN_MIN_SAMPLE_RATE,            
        MICIN_MAX_SAMPLE_RATE             
    },
};

static
PKSDATARANGE MicInPinDataRangePointersStream[] =
{
    PKSDATARANGE(&MicInPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicInWaveMiniportPins[] =
{
    // Wave In Bridge Pin (Capture - From Topology) KSPIN_WAVE_BRIDGE
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
            SIZEOF_ARRAY(MicInPinDataRangePointersBridge),
            MicInPinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },

    // Wave In Streaming Pin (Capture) KSPIN_WAVEIN_HOST
    {
        MICIN_MAX_INPUT_STREAMS,
        MICIN_MAX_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(MicInPinDataRangePointersStream),
            MicInPinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            &KSAUDFNAME_RECORDING_CONTROL,  
            0
        }
    }
};

//=============================================================================
static
PCNODE_DESCRIPTOR MicInWaveMiniportNodes[] =
{
    // KSNODE_WAVE_ADC
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_ADC,        // Type
        NULL                    // Name
    }
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicInWaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesMicInWaveFilter[] =
{
    {
        &KSPROPSETID_General,
        KSPROPERTY_GENERAL_COMPONENTID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInWaveFilter, PropertiesMicInWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicInWaveMiniportFilterDescriptor =
{
    0,                                          // Version
    &AutomationMicInWaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                   // PinSize
    SIZEOF_ARRAY(MicInWaveMiniportPins),        // PinCount
    MicInWaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                  // NodeSize
    SIZEOF_ARRAY(MicInWaveMiniportNodes),       // NodeCount
    MicInWaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(MicInWaveMiniportConnections), // ConnectionCount
    MicInWaveMiniportConnections,               // Connections
    0,                                          // CategoryCount
    NULL                                        // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_MICINWAVTABLE_H_
