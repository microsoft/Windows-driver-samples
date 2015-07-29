/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    fmtopo.cpp

Abstract:

    Implementation of topology miniport for the fm endpoint.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "fmtopo.h"
#include "fmtoptable.h"
#include <ntstrsafe.h>


#pragma code_seg("PAGE")

// Table of valid render endpoints for FM
KSTOPOLOGY_ENDPOINTID g_EndpointList[] = 
{
    { SPEAKER_TOPONAME,         KSPIN_TOPO_LINEOUT_DEST },
    { SPEAKER_HEADSET_TOPONAME, KSPIN_TOPO_LINEOUT_DEST },
    { HOSTRENDER_TOPONAME,      HOSTRENDER_PIN          }
};

KSTOPOLOGY_ENDPOINTID g_AntennaEndpoint =
{
    SPEAKER_HEADSET_TOPONAME,   KSPIN_TOPO_LINEOUT_DEST
};

//=============================================================================
NTSTATUS
CreateFmMiniportTopology
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

    Creates a new topology miniport.

Arguments:

  Unknown - 

  RefclsId -

  UnknownOuter -

  PoolType - 

  UnknownAdapter - 

  DeviceContext -

  MiniportPair -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Unknown);
    ASSERT(MiniportPair);

    UNREFERENCED_PARAMETER(UnknownAdapter);
    UNREFERENCED_PARAMETER(DeviceContext);

    CFmMiniportTopology *obj = 
        new (PoolType, MINWAVERT_POOLTAG) 
            CFmMiniportTopology( UnknownOuter,
                                   MiniportPair->TopoDescriptor,
                                   MiniportPair->DeviceMaxChannels);
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    obj->AddRef();
    *Unknown = reinterpret_cast<IUnknown*>(obj);

    return STATUS_SUCCESS;
} // CreateFmMiniportTopology

CFmMiniportTopology::CFmMiniportTopology
(
    _In_opt_    PUNKNOWN                UnknownOuter,
    _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
    _In_        USHORT                  DeviceMaxChannels
) 
: CUnknown(UnknownOuter), 
  CMiniportTopologySYSVAD(FilterDesc, DeviceMaxChannels),
  m_FmRxVolume(FMRX_VOLUME_MAXIMUM)
{
    PAGED_CODE();

    memcpy(&m_FmRxTargetEndpoint, &g_EndpointList[0], sizeof(m_FmRxTargetEndpoint));
}


//=============================================================================
CFmMiniportTopology::~CFmMiniportTopology
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

    DPF_ENTER(("[CFmMiniportTopology::~CFmMiniportTopology]"));
} // ~CFmMiniportTopology

//=============================================================================
NTSTATUS
CFmMiniportTopology::DataRangeIntersection
( 
    _In_        ULONG                   PinId,
    _In_        PKSDATARANGE            ClientDataRange,
    _In_        PKSDATARANGE            MyDataRange,
    _In_        ULONG                   OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength)
                PVOID                   ResultantFormat     OPTIONAL,
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
STDMETHODIMP
CFmMiniportTopology::GetDescription
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
STDMETHODIMP
CFmMiniportTopology::Init
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

    DPF_ENTER(("[CFmMiniportTopology::Init]"));

    NTSTATUS                    ntStatus;

    ntStatus = 
        CMiniportTopologySYSVAD::Init
        (
            UnknownAdapter,
            Port_
        );

    return ntStatus;
} // Init

