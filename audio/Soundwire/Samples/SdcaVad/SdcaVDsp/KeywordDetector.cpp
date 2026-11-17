/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    KeywordDetector.cpp

Abstract:

    Sample keyword detector management.

Environment:

    Kernel mode

--*/

#include "private.h"
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "streamengine.h"

#include "KeywordDetector.h"

#ifndef __INTELLISENSE__
#include "KeywordDetector.tmh"
#endif


#pragma code_seg("PAGE")
CKeywordDetector::CKeywordDetector(
    _In_ WDFDEVICE Device, 
    _In_ ACXCIRCUIT Circuit,
    _In_ WAVEFORMATEXTENSIBLE *DetectionFormat
)
    :
    m_streamRunning(FALSE),
    m_qpcStartCapture(0),
    m_nLastQueuedPacket(-1),
    m_SoundDetectorArmed1(FALSE),
    m_SoundDetectorArmed2(FALSE),
    m_SoundDetectorData1(0),
    m_SoundDetectorData2(0),
    m_ullKeywordStartTimestamp(0),
    m_ullKeywordStopTimestamp(0),
    m_Device(Device),
    m_Circuit(Circuit),
    m_Prepared(FALSE),
    m_Suspended(FALSE),
    m_dispatchThread(nullptr),
    m_FunctionInformation(nullptr),
    m_Initialized(FALSE)
{
    PAGED_CODE();
    DSP_CIRCUIT_CONTEXT *circuitCtx;

    memcpy(&(m_PrepareParams.DetectionFormat), DetectionFormat, sizeof(WAVEFORMATEXTENSIBLE));
    circuitCtx = GetDspCircuitContext(m_Circuit);
    m_PrepareParams.EndpointId = circuitCtx->EndpointId;

    // Assume streaming (bypass) mode
    m_PrepareParams.VadMode = VadModeStreaming;

    KeInitializeEvent(&(m_Events.Suspend), SynchronizationEvent, FALSE);
    KeInitializeEvent(&(m_Events.Resume), SynchronizationEvent, FALSE);

    // Initialize our pool of packets and the list structures
    // The packet spin locks protect the producer/consumer relationship
    // between the dpc routine and GetReadPacket
    KeInitializeSpinLock(&m_PacketPoolSpinLock);
    KeInitializeSpinLock(&m_PacketFifoSpinLock);

    // The buffering state spin lock protects the state variables
    // shared between the arm/disarm and the dpc routine
    KeInitializeSpinLock(&m_BufferingStateSpinLock);

    // current state is disarmed
    // reset fifo and buffering state
    UpdateBufferingState();
}

#pragma code_seg("PAGE")
CKeywordDetector::~CKeywordDetector()
{
    PAGED_CODE();

    m_threadExitEvent.set();
    m_threadExitedEvent.wait();

    if (m_FunctionInformation)
    {
        ExFreePool(m_FunctionInformation);
    }

    m_Initialized = FALSE;
}


