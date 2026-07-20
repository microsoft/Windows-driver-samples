/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    PositionSimClock.h

Abstract:

    Simulated clock for keeping track of stream position.

Environment:

    Kernel mode

--*/

#pragma once

#include "PositionClock.h"

#define HNSTIME_PER_MILLISECOND 10000

class CPositionSimClock : public IPositionClock
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CPositionSimClock();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    ~CPositionSimClock();

    virtual
        __drv_maxIRQL(PASSIVE_LEVEL)
        void Pause();

    virtual
        __drv_maxIRQL(PASSIVE_LEVEL)
        void Run();

    virtual
        __drv_maxIRQL(PASSIVE_LEVEL)
        void Stop();

    virtual
        __drv_maxIRQL(PASSIVE_LEVEL)
        ULONGLONG GetElapsedTime(_Out_ PULONGLONG pQpcTimeStamp);

protected:
    ULONGLONG m_StartTime;
    ULONGLONG m_ElapsedTimeWhenPaused;
    ULONGLONG m_QpcTimeStamp;

    KSPIN_LOCK m_Lock;

    typedef enum _SimClockState_t
    {
        SIM_CLOCK_STATE_STOP,
        SIM_CLOCK_STATE_PAUSE,
        SIM_CLOCK_STATE_RUN,

        SIM_CLOCK_STATE_Count
    }SimClockState;

    SimClockState m_State;

    __drv_maxIRQL(DISPATCH_LEVEL)
        ULONGLONG GetElapsedTimeUnlocked();
};

