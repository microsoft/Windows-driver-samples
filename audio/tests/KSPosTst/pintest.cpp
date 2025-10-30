// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Module Name:
//
//  pintest.cpp
//
// Abstract:
//
//  Implementation file for test cases
//
// -------------------------------------------------------------------------------

#include "PreComp.h"
#include "KsPosTestTaef.h"
#include "tests.h"

#include <bestfit.h>
#include <cmath>

#define DEFAULT_PERIODICITY 0
#define DEFAULT_LATENCY_COEFFICIENT 0
#define xGetTime    g_pKsPosTst->m_pTimer->Proc

// ------------------------------------
// Test_DriftAndJitter helper functions
// ------------------------------------

HRESULT CollectSampleData
(
    CHalfApp* pHalfApp,
    PFORMAT_ENTRY pFormatEntry,
    HNSTIME requestedPeriodicity
)
{
    DWORD           periodicityInMs = (DWORD)ceil(HNSTIME_TO_MILLISECONDS_DOUBLE(requestedPeriodicity));

    if (!pFormatEntry)
    {
        ERR(g_pBasicLog, "FAIL: Current Format Entry is NULL.");
        return E_INVALIDARG;
    }

    PPERF_RESULTS pResults = &pFormatEntry->perfResults;
    PPOSITION_SET pPosSet = pResults->argPosSets;
    double        tStart = 0, tPre = 0, tPost = 0;
    UINT          i = 0;
    UINT64        u64Position;
    UINT64        positionHNS;
    UINT64        u64Frequency;
    LARGE_INTEGER liNow;
    PLONGLONG     frequency = &g_pKsPosTst->m_qpcFrequency;
    PLONGLONG     pBaseHns = &g_pKsPosTst->m_testBaseHns;
    CRtThreadRegistration* threadPriority = &g_pKsPosTst->m_threadPriority;

    // Register with MMCSS
    threadPriority->RegisterWithMMCSS();

    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHalfApp->InitializeStream(requestedPeriodicity, DEFAULT_LATENCY_COEFFICIENT));

    // run the adapter sink pin
    VERIFY_SUCCEEDED(pHalfApp->StartStream());

    tStart = xGetTime();
    Sleep(DELAY_FOR_STABILIZE);
    // Get the stream frequency in order to determine the time offset
    VERIFY_SUCCEEDED(pHalfApp->m_pAudioClock->GetFrequency(&u64Frequency));

    // Start collecting the data
    for (i = 0; i < DATA_POINTS;)
    {
        Sleep(periodicityInMs);

        tPre = xGetTime();
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &positionHNS));
        QueryPerformanceCounter(&liNow);
        tPost = xGetTime();

        if (u64Position > 0)
        {
            // only count this result if the GetPosition and GetTime happened relatively atomically
            if ((tPost - tPre) < 1.)
            {
                // Convert QPC to microseconds and then HNS. This is to prevent overflows from converting directly to HNS.
                UINT64 testHNS = liNow.QuadPart * MICROSECONDS_PER_SECOND / *frequency;
                testHNS *= HNSTIME_UNITS_PER_MICROSECOND;
                // In both sampling rate and jitter calculation, the time used is in milliseconds, so we can convert it right here
                pPosSet[i].testTime = HNSTIME_TO_MILLISECONDS_DOUBLE(testHNS - *pBaseHns);
                pPosSet[i].positionTime = HNSTIME_TO_MILLISECONDS_DOUBLE(positionHNS - *pBaseHns);
                // We store the values as seconds because it is easier to convert to bytes per second, on sampling rate calculation, and milliseconds, on jitter calculation.
                pPosSet[i].position = (double)u64Position / (double)u64Frequency;

                // Ignore repeated positions (that means they have the same QPC and should have the same position value)
                if ((i > 0) && (pPosSet[i - 1].positionTime == pPosSet[i].positionTime))
                {
                    // If the position is the same, decrement the count so that sample gets discarded by overwriting it
                    i--;
                    WARN(g_pBasicLog, "The same position was reported at %f ms: pos = %f seconds", pPosSet[i].testTime, pPosSet[i].position);
                }
                i++;
            }
            else
            {
                LOG(g_pBasicLog, "GetPosition function too slow");
            }
        }
        // If a non-zero position has not been reported during one second, break the loop and check if there is enough data
        else if ((tPost - tStart) > MILLISECONDS_PER_SECOND)
        {
            break;
        }

        pResults->cPosSets = i;
    }

    VERIFY_SUCCEEDED(pHalfApp->StopStream());

    // Unregister with MMCSS
    threadPriority->UnRegisterWithMMCSS();

    //  Did we get enough data?
    VERIFY_IS_TRUE(pResults->cPosSets >= 30);

    return S_OK;
}

