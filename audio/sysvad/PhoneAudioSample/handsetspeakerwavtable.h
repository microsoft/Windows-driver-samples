/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    handsetspeakerwavtable.h

Abstract:

    Declaration of wave miniport tables for the handsetspeaker endpoint.

    Note: the HANDSETSPEAKER in this sample shows how to do h/w loopback 
          for non-offloaded devices.

--*/

#ifndef _SYSVAD_HANDSETSPEAKERWAVTABLE_H_
#define _SYSVAD_HANDSETSPEAKERWAVTABLE_H_


// To keep the code simple assume device supports only 48KHz, 16-bit, stereo (PCM and NON-PCM)


#define HANDSETSPEAKER_DEVICE_MAX_CHANNELS                 2       // Max Channels.

#define HANDSETSPEAKER_HOST_MAX_CHANNELS                   2       // Max Channels.
#define HANDSETSPEAKER_HOST_MIN_BITS_PER_SAMPLE            16      // Min Bits Per Sample
#define HANDSETSPEAKER_HOST_MAX_BITS_PER_SAMPLE            16      // Max Bits Per Sample
#define HANDSETSPEAKER_HOST_MIN_SAMPLE_RATE                24000   // Min Sample Rate
#define HANDSETSPEAKER_HOST_MAX_SAMPLE_RATE                96000   // Max Sample Rate

#define HANDSETSPEAKER_LOOPBACK_MAX_CHANNELS               HANDSETSPEAKER_HOST_MAX_CHANNELS          // Must be equal to host pin's Max Channels.
#define HANDSETSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE        HANDSETSPEAKER_HOST_MIN_BITS_PER_SAMPLE   // Must be equal to host pin's Min Bits Per Sample
#define HANDSETSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE        HANDSETSPEAKER_HOST_MAX_BITS_PER_SAMPLE   // Must be equal to host pin's Max Bits Per Sample
#define HANDSETSPEAKER_LOOPBACK_MIN_SAMPLE_RATE            HANDSETSPEAKER_HOST_MIN_SAMPLE_RATE       // Must be equal to host pin's Min Sample Rate
#define HANDSETSPEAKER_LOOPBACK_MAX_SAMPLE_RATE            HANDSETSPEAKER_HOST_MAX_SAMPLE_RATE       // Must be equal to host pin's Max Sample Rate

#define HANDSETSPEAKER_DOLBY_DIGITAL_MAX_CHANNELS          2       // Max Channels.
#define HANDSETSPEAKER_DOLBY_DIGITAL_MIN_BITS_PER_SAMPLE   16      // Min Bits Per Sample
#define HANDSETSPEAKER_DOLBY_DIGITAL_MAX_BITS_PER_SAMPLE   16      // Max Bits Per Sample
#define HANDSETSPEAKER_DOLBY_DIGITAL_MIN_SAMPLE_RATE       44100   // Min Sample Rate
#define HANDSETSPEAKER_DOLBY_DIGITAL_MAX_SAMPLE_RATE       44100   // Max Sample Rate

//
// Max # of pin instances.
//
#define HANDSETSPEAKER_MAX_INPUT_SYSTEM_STREAMS            3
#define HANDSETSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS         MAX_OUTPUT_LOOPBACK_STREAMS

//=============================================================================
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE HandsetSpeakerHostPinSupportedDeviceFormats[] =
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
                24000,
                96000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
                32000,
                128000,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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
                44100,
                176400,
                4,
                16,
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
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

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT HandsetSpeakerHostPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &HandsetSpeakerHostPinSupportedDeviceFormats[3].DataFormat, // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        &HandsetSpeakerHostPinSupportedDeviceFormats[3].DataFormat, // 48KHz
    },
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        &HandsetSpeakerHostPinSupportedDeviceFormats[0].DataFormat, // 24KHz
    }
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES HandsetSpeakerPinDeviceFormatsAndModes[] = 
{
    {
        SystemRenderPin,
        HandsetSpeakerHostPinSupportedDeviceFormats,
        SIZEOF_ARRAY(HandsetSpeakerHostPinSupportedDeviceFormats),
        HandsetSpeakerHostPinSupportedDeviceModes,
        SIZEOF_ARRAY(HandsetSpeakerHostPinSupportedDeviceModes)
    },
    {
        RenderLoopbackPin,
        HandsetSpeakerHostPinSupportedDeviceFormats,   // Must support all the formats supported by host pin
        SIZEOF_ARRAY(HandsetSpeakerHostPinSupportedDeviceFormats),
        NULL,   // loopback doesn't support modes.
        0
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
KSDATARANGE_AUDIO HandsetSpeakerPinDataRangesStream[] =
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
        HANDSETSPEAKER_HOST_MAX_CHANNELS,           
        HANDSETSPEAKER_HOST_MIN_BITS_PER_SAMPLE,    
        HANDSETSPEAKER_HOST_MAX_BITS_PER_SAMPLE,    
        HANDSETSPEAKER_HOST_MIN_SAMPLE_RATE,            
        HANDSETSPEAKER_HOST_MAX_SAMPLE_RATE             
    },
    { // 1
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        HANDSETSPEAKER_LOOPBACK_MAX_CHANNELS,           
        HANDSETSPEAKER_LOOPBACK_MIN_BITS_PER_SAMPLE,    
        HANDSETSPEAKER_LOOPBACK_MAX_BITS_PER_SAMPLE,    
        HANDSETSPEAKER_LOOPBACK_MIN_SAMPLE_RATE,
        HANDSETSPEAKER_LOOPBACK_MAX_SAMPLE_RATE
    }
};

