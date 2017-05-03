/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minipairs.h

Abstract:

    Local audio endpoint filter definitions. 

--*/

#ifndef _SYSVAD_MINIPAIRS_H_
#define _SYSVAD_MINIPAIRS_H_

#include "speakertopo.h"
#include "speakertoptable.h"
#include "speakerwavtable.h"

#include "speakerhptopo.h"
#include "speakerhptoptable.h"
#include "speakerhpwavtable.h"

#include "hdmitopo.h"
#include "hdmitoptable.h"
#include "hdmiwavtable.h"

#include "spdiftopo.h"
#include "spdiftoptable.h"
#include "spdifwavtable.h"

#include "micintopo.h"
#include "micintoptable.h"
#include "micinwavtable.h"

#include "micarraytopo.h"
#include "micarray1toptable.h"
#include "micarray2toptable.h"
#include "micarray3toptable.h"
#include "micarraywavtable.h"
#include "micarray3wavtable.h"

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

//
// Describe buffer size constraints for WaveRT buffers
//
static struct
{
    KSAUDIO_PACKETSIZE_CONSTRAINTS2 TransportPacketConstraints;
    KSAUDIO_PACKETSIZE_PROCESSINGMODE_CONSTRAINT AdditionalProcessingConstraints[1];
} SysvadWaveRtPacketSizeConstraintsRender =
{
    {
        2 * HNSTIME_PER_MILLISECOND,                // 2 ms minimum processing interval
        FILE_BYTE_ALIGNMENT,                        // 1 byte packet size alignment
        0,                                          // no maximum packet size constraint
        2,                                          // 2 processing constraints follow
        {
            STATIC_AUDIO_SIGNALPROCESSINGMODE_DEFAULT,          // constraint for default processing mode
            128,                                                // 128 samples per processing frame
            0,                                                  // NA hns per processing frame
        },
    },
    {
        {
            STATIC_AUDIO_SIGNALPROCESSINGMODE_MOVIE,            // constraint for movie processing mode
            1024,                                               // 1024 samples per processing frame
            0,                                                  // NA hns per processing frame
        },
    }
};

const SYSVAD_DEVPROPERTY SysvadWaveFilterInterfacePropertiesRender[] =
{
    {
        &DEVPKEY_KsAudio_PacketSize_Constraints2,           // Key
        DEVPROP_TYPE_BINARY,                                // Type
        sizeof(SysvadWaveRtPacketSizeConstraintsRender),    // BufferSize
        &SysvadWaveRtPacketSizeConstraintsRender,           // Buffer
    },
};

static struct
{
    KSAUDIO_PACKETSIZE_CONSTRAINTS2 TransportPacketConstraints;
} SysvadWaveRtPacketSizeConstraintsCapture =
{
    {
        2 * HNSTIME_PER_MILLISECOND,                            // 2 ms minimum processing interval
        FILE_BYTE_ALIGNMENT,                                    // 1 byte packet size alignment
        0x100000,                                               // 1 MB maximum packet size
        1,                                                      // 1 processing constraint follows
        {
            STATIC_AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,   // constraint for communications processing mode
            0,                                                  // NA samples per processing frame
            20 * HNSTIME_PER_MILLISECOND,                       // 200000 hns (20ms) per processing frame
        },
    },
};

const SYSVAD_DEVPROPERTY SysvadWaveFilterInterfacePropertiesCapture[] =
{
    {
        &DEVPKEY_KsAudio_PacketSize_Constraints2,           // Key
        DEVPROP_TYPE_BINARY,                                // Type
        sizeof(SysvadWaveRtPacketSizeConstraintsCapture),   // BufferSize
        &SysvadWaveRtPacketSizeConstraintsCapture,          // Buffer
    },
};

//
// Render miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for speaker (internal)             *
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
PHYSICALCONNECTIONTABLE SpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpeakerMiniports =
{
    eSpeakerDevice,
    L"TopologySpeaker",                                     // make sure this name matches with KSNAME_TopologySpeaker in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &SpeakerTopoMiniportFilterDescriptor,
    0, NULL,                                                // Interface properties
    L"WaveSpeaker",                                         // make sure this name matches with KSNAME_WaveSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &SpeakerWaveMiniportFilterDescriptor,
    ARRAYSIZE(SysvadWaveFilterInterfacePropertiesRender),   // Interface properties
    SysvadWaveFilterInterfacePropertiesRender,
    SPEAKER_DEVICE_MAX_CHANNELS,
    SpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),
    SpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED,
    SpeakerModulesWaveFilter, 
    SIZEOF_ARRAY(SpeakerModulesWaveFilter),
    &SpeakerModuleNotificationDeviceId,
};