// --------------------------------------------------------------------------------------------------
// CalculateSampleRate
// --------------------------------------------------------------------------------------------------
HRESULT CalculateSampleRate
(
    PFORMAT_ENTRY pFormatEntry
)
{
    WAVEFORMATEX* pwfx = &pFormatEntry->wfxFormat.Format;
    PPERF_RESULTS   pResults = &pFormatEntry->perfResults;

    double dError, dErrorPercent;

    // Pair the positions with test HNS time to find the slope
    // Alocate memory for this type-specific array
    wil::unique_cotaskmem_ptr<DPOINT> pointArray((PDPOINT)CoTaskMemAlloc(DATA_POINTS * sizeof(DPOINT)));
    PDPOINT pArray = pointArray.get();
    // Get a pointer to the sample set
    PPOSITION_SET posSet = pResults->argPosSets;
    for (int i = 0; i < DATA_POINTS; i++)
    {
        pArray[i].x = posSet[i].positionTime;
        pArray[i].y = posSet[i].position * pwfx->nAvgBytesPerSec; // Convert to bytes
    }

    ULONG   cArray = pResults->cPosSets; // number of points

    // calculate the slope and intercept (bytes per second and latency)
    pResults->bytesPerSecond = SLOPE(pArray, cArray);
    pResults->dOffset = INTERCEPT(pArray, cArray, pResults->bytesPerSecond);

    // Convert from bytes/ms to bytes/sec
    pResults->bytesPerSecond *= MILLISECONDS_PER_SECOND;

    // The offset is used in the jitter calculation and it is used in milliseconds, so we can save it in those units
    pResults->dOffset = pResults->dOffset / pResults->bytesPerSecond * MILLISECONDS_PER_SECOND;

    // calculate errors
    dError = pResults->bytesPerSecond - (double)pwfx->nAvgBytesPerSec;
    dErrorPercent = (dError / (double)pwfx->nAvgBytesPerSec) * 100.0;

    if (g_pKsPosTst->m_fLogHistograms)
    {
        LOG(g_pBasicLog, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        LOG(g_pBasicLog, "SampleRate calculated from histogram (method of least squares)\n");

        LOG(g_pBasicLog, "Collected %d data points", pResults->cPosSets);
        LOG(g_pBasicLog, "Specified sample rate  = %6.3f bytes/sec, %6.3f samples/sec", (double)(pwfx->nBlockAlign * pwfx->nSamplesPerSec), (double)pwfx->nSamplesPerSec);
        LOG(g_pBasicLog, "Calculated sample rate = %6.3f bytes/sec, %6.3f samples/sec", pResults->bytesPerSecond, pResults->bytesPerSecond / (double)pwfx->nBlockAlign);
        LOG(g_pBasicLog, "Error = %.3f %%", dErrorPercent);
        LOG(g_pBasicLog, "Calculated time offset = %f ms", pResults->dOffset);
    }

    // evaluate the results
    VERIFY_IS_TRUE(abs(dErrorPercent) <= SAMPLERATE_THRESHOLD);

    return S_OK;
}

// --------------------------------------------------------------------------------------------------
// CalculateJitter
// --------------------------------------------------------------------------------------------------
HRESULT CalculateJitter
(
    PFORMAT_ENTRY pFormatEntry
)
{
    PPERF_RESULTS   pResults = &pFormatEntry->perfResults;

    double          dError        = 0.0;
    double          dErrorAve     = 0.0;
    double          dErrorMax     = 0.0;
    double          dStdDeviation = 0.0;
    double          reportedPosition;
    double          timeDifference;
    ULONG           i;
    PPOSITION_SET   pPosSet = pResults->argPosSets;

    if (g_pKsPosTst->m_fLogHistograms)
    {
        LOG(g_pBasicLog, "Reported    QPC         Compensated Expected");
        LOG(g_pBasicLog, "position    adjust      position    position    Error");
        LOG(g_pBasicLog, "(ms)        (ms)        (ms)        (ms)        (ms)");
        LOG(g_pBasicLog, "==========  ==========  ==========  ==========  ==========");
    }

    for (i = 0; i < pResults->cPosSets; i++)
    {
        // Convert the time offset from seconds to milliseconds
        reportedPosition = pPosSet[i].position * MILLISECONDS_PER_SECOND; // milliseconds

        // First, we compensate for the latency of the samples (they were taken 100 ms after stream start)
        pPosSet[i].position = reportedPosition - pResults->dOffset; // The offset is already in ms

        // Then, before comparing the expected and actual position, adjust the given sample to get the position corresponding to the test time.
        timeDifference = pPosSet[i].testTime - pPosSet[i].positionTime; // milliseconds
        // If the difference is positive, the time of the position is behind of what is expected and that position gets increased accordingly.
        // If the difference is negative, the time of the position is ahead of what is expected and that position gets decreased accordingly.
        // In the rare case when the QPC and time are the same, the difference is zero because we are on the spot and that is the position that should be compared.
        pPosSet[i].position += timeDifference; // milliseconds

        dError = pPosSet[i].testTime - pPosSet[i].position;
        dErrorAve += dError;

        if (g_pKsPosTst->m_fLogHistograms)
        {
            LOG(g_pBasicLog, "%10.2f, %10.2f, %10.2f, %10.2f, %10.2f", reportedPosition, timeDifference, pPosSet[i].position, pPosSet[i].testTime, dError);
        }

        if (dError < 0.)
            dError = -dError;
        if (dError > dErrorMax)
            dErrorMax = dError;
    }

    dErrorAve /= (double)pResults->cPosSets;

    VERIFY_IS_TRUE((ULONG)dErrorAve < 2);      // if this is not true then, there's something wrong with our compensation...

    for (i = 0; i < pResults->cPosSets; i++)
    {
        dError = pPosSet[i].testTime - pPosSet[i].position;
        dStdDeviation += ((dError - dErrorAve) * (dError - dErrorAve));
    }

    dStdDeviation /= (double)pResults->cPosSets;
    dStdDeviation = sqrt(dStdDeviation);

    if (g_pKsPosTst->m_fLogHistograms)
    {
        LOG(g_pBasicLog, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        LOG(g_pBasicLog, "Jitter calculated from histogram (with (samplerate drift and latency) compensation)\n");
        LOG(g_pBasicLog, "Max deviation      = %6.3f ms", dErrorMax);
        LOG(g_pBasicLog, "Standard deviation = %6.3f ms", dStdDeviation);
    }

    // evaluate the results ~~~~~~~~~~~~~
    VERIFY_IS_TRUE(abs(dErrorMax) < JITTER_MAX_THRESHOLD);
    VERIFY_IS_TRUE(abs(dStdDeviation) < JITTER_STD_THRESHOLD);

    return S_OK;
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
void Test_DriftAndJitter_Main
(
    HNSTIME requestedPeriodicity
)
{
    CHalfApp* pHalfApp = g_pKsPosTst->m_pHalf;
    CHalfApp* pHostHalfApp = g_pKsPosTst->m_pHostHalf;
    BOOL      isLoopbackPin = pHalfApp->m_ConnectorType == eLoopbackConnector;

    // Test will ignore Bluetooth devices until proper thresholds are in place.
    if (pHalfApp->m_bIsBluetooth)
    {
        SKIP(g_pBasicLog, "Test is not supported for Bluetooth pins");
        return;
    }

    // If this is a loobpack pin, start the host pin's stream
    // This can be done before collecting the samples, since the host pin will keep playing until stopped
    if (isLoopbackPin)
    {
        // Match loopback stream format with host stream format
        CloneWaveFormat(pHostHalfApp->m_pCurrentFormat.get(), wil::out_param(pHalfApp->m_pCurrentFormat));
        pHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHostHalfApp->m_u32CurrentDefaultPeriodicityInFrames;
        pHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHostHalfApp->m_u32CurrentFundamentalPeriodicityInFrames;
        pHalfApp->m_u32CurrentMinPeriodicityInFrames = pHostHalfApp->m_u32CurrentMinPeriodicityInFrames;
        pHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHostHalfApp->m_u32CurrentMaxPeriodicityInFrames;

        // Initialize and start host stream
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->CreateSineToneDataBuffer(pHostHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeStream(requestedPeriodicity, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pHostHalfApp->StartStream());
    }

    FORMAT_ENTRY results;
    memset(&results, 0, sizeof(FORMAT_ENTRY));
    results.wfxFormat.Format = *pHalfApp->m_pCurrentFormat.get();
    // Alocate memory for the sample data
    wil::unique_cotaskmem_ptr<POSITION_SET> pPositionSet((PPOSITION_SET)CoTaskMemAlloc(DATA_POINTS * sizeof(POSITION_SET)));
    results.perfResults.argPosSets = pPositionSet.get();

    // collect the data ~~~~~~~~~~~~~~~~~
    IF_FAILED_JUMP(CollectSampleData(pHalfApp, &results, requestedPeriodicity), Log);

    // Stop and cleanup the host stream since we are done collecting samples
    if (isLoopbackPin)
    {
        VERIFY_SUCCEEDED(pHostHalfApp->StopStream());
        VERIFY_SUCCEEDED(pHostHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseSineToneDataBuffer());
    }

    // calculate drift ~~~~~~~~~~~~~~~~~~
    IF_FAILED_JUMP(CalculateSampleRate(&results), Log);

    // calculate Jitter ~~~~~~~~~~~~~~~~~
    VERIFY_SUCCEEDED(CalculateJitter(&results));

    return;

Log:
    // If the drift test or the collection of samples fail,
    // there will be no logging of data (from inside the jitter test).
    // So, log whatever available values here.
    if (g_pKsPosTst->m_fLogHistograms)
    {
        PERF_RESULTS* data = &results.perfResults;
        LOG(g_pBasicLog, "Report      Reported    Position");
        LOG(g_pBasicLog, "time        position    timestamp");
        LOG(g_pBasicLog, "(ms)        (s)         (ms)");
        LOG(g_pBasicLog, "==========  ==========  ==========");
        for (UINT i = 0; i < data->cPosSets; i++)
        {
            LOG(g_pBasicLog, "%10.2f, %10.2f, %10.2f", data->argPosSets[i].testTime, data->argPosSets[i].position, data->argPosSets[i].positionTime);
        }
    }

    ERR(g_pBasicLog, "Drift and jitter test failed");
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
void Test_DriftAndJitter_Default(void)
{
    HNSTIME requestedTime = FRAMES_TO_HNSTIME_DOUBLE(g_pKsPosTst->m_pHalf->m_u32CurrentDefaultPeriodicityInFrames, g_pKsPosTst->m_pHalf->m_pCurrentFormat->nSamplesPerSec);
    if (g_pKsPosTst->m_fLogHistograms)
    {
        LOG(g_pBasicLog, "Periodicity: %d frames, %f ms", g_pKsPosTst->m_pHalf->m_u32CurrentDefaultPeriodicityInFrames, HNSTIME_TO_MILLISECONDS_DOUBLE(requestedTime));
    }
    Test_DriftAndJitter_Main(requestedTime);
}

/*
  Perform a cycle of creating a pin, starting a stream, stoping and destroying.

  This is done with the idea of putting the driver and audio hardware in an active state
  before doing time sensitive tests.
*/
void ActivateDriver(CHalfApp* pHalfApp)
{
    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
    VERIFY_SUCCEEDED(pHalfApp->StartStream());
    Sleep(100);
    VERIFY_SUCCEEDED(pHalfApp->StopStream());
    VERIFY_SUCCEEDED(pHalfApp->CleanupStream());
    VERIFY_SUCCEEDED(pHalfApp->ReleaseEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->ReleaseSineToneDataBuffer());
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
void Test_Latency_Main (HNSTIME requestedPeriodicity)
{
    CHalfApp* pHalfApp = g_pKsPosTst->m_pHalf;
    CHalfApp* pHostHalfApp = g_pKsPosTst->m_pHostHalf;
    NTSTATUS  lTimerResStatus;
    BOOL      isLoopbackPin = pHalfApp->m_ConnectorType == eLoopbackConnector;
    PLONGLONG qpcFrequency = &g_pKsPosTst->m_qpcFrequency;
    CRtThreadRegistration* threadPriority = &g_pKsPosTst->m_threadPriority;

    // Test will ignore Bluetooth devices until proper thresholds are in place.
    if (pHalfApp->m_bIsBluetooth)
    {
        SKIP(g_pBasicLog, "Test is not supported for Bluetooth pins");
        return;
    }

    lTimerResStatus = NtSetTimerResolution(timerResolution, TRUE, &actualTimerResolution);

    if (!NT_SUCCESS(lTimerResStatus))
    {
        SKIP(g_pBasicLog, "Fail to set timer precision. Skip the test.");
        return;
    }

    // Register with MMCSS
    threadPriority->RegisterWithMMCSS();

    // If it is a loopback pin, get the host pin and start the stream
    if (isLoopbackPin)
    {
        // Match loopback stream format with host stream format
        CloneWaveFormat(pHostHalfApp->m_pCurrentFormat.get(), wil::out_param(pHalfApp->m_pCurrentFormat));
        pHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHostHalfApp->m_u32CurrentDefaultPeriodicityInFrames;
        pHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHostHalfApp->m_u32CurrentFundamentalPeriodicityInFrames;
        pHalfApp->m_u32CurrentMinPeriodicityInFrames = pHostHalfApp->m_u32CurrentMinPeriodicityInFrames;
        pHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHostHalfApp->m_u32CurrentMaxPeriodicityInFrames;

        // Initialize and start host stream
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->CreateSineToneDataBuffer(pHostHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeStream(requestedPeriodicity, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pHostHalfApp->StartStream());
    }

    // Perform a stream cycle to make sure the driver is on a good state
    ActivateDriver(pHalfApp);

    // Initialize the stream
    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHalfApp->InitializeStream(requestedPeriodicity, DEFAULT_LATENCY_COEFFICIENT));

    UINT64 u64Position = 0;
    UINT64 driverHns = 0;

    double tPosPre = 0.0;
    double tPosPost = 0.0;

    LARGE_INTEGER timeStamp = { 0 };

    // mark the time
    double tRunPre = xGetTime();
    // Start the stream
    VERIFY_SUCCEEDED(pHalfApp->StartStream());
    // mark the time
    double tRunPost = xGetTime();

    LOG(g_pBasicLog, "Starting the stream took %.03f ms", (tRunPost - tRunPre));

    // loop until we get a non-zero position
    while (u64Position == 0)
    {
        Sleep(1);
        // timer before position call
        tPosPre = xGetTime();
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &driverHns));
        QueryPerformanceCounter(&timeStamp);
        // timer after position call
        tPosPost = xGetTime();
        if (g_pKsPosTst->m_fLogHistograms)
        {
            LOG(g_pBasicLog, "Testing first non-zero position reported: %llu samples, %llu HNS at %.03f ms", u64Position, driverHns, tPosPost - tRunPre);
        }

        // Warn if the cursor does not move within 500 ms. Do not fail the test as this might be because the test overslept.
        if ((tPosPost - tRunPost) >= 500.0)
        {
            WARN(g_pBasicLog, "Pin did not report a position in 500 ms");
            break;
        }
    }

    // Before stopping, get the frequency of the stream
    UINT64 u64Frequency;
    VERIFY_SUCCEEDED(pHalfApp->m_pAudioClock->GetFrequency(&u64Frequency));

    // Stop the endpoint
    VERIFY_SUCCEEDED(pHalfApp->StopStream());

    // Unregister with MMCSS
    threadPriority->UnRegisterWithMMCSS();

    // the uncertainty in time is the max possible value minus the min possible value divided by 2 (for +/- purposes)
    double tMax = tPosPost - tRunPre;
    double tMin = tPosPre - tRunPost;
    double tAve = (tMax + tMin) / 2.0;
    double tUncertainty = (tMax - tMin) / 2.0;

    // Convert the stream position to an offset of milliseconds
    double tOffset = MILLISECONDS_PER_SECOND * (double)u64Position / (double)u64Frequency;

    // Calculate the difference in time of the position time versus the test time
    // Convert QPC to microseconds and then HNS. This is to prevent overflows from converting directly to HNS.
    UINT64 testHns = MICROSECONDS_PER_SECOND * timeStamp.QuadPart / *qpcFrequency;
    testHns *= HNSTIME_UNITS_PER_MICROSECOND;

    // Make the HNS times smaller to avoid overflows (or underflows)
    PLONGLONG pBaseHns = &g_pKsPosTst->m_testBaseHns;
    INT64 adjustedTestHns = (INT64)(testHns - *pBaseHns);
    INT64 adjustedDriverHns = (INT64)(driverHns - *pBaseHns);

    INT64 timeDiff = adjustedTestHns - adjustedDriverHns;

    // Compensate the positon to take this difference into account
    tOffset += (double)timeDiff / HNSTIME_UNITS_PER_MILLISECOND;

    if (g_pKsPosTst->m_fLogHistograms)
    {
        LOG(g_pBasicLog, "Base test time: %lld HNS", *pBaseHns);
        LOG(g_pBasicLog, "Times (HNS): %llu, %llu", testHns, driverHns);
        LOG(g_pBasicLog, "Time difference: %lld HNS (%f ms)", timeDiff, (double)timeDiff / HNSTIME_UNITS_PER_MILLISECOND);
        LOG(g_pBasicLog, "First non-zero position = %llu at %g ms", u64Position, tAve);
        LOG(g_pBasicLog, "Time offset / max time: %f ms / %f ms", tOffset, tMax);
        LOG(g_pBasicLog, "Frequency: %llu", u64Frequency);
    }

    double           tLatency;
    tLatency = tAve - tOffset;
    if (tLatency < 0)
    {
        // This means that the position is between the average and max times, which should be OK (as long as it is not above the max)
        tLatency *= -1;
        LOG(g_pBasicLog, "The position is greater than the average time");
    }
    LOG(g_pBasicLog, "Adjusted latency                 = %.03f ms (+/- %.3f ms)", tLatency, tUncertainty);

    // In case the position reported by the pin (tOffset) is ahead of the run time of the pin (tMax),
    // assume that position might have jitter and apply the propper limits instead of failing the test.
    if (tOffset > tMax + JITTER_STD_THRESHOLD)
    {
        WARN(g_pBasicLog, "Pin's time offset is greater than the time it has been in the RUN state. This will become an error in the future.");
        return;
    }

    // Verify that the latency is within the threshold
    if (tLatency > LATENCY_THRESHOLD)
    {
        WARN(g_pBasicLog, "The latency exceeds the acceptable threshold. This will become an error in the future.");
        return;
    }

    // If this was a loopback pin, stop the host stream also
    if (isLoopbackPin)
    {
        VERIFY_SUCCEEDED(pHostHalfApp->StopStream());
        VERIFY_SUCCEEDED(pHostHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseSineToneDataBuffer());
    }

    // Turn off the higher resolution timer
    NtSetTimerResolution(0, FALSE, &actualTimerResolution);
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
void Test_Latency_Default(void)
{
    HNSTIME requestedTime = FRAMES_TO_HNSTIME_DOUBLE(g_pKsPosTst->m_pHalf->m_u32CurrentDefaultPeriodicityInFrames, g_pKsPosTst->m_pHalf->m_pCurrentFormat->nSamplesPerSec);
    LOG(g_pBasicLog, "Periodicity: %d frames, %f ms", g_pKsPosTst->m_pHalf->m_u32CurrentDefaultPeriodicityInFrames, HNSTIME_TO_MILLISECONDS_DOUBLE(requestedTime));
    Test_Latency_Main(requestedTime);
}
