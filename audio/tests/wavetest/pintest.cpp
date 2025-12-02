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
#include "WaveTestTaef.h"
#include "tests.h"

#define DEFAULT_PERIODICITY 0
#define DEFAULT_LATENCY_COEFFICIENT 0

static const struct
{
    GUID mode;
    LPWSTR name;
}knownModes[] =
{
    { GUID_NULL, L"NO_MODE" },
    { AUDIO_SIGNALPROCESSINGMODE_RAW, L"RAW" },
    { AUDIO_SIGNALPROCESSINGMODE_DEFAULT, L"DEFAULT" },
    { AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS, L"COMMUNICATIONS" },
    { AUDIO_SIGNALPROCESSINGMODE_SPEECH, L"SPEECH" },
    { AUDIO_SIGNALPROCESSINGMODE_MEDIA, L"MEDIA" },
    { AUDIO_SIGNALPROCESSINGMODE_MOVIE, L"MOVIE" },
    { AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION, L"NOTIFICATION" },
    { AUDIO_SIGNALPROCESSINGMODE_FAR_FIELD_SPEECH, L"FAR_FIELD_SPEECH"},
};


LPWSTR ModeName(REFGUID guidMode)
{
    for (auto m : knownModes)
    {
        if (m.mode == guidMode) { return m.name; }
    }

    return L"UNKNOWN";
}

void LogWaveFormat(WAVEFORMATEX* pwfx)
{
    WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pwfx;

    switch (pwfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        LOG(g_pBasicLog, L"    wFormatTag          = WAVE_FORMAT_PCM");
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        LOG(g_pBasicLog, L"    wFormatTag          = WAVE_FORMAT_IEEE_FLOAT");
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        LOG(g_pBasicLog, L"    wFormatTag          = WAVE_FORMAT_EXTENSIBLE");
        if (wfex->SubFormat.Data1 == WAVE_FORMAT_PCM)
            LOG(g_pBasicLog, L"    SubFormat           = WAVE_FORMAT_PCM");
        if (wfex->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT)
            LOG(g_pBasicLog, L"    SubFormat           = WAVE_FORMAT_IEEE_FLOAT");
        break;
    default:
        LOG(g_pBasicLog, L"    wFormatTag          = UNKNOWN!");
    }
    LOG(g_pBasicLog, L"    nChannel            = %d", pwfx->nChannels);
    LOG(g_pBasicLog, L"    nSamplesPerSec      = %d", pwfx->nSamplesPerSec);
    LOG(g_pBasicLog, L"    nAvgBytesPerSe      = %d", pwfx->nAvgBytesPerSec);
    LOG(g_pBasicLog, L"    nBlockAlign         = %d", pwfx->nBlockAlign);
    LOG(g_pBasicLog, L"    wBitsPerSample      = %d", pwfx->wBitsPerSample);


    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        LOG(g_pBasicLog, L"    wValidBitsPerSample = %d", wfex->Samples.wValidBitsPerSample);
        LOG(g_pBasicLog, L"    dwChannelMask       = %hd", wfex->dwChannelMask);
    }
}

// ------------------------------------------------------------------------------
// Test_StreamStateControl: Test the stream state contorl of an endpoint. In RUN state pin position should be moving,
// in PAUSE state pin position should be same all time, in STOP state pin position should be zero.
void Test_StreamStateControl(void)
{
    CHalfApp* pHalfApp = g_pWaveTest->m_pHalf;
    UINT32 u32Index = 0;
    UINT64 u64PreviousPosition;
    UINT64 u64Position;
    UINT64 u64CorrelatedSystemTime;

    // Log wave format
    LOG(g_pBasicLog, L"Test case will be run on the following format:");
    LogWaveFormat(pHalfApp->m_pCurrentFormat.get());

    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));

    // Stream start state
    VERIFY_SUCCEEDED(pHalfApp->StartStream());
    Sleep(100);
    VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position != u64PreviousPosition);
        u64PreviousPosition = u64Position;
    }

    // Stream stop state
    // IAudioEndpointControl::Stop set pin to PAUSE state
    VERIFY_SUCCEEDED(pHalfApp->StopEndpoint()); 
    Sleep(100);
    VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position == u64PreviousPosition);
        u64PreviousPosition = u64Position;
    }

    // Stream reset state
    // IAudioEndpointControl::Reset set pin to STOP state
    VERIFY_SUCCEEDED(pHalfApp->ResetEndpoint());
    Sleep(100);
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position == 0);
    }
    
    VERIFY_SUCCEEDED(pHalfApp->StartEndpoint());
    Sleep(100);

    VERIFY_SUCCEEDED(pHalfApp->StopStream());
}

