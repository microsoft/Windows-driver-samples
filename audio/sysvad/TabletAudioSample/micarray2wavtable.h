/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:
    micarray2wavtable.h

Abstract:

    Declaration of wave miniport tables for the rear mic array.

--*/

#ifndef _SYSVAD_MICARRAY2WAVTABLE_H_
#define _SYSVAD_MICARRAY2WAVTABLE_H_

//
// Mic array range.
//
#define MICARRAY2_RAW_CHANNELS                   2       // Channels for raw mode
#define MICARRAY2_PROCESSED_CHANNELS             1       // Channels for default mode
#define MICARRAY2_DEVICE_MAX_CHANNELS            2       // Max channels overall
#define MICARRAY2_16_BITS_PER_SAMPLE_PCM         16      // 16 Bits Per Sample
#define MICARRAY2_32_BITS_PER_SAMPLE_PCM         32      // 32 Bits Per Sample
#define MICARRAY2_RAW_SAMPLE_RATE                48000   // Raw sample rate
#define MICARRAY2_PROCESSED_MIN_SAMPLE_RATE      8000    // Min Sample Rate
#define MICARRAY2_PROCESSED_MAX_SAMPLE_RATE      48000   // Max Sample Rate

//
// Max # of pin instances.
//
#define MICARRAY2_MAX_INPUT_STREAMS              4

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE MicArray2PinSupportedDeviceFormats[] =
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
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    // 3 - Note the ENDPOINT_MINIPAIR structures for the mic arrays use this element as the proposed SPEECH format
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
                1,      // One channel
                16000,  // 16KHz
                32000,  // average bytes per second
                2,      // 2 bytes per frame
                16,     // 16 bits per sample container
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            16,         // valid bits per sample
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    // 4
    // 22.05 KHz 16-bit mono
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
    // 5 - Note the ENDPOINT_MINIPAIR structures for the mic arrays use this element as the proposed COMMUNICATIONS format
    // 24 KHz 16-bit mono
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
    // 6
    // 32 KHz 16-bit mono
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
                32000,
                64000,
                2,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            16,
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
    // 7
    // 44.1 KHz 16-bit mono
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
    // 8 - Note the ENDPOINT_MINIPAIR structures for the mic arrays use this last element as the proposed RAW format
    // 48 KHz 32-bit 2 channels
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
                2,
                48000,
                384000,
                8,
                32,
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
            },
            32,
            0,                                      // No channel configuration for unprocessed mic array
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT MicArray2PinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &MicArray2PinSupportedDeviceFormats[SIZEOF_ARRAY(MicArray2PinSupportedDeviceFormats)-1].DataFormat
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &MicArray2PinSupportedDeviceFormats[0].DataFormat
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_SPEECH,
        &MicArray2PinSupportedDeviceFormats[3].DataFormat
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &MicArray2PinSupportedDeviceFormats[5].DataFormat
    }
};

// This structure goes along with the keyword supported device format,
// to indicate that the audio is interleaved microphone and loopback data.
INTERLEAVED_AUDIO_FORMAT_INFORMATION InterleavedFormatInformation = 
{
    sizeof(INTERLEAVED_AUDIO_FORMAT_INFORMATION),
    4, // Microphone channel count
    0, // Microphone channel start position
    0, // No channel configuration for unprocessed mic array
    2, // Channel count for interleaved data
    2,  // Interleaved audio start position
    KSAUDIO_SPEAKER_STEREO // Channel mask for interleaved loopback data
};

//=============================================================================
static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE KeywordPin2SupportedDeviceFormats[] =
{
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
                6,       // six channels, make sure this matches InterleavedFormatInformation, if interleaving loopback audio
                16000,   // 16KHz
                192000,  // average bytes per second
                12,      // 8 bytes per frame
                16,      // 16 bits per sample container
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            16,         // valid bits per sample
            0, // No channel configuration for unprocessed mic array
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
};

