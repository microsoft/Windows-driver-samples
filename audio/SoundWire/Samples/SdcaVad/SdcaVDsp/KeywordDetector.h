/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    KeywordDetector.h

Abstract:

    Sample Keyword Detector.


--*/

#pragma once

#include <wil\resource.h>
#include "ContosoEventDetector.h"
#include "soundwirecontroller.h"
#include "sdcastreaming.h"

#define KEYWORDDETECTOR_POOLTAG 'KWS0'

typedef struct
{
    VAD_DESCRIPTOR          Descriptor;
    WAVEFORMATEXTENSIBLE    ExtraFormats[10];
} VAD_DESCRIPTOR_FORMAT, * PVAD_DESCRIPTOR_FORMAT;

typedef struct
{
    ULONG       EntitiesCount;
    ENTITY_INFO ExtraEntities[25];
} VAD_ENTITIES_EXTRA, * PVAD_ENTITIES_EXTRA;

class CKeywordDetector
{
public:
    CKeywordDetector(_In_ WDFDEVICE Device, _In_ ACXCIRCUIT Circuit, _In_ PWAVEFORMATEXTENSIBLE Format);

    ~CKeywordDetector();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS Initialize();

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
    NTSTATUS SetArmed(_In_ GUID eventId, _In_ BOOLEAN Arm);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID NotifyDetection();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetArmed(_In_ GUID eventId, _Out_ BOOLEAN *Arm);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID Run();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID Stop();

    _IRQL_requires_min_(DISPATCH_LEVEL)
    VOID DpcRoutine(_In_ LONGLONG PerformanceCounter, _In_ LONGLONG PerformanceFrequency, _Out_ BOOLEAN *isRealtime, _Out_ LONGLONG *NewPacketNumber, _Out_ ULONGLONG *NewPerformanceCount);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetReadPacket(_In_ ULONG PacketCount, _In_  ULONG PacketSize, _In_reads_(PacketSize) PVOID *Packets, _Out_ ULONG *PacketNumber,
                            _Out_ ULONGLONG *PerformanceCount, _Out_ BOOLEAN *MoreData, _Out_ ULONG *NextPacketNumber, _Out_ ULONGLONG *NextPerformanceCount);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS GetFifoStart(_Out_ ULONG *PacketNumber, _Out_ ULONGLONG *PerformanceCount);

private:

    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID UpdateBufferingState();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS ReadKeywordTimestampRegistry();

    PAGED_CODE_SEG
    NTSTATUS
    SendPropertyTo
    (
        _In_ GUID PropertySet,
        _In_ ULONG PropertyId,
        _In_ ACX_PROPERTY_VERB Verb,
        _In_ PVOID Control,
        _In_ ULONG ControlCb,
        _Inout_ PVOID Value,
        _In_ ULONG ValueCb,
        _Out_ ULONG_PTR* Information
    );


    PAGED_CODE_SEG
    NTSTATUS
    GetDeviceFunctionInformation(
        _Out_ PSDCA_FUNCTION_INFORMATION_LIST *FunctionInfo
        );

    PAGED_CODE_SEG
    NTSTATUS
    GetDeviceKwsCapabilityDescriptor(
        _Out_ PDEVICE_KWS_CAPABILITY_DESCRIPTOR Descriptor
        );
    
    PAGED_CODE_SEG
    NTSTATUS
    GetVadDescriptor(
        _Out_ PVAD_DESCRIPTOR_FORMAT Descriptor
        );

    PAGED_CODE_SEG
    NTSTATUS
    GetVadEntities    (
        _Out_ PVAD_ENTITIES_EXTRA Entities
        );
        
    PAGED_CODE_SEG
    NTSTATUS
    SetSuspendAccessEvent(
        _In_ PSDCA_KWS_NOTIFICATIONS Events
        );

    PAGED_CODE_SEG
    _Requires_lock_held_(m_csLock)
    NTSTATUS
    UpdateVadStreamState();

    PAGED_CODE_SEG
    _Requires_lock_held_(m_csLock)
    NTSTATUS
    ConfigureVadPort(
        _In_ PSDCA_KWS_PREPARE_PARAMS PrepareParams
        );
    
    PAGED_CODE_SEG
    _Requires_lock_held_(m_csLock)
    NTSTATUS
    CleanupVadPort(
        );

    static KSTART_ROUTINE s_HandleNotifications;

    PAGED_CODE_SEG
    void
    HandleNotifications();

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

    // set at initialization, safe to use in all threads
    WDFDEVICE                                       m_Device;
    ACXCIRCUIT                                      m_Circuit;
    LONGLONG                                        m_qpcFrequency;
    BOOLEAN                                         m_Initialized;
    SDCA_KWS_PREPARE_PARAMS                         m_PrepareParams;
    PSDCA_FUNCTION_INFORMATION_LIST                 m_FunctionInformation;
    DEVICE_KWS_CAPABILITY_DESCRIPTOR                m_CapabilityDescriptor;
    VAD_DESCRIPTOR_FORMAT                           m_VadDescriptor;
    SDCA_KWS_NOTIFICATIONS                          m_Events;
    PACKET_ENTRY                                    m_PacketPool[1 * SamplesPerSecond / SamplesPerPacket];    // Enough storage for 1 second of audio data
    VAD_ENTITIES_EXTRA                              m_VadEntities;

    // single thread access, no lock necessary
    LONGLONG                                        m_SoundDetectorData1;
    LONGLONG                                        m_SoundDetectorData2;
    ULONGLONG                                       m_ullKeywordStartTimestamp;
    ULONGLONG                                       m_ullKeywordStopTimestamp;
    BOOLEAN                                         m_streamRunning;

    // the following state variables are shared between dpc and stream state
    KSPIN_LOCK                                      m_BufferingStateSpinLock;
    _Guarded_by_(m_BufferingStateSpinLock)
    LONGLONG                                        m_qpcStartCapture;
    _Guarded_by_(m_BufferingStateSpinLock)
    LONGLONG                                        m_nLastQueuedPacket;

    // protected through interlocked access to the packet pool 
    KSPIN_LOCK                                      m_PacketPoolSpinLock;
    LIST_ENTRY                                      m_PacketPoolHead;

    // protected through interlocked access to the packet pool 
    KSPIN_LOCK                                      m_PacketFifoSpinLock;
    LIST_ENTRY                                      m_PacketFifoHead;


    // the following variables are shared with the sdca notification event
    // handler thread, and are protected by m_csLock
    mutable wil::fast_mutex_with_critical_region    m_csLock;
    PETHREAD                                        m_dispatchThread;
    mutable wil::kernel_event_auto_reset            m_threadExitEvent;
    mutable wil::kernel_event_manual_reset          m_threadExitedEvent{ true };

    _Guarded_by_(m_csLock)
    BOOLEAN                                         m_Prepared;

    _Guarded_by_(m_csLock)
    BOOLEAN                                         m_Suspended;

    _Guarded_by_(m_csLock)
    BOOLEAN                                         m_SoundDetectorArmed1;

    _Guarded_by_(m_csLock)
    BOOLEAN                                         m_SoundDetectorArmed2;
};

