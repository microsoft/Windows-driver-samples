/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minwavertstream.h

Abstract:

    Definition of wavert miniport class.
--*/

#ifndef _SIMPLEAUDIOSAMPLE_MINWAVERTSTREAM_H_
#define _SIMPLEAUDIOSAMPLE_MINWAVERTSTREAM_H_

#include "savedata.h"
#include "ToneGenerator.h"

//
// Structure to store notifications events in a protected list
//
typedef struct _NotificationListEntry
{
    LIST_ENTRY  ListEntry;
    PKEVENT     NotificationEvent;
} NotificationListEntry;

EXT_CALLBACK   TimerNotifyRT;

//=============================================================================
// Referenced Forward
//=============================================================================
class CMiniportWaveRT;
typedef CMiniportWaveRT *PCMiniportWaveRT;

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveRTStream 
// 
class CMiniportWaveRTStream : 
    public IDrmAudioStream,
    public IMiniportWaveRTStreamNotification,
    public IMiniportWaveRTInputStream,
    public IMiniportWaveRTOutputStream,
    public CUnknown
{
protected:
    PPORTWAVERTSTREAM           m_pPortStream;
    LIST_ENTRY                  m_NotificationList;
    PEX_TIMER                   m_pNotificationTimer;
    ULONG                       m_ulNotificationIntervalMs;
    ULONG                       m_ulCurrentWritePosition;
    LONG                        m_IsCurrentWritePositionUpdated;
    
public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveRTStream);
    ~CMiniportWaveRTStream();

    IMP_IMiniportWaveRTStream;
    IMP_IMiniportWaveRTStreamNotification;
    IMP_IMiniportWaveRTInputStream;
    IMP_IMiniportWaveRTOutputStream;
    IMP_IMiniportWaveRT;
    IMP_IDrmAudioStream;

    NTSTATUS                    Init
    ( 
        _In_  PCMiniportWaveRT    Miniport,
        _In_  PPORTWAVERTSTREAM   Stream,
        _In_  ULONG               Channel,
        _In_  BOOLEAN             Capture,
        _In_  PKSDATAFORMAT       DataFormat,
        _In_  GUID                SignalProcessingMode
    );

    // Friends
    friend class                CMiniportWaveRT;
    friend EXT_CALLBACK         TimerNotifyRT;
protected:
    CMiniportWaveRT*            m_pMiniport;
    ULONG                       m_ulPin;
    BOOLEAN                     m_bCapture;
    BOOLEAN                     m_bUnregisterStream;
    ULONG                       m_ulDmaBufferSize;
    BYTE*                       m_pDmaBuffer;
    ULONG                       m_ulNotificationsPerBuffer;
    KSSTATE                     m_KsState;
    PKTIMER                     m_pTimer;
    PRKDPC                      m_pDpc;
    ULONGLONG                   m_ullPlayPosition;
    ULONGLONG                   m_ullWritePosition;
    ULONGLONG                   m_ullLinearPosition;
    ULONGLONG                   m_ullPresentationPosition;
    ULONG                       m_ulLastOsReadPacket;
    ULONG                       m_ulLastOsWritePacket;
    LONGLONG                    m_llPacketCounter;
    ULONGLONG                   m_ullDmaTimeStamp;
    LARGE_INTEGER               m_ullPerformanceCounterFrequency;
    ULONGLONG                   m_hnsElapsedTimeCarryForward;
    ULONGLONG                   m_ullLastDPCTimeStamp;
    ULONGLONG                   m_hnsDPCTimeCarryForward;
    ULONG                       m_byteDisplacementCarryForward;
    ULONG                       m_ulDmaMovementRate;
    BOOL                        m_bLfxEnabled;
    PBOOL                       m_pbMuted;
    PLONG                       m_plVolumeLevel;
    PLONG                       m_plPeakMeter;
    PWAVEFORMATEXTENSIBLE       m_pWfExt;
    ULONG                       m_ulContentId;
    CSaveData                   m_SaveData;
    ToneGenerator               m_ToneGenerator;
    GUID                        m_SignalProcessingMode;
    BOOLEAN                     m_bEoSReceived;
    BOOLEAN                     m_bLastBufferRendered;
    KSPIN_LOCK                  m_PositionSpinLock;
    // Member variable as config params for tone generator
    ULONG                       m_ulHostCaptureToneFrequency;
    // If abs(m_dwHostCaptureToneAmplitude) + abs(m_dwHostCaptureToneDCValue) > 100
    // m_dwHostCaptureToneDCValue will be compensated to make the sum equal to 100
    DWORD                       m_dwHostCaptureToneAmplitude;   // must be between -100 to 100
    DWORD                       m_dwLoopbackCaptureToneAmplitude; // must be between -100 to 100
    DWORD                       m_dwHostCaptureToneDCOffset;   // must be between -100 to 100
    DWORD                       m_dwLoopbackCaptureToneDCOffset; // must be between -100 to 100
    DWORD                       m_dwHostCaptureToneInitialPhase;   // must be between -31416 to 31416
    DWORD                       m_dwLoopbackCaptureToneInitialPhase; // must be between -31416 to 31416
    // Member variable as config params for tone generator

