
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    handsetspeakertopo.h

Abstract:

    Declaration of topology miniport for the handsetspeaker endpoint.

--*/

#ifndef _SYSVAD_HANDSETSPEAKERTOPO_H_
#define _SYSVAD_HANDSETSPEAKERTOPO_H_

#include "basetopo.h"

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CHandsetSpeakerMiniportTopology 
//   

class CHandsetSpeakerMiniportTopology : 
    public CMiniportTopologySYSVAD,
    public IMiniportTopology,
    public CUnknown
{
  public:
    DECLARE_STD_UNKNOWN();
    CHandsetSpeakerMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels
    ) 
    : CUnknown(UnknownOuter), 
      CMiniportTopologySYSVAD(FilterDesc, DeviceMaxChannels)
    {}
    
    ~CHandsetSpeakerMiniportTopology();

    IMP_IMiniportTopology;

    NTSTATUS            PropertyHandlerJackSinkInfo
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );
    
    NTSTATUS            PropertyHandlerJackDescription
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );

    NTSTATUS            PropertyHandlerJackDescription2
    (
        _In_ PPCPROPERTY_REQUEST  PropertyRequest
    );
};
    
typedef CHandsetSpeakerMiniportTopology *PCHandsetSpeakerMiniportTopology;


NTSTATUS
CreateHandsetSpeakerMiniportTopology( 
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

NTSTATUS PropertyHandler_HandsetSpeakerTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_HANDSETSPEAKERTOPO_H_

