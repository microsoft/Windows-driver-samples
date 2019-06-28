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
// Test_VerifyPinSupportsPullMode: Test the requirement that WaveRT drivers must support pull mode audio streaming technology.
void Test_VerifyPinSupportsPullMode(void)
{
    CHalfApp*                       pHalfApp = g_pWaveTest->m_pHalf;
    BOOL                            bIsRTCapable = false;
    BOOL                            bIsEventCapable = false;

    VERIFY_SUCCEEDED(pHalfApp->InitializeEndpoint());

    VERIFY_SUCCEEDED(pHalfApp->m_pAudioDeviceEndpoint->GetRTCaps(&bIsRTCapable));

    // Only require pull mode for WaveRT drivers
    if (bIsRTCapable)
    {
        VERIFY_SUCCEEDED(pHalfApp->m_pAudioDeviceEndpoint->GetEventDrivenCapable(&bIsEventCapable));
        VERIFY_IS_TRUE(bIsEventCapable);
    }
}