static
MODE_AND_DEFAULT_FORMAT KeywordPin2SupportedDeviceModes[] =
{
    // NOTE: Because we're using the KWS apo as an EFX, this endpoint may only support a single mode.
    // You can not have an EFX when there are multiple modes supported by the driver, EFX must be implemented
    // in the driver in that case. Because we are exposing a single mode the audio engine can insert the KWSAPO
    // as an EFX. Further, in the INF associated to this driver the KWS EFX APO is registered as supporting default
    // mode. That registration is not coupled to the mode enumerated in this list.
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_SPEECH,
        &KeywordPin2SupportedDeviceFormats[SIZEOF_ARRAY(KeywordPin2SupportedDeviceFormats) - 1].DataFormat
    },
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES MicArray2PinDeviceFormatsAndModes[] = 
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
        MicArray2PinSupportedDeviceFormats,
        SIZEOF_ARRAY(MicArray2PinSupportedDeviceFormats),
        MicArray2PinSupportedDeviceModes,
        SIZEOF_ARRAY(MicArray2PinSupportedDeviceModes)
    },
    {
        KeywordCapturePin,
        KeywordPin2SupportedDeviceFormats,
        SIZEOF_ARRAY(KeywordPin2SupportedDeviceFormats),
        KeywordPin2SupportedDeviceModes,
        SIZEOF_ARRAY(KeywordPin2SupportedDeviceModes)
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
KSDATARANGE_AUDIO MicArray2PinDataRangesRawStream[] =
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
        MICARRAY2_RAW_CHANNELS,           
        MICARRAY2_32_BITS_PER_SAMPLE_PCM,    
        MICARRAY2_32_BITS_PER_SAMPLE_PCM,    
        MICARRAY2_RAW_SAMPLE_RATE,            
        MICARRAY2_RAW_SAMPLE_RATE             
    },
};

static
KSDATARANGE_AUDIO MicArray2PinDataRangesProcessedStream[] =
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_PROCESSED_MIN_SAMPLE_RATE,
        MICARRAY2_PROCESSED_MIN_SAMPLE_RATE
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        22050,
        22050
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        24000,
        24000
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        32000,
        32000
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        44100,
        44100
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
        MICARRAY2_PROCESSED_CHANNELS,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_16_BITS_PER_SAMPLE_PCM,
        MICARRAY2_PROCESSED_MAX_SAMPLE_RATE,
        MICARRAY2_PROCESSED_MAX_SAMPLE_RATE
    },
};

// if MicArray2PinDataRangesProcessedStream is changed, we MUST update MicArray2PinDataRangePointersStream too!
C_ASSERT(SIZEOF_ARRAY(MicArray2PinDataRangesProcessedStream) == 8);

static
PKSDATARANGE MicArray2PinDataRangePointersStream[] =
{
    // All supported device formats should be listed in the DataRange.
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[2]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[3]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[4]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[5]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[6]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesProcessedStream[7]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&MicArray2PinDataRangesRawStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
};

//=============================================================================
static
KSDATARANGE MicArray2PinDataRangesBridge[] =
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
PKSDATARANGE MicArray2PinDataRangePointersBridge[] =
{
    &MicArray2PinDataRangesBridge[0]
};

static
KSDATARANGE_AUDIO KeywordPin2DataRangesStream[] =
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
        6,      // max channels
        16,     // min bits per sample
        16,     // max bits per sample
        16000,  // min sample rate
        16000   // max sample rate
    },
};

static
PKSDATARANGE KeywordPin2DataRangePointersStream[] =
{
    PKSDATARANGE(&KeywordPin2DataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicArray2WaveMiniportPins[] =
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
            SIZEOF_ARRAY(MicArray2PinDataRangePointersBridge),
            MicArray2PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave In Streaming Pin (Capture) KSPIN_WAVE_HOST
    {
        MICARRAY2_MAX_INPUT_STREAMS,
        MICARRAY2_MAX_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(MicArray2PinDataRangePointersStream),
            MicArray2PinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            &KSAUDFNAME_RECORDING_CONTROL,  
            0
        }
    },
    // Keyword Detector Streaming Pin (Capture) KSPIN_WAVEIN_KEYWORD
    {
        1,
        1,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(KeywordPin2DataRangePointersStream),
            KeywordPin2DataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSNODETYPE_AUDIO_KEYWORDDETECTOR,
            NULL,
            0
        }
    }
};

//=============================================================================
static
PCNODE_DESCRIPTOR MicArray2WaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR MicArray2WaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST },
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_KEYWORD },
};

//=============================================================================
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorSupportedPatterns);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorPatterns);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorArmed);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorArmed);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorMatchResult);

DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorSupportedPatterns2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorPatterns2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorArmed2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorArmed2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorReset2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorStreamingSupport2);
DECLARE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_InterleavedFormatInformation);