#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::Initialize()
{
    PAGED_CODE();
    HANDLE handle;
    LARGE_INTEGER qpcFrequency;

    KeQueryPerformanceCounter(&qpcFrequency);
    m_qpcFrequency = qpcFrequency.QuadPart;

    // TODO: currently ignoring the results of these calls as to 
    // not break anything (since they all currently fail)

    // Retrieve capabilities to know ivad/evad, and
    // entity id's
    RETURN_NTSTATUS_IF_FAILED(GetDeviceKwsCapabilityDescriptor(&m_CapabilityDescriptor));
    RETURN_NTSTATUS_IF_FAILED(GetDeviceFunctionInformation(&m_FunctionInformation));
    RETURN_NTSTATUS_IF_TRUE(0 == m_CapabilityDescriptor.DataPathsSupported, STATUS_NOT_SUPPORTED);

    RETURN_NTSTATUS_IF_FAILED(GetVadDescriptor(&m_VadDescriptor));
    RETURN_NTSTATUS_IF_FAILED(GetVadEntities(&m_VadEntities));

    // create worker thread to handle suspended access
    RETURN_NTSTATUS_IF_FAILED(PsCreateSystemThread(&handle, THREAD_ALL_ACCESS, 0, 0, 0, CKeywordDetector::s_HandleNotifications, this));

    auto scope_exit([&handle]() {
            ZwClose(handle);
        });

    RETURN_NTSTATUS_IF_FAILED(ObReferenceObjectByHandleWithTag(handle, THREAD_ALL_ACCESS, nullptr, KernelMode, KEYWORDDETECTOR_POOLTAG, (PVOID*)&m_dispatchThread, nullptr));

    // set notification events
    RETURN_NTSTATUS_IF_FAILED(SetSuspendAccessEvent(&m_Events));

    
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void CKeywordDetector::s_HandleNotifications(PVOID context)
{
    PAGED_CODE();
    auto kws = static_cast<CKeywordDetector*>(context);
    kws->HandleNotifications();
}

_IRQL_requires_(PASSIVE_LEVEL)
void
CKeywordDetector::HandleNotifications()
{
    PAGED_CODE();
    NTSTATUS status{ STATUS_SUCCESS };
    PVOID waitObjects[] = { &m_Events.Suspend, &m_Events.Resume, m_threadExitEvent.get() };

    // start with the even reset to indicate that the thread is running
    m_threadExitedEvent.clear();
    while (true)
    {
        status = KeWaitForMultipleObjects(3, waitObjects, WaitAny, Executive, KernelMode, FALSE, nullptr, nullptr);
        if (STATUS_WAIT_0 == status)
        {
            auto lock = m_csLock.acquire();
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            m_Suspended = TRUE;
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            UpdateVadStreamState();
            continue;
        }
        if (STATUS_WAIT_1 == status)
        {
            auto lock = m_csLock.acquire();
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            m_Suspended = FALSE;
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            UpdateVadStreamState();
            continue;
        }

        else // consider as exit event
        {
            break;
        }
    }
    m_threadExitedEvent.set();
    PsTerminateSystemThread(status);
}


#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::ReadKeywordTimestampRegistry()
{
    PAGED_CODE();

    UNICODE_STRING              parametersPath;

    RTL_QUERY_REGISTRY_TABLE    paramTable[] = {
        // QueryRoutine     Flags                                               Name                            EntryContext                            DefaultType                                                     DefaultData                                 DefaultLength
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"KeywordDetectorStartTimestamp",   &m_ullKeywordStartTimestamp,    (REG_QWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_QWORD,  &m_ullKeywordStartTimestamp,        sizeof(ULONGLONG) },
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"KeywordDetectorStopTimestamp",    &m_ullKeywordStopTimestamp,     (REG_QWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_QWORD,  &m_ullKeywordStopTimestamp,         sizeof(ULONGLONG) },
        { NULL,   0,                                                        NULL,                               NULL,                                   0,                                                              NULL,                                       0 }
    };

    RtlInitUnicodeString(&parametersPath, NULL);

    // The sizeof(WCHAR) is added to the maximum length, for allowing a space for null termination of the string.
    parametersPath.MaximumLength =
        g_RegistryPath.Length + sizeof(L"\\Parameters") + sizeof(WCHAR);

#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "memory is freed by scope_exit")
    parametersPath.Buffer = (PWCH)ExAllocatePool2(PagedPool, parametersPath.MaximumLength, KEYWORDDETECTOR_POOLTAG);
    RETURN_NTSTATUS_IF_TRUE(parametersPath.Buffer == NULL, STATUS_INSUFFICIENT_RESOURCES);
    auto parametersPath_free = scope_exit([&parametersPath]() {
            PAGED_CODE();
            ExFreePool(parametersPath.Buffer);
        });

    RtlAppendUnicodeToString(&parametersPath, g_RegistryPath.Buffer);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

    RETURN_NTSTATUS_IF_FAILED(RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
        parametersPath.Buffer,
        &paramTable[0],
        NULL,
        NULL
    ));

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::ResetDetector(_In_ GUID eventId)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_TRUE(eventId != CONTOSO_KEYWORD1 &&
                            eventId != CONTOSO_KEYWORD2 &&
                            eventId != GUID_NULL,
                            STATUS_INVALID_PARAMETER);

    // Initialize detector on first use, which is going to be the 
    // initial reset of the detector.
    if(!m_Initialized)
    {
        RETURN_NTSTATUS_IF_FAILED(Initialize());
        m_Initialized = TRUE;
    }

    auto lock = m_csLock.acquire();

    if (eventId == CONTOSO_KEYWORD1)
    {
        m_SoundDetectorData1 = 0;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        m_SoundDetectorArmed1 = FALSE;
    }
    else if(eventId == CONTOSO_KEYWORD2)
    {
        m_SoundDetectorData2 = 0;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        m_SoundDetectorArmed2 = FALSE;
    }
    else if(eventId == GUID_NULL)
    {
        // When DownloadDetectorData is called to set the pattern for multiple keywords
        // at once, all keyword detectors must be reset. Also used during keyword detector
        // initialization and cleanup to restore it back to initial state and power down.
        m_SoundDetectorData1 = 0;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        m_SoundDetectorArmed1 = FALSE;
        m_SoundDetectorData2 = 0;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        m_SoundDetectorArmed2 = FALSE;
    }

#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "wil::fast_mutex lacks required SAL annotation, lock is held")
    RETURN_NTSTATUS_IF_FAILED(UpdateVadStreamState());

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::DownloadDetectorData(_In_ GUID eventId, _In_ LONGLONG Data)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_TRUE(eventId != CONTOSO_KEYWORD1 &&
                            eventId != CONTOSO_KEYWORD2 &&
                            eventId != GUID_NULL,
                            STATUS_INVALID_PARAMETER);


    // reset the detector for this event Id
    ResetDetector(eventId);

    // In this example, the driver supports detection data 
    // set with a single call for both detectors, or each
    // detector set individually.
    if (eventId == CONTOSO_KEYWORD1)
    {
        m_SoundDetectorData1 = Data;
    }
    else if(eventId == CONTOSO_KEYWORD2)
    {
        m_SoundDetectorData2 = Data;
    }
    else if(eventId == GUID_NULL)
    {
        // in this simplified example "Data" is set on both detectors,
        // however in a real system "Data" could be a data structure which
        // contains different values for each detector.
        m_SoundDetectorData1 = m_SoundDetectorData2 = Data;
    }

    return STATUS_SUCCESS;
}