// ------------------------------------------------------------------------------
// Test_StreamMinBufferSize: Test the stream with minimum processing time (buffer size).
void Test_StreamMinBufferSize(void)
{
    CHalfApp* pHalfApp = g_pWaveTest->m_pHalf;

    // Set buffer with mininum periodicity
    Test_StreamBufferSize(FRAMES_TO_HNSTIME_DOUBLE(pHalfApp->m_u32CurrentMinPeriodicityInFrames, pHalfApp->m_pCurrentFormat->nSamplesPerSec));
}

// ------------------------------------------------------------------------------
// Test_StreamMaxBufferSize: Test the stream with maximum processing time (buffer size).
void Test_StreamMaxBufferSize(void)
{
    CHalfApp* pHalfApp = g_pWaveTest->m_pHalf;

    // Set buffer with maximum periodicity
    Test_StreamBufferSize(FRAMES_TO_HNSTIME_DOUBLE(pHalfApp->m_u32CurrentMaxPeriodicityInFrames, pHalfApp->m_pCurrentFormat->nSamplesPerSec));
}

// ------------------------------------------------------------------------------
// Test_StreamBufferSize: Test the stream with requested processing time (buffer size).
void Test_StreamBufferSize(HNSTIME requestedPeriodicity)
{
    CHalfApp* pHalfApp = g_pWaveTest->m_pHalf;
    UINT32 u32Index = 0;
    UINT64 u64PreviousPosition;
    UINT64 u64Position;
    UINT64 u64CorrelatedSystemTime;

    // Log wave format
    LOG(g_pBasicLog, L"Test case will be run on the following format:");
    LogWaveFormat(pHalfApp->m_pCurrentFormat.get());

    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHalfApp->InitializeStream(requestedPeriodicity, DEFAULT_LATENCY_COEFFICIENT));
    VERIFY_SUCCEEDED(pHalfApp->StartStream());

    Sleep(100);

    // Check pin position
    VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position != u64PreviousPosition);
        u64PreviousPosition = u64Position;
    }

    VERIFY_SUCCEEDED(pHalfApp->StopStream());
}

