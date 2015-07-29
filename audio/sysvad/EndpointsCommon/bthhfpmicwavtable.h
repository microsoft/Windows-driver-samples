/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpmicwavtable.h

Abstract:

    Declaration of wave miniport tables for the Bluetooth handls-free profile (external).

--*/

#ifndef _SYSVAD_BTHHFPMICWAVTABLE_H_
#define _SYSVAD_BTHHFPMICWAVTABLE_H_

//
// Function prototypes.
//
NTSTATUS PropertyHandler_BthHfpWaveFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

//
// Bluetooth Headset Mic (external) range.
//
#define BTHHFPMIC_DEVICE_MAX_CHANNELS           1       // Max Channels.
#define BTHHFPMIC_MIN_BITS_PER_SAMPLE_PCM       8       // Min Bits Per Sample
#define BTHHFPMIC_MAX_BITS_PER_SAMPLE_PCM       16      // Max Bits Per Sample
#define BTHHFPMIC_MIN_SAMPLE_RATE               8000    // Min Sample Rate
#define BTHHFPMIC_MAX_SAMPLE_RATE               8000    // Max Sample Rate


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE BthHfpMicPinSupportedDeviceFormats[] =
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
    }
};

//
// Supported modes (only on streaming pins) - NREC not supported.
//
static
MODE_AND_DEFAULT_FORMAT BthHfpMicPinSupportedDeviceModesNoNrec[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &BthHfpMicPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpMicPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// Supported modes (only on streaming pins) - NREC supported.
//
static
MODE_AND_DEFAULT_FORMAT BthHfpMicPinSupportedDeviceModesNrec[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &BthHfpMicPinSupportedDeviceFormats[SIZEOF_ARRAY(BthHfpMicPinSupportedDeviceFormats)-1].DataFormat   
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES BthHfpMicPinDeviceFormatsAndModes[] = 
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
        BthHfpMicPinSupportedDeviceFormats,
        SIZEOF_ARRAY(BthHfpMicPinSupportedDeviceFormats),
        NULL,   // Init dynamically, see GetCapturePinSupportedDeviceModes[Count]
        0       // Init dynamically, see GetCapturePinSupportedDeviceModes[Count]
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO BthHfpMicPinDataRangesStream[] =
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
        BTHHFPMIC_DEVICE_MAX_CHANNELS,           
        BTHHFPMIC_MIN_BITS_PER_SAMPLE_PCM,    
        BTHHFPMIC_MAX_BITS_PER_SAMPLE_PCM,    
        BTHHFPMIC_MIN_SAMPLE_RATE,            
        BTHHFPMIC_MAX_SAMPLE_RATE             
    },
};

static
PKSDATARANGE BthHfpMicPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpMicPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

//=============================================================================
static
KSDATARANGE BthHfpMicPinDataRangesBridge[] =
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
PKSDATARANGE BthHfpMicPinDataRangePointersBridge[] =
{
    &BthHfpMicPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR BthHfpMicWaveMiniportPins[] =
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
            SIZEOF_ARRAY(BthHfpMicPinDataRangePointersBridge),
            BthHfpMicPinDataRangePointersBridge,
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
            SIZEOF_ARRAY(BthHfpMicPinDataRangePointersStream),
            BthHfpMicPinDataRangePointersStream,
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
PCNODE_DESCRIPTOR BthHfpMicWaveMiniportNodes[] =
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
PCCONNECTION_DESCRIPTOR BthHfpMicWaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST }
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesBthHfpMicWaveFilter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpMicWaveFilter, PropertiesBthHfpMicWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR BthHfpMicWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationBthHfpMicWaveFilter,                 // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(BthHfpMicWaveMiniportPins),        // PinCount
    BthHfpMicWaveMiniportPins,                      // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(BthHfpMicWaveMiniportNodes),       // NodeCount
    BthHfpMicWaveMiniportNodes,                     // Nodes
    SIZEOF_ARRAY(BthHfpMicWaveMiniportConnections), // ConnectionCount
    BthHfpMicWaveMiniportConnections,               // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_BTHHFPMICWAVTABLE_H_
