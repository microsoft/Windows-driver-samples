/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    streamengine.h

Abstract:

    Virtual Streaming Engine - this module controls streaming logic for
    the device.

Environment:

    Kernel mode

--*/
#pragma once

#include "savedata.h"
#include "tonegenerator.h"
#include "WaveReader.h"
#include "SimPeakMeter.h"
#include "KeywordDetector.h"

#define HNSTIME_PER_MILLISECOND 10000

#define MAX_PACKET_COUNT 2

#define DEFAULT_FREQUENCY (220)
#define LOOPBACK_FREQUENCY (500)
#define DEFAULT_FREQUENCY (220)

class CStreamEngine
{
public:
    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AllocateRtPackets(
        _In_    ULONG           PacketCount,
        _In_    ULONG           PacketSize,
        _Out_   PACX_RTPACKET * Packets
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    FreeRtPackets(
        _Frees_ptr_ PACX_RTPACKET   Packets,
        _In_        ULONG           PacketCount
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Run();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Pause();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetPresentationPosition(
        _Out_   PULONGLONG      PositionInBlocks,
        _Out_   PULONGLONG      QPCPosition
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetCurrentPacket(
        _Out_ PULONG CurrentPacket
        );
    
    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AssignDrmContentId(
        _In_ ULONG          DrmContentId,
        _In_ PACXDRMRIGHTS  DrmRights
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetHWLatency(
        _Out_   ULONG         * FifoSize, 
        _Out_   ULONG         * Delay
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    #pragma code_seg()
    NTSTATUS
    GetLinearBufferPosition(
        _Out_   PULONGLONG  Position
        )
    {
        UNREFERENCED_PARAMETER(Position);
        return STATUS_NOT_SUPPORTED;
    }

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CSimPeakMeter *
    GetPeakMeter();

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CStreamEngine(
        _In_        ACXSTREAM       Stream, 
        _In_        ACXDATAFORMAT   StreamFormat,
        _In_opt_    CSimPeakMeter   *circuitPeakmeter
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    #pragma code_seg()
    ~CStreamEngine();

protected:
    PVOID               m_Packets[MAX_PACKET_COUNT]{ nullptr };
    ULONG               m_PacketsCount{ 0 };
    ULONG               m_PacketSize{ 0 };
    ULONG               m_FirstPacketOffset{ 0 };
    WDFTIMER            m_NotificationTimer{ nullptr };
    ACX_STREAM_STATE    m_CurrentState{ AcxStreamStateStop };
    ULONG               m_CurrentPacket{ 0 };
    ULONGLONG           m_Position{ 0 };
    ACXSTREAM           m_Stream{ nullptr };
    ACXDATAFORMAT       m_StreamFormat{ nullptr };
    ULONGLONG           m_StartTime{ 0 };
    ULONGLONG           m_StartPosition{ 0 };
    ULONGLONG           m_GlitchAdjust{ 0 };
    LARGE_INTEGER       m_PerformanceCounterFrequency{ 0 };
    LARGE_INTEGER       m_CurrentPacketStart{ 0 };
    LARGE_INTEGER       m_LastPacketStart{ 0 };
    CSimPeakMeter       m_PeakMeter;
    CSimPeakMeter*      m_pCircuitPeakmeter{ nullptr };

    static
    __drv_maxIRQL(DISPATCH_LEVEL)
    _Function_class_(EVT_WDF_TIMER)
    #pragma code_seg()
    VOID s_EvtStreamPassCallback(
        _In_    WDFTIMER        Timer
        );

    // This is run every time the stream timer fires
    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    StreamPassCallback();

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ScheduleNextPass();

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    UpdatePosition();

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    ULONG
    GetBytesPerSecond();

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ProcessPacket() = 0;
};

class CRenderStreamEngine : public CStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CRenderStreamEngine(
        _In_    ACXSTREAM       Stream,
        _In_    ACXDATAFORMAT   StreamFormat,
        _In_    CSimPeakMeter   *circuitPeakmeter
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    ~CRenderStreamEngine();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();
    
    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AssignDrmContentId(
        _In_ ULONG          DrmContentId,
        _In_ PACXDRMRIGHTS  DrmRights
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    SetRenderPacket(
        _In_    ULONG           Packet,
        _In_    ULONG           Flags,
        _In_    ULONG           EosPacketLength
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    #pragma code_seg()
    NTSTATUS
    GetLinearBufferPosition(
        _Out_   PULONGLONG  Position
        );

protected:
    CSaveData m_SaveData;

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ProcessPacket();

};

class CCaptureStreamEngine : public CStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CCaptureStreamEngine(
        _In_    ACXSTREAM       Stream,
        _In_    ACXDATAFORMAT   StreamFormat
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    ~CCaptureStreamEngine();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetCapturePacket(
        _Out_   ULONG         * LastCapturePacket,
        _Out_   ULONGLONG     * QPCPacketStart,
        _Out_   BOOLEAN       * MoreData
        );

protected:
    ToneGenerator   m_ToneGenerator;
    CWaveReader     m_WaveReader;
    DWORD           m_EnableWaveCapture{ 0 };
    UNICODE_STRING  m_HostCaptureFileName{ 0 };
    UNICODE_STRING  m_LoopbackCaptureFileName{ 0 };

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ProcessPacket();

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReadRegistrySettings();
};

class CBufferedCaptureStreamEngine : public CCaptureStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CBufferedCaptureStreamEngine(
        _In_ ACXSTREAM          Stream,
        _In_ ACXDATAFORMAT      StreamFormat,
        _In_ CKeywordDetector * KeywordDetector
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    ~CBufferedCaptureStreamEngine();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Run();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Pause();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetCapturePacket(
        _Out_   ULONG         * LastCapturePacket,
        _Out_   ULONGLONG     * QPCPacketStart,
        _Out_   BOOLEAN       * MoreData
        );

protected:
    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ProcessPacket() {}

    // This is run every time the stream timer fires
    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    StreamPassCallback();

    CKeywordDetector * m_KeywordDetector{ nullptr };
};


// Define DSP circuit/stream pin context.
//
typedef struct _STREAM_TIMER_CONTEXT {
    CStreamEngine * StreamEngine;
} STREAM_TIMER_CONTEXT, *PSTREAM_TIMER_CONTEXT;

#pragma code_seg()
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(STREAM_TIMER_CONTEXT, GetStreamTimerContext)
