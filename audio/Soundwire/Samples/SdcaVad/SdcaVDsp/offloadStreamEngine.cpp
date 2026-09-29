/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    offloadStreamEngine.cpp

Abstract:

    Virtual Streaming Engine - this module controls offload streaming logic for
    the device.

Environment:

    Kernel mode

--*/

#include "private.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "offloadStreamEngine.h"

#ifndef __INTELLISENSE__
#include "offloadStreamEngine.tmh"
#endif

_Use_decl_annotations_
PAGED_CODE_SEG
COffloadStreamEngine::COffloadStreamEngine(
    ACXSTREAM       Stream,
    ACXDATAFORMAT   StreamFormat,
    CSimPeakMeter   *circuitPeakmeter
) :CStreamEngine(Stream, StreamFormat, circuitPeakmeter)
{
    PAGED_CODE();

    m_BufferReadTimer = NULL;
    m_LastBufferTimer = NULL;
    m_PacketsWritten = 0;
    m_PacketsRead = 0;
    m_SinglePacketPosition = 0;
}

_Use_decl_annotations_
#pragma code_seg()
COffloadStreamEngine::~COffloadStreamEngine()
{
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::PrepareHardware()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    RETURN_NTSTATUS_IF_FAILED(CStreamEngine::PrepareHardware());
    // CStreamEngine::PrepareHardware will update state to Pause, but we don't
    // want to be in Pause state if any of the below actions fail.
    m_CurrentState = AcxStreamStateStop;

    //
    // Buffer read callbacks
    //
    WDF_TIMER_CONFIG timerConfig;
    LONG period = (LONG)((ULONGLONG)m_PacketSize * HNS_PER_SEC / (ULONGLONG)GetBytesPerSecond());
    WDF_TIMER_CONFIG_INIT_PERIODIC(
        &timerConfig,
        COffloadStreamEngine::s_EvtBufferReadTimerCallback,
        period / HNSTIME_PER_MILLISECOND
    );
    timerConfig.UseHighResolutionTimer = WdfTrue;

    WDF_OBJECT_ATTRIBUTES timerAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&timerAttributes, STREAM_TIMER_CONTEXT);
    timerAttributes.ParentObject = m_Stream;

    RETURN_NTSTATUS_IF_FAILED(WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        &m_BufferReadTimer
    ));

    auto bt_free = scope_exit([this]() {
        WdfObjectDelete(m_BufferReadTimer);
        m_BufferReadTimer = NULL;
        });

    PSTREAM_TIMER_CONTEXT timerCtx;
    timerCtx = GetStreamTimerContext(m_BufferReadTimer);
    timerCtx->StreamEngine = this;

    //
    // Last Buffer read callback
    //
    WDF_TIMER_CONFIG_INIT(
        &timerConfig,
        COffloadStreamEngine::s_EvtLastBufferTimerCallback
    );
    timerConfig.UseHighResolutionTimer = WdfTrue;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&timerAttributes, STREAM_TIMER_CONTEXT);
    timerAttributes.ParentObject = m_Stream;

    RETURN_NTSTATUS_IF_FAILED(WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        &m_LastBufferTimer
    ));

    auto lbt_free = scope_exit([this]() {
        WdfObjectDelete(m_LastBufferTimer);
        m_LastBufferTimer = NULL;
        });

    timerCtx = GetStreamTimerContext(m_LastBufferTimer);
    timerCtx->StreamEngine = this;

    RETURN_NTSTATUS_IF_FAILED(m_SaveData.SetDataFormat((PKSDATAFORMAT)AcxDataFormatGetKsDataFormat(m_StreamFormat)));

    RETURN_NTSTATUS_IF_FAILED(m_SaveData.Initialize(TRUE));

    RETURN_NTSTATUS_IF_FAILED(m_SaveData.SetMaxWriteSize(m_PacketSize * m_PacketsCount * MAX_FILE_WRITE_FRAMES));

    m_CurrentState = AcxStreamStatePause;

    bt_free.release();
    lbt_free.release();

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::ReleaseHardware()
{
    PAGED_CODE();

    m_SaveData.WaitAllWorkItems();
    m_SaveData.Cleanup();

    if (m_BufferReadTimer)
    {
        WdfTimerStop(m_BufferReadTimer, TRUE);
        WdfObjectDelete(m_BufferReadTimer);
        m_BufferReadTimer = NULL;
    }

    if (m_LastBufferTimer)
    {
        WdfTimerStop(m_LastBufferTimer, TRUE);
        WdfObjectDelete(m_LastBufferTimer);
        m_LastBufferTimer = NULL;
    }

    m_LinearBufferClock.Stop();

    m_PacketsWritten = 0;
    m_PacketsRead = 0;

    CStreamEngine::ReleaseHardware();

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::Run()
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::Run");

    if (m_CurrentState != AcxStreamStatePause)
    {
        status = STATUS_INVALID_STATE_TRANSITION;
        return status;
    }

    ULONGLONG bytesPerSec = GetBytesPerSecond();

    ULONGLONG elapsedTimeWhenPaused = m_LinearBufferClock.GetElapsedTime(NULL);
    if (elapsedTimeWhenPaused)
    {
        // Stream has resumed from pause
        // Calculate remaining buffer for next notification

        // Remaining buffer from when stream was paused
        // Hardware might have cycled more than the bytes written
        // This can happen if there was a glitch and hardware was
        // starved
        ULONGLONG packetTime = (ULONGLONG)m_PacketSize * HNS_PER_SEC / bytesPerSec;
        LONG remainingTime = (LONG)((ULONGLONG)elapsedTimeWhenPaused % packetTime);
        WdfTimerStart(m_BufferReadTimer, WDF_REL_TIMEOUT_IN_MS(remainingTime / HNSTIME_PER_MILLISECOND));
        DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::Run - Notification Timer Started - first timeout :%d ms", (ULONG)(remainingTime / HNSTIME_PER_MILLISECOND));
    }
    else
    {
        // Run has been called first time on this stream
        LONG period = (LONG)((ULONGLONG)m_PacketSize * HNS_PER_SEC / (ULONGLONG)bytesPerSec);
        WdfTimerStart(m_BufferReadTimer, WDF_REL_TIMEOUT_IN_MS(period / HNSTIME_PER_MILLISECOND));
        DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::Run - Notification Timer Started - first timeout :%d ms", (ULONG)(period / HNSTIME_PER_MILLISECOND));
    }

    m_LinearBufferClock.Run();
    m_CurrentState = AcxStreamStateRun;

    m_PeakMeter.StartStream();
    m_pCircuitPeakmeter->StartStream();

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::Pause()
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::Pause - from %d", m_CurrentState);

    RETURN_NTSTATUS_IF_TRUE(m_CurrentState != AcxStreamStateRun, STATUS_INVALID_STATE_TRANSITION);

    WdfTimerStop(m_BufferReadTimer, TRUE);

    m_LinearBufferClock.Pause();

    m_PeakMeter.StopStream();
    m_pCircuitPeakmeter->StopStream();

    m_CurrentState = AcxStreamStatePause;

    return status;
}

