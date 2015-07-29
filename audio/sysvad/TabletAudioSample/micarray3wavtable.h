/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarray3wavtable.h

Abstract:

    Declaration of wave miniport tables for the combined mic array (front/back).

--*/

#ifndef _SYSVAD_MICARRAY3WAVTABLE_H_
#define _SYSVAD_MICARRAY3WAVTABLE_H_

//
// Mic array range.
//
#define MICARRAY3_RAW_CHANNELS                   4       // Channels for raw mode
#define MICARRAY3_PROCESSED_CHANNELS             1       // Channels for default mode
#define MICARRAY3_DEVICE_MAX_CHANNELS            4       // Max channels overall
#define MICARRAY3_MIN_BITS_PER_SAMPLE_PCM        16      // Min Bits Per Sample
#define MICARRAY3_MAX_BITS_PER_SAMPLE_PCM        16      // Max Bits Per Sample
#define MICARRAY3_RAW_SAMPLE_RATE                48000   // Raw sample rate

//
// Max # of pin instances.
//
#define MICARRAY3_MAX_INPUT_STREAMS              2       // Raw + Default streams

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE MicArray3PinSupportedDeviceFormats[] =
{
    // 0 - Note the ENDPOINT_MINIPAIR structures for the mic arrays use this first element as the proposed DEFAULT format
    // 48 KHz 16-bit mono
    {
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
    },
    // 1
    // 16 KHz 16-bit mono
    {
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
    // 2
    // 11.025 KHz 16-bit mono
    {
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
    // 3
    // 8 KHz 16-bit mono
    {
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
    // 4 - Note the ENDPOINT_MINIPAIR structures for the mic arrays use this last element as the proposed RAW format
    // 48 KHz 16-bit 4 channels
    {
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
                4,
                48000,
                384000,
                8,
                16,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            16,
            0,                                      // No channel configuration for unprocessed mic array
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT MicArray3PinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &MicArray3PinSupportedDeviceFormats[SIZEOF_ARRAY(MicArray3PinSupportedDeviceFormats)-1].DataFormat   
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &MicArray3PinSupportedDeviceFormats[0].DataFormat   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES MicArray3PinDeviceFormatsAndModes[] = 
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
        MicArray3PinSupportedDeviceFormats,
        SIZEOF_ARRAY(MicArray3PinSupportedDeviceFormats),
        MicArray3PinSupportedDeviceModes,
        SIZEOF_ARRAY(MicArray3PinSupportedDeviceModes)
    }
};

//=============================================================================
// Data ranges
//
// See CMiniportWaveRT::DataRangeIntersection.
//
// Both mono and two-channel formats are supported for the mic arrays. The
// design of this sample driver's data range intersection handler requires a
// separate data for each supported channel count.
//
static
KSDATARANGE_AUDIO MicArray3PinDataRangesRawStream[] =
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
        MICARRAY3_RAW_CHANNELS,           
        MICARRAY3_MIN_BITS_PER_SAMPLE_PCM,    
        MICARRAY3_MAX_BITS_PER_SAMPLE_PCM,    
        MICARRAY3_RAW_SAMPLE_RATE,            
        MICARRAY3_RAW_SAMPLE_RATE             
    },
};

static
KSDATARANGE_AUDIO MicArray3PinDataRangesProcessedStream[] =
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
        MICARRAY3_PROCESSED_CHANNELS,
        MICARRAY3_MIN_BITS_PER_SAMPLE_PCM,
        MICARRAY3_MAX_BITS_PER_SAMPLE_PCM,
        48000,
        48000
    },
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
        MICARRAY3_PROCESSED_CHANNELS,
        MICARRAY3_MIN_BITS_PER_SAMPLE_PCM,
        MICARRAY3_MAX_BITS_PER_SAMPLE_PCM,
        16000,
        16000
    },
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
        MICARRAY3_PROCESSED_CHANNELS,
        MICARRAY3_MIN_BITS_PER_SAMPLE_PCM,
        MICARRAY3_MAX_BITS_PER_SAMPLE_PCM,
        11025,
        11025
    },
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
        MICARRAY3_PROCESSED_CHANNELS,
        MICARRAY3_MIN_BITS_PER_SAMPLE_PCM,
        MICARRAY3_MAX_BITS_PER_SAMPLE_PCM,
        8000,
        8000
    },
};

static
PKSDATARANGE MicArray3PinDataRangePointersStream[] =
{
    PKSDATARANGE(&MicArray3PinDataRangesProcessedStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray3PinDataRangesRawStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
};

//=============================================================================
static
KSDATARANGE MicArray3PinDataRangesBridge[] =
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
PKSDATARANGE MicArray3PinDataRangePointersBridge[] =
{
    &MicArray3PinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicArray3WaveMiniportPins[] =
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
            SIZEOF_ARRAY(MicArray3PinDataRangePointersBridge),
            MicArray3PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
  
    // Wave In Streaming Pin (Capture) KSPIN_WAVE_HOST
    {
        MICARRAY3_MAX_INPUT_STREAMS,
        MICARRAY3_MAX_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(MicArray3PinDataRangePointersStream),
            MicArray3PinDataRangePointersStream,
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
PCNODE_DESCRIPTOR MicArray3WaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR MicArray3WaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST }
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesMicArray3WaveFilter[] =
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
    },
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray3WaveFilter, PropertiesMicArray3WaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicArray3WaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationMicArray3WaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(MicArray3WaveMiniportPins),        // PinCount
    MicArray3WaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(MicArray3WaveMiniportNodes),       // NodeCount
    MicArray3WaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(MicArray3WaveMiniportConnections), // ConnectionCount
    MicArray3WaveMiniportConnections,               // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_MICARRAY3WAVTABLE_H_

