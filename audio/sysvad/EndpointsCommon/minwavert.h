/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minwavert.h

Abstract:

    Definition of wavert miniport class.

--*/

#ifndef _SYSVAD_MINWAVERT_H_
#define _SYSVAD_MINWAVERT_H_

#ifdef SYSVAD_BTH_BYPASS
#include "bthhfpmicwavtable.h"
#endif // SYSVAD_BTH_BYPASS

//=============================================================================
// Referenced Forward
//=============================================================================
class CMiniportWaveRTStream;
typedef CMiniportWaveRTStream *PCMiniportWaveRTStream;

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CKeywordDetector
//
class CKeywordDetector
{
public:
    CKeywordDetector();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID ResetDetector();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID DownloadDetectorData(_In_ LONGLONG Data);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    LONGLONG GetDetectorData();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ULONGLONG GetStartTimestamp();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ULONGLONG GetStopTimestamp();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS SetArmed(_In_ BOOL Arm);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOL GetArmed();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID Run();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID Stop();

    _IRQL_requires_min_(DISPATCH_LEVEL)
    VOID DpcRoutine(_In_ LONGLONG PerformanceCounter, _In_ LONGLONG PerformanceFrequency);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetReadPacket(_In_ ULONG PacketsPerWaveRtBuffer, _In_  ULONG WaveRtBufferSize, _Out_writes_(WaveRtBufferSize) BYTE *WaveRtBuffer, _Out_ ULONG *PacketNumber, _Out_ ULONGLONG *PerformanceCount, _Out_ BOOL *MoreData);

private:
    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID ResetFifo();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS ReadKeywordTimestampRegistry();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID StartBufferingStream();

    // The Contoso keyword detector processes 10ms packets of 16KHz 16-bit PCM
    // audio samples
    static const int SamplesPerSecond = 16000;
    static const int SamplesPerPacket = (10 * SamplesPerSecond / 1000);

    typedef struct
    {
        LIST_ENTRY  ListEntry;
        LONGLONG    PacketNumber;
        LONGLONG    QpcWhenSampled;
        UINT16      Samples[SamplesPerPacket];
    } PACKET_ENTRY;

    BOOL            m_SoundDetectorArmed;
    LONGLONG        m_SoundDetectorData;

    LONGLONG        m_qpcStartCapture;
    LONGLONG        m_nLastQueuedPacket;

    ULONGLONG       m_ullKeywordStartTimestamp;
    ULONGLONG       m_ullKeywordStopTimestamp;

    KSPIN_LOCK      PacketPoolSpinLock;
    LIST_ENTRY      PacketPoolHead;
    PACKET_ENTRY    PacketPool[1 * SamplesPerSecond / SamplesPerPacket];    // Enough storage for 1 second of audio data

