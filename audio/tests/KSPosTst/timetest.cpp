// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Module Name:
//
//  timetest.cpp
//
// Abstract:
//
//  Functions to precisely measure time.
//
// -------------------------------------------------------------------------------

#include "PreComp.h"
#include <math.h>

// ------------------------------------------------------------------------------
// returns millisecs
double tpQPC (void)
{
    static LARGE_INTEGER liFrequency = { 0 };
    static LARGE_INTEGER liStart = { 0 };
    static bool bInitialized = false;

    if (!bInitialized)
    {
        if (!QueryPerformanceFrequency(&liFrequency))
        {
            throw ("QueryPerformanceFrequency failed");
        }
        
        if (!QueryPerformanceCounter(&liStart))
        {
            throw ("QueryPerformanceCounter failed");
        }
        
        bInitialized = true;
    }

    LARGE_INTEGER liNow = { 0 };

    if (!QueryPerformanceCounter(&liNow))
    {
        throw ("QueryPerformanceCounter failed");
    }

    // milliseconds since test start
    return 1000.0 * (liNow.QuadPart - liStart.QuadPart) / liFrequency.QuadPart;
}