_Use_decl_annotations_
#pragma code_seg()
NTSTATUS
COffloadStreamEngine::GetPresentationPosition(
    PULONGLONG      PositionInBlocks,
    PULONGLONG      QPCPosition
)
{
    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::GetPresentationPosition");

    ULONG blockAlign;
    LARGE_INTEGER qpc;

    blockAlign = AcxDataFormatGetBlockAlign(m_StreamFormat);
    qpc = KeQueryPerformanceCounter(NULL);

    ULONGLONG streamPosition = m_LinearBufferClock.GetElapsedTime(NULL);

    // Simulate Presentation position lag by 20 ms
    if (streamPosition > (OFFLOAD_PRESENTATION_POSITION_LAG_IN_MS * HNSTIME_PER_MILLISECOND))
    {
        streamPosition -= (OFFLOAD_PRESENTATION_POSITION_LAG_IN_MS * HNSTIME_PER_MILLISECOND);
    }
    else
    {
        streamPosition = 0;
    }

    *PositionInBlocks = (streamPosition * GetBytesPerSecond() / HNS_PER_SEC) / blockAlign;

    *QPCPosition = (ULONGLONG)qpc.QuadPart;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
#pragma code_seg()
NTSTATUS
COffloadStreamEngine::GetLinearBufferPosition(
    PULONGLONG  Position
)
{
    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::GetLinearBufferPosition");

    ULONGLONG bytesPerSecond = GetBytesPerSecond();
    ULONGLONG currentTime = m_LinearBufferClock.GetElapsedTime(NULL);

    // Update position
    *Position = currentTime * bytesPerSecond / HNS_PER_SEC;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::SetCurrentWritePosition(
    ULONG   Position
)
{
    PAGED_CODE();

    if (m_PacketsCount == 1)
    {
        return SetCurrentWritePositionSinglePacket(Position);
    }

    // Determine buffer
    // Designed for ping-pong
    // Position == m_PacketSize, ping buffer was just filled
    // Position == m_PacketSize * 2, pong buffer was just filled
    ULONG packetIndex = Position == m_PacketSize ? 0 : 1;

    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::SetCurrentWritePosition, Position = 0x%08x, PacketIndex :%d", Position, packetIndex);

    //
    // Detect if the packet just written is incorrect
    //
    ULONG packetsRead = (ULONG)InterlockedCompareExchange((LONG *)&m_PacketsRead, -1, -1);

    if (m_CurrentState == AcxStreamStateRun)
    {
        ULONG expectedPacketIndex = (packetsRead % 2) ? 0 : 1;
        if (packetIndex != expectedPacketIndex)
        {
            DrvLogError(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine incorrect packet write: %d, Packets Read: %08d", packetIndex, packetsRead);
            // \TODO: ACX doesn't recover from this error
            // Continuing automatically recovers with next call
            //return STATUS_DATA_OVERRUN;
        }
    }

    //
    // Catch up to packets read + 1
    // This is to recover from condition when OS was not writing enough data
    //
    (ULONG)InterlockedExchange((LONG *)&m_PacketsWritten, packetsRead + 1);

    PBYTE packetBuffer = NULL;
    packetBuffer = (PBYTE)m_Packets[packetIndex];
    // Packet 0 starts at an offset if the size isn't a multiple of page_size
    if (packetIndex == 0)
    {
        packetBuffer += m_FirstPacketOffset;
    }

    m_SaveData.WriteData(packetBuffer, m_PacketSize);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::SetCurrentWritePositionSinglePacket(
    ULONG   Position
)
{
    PAGED_CODE();

    // Offload streams are almost always 2-packet. However, it is possible to create an offload stream
    // as a timer-driven (single packet) stream. This code will ensure correct behavior in this case.
    ULONG packetIndex = 0;

    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::SetCurrentWritePositionSinglePacket, Position = 0x%08x, PacketIndex :%d", Position, packetIndex);

    //
    // Detect if the packet just written is incorrect
    //
    ULONG packetsRead = (ULONG)InterlockedCompareExchange((LONG *)&m_PacketsRead, -1, -1);

    // Position has wrapped, can increment the written count.
    if (Position < m_SinglePacketPosition)
    {
        //
        // Catch up to packets read + 1
        // This is to recover from condition when OS was not writing enough data
        //
        (ULONG)InterlockedExchange((LONG*)&m_PacketsWritten, packetsRead + 1);
    }

    PBYTE packetBuffer = NULL;
    packetBuffer = (PBYTE)m_Packets[packetIndex];
    // Packet 0 starts at an offset if the size isn't a multiple of page_size
    // For single-packet the offset should be 0.
    packetBuffer += m_FirstPacketOffset;

    // AudioKSE adds 1 to the position
    Position -= 1;
    Position %= m_PacketSize;

    if (Position <= m_SinglePacketPosition)
    {
        // Handle the case of wraparound by copying from the last position to the end of the buffer
        m_SaveData.WriteData(packetBuffer + m_SinglePacketPosition, m_PacketSize - m_SinglePacketPosition);
        m_SinglePacketPosition = 0;
    }
    // Write from the last position (0 in the case of wraparound) to the new Position
    m_SaveData.WriteData(packetBuffer + m_SinglePacketPosition, Position);
    m_SinglePacketPosition = Position;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::SetLastBufferPosition(
    ULONG   Position
)
{
    PAGED_CODE();

    ULONG bytesPerSec = GetBytesPerSecond();

    // Determine buffer
    ULONG packetIndex = Position < m_PacketSize ? 0 : 1;
    ULONG lastBufferSize = Position <= m_PacketSize ? Position : Position - m_PacketSize;

    // time for rendering last buffer
    ULONGLONG lastBufferTime = (ULONGLONG)lastBufferSize * HNS_PER_SEC / (ULONGLONG)bytesPerSec;

    ULONGLONG totalStreamTime = (ULONGLONG)m_PacketsWritten* (ULONGLONG)m_PacketSize* HNS_PER_SEC / (ULONGLONG)bytesPerSec;
    totalStreamTime += (ULONGLONG)lastBufferTime;

    // Simulate Presentation position lag by 20 ms
    ULONGLONG presentationTime = m_LinearBufferClock.GetElapsedTime(NULL) - (OFFLOAD_PRESENTATION_POSITION_LAG_IN_MS * HNSTIME_PER_MILLISECOND);

    lastBufferTime = totalStreamTime - presentationTime;

    //
    // Start last buffer timer
    //
    RETURN_NTSTATUS_IF_TRUE_MSG(NULL == m_LastBufferTimer, STATUS_INVALID_PARAMETER, L"Set Last Buffer Position called out of sequence - without calling prepare hardware");
    WdfTimerStart(m_LastBufferTimer, WDF_REL_TIMEOUT_IN_MS(lastBufferTime / HNSTIME_PER_MILLISECOND));

    PBYTE packetBuffer = NULL;
    packetBuffer = (PBYTE)m_Packets[packetIndex];
    // Packet 0 starts at an offset if the size isn't a multiple of page_size
    if (packetIndex == 0)
    {
        packetBuffer += m_FirstPacketOffset;
    }

    m_SaveData.WriteData(packetBuffer, lastBufferSize);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
COffloadStreamEngine::AssignDrmContentId(
    ULONG,
    PACXDRMRIGHTS  DrmRights
)
{
    PAGED_CODE();

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
#pragma code_seg()
VOID
COffloadStreamEngine::s_EvtBufferReadTimerCallback(
    WDFTIMER        Timer
)
{
    COffloadStreamEngine * This;
    PSTREAM_TIMER_CONTEXT timerCtx;

    // Get our stream engine pointer from the timer context
    timerCtx = GetStreamTimerContext(Timer);
    This = (COffloadStreamEngine *)timerCtx->StreamEngine;

    // Call the BufferReadCallback for the engine
    This->BufferReadCallback();
}

// Callback indicating buffer read complete
_Use_decl_annotations_
#pragma code_seg()
VOID
COffloadStreamEngine::BufferReadCallback()
{
    // Save the time at which we moved to the next packet
    ULONGLONG qpcCompleted;
    qpcCompleted = (ULONGLONG)KeQueryPerformanceCounter(NULL).QuadPart;

    ULONG packetsWritten = (ULONG)InterlockedCompareExchange((LONG*)&m_PacketsWritten, -1, -1);

    // We've completed a packet! Increment our currently active packet
    ULONG packetsRead  = (ULONG)InterlockedIncrement((LONG *)&m_PacketsRead) - 1;

    //
    // \TODO
    // Detect if hardware has cycled more than the OS.
    // Can happen if application doesn't write data on time
    //
    if(packetsRead > packetsWritten)
    {
        DrvLogError(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine starved PacketsWritten: %08d, Packets Read: %08d", packetsWritten, packetsRead);
    }

    // Tell ACX we've completed the packet.
    // 0 based packet count
    (void)AcxRtStreamNotifyPacketComplete(m_Stream, (ULONGLONG)packetsRead, qpcCompleted);
    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::BufferReadCallback packet complete - %d", packetsRead);
}

_Use_decl_annotations_
#pragma code_seg()
VOID
COffloadStreamEngine::s_EvtLastBufferTimerCallback(
    WDFTIMER        Timer
)
{
    COffloadStreamEngine * This;
    PSTREAM_TIMER_CONTEXT timerCtx;

    // Get our stream engine pointer from the timer context
    timerCtx = GetStreamTimerContext(Timer);
    This = (COffloadStreamEngine *)timerCtx->StreamEngine;

    // Call the LastBufferRenderComplete for the engine
    This->LastBufferRenderComplete();
}

_Use_decl_annotations_
#pragma code_seg()
VOID
COffloadStreamEngine::LastBufferRenderComplete()
{
    // Save the time at which we moved to the next packet
    ULONGLONG qpcCompleted;
    qpcCompleted = (ULONGLONG)KeQueryPerformanceCounter(NULL).QuadPart;

    ULONGLONG completedPacket;
    completedPacket = (ULONG)InterlockedIncrement((LONG*)&m_PacketsRead) - 1;

    // Tell ACX we've completed the packet.
    (void)AcxRtStreamNotifyPacketComplete(m_Stream, completedPacket, qpcCompleted);
    DrvLogVerbose(g_SDCAVDspLog, FLAG_STREAM, L"COffloadStreamEngine::LastBufferRenderComplete packet complete - %d", (ULONG)completedPacket);
}

_Use_decl_annotations_
#pragma code_seg()
VOID
COffloadStreamEngine::ProcessPacket()
{
}

