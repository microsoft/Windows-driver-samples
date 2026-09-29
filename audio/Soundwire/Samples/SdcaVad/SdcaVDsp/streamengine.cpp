/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    StreamEngine.cpp

Abstract:

    Virtual Streaming Engine - this module controls streaming logic for
    the device.

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "streamengine.h"

#ifndef __INTELLISENSE__
#include "streamengine.tmh"
#endif

_Use_decl_annotations_
PAGED_CODE_SEG
CStreamEngine::CStreamEngine(
    ACXSTREAM Stream,
    ACXDATAFORMAT StreamFormat,
    CSimPeakMeter *circuitPeakmeter
    )
    : m_PacketsCount(0),
      m_PacketSize(0),
      m_FirstPacketOffset(0),
      m_NotificationTimer(NULL),
      m_CurrentState(AcxStreamStateStop),
      m_CurrentPacket(0),
      m_Position(0),
      m_Stream(Stream),
      m_StreamFormat(StreamFormat),
      m_StartTime(0),
      m_StartPosition(0),
      m_GlitchAdjust(0),
      m_pCircuitPeakmeter(circuitPeakmeter)
{
    PAGED_CODE();

    KeQueryPerformanceCounter(&m_PerformanceCounterFrequency);
    RtlZeroMemory(m_Packets, sizeof(m_Packets));
}

