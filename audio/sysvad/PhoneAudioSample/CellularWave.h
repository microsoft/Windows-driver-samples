/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellularwave.h

Abstract:

    Definition of wavert miniport class for cellular

--*/

#include "MinWaveRT.h"

#ifndef _SYSVAD_CELLULARMINWAVERT_H_
#define _SYSVAD_CELLULARMINWAVERT_H_

NTSTATUS
CreateCellularMiniportWaveRT
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
// CCellularMiniportWaveRT
//   
class CCellularMiniportWaveRT : 
    public CMiniportWaveRT,
    public IAdapterPowerManagement
{
private:
    ULONG                               m_TelephonyProviderId;
    TELEPHONY_CALLTYPE                  m_TelephonyCallType;
    TELEPHONY_CALLTYPE                  m_PreviousTelephonyCallType;
    TELEPHONY_CALLSTATE                 m_TelephonyCallState; 
    BOOL                                m_bTelephonyHold;
    BOOL                                m_bTelephonyMuteTx;

public:
    DECLARE_STD_UNKNOWN();
    IMP_IAdapterPowerManagement;

    CCellularMiniportWaveRT(
        _In_            PUNKNOWN                                UnknownAdapter,
        _In_            PENDPOINT_MINIPAIR                      MiniportPair,
        _In_opt_        PVOID                                   DeviceContext
        )
        :CMiniportWaveRT(UnknownAdapter, MiniportPair, DeviceContext),
        m_TelephonyProviderId(0),
        m_TelephonyCallType(TELEPHONY_CALLTYPE_CIRCUITSWITCHED),
        m_PreviousTelephonyCallType(TELEPHONY_CALLTYPE_CIRCUITSWITCHED),
        m_TelephonyCallState(TELEPHONY_CALLSTATE_DISABLED),
        m_bTelephonyHold(FALSE),
        m_bTelephonyMuteTx(FALSE)
    {
        if (IsCellularDevice())
        {
            if (m_DeviceFlags & ENDPOINT_CELLULAR_PROVIDER1)
            {
                m_TelephonyProviderId = 0;
            }
            else if (m_DeviceFlags & ENDPOINT_CELLULAR_PROVIDER2)
            {
                m_TelephonyProviderId = 1;
            }                
        }
    }

    ~CCellularMiniportWaveRT();

    friend NTSTATUS PropertyHandler_CellularWaveFilter
    (   
        _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );   

public:
    NTSTATUS PropertyHandler_TelephonyProviderId
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandler_TelephonyCallInfo
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandler_TelephonyCallState
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandler_TelephonyCallHold
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandler_TelephonyMuteTx
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

private:
    NTSTATUS UpdateTopologyJackState
    (
        _In_ ULONG TelephonyId,
        _In_ BOOL NewState
    );
};
typedef CCellularMiniportWaveRT *PCCellularMiniportWaveRT;

#endif // _SYSVAD_CELLULARMINWAVERT_H_