// ------------------------------------------------------------------------------
// Test_StreamDifferentFormat: Test the stream with all supported formats.
void Test_StreamDifferentFormat(void)
{
    CHalfApp* pHalfApp = g_pWaveTest->m_pHalf;
    UINT32 u32FormatIndex = 0;
    UINT32 u32Index = 0;
    UINT64 u64PreviousPosition;
    UINT64 u64Position;
    UINT64 u64CorrelatedSystemTime;

    // Loop through supported formats and stream with different format
    for (;u32FormatIndex < pHalfApp->m_cFormatRecords; u32FormatIndex++) {

        // Change the current format
        CloneWaveFormat((WAVEFORMATEX *)&pHalfApp->m_pFormatRecords[u32FormatIndex].wfxEx, wil::out_param(pHalfApp->m_pCurrentFormat));
        pHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHalfApp->m_pFormatRecords[u32FormatIndex].defaultPeriodInFrames;
        pHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHalfApp->m_pFormatRecords[u32FormatIndex].fundamentalPeriodInFrames;
        pHalfApp->m_u32CurrentMinPeriodicityInFrames = pHalfApp->m_pFormatRecords[u32FormatIndex].minPeriodInFrames;
        pHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHalfApp->m_pFormatRecords[u32FormatIndex].maxPeriodInFrames;

        // Log the current format
        LOG(g_pBasicLog, L"Current format:");
        LogWaveFormat(pHalfApp->m_pCurrentFormat.get());

        // Initialize endpoints and start stream with current format
        VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pHalfApp->CreateSineToneDataBuffer(pHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pHalfApp->StartStream());

        Sleep(100);

        // Check pin position
        VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
        for (u32Index = 0; u32Index < 10; u32Index++) {
            Sleep(50);
            VERIFY_SUCCEEDED(pHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
            VERIFY_IS_TRUE(u64Position != u64PreviousPosition);
            u64PreviousPosition = u64Position;
        }

        // Make sure to stop stearm and clean up stream, endpoints and data buffer before change to next format
        VERIFY_SUCCEEDED(pHalfApp->StopStream());
        VERIFY_SUCCEEDED(pHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pHalfApp->ReleaseSineToneDataBuffer());
    }
}

// ------------------------------------------------------------------------------
// Test_StreamMultipleModes: Create 2 pin instances in different mode (if supported) and test streaming.
void Test_StreamMultipleModes(void)
{
    CHalfApp*                       pMainHalfApp = g_pWaveTest->m_pHalf;
    UINT32                          u32PinInstanceCount = 0;
    AUDIO_SIGNALPROCESSINGMODE      secondMode;
    CHalfApp*                       pSecondHalfApp;
    UINT32 u32Index = 0;
    UINT64 u64PreviousPosition1;
    UINT64 u64PreviousPosition2;
    UINT64 u64Position1;
    UINT64 u64Position2;
    UINT64 u64CorrelatedSystemTime1;
    UINT64 u64CorrelatedSystemTime2;

    // Check processing mode count
    if (pMainHalfApp->m_cModes < 2) {
        SKIP(g_pBasicLog, L"SKIP: Current pin does not support multiple modes. Skipping multiple modes test. ");
        return;
    }
    // Check pin instance count, if less that 2, then multiple pin streaming is not supported
    VERIFY_SUCCEEDED(pMainHalfApp->GetCurrentAvailiablePinInstanceCount(&u32PinInstanceCount));
    if (u32PinInstanceCount < 2) {
        SKIP(g_pBasicLog, L"SKIP: Current availiable pin instance count is less than 2. Skipping multiple modes test. ");
        return;
    }

    // Loop through each mode, create second half app, and start stream
    for (ULONG i = 0; i < pMainHalfApp->m_cModes; i++) {
        secondMode = pMainHalfApp->m_pModes[i];
        if (IsEqualGUID(pMainHalfApp->m_Mode, secondMode)) {
            continue;
        }

        VERIFY_SUCCEEDED(pMainHalfApp->GetSecondHalfApp(secondMode, &pSecondHalfApp));

        // Log both modes and formats
        LOG(g_pBasicLog, L"First stream is in %s mode", ModeName(pMainHalfApp->m_Mode));
        LOG(g_pBasicLog, L"First stream will be run on the following format:");
        LogWaveFormat(pMainHalfApp->m_pCurrentFormat.get());
        LOG(g_pBasicLog, L"Second stream is in %s mode", ModeName(pSecondHalfApp->m_Mode));
        LOG(g_pBasicLog, L"Second stream will be run on the following format:");
        LogWaveFormat(pSecondHalfApp->m_pCurrentFormat.get());

        // Initialize and start first stream
        VERIFY_SUCCEEDED(pMainHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pMainHalfApp->CreateSineToneDataBuffer(pMainHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pMainHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pMainHalfApp->StartStream());

        Sleep(100);

        // Initialize and start second stream
        VERIFY_SUCCEEDED(pSecondHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pSecondHalfApp->CreateSineToneDataBuffer(pSecondHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pSecondHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pSecondHalfApp->StartStream());

        Sleep(100);

        // Check position for two streams
        VERIFY_SUCCEEDED(pMainHalfApp->GetPosition(&u64PreviousPosition1, &u64CorrelatedSystemTime1));
        VERIFY_SUCCEEDED(pSecondHalfApp->GetPosition(&u64PreviousPosition2, &u64CorrelatedSystemTime2));
        for (u32Index = 0; u32Index < 10; u32Index++) {
            Sleep(50);
            VERIFY_SUCCEEDED(pMainHalfApp->GetPosition(&u64Position1, &u64CorrelatedSystemTime1));
            VERIFY_SUCCEEDED(pSecondHalfApp->GetPosition(&u64Position2, &u64CorrelatedSystemTime2));
            VERIFY_IS_TRUE(u64Position1 != u64PreviousPosition1);
            VERIFY_IS_TRUE(u64Position2 != u64PreviousPosition2);
            u64PreviousPosition1 = u64Position1;
            u64PreviousPosition2 = u64Position2;
        }

        // Stop and cleanup first stream
        VERIFY_SUCCEEDED(pMainHalfApp->StopStream());
        VERIFY_SUCCEEDED(pMainHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pMainHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pMainHalfApp->ReleaseSineToneDataBuffer());

        // Stop and cleanup second stream
        VERIFY_SUCCEEDED(pSecondHalfApp->StopStream());
        VERIFY_SUCCEEDED(pSecondHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pSecondHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pSecondHalfApp->ReleaseSineToneDataBuffer());
        delete pSecondHalfApp;
    }
}

// ------------------------------------------------------------------------------
// Test_LoopbackStreamStateControl: Test the stream state contorl of an loopback endpoint. In RUN state pin position should
// be moving, in PAUSE state pin position should be same all time, in STOP state pin position should be zero.
void Test_LoopbackStreamStateControl(void)
{
    CHalfApp*                       pLoopbackHalfApp = g_pWaveTest->m_pHalf;
    CHalfApp*                       pHostHalfApp;
    UINT32                          u32Index = 0;
    UINT64                          u64PreviousPosition;
    UINT64                          u64Position;
    UINT64                          u64CorrelatedSystemTime;

    // Get the corresponding host half app.
    VERIFY_SUCCEEDED(pLoopbackHalfApp->GetHostHalfApp(&pHostHalfApp));

    // Match loopback stream format with host stream format
    CloneWaveFormat(pHostHalfApp->m_pCurrentFormat.get(), wil::out_param(pLoopbackHalfApp->m_pCurrentFormat));
    pLoopbackHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHostHalfApp->m_u32CurrentDefaultPeriodicityInFrames;
    pLoopbackHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHostHalfApp->m_u32CurrentFundamentalPeriodicityInFrames;
    pLoopbackHalfApp->m_u32CurrentMinPeriodicityInFrames = pHostHalfApp->m_u32CurrentMinPeriodicityInFrames;
    pLoopbackHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHostHalfApp->m_u32CurrentMaxPeriodicityInFrames;

    // Log wave format
    LOG(g_pBasicLog, L"Test case will be run on the following format:");
    LogWaveFormat(pLoopbackHalfApp->m_pCurrentFormat.get());

    // Initialize and start host stream
    VERIFY_SUCCEEDED(pHostHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pHostHalfApp->CreateSineToneDataBuffer(pHostHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pHostHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
    VERIFY_SUCCEEDED(pHostHalfApp->StartStream());

    // Initialize and start loopback stream
    VERIFY_SUCCEEDED(pLoopbackHalfApp->InitializeEndpoint());
    VERIFY_SUCCEEDED(pLoopbackHalfApp->CreateSineToneDataBuffer(pLoopbackHalfApp->m_pCurrentFormat.get()));
    VERIFY_SUCCEEDED(pLoopbackHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));

    // Stream start state
    VERIFY_SUCCEEDED(pLoopbackHalfApp->StartStream());
    Sleep(100);
    VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position != u64PreviousPosition);
        u64PreviousPosition = u64Position;
    }

    // Stream stop state
    // IAudioEndpointControl::Stop set pin to PAUSE state
    VERIFY_SUCCEEDED(pLoopbackHalfApp->StopEndpoint());
    Sleep(100);
    VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position == u64PreviousPosition);
        u64PreviousPosition = u64Position;
    }

    // Stream reset state
    // IAudioEndpointControl::Reset set pin to STOP state
    VERIFY_SUCCEEDED(pLoopbackHalfApp->ResetEndpoint());
    Sleep(100);
    for (u32Index = 0; u32Index < 10; u32Index++) {
        Sleep(50);
        VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
        VERIFY_IS_TRUE(u64Position == 0);
    }

    VERIFY_SUCCEEDED(pLoopbackHalfApp->StartEndpoint());
    Sleep(100);

    VERIFY_SUCCEEDED(pLoopbackHalfApp->StopStream());

    // Stop and cleanup host stream
    VERIFY_SUCCEEDED(pHostHalfApp->StopStream());
    VERIFY_SUCCEEDED(pHostHalfApp->CleanupStream());
    VERIFY_SUCCEEDED(pHostHalfApp->ReleaseEndpoint());
    VERIFY_SUCCEEDED(pHostHalfApp->ReleaseSineToneDataBuffer());

    delete pHostHalfApp;
}

