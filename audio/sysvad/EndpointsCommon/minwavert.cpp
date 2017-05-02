/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minwavert.cpp

Abstract:

    Implementation of wavert miniport.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include <limits.h>
#include "ContosoKeywordDetector.h"
#include "SysVadShared.h"
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "IHVPrivatePropertySet.h"
#include "AudioModuleHelper.h"


//=============================================================================
// CMiniportWaveRT
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CreateMiniportWaveRTSYSVAD
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

    CMiniportWaveRT *obj = new (PoolType, MINWAVERT_POOLTAG) CMiniportWaveRT
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
        ExFreePoolWithTag( m_pbMuted, MINWAVERT_POOLTAG );
        m_pbMuted = NULL;
    }

    if (m_plVolumeLevel)
    {
        ExFreePoolWithTag( m_plVolumeLevel, MINWAVERT_POOLTAG );
        m_plVolumeLevel = NULL;
    }

    if (m_plPeakMeter)
    {
        ExFreePoolWithTag( m_plPeakMeter, MINWAVERT_POOLTAG );
        m_plPeakMeter = NULL;
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

    if (m_pPortClsNotifications)
    {
        m_pPortClsNotifications->Release();
        m_pPortClsNotifications = NULL;
    }
    
    if (m_SystemStreams)
    {
        ExFreePoolWithTag( m_SystemStreams, MINWAVERT_POOLTAG );
        m_SystemStreams = NULL;
    }

    if (m_OffloadStreams)
    {
        ExFreePoolWithTag( m_OffloadStreams, MINWAVERT_POOLTAG );
        m_OffloadStreams = NULL;
    }
    
    if (m_LoopbackStreams)
    {
        ExFreePoolWithTag( m_LoopbackStreams, MINWAVERT_POOLTAG );
        m_LoopbackStreams = NULL;
    }

    if (m_pAudioModules)
    {
        FreeStreamAudioModules(m_pAudioModules, GetAudioModuleListCount());
        m_pAudioModules = NULL;
    }

#ifdef SYSVAD_BTH_BYPASS
    if (IsBthHfpDevice())
    {
        SAFE_RELEASE(m_BthHfpDevice);
    }
#endif // SYSVAD_BTH_BYPASS

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

  This sample just validates the # of channels and lets the class handler
  do the rest.

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

  Remarks:

    This sample driver's custom data intersection handler handles all the
    audio endpoints defined in this driver. Some endpoints support mono formats
    while others do not. The handler is written such that it requires an exact
    match in MaximumChannels. This simplifies the handler but requires the pin
    data ranges to include a separate data range for mono formats if the pin
    supports mono formats.

--*/
{
    ULONG                   requiredSize;

    UNREFERENCED_PARAMETER(PinId);
    UNREFERENCED_PARAMETER(ResultantFormat);

    PAGED_CODE();

    if (!IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    requiredSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);

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
    m_ulLoopbackAllocated           = 0;
    m_ulSystemAllocated             = 0;
    m_ulOffloadAllocated            = 0;
    m_dwCaptureAllocatedModes       = 0;
    m_dwBiDiCaptureAllocatedModes   = 0;
    m_dwSystemAllocatedModes        = 0;
    m_SystemStreams                 = NULL;
    m_OffloadStreams                = NULL;
    m_LoopbackStreams               = NULL;
    m_bGfxEnabled                   = FALSE;
    m_pbMuted                       = NULL;
    m_plVolumeLevel                 = NULL;
    m_plPeakMeter                   = NULL;
    m_pMixFormat                    = NULL;
    m_pDeviceFormat                 = NULL;
    m_ulMixDrmContentId             = 0;
    m_LoopbackProtection            = CONSTRICTOR_OPTION_DISABLE;
    RtlZeroMemory(&m_MixDrmRights, sizeof(m_MixDrmRights));

    // 
    // For port notification support.
    //
    if (!NT_SUCCESS(Port_->QueryInterface(IID_IPortClsNotifications, (PVOID *)&m_pPortClsNotifications)))
    {
        m_pPortClsNotifications = NULL;
    }

    //
    // Init all the modules associated to this miniport.
    //
    ULONG cModules = GetAudioModuleListCount();
    if (cModules)
    {
        //
        // Module list size.
        //
        size = cModules * sizeof(AUDIOMODULE);
        m_pAudioModules = (AUDIOMODULE *)ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
        if (m_pAudioModules == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(m_pAudioModules, size);

        for (ULONG i=0; i<cModules; ++i)
        {
            PAUDIOMODULE_DESCRIPTOR moduleDesc = &m_pMiniportPair->ModuleList[i];

            //
            // Init run-time module element.
            //
            m_pAudioModules[i].Descriptor = moduleDesc;
            m_pAudioModules[i].Context    = NULL;
            m_pAudioModules[i].InstanceId = moduleDesc->InstanceId;
            m_pAudioModules[i].Enabled    = TRUE;
            //
            // Module context size.
            //
            size = moduleDesc->ContextSize;
            if (size)
            {
                m_pAudioModules[i].Context = 
                    ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
                
                if (m_pAudioModules[i].Context == NULL)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                RtlZeroMemory(m_pAudioModules[i].Context, size);
            }

            
            if (moduleDesc->InitClass)
            {
                KSAUDIOMODULE_NOTIFICATION  NotificationHeader;
                NotificationHeader.ProviderId.DeviceId = *GetAudioModuleNotificationDeviceId();
                NotificationHeader.ProviderId.ClassId = *moduleDesc->ClassId;
                NotificationHeader.ProviderId.InstanceId = moduleDesc->InstanceId;
                
                ntStatus = moduleDesc->InitClass(moduleDesc,
                                                 m_pAudioModules[i].Context,
                                                 size,
                                                 &NotificationHeader,
                                                 m_pPortClsNotifications);
                if (!NT_SUCCESS(ntStatus))
                {
                    ASSERT(FALSE);
                    return ntStatus;
                }
            }
        }
    }

    //
    // Init the audio-engine used by the render devices.
    //
    if (IsRenderDevice())
    {
        // Basic validation
        if (m_ulMaxSystemStreams == 0 ||
            m_ulMaxLoopbackStreams == 0)
        {
            return STATUS_INVALID_DEVICE_STATE;
        }
            
        // System streams.
        size = sizeof(PCMiniportWaveRTStream) * m_ulMaxSystemStreams;
        m_SystemStreams = (PCMiniportWaveRTStream *)ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
        if (m_SystemStreams == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(m_SystemStreams, size);

        // Loopback streams.
        size = sizeof(PCMiniportWaveRTStream) * m_ulMaxLoopbackStreams;
        m_LoopbackStreams = (PCMiniportWaveRTStream *)ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
        if (m_LoopbackStreams == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(m_LoopbackStreams, size);

        // Basic validation
        if (IsOffloadSupported())
        {
            PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pDeviceFormats;
            ULONG numDeviceFormats;

            if (m_ulMaxOffloadStreams == 0)
            {
                return STATUS_INVALID_DEVICE_STATE;
            }
            
            // Offload streams.
            size = sizeof(PCMiniportWaveRTStream) * m_ulMaxOffloadStreams;
            m_OffloadStreams = (PCMiniportWaveRTStream *)ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
            if (m_OffloadStreams == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(m_OffloadStreams, size);

            // Formats. 
            m_pDeviceFormat = (PKSDATAFORMAT_WAVEFORMATEXTENSIBLE)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE), MINWAVERT_POOLTAG);
            if (m_pDeviceFormat == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            numDeviceFormats = GetAudioEngineSupportedDeviceFormats(&pDeviceFormats);

            if (numDeviceFormats < 1)
            {
                return STATUS_UNSUCCESSFUL;
            }
            RtlCopyMemory((PVOID)(m_pDeviceFormat), (PVOID)(pDeviceFormats), sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE));

            m_pMixFormat = (PKSDATAFORMAT_WAVEFORMATEXTENSIBLE)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE), MINWAVERT_POOLTAG);
            if (m_pMixFormat == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            m_pMixFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);
            m_pMixFormat->DataFormat.Flags = 0;
            m_pMixFormat->DataFormat.Reserved = 0;
            m_pMixFormat->DataFormat.SampleSize = 0;
            m_pMixFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            m_pMixFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            m_pMixFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            m_pMixFormat->WaveFormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            m_pMixFormat->WaveFormatExt.Format.nChannels = 2;
            m_pMixFormat->WaveFormatExt.Format.nSamplesPerSec = 48000;
            m_pMixFormat->WaveFormatExt.Format.nBlockAlign = 4;
            m_pMixFormat->WaveFormatExt.Format.nAvgBytesPerSec = 192000;
            m_pMixFormat->WaveFormatExt.Format.wBitsPerSample = 16;
            m_pMixFormat->WaveFormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
            m_pMixFormat->WaveFormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            m_pMixFormat->WaveFormatExt.Samples.wValidBitsPerSample = 16;
            m_pMixFormat->WaveFormatExt.dwChannelMask = KSAUDIO_SPEAKER_STEREO;

            m_bGfxEnabled = FALSE;

            m_pbMuted = (PBOOL)ExAllocatePoolWithTag(NonPagedPoolNx, m_DeviceMaxChannels * sizeof(BOOL), MINWAVERT_POOLTAG);
            if (m_pbMuted == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(m_pbMuted, m_DeviceMaxChannels * sizeof(BOOL));

            m_plVolumeLevel = (PLONG)ExAllocatePoolWithTag(NonPagedPoolNx, m_DeviceMaxChannels * sizeof(LONG), MINWAVERT_POOLTAG);
            if (m_plVolumeLevel == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(m_plVolumeLevel, m_DeviceMaxChannels * sizeof(LONG));

            m_plPeakMeter = (PLONG)ExAllocatePoolWithTag(NonPagedPoolNx, m_DeviceMaxChannels * sizeof(LONG), MINWAVERT_POOLTAG);
            if (m_plPeakMeter == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(m_plPeakMeter, m_DeviceMaxChannels * sizeof(LONG));
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
        ntStatus = ValidateStreamCreate(Pin, Capture, signalProcessingMode);
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
        stream = new (NonPagedPoolNx, MINWAVERT_POOLTAG) 
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
    // In this sample, IMiniportAudioEngineNode is supported only for offloading endpoints.
    // at thso moment, offload could only be enabled for  render endpoints not capture.
    // Incorrectly support IMiniportAudioEngineNode interface by the miniport without underlying 
    // HWAudioEngine node will cause miniport::Init to fail.
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportAudioEngineNode) && IsOffloadSupported())
    {
         *Object = (PVOID)(IMiniportAudioEngineNode*)this;
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
    _In_    BOOLEAN _Capture,
    _In_    GUID    _SignalProcessingMode
)
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::ValidateStreamCreate]"));

    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;

    if (_Capture)
    {
        if (IsLoopbackPin(_Pin))
        {
            if (m_ulLoopbackAllocated < m_ulMaxLoopbackStreams)
            {
                ntStatus = STATUS_SUCCESS;
            }
            else 
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else if (IsSystemCapturePin(_Pin))
        {
            VERIFY_MODE_RESOURCES_AVAILABLE(m_dwCaptureAllocatedModes, _SignalProcessingMode, ntStatus)
        }
        else if (IsCellularBiDiCapturePin(_Pin))
        {
            VERIFY_MODE_RESOURCES_AVAILABLE(m_dwBiDiCaptureAllocatedModes, _SignalProcessingMode, ntStatus)
        }
        else if (m_DeviceFormatsAndModes[_Pin].PinType == PINTYPE::KeywordCapturePin)
        {
            ntStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        if (IsSystemRenderPin(_Pin))
        {
            VERIFY_MODE_RESOURCES_AVAILABLE(m_dwSystemAllocatedModes, _SignalProcessingMode, ntStatus)
        }
        else if (IsOffloadPin(_Pin))
        {
            if (m_ulOffloadAllocated < m_ulMaxOffloadStreams)
            {
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return ntStatus;
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
    
    DPF_ENTER(("[CMiniportWaveRT::StreamOpened]"));
    
    if (IsSystemCapturePin(_Pin))
    {
        ALLOCATE_MODE_RESOURCES(m_dwCaptureAllocatedModes, _Stream->GetSignalProcessingMode())
        return STATUS_SUCCESS;
    }
    if (IsCellularBiDiCapturePin(_Pin))
    {
        ALLOCATE_MODE_RESOURCES(m_dwBiDiCaptureAllocatedModes, _Stream->GetSignalProcessingMode())
        return STATUS_SUCCESS;
    }
    else if (IsLoopbackPin(_Pin))
    {
        m_ulLoopbackAllocated++;
        streams = m_LoopbackStreams;
        count = m_ulMaxLoopbackStreams;
        _Stream->m_SaveData.Disable(m_MixDrmRights.CopyProtect);
    }
    else if (IsSystemRenderPin(_Pin))
    {
        ALLOCATE_MODE_RESOURCES(m_dwSystemAllocatedModes, _Stream->GetSignalProcessingMode())
        m_ulSystemAllocated++;
        streams = m_SystemStreams;
        count = m_ulMaxSystemStreams;
    }
    else if (IsOffloadPin(_Pin))
    {
        m_ulOffloadAllocated++;
        streams = m_OffloadStreams;
        count = m_ulMaxOffloadStreams;
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
        FREE_MODE_RESOURCES(m_dwCaptureAllocatedModes, _Stream->GetSignalProcessingMode())
        return STATUS_SUCCESS;
    }
    if (IsCellularBiDiCapturePin(_Pin))
    {
        FREE_MODE_RESOURCES(m_dwBiDiCaptureAllocatedModes, _Stream->GetSignalProcessingMode())
        return STATUS_SUCCESS;
    }
    else if (IsLoopbackPin(_Pin))
    {
        m_ulLoopbackAllocated--;
        streams = m_LoopbackStreams;
        count = m_ulMaxLoopbackStreams;
    }
    else if (IsSystemRenderPin(_Pin))
    {
        FREE_MODE_RESOURCES(m_dwSystemAllocatedModes, _Stream->GetSignalProcessingMode())
        m_ulSystemAllocated--;
        streams = m_SystemStreams;
        count = m_ulMaxSystemStreams;
        updateDrmRights = true;
    }
    else if (IsOffloadPin(_Pin))
    {
        m_ulOffloadAllocated--;
        streams = m_OffloadStreams;
        count = m_ulMaxOffloadStreams;
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
        IsLoopbackPin(kspPin->PinId) ||
        IsOffloadPin(kspPin->PinId) ||
        IsSystemCapturePin(kspPin->PinId)||
        IsCellularBiDiCapturePin(kspPin->PinId))
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

    // Only SET is supported for this property
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) == 0)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pKsFormat = (PKSDATAFORMAT)PropertyRequest->Value;
    ntStatus = IsFormatSupported(kspPin->PinId, 
                                 IsSystemCapturePin(kspPin->PinId) || IsCellularBiDiCapturePin(kspPin->PinId) || 
                                    IsLoopbackPin(kspPin->PinId),
                                 pKsFormat);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    //
    // Make sure there are enough resources to handle a new pin creation with
    // this format.
    //
    if (IsOffloadPin(kspPin->PinId))
    {
        ntStatus = ValidateStreamCreate(kspPin->PinId, FALSE, AUDIO_SIGNALPROCESSINGMODE_DEFAULT);
    }

    return ntStatus;
} // PropertyHandlerProposedFormat

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandlerModulesListRequest
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    // This specific APO->driver communication example is mainly added to show how this communication is done.
    // The module list only lives on the wave filter and it can have modules that are for all pins and some that 
    // are only on specific pins.

    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerModulesListRequest]"));

    return AudioModule_GenericHandler_ModulesListRequest(
                PropertyRequest,
                GetAudioModuleList(),
                GetAudioModuleListCount());
} // PropertyHandlerModulesListRequest

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandlerModuleCommand
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerModuleCommand]"));

    return AudioModule_GenericHandler_ModuleCommand(
                PropertyRequest,
                GetAudioModuleList(),
                GetAudioModuleListCount());
} // PropertyHandlerModuleCommand

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandlerModuleNotificationDeviceId
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerModuleNotificationDeviceId]"));

    return AudioModule_GenericHandler_ModuleNotificationDeviceId(
                PropertyRequest,
                GetAudioModuleNotificationDeviceId());
} // PropertyHandlerModuleNotificationDeviceId


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
CMiniportWaveRT::PropertyHandlerEffectListRequest
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
{
    GUID StreamEffectList[] =
    {
        AUDIO_EFFECT_TYPE_LOUDNESS_EQUALIZER,
        AUDIO_EFFECT_TYPE_VIRTUAL_SURROUND
    };

    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::PropertyHandlerEffectListRequest]"));

    // This specific APO->driver communication example is mainly added to show to this communication is done.
    // It skips the pin id validation and returns pin specific answers to the caller, which a real miniport 
    // audio driver probably needs to take care of.

    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        return PropertyHandler_BasicSupport(PropertyRequest, PropertyRequest->PropertyItem->Flags, VT_ILLEGAL);
    }

    // Verify instance data stores at least KSP_PIN fields beyond KSPPROPERTY.
    if (PropertyRequest->InstanceSize < (sizeof(KSP_PIN) - RTL_SIZEOF_THROUGH_FIELD(KSP_PIN, Property)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        PKSMULTIPLE_ITEM ksMultipleItem;
        ULONG ulEffectsCount = ARRAYSIZE(StreamEffectList);
        ULONG cbMinSize;
        LPGUID pEffectGuids = NULL;

        // Compute min value size requirements    
        cbMinSize = sizeof(KSMULTIPLE_ITEM) + ulEffectsCount * sizeof(GUID);

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
        // Value is a KSMULTIPLE_ITEM followed by list of GUIDs.
        ksMultipleItem = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
        pEffectGuids = (LPGUID)(ksMultipleItem + 1);

        // Copy effect guid 
        RtlCopyMemory(pEffectGuids, StreamEffectList, ulEffectsCount * sizeof(GUID));

        // Miniport filled in the list of GUIDs. Fill in the KSMULTIPLE_ITEM header.
        ksMultipleItem->Size = sizeof(KSMULTIPLE_ITEM) + ulEffectsCount * sizeof(GUID);
        ksMultipleItem->Count = ulEffectsCount;

        PropertyRequest->ValueSize = ksMultipleItem->Size;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_DEVICE_REQUEST;

} // PropertyHandlerEffectListRequest

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

    ulContentIds = new (NonPagedPoolNx, MINWAVERT_POOLTAG) ULONG[m_ulMaxSystemStreams + m_ulMaxOffloadStreams];
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

    for (ULONG i = 0; i < m_ulMaxOffloadStreams; i++)
    {
        ASSERT(IsOffloadSupported());
        
        if (m_OffloadStreams[i])
        {
            ulContentIds[ulContentIndex] = m_OffloadStreams[i]->m_ulContentId;
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

        //
        // If DigitalOutputDisable or CopyProtect is true, enable HDCP
        // 
        if (m_DeviceType == eHdmiRenderDevice &&
            (m_MixDrmRights.DigitalOutputDisable || m_MixDrmRights.CopyProtect))
        {
            // Enable HDCP here.
        }
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

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::AllocStreamAudioModules
(
    _In_  const GUID *      SignalProcessingMode,
    _Out_ AUDIOMODULE **    ppAudioModules,
    _Out_ ULONG *           pAudioModuleCount
)
{
    NTSTATUS        ntStatus = STATUS_INVALID_DEVICE_STATE;
    AUDIOMODULE *   pAudioModules = NULL;
    ULONG           cModules = 0;    
    ULONG           i, j;
    size_t          size;

    PAGED_CODE();
    
    //
    // Init out parameters.
    //
    *ppAudioModules = NULL;
    *pAudioModuleCount = 0;

    //
    // Nothing to do if there are no modules.
    //
    if (m_pAudioModules == NULL)
    {
        ntStatus = STATUS_SUCCESS;
        goto exit;
    }
    
    //
    // Find the # of modules associated with this stream.
    //
    for (i=0; i<GetAudioModuleListCount(); ++i)
    {
        const AUDIOMODULE_DESCRIPTOR * moduleDesc = m_pAudioModules[i].Descriptor;
        
        if (IsEqualGUIDAligned(*moduleDesc->ProcessingMode, *SignalProcessingMode) ||
            IsEqualGUIDAligned(*moduleDesc->ProcessingMode, NULL_GUID))
        {
            cModules++;
        }
    }

    //
    // All done if module count is zero.
    //
    if (cModules == 0)
    {
        ntStatus = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Alloc modules infrastructure.
    //
    size = cModules * sizeof(AUDIOMODULE);
#pragma prefast(suppress:__WARNING_MEMORY_LEAK,"No leaking, stream obj dtor calls FreeStreamAudioModules")
    pAudioModules = (AUDIOMODULE *)ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
    if (pAudioModules == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    RtlZeroMemory(pAudioModules, size);
    
    for (i=0, j=0; i<GetAudioModuleListCount() && j<cModules; ++i)
    {
        const AUDIOMODULE_DESCRIPTOR * moduleDesc = m_pAudioModules[i].Descriptor;
    
        if (IsEqualGUIDAligned(*moduleDesc->ProcessingMode, *SignalProcessingMode) ||
            IsEqualGUIDAligned(*moduleDesc->ProcessingMode, NULL_GUID))
        {
            ULONG CfgInstanceId;
            
            //
            // Init run-time module element.
            //
            pAudioModules[j].Descriptor = moduleDesc;
            pAudioModules[j].Context    = NULL;

            //
            // Create a unique InstanceId for this module instance.
            // This sample uses 24bits index which wraps around after 16M 
            // module instances for a specific class ID/Class config id. 
            // A real driver should reuse instance ids of deleted module
            // instances, i.e., the driver should use a mapping between 
            // index <--> module info.
            //
            CfgInstanceId = InterlockedIncrement((LONG*)&m_pAudioModules[i].NextCfgInstanceId);
            
            pAudioModules[j].InstanceId = 
                AUDIOMODULE_INSTANCE_ID(AUDIOMODULE_GET_CLASSCFGID(m_pAudioModules[i].InstanceId),
                                        CfgInstanceId);
                                        
            pAudioModules[j].Enabled = m_pAudioModules[i].Enabled;
        
            //
            // Alloc context for module instance.
            //
            size = moduleDesc->ContextSize;
            if (size)
            {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK,"No leaking, stream obj dtor calls FreeStreamAudioModules")
                pAudioModules[j].Context = 
                    ExAllocatePoolWithTag(NonPagedPoolNx, size, MINWAVERT_POOLTAG);
                
                if (pAudioModules[j].Context == NULL)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
                RtlZeroMemory(pAudioModules[j].Context, size);
            }
        
            //
            // Init this module instance.
            //
            if (moduleDesc->InitInstance)
            {
                ntStatus = moduleDesc->InitInstance(moduleDesc,
                                                 m_pAudioModules[i].Context,
                                                 pAudioModules[j].Context,
                                                 size,
                                                 pAudioModules[j].InstanceId);
                if (!NT_SUCCESS(ntStatus))
                {
                    ASSERT(FALSE);
                    goto exit;
                }
            }

            //
            // Update stream module array index.
            //
            j++;
        }
    }

    //
    // Return the list of modules.
    //
    *ppAudioModules = pAudioModules;
    *pAudioModuleCount = cModules;

    ntStatus = STATUS_SUCCESS;
    
exit:

    if (!NT_SUCCESS(ntStatus))
    {
        if (pAudioModules != NULL)
        {
            FreeStreamAudioModules(pAudioModules, cModules);
            pAudioModules = NULL;
            cModules = 0;
        }
    }
    
    return ntStatus;
}

#pragma code_seg("PAGE")
VOID
CMiniportWaveRT::FreeStreamAudioModules
(
    _In_ AUDIOMODULE *     pAudioModules,
    _In_ ULONG             AudioModuleCount
)
{
    PAGED_CODE();
    
    if (pAudioModules != NULL)
    {
        ASSERT(AudioModuleCount);
        
        for (ULONG i=0; i<AudioModuleCount; ++i)
        {
            if (pAudioModules[i].Context)
            {
                if (pAudioModules[i].Descriptor->Cleanup)
                {
                    pAudioModules[i].Descriptor->Cleanup(pAudioModules[i].Context);
                }
                
                ExFreePoolWithTag(pAudioModules[i].Context, MINWAVERT_POOLTAG);
                pAudioModules[i].Context = NULL;
            }
        }

        ExFreePoolWithTag(pAudioModules, MINWAVERT_POOLTAG);
    }
}

//=============================================================================
#pragma code_seg("PAGE")

DEFINE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorSupportedPatterns)
{
    CONTOSO_SUPPORTEDPATTERNSVALUE *value;

    PAGED_CODE();

    NT_ASSERT(PropertyRequest->ValueSize >= sizeof(*value));

    // Does this filter support a sound detector?
    if ((m_DeviceFlags & ENDPOINT_SOUNDDETECTOR_SUPPORTED) == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    value = (CONTOSO_SUPPORTEDPATTERNSVALUE*)PropertyRequest->Value;

    RtlZeroMemory(value, sizeof(*value));

    value->MultipleItem.Size = sizeof(*value);
    value->MultipleItem.Count = 1;
    value->PatternType[0] = CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER;

    PropertyRequest->ValueSize = sizeof(*value);

    return STATUS_SUCCESS;
}

DEFINE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorPatterns)
{
    KSMULTIPLE_ITEM *itemsHeader;
    SOUNDDETECTOR_PATTERNHEADER *patternHeader;
    CONTOSO_KEYWORDCONFIGURATION *pattern;
    ULONG cbRemaining;                          // Tracks bytes remaining in property value

    PAGED_CODE();

    cbRemaining = PropertyRequest->ValueSize;

    // The SYSVADPROPERTY_ITEM for this property ensures the value size is at
    // least sizeof KSMULTIPLE_ITEM.
    if (cbRemaining < sizeof(KSMULTIPLE_ITEM))
    {
        return STATUS_INVALID_PARAMETER;
    }

    itemsHeader = (KSMULTIPLE_ITEM*)PropertyRequest->Value;

    // Verify property value is large enough to include the items
    if (itemsHeader->Size > cbRemaining)
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    // No items so clear the configuration.
    if (itemsHeader->Count == 0)
    {
        m_KeywordDetector.ResetDetector();
        return STATUS_SUCCESS;
    }

    // This sample supports only 1 pattern type.
    if (itemsHeader->Count > 1)
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_NOT_SUPPORTED;
    }

    // Bytes remaining after the items header
    cbRemaining = itemsHeader->Size - sizeof(*itemsHeader);

    // Verify the property value is large enough to include the pattern header.
    if (cbRemaining < sizeof(SOUNDDETECTOR_PATTERNHEADER))
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    patternHeader = (SOUNDDETECTOR_PATTERNHEADER*)(itemsHeader + 1);

    // Verify the pattern type is supported.
    if (patternHeader->PatternType != CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER)
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_NOT_SUPPORTED;
    }

    // Verify the property value is large enough for the pattern.
    if (cbRemaining < patternHeader->Size)
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    // Verify the pattern is large enough.
    if (patternHeader->Size != sizeof(CONTOSO_KEYWORDCONFIGURATION))
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    pattern = (CONTOSO_KEYWORDCONFIGURATION*)(patternHeader);

    // Program the hardware.
    m_KeywordDetector.DownloadDetectorData(pattern->ContosoDetectorConfigurationData);

    return STATUS_SUCCESS;
}

DEFINE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorArmed)
{
    PAGED_CODE();

    // The SYSVADPROPERTY_ITEM for this property ensures the value size is at
    // least sizeof BOOL.
    NT_ASSERT(PropertyRequest->ValueSize >= sizeof(BOOL));

    RtlZeroMemory(PropertyRequest->Value, PropertyRequest->ValueSize);
    *(BOOL*)PropertyRequest->Value = m_KeywordDetector.GetArmed();

    return STATUS_SUCCESS;
}

