
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    FMtopo.h

Abstract:

    Declaration of topology miniport for the fm endpoint.

--*/

#ifndef _SYSVAD_FMTOPO_H_
#define _SYSVAD_FMTOPO_H_

#include "basetopo.h"

// make sure all of these names matches with KSNAME_* in the inf's [Strings] section
#define SPEAKER_TOPONAME L"TopologySpeaker"
#define SPEAKER_HEADSET_TOPONAME L"TopologySpeakerHeadset"

// Special routing endpoint
#define HOSTRENDER_TOPONAME L"HostRender"
#define HOSTRENDER_PIN 0

// FM Volume Stepping information
#define FMRX_VOLUME_MAXIMUM 0                  //   0 dB
#define FMRX_VOLUME_MINIMUM (-96 * 0x10000)    // -96 dB
#define FMRX_VOLUME_STEPPING (0x8000)          // 0.5 dB steps


//=============================================================================
// Defines
//=============================================================================
// {949F42F1-A5F5-4FBD-B4BB-C543ACC24AE0}
DEFINE_GUID(IID_IFmTopology,
    0x949f42f1, 0xa5f5, 0x4fbd, 0xb4, 0xbb, 0xc5, 0x43, 0xac, 0xc2, 0x4a, 0xe0);


//=============================================================================
// Interfaces
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// IFmTopology
//
DECLARE_INTERFACE_(IFmTopology, IMiniportTopology)
{
    STDMETHOD_(NTSTATUS, UpdateTopologyJackState)
    (
        THIS_
        _In_ BOOL NewState
    ) PURE;
};

typedef IFmTopology *PFMTOPOLOGY;

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CFmMiniportTopology 
//   

class CFmMiniportTopology : 
    public CMiniportTopologySYSVAD,
    public IFmTopology,
    public CUnknown
{
  private:
    KSTOPOLOGY_ENDPOINTID m_FmRxTargetEndpoint;
    KSTOPOLOGY_ENDPOINTID m_FmRxAntennaEndpoint;
    ULONG m_FmRxVolume;

  public:
    DECLARE_STD_UNKNOWN();
    CFmMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels
    );

    ~CFmMiniportTopology();

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

    NTSTATUS PropertyHandlerFmRxEndpointId
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS PropertyHandlerFmRxVolume
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandlerFmRxAntennaEndpointId
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS EventHandler_JackState
    (
        _In_ PPCEVENT_REQUEST _pEventRequest
    );

    STDMETHODIMP_(NTSTATUS) UpdateTopologyJackState
    (
        _In_ BOOL NewState
    );
};
    
typedef CFmMiniportTopology *PCFmMiniportTopology;

NTSTATUS
CreateFmMiniportTopology( 
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

NTSTATUS PropertyHandler_FmRxTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_FMTOPO_H_

