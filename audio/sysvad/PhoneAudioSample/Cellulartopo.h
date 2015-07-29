
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellulartopo.h

Abstract:

    Declaration of topology miniport for the cellular endpoint.

--*/

#ifndef _SYSVAD_CELLULARTOPO_H_
#define _SYSVAD_CELLULARTOPO_H_

#include "basetopo.h"

// make sure all of these names matches with KSNAME_* in the inf's [Strings] section
#define SPEAKER_TOPONAME L"TopologySpeaker"
#define SPEAKER_HEADSET_TOPONAME L"TopologySpeakerHeadset"
#define MIC_HEADSET_TOPONAME L"TopologyMicHeadset"
#define MIC_ARRAY1_TOPONAME L"TopologyMicArray1"
#define HANDSET_SPEAKER_TOPONAME L"TopologyHandsetSpeaker"
#define HANDSET_MIC_TOPONAME L"TopologyHandsetMic"

// from bthhfpminipairs.h, static name of bluetooth speaker and mic topologies
#define HFP_SPEAKER_TOPONAME L"TopologyBthHfpSpeaker"
#define HFP_MIC_TOPONAME L"TopologyBthHfpMic"

// Special routing endpoints
#define HOSTRENDER_TOPONAME L"HostRender"
#define HOSTRENDER_PIN 0
#define HOSTCAPTURE_TOPONAME L"HostCapture"
#define HOSTCAPTURE_PIN 0

// Telephony Volume Stepping information
#define TELEPHONY_VOLUME_MAXIMUM 0                  //   0 dB
#define TELEPHONY_VOLUME_MINIMUM (-30 * 0x10000)    // -30 dB
#define TELEPHONY_VOLUME_STEPPING (3 * 0x10000)     //  10 steps


//=============================================================================
// Defines
//=============================================================================
// {3829F9C3-F341-4B82-976F-A635EB3CEA0D}
DEFINE_GUID(IID_ICellularTopology, 
0x3829f9c3, 0xf341, 0x4b82, 0x97, 0x6f, 0xa6, 0x35, 0xeb, 0x3c, 0xea, 0xd);


//=============================================================================
// Interfaces
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// ICellularTopology
//
DECLARE_INTERFACE_(ICellularTopology, IMiniportTopology)
{
    STDMETHOD_(NTSTATUS, UpdateTopologyJackState)
    (
        THIS_
        _In_ ULONG TelephonyId,
        _In_ BOOL NewState
    ) PURE;
};

typedef ICellularTopology *PCELLULARTOPOLOGY;

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CCellularMiniportTopology 
//   

class CCellularMiniportTopology : 
    public CMiniportTopologySYSVAD,
    public ICellularTopology,
    public CUnknown
{
  private:
    KSTOPOLOGY_ENDPOINTID m_CellularRenderEndpoint;
    KSTOPOLOGY_ENDPOINTID m_CellularCaptureEndpoint;
    ULONG m_CellularVolume;

  public:
    DECLARE_STD_UNKNOWN();
    CCellularMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels
    );

    ~CCellularMiniportTopology();

    IMP_IMiniportTopology;

    NTSTATUS PropertyHandlerJackSinkInfo
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );
    
    NTSTATUS PropertyHandlerJackDescription
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS PropertyHandlerJackDescription2
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS PropertyHandlerTelephonyEndpointIdPair
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS PropertyHandlerTelephonyVolume
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS EventHandler_JackState
    (
        _In_ PPCEVENT_REQUEST _pEventRequest
    );

    STDMETHODIMP_(NTSTATUS) UpdateTopologyJackState
    (
        _In_ ULONG TelephonyId,
        _In_ BOOL NewState
    );
};
    
typedef CCellularMiniportTopology *PCCellularMiniportTopology;

NTSTATUS
CreateCellularMiniportTopology( 
    _Out_           PUNKNOWN                              * Unknown,
    _In_            REFCLSID,
    _In_opt_        PUNKNOWN                                UnknownOuter,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    _In_            POOL_TYPE                               PoolType,
    _In_            PUNKNOWN                                UnknownAdapter,
    _In_opt_        PVOID                                   DeviceContext,
    _In_            PENDPOINT_MINIPAIR                      MiniportPair
    );

NTSTATUS PropertyHandler_CellularTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_CELLULARTOPO_H_

