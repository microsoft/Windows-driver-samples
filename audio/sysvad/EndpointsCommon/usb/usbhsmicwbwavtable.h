/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpmicwbwavtable.h

Abstract:

    Declaration of wave miniport tables for the Bluetooth handls-free profile (external),
    for Bluetooth connections that support Wideband Speech.

--*/

#ifndef _SYSVAD_BTHHFPMICWIDEBANDWAVTABLE_H_
#define _SYSVAD_BTHHFPMICWBWAVTABLE_H_

//
// Function prototypes.
//
NTSTATUS PropertyHandler_BthHfpWaveFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

//
// Bluetooth Headset Mic (external) range.
//
#define BTHHFPMICWB_DEVICE_MAX_CHANNELS           1       // Max Channels.
#define BTHHFPMICWB_MIN_BITS_PER_SAMPLE_PCM       8       // Min Bits Per Sample
#define BTHHFPMICWB_MAX_BITS_PER_SAMPLE_PCM       16      // Max Bits Per Sample
#define BTHHFPMICWB_MIN_SAMPLE_RATE               8000    // Min Sample Rate
#define BTHHFPMICWB_MAX_SAMPLE_RATE               16000    // Max Sample Rate


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpMicWBPinSupportedDeviceFormats[] =
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
    }
};

//
// Supported modes (only on streaming pins) - NREC not supported.
//
static
MODE_AND_DEFAULT_FORMAT BthHfpMicWBPinSupportedDeviceModesNoNrec[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpMicWBPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpMicWBPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// Supported modes (only on streaming pins) - NREC supported.
//
static
MODE_AND_DEFAULT_FORMAT BthHfpMicWBPinSupportedDeviceModesNrec[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &BthHfpMicWBPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpMicWBPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES BthHfpMicWBPinDeviceFormatsAndModes[] = 
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
        BthHfpMicWBPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpMicWBPinSupportedDeviceFormats),
        NULL,   // Init dynamically, see GetCapturePinSupportedDeviceModes[Count]
        0       // Init dynamically, see GetCapturePinSupportedDeviceModes[Count]
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO BthHfpMicWBPinDataRangesStream[] =
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
        BTHHFPMICWB_DEVICE_MAX_CHANNELS,           
        BTHHFPMICWB_MIN_BITS_PER_SAMPLE_PCM,    
        BTHHFPMICWB_MAX_BITS_PER_SAMPLE_PCM,    
        BTHHFPMICWB_MIN_SAMPLE_RATE,            
        BTHHFPMICWB_MIN_SAMPLE_RATE             
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
        BTHHFPMICWB_DEVICE_MAX_CHANNELS,
        BTHHFPMICWB_MIN_BITS_PER_SAMPLE_PCM,
        BTHHFPMICWB_MAX_BITS_PER_SAMPLE_PCM,
        BTHHFPMICWB_MAX_SAMPLE_RATE,
        BTHHFPMICWB_MAX_SAMPLE_RATE
    },
};

static
PKSDATARANGE BthHfpMicWBPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpMicWBPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList),
    PKSDATARANGE(&BthHfpMicWBPinDataRangesStream[1]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

//=============================================================================
static
KSDATARANGE BthHfpMicWBPinDataRangesBridge[] =
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
PKSDATARANGE BthHfpMicWBPinDataRangePointersBridge[] =
{
    &BthHfpMicWBPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR BthHfpMicWBWaveMiniportPins[] =
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
            SIZEOF_ARRAY(BthHfpMicWBPinDataRangePointersBridge),
            BthHfpMicWBPinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
  
    // Wave In Streaming Pin (Capture) KSPIN_WAVEIN_HOST
    {
        MAX_INPUT_STREAMS,
        MAX_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(BthHfpMicWBPinDataRangePointersStream),
            BthHfpMicWBPinDataRangePointersStream,
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
PCNODE_DESCRIPTOR BthHfpMicWBWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR BthHfpMicWBWaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST }
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesBthHfpMicWBWaveFilter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpMicWBWaveFilter, PropertiesBthHfpMicWBWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR BthHfpMicWBWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationBthHfpMicWBWaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(BthHfpMicWBWaveMiniportPins),        // PinCount
    BthHfpMicWBWaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(BthHfpMicWBWaveMiniportNodes),       // NodeCount
    BthHfpMicWBWaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(BthHfpMicWBWaveMiniportConnections), // ConnectionCount
    BthHfpMicWBWaveMiniportConnections,               // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_BTHHFPMICWBWAVTABLE_H_
