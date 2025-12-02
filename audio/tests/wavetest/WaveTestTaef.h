// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  WaveTestTaef.h
//
// Abstract:
//
//  Include file for WaveTestTaef
//
// -------------------------------------------------------------------------------
#pragma once

#include <HalfApp.h>

extern IBasicLog*   g_pBasicLog;

// Define custom test resource properties
namespace WEX {
    namespace TestExecution {
        namespace TestResourceProperty
        {
            static const wchar_t c_szMode[] = L"Mode";
            static const wchar_t c_szPin[] = L"Pin";
        }
    }
}

namespace WDMAudio
{
    class WaveTestPreTest
    {
        BEGIN_TEST_CLASS(WaveTestPreTest)
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x86")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Server x64")
            TEST_CLASS_PROPERTY(L"Kits.MinRelease", L"19H1")
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"Full")
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"OnecoreUAP")
            TEST_CLASS_PROPERTY(L"Kits.IsInProc", L"True")
            TEST_CLASS_PROPERTY(L"Kits.PublishingOrganization", L"Microsoft Corporation")
            TEST_CLASS_PROPERTY(L"Kits.TestType", L"Development")
            TEST_CLASS_PROPERTY(L"Kits.DevelopmentPhase", L"Development and Integration")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

            BEGIN_TEST_METHOD(TAEF_VerifyAllEndpointsPluggedIn)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
            END_TEST_METHOD()
    };

    class WaveTest
    {
        BEGIN_TEST_CLASS(WaveTest)
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x86")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Server x64")
            TEST_CLASS_PROPERTY(L"Kits.MinRelease", L"19H1")
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"Full")
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"OnecoreUAP")
            TEST_CLASS_PROPERTY(L"Kits.IsInProc", L"True")
            TEST_CLASS_PROPERTY(L"Kits.Parameter", L"DeviceID")
            TEST_CLASS_PROPERTY(L"Kits.Parameter.DeviceID.Description", L"Device id of device under test")
            TEST_CLASS_PROPERTY(L"Kits.PublishingOrganization", L"Microsoft Corporation")
            TEST_CLASS_PROPERTY(L"Kits.TestType", L"Development")
            TEST_CLASS_PROPERTY(L"Kits.DevelopmentPhase", L"Development and Integration")
            TEST_CLASS_PROPERTY(L"Subfeature", L"Wave Driver Tests")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")







        END_TEST_CLASS()

            TEST_CLASS_SETUP(WaveTestSetup)
            TEST_CLASS_CLEANUP(WaveTestCleanUp)

            TEST_METHOD_SETUP(TestCaseSetup)
            TEST_METHOD_CLEANUP(TestCaseCleanUp)
            
            BEGIN_TEST_METHOD(TAEF_StreamStateControl)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*OFFLOAD*'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TAEF_LoopbackStreamStateControl)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*LOOPBACK*'")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(TAEF_StreamMinBufferSize)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*OFFLOAD*'")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(TAEF_StreamMaxBufferSize)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*OFFLOAD*'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TAEF_StreamDifferentFormat)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*OFFLOAD*'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TAEF_LoopbackStreamDifferentFormat)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*LOOPBACK*'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TAEF_StreamMultipleModes)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*RAW' OR @Id = '*DEFAULT'")
            END_TEST_METHOD()
           
            BEGIN_TEST_METHOD(TAEF_VerifyPinWaveRTConformance)
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Type = 'Render' OR @Type = 'Capture'")
            END_TEST_METHOD()
           
    public:
        CHalfApp*   m_pHalf;
    };
}

extern WDMAudio::WaveTest*  g_pWaveTest;
