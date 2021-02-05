/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minwavert.cpp

Abstract:

    Implementation of wavert miniport.
--*/

#pragma warning (disable : 4127)

#include "definitions.h"
#include <limits.h>
#include "endpoints.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "micarraywavtable.h"

#define EFFECTS_LIST_COUNT 2

//=============================================================================
// CMiniportWaveRT
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CreateMiniportWaveRTSimpleAudioSample
( 
    _Out_           PUNKNOWN                              * Unknown,
    _In_            REFCLSID,
    _In_opt_        PUNKNOWN                                UnknownOuter,
    _In_            POOL_FLAGS                              PoolFlags,
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

    CMiniportWaveRT *obj = new (PoolFlags, MINWAVERT_POOLTAG) CMiniportWaveRT
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
CMiniportWaveRT::~CMiniportWaveRT
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

    DPF_ENTER(("[CMiniportWaveRT::~CMiniportWaveRT]"));

    if (m_pDeviceFormat)
    {
        ExFreePoolWithTag( m_pDeviceFormat, MINWAVERT_POOLTAG );
        m_pDeviceFormat = NULL;
    }

    if (m_pMixFormat)
    {
        ExFreePoolWithTag( m_pMixFormat, MINWAVERT_POOLTAG );
        m_pMixFormat = NULL;
    }

    if (m_pbMuted)
    {
        ExFreePoolWithTag(m_pbMuted, MINWAVERT_POOLTAG);
        m_pbMuted = NULL;
    }

    if (m_plVolumeLevel)
    {
        ExFreePoolWithTag(m_plVolumeLevel, MINWAVERT_POOLTAG);
        m_plVolumeLevel = NULL;
    }

    if (m_pDrmPort)
    {
        m_pDrmPort->Release();
        m_pDrmPort = NULL;
    }

    if (m_pPortEvents)
    {
        m_pPortEvents->Release();
        m_pPortEvents = NULL;
    }
    
    if (m_SystemStreams)
    {
        ExFreePoolWithTag( m_SystemStreams, MINWAVERT_POOLTAG );
        m_SystemStreams = NULL;
    }

} // ~CMiniportWaveRT

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRT::DataRangeIntersection
( 
    _In_        ULONG                       PinId,
    _In_        PKSDATARANGE                ClientDataRange,
    _In_        PKSDATARANGE                MyDataRange,
    _In_        ULONG                       OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength)
                PVOID                       ResultantFormat,
    _Out_       PULONG                      ResultantFormatLength 
)
/*++

Routine Description:

  The DataRangeIntersection function determines the highest quality 
  intersection of two data ranges.

  This sample just sets the ResultantFormat to be the only supported
  format for the MicArray endpoint.

Arguments:

  PinId -           Pin for which data intersection is being determined. 

  ClientDataRange - Pointer to KSDATARANGE structure which contains the data 
                    range submitted by client in the data range intersection 
                    property request. 

  MyDataRange -         Pin's data range to be compared with client's data 
                        range. In this case we actually ignore our own data 
                        range, because we know that we only support one range.

  OutputBufferLength -  Size of the buffer pointed to by the resultant format 
                        parameter. 

  ResultantFormat -     Pointer to value where the resultant format should be 
                        returned. 

  ResultantFormatLength -   Actual length of the resultant format placed in 
                            ResultantFormat. This should be less than or equal 
                            to OutputBufferLength. 

  Return Value:

    NT status code.

--*/
{
    UNREFERENCED_PARAMETER(PinId);

    ULONG                   requiredSize;

    PAGED_CODE();

    if (!IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    //If called for the mic array pin, set ResultantFormat to be the endpoint's only supported format.
    //Otherwise, allow the class handler to set ResultantFormat.  
    if ((this->m_DeviceType) == eMicArrayDevice1)
    {
        requiredSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);

        //
        // Validate return buffer size, if the request is only for the
        // size of the resultant structure, return it now before
        // returning other types of errors.
        //
        if (!OutputBufferLength)
        {
            *ResultantFormatLength = requiredSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        else if (OutputBufferLength < requiredSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //Set ResultantFormat to be the only supported format for the MicArray endpoint. 
        PKSDATAFORMAT_WAVEFORMATEXTENSIBLE resultantFormat;
        resultantFormat = (PKSDATAFORMAT_WAVEFORMATEXTENSIBLE)ResultantFormat;
        *resultantFormat = *MicArrayPinSupportedDeviceFormats;
        *ResultantFormatLength = requiredSize;

        return STATUS_SUCCESS;
    }
    else
    {
        requiredSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);

        //
        // Validate return buffer size, if the request is only for the
        // size of the resultant structure, return it now before
        // returning other types of errors.
        //
        if (!OutputBufferLength)
        {
            *ResultantFormatLength = requiredSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        else if (OutputBufferLength < requiredSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        // Verify channel count is supported. This routine assumes a separate data
        // range for each supported channel count.
        if (((PKSDATARANGE_AUDIO)MyDataRange)->MaximumChannels != ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels)
        {
            return STATUS_NO_MATCH;
        }

        //
        // Ok, let the class handler do the rest.
        //
        return STATUS_NOT_IMPLEMENTED;
    }

} // DataRangeIntersection

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRT::GetDescription
( 
    _Out_ PPCFILTER_DESCRIPTOR * OutFilterDescriptor 
)
/*++

Routine Description:

  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure.

Arguments:

  OutFilterDescriptor - Pointer to the filter description. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    *OutFilterDescriptor = &m_FilterDesc;

    return STATUS_SUCCESS;
} // GetDescription

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRT::Init
( 
    _In_  PUNKNOWN                UnknownAdapter_,
    _In_  PRESOURCELIST           ResourceList_,
    _In_  PPORTWAVERT             Port_ 
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
    UNREFERENCED_PARAMETER(UnknownAdapter_);
    UNREFERENCED_PARAMETER(ResourceList_);
    UNREFERENCED_PARAMETER(Port_);
    PAGED_CODE();

    ASSERT(UnknownAdapter_);
    ASSERT(Port_);

    DPF_ENTER(("[CMiniportWaveRT::Init]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t   size;

    //
    // Init class data members
    //
    m_ulSystemAllocated                 = 0;
    m_SystemStreams                     = NULL;
    m_bGfxEnabled                       = FALSE;
    m_pMixFormat                        = NULL;
    m_pDeviceFormat                     = NULL;
    m_ulMixDrmContentId                 = 0;
    m_pbMuted = NULL;
    m_plVolumeLevel = NULL;
    RtlZeroMemory(&m_MixDrmRights, sizeof(m_MixDrmRights));

    //
    // Init the audio-engine used by the render devices.
    //
    if (IsRenderDevice())
    {
        if (m_ulMaxSystemStreams == 0 )
        {
            return STATUS_INVALID_DEVICE_STATE;
        }
            
        // System streams.
        size = sizeof(PCMiniportWaveRTStream) * m_ulMaxSystemStreams;
        m_SystemStreams = (PCMiniportWaveRTStream *)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, MINWAVERT_POOLTAG);
        if (m_SystemStreams == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        // 
        // For DRM support.
        //
        if (!NT_SUCCESS(Port_->QueryInterface(IID_IDrmPort2, (PVOID *)&m_pDrmPort)))
        {
            m_pDrmPort = NULL;
        }
    }
    
    // 
    // For KS event support.
    //
    if (!NT_SUCCESS(Port_->QueryInterface(IID_IPortEvents, (PVOID *)&m_pPortEvents)))
    {
        m_pPortEvents = NULL;
    }
    
    return ntStatus;
} // Init

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRT::NewStream
( 
    _Out_ PMINIPORTWAVERTSTREAM * OutStream,
    _In_  PPORTWAVERTSTREAM       OuterUnknown,
    _In_  ULONG                   Pin,
    _In_  BOOLEAN                 Capture,
    _In_  PKSDATAFORMAT           DataFormat
)
/*++

Routine Description:

  The NewStream function creates a new instance of a logical stream 
  associated with a specified physical channel. Callers of NewStream should 
  run at IRQL PASSIVE_LEVEL.

Arguments:

  OutStream -

  OuterUnknown -

  Pin - 

  Capture - 

  DataFormat -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutStream);
    ASSERT(DataFormat);

    DPF_ENTER(("[CMiniportWaveRT::NewStream]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PCMiniportWaveRTStream      stream = NULL;
    GUID                        signalProcessingMode = AUDIO_SIGNALPROCESSINGMODE_DEFAULT;
    
    *OutStream = NULL;

     //
    // If the data format attributes were specified, extract them.
    //
    if ( DataFormat->Flags & KSDATAFORMAT_ATTRIBUTES )
    {
        // The attributes are aligned (QWORD alignment) after the data format
        PKSMULTIPLE_ITEM attributes = (PKSMULTIPLE_ITEM) (((PBYTE)DataFormat) + ((DataFormat->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
        ntStatus = GetAttributesFromAttributeList(attributes, attributes->Size, &signalProcessingMode);
    }

    // Check if we have enough streams.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ValidateStreamCreate(Pin, Capture);
    }

    // Determine if the format is valid.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = IsFormatSupported(Pin, Capture, DataFormat);
    }

    // Instantiate a stream. Stream must be in
    // NonPagedPool(Nx) because of file saving.
    //
    if (NT_SUCCESS(ntStatus))
    {
        stream = new (POOL_FLAG_NON_PAGED, MINWAVERT_POOLTAG) 
            CMiniportWaveRTStream(NULL);

        if (stream)
        {
            stream->AddRef();

            ntStatus = 
                stream->Init
                ( 
                    this,
                    OuterUnknown,
                    Pin,
                    Capture,
                    DataFormat,
                    signalProcessingMode
                );
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        *OutStream = PMINIPORTWAVERTSTREAM(stream);
        (*OutStream)->AddRef();

        // The stream has references now for the caller.  The caller expects these
        // references to be there.
    }

    // This is our private reference to the stream.  The caller has
    // its own, so we can release in any case.
    //
    if (stream)
    {
        stream->Release();
    }
    
    return ntStatus;
} // NewStream

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRT::NonDelegatingQueryInterface
( 
    _In_ REFIID  Interface,
    _COM_Outptr_ PVOID * Object 
)
/*++

Routine Description:

  QueryInterface

Arguments:

  Interface - GUID

  Object - interface pointer to be returned.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVERT(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRT))
    {
        *Object = PVOID(PMINIPORTWAVERT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportAudioSignalProcessing))
    {
        *Object = PVOID(PMINIPORTAudioSignalProcessing(this));
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

    return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetDeviceDescription(_Out_ PDEVICE_DESCRIPTION DmaDeviceDescription)
{
    PAGED_CODE ();

    ASSERT (DmaDeviceDescription);

    DPF_ENTER(("[CMiniportWaveRT::GetDeviceDescription]"));

    RtlZeroMemory (DmaDeviceDescription, sizeof (DEVICE_DESCRIPTION));

    //
    // Init device description. This sample is using the same info for all m_DeviceType(s).
    // 
    
    DmaDeviceDescription->Master = TRUE;
    DmaDeviceDescription->ScatterGather = TRUE;
    DmaDeviceDescription->Dma32BitAddresses = TRUE;
    DmaDeviceDescription->InterfaceType = PCIBus;
    DmaDeviceDescription->MaximumLength = 0xFFFFFFFF;

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::GetModes
(
    _In_                                        ULONG     Pin,
    _Out_writes_opt_(*NumSignalProcessingModes) GUID*     SignalProcessingModes,
    _Inout_                                     ULONG*    NumSignalProcessingModes
)
/*

1.	If Pin is not a valid pin number, 
        return STATUS_INVALID_PARAMETER.
        
2.	If Pin is a valid pin number and it supports n modes (n>0), 
        init out-parameters and return STATUS_SUCCESS.
    
3.  Else this pin doesn't support any mode,
        return STATUS_NOT_SUPPORTED. 
        example: bridge pins or another mode-not-aware pins.

*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::GetModes]"));

    NTSTATUS                ntStatus    = STATUS_INVALID_PARAMETER;
    ULONG                   numModes    = 0;
    MODE_AND_DEFAULT_FORMAT *modeInfo   = NULL;

    if (Pin >= m_pMiniportPair->WaveDescriptor->PinCount)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // This method is valid only on the following pins:
    //  render is offload capable:
    //      sink (#0) and offload (#1) pins.
    //  render is NOT offload capable:
    //      sink (#0) pin.
    //  capture device: 
    //      source (#1) pin.
    //
    numModes = GetPinSupportedDeviceModes(Pin, &modeInfo);
    if (numModes == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
   
    // If caller requests the modes, verify sufficient buffer size then return the modes
    if (SignalProcessingModes != NULL)
    {
        if (*NumSignalProcessingModes < numModes)
        {
            *NumSignalProcessingModes = numModes;
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            goto Done;
        }

        for (ULONG i=0; i<numModes; ++i)
        {
            SignalProcessingModes[i] = modeInfo[i].Mode;
        }
    }

    ASSERT(numModes > 0);
    *NumSignalProcessingModes = numModes;
    ntStatus = STATUS_SUCCESS;

Done:   
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::ValidateStreamCreate
(
    _In_    ULONG   _Pin,
    _In_    BOOLEAN _Capture
)
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::ValidateStreamCreate]"));

    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;

    if (_Capture)
    {

        if (IsSystemCapturePin(_Pin))
        {
            VERIFY_PIN_INSTANCE_RESOURCES_AVAILABLE(ntStatus, m_ulSystemAllocated, m_ulMaxSystemStreams);
        }

    }
    else
    {

        if (IsSystemRenderPin(_Pin))
        {
            VERIFY_PIN_INSTANCE_RESOURCES_AVAILABLE(ntStatus, m_ulSystemAllocated, m_ulMaxSystemStreams);
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
VOID CMiniportWaveRT::AcquireFormatsAndModesLock()
{
    KeAcquireSpinLock(&m_DeviceFormatsAndModesLock, &m_DeviceFormatsAndModesIrql);
}

#pragma code_seg()
_Use_decl_annotations_
VOID CMiniportWaveRT::ReleaseFormatsAndModesLock()
{
    KeReleaseSpinLock(&m_DeviceFormatsAndModesLock, m_DeviceFormatsAndModesIrql);
}

//---------------------------------------------------------------------------
// GetPinSupportedDeviceFormats 
//
//  Return supported formats for a given pin.
//
//  Return value
//      The number of KSDATAFORMAT_WAVEFORMATEXTENSIBLE items.
//
//  Remarks
//      Supported formats index array follows same order as filter's pin
//      descriptor list.
//
#pragma code_seg()
_Use_decl_annotations_
ULONG CMiniportWaveRT::GetPinSupportedDeviceFormats(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) KSDATAFORMAT_WAVEFORMATEXTENSIBLE **ppFormats)
{
    PPIN_DEVICE_FORMATS_AND_MODES pDeviceFormatsAndModes = NULL;

    AcquireFormatsAndModesLock();

    pDeviceFormatsAndModes = m_DeviceFormatsAndModes;
    ASSERT(m_DeviceFormatsAndModesCount > PinId);
    ASSERT(pDeviceFormatsAndModes[PinId].WaveFormats != NULL);
    ASSERT(pDeviceFormatsAndModes[PinId].WaveFormatsCount > 0);

    if (ppFormats != NULL)
    {
        *ppFormats = pDeviceFormatsAndModes[PinId].WaveFormats;
    }

    ReleaseFormatsAndModesLock();

    return pDeviceFormatsAndModes[PinId].WaveFormatsCount;
}

//---------------------------------------------------------------------------
// GetPinSupportedDeviceModes 
//
//  Return mode information for a given pin.
//
//  Return value
//      The number of MODE_AND_DEFAULT_FORMAT items or 0 if none.
//
//  Remarks
//      Supported formats index array follows same order as filter's pin
//      descriptor list.
//
#pragma code_seg()
_Use_decl_annotations_
ULONG CMiniportWaveRT::GetPinSupportedDeviceModes(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) _On_failure_(_Deref_post_null_) MODE_AND_DEFAULT_FORMAT **ppModes)
{
    PMODE_AND_DEFAULT_FORMAT modes;
    ULONG numModes;

    AcquireFormatsAndModesLock();

    ASSERT(m_DeviceFormatsAndModesCount > PinId);
    ASSERT((m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount == 0) == (m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat == NULL));

    modes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat;
    numModes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount;

    if (ppModes != NULL)
    {
        if (numModes > 0)
        {
            *ppModes = modes;
        }
        else
        {
            // ensure that the returned pointer is NULL
            // in the event of failure (SAL annotation above
            // indicates that it must be NULL, and OACR sees a possibility
            // that it might not be).
            *ppModes = NULL;
        }
    }

    ReleaseFormatsAndModesLock();
    return numModes;
}

#pragma code_seg()
BOOL CMiniportWaveRT::IsSystemCapturePin(ULONG nPinId)
{
    AcquireFormatsAndModesLock();

    PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

    ReleaseFormatsAndModesLock();
    return (pinType == SystemCapturePin);
}

#pragma code_seg()
BOOL CMiniportWaveRT::IsSystemRenderPin(ULONG nPinId)
{
    AcquireFormatsAndModesLock();

    PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

    ReleaseFormatsAndModesLock();
    return (pinType == SystemRenderPin);
}

#pragma code_seg()
BOOL CMiniportWaveRT::IsBridgePin(ULONG nPinId)
{
    AcquireFormatsAndModesLock();

    PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

    ReleaseFormatsAndModesLock();
    return (pinType == BridgePin);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::StreamCreated
(
    _In_ ULONG                  _Pin,
    _In_ PCMiniportWaveRTStream _Stream
)
{
    PAGED_CODE();

    PCMiniportWaveRTStream * streams        = NULL;
    ULONG                    count          = 0;
    
    DPF_ENTER(("[CMiniportWaveRT::StreamCreated]"));

    if (IsSystemCapturePin(_Pin))
    {
        ALLOCATE_PIN_INSTANCE_RESOURCES(m_ulSystemAllocated);
        return STATUS_SUCCESS;
    }
    else if (IsSystemRenderPin(_Pin))
    {

        ALLOCATE_PIN_INSTANCE_RESOURCES(m_ulSystemAllocated);
        streams = m_SystemStreams;
        count = m_ulMaxSystemStreams;

    }
    
    //
    // Cache this stream's ptr.
    //
    if (streams != NULL)
    {
        ULONG i = 0;
        for (; i<count; ++i)
        {
            if (streams[i] == NULL)
            {
                streams[i] = _Stream;
                break;
            }
        }
        ASSERT(i != count);
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::StreamClosed
(
    _In_ ULONG                  _Pin,
    _In_ PCMiniportWaveRTStream _Stream
)
{
    PAGED_CODE();

    bool                      updateDrmRights = false;
    PCMiniportWaveRTStream  * streams         = NULL;
    ULONG                     count           = 0;

    DPF_ENTER(("[CMiniportWaveRT::StreamClosed]"));

    if (IsSystemCapturePin(_Pin))
    {
        FREE_PIN_INSTANCE_RESOURCES(m_ulSystemAllocated);
        return STATUS_SUCCESS;
    }
    else if (IsSystemRenderPin(_Pin))
    {

        FREE_PIN_INSTANCE_RESOURCES(m_ulSystemAllocated);
        streams = m_SystemStreams;
        count = m_ulMaxSystemStreams;
        updateDrmRights = true;

    }

    //
    // Cleanup.
    //
    if (streams != NULL)
    {
        ULONG i = 0;
        for (; i<count; ++i)
        {
            if (streams[i] == _Stream)
            {
                streams[i] = NULL;
                break;
            }
        }
        ASSERT(i != count);
    }
    
    //
    // Update mixed drm rights.
    //
    if (updateDrmRights)
    {
        UpdateDrmRights();
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::GetAttributesFromAttributeList
(
    _In_ const KSMULTIPLE_ITEM *_pAttributes,
    _In_ size_t _Size,
    _Out_ GUID* _pSignalProcessingMode
)
/*++

Routine Description:

  Processes attributes list and return known attributes.

Arguments:

  _pAttributes - pointer to KSMULTIPLE_ITEM at head of an attributes list.

  _Size - count of bytes in the buffer pointed to by _pAttributes. The routine
    verifies sufficient buffer size while processing the attributes.

  _pSignalProcessingMode - returns the signal processing mode extracted from
    the attribute list, or AUDIO_SIGNALPROCESSINGMODE_DEFAULT if the attribute
    is not present in the list.

Return Value:

  NT status code.

Remarks

    This function is currently written for a single supported attribute
    (KSATTRIBUTEID_AUDIOSIGNALPROCESSING_MODE). As additional attributes are defined in the future,
    this function should be rewritten to be data driven through tables, etc.

--*/
{
    PAGED_CODE();
    
    DPF_ENTER(("[CMiniportWaveRT::GetAttributesFromAttributeList]"));

    size_t cbRemaining = _Size;

    *_pSignalProcessingMode = AUDIO_SIGNALPROCESSINGMODE_DEFAULT;

    if (cbRemaining < sizeof(KSMULTIPLE_ITEM))
    {
        return STATUS_INVALID_PARAMETER;
    }
    cbRemaining -= sizeof(KSMULTIPLE_ITEM);

    //
    // Extract attributes.
    //
    PKSATTRIBUTE attributeHeader = (PKSATTRIBUTE)(_pAttributes + 1);

    for (ULONG i = 0; i < _pAttributes->Count; i++)
    {
        if (cbRemaining < sizeof(KSATTRIBUTE))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (attributeHeader->Attribute == KSATTRIBUTEID_AUDIOSIGNALPROCESSING_MODE)
        {
            KSATTRIBUTE_AUDIOSIGNALPROCESSING_MODE* signalProcessingModeAttribute;

            if (cbRemaining < sizeof(KSATTRIBUTE_AUDIOSIGNALPROCESSING_MODE))
            {
                return STATUS_INVALID_PARAMETER;
            }

            if (attributeHeader->Size != sizeof(KSATTRIBUTE_AUDIOSIGNALPROCESSING_MODE))
            {
                return STATUS_INVALID_PARAMETER;
            }

            signalProcessingModeAttribute = (KSATTRIBUTE_AUDIOSIGNALPROCESSING_MODE*)attributeHeader;

            // Return mode to caller.
            *_pSignalProcessingMode = signalProcessingModeAttribute->SignalProcessingMode;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        // Adjust pointer and buffer size to next attribute (QWORD aligned)
        ULONG cbAttribute = ((attributeHeader->Size + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);

        attributeHeader = (PKSATTRIBUTE) (((PBYTE)attributeHeader) + cbAttribute);
        cbRemaining -= cbAttribute;
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::IsFormatSupported
(
    _In_ ULONG          _ulPin,
    _In_ BOOLEAN        _bCapture,
    _In_ PKSDATAFORMAT  _pDataFormat
)
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::IsFormatSupported]"));

    NTSTATUS                            ntStatus = STATUS_NO_MATCH;
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE  pPinFormats = NULL;
    ULONG                               cPinFormats = 0;

    UNREFERENCED_PARAMETER(_bCapture);

    if (_ulPin >= m_pMiniportPair->WaveDescriptor->PinCount)
    {
        return STATUS_INVALID_PARAMETER;
    }

    cPinFormats = GetPinSupportedDeviceFormats(_ulPin, &pPinFormats);

    for (UINT iFormat = 0; iFormat < cPinFormats; iFormat++)
    {
        PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pFormat = &pPinFormats[iFormat];
        // KSDATAFORMAT VALIDATION
        if (!IsEqualGUIDAligned(pFormat->DataFormat.MajorFormat, _pDataFormat->MajorFormat)) { continue; }
        if (!IsEqualGUIDAligned(pFormat->DataFormat.SubFormat, _pDataFormat->SubFormat)) { continue; }
        if (!IsEqualGUIDAligned(pFormat->DataFormat.Specifier, _pDataFormat->Specifier)) { continue; }
        if (pFormat->DataFormat.FormatSize < sizeof(KSDATAFORMAT_WAVEFORMATEX)) { continue; }

        // WAVEFORMATEX VALIDATION
        PWAVEFORMATEX pWaveFormat = reinterpret_cast<PWAVEFORMATEX>(_pDataFormat + 1);
        
        if (pWaveFormat->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        {
            if (pWaveFormat->wFormatTag != EXTRACT_WAVEFORMATEX_ID(&(pFormat->WaveFormatExt.SubFormat))) { continue; }
        }
        if (pWaveFormat->nChannels  != pFormat->WaveFormatExt.Format.nChannels) { continue; }
        if (pWaveFormat->nSamplesPerSec != pFormat->WaveFormatExt.Format.nSamplesPerSec) { continue; }
        if (pWaveFormat->nBlockAlign != pFormat->WaveFormatExt.Format.nBlockAlign) { continue; }
        if (pWaveFormat->wBitsPerSample != pFormat->WaveFormatExt.Format.wBitsPerSample) { continue; }
        if (pWaveFormat->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        {
            ntStatus = STATUS_SUCCESS;
            break;
        }

        // WAVEFORMATEXTENSIBLE VALIDATION
        if (pWaveFormat->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) { continue; }

        PWAVEFORMATEXTENSIBLE pWaveFormatExt = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pWaveFormat);
        if (pWaveFormatExt->Samples.wValidBitsPerSample != pFormat->WaveFormatExt.Samples.wValidBitsPerSample) { continue; }
        if (pWaveFormatExt->dwChannelMask != pFormat->WaveFormatExt.dwChannelMask) { continue; }
        if (!IsEqualGUIDAligned(pWaveFormatExt->SubFormat, pFormat->WaveFormatExt.SubFormat)) { continue; }

        ntStatus = STATUS_SUCCESS;
        break;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandlerProposedFormat
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    PKSP_PIN                kspPin                  = NULL;
    PKSDATAFORMAT           pKsFormat               = NULL;
    ULONG                   cbMinSize               = 0;
    NTSTATUS                ntStatus                = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerProposedFormat]"));
    
    // All properties handled by this handler require at least a KSP_PIN descriptor.

    // Verify instance data stores at least KSP_PIN fields beyond KSPPROPERTY.
    if (PropertyRequest->InstanceSize < (sizeof(KSP_PIN) - RTL_SIZEOF_THROUGH_FIELD(KSP_PIN, Property)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Extract property descriptor from property request instance data
    kspPin = CONTAINING_RECORD(PropertyRequest->Instance, KSP_PIN, PinId);

    //
    // This method is valid only on streaming pins.
    //
    if (IsSystemRenderPin(kspPin->PinId) ||
        IsSystemCapturePin(kspPin->PinId))
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (IsBridgePin(kspPin->PinId))
    {
        ntStatus = STATUS_NOT_SUPPORTED;
    }
    else 
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    cbMinSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);
   
    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = PropertyRequest->PropertyItem->Flags;
        
        return PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }

    // Verify value size
    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = cbMinSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < cbMinSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        pKsFormat = (PKSDATAFORMAT)PropertyRequest->Value;
        ntStatus = IsFormatSupported(kspPin->PinId, IsSystemCapturePin(kspPin->PinId) || FALSE, pKsFormat);
        if (!NT_SUCCESS(ntStatus))
        {
            return ntStatus;
        }
    }

    return ntStatus;
} // PropertyHandlerProposedFormat

//=============================================================================
#pragma code_seg()
NTSTATUS
CMiniportWaveRT_EventHandler_PinCapsChange
(
_In_  PPCEVENT_REQUEST EventRequest
)
{
    CMiniportWaveRT* miniport = reinterpret_cast<CMiniportWaveRT*>(EventRequest->MajorTarget);
    return miniport->EventHandler_PinCapsChange(EventRequest);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
CMiniportWaveRT::EventHandler_PinCapsChange
(
_In_  PPCEVENT_REQUEST EventRequest
)
{
    if (*EventRequest->EventItem->Set != KSEVENTSETID_PinCapsChange)
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (EventRequest->Verb)
    {
        // Do we support event handling?!?
    case PCEVENT_VERB_SUPPORT:
        break;
        // We should add the event now!
    case PCEVENT_VERB_ADD:
        // If we have the interface and EventEntry is defined ...
        if (EventRequest->EventEntry)
        {
            switch (EventRequest->EventItem->Id)
            {
                // Add pincaps format change event to support the force sample rate feature
                case KSEVENT_PINCAPS_FORMATCHANGE:
                    m_pPortEvents->AddEventToEventList(EventRequest->EventEntry);
                    break;
                default:
                    return STATUS_INVALID_PARAMETER;
                    break;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
        break;

    case PCEVENT_VERB_REMOVE:
        // We cannot remove the event but we can stop generating the
        // events. However, it also doesn't hurt to always generate them ...
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandlerProposedFormat2
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    PKSP_PIN                kspPin                  = NULL;
    ULONG                   cbMinSize               = 0;
    NTSTATUS                ntStatus                = STATUS_INVALID_PARAMETER;
    ULONG                   numModes                = 0;
    MODE_AND_DEFAULT_FORMAT *modeInfo               = NULL;
    MODE_AND_DEFAULT_FORMAT *modeTemp               = NULL;
    PKSMULTIPLE_ITEM        pKsItemsHeader          = NULL;
    PKSMULTIPLE_ITEM        pKsItemsHeaderOut       = NULL;
    size_t                  cbItemsList             = 0;
    GUID                    signalProcessingMode    = {0};
    BOOLEAN                 bFound                  = FALSE;
    ULONG                   i;

    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerProposedFormat2]"));
    
    // All properties handled by this handler require at least a KSP_PIN descriptor.

    // Verify instance data stores at least KSP_PIN fields beyond KSPPROPERTY.
    if (PropertyRequest->InstanceSize < (sizeof(KSP_PIN) - RTL_SIZEOF_THROUGH_FIELD(KSP_PIN, Property)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Extract property descriptor from property request instance data
    kspPin = CONTAINING_RECORD(PropertyRequest->Instance, KSP_PIN, PinId);

    if (kspPin->PinId >= m_pMiniportPair->WaveDescriptor->PinCount)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // This property is supported only on some streaming pins.
    //
    numModes = GetPinSupportedDeviceModes(kspPin->PinId, &modeInfo);

    ASSERT((modeInfo != NULL && numModes > 0) || (modeInfo == NULL && numModes == 0));

    if (modeInfo == NULL)
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Even for pins that support modes, the pin might not support proposed formats
    //
    bFound = FALSE;
    for (i=0, modeTemp=modeInfo; i<numModes; ++i, ++modeTemp)
    {
        if (modeTemp->DefaultFormat != NULL)
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // The property is generally supported on this pin. Handle basic support request.
    //
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        return PropertyHandler_BasicSupport(PropertyRequest, PropertyRequest->PropertyItem->Flags, VT_ILLEGAL);
    }

    //
    // Get the mode if specified.
    //
    pKsItemsHeader = (PKSMULTIPLE_ITEM)(kspPin + 1);
    cbItemsList = (((PBYTE)PropertyRequest->Instance) + PropertyRequest->InstanceSize) - (PBYTE)pKsItemsHeader;

    ntStatus = GetAttributesFromAttributeList(pKsItemsHeader, cbItemsList, &signalProcessingMode);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    //
    // Get the info associated with this mode.
    //
    bFound = FALSE;
    for (i=0; i<numModes; ++i, ++modeInfo)
    {
        if (modeInfo->Mode == signalProcessingMode)
        {
            bFound = TRUE;
            break;
        }
    }

    // Either the mode isn't supported, or the driver doesn't support a
    // proprosed format for this specific mode.
    if (!bFound || modeInfo->DefaultFormat == NULL)
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Compute output data buffer.
    //
    cbMinSize = modeInfo->DefaultFormat->FormatSize;
    cbMinSize = (cbMinSize + 7) & ~7;

    pKsItemsHeaderOut = (PKSMULTIPLE_ITEM)((PBYTE)PropertyRequest->Value + cbMinSize);

    if (cbItemsList > MAXULONG)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Total # of bytes.
    ntStatus = RtlULongAdd(cbMinSize, (ULONG)cbItemsList, &cbMinSize);
    if (!NT_SUCCESS(ntStatus))
    {
        return STATUS_INVALID_PARAMETER;
    }
        
    // Property not supported.
    if (cbMinSize == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    // Verify value size
    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = cbMinSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < cbMinSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Only GET is supported for this property
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) == 0)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Copy the proposed default format.
    RtlCopyMemory(PropertyRequest->Value, modeInfo->DefaultFormat, modeInfo->DefaultFormat->FormatSize);

    // Copy back the attribute list.
    ASSERT(cbItemsList > 0);
    ((KSDATAFORMAT*)PropertyRequest->Value)->Flags = KSDATAFORMAT_ATTRIBUTES;
    RtlCopyMemory(pKsItemsHeaderOut, pKsItemsHeader, cbItemsList);
    
    PropertyRequest->ValueSize = cbMinSize;

    return STATUS_SUCCESS;
} // PropertyHandlerProposedFormat

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::UpdateDrmRights
(
    void
)
/*++

Routine Description:

  Updates the mixed DrmRights. This is done by creating an array of existing
  content ids and asking DrmPort to create a new contend id with a mixed
  DrmRights structure.
  The new DrmRights structure should be enforced, if everything goes well.

Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::UpdateDrmRights]"));
    
    NTSTATUS        ntStatus                = STATUS_UNSUCCESSFUL;
    ULONG           ulMixDrmContentId       = 0;
    BOOL            fCreatedContentId       = FALSE;
    DRMRIGHTS       MixDrmRights            = {FALSE, 0, FALSE};
    ULONG           ulContentIndex          = 0;
    ULONG*          ulContentIds            = NULL;

    //
    // This function only runs if IID_DrmPort is implemented in Wave port.
    //
    if (!m_pDrmPort)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ulContentIds = new (POOL_FLAG_NON_PAGED, MINWAVERT_POOLTAG) ULONG[m_ulMaxSystemStreams + m_ulMaxOffloadStreams];
    if (!ulContentIds)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create an array of all StreamIds.
    //
    for (ULONG i = 0; i < m_ulMaxSystemStreams; i++)
    {
        if (m_SystemStreams[i])
        {
            ulContentIds[ulContentIndex] = m_SystemStreams[i]->m_ulContentId;
            ulContentIndex++;
        }
    }

    //
    // Create the new contentId.
    //
    if (ulContentIndex)
    {
        ntStatus = 
            m_pDrmPort->CreateContentMixed
            (
                ulContentIds,
                ulContentIndex,
                &ulMixDrmContentId
            );
        
        if (NT_SUCCESS(ntStatus))
        {
            fCreatedContentId = TRUE;
            ntStatus = 
                m_pDrmPort->GetContentRights
                (
                    ulMixDrmContentId, 
                    &MixDrmRights
                );
        }
    }

    //
    // If successful, destroy the old ContentId and update global rights.
    //
    if (NT_SUCCESS(ntStatus))
    {
        m_pDrmPort->DestroyContent(m_ulMixDrmContentId);
        m_ulMixDrmContentId = ulMixDrmContentId;
        RtlCopyMemory(&m_MixDrmRights, &MixDrmRights, sizeof(m_MixDrmRights));

        //
        // At this point the driver should enforce the new DrmRights.
        // The sample driver handles DrmRights per stream basis, and 
        // stops writing the stream to disk, if CopyProtect = TRUE.
        //
    } 

    //
    // Cleanup if failed
    // 
    if (!NT_SUCCESS(ntStatus) && fCreatedContentId)
    {
        m_pDrmPort->DestroyContent(ulMixDrmContentId);
    }

    //
    // Free allocated memory.
    //
    ASSERT(ulContentIds);
    delete [] ulContentIds;
    ulContentIds = NULL;

    return ntStatus;
} // UpdateDrmRights

#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_WaveFilter
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
    CMiniportWaveRT*    pWaveHelper = reinterpret_cast<CMiniportWaveRT*>(PropertyRequest->MajorTarget);

    if (pWaveHelper == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pWaveHelper->AddRef();


    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Pin))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
                ntStatus = pWaveHelper->PropertyHandlerProposedFormat(PropertyRequest);
                break;
            
            case KSPROPERTY_PIN_PROPOSEDATAFORMAT2:
                ntStatus = pWaveHelper->PropertyHandlerProposedFormat2(PropertyRequest);
                break;

            default:
                DPF(D_TERSE, ("[PropertyHandler_WaveFilter: Invalid Device Request]"));
        }
    }

    pWaveHelper->Release();

    return ntStatus;
} // PropertyHandler_WaveFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_GenericPin
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
{
    NTSTATUS                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    CMiniportWaveRT*        pWave = NULL;
    CMiniportWaveRTStream * pStream = NULL;

    PAGED_CODE();

    if (PropertyRequest->MajorTarget == NULL ||
        PropertyRequest->MinorTarget == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    
    //
    // Get a ref to the miniport.
    //
    pWave = MajorTarget_to_Obj(PropertyRequest->MajorTarget);
    pWave->AddRef();

    //
    // Get a ref to the stream.
    //
    pStream = MinorTarget_to_Obj(PropertyRequest->MinorTarget);
    pStream->AddRef();

exit:

    SAFE_RELEASE(pStream);
    SAFE_RELEASE(pWave);
    
    return ntStatus;
}

#pragma code_seg()