static
SYSVADPROPERTY_ITEM PropertiesMicArray2WaveFilter[] =
{
    {
        {
            &KSPROPSETID_General,
            KSPROPERTY_GENERAL_COMPONENTID,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            PropertyHandler_WaveFilter,
        },
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_Pin,
            KSPROPERTY_PIN_PROPOSEDATAFORMAT,
            KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            PropertyHandler_WaveFilter,
        },
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_Pin,
            KSPROPERTY_PIN_PROPOSEDATAFORMAT2,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            PropertyHandler_WaveFilter,
        },
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_AudioEffectsDiscovery,
            KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            PropertyHandler_WaveFilter
        },
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector,
            KSPROPERTY_SOUNDDETECTOR_SUPPORTEDPATTERNS,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        0,
        sizeof(CONTOSO_SUPPORTEDPATTERNSVALUE),
        CMiniportWaveRT_Get_SoundDetectorSupportedPatterns,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector,
            KSPROPERTY_SOUNDDETECTOR_PATTERNS,
            KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        0,
        (sizeof(KSMULTIPLE_ITEM) + sizeof(CONTOSO_KEYWORDCONFIGURATION)),
        NULL,
        CMiniportWaveRT_Set_SoundDetectorPatterns,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector,
            KSPROPERTY_SOUNDDETECTOR_ARMED,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        0,
        sizeof(BOOL),
        CMiniportWaveRT_Get_SoundDetectorArmed,
        CMiniportWaveRT_Set_SoundDetectorArmed,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector,
            KSPROPERTY_SOUNDDETECTOR_MATCHRESULT,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        0,
        sizeof(CONTOSO_KEYWORDDETECTIONRESULT),
        CMiniportWaveRT_Get_SoundDetectorMatchResult,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector2,
            KSPROPERTY_SOUNDDETECTOR_SUPPORTEDPATTERNS,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSSOUNDDETECTORPROPERTY) - sizeof(KSPROPERTY),
        sizeof(CONTOSO_SUPPORTEDPATTERNSVALUE),
        CMiniportWaveRT_Get_SoundDetectorSupportedPatterns2,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector2,
            KSPROPERTY_SOUNDDETECTOR_PATTERNS,
            KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSSOUNDDETECTORPROPERTY) - sizeof(KSPROPERTY),
        (sizeof(KSMULTIPLE_ITEM) + sizeof(CONTOSO_KEYWORDCONFIGURATION)),
        NULL,
        CMiniportWaveRT_Set_SoundDetectorPatterns2,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector2,
            KSPROPERTY_SOUNDDETECTOR_ARMED,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSSOUNDDETECTORPROPERTY) - sizeof(KSPROPERTY),
        sizeof(BOOL),
        CMiniportWaveRT_Get_SoundDetectorArmed2,
        CMiniportWaveRT_Set_SoundDetectorArmed2,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector2,
            KSPROPERTY_SOUNDDETECTOR_RESET,
            KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSSOUNDDETECTORPROPERTY) - sizeof(KSPROPERTY),
        sizeof(BOOL),
        NULL,
        CMiniportWaveRT_Set_SoundDetectorReset2,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_SoundDetector2,
            KSPROPERTY_SOUNDDETECTOR_STREAMINGSUPPORT,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSSOUNDDETECTORPROPERTY) - sizeof(KSPROPERTY),
        sizeof(BOOL),
        CMiniportWaveRT_Get_SoundDetectorStreamingSupport2,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        {
            &KSPROPSETID_InterleavedAudio,
            KSPROPERTY_INTERLEAVEDAUDIO_FORMATINFORMATION,
            KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
            SysvadPropertyDispatch,
        },
        sizeof(KSP_PIN) - sizeof(KSPROPERTY),
        sizeof(INTERLEAVED_AUDIO_FORMAT_INFORMATION),
        CMiniportWaveRT_Get_InterleavedFormatInformation,
        NULL,
        NULL,
        &InterleavedFormatInformation, // format interleaving information for this endpoint
        sizeof(InterleavedFormatInformation)
    },
};

NTSTATUS CMiniportWaveRT_EventHandler_SoundDetectorMatchDetected(
    _In_  PPCEVENT_REQUEST EventRequest
    );

static
PCEVENT_ITEM EventsMicArray2WaveFilter[] =
{
    {
        &KSEVENTSETID_SoundDetector,
        KSEVENT_SOUNDDETECTOR_MATCHDETECTED,
        KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
        CMiniportWaveRT_EventHandler_SoundDetectorMatchDetected,
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationMicArray2WaveFilter, PropertiesMicArray2WaveFilter, EventsMicArray2WaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicArray2WaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationMicArray2WaveFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(MicArray2WaveMiniportPins),         // PinCount
    MicArray2WaveMiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(MicArray2WaveMiniportNodes),        // NodeCount
    MicArray2WaveMiniportNodes,                      // Nodes
    SIZEOF_ARRAY(MicArray2WaveMiniportConnections),  // ConnectionCount
    MicArray2WaveMiniportConnections,                // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};


#endif // _SYSVAD_MICARRAY2WAVTABLE_H_

