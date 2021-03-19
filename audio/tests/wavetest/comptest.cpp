// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// Module Name:
//
//  comptest.cpp
//
// Abstract:
//
//  Implementation file for test cases
//
// -------------------------------------------------------------------------------

#include "PreComp.h"
#include "WaveTestTaef.h"
#include "tests.h"

// ------------------------------------------------------------------------------
// Test_VerifyPinSupportsPullMode: Test the requirement that new drivers must be WaveRT and WaveRT drivers must support event driven audio streaming technology.
void Test_VerifyPinWaveRTConformance(void)
{
    CHalfApp*                       pHalfApp = g_pWaveTest->m_pHalf;
    BOOL                            bIsRTCapable = false;
    BOOL                            bIsEventCapable = false;

    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());

    VERIFY_SUCCEEDED(pHalfApp->m_pAudioDeviceEndpoint->GetRTCaps(&bIsRTCapable));

    // Check for our new requirement - new drivers must be WaveRT. Here we skip the test on AVStream (UAC1/HFP/A2dp)
    // For other old drivers that are not WaveRT, may issue Errata to cover the failure.
    if (!pHalfApp->m_bIsAVStream)
    {
        VERIFY_IS_TRUE(bIsRTCapable);
    }

    // Only require event driven for WaveRT drivers
    if (bIsRTCapable)
    {
        VERIFY_SUCCEEDED(pHalfApp->m_pAudioDeviceEndpoint->GetEventDrivenCapable(&bIsEventCapable));
        VERIFY_IS_TRUE(bIsEventCapable);
    }
}
