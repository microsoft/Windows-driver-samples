// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  KsPosTestTaef.cpp
//
// Abstract:
//
//  Implementation file for KsPosTestTaef
//
// -------------------------------------------------------------------------------
#include "PreComp.h"
#include "KsPosTestTaef.h"
#include "tests.h"
#include <TestResourceHelper.h>

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#define RUN_TEST_CASE(testfn) \
{ \
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures); \
    testfn(); \
}

IBasicLog * g_pBasicLog = NULL;
WDMAudio::KsPosTest* g_pKsPosTst = NULL;

TIMER_MECHANISM g_Timer =
{
    tpQPC, "QueryPerformanceCounter"
};

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

// -------------------------------------------------------------------------------
namespace WDMAudio
{
    BEGIN_MODULE()
        MODULE_PROPERTY(L"Feature", L"PortCls HCK Tests")
        MODULE_PROPERTY(L"TestResourceDependent", L"true")
        END_MODULE()

        // Create WEXBasicLog, Coinitialize Etc
        MODULE_SETUP(WDMAudioSetup)

        // Release stuff acquired in WDMAudioSetup
        MODULE_CLEANUP(WDMAudioCleanup)
};

bool WDMAudio::WDMAudioSetup()
{
    if (NULL == g_pBasicLog)
    {
        if (!(VERIFY_SUCCEEDED(CreateWexBasicLog(&g_pBasicLog))))
        {
            return false;
        }
    }

    return true;
}

bool WDMAudio::WDMAudioCleanup()
{
    if (g_pBasicLog)
    {
        g_pBasicLog->Release();
    }

    return true;
}

// -------------------------------------------------------------------------------
bool WDMAudio::KsPosTest::KsPosTestSetup()
{
    g_pKsPosTst = this;
    m_pTimer = &g_Timer;
    // The histograms can be displayed by using the command line parameter checked below
    // If the parameter is not specified, the default is to log everything.
    bool param = true;
    WEX::TestExecution::RuntimeParameters::TryGetValue(L"logHistograms", param);
    m_fLogHistograms = param;

    // Get the QPC frequency for timing on the tests.
    // This can be done here as the frequency does not change.
    LARGE_INTEGER liFrequency;
    QueryPerformanceFrequency(&liFrequency);
    m_qpcFrequency = liFrequency.QuadPart;
    LOG(g_pBasicLog, "Frequency: %llu", m_qpcFrequency);

    return true;
}

bool WDMAudio::KsPosTest::KsPosTestCleanUp()
{
    g_pKsPosTst = NULL;

    return true;
}

// -------------------------------------------------------------------------------
bool WDMAudio::KsPosTest::TestCaseSetup()
{
    CComPtr<ITestResource> spResource;
    CComQIPtr<IHalfAppContainer> spHalfContainer;

    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    Log::Comment(L"");
    Log::Comment(L"------------------------------------------------------");
    Log::Comment(L"Running test case setup");

    size_t count = Resources::Count();
    if (!VERIFY_ARE_EQUAL(count, (size_t)1)) { return false; }
    if (!VERIFY_SUCCEEDED(Resources::Item(0, &spResource))) { return false; }

    // 1. Assign m_pHalf
    spHalfContainer = spResource;
    if (!VERIFY_IS_NOT_NULL(spHalfContainer)) { return false; }
    if (!VERIFY_SUCCEEDED(spHalfContainer->GetHalfApp(&m_pHalf))) { return false; }

    // 2. Check if this is a loopback pin. If so, assign the host pin's HalfApp
    if (m_pHalf->m_ConnectorType == eLoopbackConnector)
    {
        VERIFY_SUCCEEDED(m_pHalf->GetHostHalfApp(&m_pHostHalf));
    }
    else
    {
        m_pHostHalf = NULL;
    }

    Log::Comment(L"The test will run on the following format:");
    if (m_pHostHalf != NULL)
    {
        LogWaveFormat(m_pHostHalf->m_pCurrentFormat.get());
    }
    else
    {
        LogWaveFormat(m_pHalf->m_pCurrentFormat.get());
    }

    // Get a HNS reference from the begining of the test so the HNS from system and driver times won't be so large
    LARGE_INTEGER liNow;
    QueryPerformanceCounter(&liNow);

    // Convert QPC to microseconds and then HNS. This is to prevent overflows from converting directly to HNS.
    m_testBaseHns = (liNow.QuadPart * MICROSECONDS_PER_SECOND) / m_qpcFrequency;
    m_testBaseHns *= HNSTIME_UNITS_PER_MICROSECOND;

    return true;
}

bool WDMAudio::KsPosTest::TestCaseCleanUp()
{
    // Stop the timer here, in case any latency test fails
    NtSetTimerResolution(0, FALSE, &actualTimerResolution);

    // Attempt to unregister the thread with MMCSS just in case the test failed before doing so.
    m_threadPriority.UnRegisterWithMMCSS();

    if (!CompareWaveFormat(m_pHalf->m_pCurrentFormat.get(), m_pHalf->m_pPreferredFormat.get())) {
        CloneWaveFormat(m_pHalf->m_pPreferredFormat.get(), wil::out_param(m_pHalf->m_pCurrentFormat));
        m_pHalf->m_u32CurrentDefaultPeriodicityInFrames = m_pHalf->m_u32DefaultPeriodicityInFrames;
        m_pHalf->m_u32CurrentFundamentalPeriodicityInFrames = m_pHalf->m_u32FundamentalPeriodicityInFrames;
        m_pHalf->m_u32CurrentMinPeriodicityInFrames = m_pHalf->m_u32MinPeriodicityInFrames;
        m_pHalf->m_u32CurrentMaxPeriodicityInFrames = m_pHalf->m_u32MaxPeriodicityInFrames;
    }

    if (m_pHostHalf != NULL) {
        if (!VERIFY_SUCCEEDED(m_pHostHalf->CleanupStream())) { return false; }
        if (!VERIFY_SUCCEEDED(m_pHostHalf->ReleaseEndpoint())) { return false; }
        if (!VERIFY_SUCCEEDED(m_pHostHalf->ReleaseSineToneDataBuffer())) { return false; }
        delete m_pHostHalf;
        m_pHostHalf = NULL;
    }

    if (m_pHalf != NULL) {
        if (!VERIFY_SUCCEEDED(m_pHalf->CleanupStream())) { return false; }
        if (!VERIFY_SUCCEEDED(m_pHalf->ReleaseEndpoint())) { return false; }
        if (!VERIFY_SUCCEEDED(m_pHalf->ReleaseSineToneDataBuffer())) { return false; }

        m_pHalf = NULL;
    }

    return true;
}

// -------------------------------------------------------------------------------
void WDMAudio::KsPosTestPreTest::TAEF_VerifyAllEndpointsPluggedIn()
{
    VerifyAllEndpointsPluggedIn();
}

// -------------------------------------------------------------------------------
void WDMAudio::KsPosTest::TAEF_DriftAndJitter()
{
    RUN_TEST_CASE(Test_DriftAndJitter_Default);
}

void WDMAudio::KsPosTest::TAEF_Latency()
{
    RUN_TEST_CASE(Test_Latency_Default);
}
