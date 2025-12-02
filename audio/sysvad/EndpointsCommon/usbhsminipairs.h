/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsminipairs.h

Abstract:

    USB Sideband Audio endpoint filter definitions. 

--*/

#ifndef _SYSVAD_USBHSMINIPAIRS_H_
#define _SYSVAD_USBHSMINIPAIRS_H_

#include "usbhsspeakertopo.h"
#include "usbhsspeakertoptable.h"
#include "usbhsspeakerwavtable.h"

#include "usbhsmictopo.h"
#include "usbhsmictoptable.h"
#include "usbhsmicwavtable.h"

NTSTATUS
CreateMiniportWaveRTSYSVAD
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_FLAGS,
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
    _In_        POOL_FLAGS,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

/*
* USB Headset speaker miniports.
*
**********************************************************************************************
* Topology/Wave bridge connection for USB Headset Speaker                                    *
*                                                                                            *
*              +---------------------------------+                +-------+                  *
*              | Wave                            |                | Topo  |                  *
*              |                                 |                |       |                  *
*              |      +---------------------+    |                |       |                  *
*              |      |                     |    |                |       |                  *
* System   --->|0-----|1  HW Audio Engine  3|---2|---> Loopback   |       |                  *
*              |      |        Node         |    |                |       |                  *
* Offload  --->|1-----|2                   0|---3|===============>|0---->1|---> Line Out     *
*              |      +---------------------+    |                |       |                  *
*              |                                 |                |       |                  *
*              +---------------------------------+                +-------+                  *
*                                                                                            *
**********************************************************************************************/

static
PHYSICALCONNECTIONTABLE UsbHsSpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

// A subdevice is created for each HFP device using the reference string in the form of:
// <Base_Reference_String>-<Hashcode>
// where Base_Reference_String is one of the following template names
#define USBHS_SPEAKER_TOPO_NAME    L"TopologyUsbHsSpeaker"
#define USBHS_SPEAKER_WAVE_NAME    L"WaveUsbHsSpeaker"
#define USBHS_MIC_TOPO_NAME        L"TopologyUsbHsMic"
#define USBHS_MIC_WAVE_NAME        L"WaveUsbHsMic"
// And Hashcode is 8-character long hexadecimal value

// A reference string max length is being set to a value to accommodate them both
#define USBHS_INTERFACE_REFSTRING_MAX_LENGTH 100

static
ENDPOINT_MINIPAIR UsbHsSpeakerMiniports =
{
    eUsbHsSpeakerDevice,
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_TOPO_NAME and made unique for each side band interface
    USBHS_SPEAKER_TOPO_NAME,            // template name matches with KSNAME_TopologyBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &UsbHsSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    USBHS_SPEAKER_WAVE_NAME,            // template name matches with KSNAME_WaveBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &UsbHsSpeakerWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    1,
    UsbHsSpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(UsbHsSpeakerPinDeviceFormatsAndModes),
    UsbHsSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(UsbHsSpeakerTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED
};


/*
* Headset Microphone miniports.
*
*
************************************************************************************************************************************************
* Topology/Wave bridge connection for USB Headset Microphone                                                                                   *
*              +---------------------------------+                +---------------------------------------------------------+                  *
*              |         +-------------+         |                |      +-------------+     +-------+     +--------+       |                  *
* Capture  <---|1<-------|0    ADC    1|<-------0|<===============|1<----| peak meter  |<----|  mute |<----| volume |<---- 0|<---- Mic in      *
*              |         +-------------+         |                |      +-------------+     +-------+     +--------+       |                  *
*              +---------------------------------+                +---------------------------------------------------------+                  *
*                                                                                                                                              *
************************************************************************************************************************************************/
static
PHYSICALCONNECTIONTABLE UsbHsMicTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR UsbHsMicMiniports =
{
    eUsbHsMicDevice,
    NULL,                               // Will be generated based on BTHHFP_MIC_TOPO_NAME and made unique for each side band interface
    USBHS_MIC_TOPO_NAME,                // template name matches with KSNAME_TopologyBthHfpMic in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &UsbHsMicTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    USBHS_MIC_WAVE_NAME,                // template name matches with KSNAME_WaveBthHfpMic in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &UsbHsMicWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    1,
    UsbHsMicPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(UsbHsMicPinDeviceFormatsAndModes),
    UsbHsMicTopologyPhysicalConnections,
    SIZEOF_ARRAY(UsbHsMicTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_UsbHsRenderEndpoints[] = 
{
    &UsbHsSpeakerMiniports
};

#define g_cUsbHsRenderEndpoints  (SIZEOF_ARRAY(g_UsbHsRenderEndpoints))

// If the count of Render endpoints changes, update formula below
C_ASSERT(g_cUsbHsRenderEndpoints == 1);

//=============================================================================
//
// Capture miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_UsbHsCaptureEndpoints[] = 
{
    &UsbHsMicMiniports
};

#define g_cUsbHsCaptureEndpoints (SIZEOF_ARRAY(g_UsbHsCaptureEndpoints))

// If the count of Capture endpoints changes, update formula below
C_ASSERT(g_cUsbHsCaptureEndpoints == 1);

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave)
//
// 
// Allow 5 USB Headset Endpoints.
//
#define MAX_USBHS_DEVICES       (5)
#define g_MaxUsbHsMiniports     ((g_cUsbHsRenderEndpoints + g_cUsbHsCaptureEndpoints) * 2 * MAX_USBHS_DEVICES)

// g_MaxUsbHsMiniports is used when calculating the MaxObjects inside AddDevice.
C_ASSERT(g_MaxUsbHsMiniports == 20);

#endif // _SYSVAD_USBHSMINIPAIRS_H_

