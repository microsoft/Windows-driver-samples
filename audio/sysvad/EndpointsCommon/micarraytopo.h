
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarraytopo.h

Abstract:

    Declaration of mic array topology miniport. 

--*/

#ifndef _SYSVAD_MICARRAYTOPO_H_
#define _SYSVAD_MICARRAYTOPO_H_

#include "basetopo.h"

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CMicArrayMiniportTopology 
//   

class CMicArrayMiniportTopology : 
    public CMiniportTopologySYSVAD,
    public IMiniportTopology,
    public CUnknown
{
  public:
    DECLARE_STD_UNKNOWN();
    CMicArrayMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels,
        _In_        eDeviceType             DeviceType
    ) 
    : CUnknown(UnknownOuter), 
      CMiniportTopologySYSVAD(FilterDesc, DeviceMaxChannels),
      m_DeviceType(DeviceType)
    {
        ASSERT(m_DeviceType == eMicArrayDevice1 || 
               m_DeviceType == eMicArrayDevice2 ||
               m_DeviceType == eMicArrayDevice3);
    }
    
    ~CMicArrayMiniportTopology();

    IMP_IMiniportTopology;

    NTSTATUS            PropertyHandlerMicArrayGeometry
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

  protected:
    eDeviceType     m_DeviceType;

  protected:
    bool IsFront() 
    {
        return m_DeviceType == eMicArrayDevice1;
    }
    
    bool IsBack() 
    {
        return m_DeviceType == eMicArrayDevice2;
    }
    
    bool IsCombined() 
    {
        return m_DeviceType == eMicArrayDevice3;
    }
};
    
typedef CMicArrayMiniportTopology *PCMicArrayMiniportTopology;


NTSTATUS
CreateMicArrayMiniportTopology( 
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

NTSTATUS PropertyHandler_MicArrayTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS PropertyHandler_MicArrayTopology(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_MICARRAYTOPO_H_

