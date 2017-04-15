/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbheadsetminipairs.h

Abstract:

    USB Sideband Audio endpoint filter definitions. 

--*/

#ifndef _SYSVAD_USBHEADSETMINIPAIRS_H_
#define _SYSVAD_USBHEADSETMINIPAIRS_H_

#include "usbheadsettopo.h"
#include "usbheadsettoptable.h"
#include "usbheadsetwavtable.h"

NTSTATUS
CreateMiniportWaveRTSYSVAD
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_TYPE,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

NTSTATUS
CreateMiniportTopologySYSVAD
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_TYPE,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

/*
* Headset miniports.
*
************************************************************************************************************************************************
* Topology/Wave bridge connection for USB Headset                                                                                              *
*                                                                                                                                              *
*              +---------------------------------+                +---------------------------------------------------------+                  *
*              | Wave                            |                | Topo                                                    |                  *
*              |                                 |                |                                                         |                  *
*              |      +---------------------+    |                |                                                         |                  *
*              |      |                     |    |                |                                                         |                  *
* System   --->|0-----|1  HW Audio Engine  3|---2|---> Loopback   |                                                         |                  *
*              |      |        Node         |    |                |                                                         |                  *
* Offload  --->|1-----|2                   0|---3|===============>|0------------------------------------------------------>1|---> Line Out     *
*              |      +---------------------+    |                |                                                         |                  *
*              |                                 |                |                                                         |                  *
*              |         +-------------+         |                |      +-------------+     +-------+     +--------+       |                  *
* Capture  <---|5<-------|0    ADC    1|<-------4|<===============|3<----| peak meter  |<----|  mute |<----| volume |<---- 2|<---- Mic in      *
*              |         +-------------+         |                |      +-------------+     +-------+     +--------+       |                  *
*              +---------------------------------+                +---------------------------------------------------------+                  *
*                                                                                                                                              *
*                                                                                                                                              *
************************************************************************************************************************************************/

static
PHYSICALCONNECTIONTABLE UsbHeadsetTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_USBHS_WAVEOUT_SOURCE,    // TopologyIn
        KSPIN_WAVE_RENDER_USBHS_SOURCE,     // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    },
    {
        KSPIN_TOPO_USBHS_BRIDGE,            // TopologyOut
        KSPIN_WAVE_CAPTURE_USBHS_BRIDGE,    // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR UsbHsSpeakerMiniports =
{
    eBthHfpSpeakerDevice,
    L"TopologyBthHfpSpeaker",               // make sure this name matches with SYSVAD.TopologyBthHfpSpeaker.szPname in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &BthHfpSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveBthHfpSpeaker",                   // make sure this name matches with SYSVAD.WaveBthHfpSpeaker.szPname in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpSpeakerWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    1,
    BthHfpSpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpSpeakerPinDeviceFormatsAndModes),
    BthHfpSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpSpeakerTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED
};

static
ENDPOINT_MINIPAIR BthHfpSpeakerWBMiniports =
{
    eBthHfpSpeakerDevice,
    L"TopologyBthHfpSpeaker",               // make sure this name matches with SYSVAD.TopologyBthHfpSpeaker.szPname in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &BthHfpSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveBthHfpSpeaker",                   // make sure this name matches with SYSVAD.WaveBthHfpSpeaker.szPname in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpSpeakerWBWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    1,
    BthHfpSpeakerWBPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpSpeakerWBPinDeviceFormatsAndModes),
    BthHfpSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpSpeakerTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED
};


static
ENDPOINT_MINIPAIR BthHfpMicMiniports =
{
    eBthHfpMicDevice,
    L"TopologyBthHfpMic",                   // make sure this name matches with SYSVAD.TopologyBthHfpMic.szPname in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &BthHfpMicTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveBthHfpMic",                       // make sure this name matches with SYSVAD.WaveBthHfpMic.szPname in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpMicWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    1,
    BthHfpMicPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpMicPinDeviceFormatsAndModes),
    BthHfpMicTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpMicTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

static
ENDPOINT_MINIPAIR BthHfpMicWBMiniports =
{
    eBthHfpMicDevice,
    L"TopologyBthHfpMic",                   // make sure this name matches with SYSVAD.TopologyBthHfpMic.szPname in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &BthHfpMicTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveBthHfpMic",                       // make sure this name matches with SYSVAD.WaveBthHfpMic.szPname in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpMicWBWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    1,
    BthHfpMicWBPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpMicWBPinDeviceFormatsAndModes),
    BthHfpMicTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpMicTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_BthHfpRenderEndpoints[] = 
{
    &BthHfpSpeakerMiniports,
    &BthHfpSpeakerWBMiniports
};

#define g_cBthHfpRenderEndpoints  (SIZEOF_ARRAY(g_BthHfpRenderEndpoints))

// If the count of Render endpoints changes, update formula below
C_ASSERT(g_cBthHfpRenderEndpoints == 2);

//=============================================================================
//
// Capture miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_BthHfpCaptureEndpoints[] = 
{
    &BthHfpMicMiniports,
    &BthHfpMicWBMiniports
};

#define g_cBthHfpCaptureEndpoints (SIZEOF_ARRAY(g_BthHfpCaptureEndpoints))

// If the count of Capture endpoints changes, update formula below
C_ASSERT(g_cBthHfpCaptureEndpoints == 2);

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave) / 2 (since we have one each for narrowband-only and wideband)
//
#define g_MaxBthHfpMiniports  ((g_cBthHfpRenderEndpoints + g_cBthHfpCaptureEndpoints))

// g_MaxBthHfpMiniports is used when calculating the MaxObjects inside AddDevice.
C_ASSERT(g_MaxBthHfpMiniports == 4);

#endif // _SYSVAD_USBHEADSETMINIPAIRS_H_

