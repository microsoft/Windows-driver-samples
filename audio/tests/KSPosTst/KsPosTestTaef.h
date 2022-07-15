// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  KsPosTestTaef.h
//
// Abstract:
//
//  Include file for KsPosTestTaef
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
    class KsPosTestPreTest
    {
        BEGIN_TEST_CLASS(KsPosTestPreTest)
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

    class KsPosTest
    {
        BEGIN_TEST_CLASS(KsPosTest)
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"Full")
            TEST_CLASS_PROPERTY(L"Kits.CorePackageComposition", L"OnecoreUAP")
            TEST_CLASS_PROPERTY(L"Kits.DevelopmentPhase", L"Development and Integration")
            TEST_CLASS_PROPERTY(L"Kits.IsInProc", L"True")
            TEST_CLASS_PROPERTY(L"Kits.MinRelease", L"19H1")
            TEST_CLASS_PROPERTY(L"Kits.Parameter", L"DeviceID")
            TEST_CLASS_PROPERTY(L"Kits.Parameter.DeviceID.Description", L"Device id of device under test")
            TEST_CLASS_PROPERTY(L"Kits.Parameter", L"DriverVerifierCustomizeConfiguration")
            TEST_CLASS_PROPERTY(L"Kits.Parameter.DriverVerifierCustomizeConfiguration.Description", L"Specifies that this test may want to automatically update Driver Verifier settings")
            TEST_CLASS_PROPERTY(L"Kits.Parameter.DriverVerifierCustomizeConfiguration.Default", L"True")
            TEST_CLASS_PROPERTY(L"Kits.Parameter.DriverVerifierCustomizeConfiguration.IsUserSettable", L"False")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x86")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM64")
            TEST_CLASS_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Server x64")
            TEST_CLASS_PROPERTY(L"Kits.PublishingOrganization", L"Microsoft Corporation")
            TEST_CLASS_PROPERTY(L"Kits.TestType", L"Development")
            TEST_CLASS_PROPERTY(L"Subfeature", L"Wave Driver Tests")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")







        END_TEST_CLASS()

            TEST_CLASS_SETUP(KsPosTestSetup)
            TEST_CLASS_CLEANUP(KsPosTestCleanUp)

            TEST_METHOD_SETUP(TestCaseSetup)
            TEST_METHOD_CLEANUP(TestCaseCleanUp)

            BEGIN_TEST_METHOD(TAEF_DriftAndJitter)
                TEST_METHOD_PROPERTY(L"HCKCategory", L"Functional")
                TEST_METHOD_PROPERTY(L"HCKCategory", L"Certification")
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*LOOPBACK*'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TAEF_Latency)
                TEST_METHOD_PROPERTY(L"HCKCategory", L"Functional")
                TEST_METHOD_PROPERTY(L"HCKCategory", L"Certification")
                TEST_METHOD_PROPERTY(L"TestClassification:Scope", L"Feature")
                TEST_METHOD_PROPERTY(L"ResourceSelection", L"@Id = '*HOST*' OR @Id = '*LOOPBACK*'")
            END_TEST_METHOD()
                      
    public:
        CHalfApp*           m_pHalf;
        CHalfApp*           m_pHostHalf;        // HalfApp corresponding to the host pin of a loopback pin, if any
        PTIMER_MECHANISM    m_pTimer;
        BOOL                m_fLogHistograms;
        LONGLONG            m_qpcFrequency;     // QPC frequency.
        LONGLONG            m_testBaseHns;      // Base time, in HNS, used inside the tests to make QPC times smaller.
        CRtThreadRegistration m_threadPriority;
    };
}

extern WDMAudio::KsPosTest* g_pKsPosTst;