// ------------------------------------------------------------------------------
// Test_LoopbackStreamDifferentFormat: Test the loopback stream with all formats supported by host pin.
void Test_LoopbackStreamDifferentFormat(void)
{
    CHalfApp*                       pLoopbackHalfApp = g_pWaveTest->m_pHalf;
    CHalfApp*                       pHostHalfApp;
    UINT32                          u32FormatIndex = 0;
    UINT32                          u32Index = 0;
    UINT64                          u64PreviousPosition;
    UINT64                          u64Position;
    UINT64                          u64CorrelatedSystemTime;

    // Get the corresponding host half app.
    VERIFY_SUCCEEDED(pLoopbackHalfApp->GetHostHalfApp(&pHostHalfApp));

    // Loop through supported formats and stream with different format
    for (;u32FormatIndex < pHostHalfApp->m_cFormatRecords; u32FormatIndex++) {

        // Change the current format
        CloneWaveFormat((WAVEFORMATEX *)&pHostHalfApp->m_pFormatRecords[u32FormatIndex].wfxEx, wil::out_param(pHostHalfApp->m_pCurrentFormat));
        pHostHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHostHalfApp->m_pFormatRecords[u32FormatIndex].defaultPeriodInFrames;
        pHostHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHostHalfApp->m_pFormatRecords[u32FormatIndex].fundamentalPeriodInFrames;
        pHostHalfApp->m_u32CurrentMinPeriodicityInFrames = pHostHalfApp->m_pFormatRecords[u32FormatIndex].minPeriodInFrames;
        pHostHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHostHalfApp->m_pFormatRecords[u32FormatIndex].maxPeriodInFrames;

        // Match loopback stream format with host stream format
        CloneWaveFormat(pHostHalfApp->m_pCurrentFormat.get(), wil::out_param(pLoopbackHalfApp->m_pCurrentFormat));
        pLoopbackHalfApp->m_u32CurrentDefaultPeriodicityInFrames = pHostHalfApp->m_u32CurrentDefaultPeriodicityInFrames;
        pLoopbackHalfApp->m_u32CurrentFundamentalPeriodicityInFrames = pHostHalfApp->m_u32CurrentFundamentalPeriodicityInFrames;
        pLoopbackHalfApp->m_u32CurrentMinPeriodicityInFrames = pHostHalfApp->m_u32CurrentMinPeriodicityInFrames;
        pLoopbackHalfApp->m_u32CurrentMaxPeriodicityInFrames = pHostHalfApp->m_u32CurrentMaxPeriodicityInFrames;

        // Log the current format
        LOG(g_pBasicLog, L"Current format:");
        LogWaveFormat(pLoopbackHalfApp->m_pCurrentFormat.get());

        // Initialize and start host stream
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->CreateSineToneDataBuffer(pHostHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pHostHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pHostHalfApp->StartStream());

        // Initialize and start loopback stream
        VERIFY_SUCCEEDED(pLoopbackHalfApp->InitializeEndpoint());
        VERIFY_SUCCEEDED(pLoopbackHalfApp->CreateSineToneDataBuffer(pLoopbackHalfApp->m_pCurrentFormat.get()));
        VERIFY_SUCCEEDED(pLoopbackHalfApp->InitializeStream(DEFAULT_PERIODICITY, DEFAULT_LATENCY_COEFFICIENT));
        VERIFY_SUCCEEDED(pLoopbackHalfApp->StartStream());

        Sleep(100);

        // Check pin position
        VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64PreviousPosition, &u64CorrelatedSystemTime));
        for (u32Index = 0; u32Index < 10; u32Index++) {
            Sleep(50);
            VERIFY_SUCCEEDED(pLoopbackHalfApp->GetPosition(&u64Position, &u64CorrelatedSystemTime));
            VERIFY_IS_TRUE(u64Position != u64PreviousPosition);
            u64PreviousPosition = u64Position;
        }

        // Make sure to stop and clean up loopback stream, endpoints and data buffer before change to next format
        VERIFY_SUCCEEDED(pLoopbackHalfApp->StopStream());
        VERIFY_SUCCEEDED(pLoopbackHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pLoopbackHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pLoopbackHalfApp->ReleaseSineToneDataBuffer());

        // Stop and cleanup host stream
        VERIFY_SUCCEEDED(pHostHalfApp->StopStream());
        VERIFY_SUCCEEDED(pHostHalfApp->CleanupStream());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseEndpoint());
        VERIFY_SUCCEEDED(pHostHalfApp->ReleaseSineToneDataBuffer());
    }

    delete pHostHalfApp;
}
