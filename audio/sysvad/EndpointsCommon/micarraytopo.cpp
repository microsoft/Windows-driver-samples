/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarraytopo.cpp

Abstract:

    Implementation of topology miniport for the mic array.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "micarraytopo.h"
#include "micarray1toptable.h"

#pragma code_seg("PAGE")

//=============================================================================
NTSTATUS
CreateMicArrayMiniportTopology
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

    CMicArrayMiniportTopology *obj = 
        new (PoolType, MINTOPORT_POOLTAG) 
            CMicArrayMiniportTopology( UnknownOuter,
                                       MiniportPair->TopoDescriptor,
                                       MiniportPair->DeviceMaxChannels,
                                       MiniportPair->DeviceType );
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    obj->AddRef();
    
    *Unknown = reinterpret_cast<IUnknown*>(obj);

    return STATUS_SUCCESS;
} // CreateMicArrayMiniportTopology

//=============================================================================
CMicArrayMiniportTopology::~CMicArrayMiniportTopology
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

    DPF_ENTER(("[CMicArrayMiniportTopology::~CMicArrayMiniportTopology]"));
} // ~CMicArrayMiniportTopology

//=============================================================================
NTSTATUS
CMicArrayMiniportTopology::DataRangeIntersection
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
CMicArrayMiniportTopology::GetDescription
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
CMicArrayMiniportTopology::Init
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

  UnknownAdapter - A pointer to the IUnknown interface of the adapter object. 

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

    DPF_ENTER(("[CMicArrayMiniportTopology::Init]"));

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
CMicArrayMiniportTopology::NonDelegatingQueryInterface
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
CMicArrayMiniportTopology::PropertyHandlerMicArrayGeometry
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Audio, KSPROPERTY_AUDIO_MIC_ARRAY_GEOMETRY )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerMicArrayGeometry]"));

    NTSTATUS    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG       nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if (nPinId == KSPIN_TOPO_MIC_ELEMENTS)
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
                ULONG cElements = IsCombined()? 4 : 2;
                ULONG cbNeeded = FIELD_OFFSET(KSAUDIO_MIC_ARRAY_GEOMETRY, KsMicCoord) +
                                 cElements * sizeof(KSAUDIO_MICROPHONE_COORDINATES);

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
                        PKSAUDIO_MIC_ARRAY_GEOMETRY pMAG = (PKSAUDIO_MIC_ARRAY_GEOMETRY)PropertyRequest->Value;
                        const SHORT MicArray_180_Degrees = 31416; // 10000 * pi
                        const SHORT MicArray_45_Degrees  = 7854;  // 10000 * pi / 4

                        if (IsFront())
                        {
                            // fill in mic array geometry fields
                            pMAG->usVersion = 0x0100;           // Version of Mic array specification (0x0100)
                            pMAG->usMicArrayType = (USHORT)KSMICARRAY_MICARRAYTYPE_LINEAR;        // Type of Mic Array
                            pMAG->wVerticalAngleBegin = -MicArray_45_Degrees; // Work Volume Vertical Angle Begin
                            pMAG->wVerticalAngleEnd   =  MicArray_45_Degrees; // Work Volume Vertical Angle End
                            pMAG->wHorizontalAngleBegin = 0;    // Work Volume HorizontalAngle Begin
                            pMAG->wHorizontalAngleEnd   = 0;    // Work Volume HorizontalAngle End
                            pMAG->usFrequencyBandLo = 100;      // Low end of Freq Range
                            pMAG->usFrequencyBandHi = 8000;     // High end of Freq Range
                            
                            pMAG->usNumberOfMicrophones = 2;    // Count of microphone coordinate structures to follow.

                            pMAG->KsMicCoord[0].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[0].wXCoord = 0; 
                            pMAG->KsMicCoord[0].wYCoord = -100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[0].wZCoord = 0;         
                            pMAG->KsMicCoord[0].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[0].wHorizontalAngle = 0;

                            pMAG->KsMicCoord[1].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[1].wXCoord = 0;  
                            pMAG->KsMicCoord[1].wYCoord = 100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[1].wZCoord = 0;         
                            pMAG->KsMicCoord[1].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[1].wHorizontalAngle = 0;
                        }
                        else if (IsBack()) // in this sample the geometries for front and back are the same.
                        {
                            // fill in mic array geometry fields
                            pMAG->usVersion = 0x0100;           // Version of Mic array specification (0x0100)
                            pMAG->usMicArrayType = (USHORT)KSMICARRAY_MICARRAYTYPE_LINEAR;        // Type of Mic Array
                            pMAG->wVerticalAngleBegin = -MicArray_45_Degrees;  // Work Volume Vertical Angle Begin
                            pMAG->wVerticalAngleEnd   =  MicArray_45_Degrees;  // Work Volume Vertical Angle End
                            pMAG->wHorizontalAngleBegin = 0;    // Work Volume HorizontalAngle Begin
                            pMAG->wHorizontalAngleEnd   = 0;    // Work Volume HorizontalAngle End
                            pMAG->usFrequencyBandLo = 100;      // Low end of Freq Range
                            pMAG->usFrequencyBandHi = 8000;     // High end of Freq Range
                            
                            pMAG->usNumberOfMicrophones = 2;    // Count of microphone coordinate structures to follow.

                            pMAG->KsMicCoord[0].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[0].wXCoord = 0; 
                            pMAG->KsMicCoord[0].wYCoord = -100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[0].wZCoord = 0;         
                            pMAG->KsMicCoord[0].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[0].wHorizontalAngle = 0;

                            pMAG->KsMicCoord[1].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[1].wXCoord = 0;  
                            pMAG->KsMicCoord[1].wYCoord = 100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[1].wZCoord = 0;         
                            pMAG->KsMicCoord[1].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[1].wHorizontalAngle = 0;
                        }
                        else 
                        {
                            // fill in mic array geometry fields
                            pMAG->usVersion = 0x0100;           // Version of Mic array specification (0x0100)
                            pMAG->usMicArrayType = (USHORT)KSMICARRAY_MICARRAYTYPE_LINEAR;        // Type of Mic Array
                            pMAG->wVerticalAngleBegin = -MicArray_45_Degrees; // Work Volume Vertical Angle Begin 
                            pMAG->wVerticalAngleEnd   =  MicArray_45_Degrees; // Work Volume Vertical Angle End
                            pMAG->wHorizontalAngleBegin = -MicArray_180_Degrees; // Work Volume HorizontalAngle Begin
                            pMAG->wHorizontalAngleEnd   =  MicArray_180_Degrees; // Work Volume HorizontalAngle End
                            pMAG->usFrequencyBandLo = 100;      // Low end of Freq Range
                            pMAG->usFrequencyBandHi = 8000;     // High end of Freq Range
                            
                            pMAG->usNumberOfMicrophones = 4;    // Count of microphone coordinate structures to follow.

                            // front elements
                            pMAG->KsMicCoord[0].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[0].wXCoord = 0; 
                            pMAG->KsMicCoord[0].wYCoord = -100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[0].wZCoord = 0;         
                            pMAG->KsMicCoord[0].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[0].wHorizontalAngle = 0;

                            pMAG->KsMicCoord[1].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[1].wXCoord = 0;  
                            pMAG->KsMicCoord[1].wYCoord = 100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[1].wZCoord = 0;         
                            pMAG->KsMicCoord[1].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[1].wHorizontalAngle = 0;

                            // back elements
                            pMAG->KsMicCoord[2].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[2].wXCoord = 0; 
                            pMAG->KsMicCoord[2].wYCoord = -100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[2].wZCoord = 0;         
                            pMAG->KsMicCoord[2].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[2].wHorizontalAngle = -MicArray_180_Degrees;
                            
                            pMAG->KsMicCoord[3].usType = (USHORT)KSMICARRAY_MICTYPE_CARDIOID;          
                            pMAG->KsMicCoord[3].wXCoord = 0;  
                            pMAG->KsMicCoord[3].wYCoord = 100; // mic elements are 200 mm apart        
                            pMAG->KsMicCoord[3].wZCoord = 0;         
                            pMAG->KsMicCoord[3].wVerticalAngle = 0;  
                            pMAG->KsMicCoord[3].wHorizontalAngle = -MicArray_180_Degrees;
                            
                        }
                        
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
CMicArrayMiniportTopology::PropertyHandlerJackDescription
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION )

