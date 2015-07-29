/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    fmwave.cpp

Abstract:

    Implementation of wavert miniport for fm

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "FmWave.h"
#include "FmTopo.h"
#include "FmToptable.h"

//=============================================================================
// CFmMiniportWaveRT
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
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

    CMiniportWaveRT *obj = new (PoolType, MINWAVERT_POOLTAG) CFmMiniportWaveRT
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
CFmMiniportWaveRT::~CFmMiniportWaveRT
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

    DPF_ENTER(("[CFmMiniportWaveRT::~CFmMiniportWaveRT]"));

} // ~CFmMiniportWaveRT


//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CFmMiniportWaveRT::NonDelegatingQueryInterface
( 
    _In_         REFIID                  Interface,
    _COM_Outptr_ PVOID                   * Object 
)
/*++

Routine Description:

  QueryInterface for CFmMiniportWaveRT

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
PropertyHandler_FmRxWaveFilter
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
    CFmMiniportWaveRT*    pWaveHelper = reinterpret_cast<CFmMiniportWaveRT*>(PropertyRequest->MajorTarget);

    if (pWaveHelper == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pWaveHelper->AddRef();

    if (pWaveHelper->m_DeviceType == eFmRxDevice &&
        IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_FMRXControl))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
        case KSPROPERTY_FMRX_STATE:
            ntStatus = pWaveHelper->PropertyHandler_FmRxState(PropertyRequest);
            break;

        default:
            DPF(D_TERSE, ("[PropertyHandler_FmRxWaveFilter: Invalid Device Request"));
        }
    }
    else
    {
        ntStatus = PropertyHandler_WaveFilter(PropertyRequest);
    }

    pWaveHelper->Release();

    return ntStatus;
} // PropertyHandler_FmRxWaveFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CFmMiniportWaveRT::PropertyHandler_FmRxState
(
_In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CFmMiniportWaveRT::PropertyHandler_FmRxState]"));

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        ntStatus = PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }
    else
    {
        ntStatus =
            ValidatePropertyParams
            (
                PropertyRequest,
                sizeof(BOOL),
                0
            );

        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                BOOL* fmrxState = (BOOL*)PropertyRequest->Value;
                *fmrxState = m_bFmRxState;
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                BOOL fmrxNewState = *(BOOL*)PropertyRequest->Value;
                if (fmrxNewState == m_bFmRxState)
                {
                    ntStatus = STATUS_SUCCESS;
                }
                else
                {
                    // Turn on FM hardware
                    m_bFmRxState = fmrxNewState;

                    // NOTE: Set the Jack state to Active ONLY if the driver supports 
                    // recording FM data. If the driver doesn't support recording FM
                    // data, the jack state should never be changed to Active
                    ntStatus = UpdateTopologyJackState(m_bFmRxState);
                    
                    // When we're using FM, we need idle power management disabled.
                    // passing true here enables power management, false disables.
                    // when FM is enabled (true), we want to pass false do disable idle
                    // power management
                    m_pAdapterCommon->SetIdlePowerManagement(m_pMiniportPair, m_bFmRxState?FALSE:TRUE);
                }
            }
            else
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandler_FmRxState



#pragma code_seg("PAGE")
NTSTATUS
CFmMiniportWaveRT::UpdateTopologyJackState
(
    _In_ BOOL NewState
)
{
    PAGED_CODE ();
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IUnknown *pUnknown = NULL;
    IFmTopology *pFmTopology = NULL;

    // GetFilters returns the topology port, then topology miniport, then wave port, then wave miniport.
    // we only need the topology miniport.
    ntStatus = m_pAdapterCommon->GetFilters(m_pMiniportPair, NULL, &pUnknown, NULL, NULL);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = pUnknown->QueryInterface(IID_IFmTopology, (PVOID *)&pFmTopology);
        if (NT_SUCCESS(ntStatus))
        {      
            ntStatus = pFmTopology->UpdateTopologyJackState(NewState);
        }
    }

    SAFE_RELEASE(pFmTopology);
    SAFE_RELEASE(pUnknown);

    return STATUS_SUCCESS;
}
#pragma code_seg()


//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CFmMiniportWaveRT::PowerChangeState
( 
    _In_  POWER_STATE             NewState 
)
{
    DPF_ENTER(("[CFmMiniportWaveRT::PowerChangeState]"));
    UNREFERENCED_PARAMETER(NewState);

    /*
    Turn on and off fm specific hardware here, if applicable.
    */

} // PowerStateChange

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CFmMiniportWaveRT::QueryDeviceCapabilities
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
CFmMiniportWaveRT::QueryPowerChangeState
( 
    _In_  POWER_STATE             NewStateQuery 
)
{
    UNREFERENCED_PARAMETER(NewStateQuery);

    DPF_ENTER(("[CFmMiniportWaveRT::QueryPowerChangeState]"));

    // if we're in a call state other than disabled, then we need the hardware turned on
    if (m_bFmRxState)
    {
        return STATUS_RESOURCE_IN_USE;
    }

    return STATUS_SUCCESS;
} // QueryPowerChangeState


