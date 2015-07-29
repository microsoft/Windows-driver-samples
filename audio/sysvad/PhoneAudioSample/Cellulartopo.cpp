/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellulartopo.cpp

Abstract:

    Implementation of topology miniport for the cellular endpoint.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "cellulartopo.h"
#include "cellulartoptable.h"
#include <ntstrsafe.h>


#pragma code_seg("PAGE")


// table of valid render/capture combinations for cellular routing.
// speaker and mic (speakerphone)
// headset speaker and headset mic (wired headset)
// headset speaker and mic (wired headset without microphone)
// handset speaker and handset mic (handset)
// HFP speaker and HFP mic (bluetooth hands free)
// special host routing pair
KSTOPOLOGY_ENDPOINTIDPAIR g_EndpointList[] =
{
    { HANDSET_SPEAKER_TOPONAME,     KSPIN_TOPO_LINEOUT_DEST, HANDSET_MIC_TOPONAME,      KSPIN_TOPO_MIC_ELEMENTS },
    { SPEAKER_TOPONAME,             KSPIN_TOPO_LINEOUT_DEST, MIC_ARRAY1_TOPONAME,       KSPIN_TOPO_MIC_ELEMENTS },
    { SPEAKER_HEADSET_TOPONAME,     KSPIN_TOPO_LINEOUT_DEST, MIC_HEADSET_TOPONAME,      KSPIN_TOPO_MIC_ELEMENTS },
    { SPEAKER_HEADSET_TOPONAME,     KSPIN_TOPO_LINEOUT_DEST, MIC_ARRAY1_TOPONAME,       KSPIN_TOPO_MIC_ELEMENTS },
    { HFP_SPEAKER_TOPONAME,         KSPIN_TOPO_LINEOUT_DEST, HFP_MIC_TOPONAME,          KSPIN_TOPO_MIC_ELEMENTS },
    { HOSTRENDER_TOPONAME,          HOSTRENDER_PIN,          HOSTCAPTURE_TOPONAME,      HOSTCAPTURE_PIN         }
};

//=============================================================================
NTSTATUS
CreateCellularMiniportTopology
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

    CCellularMiniportTopology *obj = 
        new (PoolType, MINWAVERT_POOLTAG) 
            CCellularMiniportTopology( UnknownOuter,
                                   MiniportPair->TopoDescriptor,
                                   MiniportPair->DeviceMaxChannels);
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    obj->AddRef();
    *Unknown = reinterpret_cast<IUnknown*>(obj);

    return STATUS_SUCCESS;
} // CreateCellularMiniportTopology

CCellularMiniportTopology::CCellularMiniportTopology
(
    _In_opt_    PUNKNOWN                UnknownOuter,
    _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
    _In_        USHORT                  DeviceMaxChannels
) 
: CUnknown(UnknownOuter), 
  CMiniportTopologySYSVAD(FilterDesc, DeviceMaxChannels),
  m_CellularVolume(0)
{
    PAGED_CODE();

    memcpy(&m_CellularRenderEndpoint, &(g_EndpointList[0].RenderEndpoint), sizeof(m_CellularRenderEndpoint));
    memcpy(&m_CellularCaptureEndpoint, &(g_EndpointList[0].CaptureEndpoint), sizeof(m_CellularCaptureEndpoint));
}


//=============================================================================
CCellularMiniportTopology::~CCellularMiniportTopology
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

    DPF_ENTER(("[CCellularMiniportTopology::~CCellularMiniportTopology]"));
} // ~CCellularMiniportTopology

