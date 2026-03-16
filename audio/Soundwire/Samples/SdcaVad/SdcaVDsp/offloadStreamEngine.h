/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    offloadStreamEngine.h

Abstract:

    Virtual Streaming Engine - this module controls offload streaming logic for
    the device.

Environment:

    Kernel mode

--*/

#pragma once

#include "streamengine.h"
#include "PositionSimClock.h"

#define MAX_FILE_WRITE_FRAMES (16)
#define OFFLOAD_PRESENTATION_POSITION_LAG_IN_MS (20)

class COffloadStreamEngine : public CStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    COffloadStreamEngine(
        _In_    ACXSTREAM       Stream, 
        _In_    ACXDATAFORMAT   StreamFormat,
        _In_    CSimPeakMeter   *circuitPeakmeter
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    #pragma code_seg()
    ~COffloadStreamEngine();

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
    #pragma code_seg()
    NTSTATUS
    GetPresentationPosition(
        _Out_   PULONGLONG      PositionInBlocks,
        _Out_   PULONGLONG      QPCPosition
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
    #pragma code_seg()
    NTSTATUS
    GetLinearBufferPosition(
        _Out_   PULONGLONG  Position
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    SetCurrentWritePosition(
        _In_    ULONG   Position
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    SetLastBufferPosition(
        _In_    ULONG   Position
        );

protected:
    WDFTIMER            m_BufferReadTimer;
    WDFTIMER            m_LastBufferTimer;

    CPositionSimClock   m_LinearBufferClock;

    CSaveData           m_SaveData;

    // Number of packets written by OS
    ULONG               m_PacketsWritten;

    // Number of packets read by hardware
    ULONG               m_PacketsRead;

    ULONG               m_SinglePacketPosition;

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    ProcessPacket();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    SetCurrentWritePositionSinglePacket(
        _In_    ULONG   Position
        );

    static
    __drv_maxIRQL(DISPATCH_LEVEL)
    _Function_class_(EVT_WDF_TIMER)
    #pragma code_seg()
    VOID s_EvtBufferReadTimerCallback(
        _In_    WDFTIMER        Timer
        );

    static
    __drv_maxIRQL(DISPATCH_LEVEL)
    _Function_class_(EVT_WDF_TIMER)
    #pragma code_seg()
    VOID s_EvtLastBufferTimerCallback(
        _In_    WDFTIMER        Timer
        );

    // Callback indicating buffer read complete
    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    BufferReadCallback();

    virtual
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    LastBufferRenderComplete();
};

