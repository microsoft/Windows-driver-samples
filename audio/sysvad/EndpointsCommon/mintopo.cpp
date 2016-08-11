/*++

Copyright (c) 1997-2011  Microsoft Corporation All Rights Reserved

Module Name:

    mintopo.cpp

Abstract:

    Implementation of topology miniport.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "minwavert.h"
#include "mintopo.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CreateMiniportTopologySYSVAD
( 
    _Out_           PUNKNOWN *                              Unknown,
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

    Creates a new topology miniport.

Arguments:

  Unknown - 

  RefclsId -

  PoolType -
  
  UnknownOuter -

  DeviceContext - 
  
  MiniportPair -  

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(UnknownAdapter);

    ASSERT(Unknown);
    ASSERT(MiniportPair);

    CMiniportTopology *obj = 
        new (PoolType, MINWAVERT_POOLTAG) 
            CMiniportTopology( UnknownOuter,
                               MiniportPair->TopoDescriptor,
                               MiniportPair->DeviceMaxChannels,
                               MiniportPair->DeviceType, 
                               DeviceContext );
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    obj->AddRef();
    *Unknown = reinterpret_cast<IUnknown*>(obj);

    return STATUS_SUCCESS;
} // CreateMiniportTopologySYSVAD

//=============================================================================
#pragma code_seg("PAGE")
CMiniportTopology::~CMiniportTopology
(
    void
)
/*++

Routine Description:

  Topology miniport destructor

Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopology::~CMiniportTopology]"));

#ifdef SYSVAD_BTH_BYPASS
    if (IsBthHfpDevice())
    {
        ASSERT(m_BthHfpDevice != NULL);
        
        //
        // Register with BthHfpDevice to get notification events.
        //
        if (m_DeviceType == eBthHfpMicDevice)
        {
            m_BthHfpDevice->SetMicVolumeHandler(NULL, NULL);
            m_BthHfpDevice->SetMicConnectionStatusHandler(NULL, NULL);
        }
        else 
        {
            ASSERT(m_DeviceType == eBthHfpSpeakerDevice);
            
            m_BthHfpDevice->SetSpeakerVolumeHandler(NULL, NULL);
            m_BthHfpDevice->SetSpeakerConnectionStatusHandler(NULL, NULL);
        }

        SAFE_RELEASE(m_BthHfpDevice);   // IBthHfpDeviceCommon
    }
#endif // SYSVAD_BTH_BYPASS


} // ~CMiniportTopology

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportTopology::DataRangeIntersection
( 
    _In_        ULONG                   PinId,
    _In_        PKSDATARANGE            ClientDataRange,
    _In_        PKSDATARANGE            MyDataRange,
    _In_        ULONG                   OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength)
                PVOID                   ResultantFormat,
    _Out_       PULONG                  ResultantFormatLength 
)
/*++

Routine Description:

  The DataRangeIntersection function determines the highest quality 
  intersection of two data ranges.

Arguments:

  PinId - Pin for which data intersection is being determined. 

  ClientDataRange - Pointer to KSDATARANGE structure which contains the data range 
                    submitted by client in the data range intersection property 
                    request. 

  MyDataRange - Pin's data range to be compared with client's data range. 

  OutputBufferLength - Size of the buffer pointed to by the resultant format 
                       parameter. 

  ResultantFormat - Pointer to value where the resultant format should be 
                    returned. 

  ResultantFormatLength - Actual length of the resultant format that is placed 
                          at ResultantFormat. This should be less than or equal 
                          to OutputBufferLength. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    return 
        CMiniportTopologySYSVAD::DataRangeIntersection
        (
            PinId,
            ClientDataRange,
            MyDataRange,
            OutputBufferLength,
            ResultantFormat,
            ResultantFormatLength
        );
} // DataRangeIntersection

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CMiniportTopology::GetDescription
( 
    _Out_ PPCFILTER_DESCRIPTOR *  OutFilterDescriptor 
)
/*++

Routine Description:

  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure. This is the placeholder for the FromNode or ToNode fields in 
  connections which describe connections to the filter's pins. 

Arguments:

  OutFilterDescriptor - Pointer to the filter description. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    return CMiniportTopologySYSVAD::GetDescription(OutFilterDescriptor);        
} // GetDescription

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CMiniportTopology::Init
( 
    _In_ PUNKNOWN                 UnknownAdapter,
    _In_ PRESOURCELIST            ResourceList,
    _In_ PPORTTOPOLOGY            Port_ 
)
/*++

Routine Description:

  The Init function initializes the miniport. Callers of this function 
  should run at IRQL PASSIVE_LEVEL

Arguments:

  UnknownAdapter - A pointer to the Iuknown interface of the adapter object. 

  ResourceList - Pointer to the resource list to be supplied to the miniport 
                 during initialization. The port driver is free to examine the 
                 contents of the ResourceList. The port driver will not be 
                 modify the ResourceList contents. 

  Port - Pointer to the topology port object that is linked with this miniport. 

Return Value:

  NT status code.

--*/
{
    UNREFERENCED_PARAMETER(ResourceList);

    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(Port_);

    DPF_ENTER(("[CMiniportTopology::Init]"));

    NTSTATUS                    ntStatus;

    ntStatus = 
        CMiniportTopologySYSVAD::Init
        (
            UnknownAdapter,
            Port_
        );

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: CMiniportTopologySYSVAD::Init failed, 0x%x", ntStatus)),
        Done);
    
