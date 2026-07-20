/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    PositionClock.h

Abstract:

    Simulated clock Interface for keeping track of stream position.

Environment:

    Kernel mode

--*/
#pragma once

class IPositionClock
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    virtual void Pause() = 0;

    __drv_maxIRQL(PASSIVE_LEVEL)
    virtual void Run() = 0;

    __drv_maxIRQL(PASSIVE_LEVEL)
    virtual void Stop() = 0;

    __drv_maxIRQL(PASSIVE_LEVEL)
    virtual ULONGLONG GetElapsedTime(_Out_ PULONGLONG pQpcTimeStamp) = 0;
};

