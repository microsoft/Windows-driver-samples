/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellularwave.cpp

Abstract:

    Implementation of wavert miniport for cellular

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "CellularWave.h"
#include "CellularTopo.h"
#include "CellularToptable.h"

//=============================================================================
// CCellularMiniportWaveRT
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
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
)
/*++

Routine Description:

  Create the wavert miniport.

Arguments:

  Unknown - 

  RefClsId -

  UnknownOuter -

  PoolType -

  UnkownAdapter -

  DeviceContext -

  MiniportPair -

Return Value:

  NT status code.

--*/
{
    UNREFERENCED_PARAMETER(UnknownOuter);

    PAGED_CODE();

    ASSERT(Unknown);
    ASSERT(MiniportPair);

    CMiniportWaveRT *obj = new (PoolType, MINWAVERT_POOLTAG) CCellularMiniportWaveRT
                                                             (
                                                                UnknownAdapter,
                                                                MiniportPair,
                                                                DeviceContext
                                                             );
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    obj->AddRef();
    *Unknown = reinterpret_cast<IUnknown*>(obj);

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
CCellularMiniportWaveRT::~CCellularMiniportWaveRT
( 
    void 
)
/*++

Routine Description:

  Destructor for wavert miniport

Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::~CCellularMiniportWaveRT]"));

} // ~CCellularMiniportWaveRT


//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CCellularMiniportWaveRT::NonDelegatingQueryInterface
( 
    _In_         REFIID                  Interface,
    _COM_Outptr_ PVOID                   * Object 
)
/*++

Routine Description:

  QueryInterface for CCellularMiniportWaveRT

Arguments:

  Interface - GUID of the interface

  Object - interface object to be returned.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        // We reference the interface for the caller.

        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return CMiniportWaveRT::NonDelegatingQueryInterface(Interface, Object);
}

#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_CellularWaveFilter
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects general property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    CCellularMiniportWaveRT*    pWaveHelper = reinterpret_cast<CCellularMiniportWaveRT*>(PropertyRequest->MajorTarget);

    if (pWaveHelper == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pWaveHelper->AddRef();

    if (pWaveHelper->m_DeviceType == eCellularDevice &&
             IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_TelephonyControl))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_TELEPHONY_PROVIDERID:
                ntStatus = pWaveHelper->PropertyHandler_TelephonyProviderId(PropertyRequest);
                break;

            case KSPROPERTY_TELEPHONY_CALLINFO:
                ntStatus = pWaveHelper->PropertyHandler_TelephonyCallInfo(PropertyRequest);
                break;

            case KSPROPERTY_TELEPHONY_CALLCONTROL:
            case KSPROPERTY_TELEPHONY_PROVIDERCHANGE:
                ntStatus = pWaveHelper->PropertyHandler_TelephonyCallState(PropertyRequest);
                break;

            case KSPROPERTY_TELEPHONY_CALLHOLD:
                ntStatus = pWaveHelper->PropertyHandler_TelephonyCallHold(PropertyRequest);
                break;

            case KSPROPERTY_TELEPHONY_MUTE_TX:
                ntStatus = pWaveHelper->PropertyHandler_TelephonyMuteTx(PropertyRequest);
                break;

            default:
                DPF(D_TERSE, ("[PropertyHandler_CellularWaveFilter: Invalid Device Request]"));
        }
    }
    else 
    {
        ntStatus = PropertyHandler_WaveFilter(PropertyRequest);
    }

    pWaveHelper->Release();

    return ntStatus;
} // PropertyHandler_CellularWaveFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::PropertyHandler_TelephonyProviderId
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::PropertyHandler_TelephonyProviderId]"));
    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ULONG cbMinSize = sizeof(ULONG);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                ULONG* providerId = (ULONG*)PropertyRequest->Value;
                // Return supported TelephonyProviderId (executor id).
                *providerId = m_TelephonyProviderId;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                 ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_TelephonyProviderId

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::PropertyHandler_TelephonyCallInfo
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::PropertyHandler_TelephonyCallInfo]"));
    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ULONG cbMinSize = sizeof(KSTELEPHONY_CALLINFO);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                PKSTELEPHONY_CALLINFO TelephonyCallInfo = static_cast<PKSTELEPHONY_CALLINFO>(PropertyRequest->Value);
                // Return current CallType and CallState information. CallType information is not valid when the CallState is disabled.  
                TelephonyCallInfo->CallType = m_TelephonyCallType;
                TelephonyCallInfo->CallState = m_TelephonyCallState;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                 ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_TelephonyCallInfo

#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::UpdateTopologyJackState
(
    _In_ ULONG TelephonyId,
    _In_ BOOL NewState
)
{
    PAGED_CODE ();
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IUnknown *pUnknown = NULL;
    ICellularTopology *pCellularTopology = NULL;

    // GetFilters returns the topology port, then topology miniport, then wave port, then wave miniport.
    // we only need the topology miniport.
    ntStatus = m_pAdapterCommon->GetFilters(m_pMiniportPair, NULL, &pUnknown, NULL, NULL);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = pUnknown->QueryInterface(IID_ICellularTopology, (PVOID *)&pCellularTopology);
        if (NT_SUCCESS(ntStatus))
        {      
            ntStatus = pCellularTopology->UpdateTopologyJackState(TelephonyId, NewState);
        }
    }

    SAFE_RELEASE(pCellularTopology);
    SAFE_RELEASE(pUnknown);

    return STATUS_SUCCESS;
}
#pragma code_seg()

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::PropertyHandler_TelephonyCallState
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::PropertyHandler_TelephonyCallState]"));
    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ULONG cbMinSize = 0;

        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_CALLCONTROL)
        {
            cbMinSize = sizeof(KSTELEPHONY_CALLCONTROL);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_PROVIDERCHANGE)
        {
            cbMinSize = sizeof(KSTELEPHONY_PROVIDERCHANGE);
        }

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_CALLCONTROL)
                {
                    PKSTELEPHONY_CALLCONTROL TelephonyCallControl = static_cast<PKSTELEPHONY_CALLCONTROL>(PropertyRequest->Value);

                    // KSPROPERTY_TELEPHONY_CALLCONTROL has info about CallType and CallControlOp. TELEPHONY_CALLCONTROLOP_ENABLE will start
                    // cellular call from audio driver perspective, update jack state for associated cellular bidi endpoint to active,
                    // save call type and update the call state to Enabled. TELEPHONY_CALLCONTROLOP_DISABLE will terminate cellular call
                    // from audio driver perspective, update jack state for associated cellular bidi endpoint to unplugged and update 
                    // call state to Disabled. Call type is ignored in this case.
                    switch (TelephonyCallControl->CallControlOp)
                    {
                        case TELEPHONY_CALLCONTROLOP_DISABLE:
                            ASSERT(m_TelephonyCallState != TELEPHONY_CALLSTATE_DISABLED);
                            m_TelephonyCallState = TELEPHONY_CALLSTATE_DISABLED;
                            UpdateTopologyJackState(m_TelephonyProviderId, FALSE);
                            // Release our idle power management requirement
                            m_pAdapterCommon->SetIdlePowerManagement(m_pMiniportPair, TRUE);
                            ntStatus = STATUS_SUCCESS;
                            break;

                        case TELEPHONY_CALLCONTROLOP_ENABLE:
                            ASSERT(m_TelephonyCallState != TELEPHONY_CALLSTATE_ENABLED);
                            m_TelephonyCallType = TelephonyCallControl->CallType;
                            m_TelephonyCallState = TELEPHONY_CALLSTATE_ENABLED;
                            UpdateTopologyJackState(m_TelephonyProviderId, TRUE);
                            // When we're in a call, we need idle power management disabled.
                            m_pAdapterCommon->SetIdlePowerManagement(m_pMiniportPair, FALSE);
                            ntStatus = STATUS_SUCCESS;
                            break;

                        default:
                            ntStatus = STATUS_INVALID_PARAMETER;
                            break;
                    }
                }
                else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_PROVIDERCHANGE)
                {
                    PKSTELEPHONY_PROVIDERCHANGE TelephonyProviderChange = static_cast<PKSTELEPHONY_PROVIDERCHANGE>(PropertyRequest->Value);

                    // KSPROPERTY_TELEPHONY_PROVIDERCHANGE has info about CallType and ProviderChangeOp. TELEPHONY_PROVIDERCHANGEOP_BEGIN will indicate
                    // start of SRVCC to audio driver. Audio driver will then start the SRVCC and update the call state to ProviderTransition.
                    // TELEPHONY_PROVIDERCHANGEOP_END will indicate end of SRVCC to audio driver and driver will end SRVCC process and update 
                    // call state to Enabled. TELEPHONY_PROVIDERCHANGEOP_CANCEL will indicate SRVCC was aborted and audio driver will revert back to
                    // pre SRVCC call.
                    switch (TelephonyProviderChange->ProviderChangeOp)
                    {
                        case TELEPHONY_PROVIDERCHANGEOP_END:
                            m_TelephonyCallState = TELEPHONY_CALLSTATE_ENABLED;
                            ntStatus = STATUS_SUCCESS;
                            break;

                        case TELEPHONY_PROVIDERCHANGEOP_BEGIN:
                            m_PreviousTelephonyCallType = m_TelephonyCallType;
                            m_TelephonyCallType = TelephonyProviderChange->CallType;
                            m_TelephonyCallState = TELEPHONY_CALLSTATE_PROVIDERTRANSITION;
                            ntStatus = STATUS_SUCCESS;
                            break;

                        case TELEPHONY_PROVIDERCHANGEOP_CANCEL:
                            m_TelephonyCallType = m_PreviousTelephonyCallType;
                            m_TelephonyCallState = TELEPHONY_CALLSTATE_ENABLED;
                            ntStatus = STATUS_SUCCESS;
                            break;

                        default:
                            ntStatus = STATUS_INVALID_PARAMETER;
                            break;
                    }
                }
                else
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                 ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_TelephonyCallState

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::PropertyHandler_TelephonyCallHold
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::PropertyHandler_TelephonyCallHold]"));
    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ULONG cbMinSize = sizeof(BOOL);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                BOOL* bTelephonyHold = (BOOL*) PropertyRequest->Value;
                // Return if the call is on hold or not
                *bTelephonyHold = m_bTelephonyHold;
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                BOOL* bTelephonyHold = (BOOL*) PropertyRequest->Value;
                m_bTelephonyHold = *bTelephonyHold;

                // If this property is TRUE, put the call on hold and update the call state to hold.
                // If this property is FALSE, take the call out of hold and update the call state to enabled.
                m_TelephonyCallState = (*bTelephonyHold) ? TELEPHONY_CALLSTATE_HOLD : TELEPHONY_CALLSTATE_ENABLED;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                 ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_TelephonyCallHold

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CCellularMiniportWaveRT::PropertyHandler_TelephonyMuteTx
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CCellularMiniportWaveRT::PropertyHandler_TelephonyMuteTx]"));
    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ULONG cbMinSize = sizeof(BOOL);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                BOOL* bTxMute = (BOOL*) PropertyRequest->Value;
                // Return if the cellular Tx is on muted or not
                *bTxMute = m_bTelephonyMuteTx;
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                BOOL* bTxMute = (BOOL*) PropertyRequest->Value;
                // If this property is TRUE, mute cellular Tx data. However, it should not mute the data that was injected to KSNODETYPE_TELEPHONY_TX. 
                // If this property is FALSE, unmute cellular Tx data.
                m_bTelephonyMuteTx = *bTxMute;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                 ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_TelephonyMuteTx

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CCellularMiniportWaveRT::PowerChangeState
( 
    _In_  POWER_STATE             NewState 
)
{
    DPF_ENTER(("[CCellularMiniportWaveRT::PowerChangeState]"));
    UNREFERENCED_PARAMETER(NewState);

    /*
    Turn on and off cellular specific hardware here, if applicable.
    */

} // PowerStateChange

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CCellularMiniportWaveRT::QueryDeviceCapabilities
( 
    _Inout_updates_bytes_(sizeof(DEVICE_CAPABILITIES)) PDEVICE_CAPABILITIES    PowerDeviceCaps 
)
{
    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    /*
    Unused at the miniport layer. Any device capabilities should be reported in CAdapterCommon
    */

    return (STATUS_SUCCESS);
} // QueryDeviceCapabilities

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CCellularMiniportWaveRT::QueryPowerChangeState
( 
    _In_  POWER_STATE             NewStateQuery 
)
{
    UNREFERENCED_PARAMETER(NewStateQuery);

    DPF_ENTER(("[CCellularMiniportWaveRT::QueryPowerChangeState]"));

    // if we're in a call state other than disabled, then we need the hardware turned on.
    if (TELEPHONY_CALLSTATE_DISABLED != m_TelephonyCallState)
    {
        return STATUS_RESOURCE_IN_USE;
    }

    return STATUS_SUCCESS;
} // QueryPowerChangeState


