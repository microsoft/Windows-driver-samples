/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    a2dphpminipairs.h

Abstract:

    A2DP Sideband Audio endpoint filter definitions. 

--*/

#ifndef _SYSVAD_A2DPHPMINIPAIRS_H_
#define _SYSVAD_A2DPHPMINIPAIRS_H_

#include "a2dphpspeakertopo.h"
#include "a2dphpspeakertoptable.h"
#include "a2dphpspeakerwavtable.h"

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
* A2DP Headphone speaker miniports.
*
**********************************************************************************************
* Topology/Wave bridge connection for A2DP Headphone Speaker                                    *
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
PHYSICALCONNECTIONTABLE A2dpHpSpeakerTopologyPhysicalConnections[] =
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
#define A2DPHP_SPEAKER_TOPO_NAME    L"TopologyA2dpHpSpeaker"
#define A2DPHP_SPEAKER_WAVE_NAME    L"WaveA2dpHpSpeaker"
// And Hashcode is 8-character long hexadecimal value

// A reference string max length is being set to a value to accommodate them both
#define A2DPHP_INTERFACE_REFSTRING_MAX_LENGTH 100

static
ENDPOINT_MINIPAIR A2dpHpSpeakerMiniports =
{
    eA2dpHpSpeakerDevice,
    NULL,                               // Will be generated based on A2DPHP_SPEAKER_TOPO_NAME and made unique for each side band interface
    A2DPHP_SPEAKER_TOPO_NAME,            // template name matches with KSNAME_TopologyBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportTopologySYSVAD,
    &A2dpHpSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    NULL,                               // Will be generated based on A2DPHP_SPEAKER_WAVE_NAME and made unique for each side band interface
    A2DPHP_SPEAKER_WAVE_NAME,            // template name matches with KSNAME_WaveBthHfpSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &A2dpHpSpeakerWaveMiniportFilterDescriptor,
    0, NULL,                            // Interface properties
    1,
    A2dpHpSpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(A2dpHpSpeakerPinDeviceFormatsAndModes),
    A2dpHpSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(A2dpHpSpeakerTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_A2dpHpRenderEndpoints[] = 
{
    &A2dpHpSpeakerMiniports
};

#define g_cA2dpHpRenderEndpoints  (SIZEOF_ARRAY(g_A2dpHpRenderEndpoints))

// If the count of Render endpoints changes, update formula below
C_ASSERT(g_cA2dpHpRenderEndpoints == 1);

//=============================================================================
//
// Total miniports = # endpoints (topology + wave)
//
// 
// Allow 5 A2DP Headphone Endpoints.
//
#define MAX_A2DPHS_DEVICES      (5)
#define g_MaxA2dpHpMiniports    ((g_cA2dpHpRenderEndpoints) * 2 * MAX_A2DPHS_DEVICES)

// g_MaxA2dpHpMiniports is used when calculating the MaxObjects inside AddDevice.
C_ASSERT(g_MaxA2dpHpMiniports == 10);

#endif // _SYSVAD_A2DPHPMINIPAIRS_H_

