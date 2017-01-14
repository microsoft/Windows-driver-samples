/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    HwSimClock.cpp

Abstract:

    Implementation of SYSVAD Hardware Simulated clock class.

    This class calculates elapsed stream time based on QPC by maintaining
	start time in CPU counter and returning elapsed time by a simple calculation
	of "current time - start time".

--*/

#include "HwSimClock.h"



CHwSimClock::CHwSimClock()
{
}


CHwSimClock::~CHwSimClock()
{
}