DEFINE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Set_SoundDetectorArmed)
{
    NTSTATUS ntStatus;
    BOOL armed;

    PAGED_CODE();

    // The SYSVADPROPERTY_ITEM for this property ensures the value size is at
    // least sizeof BOOL.
    NT_ASSERT(PropertyRequest->ValueSize >= sizeof(BOOL));

    armed = ((*(BOOL*)PropertyRequest->Value) != 0);

    ntStatus = m_KeywordDetector.SetArmed(armed);

    if (NT_SUCCESS(ntStatus) && armed)
    {
        // FUTURE-2014/10/20 For now immediately signal a detection as soon as
        // it is armed, but later, find a better way to demonstrate this from
        // within CKeywordDetector.
        m_pPortEvents->GenerateEventList(const_cast<GUID*>(&KSEVENTSETID_SoundDetector), KSEVENT_SOUNDDETECTOR_MATCHDETECTED, FALSE, 0, FALSE, 0);
        m_KeywordDetector.SetArmed(FALSE);
    }

    return ntStatus;
}

DEFINE_CLASSPROPERTYHANDLER(CMiniportWaveRT, Get_SoundDetectorMatchResult)
{
    CONTOSO_KEYWORDDETECTIONRESULT *value;
    
    PAGED_CODE();
    
    if (PropertyRequest->ValueSize < sizeof(*value))
    {
        return STATUS_INVALID_PARAMETER;
    }

    value = (CONTOSO_KEYWORDDETECTIONRESULT *)PropertyRequest->Value;

    RtlZeroMemory(value, sizeof(*value));

    value->Header.Size = sizeof(CONTOSO_KEYWORDDETECTIONRESULT);
    value->Header.PatternType = CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER;
    value->ContosoDetectorResultData = m_KeywordDetector.GetDetectorData();

    PropertyRequest->ValueSize = sizeof(*value);
    
    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CMiniportWaveRT_EventHandler_SoundDetectorMatchDetected
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    CMiniportWaveRT* miniport = reinterpret_cast<CMiniportWaveRT*>(EventRequest->MajorTarget);
    return miniport->EventHandler_SoundDetectorMatchDetected(EventRequest);
}

