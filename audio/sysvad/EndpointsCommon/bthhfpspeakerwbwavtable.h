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
#define BTHHFPSPEAKERWB_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define BTHHFPSPEAKERWB_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define BTHHFPSPEAKERWB_HOST_MIN_SAMPLE_RATE                16000   // Min Sample Rate
#define BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE                16000   // Max Sample Rate

//=============================================================================
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
        BridgePin,
        NULL,
        0,
        NULL,
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
        BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE,
        BTHHFPSPEAKERWB_HOST_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE BthHfpSpeakerWBPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerWBPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
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
    // KSNODE_WAVE_DAC
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_DAC,            // Type KSNODETYPE_DAC
        NULL                        // Name
    }
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR BthHfpSpeakerWBWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER3_SINK_SYSTEM,     KSNODE_WAVE_DAC,   1 },
    { KSNODE_WAVE_DAC,          0,                                  PCFILTER_NODE,     KSPIN_WAVE_RENDER3_SOURCE },
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
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
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

