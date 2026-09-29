/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    PositionSimClock.cpp

Abstract:

    Simulated clock for keeping track of stream position.

Environment:

    Kernel mode

--*/

#include "private.h"
#include "PositionSimClock.h"

#ifndef __INTELLISENSE__
#include "positionsimclock.tmh"
#endif

_Use_decl_annotations_
PAGED_CODE_SEG
CPositionSimClock::CPositionSimClock()
{
    PAGED_CODE();

    m_State = SIM_CLOCK_STATE_STOP;
    m_ElapsedTimeWhenPaused = 0;
    m_StartTime = 0;
    m_QpcTimeStamp = 0;

    KeInitializeSpinLock(&m_Lock);
}

#pragma code_seg()
_Use_decl_annotations_
CPositionSimClock::~CPositionSimClock()
{
}

#pragma code_seg()
_Use_decl_annotations_
void CPositionSimClock::Run()
{
    KIRQL irql = PASSIVE_LEVEL;
    KeAcquireSpinLock(&m_Lock, &irql);

    if (m_State == SIM_CLOCK_STATE_STOP ||
        m_State == SIM_CLOCK_STATE_PAUSE)
    {
        m_StartTime = KeQueryInterruptTimePrecise(&m_QpcTimeStamp);
    }

    m_State = SIM_CLOCK_STATE_RUN;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"CPositionSimClock::Run SIM_CLOCK_STATE_RUN : %lld", m_StartTime);

    KeReleaseSpinLock(&m_Lock, irql);
}

#pragma code_seg()
_Use_decl_annotations_
void CPositionSimClock::Pause()
{
    KIRQL irql = PASSIVE_LEVEL;
    KeAcquireSpinLock(&m_Lock, &irql);

    if (m_State == SIM_CLOCK_STATE_RUN)
    {
        m_ElapsedTimeWhenPaused = GetElapsedTimeUnlocked();
        DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"CPositionSimClock::Pause ElapsedTimeWhenPaused : %lld", m_ElapsedTimeWhenPaused);
    }

    m_State = SIM_CLOCK_STATE_PAUSE;

    KeReleaseSpinLock(&m_Lock, irql);
}

#pragma code_seg()
_Use_decl_annotations_
void CPositionSimClock::Stop()
{
    KIRQL irql = PASSIVE_LEVEL;
    KeAcquireSpinLock(&m_Lock, &irql);

    m_State = SIM_CLOCK_STATE_STOP;
    m_StartTime = 0;
    m_ElapsedTimeWhenPaused = 0;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"CPositionSimClock::Stop");

    KeReleaseSpinLock(&m_Lock, irql);
}

#pragma code_seg()
_Use_decl_annotations_
ULONGLONG CPositionSimClock::GetElapsedTimeUnlocked()
{
    ULONGLONG current_time = KeQueryInterruptTimePrecise(&m_QpcTimeStamp);

    ULONGLONG elapsedTime = (current_time - m_StartTime) + m_ElapsedTimeWhenPaused;

    return elapsedTime;
}

#pragma code_seg()
_Use_decl_annotations_
ULONGLONG CPositionSimClock::GetElapsedTime(PULONGLONG pQpcTimeStamp)
{
    KIRQL irql = PASSIVE_LEVEL;
    KeAcquireSpinLock(&m_Lock, &irql);

    ULONGLONG elapsedTime = 0;
    if (m_State == SIM_CLOCK_STATE_RUN)
    {
        elapsedTime = GetElapsedTimeUnlocked();
    }
    else
    {
        elapsedTime = m_ElapsedTimeWhenPaused;
    }

    if (pQpcTimeStamp)
    {
        *pQpcTimeStamp = m_QpcTimeStamp;
    }

    KeReleaseSpinLock(&m_Lock, irql);

    return elapsedTime;
}