Arguments:

  PropertyRequest - 

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

        if (nPinId == KSPIN_TOPO_MIC_ELEMENTS)
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

                        pDesc->ChannelMapping   = 0;                // Don't specify channel mask for array mic
                        pDesc->Color            = 0x00000000;       // Black.  This is an integrated device
                        pDesc->ConnectionType   = eConnTypeUnknown; // Integrated.
                        pDesc->GenLocation      = eGenLocPrimaryBox;
                        pDesc->GeoLocation      = IsFront() ? eGeoLocFront : 
                                                    (IsBack() ? eGeoLocRear : eGeoLocNotApplicable);
                        pDesc->PortConnection   = ePortConnIntegratedDevice;
                        pDesc->IsConnected      = TRUE;             // This is an integrated device, so it's always "connected"
                        
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
CMicArrayMiniportTopology::PropertyHandlerJackDescription2
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

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if (nPinId == KSPIN_TOPO_MIC_ELEMENTS)
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
                        pDesc->JackCapabilities = 0;
                        
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
PropertyHandler_MicArrayTopoFilter
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

    DPF_ENTER(("[PropertyHandler_MicArrayTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCMicArrayMiniportTopology  pMiniport = (PCMicArrayMiniportTopology)PropertyRequest->MajorTarget;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Audio))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MIC_ARRAY_GEOMETRY)
        {
            ntStatus = pMiniport->PropertyHandlerMicArrayGeometry(PropertyRequest);
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack))
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

    return ntStatus;
} // PropertyHandler_TopoFilter

//=============================================================================
NTSTATUS
PropertyHandler_MicArrayTopology
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

    DPF_ENTER(("[PropertyHandler_MicArrayTopology]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    PCMicArrayMiniportTopology pMiniport = (PCMicArrayMiniportTopology)PropertyRequest->MajorTarget;

    return pMiniport->PropertyHandlerGeneric(PropertyRequest);
} // PropertyHandler_MicArrayTopology

#pragma code_seg()