    KSPIN_LOCK      PacketFifoSpinLock;
    LIST_ENTRY      PacketFifoHead;

};

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveRT
//   
class CMiniportWaveRT : 
    public IMiniportWaveRT,
    public IMiniportAudioEngineNode,
    public IMiniportAudioSignalProcessing,
    public CUnknown
{
private:
    ULONG                               m_ulLoopbackAllocated;
    ULONG                               m_ulSystemAllocated;
    ULONG                               m_ulOffloadAllocated;
    ULONG                               m_ulKeywordDetectorAllocated;

    ULONG                               m_ulMaxSystemStreams;
    ULONG                               m_ulMaxOffloadStreams;
    ULONG                               m_ulMaxLoopbackStreams;
    ULONG                               m_ulMaxKeywordDetectorStreams;

    // weak ref of running streams.
    PCMiniportWaveRTStream            * m_SystemStreams;
    PCMiniportWaveRTStream            * m_OffloadStreams;
    PCMiniportWaveRTStream            * m_LoopbackStreams;

    BOOL                                m_bGfxEnabled;
    PBOOL                               m_pbMuted;
    PLONG                               m_plVolumeLevel;
    PLONG                               m_plPeakMeter;
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE  m_pMixFormat;
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE  m_pDeviceFormat;
    PCFILTER_DESCRIPTOR                 m_FilterDesc;
    PIN_DEVICE_FORMATS_AND_MODES *      m_DeviceFormatsAndModes;
    FAST_MUTEX                          m_DeviceFormatsAndModesLock; // To serialize access.
    ULONG                               m_DeviceFormatsAndModesCount; 
    USHORT                              m_DeviceMaxChannels;
    PDRMPORT                            m_pDrmPort;
    DRMRIGHTS                           m_MixDrmRights;
    ULONG                               m_ulMixDrmContentId;
    CONSTRICTOR_OPTION                  m_LoopbackProtection;

    CKeywordDetector                    m_KeywordDetector;

    union {
        PVOID                           m_DeviceContext;
#if defined(SYSVAD_BTH_BYPASS)
        PSIDEBANDDEVICECOMMON           m_pSidebandDevice;
#endif  // defined(SYSVAD_BTH_BYPASS)
    };

    AUDIOMODULE *                       m_pAudioModules;

protected:
    PADAPTERCOMMON                      m_pAdapterCommon;
    ULONG                               m_DeviceFlags;
    eDeviceType                         m_DeviceType;
    PPORTEVENTS                         m_pPortEvents;
    PPORTCLSNOTIFICATIONS               m_pPortClsNotifications;
    PENDPOINT_MINIPAIR                  m_pMiniportPair;


public:
    DECLARE_PROPERTYHANDLER(Get_SoundDetectorSupportedPatterns);
    DECLARE_PROPERTYHANDLER(Set_SoundDetectorPatterns);
    DECLARE_PROPERTYHANDLER(Get_SoundDetectorArmed);
    DECLARE_PROPERTYHANDLER(Set_SoundDetectorArmed);
    DECLARE_PROPERTYHANDLER(Get_SoundDetectorMatchResult);
    
    NTSTATUS EventHandler_PinCapsChange
    (
        _In_  PPCEVENT_REQUEST EventRequest
    );

    NTSTATUS EventHandler_SoundDetectorMatchDetected
    (
        _In_  PPCEVENT_REQUEST EventRequest
    );

    NTSTATUS ValidateStreamCreate
    (
        _In_ ULONG _Pin, 
        _In_ BOOLEAN _Capture
    );
    
    NTSTATUS StreamCreated
    (
        _In_ ULONG                  _Pin,
        _In_ PCMiniportWaveRTStream _Stream
    );
    
    NTSTATUS StreamClosed
    (
        _In_ ULONG _Pin,
        _In_ PCMiniportWaveRTStream _Stream
    );
    
    NTSTATUS IsFormatSupported
    ( 
        _In_ ULONG          _ulPin, 
        _In_ BOOLEAN        _bCapture,
        _In_ PKSDATAFORMAT  _pDataFormat
    );

    static NTSTATUS GetAttributesFromAttributeList
    (
        _In_ const KSMULTIPLE_ITEM *_pAttributes,
        _In_ size_t _Size,
        _Out_ GUID* _pSignalProcessingMode
    );

protected:
    NTSTATUS UpdateDrmRights
    (
        void
    );
    
    NTSTATUS SetLoopbackProtection
    (
        _In_ CONSTRICTOR_OPTION ulProtectionOption
    );

    VOID AddEventToEventList
    (
        _In_  PKSEVENT_ENTRY    EventEntry
    );

    VOID                        GenerateEventList
    (
        _In_opt_    GUID       *Set,
        _In_        ULONG       EventId,
        _In_        BOOL        PinEvent,
        _In_        ULONG       PinId,
        _In_        BOOL        NodeEvent,
        _In_        ULONG       NodeId
    );

public:
    DECLARE_STD_UNKNOWN();

#pragma code_seg("PAGE")
    CMiniportWaveRT(
        _In_            PUNKNOWN                                UnknownAdapter,
        _In_            PENDPOINT_MINIPAIR                      MiniportPair,
        _In_opt_        PVOID                                   DeviceContext
    )
        :CUnknown(0),
        m_ulMaxSystemStreams(0),
        m_ulMaxOffloadStreams(0),
        m_ulMaxLoopbackStreams(0),
        m_ulMaxKeywordDetectorStreams(0),
        m_DeviceType(MiniportPair->DeviceType),
        m_DeviceContext(DeviceContext),
        m_DeviceMaxChannels(MiniportPair->DeviceMaxChannels),
        m_DeviceFormatsAndModes(MiniportPair->PinDeviceFormatsAndModes),
        m_DeviceFormatsAndModesCount(MiniportPair->PinDeviceFormatsAndModesCount),
        m_DeviceFlags(MiniportPair->DeviceFlags),
        m_pMiniportPair(MiniportPair),
        m_pAudioModules(NULL),
        m_pPortClsNotifications(NULL)
    {
        PAGED_CODE();

        m_pAdapterCommon = (PADAPTERCOMMON)UnknownAdapter; // weak ref.

        if (MiniportPair->WaveDescriptor)
        {
            RtlCopyMemory(&m_FilterDesc, MiniportPair->WaveDescriptor, sizeof(m_FilterDesc));
            
            //
            // Get the max # of pin instances.
            //
            if (IsRenderDevice())
            {
                if (IsOffloadSupported())
                {
                    if (m_FilterDesc.PinCount > KSPIN_WAVE_RENDER_SOURCE)
                    {
                        m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER_SINK_SYSTEM].MaxFilterInstanceCount;
                        m_ulMaxOffloadStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER_SINK_OFFLOAD].MaxFilterInstanceCount;
                        m_ulMaxLoopbackStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER_SINK_LOOPBACK].MaxFilterInstanceCount;
                    }
                }
                else
                {
                    if (m_FilterDesc.PinCount > KSPIN_WAVE_RENDER2_SOURCE)
                    {
                        m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER2_SINK_SYSTEM].MaxFilterInstanceCount;
                        m_ulMaxLoopbackStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER2_SINK_LOOPBACK].MaxFilterInstanceCount;
                    }
                    else if(m_FilterDesc.PinCount > KSPIN_WAVE_RENDER3_SOURCE)
                    {
                        m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVE_RENDER3_SINK_SYSTEM].MaxFilterInstanceCount;
                    }
                }
            }
            else
            {
                // cellular capture follows a different pin ordering than a standard wavein pin & bridge.
                if (IsCellularDevice())
                {
                    m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVE_BIDI].MaxFilterInstanceCount;
                }
                else
                {
                    //
                    // >= comparison here because capture bridge pin comes first in the enumeration
                    //
                    if (m_FilterDesc.PinCount >= KSPIN_WAVEIN_HOST)
                    {
                        m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVEIN_HOST].MaxFilterInstanceCount;
                    }
                    if (m_FilterDesc.PinCount >= KSPIN_WAVEIN_KEYWORD)
                    {
                        m_ulMaxKeywordDetectorStreams = m_FilterDesc.Pins[KSPIN_WAVEIN_KEYWORD].MaxFilterInstanceCount;
                    }
                }
            }
        }

