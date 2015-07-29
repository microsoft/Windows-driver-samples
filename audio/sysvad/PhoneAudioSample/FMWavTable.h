/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    fmwavtable.h

Abstract:

    Declaration of wave miniport tables for the fm endpoint.

    Note: the FM in this sample shows how to do h/w loopback 
          for non-offloaded devices.

--*/

#ifndef _SYSVAD_FMWAVTABLE_H_
#define _SYSVAD_FMWAVTABLE_H_


#define FMRX_DEVICE_MAX_CHANNELS                 2       // Max Channels.

#define FMRX_MAX_CHANNELS                2       // Max Channels.
#define FMRX_MIN_BITS_PER_SAMPLE         16      // Min Bits Per Sample
#define FMRX_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define FMRX_MIN_SAMPLE_RATE             48000   // Min Sample Rate
#define FMRX_MAX_SAMPLE_RATE             48000   // Max Sample Rate
#define FMRX_MAX_INPUT_SYSTEM_STREAMS    1       // Raw stream


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE FmRxPinSupportedDeviceFormats[] =
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
MODE_AND_DEFAULT_FORMAT FmRxPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &FmRxPinSupportedDeviceFormats[SIZEOF_ARRAY(FmRxPinSupportedDeviceFormats) - 1].DataFormat
    }
};

//=============================================================================
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static
PIN_DEVICE_FORMATS_AND_MODES FmRxPinDeviceFormatsAndModes[] = 
{
    { // RX
        SystemCapturePin,
        FmRxPinSupportedDeviceFormats,
        SIZEOF_ARRAY(FmRxPinSupportedDeviceFormats),
        FmRxPinSupportedDeviceModes,
        SIZEOF_ARRAY(FmRxPinSupportedDeviceModes)
    },
    {
        BridgePin,
        NULL,
        0,
        NULL,
        0,
    },
};

//=============================================================================
static
KSDATARANGE_AUDIO FmPinDataRangesStream[] =
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
        FMRX_MAX_CHANNELS,           
        FMRX_MIN_BITS_PER_SAMPLE,    
        FMRX_MAX_BITS_PER_SAMPLE,    
        FMRX_MIN_SAMPLE_RATE,            
        FMRX_MAX_SAMPLE_RATE             
    },
};

static
PKSDATARANGE FmPinDataRangePointersStream[] =
{
    PKSDATARANGE(&FmPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

//=============================================================================
static
KSDATARANGE FmPinDataRangesBridge[] =
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
PKSDATARANGE FmPinDataRangePointersBridge[] =
{
    &FmPinDataRangesBridge[0]
};


//=============================================================================
static
PCPIN_DESCRIPTOR FmWaveMiniportPins[] =
{
    // Wave In Streaming Pin KSPIN_WAVE_FMRX
    {
        FMRX_MAX_INPUT_SYSTEM_STREAMS,
        FMRX_MAX_INPUT_SYSTEM_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(FmPinDataRangePointersStream),
            FmPinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave In Bridge Pin KSPIN_WAVE_FMRX_BRIDGE
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
            SIZEOF_ARRAY(FmPinDataRangePointersBridge),
            FmPinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
};

//=============================================================================
//
//                         -----
//                         |   |      
//   FM RX bridge pin 1 -->|   |--> 0 FM RX pin
//                         |   |
//                         -----

static
PCCONNECTION_DESCRIPTOR FmWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_FMRX_BRIDGE,   PCFILTER_NODE,              KSPIN_WAVE_FMRX },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesFmWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },

    {
        &KSPROPSETID_FMRXControl,
        KSPROPERTY_FMRX_STATE,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxWaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationFmWaveFilter, PropertiesFmWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR FmWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationFmWaveFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(FmWaveMiniportPins),         // PinCount
    FmWaveMiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    0,                                              // NodeCount
    NULL,                                           // Nodes
    SIZEOF_ARRAY(FmWaveMiniportConnections),  // ConnectionCount
    FmWaveMiniportConnections,                // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_FMWAVTABLE_H_

