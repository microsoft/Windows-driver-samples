/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpminipairs.h

Abstract:

    Bluetooth HFP audio endpoint filter definitions. 

--*/

#ifndef _SYSVAD_BTHHFPMINIPAIRS_H_
#define _SYSVAD_BTHHFPMINIPAIRS_H_

#include "bthhfpspeakertopo.h"
#include "bthhfpspeakertoptable.h"
#include "bthhfpspeakerwavtable.h"
#include "bthhfpspeakerwbwavtable.h"

#include "bthhfpmictopo.h"
#include "bthhfpmictoptable.h"
#include "bthhfpmicwavtable.h"
#include "bthhfpmicwbwavtable.h"

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

//
// Render miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for BTH-HFP speaker                *
*                                                                    *
*              +------+                +------+                      *
*              | Wave |                | Topo |                      *
*              |      |                |      |                      *
* System   --->|0    2|---> Loopback   |      |                      *
*              |      |                |      |                      *
* Offload  --->|1    3|--------------->|0    1|---> Line Out         *
*              |      |                |      |                      *
*              +------+                +------+                      *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE BthHfpSpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,      // TopologyIn
        KSPIN_WAVE_RENDER3_SOURCE,      // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

// A subdevice is created for each HFP device using the reference string in the form of:
// <Base_Reference_String>-<Hashcode>
// where Base_Reference_String is one of the following template names
#define BTHHFP_SPEAKER_TOPO_NAME    L"TopologyBthHfpSpeaker"
#define BTHHFP_SPEAKER_WAVE_NAME    L"WaveBthHfpSpeaker"
#define BTHHFP_MIC_TOPO_NAME        L"TopologyBthHfpMic"
#define BTHHFP_MIC_WAVE_NAME        L"WaveBthHfpMic"
// And Hashcode is 8-character long hexadecimal value

// A reference string max length is being set to a value to accommodate them both
#define BTHHFP_INTERFACE_REFSTRING_MAX_LENGTH 100

static
ENDPOINT_MINIPAIR BthHfpSpeakerMiniports =
{
    eBthHfpSpeakerDevice,
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_TOPO_NAME and made unique for each side band interface
    BTHHFP_SPEAKER_TOPO_NAME,           // template name matches with KSNAME_TopologyBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &BthHfpSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    BTHHFP_SPEAKER_WAVE_NAME,           // template name matches with KSNAME_WaveBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpSpeakerWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    1,
    BthHfpSpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpSpeakerPinDeviceFormatsAndModes),
    BthHfpSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpSpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

static
ENDPOINT_MINIPAIR BthHfpSpeakerWBMiniports =
{
    eBthHfpSpeakerDevice,
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_TOPO_NAME and made unique for each side band interface
    BTHHFP_SPEAKER_TOPO_NAME,           // Template name that matches with KSNAME_TopologyBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &BthHfpSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    BTHHFP_SPEAKER_WAVE_NAME,           // Template name matches with KSNAME_WaveBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpSpeakerWBWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    1,
    BthHfpSpeakerWBPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(BthHfpSpeakerWBPinDeviceFormatsAndModes),
    BthHfpSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(BthHfpSpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

//
// Capture miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for BTH-HFP mic                    *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE BthHfpMicTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,              // TopologyOut
        KSPIN_WAVE_BRIDGE,              // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR BthHfpMicMiniports =
{
    eBthHfpMicDevice,
    NULL,                               // Will be generated based on BTHHFP_MIC_TOPO_NAME and made unique for each side band interface
    BTHHFP_MIC_TOPO_NAME,               // template name matches with KSNAME_TopologyBthHfpMic in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &BthHfpMicTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    BTHHFP_MIC_WAVE_NAME,               // template name matches with KSNAME_WaveBthHfpMic in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpMicWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
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
    NULL,                               // Will be generated based on BTHHFP_MIC_TOPO_NAME and made unique for each side band interface
    BTHHFP_MIC_TOPO_NAME,               // template name matches with KSNAME_TopologyBthHfpMic in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &BthHfpMicTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on BTHHFP_SPEAKER_WAVE_NAME and made unique for each side band interface
    BTHHFP_MIC_WAVE_NAME,               // template name matches with KSNAME_WaveBthHfpMic in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &BthHfpMicWBWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
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
// 
// Allow 10 Bluetooth hands-free profile devices.
//
#define MAX_BTHHFP_DEVICES      (10)
#define g_MaxBthHfpMiniports    ((g_cBthHfpRenderEndpoints + g_cBthHfpCaptureEndpoints) * MAX_BTHHFP_DEVICES)

// g_MaxBthHfpMiniports is used when calculating the MaxObjects inside AddDevice.
C_ASSERT(g_MaxBthHfpMiniports == 40);

#endif // _SYSVAD_BTHHFPMINIPAIRS_H_