#if defined(SYSVAD_BTH_BYPASS)
        if (IsSidebandDevice())
        {
            if (m_pSidebandDevice != NULL)
            {
                // This ref is released on dtor.
                m_pSidebandDevice->AddRef(); // strong ref.
            }

        }
        ExInitializeFastMutex(&m_DeviceFormatsAndModesLock);
#endif // defined(SYSVAD_BTH_BYPASS)
    }

#pragma code_seg()

    ~CMiniportWaveRT();

    IMP_IMiniportWaveRT;
    IMP_IMiniportAudioEngineNode;
    IMP_IMiniportAudioSignalProcessing;
    
    // Friends
    friend class        CMiniportWaveRTStream;
    friend class        CMiniportTopologySimple;
    
    friend NTSTATUS PropertyHandler_WaveFilter
    (   
        _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );   

public:
    VOID DpcRoutine(LONGLONG PerformanceCounter, LONGLONG PerformanceFrequency)
    {
        m_KeywordDetector.DpcRoutine(PerformanceCounter, PerformanceFrequency);
    }

    NTSTATUS PropertyHandlerEffectListRequest
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );    

    NTSTATUS PropertyHandlerProposedFormat
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandlerProposedFormat2
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandlerAudioEffectsDiscoveryEffectsList
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );
    
    NTSTATUS PropertyHandlerModulesListRequest
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );

    NTSTATUS PropertyHandlerModuleCommand
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );
    
    NTSTATUS PropertyHandlerModuleNotificationDeviceId
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );


    PADAPTERCOMMON GetAdapterCommObj() 
    {
        return m_pAdapterCommon; 
    };