//=============================================================================
STDMETHODIMP
CFmMiniportTopology::NonDelegatingQueryInterface
( 
    _In_         REFIID                  Interface,
    _COM_Outptr_ PVOID                   * Object 
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
    else if (IsEqualGUIDAligned(Interface, IID_IFmTopology))
    {
        *Object = PVOID(PFMTOPOLOGY(this));
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
NTSTATUS
CFmMiniportTopology::PropertyHandlerJackDescription
( 
    _In_        PPCPROPERTY_REQUEST                      PropertyRequest
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION )

Arguments:

  PropertyRequest   

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescription]"));

    ULONG    cJackDescriptions = ARRAYSIZE(FmJackDescriptions);
    PKSJACK_DESCRIPTION * JackDescriptions = FmJackDescriptions;

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
NTSTATUS
CFmMiniportTopology::PropertyHandlerJackDescription2
(
_In_ PPCPROPERTY_REQUEST      PropertyRequest
)
/*++

Routine Description:

Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION2 )

Arguments:

PropertyRequest -

Return Value:

NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescription2]"));

    NTSTATUS                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG                   nPinId = (ULONG)-1;

    ULONG                   cJackDescriptions = ARRAYSIZE(FmJackDescriptions);
    PKSJACK_DESCRIPTION *   JackDescriptions = FmJackDescriptions;

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
                ULONG cbNeeded = sizeof(KSMULTIPLE_ITEM)+sizeof(KSJACK_DESCRIPTION2);

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
                        PKSJACK_DESCRIPTION2 pDesc = (PKSJACK_DESCRIPTION2)(pMI + 1);

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
                        pDesc->JackCapabilities = JACKDESC2_PRESENCE_DETECT_CAPABILITY;

                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
}

//=============================================================================
NTSTATUS
CFmMiniportTopology::PropertyHandlerFmRxEndpointId
(
_In_        PPCPROPERTY_REQUEST                      PropertyRequest
)
/*++

Routine Description:

Handles ( KSPROPSETID_FMRXTopology, KSPROPERTY_FMRX_ENDPOINTID )

Arguments:

PropertyRequest

Return Value:

NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerFmRxEndpointId]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        ULONG ulExpectedSize = (sizeof(KSPROPERTY_DESCRIPTION)+sizeof(KSPROPERTY_MEMBERSHEADER)+sizeof(g_EndpointList));

        if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
        {
            // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
            //
            PKSPROPERTY_DESCRIPTION PropDesc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

            PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET;
            PropDesc->DescriptionSize = ulExpectedSize;
            PropDesc->PropTypeSet.Set = KSPROPSETID_FMRXTopology;
            PropDesc->PropTypeSet.Id = KSPROPERTY_FMRX_ENDPOINTID;
            PropDesc->PropTypeSet.Flags = 0;
            PropDesc->MembersListCount = 1;
            PropDesc->Reserved = 0;

            // buffer is big enough to hold the full data, add that
            if (PropertyRequest->ValueSize >= ulExpectedSize)
            {
                PKSPROPERTY_MEMBERSHEADER MembersHeader =
                    PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

                MembersHeader->MembersFlags = KSPROPERTY_MEMBER_VALUES;
                MembersHeader->MembersSize = sizeof(KSTOPOLOGY_ENDPOINTID);
                MembersHeader->MembersCount = SIZEOF_ARRAY(g_EndpointList);
                MembersHeader->Flags = 0;

                memcpy(MembersHeader + 1, g_EndpointList, sizeof(g_EndpointList));

                // tell them how much space we really used, which controls how much data is copied into the user buffer.
                PropertyRequest->ValueSize = ulExpectedSize;
            }
            else
            {
                // tell them how much space we really used, which controls how much data is copied into the user buffer.
                PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
            }

            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            // if return buffer can hold a ULONG, return the access flags.
            *(PULONG(PropertyRequest->Value)) = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET;
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = ulExpectedSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        ntStatus =
            ValidatePropertyParams
            (
                PropertyRequest,
                sizeof(KSTOPOLOGY_ENDPOINTID),
                0
            );

        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                PKSTOPOLOGY_ENDPOINTID EndpointId = static_cast<PKSTOPOLOGY_ENDPOINTID>(PropertyRequest->Value);

                memcpy(EndpointId, &m_FmRxTargetEndpoint, sizeof(m_FmRxTargetEndpoint));

                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                PKSTOPOLOGY_ENDPOINTID EndpointId = static_cast<PKSTOPOLOGY_ENDPOINTID>(PropertyRequest->Value);

                ntStatus = STATUS_INVALID_PARAMETER;
                for (int i = 0; i < SIZEOF_ARRAY(g_EndpointList); i++)
                {
                    if (g_EndpointList[i].PinId == EndpointId->PinId &&
                        0 == _wcsnicmp(g_EndpointList[i].TopologyName, EndpointId->TopologyName, MAX_PATH))
                    {
                        memcpy(&m_FmRxTargetEndpoint, EndpointId, sizeof(m_FmRxTargetEndpoint));

                        if (HOSTRENDER_PIN == EndpointId->PinId &&
                            0 == _wcsnicmp(HOSTRENDER_TOPONAME, EndpointId->TopologyName, MAX_PATH))
                        {
                            // This is an indication that FM audio is being routed through OS (For example, user plugs in
                            // USB headset for FM audio) and driver must stop rendering FM audio to any physical audio endpoint.
                        }
                        else
                        {
                            // Route FM audio to this new endpoint.
                        }
                        ntStatus = STATUS_SUCCESS;
                        break;
                    }
                }
            }
            else
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
}

//=============================================================================
NTSTATUS
CFmMiniportTopology::PropertyHandlerFmRxVolume
(
_In_        PPCPROPERTY_REQUEST                      PropertyRequest
)
/*++

Routine Description:

Handles ( KSPROPSETID_FMRXTopology, KSPROPERTY_FMRX_VOLUME )

Arguments:

PropertyRequest

Return Value:

NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerFmRxVolume]"));

    UNREFERENCED_PARAMETER(PropertyRequest);

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus =
            PropertyHandler_BasicSupportVolume
            (
            PropertyRequest,
            1                   // FM audio technically supports Stereo, but we only support a single volume
            );

        ULONG cbFullProperty =
            sizeof(KSPROPERTY_DESCRIPTION)+
            sizeof(KSPROPERTY_MEMBERSHEADER)+
            sizeof(KSPROPERTY_STEPPING_LONG);

        if (NT_SUCCESS(ntStatus) && PropertyRequest->ValueSize >= cbFullProperty)
        {
            // Access proper values
            PKSPROPERTY_DESCRIPTION PropDesc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);
            PKSPROPERTY_MEMBERSHEADER Members =
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);
            PKSPROPERTY_STEPPING_LONG Range =
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            Members->Flags = 0;

            Range->Bounds.SignedMaximum = FMRX_VOLUME_MAXIMUM;
            Range->Bounds.SignedMinimum = FMRX_VOLUME_MINIMUM;
            Range->SteppingDelta = FMRX_VOLUME_STEPPING;
        }
    }
    else
    {
        ntStatus =
            ValidatePropertyParams
            (
                PropertyRequest,
                sizeof(LONG),       // volume value is a LONG
                0                   // No support for channel since this is a property 
                                    // on the topo filter, not a volume node.
            );


        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                LONG* volume = (LONG*)PropertyRequest->Value;
                // Return current FM volume
                *volume = m_FmRxVolume;
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                LONG* volume = (LONG*)PropertyRequest->Value;

                LONG normalizedVolume =
                    VALUE_NORMALIZE_IN_RANGE_EX
                    (
                    *volume,
                    FMRX_VOLUME_MINIMUM,
                    FMRX_VOLUME_MAXIMUM,
                    FMRX_VOLUME_STEPPING
                    );
                if (normalizedVolume == *volume)
                {
                    m_FmRxVolume = *volume;
                    ntStatus = STATUS_SUCCESS;
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
}

//=============================================================================
NTSTATUS
CFmMiniportTopology::PropertyHandlerFmRxAntennaEndpointId
(
_In_        PPCPROPERTY_REQUEST                      PropertyRequest
)
/*++

Routine Description:

Handles ( KSPROPSETID_FMRXTopology, KSPROPERTY_FMRX_ANTENNAENDPOINTID )

Arguments:

PropertyRequest

Return Value:

NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerFmRxAntennaEndpoint]"));

    UNREFERENCED_PARAMETER(PropertyRequest);

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

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
            sizeof(KSTOPOLOGY_ENDPOINTID),
            0
            );


        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                PKSTOPOLOGY_ENDPOINTID pEndpointId = (PKSTOPOLOGY_ENDPOINTID)PropertyRequest->Value;
                memcpy(pEndpointId, &g_AntennaEndpoint, sizeof(*pEndpointId));
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
}

//=============================================================================
NTSTATUS
PropertyHandler_FmRxTopoFilter
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

    DPF_ENTER(("[PropertyHandler_FmRxTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCFmMiniportTopology  pMiniport = (PCFmMiniportTopology)PropertyRequest->MajorTarget;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION)
        {
            ntStatus = pMiniport->PropertyHandlerJackDescription(PropertyRequest);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION2)
        {
            ntStatus = pMiniport->PropertyHandlerJackDescription2(PropertyRequest);
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_FMRXTopology))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_FMRX_ENDPOINTID)
        {
            ntStatus = pMiniport->PropertyHandlerFmRxEndpointId(PropertyRequest);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_FMRX_VOLUME)
        {
            ntStatus = pMiniport->PropertyHandlerFmRxVolume(PropertyRequest);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_FMRX_ANTENNAENDPOINTID)
        {
            ntStatus = pMiniport->PropertyHandlerFmRxAntennaEndpointId(PropertyRequest);
        }
    }

    return ntStatus;
} // PropertyHandler_FmRxTopoFilter

#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CFmMiniportTopology::UpdateTopologyJackState
(
    _In_ BOOL NewState
)
{
    PAGED_CODE ();

    FmRxJackDesc.IsConnected = NewState;

    // we have multiple pins which share the same jack state,
    // notify for every pin using that jack state
    GenerateEventList(
        (GUID*)&KSEVENTSETID_PinCapsChange, // event set. NULL is a wild card for all events.
        KSEVENT_PINCAPS_JACKINFOCHANGE,     // event ID.
        TRUE,                               // use pid ID.
        KSPIN_TOPO_FMRX,                    // pin ID.
        FALSE,                              // do not use node ID.
        ULONG(-1));                         // node ID, not used.
    
    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CFmMiniportWaveRT_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    CFmMiniportTopology* miniport = reinterpret_cast<CFmMiniportTopology*>(EventRequest->MajorTarget);
    return miniport->EventHandler_JackState(EventRequest);
}

NTSTATUS CFmMiniportTopology::EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    if (EventRequest->Verb == PCEVENT_VERB_ADD)
    {
        m_PortEvents->AddEventToEventList(EventRequest->EventEntry);
    }

    return STATUS_SUCCESS;
}