static
PKSDATARANGE HandsetSpeakerPinDataRangePointersStream[] =
{
    PKSDATARANGE(&HandsetSpeakerPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

static
PKSDATARANGE HandsetSpeakerPinDataRangePointersLoopbackStream[] =
{
    PKSDATARANGE(&HandsetSpeakerPinDataRangesStream[1])
};

//=============================================================================
static
KSDATARANGE HandsetSpeakerPinDataRangesBridge[] =
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
PKSDATARANGE HandsetSpeakerPinDataRangePointersBridge[] =
{
    &HandsetSpeakerPinDataRangesBridge[0]
};


//=============================================================================
static
PCPIN_DESCRIPTOR HandsetSpeakerWaveMiniportPins[] =
{
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_SYSTEM
    {
        HANDSETSPEAKER_MAX_INPUT_SYSTEM_STREAMS,
        HANDSETSPEAKER_MAX_INPUT_SYSTEM_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(HandsetSpeakerPinDataRangePointersStream),
            HandsetSpeakerPinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer) KSPIN_WAVE_RENDER2_SINK_LOOPBACK
    {
        HANDSETSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS,
        HANDSETSPEAKER_MAX_OUTPUT_LOOPBACK_STREAMS, 
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(HandsetSpeakerPinDataRangePointersLoopbackStream),
            HandsetSpeakerPinDataRangePointersLoopbackStream,
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
            SIZEOF_ARRAY(HandsetSpeakerPinDataRangePointersBridge),
            HandsetSpeakerPinDataRangePointersBridge,
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
PCPROPERTY_ITEM HandsetSpeakerPropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetSpeakerVolume, HandsetSpeakerPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM HandsetSpeakerPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetSpeakerMute, HandsetSpeakerPropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM HandsetSpeakerPropertiesPeakMeter[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_PEAKMETER2,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  },
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_CPU_RESOURCES,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_WaveFilter
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetSpeakerPeakMeter, HandsetSpeakerPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR HandsetSpeakerWaveMiniportNodes[] =
{
    // KSNODE_WAVE_SUM
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_SUM,            // Type
        NULL                        // Name
    },
    // KSNODE_WAVE_VOLUME
    {
      0,                            // Flags
      &AutomationHandsetSpeakerVolume,        // AutomationTable
      &KSNODETYPE_VOLUME,           // Type
      &KSAUDFNAME_WAVE_VOLUME       // Name
    },
    // KSNODE_WAVE_MUTE
    {
      0,                            // Flags
      &AutomationHandsetSpeakerMute,          // AutomationTable
      &KSNODETYPE_MUTE,             // Type
      &KSAUDFNAME_WAVE_MUTE         // Name
    },
    // KSNODE_WAVE_PEAKMETER
    {
      0,                            // Flags
      &AutomationHandsetSpeakerPeakMeter,     // AutomationTable
      &KSNODETYPE_PEAKMETER,        // Type
      &KSAUDFNAME_PEAKMETER         // Name
    }
};

C_ASSERT( KSNODE_WAVE_SUM == 0 );
C_ASSERT( KSNODE_WAVE_VOLUME  == 1 );
C_ASSERT( KSNODE_WAVE_MUTE    == 2 );
C_ASSERT( KSNODE_WAVE_PEAKMETER == 3 );


//=============================================================================
//
// Note: 
// 1) A sum node can be used to 'sum' multiple streams associated to different
//    instances of the same pin.  
// 2) Any node output has an implicit 'tee' node associated with it. Thus in the 
//    scenario below it is ok for the peak meter node to have two identical outputs.
//
//                       ----------------------------------------------------------
//                       |                                                        |      
//    System Pin 0       |    |-----|    |--------|   |------|   |-----------|    |--> 1 Loopback Pin
//   [instances:0...n]-->| -->| Sum |--->| Volume |-->| Mute |-->| PeakMeter |--> |       
//                       |    |-----|    |--------|   |------|   |-----------|    |--> 2 KSPIN_WAVE_RENDER2_SOURCE
//                       |                                                        | 
//                       ----------------------------------------------------------

static
PCCONNECTION_DESCRIPTOR HandsetSpeakerWaveMiniportConnections[] =
{
    { PCFILTER_NODE,            KSPIN_WAVE_RENDER2_SINK_SYSTEM, KSNODE_WAVE_SUM,            1 },
    { KSNODE_WAVE_SUM,          0,                              KSNODE_WAVE_VOLUME,         1 },
    { KSNODE_WAVE_VOLUME,       0,                              KSNODE_WAVE_MUTE,           1 },
    { KSNODE_WAVE_MUTE,         0,                              KSNODE_WAVE_PEAKMETER,      1 },
    { KSNODE_WAVE_PEAKMETER,    2,                              PCFILTER_NODE,              KSPIN_WAVE_RENDER2_SINK_LOOPBACK },
    { KSNODE_WAVE_PEAKMETER,    0,                              PCFILTER_NODE,              KSPIN_WAVE_RENDER2_SOURCE },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesHandsetSpeakerWaveFilter[] =
{
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetSpeakerWaveFilter, PropertiesHandsetSpeakerWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR HandsetSpeakerWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationHandsetSpeakerWaveFilter,                      // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(HandsetSpeakerWaveMiniportPins),             // PinCount
    HandsetSpeakerWaveMiniportPins,                           // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(HandsetSpeakerWaveMiniportNodes),            // NodeCount
    HandsetSpeakerWaveMiniportNodes,                          // Nodes
    SIZEOF_ARRAY(HandsetSpeakerWaveMiniportConnections),      // ConnectionCount
    HandsetSpeakerWaveMiniportConnections,                    // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};

#endif // _SYSVAD_HANDSETSPEAKERWAVTABLE_H_