#pragma code_seg()

#ifdef SYSVAD_BTH_BYPASS
    NTSTATUS PropertyHandler_BthHfpAudioEffectsDiscoveryEffectsList  
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );
#endif  // SYSVAD_BTH_BYPASS

    //---------------------------------------------------------------------------------------------------------
    // volume
    //---------------------------------------------------------------------------------------------------------
    NTSTATUS GetVolumeChannelCount
    (
        _Out_  UINT32 * pulChannelCount
    );
    
    NTSTATUS GetVolumeSteppings
    (
        _Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, 
        _In_  UINT32    _ui32DataSize
    );
    
    NTSTATUS GetChannelVolume
    (
        _In_  UINT32    _uiChannel, 
        _Out_  LONG *   _pVolume
    );
    
    NTSTATUS SetChannelVolume
    (
        _In_  UINT32    _uiChannel, 
        _In_  LONG      _Volume
    );

    //-----------------------------------------------------------------------------
    // metering 
    //-----------------------------------------------------------------------------
    NTSTATUS GetPeakMeterChannelCount
    (
        _Out_  UINT32 * pulChannelCount
    );
    
    NTSTATUS GetPeakMeterSteppings
    (
        _Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, 
        _In_  UINT32    _ui32DataSize
    );
    
    NTSTATUS GetChannelPeakMeter
    (
        _In_  UINT32    _uiChannel, 
        _Out_  LONG *   _plPeakMeter
    );

    //-----------------------------------------------------------------------------
    // mute
    //-----------------------------------------------------------------------------
    NTSTATUS GetMuteChannelCount
    (
        _Out_  UINT32 * pulChannelCount
    );
    
    NTSTATUS GetMuteSteppings
    (
        _Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, 
        _In_  UINT32    _ui32DataSize
    );
    
    NTSTATUS GetChannelMute
    (
        _In_  UINT32    _uiChannel, 
        _Out_  BOOL *   _pbMute
    );
    
    NTSTATUS SetChannelMute
    (
        _In_  UINT32    _uiChannel, 
        _In_  BOOL      _bMute
    );

