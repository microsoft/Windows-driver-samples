#include <stdafx.h>

#define FIVE_MIN_IN_SEC 300

class CAPOStressTest
{
    CAPOStressTest(){};
    ~CAPOStressTest(){};

    BEGIN_TEST_CLASS(CAPOStressTest)
        START_APPVERIFIFER
        TEST_CLASS_PROPERTY(L"Owner", L"auddev")
        TEST_CLASS_PROPERTY(L"TestClassification", L"Feature")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"audiodg.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"audioenginebaseapop.h")
        TEST_CLASS_PROPERTY(L"RunAs", L"System")
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
    BEGIN_TEST_METHOD(AudioAPOStressTest)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"5C35BC8F-FAE1-434B-B842-6B58353696AB")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"1EB5F7F0-CA62-411E-9006-53A23F488332")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Stress the APO - AudioAPOStressTest")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"30")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"60")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: AudioAPOStressTest")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()
};

HRESULT BasicAudioStreaming(IMMDevice *pEndpoint);
HRESULT DisableEnableEndpoint(IMMDevice *pEndpoint);
HRESULT BasicAudioCapture(IMMDevice *pEndpoint);
HRESULT BasicAudioLoopback(IMMDevice *pEndpoint);
HRESULT BasicSpatialAudio(IMMDevice *pEndpoint);
HRESULT BasicOffloadStreaming(IMMDevice *pEndpoint);
void RunStressTest(int timeInSeconds, std::vector<wil::com_ptr<IMMDevice>>& pEndpoints);

bool SetupSkipRTHeap();
bool CleanupSkipRTHeap();