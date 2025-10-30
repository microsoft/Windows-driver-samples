// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  PreComp.h
//
// Abstract:
//
//  Precompiled common headers
//
// -------------------------------------------------------------------------------































// Number of milliseconds that the test will be delayed to wait for the stream to stabilize
#define DELAY_FOR_STABILIZE 100

// Number of milliseconds in a second
#define MILLISECONDS_PER_SECOND 1000

// Number of microseconds in a second
#define MICROSECONDS_PER_SECOND 1000000

// Number of HNS units in one second
#define HNSTIME_UNITS_PER_SECOND 10000000

// Number of HNS units in one millisecond
#define HNSTIME_UNITS_PER_MILLISECOND 10000

// Number of HNS units per microsecond
#define HNSTIME_UNITS_PER_MICROSECOND 10

typedef double (*TIMEPROC) (void);
double  tpQPC(void);

typedef struct _tag_tm
{
    TIMEPROC    Proc;
    LPCSTR      strRep;
} TIMER_MECHANISM, * PTIMER_MECHANISM;

// ----------------------------------------------------------
// Timer functions
extern "C" __kernel_entry NTSYSCALLAPI NTSTATUS NTAPI NtSetTimerResolution(
    _In_ ULONG DesiredTime, _In_ BOOLEAN SetResolution, _Out_ PULONG ActualTime);

// 1 ms timer resolution
static ULONG timerResolution = (1 * 10000);
static ULONG actualTimerResolution;

// ------------------------------------------------------------------------------
// structs used to hold results of perf tests
#define DATA_POINTS     100

typedef struct
{
    double testTime;     // Time the sample was taken, in milliseconds, relative to the start of the sample collection
    double positionTime; // Time of the reported position, in milliseconds, relative to the start of the sample collection
    double position;     // Position, in seconds, relative to the start of the sample collection
} POSITION_SET, * PPOSITION_SET;

typedef struct
{
    PPOSITION_SET   argPosSets;
    ULONG           cPosSets;
    ULONG           ctLatencyMeas;
    ULONG           csBufferMeas;
    double          bytesPerSecond; // measured (in bytes/sec, NOT samples/sec)
    double          dOffset;        // measured
} PERF_RESULTS, * PPERF_RESULTS;

typedef struct
{
    WAVEFORMATEXTENSIBLE    wfxFormat;
    PERF_RESULTS            perfResults;        // 
} FORMAT_ENTRY, * PFORMAT_ENTRY;