//=============================================================================
NTSTATUS
CCellularMiniportTopology::DataRangeIntersection
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
CCellularMiniportTopology::GetDescription
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
CCellularMiniportTopology::Init
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

    DPF_ENTER(("[CCellularMiniportTopology::Init]"));

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
CCellularMiniportTopology::NonDelegatingQueryInterface
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
    else if (IsEqualGUIDAligned(Interface, IID_ICellularTopology))
    {
        *Object = PVOID(PCELLULARTOPOLOGY(this));
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
CCellularMiniportTopology::PropertyHandlerJackDescription
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

    ULONG    cJackDescriptions = ARRAYSIZE(CellularJackDescriptions);
    PKSJACK_DESCRIPTION * JackDescriptions = CellularJackDescriptions;

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
CCellularMiniportTopology::PropertyHandlerJackDescription2
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
    
    ULONG                   cJackDescriptions = ARRAYSIZE(CellularJackDescriptions);
    PKSJACK_DESCRIPTION *   JackDescriptions = CellularJackDescriptions;

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
CCellularMiniportTopology::PropertyHandlerTelephonyEndpointIdPair
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_TelephonyTopology, KSPROPERTY_TELEPHONY_ENDPOINTIDPAIR )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerTelephonyEndpointIdPair]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {   
        ntStatus = STATUS_INVALID_PARAMETER;
        ULONG ulExpectedSize = (sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(g_EndpointList));

        if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
        {
            // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
            //
            PKSPROPERTY_DESCRIPTION PropDesc = 
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);
    
            PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_SET |KSPROPERTY_TYPE_GET;
            PropDesc->DescriptionSize   = ulExpectedSize;
            PropDesc->PropTypeSet.Set   = KSPROPSETID_TelephonyTopology;
            PropDesc->PropTypeSet.Id    = KSPROPERTY_TELEPHONY_ENDPOINTIDPAIR;
            PropDesc->PropTypeSet.Flags = 0;
            PropDesc->MembersListCount  = 1;
            PropDesc->Reserved          = 0;

            // buffer is big enough to hold the full data, add that
            if (PropertyRequest->ValueSize >= ulExpectedSize)
            {
                PKSPROPERTY_MEMBERSHEADER MembersHeader = 
                    PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

                MembersHeader->MembersFlags = KSPROPERTY_MEMBER_VALUES;
                MembersHeader->MembersSize = sizeof(KSTOPOLOGY_ENDPOINTIDPAIR);
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
            *(PULONG(PropertyRequest->Value)) = KSPROPERTY_TYPE_SET |KSPROPERTY_TYPE_GET;
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;                    
        }
        else if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = ulExpectedSize;
            ntStatus =  STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        ULONG cbNeeded = sizeof(KSTOPOLOGY_ENDPOINTIDPAIR);

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
                PKSTOPOLOGY_ENDPOINTIDPAIR EndpointIdPair = static_cast<PKSTOPOLOGY_ENDPOINTIDPAIR>(PropertyRequest->Value);

                // Return endpoints (render and capture) info where cellular audio is being routed to.
                memcpy(&(EndpointIdPair->RenderEndpoint), &m_CellularRenderEndpoint, sizeof(m_CellularRenderEndpoint));
                memcpy(&(EndpointIdPair->CaptureEndpoint), &m_CellularCaptureEndpoint, sizeof(m_CellularCaptureEndpoint));

                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                PKSTOPOLOGY_ENDPOINTIDPAIR EndpointIdPair = static_cast<PKSTOPOLOGY_ENDPOINTIDPAIR>(PropertyRequest->Value);

                // verify the requested combination is in the list of valid endpoint combinations, fail with
                // invalid parameter if not found.
                ntStatus = STATUS_INVALID_PARAMETER;
                for (int i = 0; i < SIZEOF_ARRAY(g_EndpointList); i++)
                {
                    if (g_EndpointList[i].RenderEndpoint.PinId == EndpointIdPair->RenderEndpoint.PinId &&
                        0 == _wcsnicmp(g_EndpointList[i].RenderEndpoint.TopologyName, EndpointIdPair->RenderEndpoint.TopologyName, MAX_PATH) &&
                        g_EndpointList[i].CaptureEndpoint.PinId == EndpointIdPair->CaptureEndpoint.PinId &&
                        0 == _wcsnicmp(g_EndpointList[i].CaptureEndpoint.TopologyName, EndpointIdPair->CaptureEndpoint.TopologyName, MAX_PATH))
                    {
                        if (HOSTRENDER_PIN == EndpointIdPair->RenderEndpoint.PinId &&
                            0 == _wcsnicmp(HOSTRENDER_TOPONAME, EndpointIdPair->RenderEndpoint.TopologyName, MAX_PATH) &&
                            HOSTCAPTURE_PIN == EndpointIdPair->CaptureEndpoint.PinId &&
                            0 == _wcsnicmp(HOSTCAPTURE_TOPONAME, EndpointIdPair->CaptureEndpoint.TopologyName, MAX_PATH))
                        {
                            // This is an indication that cellular audio is being routed through OS (For example, user plugs in USB headset
                            // for cellular audio) and driver must stop rendering cellular audio to any physical audio endpoint. Driver must
                            // also stop capturing data from any physical audio endpoint for cellular. This will update audio routing for 
                            // all the active cellular calls.
                        }
                        else
                        {
                            // Route cellular audio to these new endpoints (render and capture). This will update audio routing for 
                            // all the active cellular calls.
                        }

                        memcpy(&m_CellularRenderEndpoint, &(EndpointIdPair->RenderEndpoint), sizeof(m_CellularRenderEndpoint));
                        memcpy(&m_CellularCaptureEndpoint, &(EndpointIdPair->CaptureEndpoint), sizeof(m_CellularCaptureEndpoint));

                        // we found the entry, so return success
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
CCellularMiniportTopology::PropertyHandlerTelephonyVolume
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_TelephonyTopology, KSPROPERTY_TELEPHONY_VOLUME )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{

    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerTelephonyVolume]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupportVolume
            (
                PropertyRequest,
                1                   // Telephony audio is only ever 1 channel
            );

        ULONG cbFullProperty = 
            sizeof(KSPROPERTY_DESCRIPTION) +
            sizeof(KSPROPERTY_MEMBERSHEADER) +
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

            // Telephony volume is only ever single channel, so the
            // MULTICHANNEL and UNIFORM flags are meaningless
            Members->Flags = 0;

            // Telephony Values that may be different from standard volume values
            Range->Bounds.SignedMaximum = TELEPHONY_VOLUME_MAXIMUM;
            Range->Bounds.SignedMinimum = TELEPHONY_VOLUME_MINIMUM;
            Range->SteppingDelta        = TELEPHONY_VOLUME_STEPPING;
        }
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest,
                sizeof(LONG),       // volume value is a LONG
                0                   // there is no channel associated with telephony volume
            );


        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                LONG* volume = (LONG*)PropertyRequest->Value;
                // Return current cellular volume (Incall volume).
                *volume = m_CellularVolume;
                PropertyRequest->ValueSize = sizeof(LONG);
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                LONG* volume = (LONG*)PropertyRequest->Value;

                LONG normalizedVolume = 
                    VALUE_NORMALIZE_IN_RANGE_EX
                    (
                        *volume, 
                        TELEPHONY_VOLUME_MINIMUM, 
                        TELEPHONY_VOLUME_MAXIMUM, 
                        TELEPHONY_VOLUME_STEPPING
                    );
                if (normalizedVolume == *volume)
                {
                    // Update the cellular volume (Incall volume). This will update cellular volume for all the active cellular calls.
                    m_CellularVolume = *volume;
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
PropertyHandler_CellularTopoFilter
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

    DPF_ENTER(("[PropertyHandler_CellularTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCCellularMiniportTopology  pMiniport = (PCCellularMiniportTopology)PropertyRequest->MajorTarget;

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
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_TelephonyTopology))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_ENDPOINTIDPAIR)
        {
            ntStatus =  pMiniport->PropertyHandlerTelephonyEndpointIdPair(PropertyRequest);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_TELEPHONY_VOLUME)
        {
            ntStatus =  pMiniport->PropertyHandlerTelephonyVolume(PropertyRequest);
        }
    }


    return ntStatus;
} // PropertyHandler_CellularTopoFilter