// The following function is only applicable to single keyword detection systems,
// and assumes keyword detector #1.
#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::GetDetectorData(_In_ GUID eventId, _Out_ LONGLONG *Data)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_TRUE(eventId != CONTOSO_KEYWORD1 &&
                            eventId != CONTOSO_KEYWORD2 &&
                            eventId != GUID_NULL,
                            STATUS_INVALID_PARAMETER);

    *Data = 0;

    if (eventId == CONTOSO_KEYWORD1)
    {
        *Data = m_SoundDetectorData1;
    }
    else if(eventId == CONTOSO_KEYWORD2)
    {
        *Data = m_SoundDetectorData2;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONGLONG CKeywordDetector::GetStartTimestamp()
{
    PAGED_CODE();

    return m_ullKeywordStartTimestamp;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONGLONG CKeywordDetector::GetStopTimestamp()
{
    PAGED_CODE();

    return m_ullKeywordStopTimestamp;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::SetArmed(_In_ GUID eventId, _In_ BOOLEAN Arm)
{
    PAGED_CODE();

    BOOLEAN previousDetector1State = FALSE;
    BOOLEAN previousDetector2State = FALSE;

    RETURN_NTSTATUS_IF_TRUE(eventId != CONTOSO_KEYWORD1 &&
                            eventId != CONTOSO_KEYWORD2 &&
                            eventId != GUID_NULL,
                            STATUS_INVALID_PARAMETER);

    // lock scope enter
    {   
        auto lock = m_csLock.acquire();

        // the previous state is "armed" if either detector is armed.
        // this reflects the fact that both detectors are sharing the
        // same stream.
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        previousDetector1State = m_SoundDetectorArmed1;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        previousDetector2State = m_SoundDetectorArmed2;

        auto revertOnFailure = scope_exit([&]() {
                PAGED_CODE();
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
                m_SoundDetectorArmed1 = previousDetector1State;
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
                m_SoundDetectorArmed2 = previousDetector2State;
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "wil::fast_mutex lacks required SAL annotation, lock is held")
                UpdateVadStreamState();
            });

        if (eventId == CONTOSO_KEYWORD1)
        {
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            m_SoundDetectorArmed1 = Arm;
        }
        else if(eventId == CONTOSO_KEYWORD2)
        {
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
            m_SoundDetectorArmed2 = Arm;
        }

#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        RETURN_NTSTATUS_IF_FAILED(UpdateVadStreamState());

        revertOnFailure.release();
    }

    // Change buffering state if needed
    UpdateBufferingState();

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::GetArmed(_In_ GUID eventId, _Out_ BOOLEAN *Arm)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_TRUE(eventId != CONTOSO_KEYWORD1 &&
                            eventId != CONTOSO_KEYWORD2 &&
                            eventId != GUID_NULL,
                            STATUS_INVALID_PARAMETER);

    auto lock = m_csLock.acquire();

    *Arm = FALSE;

    if (eventId == CONTOSO_KEYWORD1)
    {
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        *Arm = m_SoundDetectorArmed1;
    }
    else if(eventId == CONTOSO_KEYWORD2)
    {
#pragma prefast(suppress:__WARNING_NEED_NO_COMPETING_THREAD, "wil::fast_mutex lacks required SAL annotation, lock is held")
        *Arm = m_SoundDetectorArmed2;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::Run()
{
    PAGED_CODE();
    m_streamRunning = TRUE;
    UpdateBufferingState();
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::Stop()
{
    PAGED_CODE();
    m_streamRunning = FALSE;
    UpdateBufferingState();
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::UpdateBufferingState()
{
    BOOL armed = FALSE;
    KIRQL irql = PASSIVE_LEVEL;

    {
        auto lock = m_csLock.acquire();
#pragma prefast(suppress:__WARNING_RACE_CONDITION, "wil::fast_mutex lacks required SAL annotation, lock is held")
        armed = m_SoundDetectorArmed1 | m_SoundDetectorArmed2;
    }

    // acquire buffering state spin lock to synchronize state changes with the running dpc routine
    KeAcquireSpinLock(&m_BufferingStateSpinLock, &irql);

    if (armed || m_streamRunning)
    {
        // if we're armed or stream running, and not buffering, start buffering
        // if m_qpcStartCapture is not 0, then it's already buffering
        if (m_qpcStartCapture == 0)
        {
            m_qpcStartCapture = KeQueryPerformanceCounter(NULL).QuadPart;
        }
    }
    else
    {
        // if we're disarmed and no stream running, reset buffering
        m_qpcStartCapture = 0;
        m_nLastQueuedPacket = (-1);
        InitializeListHead(&m_PacketPoolHead);
        InitializeListHead(&m_PacketFifoHead);
        
        for (int i = 0; i < ARRAYSIZE(m_PacketPool); i++)
        {
            InsertTailList(&m_PacketPoolHead, &m_PacketPool[i].ListEntry);
        }
    }

    KeReleaseSpinLock(&m_BufferingStateSpinLock, irql);
    
    return;
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID CKeywordDetector::NotifyDetection()
{
    KIRQL irql = PASSIVE_LEVEL;

    // Because we are modifying shared buffer state to simulate a notification,
    // we need to acquire the spin lock to synchronize this with the dpc routine
    KeAcquireSpinLock(&m_BufferingStateSpinLock, &irql);

    // A detection will only happen if armed and the
    // stream is already running. If there isn't a client
    // running, then set the stream start time to align
    // with this detection.
    if (!m_streamRunning)
    {
        // the start capture time is now.
        m_qpcStartCapture = KeQueryPerformanceCounter(NULL).QuadPart;

        // The following code is for testing purposes only.
        // m_qpcFrequency is defined to be the number of ticks in 1 second.
        // Use the stream start time (the current time retrieved in StartBufferStream) to 
        // mark when the keyword ended, and the start time minus 1 second worth of ticks
        // to mark when the keyword started. Also, adjust the stream start time to align
        // to this new keyword start time, so that the simulated stream contains the full keyword.

        m_ullKeywordStopTimestamp = m_qpcStartCapture; // stop time is the current time
        m_qpcStartCapture = m_qpcStartCapture - m_qpcFrequency; // buffer start time is 1 second ago
        m_ullKeywordStartTimestamp = m_qpcStartCapture; // buffer start time = keyword start time

    }
    else
    {
        // The following code is for testing purposes only.
        // If the stream is running, we cannot modify qpcStartCapture to be in
        // the past, so instead make the keyword start & stop times fit within the
        // time period that the keyword has been running. If it has been running
        // for more than 1 second, then set the keyword start time to be 1 second back
        // into the stream, as though we just figured out there was a keyword there.
        // If it has been running less than one second, then the keyword size ends
        // up being however long the stream has been running. 

        LARGE_INTEGER qpc;
        qpc = KeQueryPerformanceCounter(NULL);

        m_ullKeywordStopTimestamp = qpc.QuadPart; // stop time is the current time

        if (m_qpcStartCapture < (qpc.QuadPart - m_qpcFrequency))
        {
            m_ullKeywordStartTimestamp = (qpc.QuadPart - m_qpcFrequency); 
        }
        else
        {
            m_ullKeywordStartTimestamp = m_qpcStartCapture;
        }
    }

    KeReleaseSpinLock(&m_BufferingStateSpinLock, irql);

    return;
}

#pragma code_seg()
_IRQL_requires_min_(DISPATCH_LEVEL)
VOID CKeywordDetector::DpcRoutine(
                                _In_ LONGLONG PerformanceCounter,
                                _In_ LONGLONG PerformanceFrequency,
                                _Out_ BOOLEAN *isRealtime,
                                _Out_ LONGLONG *NewPacketNumber,
                                _Out_ ULONGLONG *NewPerformanceCount)
{
    LONGLONG currentPacket;
    LONGLONG packetsToQueue;

    KIRQL irql = PASSIVE_LEVEL;

    // used to synchronize buffering state variables with stream state changes,
    // arming changes, etc.
    KeAcquireSpinLock(&m_BufferingStateSpinLock, &irql);

    *isRealtime = FALSE;
    *NewPacketNumber = 0;
    *NewPerformanceCount = 0;

    // TODO: the timer only runs when the stream is open, but really for KWS it should be building up a collection of burst data
    // in the queue from 1.5 sec before the trigger happens. Is there some way to simulate that behavior here? Without doing that,
    // there isn't really a burst that happens, just a trickle because while the timestamps will be right, the queue won't contain
    // anything until the timer fires at the normal rate.

    if (m_qpcStartCapture > 0)
    {
        currentPacket = (PerformanceCounter - m_qpcStartCapture) * (SamplesPerSecond / SamplesPerPacket) / PerformanceFrequency;
        packetsToQueue = currentPacket - m_nLastQueuedPacket;

        // If the fifo is empty, and we're going to add something, then we are realtime
        *isRealtime = IsListEmpty(&m_PacketFifoHead) && packetsToQueue > 0;

        *NewPacketNumber = m_nLastQueuedPacket+1;
        *NewPerformanceCount = m_qpcStartCapture + (*NewPacketNumber * m_qpcFrequency * SamplesPerPacket / SamplesPerSecond);

        while (packetsToQueue > 0)
        {
            LIST_ENTRY*     packetListEntry;
            PACKET_ENTRY*   packetEntry;

            do
            {
                packetListEntry = ExInterlockedRemoveHeadList(&m_PacketPoolHead, &m_PacketPoolSpinLock);
                if (packetListEntry != NULL) break;

                // Pool is empty, no room to buffer more, an overrun is occurring. Drop and reuse the
                // oldest packet from head of fifo.

                // Since the pool is empty, the fifo should be full. However, although unlikely, the
                // driver might empty the fifo before this routine removes a packet. In that case, the
                // pool should have packets available again. Therefore this is a retry loop.
                packetListEntry = ExInterlockedRemoveHeadList(&m_PacketFifoHead, &m_PacketFifoSpinLock);
                if (packetListEntry != NULL) break;
            } while (TRUE);

            packetEntry = CONTAINING_RECORD(packetListEntry, PACKET_ENTRY, ListEntry);

            packetEntry->PacketNumber = ++m_nLastQueuedPacket;
            packetEntry->QpcWhenSampled = m_qpcStartCapture + (packetEntry->PacketNumber * PerformanceFrequency * SamplesPerPacket / SamplesPerSecond);

            // TODO: this should really put something real in the buffer. Use the sine tone generator maybe?
            RtlZeroMemory(&packetEntry->Samples[0], sizeof(packetEntry->Samples));

            ExInterlockedInsertTailList(&m_PacketFifoHead, packetListEntry, &m_PacketFifoSpinLock);

            packetsToQueue -= 1;
        }
    }
    
    KeReleaseSpinLock(&m_BufferingStateSpinLock, irql);
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::GetFifoStart(_Out_ ULONG *PacketNumber, _Out_ ULONGLONG *PerformanceCount)
{
    NTSTATUS status = STATUS_DEVICE_NOT_READY;
    KIRQL irql = PASSIVE_LEVEL;

    // acquire the fifo spin lock in order to safely inspect the head of the fifo
    KeAcquireSpinLock(&m_PacketFifoSpinLock, &irql);

    // peek at the first entry in the list, and retrieve the required packet number and qpc for it
    if (!IsListEmpty(m_PacketFifoHead.Flink))
    {
        PACKET_ENTRY *packetEntry;

        packetEntry = CONTAINING_RECORD(m_PacketFifoHead.Flink, PACKET_ENTRY, ListEntry);

        status = RtlLongLongToULong(packetEntry->PacketNumber, PacketNumber);
        if (NT_SUCCESS(status))
        {
            *PerformanceCount = packetEntry->QpcWhenSampled;
            status = STATUS_SUCCESS;
        }
    }

    KeReleaseSpinLock(&m_PacketFifoSpinLock, irql);

    return status;
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CKeywordDetector::GetReadPacket
(
    _In_ ULONG PacketCount,
    _In_ ULONG PacketSize,
    _In_reads_(PacketSize) PVOID *Packets,
    _Out_ ULONG *PacketNumber,
    _Out_ ULONG64 *PerformanceCounterValue,
    _Out_ BOOLEAN *MoreData,
    _Out_ ULONG *NextPacketNumber,
    _Out_ ULONGLONG *NextPerformanceCount
)
{
    NTSTATUS status = STATUS_DEVICE_NOT_READY;
    LIST_ENTRY *packetListEntry = NULL;

    // This call is synchronized with the dpc routine through the packet list
    // spin locks, as a producer consumer relationship.
    // buffering state variables are not available, and taking the buffering state
    // lock here would introduce lock contention between the producer and the consumer.
    *PacketNumber = 0;
    *PerformanceCounterValue = 0;
    *MoreData = FALSE;
    *NextPacketNumber = 0;
    *NextPerformanceCount = 0;

    packetListEntry = ExInterlockedRemoveHeadList(&m_PacketFifoHead, &m_PacketFifoSpinLock);
    if (packetListEntry != NULL)
    {
        BYTE *packetData;
        PACKET_ENTRY *packetEntry;

        packetEntry = CONTAINING_RECORD(packetListEntry, PACKET_ENTRY, ListEntry);

        status = RtlLongLongToULong(packetEntry->PacketNumber, PacketNumber);
        if (NT_SUCCESS(status))
        {
            packetData = (PBYTE) Packets[(*PacketNumber) % PacketCount];

            *PerformanceCounterValue = packetEntry->QpcWhenSampled;

            if (NT_SUCCESS(GetFifoStart(NextPacketNumber, NextPerformanceCount)))
            {
                *MoreData = TRUE;
            }

            // TODO: the packet size here needs to line up to the packet size allocated.
            // Also, handle the first packet offset
            RtlCopyMemory(packetData, packetEntry->Samples, min(sizeof(packetEntry->Samples), PacketSize));
        }

        ExInterlockedInsertTailList(&m_PacketPoolHead, packetListEntry, &m_PacketPoolSpinLock);
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::SendPropertyTo
(
    _In_ GUID PropertySet,
    _In_ ULONG PropertyId,
    _In_ ACX_PROPERTY_VERB Verb,
    _In_ PVOID Control,
    _In_ ULONG ControlCb,
    _Inout_ PVOID Value,
    _In_ ULONG ValueCb,
    _Out_ ULONG_PTR* Information
)
{
    PAGED_CODE();

    ACXPIN pin;
    pin = AcxCircuitGetPinById(m_Circuit, DspCapturePinTypeBridge);
    RETURN_NTSTATUS_IF_TRUE(pin == NULL, STATUS_INVALID_PARAMETER);

    DSP_PIN_CONTEXT* pinCtx = GetDspPinContext(pin);
    RETURN_NTSTATUS_IF_TRUE(pinCtx == NULL, STATUS_INVALID_PARAMETER);

    RETURN_NTSTATUS_IF_TRUE(pinCtx->TargetCircuit == NULL, STATUS_INVALID_DEVICE_STATE);

    ACX_REQUEST_PARAMETERS requestParams;
    ACX_REQUEST_PARAMETERS_INIT_PROPERTY(
        &requestParams,
        PropertySet,
        PropertyId,
        Verb,
        AcxItemTypeCircuit,
        0,
        Control, ControlCb,
        Value, ValueCb
    );

    WDFREQUEST request;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Device;
    RETURN_NTSTATUS_IF_FAILED(WdfRequestCreate(&attributes, AcxTargetCircuitGetWdfIoTarget(pinCtx->TargetCircuit), &request));
    
    auto request_free = scope_exit([&request]() {
        WdfObjectDelete(request);
        });

    RETURN_NTSTATUS_IF_FAILED(AcxTargetCircuitFormatRequestForProperty(pinCtx->TargetCircuit, request, &requestParams));

    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(5));

    RETURN_NTSTATUS_IF_TRUE(!WdfRequestSend(request, AcxTargetCircuitGetWdfIoTarget(pinCtx->TargetCircuit), &sendOptions), STATUS_INVALID_DEVICE_REQUEST);

    NTSTATUS status = (WdfRequestGetStatus(request));

    if (Information)
    {
        *Information = WdfRequestGetInformation(request);
    }
    if (status == STATUS_BUFFER_OVERFLOW && ValueCb == 0)
    {
        // Don't trace this error, it's normal
        return status;
    }

    RETURN_NTSTATUS_IF_FAILED(status);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::GetDeviceFunctionInformation
(
    _Out_ PSDCA_FUNCTION_INFORMATION_LIST *FunctionInfo
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR requiredBufferSize = 0;

    RETURN_NTSTATUS_IF_TRUE(nullptr == FunctionInfo, STATUS_INVALID_PARAMETER);

    status = SendPropertyTo(KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_FUNCTION_INFORMATION,
        AcxPropertyVerbGet,
        nullptr, 0,
        nullptr, 0,
        &requiredBufferSize);

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        // expect a buffer overflow error, confirm size is valid
        if (requiredBufferSize >= sizeof(SDCA_FUNCTION_INFORMATION_LIST))
        {
            // size is valid, allocate and retrieve
            *FunctionInfo = (PSDCA_FUNCTION_INFORMATION_LIST) ExAllocatePool2(POOL_FLAG_NON_PAGED, requiredBufferSize, DRIVER_TAG);
            RETURN_NTSTATUS_IF_TRUE(nullptr == *FunctionInfo, STATUS_INSUFFICIENT_RESOURCES);
            status = SendPropertyTo(KSPROPERTYSETID_Sdca,
                KSPROPERTY_SDCA_FUNCTION_INFORMATION,
                AcxPropertyVerbGet,
                nullptr, 0,
                *FunctionInfo, sizeof(SDCA_FUNCTION_INFORMATION_LIST),
                nullptr);
        }
        else
        {
            // correct buffer overflow error, but size is wrong
            RETURN_NTSTATUS_IF_FAILED(STATUS_UNSUCCESSFUL);
        }
    }
    else if (NT_SUCCESS(status))
    {
        // call should not succeeded with a null buffer pointer
        RETURN_NTSTATUS_IF_FAILED(STATUS_INVALID_DEVICE_REQUEST);
    }

    RETURN_NTSTATUS_IF_FAILED(status);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::GetDeviceKwsCapabilityDescriptor
(
    _Out_ PDEVICE_KWS_CAPABILITY_DESCRIPTOR Descriptor
)
{
    PAGED_CODE();

    memset(Descriptor, 0, sizeof(DEVICE_KWS_CAPABILITY_DESCRIPTOR));
    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_DEVICE_CAPABILITY,
        AcxPropertyVerbGet,
        nullptr, 0,
        Descriptor, sizeof(DEVICE_KWS_CAPABILITY_DESCRIPTOR),
        nullptr));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::GetVadDescriptor(
    _Out_ PVAD_DESCRIPTOR_FORMAT Descriptor
    )
{
    PAGED_CODE();

    // For simplicity, this sample code uses a static-sized VAD_DESCRIPTOR
    // with room for 11 total formats. This still has the potential to
    // fail if the target device supports more than that many formats for VAD
    memset(Descriptor, 0, sizeof(VAD_DESCRIPTOR_FORMAT));
    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_VAD_CAPABILITY,
        AcxPropertyVerbGet,
        nullptr, 0,
        Descriptor, sizeof(VAD_DESCRIPTOR_FORMAT),
        nullptr));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::GetVadEntities(
    _Out_ PVAD_ENTITIES_EXTRA Entities
    )
{
    PAGED_CODE();

    // For simplicity, this sample code uses a static-sized VAD_ENTITIES
    // with room for 25 total elements. This still has the potential to
    // fail if the target device has more than 25 elements.
    memset(Entities, 0, sizeof(VAD_ENTITIES_EXTRA));
    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_VAD_ENTITIES,
        AcxPropertyVerbGet,
        nullptr, 0,
        Entities, sizeof(VAD_ENTITIES_EXTRA),
        nullptr));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
CKeywordDetector::SetSuspendAccessEvent
(
    _In_ PSDCA_KWS_NOTIFICATIONS Events
)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_ACCESS_EVENTS,
        AcxPropertyVerbSet,
        nullptr, 0,
        Events, sizeof(SDCA_KWS_NOTIFICATIONS),
        nullptr));

    return STATUS_SUCCESS;
}

// There are three states. Disarmed, Armed and Suspended,
// and Armed and Prepared.

// If we're Disarmed, we need to clean up the vad stream, suspend/resume
// state doesn't matter.

// if we're Armed and Suspended, we should be detecting but are experiencing
// a period of deafness due to the codec driver needing to access the hardware.
// So, we need to clean up the vad stream and wait for the resume notification
// to recreate the vad stream

// if we're Armed and Prepared, then we're actively detecting, so we
// need the stream prepared.
PAGED_CODE_SEG
_Requires_lock_held_(m_csLock)
NTSTATUS
CKeywordDetector::UpdateVadStreamState()
{
    PAGED_CODE();

    if (m_SoundDetectorArmed1 || m_SoundDetectorArmed2)
    {
        // if we're armed, not prepared, and not suspended, then
        // we need to move to the armed and prepared state.
        if (!m_Prepared && !m_Suspended)
        {
            // To move into the Armed and Prepared state
            // we need to prepare the vad stream
            RETURN_NTSTATUS_IF_FAILED(ConfigureVadPort(&m_PrepareParams));
        }
        // if we are armed, prepared, and suspended, then
        // we need to move to the armed and suspended state
        else if (m_Prepared && m_Suspended)
        {
            // To move into the Armed and Suspended state
            // we need to cleanup the VAD stream
            RETURN_NTSTATUS_IF_FAILED(CleanupVadPort());
        }
        // else
        // if we are armed, prepared, and not suspended, then we are in
        // the armed and prepared state, nothing else to do.

        // Or, if we are armed, not prepared, and suspended, then we are in
        // the armed and suspended state, nothing else to do.
    }
    else
    {
        if (m_Prepared)
        {
            // moving into the disarmed state
            RETURN_NTSTATUS_IF_FAILED(CleanupVadPort());
        }
    }

    return STATUS_SUCCESS;
}


PAGED_CODE_SEG
_Requires_lock_held_(m_csLock)
NTSTATUS
CKeywordDetector::ConfigureVadPort
(
    _In_ PSDCA_KWS_PREPARE_PARAMS PrepareParams
)
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_CONFIGURE_VAD_PORT,
        AcxPropertyVerbSet,
        nullptr, 0,
        PrepareParams, sizeof(SDCA_KWS_PREPARE_PARAMS),
        nullptr));

    m_Prepared = TRUE;

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
_Requires_lock_held_(m_csLock)
NTSTATUS
CKeywordDetector::CleanupVadPort(    )
{
    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(SendPropertyTo(KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_CLEANUP_VAD_PORT,
        AcxPropertyVerbSet,
        nullptr, 0,
        NULL, 0,
        nullptr));

    m_Prepared = FALSE;

    return STATUS_SUCCESS;
}

