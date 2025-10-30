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
#ifdef SYSVAD_USB_SIDEBAND
#include "usbhsmicwavtable.h"
#endif // SYSVAD_USB_SIDEBAND

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
    NTSTATUS ResetDetector(_In_ GUID eventId);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS DownloadDetectorData(_In_ GUID eventId, _In_ LONGLONG Data);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetDetectorData(_In_ GUID eventId, _Out_ LONGLONG *Data);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ULONGLONG GetStartTimestamp();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ULONGLONG GetStopTimestamp();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS SetArmed(_In_ GUID eventId, _In_ BOOL Arm);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID NotifyDetection();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetArmed(_In_ GUID eventId, _Out_ BOOL *Arm);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetStreamingSupport(_In_ GUID eventId, _Out_ BOOL *Support);

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

    BOOL            m_streamRunning;

    BOOL            m_SoundDetectorArmed1;
    BOOL            m_SoundDetectorArmed2;
    LONGLONG        m_SoundDetectorData1;
    LONGLONG        m_SoundDetectorData2;

    LONGLONG        m_qpcStartCapture;
    LONGLONG        m_qpcFrequency;
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
#pragma code_seg()
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
    KSPIN_LOCK                          m_DeviceFormatsAndModesLock; // To serialize access.
    KIRQL                               m_DeviceFormatsAndModesIrql;
    ULONG                               m_DeviceFormatsAndModesCount; 
    USHORT                              m_DeviceMaxChannels;
    PDRMPORT                            m_pDrmPort;
    DRMRIGHTS                           m_MixDrmRights;
    ULONG                               m_ulMixDrmContentId;
    CONSTRICTOR_OPTION                  m_LoopbackProtection;

    CKeywordDetector                    m_KeywordDetector;

    union {
        PVOID                           m_DeviceContext;
#if defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)
        PSIDEBANDDEVICECOMMON           m_pSidebandDevice;
#endif  // defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)
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

    DECLARE_PROPERTYHANDLER(Get_SoundDetectorSupportedPatterns2);
    DECLARE_PROPERTYHANDLER(Set_SoundDetectorPatterns2);
    DECLARE_PROPERTYHANDLER(Get_SoundDetectorArmed2);
    DECLARE_PROPERTYHANDLER(Set_SoundDetectorArmed2);
    DECLARE_PROPERTYHANDLER(Set_SoundDetectorReset2);
    DECLARE_PROPERTYHANDLER(Get_SoundDetectorStreamingSupport2);

    DECLARE_PROPERTYHANDLER(Get_InterleavedFormatInformation);


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

    VOID SendPNPNotification(
        _In_ const GUID *                   NotificationId,
        _In_ PVOID                          NotificationBuffer, 
        _In_ USHORT                         NotificationBufferCb
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
                    // capture bridge pin comes first in the enumeration
                    //
                    if (m_FilterDesc.PinCount > KSPIN_WAVEIN_HOST)
                    {
                        m_ulMaxSystemStreams = m_FilterDesc.Pins[KSPIN_WAVEIN_HOST].MaxFilterInstanceCount;
                    }
                    if (m_FilterDesc.PinCount > KSPIN_WAVEIN_KEYWORD)
                    {
                        m_ulMaxKeywordDetectorStreams = m_FilterDesc.Pins[KSPIN_WAVEIN_KEYWORD].MaxFilterInstanceCount;
                    }
                }
            }
        }

#if defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)
        if (IsSidebandDevice())
        {
            if (m_pSidebandDevice != NULL)
            {
                // This ref is released on dtor.
                m_pSidebandDevice->AddRef(); // strong ref.
            }

        }

#endif // defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)

        KeInitializeSpinLock(&m_DeviceFormatsAndModesLock);
        m_DeviceFormatsAndModesIrql = PASSIVE_LEVEL;
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
#ifdef SYSVAD_USB_SIDEBAND
    NTSTATUS PropertyHandler_UsbHsAudioEffectsDiscoveryEffectsList  
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );
#endif  // SYSVAD_USB_SIDEBAND
#ifdef SYSVAD_A2DP_SIDEBAND
    NTSTATUS PropertyHandler_A2dpHpAudioEffectsDiscoveryEffectsList  
    (
        _In_ PPCPROPERTY_REQUEST PropertyRequest
    );
