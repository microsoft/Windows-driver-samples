// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Module Name:
//
//  locallimits.h
//
// Abstract:
//
//  defines the tolerances for WHQL failures for this test app
//
// -------------------------------------------------------------------------------

// max latency for rendering or capturing is 20 ms
#define LATENCY_THRESHOLD           20.

//  Real time sampling rates must be within 1% of the theoretical rate,
//  per the format, as required by the PC98/PC99 HW Design Guide.
#define SAMPLERATE_THRESHOLD        1.0

//  audio devices are required to be SAMPLEACCURATE (PC9x HW Design Guide)..  
//  but for some reason we are making them ms accurate??
#define JITTER_STD_THRESHOLD        1.0       // 1 ms max allowed for std-deviation
#define JITTER_MAX_THRESHOLD        4.0       // 4 ms max allowed for max deviation