private:
#pragma code_seg("PAGE")
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
    _Post_satisfies_(return > 0)
    ULONG GetPinSupportedDeviceFormats(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) KSDATAFORMAT_WAVEFORMATEXTENSIBLE **ppFormats)
    {
        PAGED_CODE();

        PPIN_DEVICE_FORMATS_AND_MODES pDeviceFormatsAndModes = NULL;

        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        pDeviceFormatsAndModes = m_DeviceFormatsAndModes;
        ASSERT(m_DeviceFormatsAndModesCount > PinId);
        ASSERT(pDeviceFormatsAndModes[PinId].WaveFormats != NULL);
        ASSERT(pDeviceFormatsAndModes[PinId].WaveFormatsCount > 0);

        if (ppFormats != NULL)
        {
            *ppFormats = pDeviceFormatsAndModes[PinId].WaveFormats;
        }
        
        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);

        return pDeviceFormatsAndModes[PinId].WaveFormatsCount;
    }

    //---------------------------------------------------------------------------
    // GetAudioEngineSupportedDeviceFormats 
    //
    //  Return supported device formats for the audio engine node.
    //
    //  Return value
    //      The number of KSDATAFORMAT_WAVEFORMATEXTENSIBLE items.
    //
    //  Remarks
    //      Supported formats index array follows same order as filter's pin
    //      descriptor list. This routine assumes the engine formats are the
    //      last item in the filter's array of PIN_DEVICE_FORMATS_AND_MODES.
    //
    _Post_satisfies_(return > 0)
    ULONG GetAudioEngineSupportedDeviceFormats(_Outptr_opt_result_buffer_(return) KSDATAFORMAT_WAVEFORMATEXTENSIBLE **ppFormats)
    {
        ULONG i;
        PPIN_DEVICE_FORMATS_AND_MODES pDeviceFormatsAndModes = NULL;

        PAGED_CODE();

        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        pDeviceFormatsAndModes = m_DeviceFormatsAndModes;

        // By convention, the audio engine node's device formats are the last
        // entry in the PIN_DEVICE_FORMATS_AND_MODES list.
        
        // Since this endpoint apparently supports offload, there must be at least a system,
        // offload, and loopback pin, plus the entry for the device formats.
        ASSERT(m_DeviceFormatsAndModesCount > 3);

        i = m_DeviceFormatsAndModesCount - 1;                       // Index of last list entry

        ASSERT(pDeviceFormatsAndModes[i].PinType == NoPin);
        ASSERT(pDeviceFormatsAndModes[i].WaveFormats != NULL);
        ASSERT(pDeviceFormatsAndModes[i].WaveFormatsCount > 0);

        if (ppFormats != NULL)
        {
            *ppFormats = pDeviceFormatsAndModes[i].WaveFormats;
        }

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return pDeviceFormatsAndModes[i].WaveFormatsCount;
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
    _Success_(return != 0)
    ULONG GetPinSupportedDeviceModes(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) _On_failure_(_Deref_post_null_) MODE_AND_DEFAULT_FORMAT **ppModes)
    {
        PMODE_AND_DEFAULT_FORMAT modes;
        ULONG numModes;

        PAGED_CODE();

        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        ASSERT(m_DeviceFormatsAndModesCount > PinId);
        ASSERT((m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount == 0) == (m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat == NULL));

        modes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat;
        numModes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount;

#ifdef SYSVAD_BTH_BYPASS
        // Special handling for the SCO bypass endpoint, whose modes are determined at runtime
        if (m_DeviceType == eBthHfpMicDevice)
        {
            ASSERT(m_pSidebandDevice != NULL);
            if (m_pSidebandDevice->IsNRECSupported())
            {
                modes = BthHfpMicPinSupportedDeviceModesNrec;
                numModes = ARRAYSIZE(BthHfpMicPinSupportedDeviceModesNrec);
            }
            else
            {
                modes = BthHfpMicPinSupportedDeviceModesNoNrec;
                numModes = ARRAYSIZE(BthHfpMicPinSupportedDeviceModesNoNrec);
            }
        }
#endif // SYSVAD_BTH_BYPASS

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

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return numModes;
    }
#pragma code_seg()

protected:
#pragma code_seg("PAGE")
    BOOL IsRenderDevice()
    {
        PAGED_CODE();
        return (m_DeviceType == eSpeakerDevice   ||
                m_DeviceType == eSpeakerHpDevice ||        
                m_DeviceType == eSpeakerHsDevice ||
                m_DeviceType == eHdmiRenderDevice  ||
                m_DeviceType == eSpdifRenderDevice ||
                m_DeviceType == eBthHfpSpeakerDevice ||
                m_DeviceType == eHandsetSpeakerDevice) ? TRUE : FALSE;
    }

    BOOL IsCellularDevice()
    {
        PAGED_CODE();
        return (m_DeviceType == eCellularDevice) ? TRUE : FALSE;
    }

    BOOL IsLoopbackSupported()
    {
        PAGED_CODE();

        //
        // It is assumed that loopback is supported when offload is supported
        //
        return (m_DeviceFlags & (ENDPOINT_LOOPBACK_SUPPORTED | ENDPOINT_OFFLOAD_SUPPORTED)) ? TRUE : FALSE;
    }

    BOOL IsOffloadSupported()
    {
        PAGED_CODE();
        return (m_DeviceFlags & ENDPOINT_OFFLOAD_SUPPORTED) ? TRUE : FALSE;
    }

    BOOL IsSystemCapturePin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == SystemCapturePin);
    }

    BOOL IsCellularBiDiCapturePin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == TelephonyBidiPin);
    }

    BOOL IsSystemRenderPin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == SystemRenderPin);
    }

    BOOL IsLoopbackPin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == RenderLoopbackPin);
    }

    BOOL IsOffloadPin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == OffloadRenderPin);
    }

    BOOL IsBridgePin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == BridgePin);
    }

    BOOL IsKeywordDetectorPin(ULONG nPinId)
    {
        PAGED_CODE();
        ExAcquireFastMutex(&m_DeviceFormatsAndModesLock);

        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;

        ExReleaseFastMutex(&m_DeviceFormatsAndModesLock);
        return (pinType == KeywordCapturePin);
    }

    // These three pins are the pins used by the audio engine for host, loopback, and offload.
    ULONG GetSystemPinId()
    {
        PAGED_CODE();
        ASSERT(IsRenderDevice());
        ASSERT(!IsCellularDevice());
        return IsOffloadSupported() ? KSPIN_WAVE_RENDER_SINK_SYSTEM : KSPIN_WAVE_RENDER2_SINK_SYSTEM;
    }


    ULONG GetLoopbackPinId()
    {
        PAGED_CODE();
        ASSERT(IsRenderDevice());
        ASSERT(!IsCellularDevice());
        return IsOffloadSupported() ? KSPIN_WAVE_RENDER_SINK_LOOPBACK : KSPIN_WAVE_RENDER2_SINK_LOOPBACK;
    }


    ULONG GetOffloadPinId()
    {
        PAGED_CODE();
        ASSERT(IsRenderDevice());
        ASSERT(IsOffloadSupported());
        ASSERT(!IsCellularDevice());
        return KSPIN_WAVE_RENDER_SINK_OFFLOAD;
    }