public:

    NTSTATUS GetVolumeChannelCount
    (
        _Out_ UINT32* puiChannelCount
    );

    NTSTATUS GetVolumeSteppings
    (
        _Out_writes_bytes_(_ui32DataSize) PKSPROPERTY_STEPPING_LONG _pKsPropStepLong,
        _In_  UINT32 _ui32DataSize
    );

    NTSTATUS GetChannelVolume
    (
        _In_  UINT32 _uiChannel,
        _Out_  LONG* _pVolume
    );

    NTSTATUS SetChannelVolume
    (
        _In_  UINT32 _uiChannel,
        _In_  LONG _Volume
    );

    NTSTATUS GetMuteChannelCount
    (
        _Out_ UINT32* puiChannelCount
    );

    NTSTATUS GetMuteSteppings
    (
        _Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong,
        _In_  UINT32 _ui32DataSize
    );

    NTSTATUS GetChannelMute
    (
        _In_  UINT32 _uiChannel,
        _Out_  BOOL* _pbMute
    );

    NTSTATUS SetChannelMute
    (
        _In_  UINT32 _uiChannel,
        _In_  BOOL _bMute
    );

    //presentation
    NTSTATUS GetPresentationPosition
    (
        _Out_  KSAUDIO_PRESENTATION_POSITION *_pPresentationPosition
    );

    NTSTATUS SetCurrentWritePosition
    (
        _In_  ULONG ulCurrentWritePosition
    );

        
    ULONG GetCurrentWaveRTWritePosition() 
    {
        return m_ulCurrentWritePosition;
    };

    // To support simple underrun validation.
    BOOL IsCurrentWaveRTWritePositionUpdated() 
    {
        return InterlockedExchange(&m_IsCurrentWritePositionUpdated, 0) ? TRUE : FALSE;
    };

    GUID GetSignalProcessingMode()
    {
        return m_SignalProcessingMode;
    }
    
private:

    //
    // Helper functions.
    //
    
#pragma code_seg()

    VOID WriteBytes
    (
        _In_ ULONG ByteDisplacement
    );
        
    VOID ReadBytes
    (
        _In_ ULONG ByteDisplacement
    );
    
    VOID UpdatePosition
    (
        _In_ LARGE_INTEGER ilQPC
    );
    
    NTSTATUS SetCurrentWritePositionInternal
    (
        _In_  ULONG ulCurrentWritePosition
    );
    
    NTSTATUS GetPositions
    (
        _Out_opt_  ULONGLONG *      _pullLinearBufferPosition, 
        _Out_opt_  ULONGLONG *      _pullPresentationPosition, 
        _Out_opt_  LARGE_INTEGER *  _pliQPCTime
    );

    NTSTATUS ReadRegistrySettings();
    
};
typedef CMiniportWaveRTStream *PCMiniportWaveRTStream;
#endif // _SIMPLEAUDIOSAMPLE_MINWAVERTSTREAM_H_