#ifdef SYSVAD_BTH_BYPASS
    if (IsBthHfpDevice())
    {
        PBTHHFPDEVICECOMMON bthHfpDevice = NULL;
        
        bthHfpDevice = GetBthHfpDevice(); // weak ref.
        ASSERT(bthHfpDevice != NULL);
        
        //
        // Register with BthHfpDevice to get notification events.
        //
        if (m_DeviceType == eBthHfpMicDevice)
        {
            bthHfpDevice->SetMicVolumeHandler(
                EvtMicVolumeHandler,                // handler
                PCMiniportTopology(this));          // context.
            
            bthHfpDevice->SetMicConnectionStatusHandler(
                EvtMicConnectionStatusHandler,      // handler
                PCMiniportTopology(this));          // context.
        }
        else 
        {
            ASSERT(m_DeviceType == eBthHfpSpeakerDevice);
            
            bthHfpDevice->SetSpeakerVolumeHandler(
                EvtSpeakerVolumeHandler,            // handler
                PCMiniportTopology(this));          // context.
            
            bthHfpDevice->SetSpeakerConnectionStatusHandler(
                EvtSpeakerConnectionStatusHandler,  // handler
                PCMiniportTopology(this));          // context.
        }
    }
#endif  // SYSVAD_BTH_BYPASS

