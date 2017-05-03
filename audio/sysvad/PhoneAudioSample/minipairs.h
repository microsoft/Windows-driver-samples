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

#include "speakerhstopo.h"
#include "speakerhstoptable.h"
#include "speakerhswavtable.h"

#include "michstopo.h"
#include "michstoptable.h"
#include "michswavtable.h"

#include "micarraytopo.h"
#include "micarray1toptable.h"
#include "micarraywavtable.h"

#include "cellulartopo.h"
#include "cellularwave.h"
#include "cellulartoptable.h"
#include "cellularwavtable.h"

#include "fmtopo.h"
#include "fmwave.h"
#include "fmtoptable.h"
#include "fmwavtable.h"

#include "handsetspeakertopo.h"
#include "handsetspeakertoptable.h"
#include "handsetspeakerwavtable.h"

#include "handsetmictopo.h"
#include "handsetmictoptable.h"
#include "handsetmicwavtable.h"


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
        KSPIN_WAVE_RENDER_SOURCE,    // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpeakerMiniports =
{
    eSpeakerDevice,
    SPEAKER_TOPONAME,                       // defined in cellulartopo.h for use with cellular routing control
    CreateMiniportTopologySYSVAD,
    &SpeakerTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveSpeaker",                         // make sure this name matches with KSNAME_WaveSpeaker in the inf's [Strings] section
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
* Topology/Wave bridge connection for speaker (external:headset)   *
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
PHYSICALCONNECTIONTABLE SpeakerHsTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,    // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpeakerHsMiniports =
{
    eSpeakerHsDevice,
    SPEAKER_HEADSET_TOPONAME,               // defined in cellulartopo.h for use with cellular routing control
    CreateMiniportTopologySYSVAD,
    &SpeakerHsTopoMiniportFilterDescriptor, // use the speaker headset topo filter descriptor, not headset.
    0, NULL,                                // Interface properties
    L"WaveSpeakerHeadset",                  // make sure this name matches with KSNAME_WaveSpeakerHeadset in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &SpeakerHsWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    SPEAKERHS_DEVICE_MAX_CHANNELS,
    SpeakerHsPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerHsPinDeviceFormatsAndModes),
    SpeakerHsTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerHsTopologyPhysicalConnections),
    ENDPOINT_OFFLOAD_SUPPORTED,
    NULL, 0, NULL,                          // audio module settings.
};

//
// headset microphone miniports.
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
PHYSICALCONNECTIONTABLE MicHsTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicHsMiniports =
{
    eMicHsDevice,
    MIC_HEADSET_TOPONAME,                   // defined in cellulartopo.h for use with cellular routing control
    CreateMiniportTopologySYSVAD,
    &MicHsTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveMicHeadset",                      // make sure this name matches with KSNAME_WaveMicHeadset in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &MicHsWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    MICHS_DEVICE_MAX_CHANNELS,
    MicHsPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicHsPinDeviceFormatsAndModes),
    MicHsTopologyPhysicalConnections,
    SIZEOF_ARRAY(MicHsTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                          // audio module settings.
};


//
// Capture miniports.
//

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
    L"TopologyMicArray1",                   // make sure this name matches with KSNAME_TopologyMicArray1 in the inf's [Strings] section 
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveMicArray1",                       // make sure this name matches with KSNAME_WaveMicArray1 in the inf's [Strings] section
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
* Topology/Wave bridge connection for cellular endpoint              *
*                                                                    *
*                +------+                +------+                    *
*                | Wave1|                | Topo |                    *
*                |      |                |      |                    *
*                |      |                |      |                    *
*                |      |                |      |                    *
* System BiDi <--|0    1|<---------------|1    0|<-- BIDI            *
*                |      |                |      |                    *
*                +------+                |      |                    *
*                                        |      |                    *
*                +------+                |      |                    *
*                | Wave2|                |      |                    *
*                |      |                |      |                    *
*                |      |                |      |                    *
*                |      |                |      |                    *
* System BiDi <--|0    1|<---------------|3    2|<-- BIDI            *
*                |      |                |      |                    *
*                +------+                +------+                    *
*                                                                    *
*********************************************************************/

static
PHYSICALCONNECTIONTABLE CellularTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BIDI1_BRIDGE,   // TopologyOut
        KSPIN_WAVE_BIDI_BRIDGE,   // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR CellularMiniports =
{
    eCellularDevice,
    L"TopologyCellular",                    // make sure this name matches with KSNAME_TopologyCellular in the inf's [Strings] section 
    CreateCellularMiniportTopology,
    &CellularTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveCellular",                        // make sure this name matches with KSNAME_WaveCellular in the inf's [Strings] section
    CreateCellularMiniportWaveRT,
    &CellularWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    CELLULAR_DEVICE_MAX_CHANNELS,
    CellularPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(CellularPinDeviceFormatsAndModes),
    CellularTopologyPhysicalConnections,
    SIZEOF_ARRAY(CellularTopologyPhysicalConnections),
    ENDPOINT_CELLULAR_PROVIDER1,
    NULL, 0, NULL,                          // audio module settings.
};