#pragma code_seg()
NTSTATUS CMiniportWaveRT::EventHandler_SoundDetectorMatchDetected
(
    _In_  PPCEVENT_REQUEST EventRequest
)
{
    if (EventRequest->Verb == PCEVENT_VERB_ADD)
    {
        _IRQL_limited_to_(PASSIVE_LEVEL);
        m_pPortEvents->AddEventToEventList(EventRequest->EventEntry);
    }
    return STATUS_SUCCESS;
}

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

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_SysVAD))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_SYSVAD_DEFAULTSTREAMEFFECTS:
                ntStatus = pWaveHelper->PropertyHandlerEffectListRequest(PropertyRequest);
                break;
            default:
                DPF(D_TERSE, ("[PropertyHandler_WaveFilter: Invalid Device Request]"));
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Pin))
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
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_AudioModule))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
        case KSPROPERTY_AUDIOMODULE_DESCRIPTORS:
            ntStatus = pWaveHelper->PropertyHandlerModulesListRequest(PropertyRequest);
            break;
        case KSPROPERTY_AUDIOMODULE_COMMAND:
            ntStatus = pWaveHelper->PropertyHandlerModuleCommand(PropertyRequest);
            break;
        case KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID:
            ntStatus = pWaveHelper->PropertyHandlerModuleNotificationDeviceId(PropertyRequest);
            break;

        default:
            DPF(D_TERSE, ("[PropertyHandler_WaveFilter: Invalid Device Request]"));
        }
    }
    else if ((pWaveHelper->m_DeviceType == eHdmiRenderDevice || 
              pWaveHelper->m_DeviceType == eCellularDevice || 
              pWaveHelper->m_DeviceType == eHandsetSpeakerDevice) &&
             IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Audio))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_AUDIO_VOLUMELEVEL:
                ntStatus = PropertyHandler_Volume(
                                    pWaveHelper->m_pAdapterCommon,
                                    PropertyRequest,
                                    pWaveHelper->m_DeviceMaxChannels);
                break;
            
            case KSPROPERTY_AUDIO_MUTE:
                ntStatus = PropertyHandler_Mute(
                                    pWaveHelper->m_pAdapterCommon,
                                    PropertyRequest,
                                    pWaveHelper->m_DeviceMaxChannels);
                break;
            
            case KSPROPERTY_AUDIO_PEAKMETER2:
                ntStatus = PropertyHandler_PeakMeter2(
                                    pWaveHelper->m_pAdapterCommon,
                                    PropertyRequest,
                                    pWaveHelper->m_DeviceMaxChannels);
                break;
            
            case KSPROPERTY_AUDIO_CPU_RESOURCES:
                ntStatus = PropertyHandler_CpuResources(PropertyRequest);
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
PropertyHandler_OffloadPin
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_OffloadPin))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            //KSPROPERTY_OFFLOAD_PIN_VERIFY_STREAM_OBJECT_POINTER

            case KSPROPERTY_OFFLOAD_PIN_GET_STREAM_OBJECT_POINTER:
            {
                if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    ULONG cbMinSize = sizeof(ULONG_PTR);

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
                        ULONG_PTR *streamObjectPtr = static_cast<ULONG_PTR*>(PropertyRequest->Value);
                        *streamObjectPtr = (ULONG_PTR)(PropertyRequest->MinorTarget);
                        ntStatus = STATUS_SUCCESS;
                    }
                }

                else
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
            break;
            case KSPROPERTY_OFFLOAD_PIN_VERIFY_STREAM_OBJECT_POINTER:
            {
                if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ULONG cbMinSize = sizeof(ULONG_PTR);
                    if (PropertyRequest->InstanceSize < cbMinSize)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ULONG_PTR *streamObjectPtr = static_cast<ULONG_PTR*>(PropertyRequest->Instance);
                        if (*streamObjectPtr == (ULONG_PTR)(PropertyRequest->MinorTarget))
                        {
                            ntStatus = STATUS_SUCCESS;
                        }
                        else
                        {
                            ntStatus = STATUS_UNSUCCESSFUL;
                        }
                    }
                }
                else
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
            break;
            default:
                DPF(D_TERSE, ("[PropertyHandler_OffloadPin: Invalid Request]"));
        }
    }
    return ntStatus;
}

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

    //
    // Invoke appropriate handle.
    //
    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_AudioModule))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
        case KSPROPERTY_AUDIOMODULE_DESCRIPTORS:
            ntStatus = pStream->PropertyHandlerModulesListRequest(PropertyRequest);
            break;
        case KSPROPERTY_AUDIOMODULE_COMMAND:
            ntStatus = pStream->PropertyHandlerModuleCommand(PropertyRequest);
            break;
        case KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID:
            // Filter handles this prop.
            ntStatus = pWave->PropertyHandlerModuleNotificationDeviceId(PropertyRequest);
            break;

        default:
            DPF(D_TERSE, ("[PropertyHandler_GenericPin: Invalid Device Request]"));
        }
    }