/*********************************************************************
* Topology/Wave bridge connection for speaker (external:headphone)   *
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
PHYSICALCONNECTIONTABLE SpeakerHpTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpeakerHpMiniports =
{
    eSpeakerHpDevice,
    L"TopologySpeakerHeadphone",            // make sure this name matches with KSNAME_TopologySpeakerHeadphone in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &SpeakerHpTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveSpeakerHeadphone",                // make sure this name matches with KSNAME_WaveSpeakerHeadphone in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &SpeakerHpWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    SPEAKERHP_DEVICE_MAX_CHANNELS,
    SpeakerHpPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerHpPinDeviceFormatsAndModes),
    SpeakerHpTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerHpTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED,
    SpeakerHpModulesWaveFilter, 
    SIZEOF_ARRAY(SpeakerHpModulesWaveFilter),
    &SpeakerHpModuleNotificationDeviceId,
};

/*********************************************************************
* Topology/Wave bridge connection for hdmi endpoint                  *
*                                                                    *
*              +------+                +------+                      *
*              | Wave |                | Topo |                      *
*              |      |                |      |                      *
* System   --->|0    1|---> Loopback   |      |                      *
*              |      |                |      |                      *
*              |     2|--------------->|0    1|---> Line Out         *
*              |      |                |      |                      *
*              +------+                +------+                      *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE HdmiTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER2_SOURCE,  // WaveOut (no offloading)
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR HdmiMiniports =
{
    eHdmiRenderDevice,
    L"TopologyHdmi",                        // make sure this name matches with KSNAME_TopologyHdmi in the inf's [Strings] section 
    CreateHdmiMiniportTopology,
    &HdmiTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveHdmi",                            // make sure this name matches with KSNAME_WaveHdmi in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &HdmiWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    HDMI_DEVICE_MAX_CHANNELS,
    HdmiPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(HdmiPinDeviceFormatsAndModes),
    HdmiTopologyPhysicalConnections,
    SIZEOF_ARRAY(HdmiTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                          // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for spdif (internal)               *
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
PHYSICALCONNECTIONTABLE SpdifTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpdifMiniports =
{
    eSpdifRenderDevice,
    L"TopologySpdif",                                       // make sure this name matches with KSNAME_TopologySpeaker in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &SpdifTopoMiniportFilterDescriptor,
    0, NULL,                                                // Interface properties
    L"WaveSpdif",                                           // make sure this name matches with KSNAME_WaveSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &SpdifWaveMiniportFilterDescriptor,
    ARRAYSIZE(SysvadWaveFilterInterfacePropertiesRender),   // Interface properties
    SysvadWaveFilterInterfacePropertiesRender,
    SPDIF_DEVICE_MAX_CHANNELS,
    SpdifPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpdifPinDeviceFormatsAndModes),
    SpdifTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpdifTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED,
    NULL, 0, NULL,                                          // audio module settings.
};

//
// Capture miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for mic in                         *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicInTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicInMiniports =
{
    eMicInDevice,
    L"TopologyMicIn",                       // make sure this name matches with KSNAME_TopologyMicIn in the inf's [Strings] section 
    CreateMiniportTopologySYSVAD,
    &MicInTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveMicIn",                           // make sure this name matches with KSNAME_WaveMicIn in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &MicInWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    MICIN_DEVICE_MAX_CHANNELS,
    MicInPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicInPinDeviceFormatsAndModes),
    MicInTopologyPhysicalConnections,
    SIZEOF_ARRAY(MicInTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                          // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for mic array  1 (front)           *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicArray1TopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicArray1Miniports =
{
    eMicArrayDevice1,
    L"TopologyMicArray1",                                   // make sure this name matches with KSNAME_TopologyMicArray1 in the inf's [Strings] section 
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor,
    0, NULL,                                                // Interface properties
    L"WaveMicArray1",                                       // make sure this name matches with KSNAME_WaveMicArray1 in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &MicArrayWaveMiniportFilterDescriptor,
    ARRAYSIZE(SysvadWaveFilterInterfacePropertiesCapture),  // Interface properties
    SysvadWaveFilterInterfacePropertiesCapture,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_SOUNDDETECTOR_SUPPORTED,
    NULL, 0, NULL,                          // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for mic array  2 (back)            *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicArray2TopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicArray2Miniports =
{
    eMicArrayDevice2,
    L"TopologyMicArray2",                   // make sure this name matches with KSNAME_TopologyMicArray2 in the inf's [Strings] section 
    CreateMicArrayMiniportTopology,
    &MicArray2TopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveMicArray2",                       // make sure this name matches with KSNAME_WaveMicArray2 in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &MicArrayWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray2TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray2TopologyPhysicalConnections),
    ENDPOINT_SOUNDDETECTOR_SUPPORTED,
    NULL, 0, NULL,                          // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for mic array  3 (combined)        *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicArray3TopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicArray3Miniports =
{
    eMicArrayDevice3,
    L"TopologyMicArray3",                   // make sure this name matches with KSNAME_TopologyMicArray3 in the inf's [Strings] section 
    CreateMicArrayMiniportTopology,
    &MicArray3TopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveMicArray3",                       // make sure this name matches with KSNAME_WaveMicArray3 in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &MicArray3WaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    MICARRAY3_DEVICE_MAX_CHANNELS,
    MicArray3PinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArray3PinDeviceFormatsAndModes),
    MicArray3TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray3TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                          // audio module settings.
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_RenderEndpoints[] = 
{
    &SpeakerMiniports,
    &SpeakerHpMiniports,
    &HdmiMiniports,
    &SpdifMiniports,
};

#define g_cRenderEndpoints  (SIZEOF_ARRAY(g_RenderEndpoints))

//=============================================================================
//
// Capture miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_CaptureEndpoints[] = 
{
    &MicInMiniports,
    &MicArray1Miniports,
    &MicArray2Miniports,
    &MicArray3Miniports,
};

#define g_cCaptureEndpoints (SIZEOF_ARRAY(g_CaptureEndpoints))

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave).
//
#define g_MaxMiniports  ((g_cRenderEndpoints + g_cCaptureEndpoints) * 2)

#endif // _SYSVAD_MINIPAIRS_H_

