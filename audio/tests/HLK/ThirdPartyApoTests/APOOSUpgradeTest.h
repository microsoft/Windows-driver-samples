#include <stdafx.h>

class CAPOUpgradeTest
{
    CAPOUpgradeTest(){};
    ~CAPOUpgradeTest(){};

    BEGIN_TEST_CLASS(CAPOUpgradeTest)
        START_OSUPGRADE
        START_APPVERIFIFER_UPGRADE
        TEST_CLASS_PROPERTY(L"Owner", L"auddev")
        TEST_CLASS_PROPERTY(L"TestClassification", L"Feature")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"audiodg.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"audioenginebaseapop.h")
        TEST_CLASS_PROPERTY(L"RunAs", L"Elevated")
        TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        TEST_CLASS_PROPERTY(L"Ignore", L"false")
        TEST_CLASS_PROPERTY(L"EtwLogger:WPRProfileFile", L"Audio-Tests.wprp")
        TEST_CLASS_PROPERTY(L"EtwLogger:WPRProfile", L"MultimediaCategory.Verbose.File")
        END_APPVERIFIER
    END_TEST_CLASS()

public:
    TEST_METHOD_SETUP(setUpMethod);
    TEST_METHOD_CLEANUP(tearDownMethod);

protected:
    BEGIN_TEST_METHOD(TestUpgrade)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"B98FD3CB-2E6E-4152-BA85-3021149B8E86")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"C467E942-EB43-477F-8E2A-01D07FEF90C4")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify APOs Work After an OS Upgrade - TestUpgrade")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"60")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"240")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestUpgrade")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()
};