#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CCellularMiniportTopology::UpdateTopologyJackState
(
    _In_ ULONG TelephonyId,
    _In_ BOOL NewState
)
{
    PAGED_CODE ();

    struct JackState
    {
        ULONG TelephonyId;
        ULONG PinId;
        PKSJACK_DESCRIPTION JackControl;
    };

    JackState Jacks[] =
    {
        {
            0,
            KSPIN_TOPO_BIDI1,
            &CellularJackDesc
        },
        {
            1,
            KSPIN_TOPO_BIDI2,
            &CellularJackDesc2
        }
    };

    // Query the registry and update the jack state for all of the pins.
    for (int i = 0; i < SIZEOF_ARRAY(Jacks); i++)
    {
        if (Jacks[i].TelephonyId == TelephonyId)
        {
            Jacks[i].JackControl->IsConnected = NewState;

            // we have multiple pins which share the same jack state,
            // notify for every pin using that jack state
            GenerateEventList(
                (GUID*)&KSEVENTSETID_PinCapsChange, // event set. NULL is a wild card for all events.
                KSEVENT_PINCAPS_JACKINFOCHANGE,     // event ID.
                TRUE,                               // use pid ID.
                Jacks[i].PinId,                                // pin ID.
                FALSE,                              // do not use node ID.
                ULONG(-1));                         // node ID, not used.
        }
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CCellularMiniportWaveRT_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    CCellularMiniportTopology* miniport = reinterpret_cast<CCellularMiniportTopology*>(EventRequest->MajorTarget);
    return miniport->EventHandler_JackState(EventRequest);
}

NTSTATUS CCellularMiniportTopology::EventHandler_JackState
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