#endif  // SYSVAD_A2DP_SIDEBAND

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
    _IRQL_raises_(DISPATCH_LEVEL)
    _Acquires_lock_(m_DeviceFormatsAndModesLock)
    _Requires_lock_not_held_(m_DeviceFormatsAndModesLock)
    _IRQL_saves_global_(SpinLock, m_DeviceFormatsAndModesIrql)
    VOID AcquireFormatsAndModesLock();

    _Releases_lock_(m_DeviceFormatsAndModesLock)
    _Requires_lock_held_(m_DeviceFormatsAndModesLock)
    _IRQL_restores_global_(SpinLock, m_DeviceFormatsAndModesIrql)
    VOID ReleaseFormatsAndModesLock();

    _Post_satisfies_(return > 0)
    ULONG GetPinSupportedDeviceFormats(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) KSDATAFORMAT_WAVEFORMATEXTENSIBLE **ppFormats);

    _Post_satisfies_(return > 0)
    ULONG GetAudioEngineSupportedDeviceFormats(_Outptr_opt_result_buffer_(return) KSDATAFORMAT_WAVEFORMATEXTENSIBLE **ppFormats);

    _Success_(return != 0)
    ULONG GetPinSupportedDeviceModes(_In_ ULONG PinId, _Outptr_opt_result_buffer_(return) _On_failure_(_Deref_post_null_) MODE_AND_DEFAULT_FORMAT **ppModes);

#pragma code_seg()

protected:
#pragma code_seg()
    BOOL IsRenderDevice()
    {
        return (m_DeviceType == eSpeakerDevice   ||
                m_DeviceType == eSpeakerHpDevice ||        
                m_DeviceType == eSpeakerHsDevice ||
                m_DeviceType == eHdmiRenderDevice  ||
                m_DeviceType == eSpdifRenderDevice ||
                m_DeviceType == eBthHfpSpeakerDevice ||
                m_DeviceType == eUsbHsSpeakerDevice  ||
                m_DeviceType == eA2dpHpSpeakerDevice ||
                m_DeviceType == eHandsetSpeakerDevice) ? TRUE : FALSE;
    }

    BOOL IsCellularDevice()
    {
        return (m_DeviceType == eCellularDevice) ? TRUE : FALSE;
    }

    BOOL IsLoopbackSupported()
    {

        //
        // It is assumed that loopback is supported when offload is supported
        //
        return (m_DeviceFlags & (ENDPOINT_LOOPBACK_SUPPORTED | ENDPOINT_OFFLOAD_SUPPORTED)) ? TRUE : FALSE;
    }

    BOOL IsOffloadSupported()
    {
        return (m_DeviceFlags & ENDPOINT_OFFLOAD_SUPPORTED) ? TRUE : FALSE;
    }

    BOOL IsSystemCapturePin(ULONG nPinId);

    BOOL IsCellularBiDiCapturePin(ULONG nPinId);

    BOOL IsSystemRenderPin(ULONG nPinId);

    BOOL IsLoopbackPin(ULONG nPinId);

    BOOL IsOffloadPin(ULONG nPinId);

    BOOL IsBridgePin(ULONG nPinId);

    BOOL IsKeywordDetectorPin(ULONG nPinId);

    // These three pins are the pins used by the audio engine for host, loopback, and offload.
    ULONG GetSystemPinId()
    {
        ASSERT(IsRenderDevice());
        ASSERT(!IsCellularDevice());
        return IsOffloadSupported() ? KSPIN_WAVE_RENDER_SINK_SYSTEM : KSPIN_WAVE_RENDER2_SINK_SYSTEM;
    }


    ULONG GetLoopbackPinId()
    {
        ASSERT(IsRenderDevice());
        ASSERT(!IsCellularDevice());
        return IsOffloadSupported() ? KSPIN_WAVE_RENDER_SINK_LOOPBACK : KSPIN_WAVE_RENDER2_SINK_LOOPBACK;
    }


    ULONG GetOffloadPinId()
    {
        ASSERT(IsRenderDevice());
        ASSERT(IsOffloadSupported());
        ASSERT(!IsCellularDevice());
        return KSPIN_WAVE_RENDER_SINK_OFFLOAD;
    }


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

#if defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)
public:
#pragma code_seg()
    BOOL IsSidebandDevice()
    {
        return (m_DeviceType == eBthHfpMicDevice ||
                m_DeviceType == eBthHfpSpeakerDevice ||
                m_DeviceType == eUsbHsMicDevice ||
                m_DeviceType == eUsbHsSpeakerDevice ||
                m_DeviceType == eA2dpHpSpeakerDevice ) ? TRUE : FALSE;
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
#endif // defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)

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

