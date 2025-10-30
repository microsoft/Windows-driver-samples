// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  WaveTestTaef.cpp
//
// Abstract:
//
//  Implementation file for WaveTestTaef
//
// -------------------------------------------------------------------------------
#include "PreComp.h"
#include "WaveTestTaef.h"
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
WDMAudio::WaveTest * g_pWaveTest = NULL;

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
bool WDMAudio::WaveTest::WaveTestSetup()
{
    g_pWaveTest = this;

    return true;
}

bool WDMAudio::WaveTest::WaveTestCleanUp()
{
    g_pWaveTest = NULL;

    return true;
}

// -------------------------------------------------------------------------------
bool WDMAudio::WaveTest::TestCaseSetup()
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

    return true;
}

bool WDMAudio::WaveTest::TestCaseCleanUp()
{
    if (!CompareWaveFormat(m_pHalf->m_pCurrentFormat.get(), m_pHalf->m_pPreferredFormat.get())) {
        CloneWaveFormat(m_pHalf->m_pPreferredFormat.get(), wil::out_param(m_pHalf->m_pCurrentFormat));
        m_pHalf->m_u32CurrentDefaultPeriodicityInFrames = m_pHalf->m_u32DefaultPeriodicityInFrames;
        m_pHalf->m_u32CurrentFundamentalPeriodicityInFrames = m_pHalf->m_u32FundamentalPeriodicityInFrames;
        m_pHalf->m_u32CurrentMinPeriodicityInFrames = m_pHalf->m_u32MinPeriodicityInFrames;
        m_pHalf->m_u32CurrentMaxPeriodicityInFrames = m_pHalf->m_u32MaxPeriodicityInFrames;
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
void WDMAudio::WaveTestPreTest::TAEF_VerifyAllEndpointsPluggedIn()
{
    VerifyAllEndpointsPluggedIn();
}

// -------------------------------------------------------------------------------
void WDMAudio::WaveTest::TAEF_StreamStateControl()
{
    RUN_TEST_CASE(Test_StreamStateControl)
}

void WDMAudio::WaveTest::TAEF_LoopbackStreamStateControl()
{
    RUN_TEST_CASE(Test_LoopbackStreamStateControl)
}

void WDMAudio::WaveTest::TAEF_StreamMinBufferSize()
{
    RUN_TEST_CASE(Test_StreamMinBufferSize)
}

void WDMAudio::WaveTest::TAEF_StreamMaxBufferSize()
{
    RUN_TEST_CASE(Test_StreamMaxBufferSize)
}

void WDMAudio::WaveTest::TAEF_StreamDifferentFormat()
{
    RUN_TEST_CASE(Test_StreamDifferentFormat)

}

void WDMAudio::WaveTest::TAEF_LoopbackStreamDifferentFormat()
{
    RUN_TEST_CASE(Test_LoopbackStreamDifferentFormat)

}

void WDMAudio::WaveTest::TAEF_StreamMultipleModes()
{
    RUN_TEST_CASE(Test_StreamMultipleModes)
}

void WDMAudio::WaveTest::TAEF_VerifyPinWaveRTConformance()
{
    RUN_TEST_CASE(Test_VerifyPinWaveRTConformance)
}