#pragma code_seg()

    ULONG
    GetAudioModuleDescriptorListCount()
    {
        return m_pMiniportPair->ModuleListCount;
    }

    const PAUDIOMODULE_DESCRIPTOR
    GetAudioModuleDescriptor(
        _In_ ULONG Index
        )
    {
        ASSERT(Index < GetAudioModuleDescriptorListCount());
        return &m_pMiniportPair->ModuleList[Index];
    }

    const PAUDIOMODULE_DESCRIPTOR
    GetAudioModuleDescriptorList()
    {
        return m_pMiniportPair->ModuleList;
    }
    
    ULONG
    GetAudioModuleListCount()
    {
        return GetAudioModuleDescriptorListCount();
    }

    AUDIOMODULE *
    GetAudioModule(
        _In_ ULONG Index
        )
    {
        ASSERT(Index < GetAudioModuleListCount());
        return &m_pAudioModules[Index];
    }

    AUDIOMODULE *
    GetAudioModuleList()
    {
        return m_pAudioModules;
    }
    
    const GUID *
    GetAudioModuleNotificationDeviceId()
    {
        return m_pMiniportPair->ModuleNotificationDeviceId;
    }
    
    NTSTATUS
    AllocStreamAudioModules(
        _In_ const GUID *       SignalProcessingMode,
        _Out_ AUDIOMODULE **    AudioModule,
        _Out_ ULONG *           AudioModuleCount
        );
        
    VOID
    FreeStreamAudioModules(
        _In_ AUDIOMODULE *      AudioModule,
        _In_ ULONG              AudioModuleCount
        );

#if defined(SYSVAD_BTH_BYPASS)
public:
#pragma code_seg()
    BOOL IsSidebandDevice()
    {
        return (m_DeviceType == eBthHfpMicDevice ||
                m_DeviceType == eBthHfpSpeakerDevice ) ? TRUE : FALSE;
    }

    // Returns a weak ref to the Bluetooth HFP device.
    PSIDEBANDDEVICECOMMON GetSidebandDevice() 
    {
        PSIDEBANDDEVICECOMMON sidebandDevice = NULL;
        

        if (IsSidebandDevice())
        {
            if (m_pSidebandDevice != NULL)
            {
                sidebandDevice = m_pSidebandDevice;
            }
        }
    
        return sidebandDevice;
    }
#pragma code_seg()
#endif // defined(SYSVAD_BTH_BYPASS)

#ifdef SYSVAD_BTH_BYPASS
#pragma code_seg()
    static
    VOID
    EvtFormatChangeHandler
    (
        _In_opt_    PVOID   Context
    );

#endif //#ifdef SYSVAD_BTH_BYPASS
};

typedef CMiniportWaveRT *PCMiniportWaveRT;

#endif // _SYSVAD_MINWAVERT_H_

