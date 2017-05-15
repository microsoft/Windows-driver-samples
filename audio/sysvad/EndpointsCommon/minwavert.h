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
    DWORD                               m_dwCaptureAllocatedModes;
    DWORD                               m_dwBiDiCaptureAllocatedModes;
    DWORD                               m_dwSystemAllocatedModes;

    ULONG                               m_ulMaxSystemStreams;
    ULONG                               m_ulMaxOffloadStreams;
    ULONG                               m_ulMaxLoopbackStreams;

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
    ULONG                               m_DeviceFormatsAndModesCount; 
    USHORT                              m_DeviceMaxChannels;
    PDRMPORT                            m_pDrmPort;
    DRMRIGHTS                           m_MixDrmRights;
    ULONG                               m_ulMixDrmContentId;
    CONSTRICTOR_OPTION                  m_LoopbackProtection;

    CKeywordDetector                    m_KeywordDetector;

    union {
        PVOID                           m_DeviceContext;
#ifdef SYSVAD_BTH_BYPASS
        PBTHHFPDEVICECOMMON             m_BthHfpDevice;
#endif  // SYSVAD_BTH_BYPASS
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

    NTSTATUS EventHandler_SoundDetectorMatchDetected
    (
        _In_  PPCEVENT_REQUEST EventRequest
    );

    NTSTATUS ValidateStreamCreate
    (
        _In_ ULONG _Pin, 
        _In_ BOOLEAN _Capture,
        _In_ GUID _SignalProcessingMode
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
                }
            }
        }

#ifdef SYSVAD_BTH_BYPASS
        if (IsBthHfpDevice())
        {
            if (m_BthHfpDevice != NULL)
            {
                // This ref is released on dtor.
                m_BthHfpDevice->AddRef(); // strong ref.
            }
        }
#endif // SYSVAD_BTH_BYPASS
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

        ASSERT(m_DeviceFormatsAndModesCount > PinId);
        ASSERT(m_DeviceFormatsAndModes[PinId].WaveFormats != NULL);
        ASSERT(m_DeviceFormatsAndModes[PinId].WaveFormatsCount > 0);

        if (ppFormats != NULL)
        {
            *ppFormats = m_DeviceFormatsAndModes[PinId].WaveFormats;
        }
        
        return m_DeviceFormatsAndModes[PinId].WaveFormatsCount;
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

        PAGED_CODE();

        // By convention, the audio engine node's device formats are the last
        // entry in the PIN_DEVICE_FORMATS_AND_MODES list.
        
        // Since this endpoint apparently supports offload, there must be at least a system,
        // offload, and loopback pin, plus the entry for the device formats.
        ASSERT(m_DeviceFormatsAndModesCount > 3);

        i = m_DeviceFormatsAndModesCount - 1;                       // Index of last list entry

        ASSERT(m_DeviceFormatsAndModes[i].PinType == NoPin);
        ASSERT(m_DeviceFormatsAndModes[i].WaveFormats != NULL);
        ASSERT(m_DeviceFormatsAndModes[i].WaveFormatsCount > 0);

        if (ppFormats != NULL)
        {
            *ppFormats = m_DeviceFormatsAndModes[i].WaveFormats;
        }

        return m_DeviceFormatsAndModes[i].WaveFormatsCount;
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

        ASSERT(m_DeviceFormatsAndModesCount > PinId);
        ASSERT((m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount == 0) == (m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat == NULL));

        modes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormat;
        numModes = m_DeviceFormatsAndModes[PinId].ModeAndDefaultFormatCount;

#ifdef SYSVAD_BTH_BYPASS
        // Special handling for the SCO bypass endpoint, whose modes are determined at runtime
        if (m_DeviceType == eBthHfpMicDevice)
        {
            ASSERT(m_BthHfpDevice != NULL);
            if (m_BthHfpDevice->IsNRECSupported())
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

    BOOL IsOffloadSupported()
    {
        PAGED_CODE();
        return (m_DeviceFlags & ENDPOINT_OFFLOAD_SUPPORTED) ? TRUE : FALSE;
    }

    BOOL IsSystemCapturePin(ULONG nPinId)
    {
        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;
        PAGED_CODE();
        return (pinType == SystemCapturePin);
    }

    BOOL IsCellularBiDiCapturePin(ULONG nPinId)
    {
        PAGED_CODE();
        return (m_DeviceFormatsAndModes[nPinId].PinType == TelephonyBidiPin);
    }

    BOOL IsSystemRenderPin(ULONG nPinId)
    {
        PINTYPE pinType = m_DeviceFormatsAndModes[nPinId].PinType;
        PAGED_CODE();
        return (pinType == SystemRenderPin);
    }

    BOOL IsLoopbackPin(ULONG nPinId)
    {
        PAGED_CODE();
        return (m_DeviceFormatsAndModes[nPinId].PinType == RenderLoopbackPin);
    }

    BOOL IsOffloadPin(ULONG nPinId)
    {
        PAGED_CODE();
        return (m_DeviceFormatsAndModes[nPinId].PinType == OffloadRenderPin);
    }

    BOOL IsBridgePin(ULONG nPinId)
    {
        PAGED_CODE();
        return (m_DeviceFormatsAndModes[nPinId].PinType == BridgePin);
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

#ifdef SYSVAD_BTH_BYPASS
public:
#pragma code_seg("PAGE")
    BOOL IsBthHfpDevice()
    {
        PAGED_CODE();
        return (m_DeviceType == eBthHfpMicDevice ||
                m_DeviceType == eBthHfpSpeakerDevice) ? TRUE : FALSE;
    }

    // Returns a weak ref to the Bluetooth HFP device.
    PBTHHFPDEVICECOMMON GetBthHfpDevice() 
    {
        PBTHHFPDEVICECOMMON bthHfpDevice = NULL;
        
        PAGED_CODE();

        if (IsBthHfpDevice())
        {
            if (m_BthHfpDevice != NULL)
            {
                bthHfpDevice = m_BthHfpDevice;
            }
        }
    
        return bthHfpDevice;
    }
#pragma code_seg()
#endif // SYSVAD_BTH_BYPASS
};
typedef CMiniportWaveRT *PCMiniportWaveRT;

#endif // _SYSVAD_MINWAVERT_H_