Done:
    return ntStatus;
} // Init

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CMiniportTopology::NonDelegatingQueryInterface
( 
    _In_ REFIID                  Interface,
    _COM_Outptr_ PVOID      * Object 
)
/*++

Routine Description:

  QueryInterface for MiniportTopology

Arguments:

  Interface - GUID of the interface

  Object - interface object to be returned.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology))
    {
        *Object = PVOID(PMINIPORTTOPOLOGY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        // We reference the interface for the caller.
        PUNKNOWN(*Object)->AddRef();
        return(STATUS_SUCCESS);
    }

    return(STATUS_INVALID_PARAMETER);
} // NonDelegatingQueryInterface

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportTopology::PropertyHandlerJackDescription
( 
    _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
    _In_        ULONG                                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *  JackDescriptions
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION )

Arguments:

  PropertyRequest       - 
  cJackDescriptions     - # of elements in the jack descriptions array. 
  JackDescriptions      - Array of jack descriptions pointers. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescription]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if ((nPinId < cJackDescriptions) && (JackDescriptions[nPinId] != NULL))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    (
                        PropertyRequest,
                        KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
                        VT_ILLEGAL
                    );
            }
            else
            {
                ULONG cbNeeded = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);

                if (PropertyRequest->ValueSize == 0)
                {
                    PropertyRequest->ValueSize = cbNeeded;
                    ntStatus = STATUS_BUFFER_OVERFLOW;
                }
                else if (PropertyRequest->ValueSize < cbNeeded)
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                    {
                        PKSMULTIPLE_ITEM pMI = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
                        PKSJACK_DESCRIPTION pDesc = (PKSJACK_DESCRIPTION)(pMI+1);

                        pMI->Size = cbNeeded;
                        pMI->Count = 1;

                        RtlCopyMemory(pDesc, JackDescriptions[nPinId], sizeof(KSJACK_DESCRIPTION));
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportTopology::PropertyHandlerJackDescription2
( 
    _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
    _In_        ULONG                                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *  JackDescriptions,
    _In_        DWORD                                    JackCapabilities
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION2 )

Arguments:

  PropertyRequest       - 
  cJackDescriptions     - # of elements in the jack descriptions array. 
  JackDescriptions      - Array of jack descriptions pointers. 
  JackCapabilities      - Jack capabilities flags.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescription2]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if ((nPinId < cJackDescriptions) && (JackDescriptions[nPinId] != NULL))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    (
                        PropertyRequest,
                        KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
                        VT_ILLEGAL
                    );
            }
            else
            {
                ULONG cbNeeded = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION2);

                if (PropertyRequest->ValueSize == 0)
                {
                    PropertyRequest->ValueSize = cbNeeded;
                    ntStatus = STATUS_BUFFER_OVERFLOW;
                }
                else if (PropertyRequest->ValueSize < cbNeeded)
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                    {
                        PKSMULTIPLE_ITEM pMI = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
                        PKSJACK_DESCRIPTION2 pDesc = (PKSJACK_DESCRIPTION2)(pMI+1);

                        pMI->Size = cbNeeded;
                        pMI->Count = 1;
                        
                        RtlZeroMemory(pDesc, sizeof(KSJACK_DESCRIPTION2));

                        //
                        // Specifies the lower 16 bits of the DWORD parameter. This parameter indicates whether 
                        // the jack is currently active, streaming, idle, or hardware not ready.
                        //
                        pDesc->DeviceStateInfo = 0;

                        //
                        // From MSDN:
                        // "If an audio device lacks jack presence detection, the IsConnected member of
                        // the KSJACK_DESCRIPTION structure must always be set to TRUE. To remove the 
                        // ambiguity that results from this dual meaning of the TRUE value for IsConnected, 
                        // a client application can call IKsJackDescription2::GetJackDescription2 to read 
                        // the JackCapabilities flag of the KSJACK_DESCRIPTION2 structure. If this flag has
                        // the JACKDESC2_PRESENCE_DETECT_CAPABILITY bit set, it indicates that the endpoint 
                        // does in fact support jack presence detection. In that case, the return value of 
                        // the IsConnected member can be interpreted to accurately reflect the insertion status
                        // of the jack."
                        //
                        // Bit definitions:
                        // 0x00000001 - JACKDESC2_PRESENCE_DETECT_CAPABILITY
                        // 0x00000002 - JACKDESC2_DYNAMIC_FORMAT_CHANGE_CAPABILITY 
                        //
                        pDesc->JackCapabilities = JackCapabilities;
                        
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
}

#ifdef SYSVAD_BTH_BYPASS
//=============================================================================
#pragma code_seg()
VOID
CMiniportTopology::EvtSpeakerVolumeHandler
(
    _In_opt_    PVOID   Context
)
{
    DPF_ENTER(("[CMiniportTopologySYSVAD::EvtSpeakerVolumeHandler]"));

    PCMiniportTopology This = PCMiniportTopology(Context);
    if (This == NULL)
    {
        DPF(D_ERROR, ("EvtSpeakerVolumeHandler: context is null")); 
        return;
    }
    
    This->GenerateEventList(
        (GUID*)&KSEVENTSETID_AudioControlChange, // event set. NULL is a wild card for all events.
        KSEVENT_CONTROL_CHANGE,             // event ID.
        FALSE,                              // do not use pid ID.
        ULONG(-1),                          // pin ID, not used.
        TRUE,                               // use node ID
        KSNODE_TOPO_VOLUME);                // node ID.
}

//=============================================================================
#pragma code_seg()
VOID
CMiniportTopology::EvtSpeakerConnectionStatusHandler
(
    _In_opt_    PVOID   Context
)
{
    DPF_ENTER(("[CMiniportTopologySYSVAD::EvtSpeakerConnectionStatusHandler]"));

    PCMiniportTopology This = PCMiniportTopology(Context);
    if (This == NULL)
    {
        DPF(D_ERROR, ("EvtSpeakerConnectionStatusHandler: context is null")); 
        return;
    }
    
    This->GenerateEventList(
        (GUID*)&KSEVENTSETID_PinCapsChange, // event set. NULL is a wild card for all events.
        KSEVENT_PINCAPS_JACKINFOCHANGE,     // event ID.
        TRUE,                               // use pid ID.
        KSPIN_TOPO_LINEOUT_DEST,            // pin ID.
        FALSE,                              // do not use node ID.
        ULONG(-1));                         // node ID, not used.
}

//=============================================================================
#pragma code_seg()
VOID
CMiniportTopology::EvtMicVolumeHandler
(
    _In_opt_    PVOID   Context
)
{
    DPF_ENTER(("[CMiniportTopologySYSVAD::EvtMicVolumeHandler]"));

    PCMiniportTopology This = PCMiniportTopology(Context);
    if (This == NULL)
    {
        DPF(D_ERROR, ("EvtMicVolumeHandler: context is null")); 
        return;
    }
    
    This->GenerateEventList(
        (GUID*)&KSEVENTSETID_AudioControlChange, // event set. NULL is a wild card for all events.
        KSEVENT_CONTROL_CHANGE,             // event ID.
        FALSE,                              // do not use pid ID.
        ULONG(-1),                          // pin ID, not used.
        TRUE,                               // use node ID.
        KSNODE_TOPO_VOLUME);                // node ID.
}

//=============================================================================
#pragma code_seg()
VOID
CMiniportTopology::EvtMicConnectionStatusHandler
(
    _In_opt_    PVOID   Context
)
{
    DPF_ENTER(("[CMiniportTopologySYSVAD::EvtMicConnectionStatusHandler]"));

    PCMiniportTopology This = PCMiniportTopology(Context);
    if (This == NULL)
    {
        DPF(D_ERROR, ("EvtMicConnectionStatusHandler: context is null")); 
        return;
    }
    
    This->GenerateEventList(
        (GUID*)&KSEVENTSETID_PinCapsChange, // event set. NULL is a wild card for all events.
        KSEVENT_PINCAPS_JACKINFOCHANGE,     // event ID.
        TRUE,                               // use pid ID.
        KSPIN_TOPO_MIC_ELEMENTS,            // pin ID.
        FALSE,                              // do not use node ID.
        ULONG(-1));                         // node ID, not used.
}
#endif  // SYSVAD_BTH_BYPASS

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_Topology
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_Topology]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    return 
        ((PCMiniportTopology)
        (PropertyRequest->MajorTarget))->PropertyHandlerGeneric
                    (
                        PropertyRequest
                    );
} // PropertyHandler_Topology

#pragma code_seg()

//=============================================================================
NTSTATUS CMiniportTopology_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    CMiniportTopology* miniport = reinterpret_cast<CMiniportTopology*>(EventRequest->MajorTarget);
    if (EventRequest->Verb == PCEVENT_VERB_ADD)
    {
        miniport->AddEventToEventList(EventRequest->EventEntry);
    }
    return STATUS_SUCCESS;
}