_Use_decl_annotations_
#pragma code_seg()
CStreamEngine::~CStreamEngine()
{
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::AllocateRtPackets(
    ULONG           PacketCount,
    ULONG           PacketSize,
    PACX_RTPACKET * Packets
    )
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    PVOID packetBuffer = NULL;
    PACX_RTPACKET packets = NULL;

    auto exit = scope_exit([&]() {
        if (packetBuffer)
        {
            ExFreePoolWithTag(packetBuffer, DRIVER_TAG);
        }
        if (packets)
        {
            FreeRtPackets(packets, PacketCount);
        }
    });

    RETURN_NTSTATUS_IF_TRUE(PacketCount > MAX_PACKET_COUNT, STATUS_INVALID_PARAMETER);

    size_t packetsSize = 0;
    RETURN_NTSTATUS_IF_FAILED(RtlSizeTMult(PacketCount, sizeof(ACX_RTPACKET), &packetsSize));
    
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "On error packets gets freed inside scope_exit.")
    packets = (PACX_RTPACKET)ExAllocatePool2(POOL_FLAG_NON_PAGED, packetsSize, DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(!packets, STATUS_NO_MEMORY);

    // ExAllocatePool2 zeros memory.

    // We need to allocate page-aligned buffers, to ensure no kernel memory leaks
    // to user space. Round up the packet size to page aligned, then calculate
    // the first packet's buffer offset so packet 0 ends on a page boundary and
    // packet 1 begins on a page boundary.
    ULONG packetAllocSizeInPages = 0;
    ULONG packetAllocSizeInBytes = 0;
    ULONG firstPacketOffset = 0;
    RETURN_NTSTATUS_IF_FAILED(RtlULongAdd(PacketSize, PAGE_SIZE - 1, &packetAllocSizeInPages));

    packetAllocSizeInPages = packetAllocSizeInPages / PAGE_SIZE;
    packetAllocSizeInBytes = PAGE_SIZE * packetAllocSizeInPages;
    firstPacketOffset = packetAllocSizeInBytes - PacketSize;

    ULONG i;
    for (i = 0; i < PacketCount; ++i)
    {
        PMDL pMdl = NULL;

        ACX_RTPACKET_INIT(&packets[i]);

        packetBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, packetAllocSizeInBytes, DRIVER_TAG);
        RETURN_NTSTATUS_IF_TRUE(packetBuffer == NULL, STATUS_NO_MEMORY);

        // ExAllocatePool2 zeros memory.

        pMdl = IoAllocateMdl(packetBuffer, packetAllocSizeInBytes, FALSE, FALSE, NULL);
        RETURN_NTSTATUS_IF_TRUE(pMdl == NULL, STATUS_NO_MEMORY);

        MmBuildMdlForNonPagedPool(pMdl);

        WDF_MEMORY_DESCRIPTOR_INIT_MDL(
            &((packets)[i].RtPacketBuffer),
            pMdl,
            packetAllocSizeInBytes);

        packets[i].RtPacketSize = PacketSize;
        if (i == 0)
        {
            packets[i].RtPacketOffset = firstPacketOffset;
        }
        else
        {
            packets[i].RtPacketOffset = 0;
        }
        m_Packets[i] = packetBuffer;

        packetBuffer = NULL;
    }

    *Packets = packets;
    packets = NULL;
    m_PacketsCount = PacketCount;
    m_PacketSize = PacketSize;
    m_FirstPacketOffset = firstPacketOffset;

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CStreamEngine::FreeRtPackets(
    PACX_RTPACKET   Packets,
    ULONG           PacketCount
)
{
    ULONG i;
    PVOID buffer;

    PAGED_CODE();

    for (i = 0; i < PacketCount; ++i)
    {
        if (Packets[i].RtPacketBuffer.u.MdlType.Mdl)
        {
            buffer = MmGetMdlVirtualAddress(Packets[i].RtPacketBuffer.u.MdlType.Mdl);
            IoFreeMdl(Packets[i].RtPacketBuffer.u.MdlType.Mdl);
            ExFreePool(buffer);
        }
    }

    ExFreePool(Packets);
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::PrepareHardware()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;
    WDF_TIMER_CONFIG_INIT(&timerConfig, CStreamEngine::s_EvtStreamPassCallback);
    timerConfig.AutomaticSerialization = TRUE;
    timerConfig.UseHighResolutionTimer = WdfTrue;
    timerConfig.Period = 0;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&timerAttributes, STREAM_TIMER_CONTEXT);
    timerAttributes.ParentObject = m_Stream;

    RETURN_NTSTATUS_IF_FAILED(WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        &m_NotificationTimer
    ));

    PSTREAM_TIMER_CONTEXT timerCtx;
    timerCtx = GetStreamTimerContext(m_NotificationTimer);
    timerCtx->StreamEngine = this;

    m_CurrentState = AcxStreamStatePause;

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::ReleaseHardware()
{
    PAGED_CODE();

    if (m_NotificationTimer)
    {
        WdfTimerStop(m_NotificationTimer, TRUE);
        WdfObjectDelete(m_NotificationTimer);
        m_NotificationTimer = NULL;
    }

    KeFlushQueuedDpcs();

    m_Position = 0;
    m_GlitchAdjust = 0;
    m_CurrentPacket = 0;

    m_CurrentState = AcxStreamStateStop;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::Pause()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"CStreamEngine::Pause - from %d", m_CurrentState);

    RETURN_NTSTATUS_IF_TRUE(m_CurrentState != AcxStreamStateRun, STATUS_INVALID_STATE_TRANSITION);

    m_PeakMeter.StopStream();
    if (m_pCircuitPeakmeter)
    {
        m_pCircuitPeakmeter->StopStream();
    }

    WdfTimerStop(m_NotificationTimer, TRUE);

    // Save the position we paused at.
    UpdatePosition();

    m_CurrentState = AcxStreamStatePause;

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::Run()
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"CStreamEngine::Run");

    if (m_CurrentState != AcxStreamStatePause)
    {
        status = STATUS_INVALID_STATE_TRANSITION;
        return status;
    }

    m_PeakMeter.StartStream();
    if (m_pCircuitPeakmeter)
    {
        m_pCircuitPeakmeter->StartStream();
    }

    // Save the time and position - if we ran and paused previously, the StartTime and StartPosition will allow
    // us to continue scheduling packet completions correctly, while still reporting absolute position from the
    // start of the stream.
    m_StartTime = KSCONVERT_PERFORMANCE_TIME(m_PerformanceCounterFrequency.QuadPart, KeQueryPerformanceCounter(NULL));
    m_StartPosition = m_Position;

    // Reset time we've lost to glitches
    m_GlitchAdjust = 0;

    ScheduleNextPass();

    m_CurrentState = AcxStreamStateRun;

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::GetPresentationPosition(
    PULONGLONG      PositionInBlocks,
    PULONGLONG      QPCPosition
)
{
    PAGED_CODE();

    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"CStreamEngine::GetPresentationPosition");

    ULONG blockAlign;
    LARGE_INTEGER qpc;

    blockAlign = AcxDataFormatGetBlockAlign(m_StreamFormat);
    qpc = KeQueryPerformanceCounter(NULL);

    // Update the position based on the current time
    UpdatePosition();

    *PositionInBlocks = m_Position / blockAlign;

    *QPCPosition = (ULONGLONG)qpc.QuadPart;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::AssignDrmContentId(
    ULONG          DrmContentId,
    PACXDRMRIGHTS  DrmRights
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DrmContentId);
    UNREFERENCED_PARAMETER(DrmRights);
    
    //
    // At this point the driver should enforce the new DrmRights.
    //
    // HDMI render: if DigitalOutputDisable or CopyProtect is true, enable HDCP.
    // 
    // From MSDN:
    //
    // This sample doesn't forward protected content, but if your driver uses 
    // lower layer drivers or a different stack to properly work, please see the 
    // following info from MSDN:
    //
    // "Before allowing protected content to flow through a data path, the system
    // verifies that the data path is secure. To do so, the system authenticates
    // each module in the data path beginning at the upstream end of the data path
    // and moving downstream. As each module is authenticated, that module gives
    // the system information about the next module in the data path so that it
    // can also be authenticated. To be successfully authenticated, a module's 
    // binary file must be signed as DRM-compliant.
    //
    // Two adjacent modules in the data path can communicate with each other in 
    // one of several ways. If the upstream module calls the downstream module 
    // through IoCallDriver, the downstream module is part of a WDM driver. In 
    // this case, the upstream module calls the AcxDrmForwardContentToDeviceObject
    // function to provide the system with the device object representing the 
    // downstream module. (If the two modules communicate through the downstream
    // module's content handlers, the upstream module calls AcxDrmAddContentHandlers
    // instead.)
    //
    // For more information, see MSDN's DRM Functions and Interfaces.
    //

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::GetHWLatency(
    ULONG         * FifoSize,
    ULONG         * Delay
)
{
    PAGED_CODE();

    *FifoSize = 128;
    *Delay = 0;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
CSimPeakMeter *
CStreamEngine::GetPeakMeter()
{
    PAGED_CODE();

    return &m_PeakMeter;
}

_Use_decl_annotations_
#pragma code_seg()
VOID
CStreamEngine::s_EvtStreamPassCallback(
    WDFTIMER        Timer
)
{
    CStreamEngine * This;
    PSTREAM_TIMER_CONTEXT timerCtx;

    // Get our stream engine pointer from the timer context
    timerCtx = GetStreamTimerContext(Timer);
    This = timerCtx->StreamEngine;

    // Call the StreamPassCallback for the engine
    This->StreamPassCallback();
}

// This is run every time the stream timer fires
_Use_decl_annotations_
#pragma code_seg()
VOID
CStreamEngine::StreamPassCallback()
{
    ULONGLONG completedPacket;
    ULONGLONG qpcCompleted;

    // Save the time at which we moved to the next packet
    qpcCompleted = (ULONGLONG)KeQueryPerformanceCounter(NULL).QuadPart;

    // Process the packet (e.g. save render to file/generate capture data)
    ProcessPacket();

    // We've completed a packet! Increment our currently active packet
    completedPacket = (ULONG)InterlockedIncrement((LONG*)&m_CurrentPacket) - 1;

    InterlockedExchange64(&m_LastPacketStart.QuadPart, m_CurrentPacketStart.QuadPart);
    InterlockedExchange64(&m_CurrentPacketStart.QuadPart, qpcCompleted);

    // Tell ACX we've completed the packet.
    (void)AcxRtStreamNotifyPacketComplete(m_Stream, completedPacket, qpcCompleted);

    // Schedule when our new current packet will finish
    ScheduleNextPass();
}

_Use_decl_annotations_
#pragma code_seg()
VOID
CStreamEngine::ScheduleNextPass()
{
    LONGLONG delay = 0;
    ULONG bytesPerSecond;
    ULONGLONG nextPacket = 0;
    ULONGLONG nextPacketStartPosition = 0;
    ULONGLONG nextPacketPositionFromLastPause = 0;
    ULONGLONG nextPacketTimeFromLastPauseHns = 0;
    ULONGLONG nextPacketTime = 0;
    ULONGLONG currentTime;
    BOOLEAN inTimerQueue = FALSE;

    // Get the number of bytes per second from our stored stream format
    bytesPerSecond = GetBytesPerSecond();

    // Calculate the absolute position of the beginning of the next packet from the beginning of the stream
    nextPacket = m_CurrentPacket + 1;
    nextPacketStartPosition = nextPacket * m_PacketSize;

    // Adjust next packet position to account for the last time we resumed from Pause
    nextPacketPositionFromLastPause = nextPacketStartPosition - m_StartPosition;

    // Convert from bytes to HNS (to prevent truncation, multiply first then divide)
    nextPacketTimeFromLastPauseHns = nextPacketPositionFromLastPause * HNS_PER_SEC / bytesPerSecond;

    // Next packet time is Time @ resume from Pause, offset for lost time due to glitch, with next packet time added
    nextPacketTime = m_StartTime + m_GlitchAdjust + nextPacketTimeFromLastPauseHns;

    currentTime = KSCONVERT_PERFORMANCE_TIME(m_PerformanceCounterFrequency.QuadPart, KeQueryPerformanceCounter(NULL));

    // Determine how long we want to wait, in HNS. Negative since it's a relative wait
    delay = -(LONGLONG)(nextPacketTime - currentTime);

    // If the delay isn't negative, this means we lost some time (e.g. broken into kernel debugger). Update
    // our glitch adjust to account for that lost time, and attempt to schedule again
    if (delay >= 0)
    {
        // Glitch!!!
        // Update the glitch adjustment and set the new delay.
        m_GlitchAdjust += delay;

        StreamPassCallback();

        return;
    }

    // Start the timer for our next pass! Note the timer isn't periodic.
    inTimerQueue = WdfTimerStart(m_NotificationTimer, delay);

    // We shouldn't be scheduling our next pass if the timer was previously still pending
    ASSERT(inTimerQueue == FALSE);
}

_Use_decl_annotations_
#pragma code_seg()
VOID
CStreamEngine::UpdatePosition()
{
    ULONGLONG currentTime;
    ULONG bytesPerSecond;

    if (m_CurrentState != AcxStreamStateRun)
    {
        return;
    }
    bytesPerSecond = GetBytesPerSecond();
    currentTime = KSCONVERT_PERFORMANCE_TIME(m_PerformanceCounterFrequency.QuadPart, KeQueryPerformanceCounter(NULL));

    // Update position
    m_Position = m_StartPosition - m_GlitchAdjust + (currentTime - m_StartTime) * bytesPerSecond / HNS_PER_SEC;
}

_Use_decl_annotations_
#pragma code_seg()
ULONG
CStreamEngine::GetBytesPerSecond()
{
    ULONG bytesPerSecond;

    bytesPerSecond = AcxDataFormatGetAverageBytesPerSec(m_StreamFormat);

    return bytesPerSecond;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CStreamEngine::GetCurrentPacket(
    PULONG          CurrentPacket
    )
{
    ULONG currentPacket;
    PAGED_CODE();

    currentPacket = (ULONG)InterlockedCompareExchange((LONG*)&m_CurrentPacket, -1, -1);

    *CurrentPacket = currentPacket;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
CRenderStreamEngine::CRenderStreamEngine(
    ACXSTREAM       Stream,
    ACXDATAFORMAT   StreamFormat,
    CSimPeakMeter   *circuitPeakmeter
    )
    : CStreamEngine(Stream, StreamFormat, circuitPeakmeter)
{
    PAGED_CODE();
}

_Use_decl_annotations_
PAGED_CODE_SEG
CRenderStreamEngine::~CRenderStreamEngine()
{
    PAGED_CODE();
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CRenderStreamEngine::PrepareHardware()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    RETURN_NTSTATUS_IF_FAILED(CStreamEngine::PrepareHardware());

    // ignore failure
    RETURN_NTSTATUS_IF_FAILED(m_SaveData.SetDataFormat((PKSDATAFORMAT)AcxDataFormatGetKsDataFormat(m_StreamFormat)));

    // ignore failure
    RETURN_NTSTATUS_IF_FAILED(m_SaveData.Initialize(FALSE));

    // ignore failure
    RETURN_NTSTATUS_IF_FAILED(m_SaveData.SetMaxWriteSize(m_PacketSize * m_PacketsCount * 16));

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CRenderStreamEngine::ReleaseHardware()
{
    PAGED_CODE();

    m_SaveData.WaitAllWorkItems();
    m_SaveData.Cleanup();

    return CStreamEngine::ReleaseHardware();
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CRenderStreamEngine::AssignDrmContentId(
    ULONG          DrmContentId,
    PACXDRMRIGHTS  DrmRights
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DrmContentId);

    //
    // At this point the driver should enforce the new DrmRights.
    // The sample driver handles DrmRights per stream basis, and 
    // stops writing the stream to disk, if CopyProtect = TRUE.
    //
    // HDMI render: if DigitalOutputDisable or CopyProtect is true, enable HDCP.
    // Loopback: if CopyProtect is true, disable loopback stream.
    //
    
    //
    // Sample writes each stream seperately to disk. If the rights for this
    // stream indicates that the stream is CopyProtected, stop writing to disk.
    //
    m_SaveData.Disable(DrmRights->CopyProtect);

    //
    // From MSDN:
    //
    // This sample doesn't forward protected content, but if your driver uses 
    // lower layer drivers or a different stack to properly work, please see the 
    // following info from MSDN:
    //
    // "Before allowing protected content to flow through a data path, the system
    // verifies that the data path is secure. To do so, the system authenticates
    // each module in the data path beginning at the upstream end of the data path
    // and moving downstream. As each module is authenticated, that module gives
    // the system information about the next module in the data path so that it
    // can also be authenticated. To be successfully authenticated, a module's 
    // binary file must be signed as DRM-compliant.
    //
    // Two adjacent modules in the data path can communicate with each other in 
    // one of several ways. If the upstream module calls the downstream module 
    // through IoCallDriver, the downstream module is part of a WDM driver. In 
    // this case, the upstream module calls the AcxDrmForwardContentToDeviceObject
    // function to provide the system with the device object representing the 
    // downstream module. (If the two modules communicate through the downstream
    // module's content handlers, the upstream module calls AcxDrmAddContentHandlers
    // instead.)
    //
    // For more information, see MSDN's DRM Functions and Interfaces.
    //

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CRenderStreamEngine::SetRenderPacket(
    ULONG           Packet,
    ULONG           Flags,
    ULONG           EosPacketLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG currentPacket;

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(EosPacketLength);

    PAGED_CODE();

    currentPacket = (ULONG)InterlockedCompareExchange((LONG*)&m_CurrentPacket, -1, -1);

    if (Packet <= currentPacket)
    {
        //ASSERT(FALSE);
        status = STATUS_DATA_LATE_ERROR;
    }
    else if (Packet > currentPacket + 1)
    {
        //ASSERT(FALSE);
        status = STATUS_DATA_OVERRUN;
    }

    return status;
}

_Use_decl_annotations_
#pragma code_seg()
NTSTATUS
CRenderStreamEngine::GetLinearBufferPosition(
    _Out_   PULONGLONG  Position
)
{
    NTSTATUS status;
    ULONGLONG qpcIgnored = 0;

    // For this sample, we're borrowing the Presentation Position.
    // An actual device would return the position of the last byte
    // read from the audio buffer, not the last byte presented to the user
    status = GetPresentationPosition(Position, &qpcIgnored);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Position *= AcxDataFormatGetBlockAlign(m_StreamFormat);

    return STATUS_SUCCESS;

}

_Use_decl_annotations_
#pragma code_seg()
VOID
CRenderStreamEngine::ProcessPacket()
{
    ULONG currentPacket;
    ULONG packetIndex;
    PBYTE packetBuffer;

    currentPacket = (ULONG)InterlockedCompareExchange((LONG*)&m_CurrentPacket, -1, -1);

    packetIndex = currentPacket % m_PacketsCount;
    packetBuffer = (PBYTE)m_Packets[packetIndex];
    // Packet 0 starts at an offset if the size isn't a multiple of page_size
    if (packetIndex == 0)
    {
        packetBuffer += m_FirstPacketOffset;
    }

    m_SaveData.WriteData(packetBuffer, m_PacketSize);
}

_Use_decl_annotations_
PAGED_CODE_SEG
CCaptureStreamEngine::CCaptureStreamEngine(
    ACXSTREAM       Stream,
    ACXDATAFORMAT   StreamFormat
    )
    : CStreamEngine(Stream, StreamFormat, nullptr),
      m_EnableWaveCapture(0)
{
    PAGED_CODE();

    m_CurrentPacketStart.QuadPart = 0;
    m_LastPacketStart.QuadPart = 0;

    RtlInitUnicodeString(&m_HostCaptureFileName, NULL);
    RtlInitUnicodeString(&m_LoopbackCaptureFileName, NULL);
}

_Use_decl_annotations_
PAGED_CODE_SEG
CCaptureStreamEngine::~CCaptureStreamEngine()
{
    PAGED_CODE();

    RtlFreeUnicodeString(&m_HostCaptureFileName);
    RtlFreeUnicodeString(&m_LoopbackCaptureFileName);
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CCaptureStreamEngine::PrepareHardware()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    RETURN_NTSTATUS_IF_FAILED(CStreamEngine::PrepareHardware());

    RETURN_NTSTATUS_IF_FAILED(ReadRegistrySettings());

    if (m_EnableWaveCapture)
    {
        status = m_WaveReader.Init((PWAVEFORMATEXTENSIBLE)AcxDataFormatGetWaveFormatExtensible(m_StreamFormat), 
                                   &m_HostCaptureFileName);
        if (!NT_SUCCESS(status))
        {
            m_EnableWaveCapture = FALSE;
        }
    }
    
    if (!m_EnableWaveCapture)
    {
        status = m_ToneGenerator.Init(DEFAULT_FREQUENCY, (PWAVEFORMATEXTENSIBLE)AcxDataFormatGetWaveFormatExtensible(m_StreamFormat));
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CCaptureStreamEngine::ReleaseHardware()
{
    PAGED_CODE();

    if (m_EnableWaveCapture)
    {
        m_WaveReader.WaitAllWorkItems();
    }

    return CStreamEngine::ReleaseHardware();
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CCaptureStreamEngine::GetCapturePacket(
    ULONG         * LastCapturePacket,
    ULONGLONG     * QPCPacketStart,
    BOOLEAN       * MoreData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG currentPacket;
    LONGLONG qpcPacketStart;

    PAGED_CODE();

    currentPacket = (ULONG)InterlockedCompareExchange((LONG*)&m_CurrentPacket, -1, -1);
    qpcPacketStart = InterlockedCompareExchange64(&m_LastPacketStart.QuadPart, -1, -1);

    *LastCapturePacket = currentPacket - 1;
    *QPCPacketStart = (ULONGLONG)qpcPacketStart;
    *MoreData = FALSE;

    return status;
}

_Use_decl_annotations_
#pragma code_seg()
VOID
CCaptureStreamEngine::ProcessPacket()
{
    ULONG currentPacket;
    ULONG packetIndex;
    PBYTE packetBuffer;

    currentPacket = (ULONG)InterlockedCompareExchange((LONG*)&m_CurrentPacket, -1, -1);

    packetIndex = currentPacket % m_PacketsCount;
    packetBuffer = (PBYTE)m_Packets[packetIndex];

    // Packet 0 starts at an offset if the size isn't a multiple of page_size
    if (packetIndex == 0)
    {
        packetBuffer += m_FirstPacketOffset;
    }

    if (m_EnableWaveCapture)
    {
        m_WaveReader.ReadWaveData(packetBuffer, m_PacketSize);
    }
    else
    {
        m_ToneGenerator.GenerateSine(packetBuffer, m_PacketSize);
    }
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS 
CCaptureStreamEngine::ReadRegistrySettings()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    // TRUE only on SUCCESS
    m_EnableWaveCapture = FALSE;

    RTL_QUERY_REGISTRY_TABLE paramTable[] = {
        // QueryRoutine     Flags                                           Name                        EntryContext                  DefaultType                                                     DefaultData                   DefaultLength
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"EnableWaveCapture",       &m_EnableWaveCapture,         (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD,  &m_EnableWaveCapture,         sizeof(DWORD) },
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"HostCaptureFileName",     &m_HostCaptureFileName,       (REG_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_SZ,        &m_HostCaptureFileName,       sizeof(UNICODE_STRING) },
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"LoopbackCaptureFileName", &m_LoopbackCaptureFileName,   (REG_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_SZ,        &m_LoopbackCaptureFileName,   sizeof(UNICODE_STRING) },
        { NULL,   0,                                                        NULL,                       NULL,                         0,                                                              NULL,                         0 }
    };

    UNICODE_STRING parametersPath;
    RtlInitUnicodeString(&parametersPath, NULL);

    // The sizeof(WCHAR) is added to the maximum length, for allowing a space for null termination of the string.
    parametersPath.MaximumLength = g_RegistryPath.Length + sizeof(L"\\Parameters") + sizeof(WCHAR);

#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "memory is freed by scope_exit")
    parametersPath.Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_PAGED, parametersPath.MaximumLength, DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(parametersPath.Buffer == NULL, STATUS_INSUFFICIENT_RESOURCES);
    auto parametersPath_free = scope_exit([&parametersPath]() {
        ExFreePool(parametersPath.Buffer);
    });

    // ExAllocatePool2 zeros memory.

    RtlAppendUnicodeToString(&parametersPath, g_RegistryPath.Buffer);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

    RETURN_NTSTATUS_IF_FAILED(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                      parametersPath.Buffer,
                                      &paramTable[0],
                                      NULL,
                                      NULL));

    m_EnableWaveCapture = TRUE;

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
CBufferedCaptureStreamEngine::CBufferedCaptureStreamEngine(
    _In_ ACXSTREAM          Stream,
    _In_ ACXDATAFORMAT      StreamFormat,    
    _In_ CKeywordDetector * KeywordDetector

    )
    : CCaptureStreamEngine(Stream, StreamFormat),
    m_KeywordDetector(KeywordDetector)
{
    PAGED_CODE();
}

_Use_decl_annotations_
PAGED_CODE_SEG
CBufferedCaptureStreamEngine::~CBufferedCaptureStreamEngine()
{
    PAGED_CODE();
}

// This is run every time the stream timer fires
_Use_decl_annotations_
#pragma code_seg()
VOID
CBufferedCaptureStreamEngine::StreamPassCallback()
{
    LARGE_INTEGER qpc;
    LARGE_INTEGER qpcFrequency;
    BOOLEAN isRealtime = FALSE;
    ULONGLONG completedPacket;
    LONGLONG NewPacketNumber;
    ULONGLONG NewPerformanceCount;

    qpc = KeQueryPerformanceCounter(&qpcFrequency);

    // As this is a simulation, we still want the ScheduleNextPass to 
    // keep producing data. To that end, update the current packet
    // information used for production.
    completedPacket = (ULONG)InterlockedIncrement((LONG*)&m_CurrentPacket) - 1;
    InterlockedExchange64(&m_LastPacketStart.QuadPart, m_CurrentPacketStart.QuadPart);
    InterlockedExchange64(&m_CurrentPacketStart.QuadPart, qpc.QuadPart);


    // Add the next packet to the fifo queue
    m_KeywordDetector->DpcRoutine(qpc.QuadPart, qpcFrequency.QuadPart, &isRealtime, &NewPacketNumber, &NewPerformanceCount);

    if (isRealtime && (m_CurrentState == AcxStreamStateRun))
    {
        // We are running real time and just completed a packet, so notify.
        (void)AcxRtStreamNotifyPacketComplete(m_Stream, NewPacketNumber, NewPerformanceCount);
    }

    // Schedule when our new current packet will finish
    ScheduleNextPass();
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CBufferedCaptureStreamEngine::Pause()
{
    PAGED_CODE();

    m_KeywordDetector->Stop();
    return CCaptureStreamEngine::Pause();
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CBufferedCaptureStreamEngine::Run()
{
    PAGED_CODE();
    ULONG FrontCapturePacket;
    ULONGLONG QPCFrontPacket;

    m_KeywordDetector->Run();
    NTSTATUS status = CCaptureStreamEngine::Run();

    NTSTATUS fifoStatus = m_KeywordDetector->GetFifoStart(&FrontCapturePacket, &QPCFrontPacket);
    if (NT_SUCCESS(fifoStatus))
    {
        // We just entered the run state, so we need to trigger the packet completion for the first
        // buffer in the fifo
        (void)AcxRtStreamNotifyPacketComplete(m_Stream, FrontCapturePacket, QPCFrontPacket);
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CBufferedCaptureStreamEngine::GetCapturePacket(
    _Out_   ULONG         * LastCapturePacket,
    _Out_   ULONGLONG     * QPCPacketStart,
    _Out_   BOOLEAN       * MoreData
    )
{
    PAGED_CODE();
    ULONG nextPacketNumber;
    ULONGLONG nextQPCCount;

    // retrieve the packet from the fifo queue
    NTSTATUS status = m_KeywordDetector->GetReadPacket(m_PacketsCount, m_PacketSize, m_Packets, LastCapturePacket, QPCPacketStart, MoreData, &nextPacketNumber, &nextQPCCount);

    if (NT_SUCCESS(status) && MoreData)
    {
        (void)AcxRtStreamNotifyPacketComplete(m_Stream, nextPacketNumber, nextQPCCount);
    }

    return status;
}