static
PHYSICALCONNECTIONTABLE CellularTopologyPhysicalConnections2[] =
{
    {
        KSPIN_TOPO_BIDI2_BRIDGE,   // TopologyOut
        KSPIN_WAVE_BIDI_BRIDGE,   // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR CellularMiniports2 =
{
    eCellularDevice,
    L"TopologyCellular",                   // make sure this name matches with KSNAME_TopologyCellular in the inf's [Strings] section 
    CreateCellularMiniportTopology,
    &CellularTopoMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    L"WaveCellular2",                       // make sure this name matches with KSNAME_WaveCellular in the inf's [Strings] section
    CreateCellularMiniportWaveRT,
    &CellularWaveMiniportFilterDescriptor,
    0, NULL,                                // Interface properties
    CELLULAR_DEVICE_MAX_CHANNELS,
    CellularPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(CellularPinDeviceFormatsAndModes),
    CellularTopologyPhysicalConnections2,
    SIZEOF_ARRAY(CellularTopologyPhysicalConnections2),
    ENDPOINT_CELLULAR_PROVIDER2,
    NULL, 0, NULL,                          // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for handset speaker endpoint       *
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
PHYSICALCONNECTIONTABLE HandsetSpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER2_SOURCE,   // WaveOut (no offloading)
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR HandsetSpeakerMiniports =
{
    eHandsetSpeakerDevice,
    HANDSET_SPEAKER_TOPONAME,                       // defined in cellulartopo.h for use with cellular routing control
    CreateHandsetSpeakerMiniportTopology,
    &HandsetSpeakerTopoMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    L"WaveHandsetSpeaker",                          // make sure this name matches with KSNAME_WaveHandsetSpeaker in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &HandsetSpeakerWaveMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    HANDSETSPEAKER_DEVICE_MAX_CHANNELS,
    HandsetSpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(HandsetSpeakerPinDeviceFormatsAndModes),
    HandsetSpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(HandsetSpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                                  // audio module settings.
};

//
// handset microphone miniports.
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
PHYSICALCONNECTIONTABLE HandsetMicTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR HandsetMicMiniports =
{
    eHandsetMicDevice,
    HANDSET_MIC_TOPONAME,                           // defined in cellulartopo.h for use with cellular routing control
    CreateMiniportTopologySYSVAD,
    &HandsetMicTopoMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    L"WaveHandsetMic",                              // make sure this name matches with KSNAME_WaveHandsetMic in the inf's [Strings] section
    CreateMiniportWaveRTSYSVAD,
    &HandsetMicWaveMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    HANDSETMIC_DEVICE_MAX_CHANNELS,
    HandsetMicPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(HandsetMicPinDeviceFormatsAndModes),
    HandsetMicTopologyPhysicalConnections,
    SIZEOF_ARRAY(HandsetMicTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                                  // audio module settings.
};

/*********************************************************************
* Topology/Wave bridge connection for FM RX                          *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  FM RX   --->|0    1|===>|1    0|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE FmRxTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_FMRX_BRIDGE,          // TopologyOut
        KSPIN_WAVE_FMRX_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR FmRxMiniports =
{
    eFmRxDevice,
    L"TopologyFmRx",                                // make sure this name matches with KSNAME_TopologyFmRx
    CreateFmMiniportTopology,
    &FmTopoMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    L"WaveFmRx",                                    // make sure this name matches with KSNAME_WaveFmRx in the inf's [Strings] section
    CreateFmMiniportWaveRT,
    &FmWaveMiniportFilterDescriptor,
    0, NULL,                                        // Interface properties
    FMRX_DEVICE_MAX_CHANNELS,
    FmRxPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(FmRxPinDeviceFormatsAndModes),
    FmRxTopologyPhysicalConnections,
    SIZEOF_ARRAY(FmRxTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    NULL, 0, NULL,                                  // audio module settings.
};

//=============================================================================
//
// Render miniport pairs. NOTE: the split of render and capture is arbitrary and
// unnessary, this array could contain capture endpoints.
//
static
PENDPOINT_MINIPAIR  g_RenderEndpoints[] = 
{
    &SpeakerMiniports,
    &SpeakerHsMiniports,
    &HandsetSpeakerMiniports,
    &CellularMiniports, // SIM 1
    &CellularMiniports2 // SIM 2
};

#define g_cRenderEndpoints  (SIZEOF_ARRAY(g_RenderEndpoints))

//=============================================================================
//
// Capture miniport pairs. NOTE: the split of render and capture is arbitrary and
// unnessary, this array could contain render endpoints.
//
static
PENDPOINT_MINIPAIR  g_CaptureEndpoints[] = 
{
    &MicHsMiniports,
    &MicArray1Miniports,
    &HandsetMicMiniports,
    &FmRxMiniports
};

#define g_cCaptureEndpoints (SIZEOF_ARRAY(g_CaptureEndpoints))

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave).
//
#define g_MaxMiniports  ((g_cRenderEndpoints + g_cCaptureEndpoints) * 2)

#endif // _SYSVAD_MINIPAIRS_H_

