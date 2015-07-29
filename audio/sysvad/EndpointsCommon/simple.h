/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    simple.h

Abstract:

    Node and Pin numbers and other common definitions for simple sample.

--*/

#ifndef _SYSVAD_SIMPLE_H_
#define _SYSVAD_SIMPLE_H_

// Name Guid
// {946A7B1A-EBBC-422a-A81F-F07C8D40D3B4}
#define STATIC_NAME_SYSVAD_SIMPLE\
    0x946a7b1a, 0xebbc, 0x422a, 0xa8, 0x1f, 0xf0, 0x7c, 0x8d, 0x40, 0xd3, 0xb4
DEFINE_GUIDSTRUCT("946A7B1A-EBBC-422a-A81F-F07C8D40D3B4", NAME_SYSVAD_SIMPLE);
#define NAME_SYSVAD_SIMPLE DEFINE_GUIDNAMED(NAME_SYSVAD_SIMPLE)

//----------------------------------------------------
// New defines for the render endpoints.
//----------------------------------------------------

// Default pin instances.
#define MAX_INPUT_SYSTEM_STREAMS        1
#define MAX_INPUT_OFFLOAD_STREAMS       3
#define MAX_OUTPUT_LOOPBACK_STREAMS     1

// Wave pins - offloading supported.
enum 
{
    KSPIN_WAVE_RENDER_SINK_SYSTEM = 0, 
    KSPIN_WAVE_RENDER_SINK_OFFLOAD, 
    KSPIN_WAVE_RENDER_SINK_LOOPBACK, 
    KSPIN_WAVE_RENDER_SOURCE
};

// Wave Topology nodes - offloading supported.
enum 
{
    KSNODE_WAVE_AUDIO_ENGINE = 0
};

// Wave pins - offloading is NOT supported.
enum 
{
    KSPIN_WAVE_RENDER2_SINK_SYSTEM = 0, 
    KSPIN_WAVE_RENDER2_SINK_LOOPBACK, 
    KSPIN_WAVE_RENDER2_SOURCE
};

// Wave pins - cellular
enum 
{
    KSPIN_WAVE_BIDI = 0,
    KSPIN_WAVE_BIDI_BRIDGE
};

// Wave Topology nodes - offloading is NOT supported.
enum 
{
    KSNODE_WAVE_SUM = 0,
    KSNODE_WAVE_VOLUME,
    KSNODE_WAVE_MUTE,
    KSNODE_WAVE_PEAKMETER
};

// Topology pins.
enum
{
    KSPIN_TOPO_WAVEOUT_SOURCE = 0,
    KSPIN_TOPO_LINEOUT_DEST,
};

// Topology pins - cellular
enum
{
    KSPIN_TOPO_BIDI1 = 0,
    KSPIN_TOPO_BIDI1_BRIDGE,
    KSPIN_TOPO_BIDI2,
    KSPIN_TOPO_BIDI2_BRIDGE
};

// Topology nodes.
enum
{
    KSNODE_TOPO_WAVEOUT_VOLUME = 0,
    KSNODE_TOPO_WAVEOUT_MUTE,
    KSNODE_TOPO_WAVEOUT_PEAKMETER
};

//----------------------------------------------------
// New defines for the capture endpoints.
//----------------------------------------------------

// Default pin instances.
#define MAX_INPUT_STREAMS           1       // Number of capture streams.

// Wave pins
enum 
{
    KSPIN_WAVE_BRIDGE = 0,
    KSPIN_WAVEIN_HOST,
    KSPIN_WAVEIN_KEYWORD,
};

// Wave pins - FM
enum
{
    KSPIN_WAVE_FMRX = 0,
    KSPIN_WAVE_FMRX_BRIDGE
};

// Wave Topology nodes.
enum 
{
    KSNODE_WAVE_ADC = 0
};

// topology pins.
enum
{
    KSPIN_TOPO_MIC_ELEMENTS,
    KSPIN_TOPO_BRIDGE
};

// topology nodes.
enum
{
    KSNODE_TOPO_VOLUME,
    KSNODE_TOPO_MUTE,
    KSNODE_TOPO_PEAKMETER
};

// Topology pins - FM
enum
{
    KSPIN_TOPO_FMRX,
    KSPIN_TOPO_FMRX_BRIDGE
};

// data format attribute range definitions.
static
KSATTRIBUTE PinDataRangeSignalProcessingModeAttribute =
{
    sizeof(KSATTRIBUTE),
    0,
    STATICGUIDOF(KSATTRIBUTEID_AUDIOSIGNALPROCESSING_MODE),
};

static
PKSATTRIBUTE PinDataRangeAttributes[] =
{
    &PinDataRangeSignalProcessingModeAttribute,
};

static
KSATTRIBUTE_LIST PinDataRangeAttributeList =
{
    ARRAYSIZE(PinDataRangeAttributes),
    PinDataRangeAttributes,
};

#endif  // _SYSVAD_SIMPLE_H_