exit:

    SAFE_RELEASE(pStream);
    SAFE_RELEASE(pWave);
    
    return ntStatus;
}

// ISSUE-2014/10/20 Add synchronization mechanism throughout this class
// ISSUE-2014/10/20 Add comment headers and commenting throughout
#pragma code_seg("PAGE")
CKeywordDetector::CKeywordDetector()
    :
    m_qpcStartCapture(0),
    m_nLastQueuedPacket(-1),
    m_SoundDetectorArmed(FALSE),
    m_SoundDetectorData(0)
{
    PAGED_CODE();

    // Initialize our pool of packets and the list structures
    KeInitializeSpinLock(&PacketPoolSpinLock);
    KeInitializeSpinLock(&PacketFifoSpinLock);
    ResetFifo();
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::ResetDetector()
{
    PAGED_CODE();

    m_SoundDetectorData = 0;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::DownloadDetectorData(_In_ LONGLONG Data)
{
    PAGED_CODE();

    m_SoundDetectorData = Data;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
LONGLONG CKeywordDetector::GetDetectorData()
{
    PAGED_CODE();

    return m_SoundDetectorData;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::ResetFifo()
{
    PAGED_CODE();

    m_qpcStartCapture = 0;
    m_nLastQueuedPacket = (-1);
    InitializeListHead(&PacketPoolHead);
    InitializeListHead(&PacketFifoHead);

    for (int i = 0; i < ARRAYSIZE(PacketPool); i++)
    {
        InsertTailList(&PacketPoolHead, &PacketPool[i].ListEntry);
    }
    return;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::SetArmed(_In_ BOOL Arm)
{
    PAGED_CODE();

    BOOL previousArming = m_SoundDetectorArmed;

    m_SoundDetectorArmed = Arm;

    if (Arm && !previousArming && m_qpcStartCapture == 0)
    {
        StartBufferingStream();
    }
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOL CKeywordDetector::GetArmed()
{
    PAGED_CODE();

    return m_SoundDetectorArmed;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::Run()
{
    PAGED_CODE();

    if (m_qpcStartCapture == 0)
    {
        StartBufferingStream();
    }
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::Stop()
{
    PAGED_CODE();

    ResetFifo();
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::StartBufferingStream()
{
    LARGE_INTEGER qpc;
    LARGE_INTEGER qpcFrequency;

    PAGED_CODE();

    NT_ASSERT(m_qpcStartCapture == 0);
    NT_ASSERT(IsListEmpty(&PacketFifoHead));

    qpc = KeQueryPerformanceCounter(&qpcFrequency);
    m_qpcStartCapture = qpc.QuadPart;

    return;
}

#pragma code_seg()
_IRQL_requires_min_(DISPATCH_LEVEL)
VOID CKeywordDetector::DpcRoutine(_In_ LONGLONG PerformanceCounter, _In_ LONGLONG PerformanceFrequency)
{
    LONGLONG currentPacket;
    LONGLONG packetsToQueue;

    if (m_qpcStartCapture <= 0)
    {
        return;
    }

    currentPacket = (PerformanceCounter - m_qpcStartCapture) * (SamplesPerSecond / SamplesPerPacket) / PerformanceFrequency;
    packetsToQueue = currentPacket - m_nLastQueuedPacket;

    while (packetsToQueue > 0)
    {
        LIST_ENTRY*     packetListEntry;
        PACKET_ENTRY*   packetEntry;
        LONGLONG*       signature;

        do
        {
            packetListEntry = ExInterlockedRemoveHeadList(&PacketPoolHead, &PacketPoolSpinLock);
            if (packetListEntry != NULL) break;

            // Pool is empty, no room to buffer more, an overrun is occurring. Drop and reuse the
            // oldest packet from head of fifo.

            // Since the pool is empty, the fifo should be full. However, although unlikely, the
            // driver might empty the fifo before this routine removes a packet. In that case, the
            // pool should have packets available again. Therefore this is a retry loop.
            packetListEntry = ExInterlockedRemoveHeadList(&PacketFifoHead, &PacketFifoSpinLock);
            if (packetListEntry != NULL) break;
        } while (TRUE);

        packetEntry = CONTAINING_RECORD(packetListEntry, PACKET_ENTRY, ListEntry);

        packetEntry->PacketNumber = ++m_nLastQueuedPacket;
        packetEntry->QpcWhenSampled = m_qpcStartCapture + (packetEntry->PacketNumber * PerformanceFrequency * SamplesPerPacket / SamplesPerSecond);

        RtlZeroMemory(&packetEntry->Samples[0], sizeof(packetEntry->Samples));

        // For test purposes, embed the packet number and sample time into the audio data
        signature = (LONGLONG*)(&packetEntry->Samples[0]);

        signature[0] = packetEntry->PacketNumber;
        signature[1] = packetEntry->QpcWhenSampled;

        ExInterlockedInsertTailList(&PacketFifoHead, packetListEntry, &PacketFifoSpinLock);

        packetsToQueue -= 1;
    }
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::GetReadPacket
(
    _In_ ULONG PacketsPerWaveRtBuffer,
    _In_ ULONG WaveRtBufferSize,
    _Out_writes_(WaveRtBufferSize) BYTE *WaveRtBuffer,
    _Out_ ULONG *PacketNumber,
    _Out_ ULONG64 *PerformanceCounterValue,
    _Out_ BOOL *MoreData
)
{
    NTSTATUS ntStatus;
    BYTE *packetData;
    PACKET_ENTRY *packetEntry;
    LIST_ENTRY *packetListEntry = NULL;
    ULONG packetSize = WaveRtBufferSize / PacketsPerWaveRtBuffer;

    NT_ASSERT(SamplesPerPacket * 2 == packetSize);
    NT_ASSERT(sizeof(packetEntry->Samples) == packetSize);

    packetListEntry = ExInterlockedRemoveHeadList(&PacketFifoHead, &PacketFifoSpinLock);
    if (packetListEntry == NULL)
    {
        ntStatus = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }
    packetEntry = CONTAINING_RECORD(packetListEntry, PACKET_ENTRY, ListEntry);

    packetData = WaveRtBuffer + ((packetEntry->PacketNumber * packetSize) % WaveRtBufferSize);

    ntStatus = RtlLongLongToULong(packetEntry->PacketNumber, PacketNumber);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    *PerformanceCounterValue = packetEntry->QpcWhenSampled;
    *MoreData = !IsListEmpty(&PacketFifoHead);
    RtlCopyMemory(packetData, packetEntry->Samples, sizeof(packetEntry->Samples));

Exit:
    if (packetListEntry != NULL)
    {
        ExInterlockedInsertTailList(&PacketPoolHead, packetListEntry, &PacketPoolSpinLock);
    }

    return ntStatus;
}

