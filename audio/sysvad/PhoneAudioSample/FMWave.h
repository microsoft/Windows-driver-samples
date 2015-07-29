/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    fmwave.h

Abstract:

    Definition of wavert miniport class for fm

--*/

#include "MinWaveRT.h"

#ifndef _SYSVAD_FMMINWAVERT_H_
#define _SYSVAD_FMMINWAVERT_H_

NTSTATUS
CreateFmMiniportWaveRT
( 
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

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CFmMiniportWaveRT
//   
class CFmMiniportWaveRT : 
    public CMiniportWaveRT,
    public IAdapterPowerManagement
{
private:
    BOOL m_bFmRxState;

public:
    DECLARE_STD_UNKNOWN();
    IMP_IAdapterPowerManagement;

    CFmMiniportWaveRT(
        _In_            PUNKNOWN                                UnknownAdapter,
        _In_            PENDPOINT_MINIPAIR                      MiniportPair,
        _In_opt_        PVOID                                   DeviceContext
        )
        :CMiniportWaveRT(UnknownAdapter, MiniportPair, DeviceContext),
        m_bFmRxState(FALSE)
    {
    }

    ~CFmMiniportWaveRT();

    friend NTSTATUS PropertyHandler_FmRxWaveFilter
    (   
        _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );   

public:
    NTSTATUS PropertyHandler_FmRxState
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

private:
    NTSTATUS UpdateTopologyJackState
    (
        _In_ BOOL NewState
    );
};
typedef CFmMiniportWaveRT *PCFmMiniportWaveRT;

#endif // _SYSVAD_FMMINWAVERT_H_

