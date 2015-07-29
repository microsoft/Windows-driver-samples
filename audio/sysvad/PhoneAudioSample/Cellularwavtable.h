/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellularwavtable.h

Abstract:

    Declaration of wave miniport tables for the cellular endpoint.

--*/

#ifndef _SYSVAD_CELLULARWAVTABLE_H_
#define _SYSVAD_CELLULARWAVTABLE_H_


#define CELLULAR_DEVICE_MAX_CHANNELS                 1       // Max Channels.

#define CELLULAR_CAPTURE_MAX_CHANNELS                1       // Max Channels.
#define CELLULAR_CAPTURE_MIN_BITS_PER_SAMPLE         16      // Min Bits Per Sample
#define CELLULAR_CAPTURE_MAX_BITS_PER_SAMPLE         16      // Max Bits Per Sample
#define CELLULAR_CAPTURE_MIN_SAMPLE_RATE             16000   // Min Sample Rate
#define CELLULAR_CAPTURE_MAX_SAMPLE_RATE             16000   // Max Sample Rate

//
// Max # of pin instances.
//
#define CELLULAR_MAX_INPUT_SYSTEM_STREAMS            1       // Raw stream


//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE CellularCapturePinSupportedDeviceFormats[] =
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
    },
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT CellularCapturePinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &CellularCapturePinSupportedDeviceFormats[SIZEOF_ARRAY(CellularCapturePinSupportedDeviceFormats) - 1].DataFormat
    }
};

//=============================================================================
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static
PIN_DEVICE_FORMATS_AND_MODES CellularPinDeviceFormatsAndModes[] = 
{
    { // BIDI
        TelephonyBidiPin,
        CellularCapturePinSupportedDeviceFormats,
        SIZEOF_ARRAY(CellularCapturePinSupportedDeviceFormats),
        CellularCapturePinSupportedDeviceModes,
        SIZEOF_ARRAY(CellularCapturePinSupportedDeviceModes)
    },
    {
        BridgePin,
        NULL,
        0,
        NULL,
        0,
    }
};

//=============================================================================
static
KSDATARANGE_AUDIO CellularPinDataRangesStream[] =
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
        CELLULAR_CAPTURE_MAX_CHANNELS,           
        CELLULAR_CAPTURE_MIN_BITS_PER_SAMPLE,    
        CELLULAR_CAPTURE_MAX_BITS_PER_SAMPLE,    
        CELLULAR_CAPTURE_MIN_SAMPLE_RATE,            
        CELLULAR_CAPTURE_MAX_SAMPLE_RATE             
    }
};

static
PKSDATARANGE CellularPinDataRangePointersBiDiStream[] =
{
    PKSDATARANGE(&CellularPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

//=============================================================================
static
KSDATARANGE CellularPinDataRangesBridge[] =
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
PKSDATARANGE CellularPinDataRangePointersBridge[] =
{
    &CellularPinDataRangesBridge[0]
};


//=============================================================================
static
PCPIN_DESCRIPTOR CellularWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_BIDI
    {
        CELLULAR_MAX_INPUT_SYSTEM_STREAMS,
        CELLULAR_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(CellularPinDataRangePointersBiDiStream),
            CellularPinDataRangePointersBiDiStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Bridge Pin (Renderer) KSPIN_WAVE_BIDI_BRIDGE
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
            SIZEOF_ARRAY(CellularPinDataRangePointersBridge),
            CellularPinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    }
};

//=============================================================================
//
// Note: 
//
//                       -----
//                       |   |      
//    BiDi pin 1      <--|   |<-- 0 BiDi bridge pin
//                       -----

static
PCCONNECTION_DESCRIPTOR CellularWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_BIDI_BRIDGE, PCFILTER_NODE,              KSPIN_WAVE_BIDI }
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesCellularWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_PROVIDERID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_CALLINFO,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_CALLCONTROL,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_PROVIDERCHANGE,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_CALLHOLD,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },

    {
        &KSPROPSETID_TelephonyControl,
        KSPROPERTY_TELEPHONY_MUTE_TX,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularWaveFilter
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationCellularWaveFilter, PropertiesCellularWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR CellularWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationCellularWaveFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(CellularWaveMiniportPins),         // PinCount
    CellularWaveMiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    0,                                              // NodeCount
    NULL,                                           // Nodes
    SIZEOF_ARRAY(CellularWaveMiniportConnections),  // ConnectionCount
    CellularWaveMiniportConnections,                // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_CELLULARWAVTABLE_H_

