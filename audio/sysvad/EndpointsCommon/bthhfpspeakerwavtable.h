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



#define BTHHFPSPEAKER_DEVICE_MAX_CHANNELS                 1         // Max Channels.

#define BTHHFPSPEAKER_HOST_MAX_CHANNELS                   1         // Max Channels.
#define BTHHFPSPEAKER_HOST_MIN_BITS_PER_SAMPLE            16        // Min Bits Per Sample
#define BTHHFPSPEAKER_HOST_MAX_BITS_PER_SAMPLE            16        // Max Bits Per Sample
#define BTHHFPSPEAKER_HOST_MIN_SAMPLE_RATE                8000      // Min Sample Rate
#define BTHHFPSPEAKER_HOST_MAX_SAMPLE_RATE                8000      // Max Sample Rate

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
        BridgePin,
        NULL,
        0,
        NULL,
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
    }
};

static
PKSDATARANGE BthHfpSpeakerPinDataRangePointersStream[] =
{
    PKSDATARANGE(&BthHfpSpeakerPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
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
PCCONNECTION_DESCRIPTOR BthHfpSpeakerWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER3_SINK_SYSTEM,     KSNODE_WAVE_DAC,   1 },
    { KSNODE_WAVE_DAC,          0,                                  PCFILTER_NODE,     KSPIN_WAVE_RENDER3_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesBthHfpSpeakerWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
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

NTSTATUS CMiniportWaveRT_EventHandler_PinCapsChange(
    _In_  PPCEVENT_REQUEST EventRequest
    );

static PCEVENT_ITEM BthHfpSpeakerFormatChangePinEvent[] = {
    {
        &KSEVENTSETID_PinCapsChange,
        KSEVENT_PINCAPS_FORMATCHANGE,
        KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
        CMiniportWaveRT_EventHandler_PinCapsChange
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationBthHfpSpeakerWaveFilter, PropertiesBthHfpSpeakerWaveFilter, BthHfpSpeakerFormatChangePinEvent);


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

