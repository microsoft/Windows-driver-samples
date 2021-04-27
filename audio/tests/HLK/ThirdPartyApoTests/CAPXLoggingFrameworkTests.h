#include <stdafx.h>


#include <wil\resultmacros.h>
#include <wil\com.h>
#include <audioclient.h>
#include <audioclientp.h>
#include <audiosessiontypesp.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "etwlistener.h"
#include <functiondiscoverykeys.h>
#include <slpublic.h>

#include <DevPKey.h>
#include <AudioEngineBaseAPOP.h>
#include "TestMediaType.h"
#include <audioenginebaseapo.h>

#include <WexTestClass.h>

#include <list>
#include <memory>

#include "APOStressTest.h"
#include "CAPXHelper.h"

using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

#define POLICY_KERNEL_ONECORE_DEVICE_FAMILY_ID      L"Kernel-OneCore-DeviceFamilyID"

class CAPOLoggingFrameworkTest
{
    CAPOLoggingFrameworkTest(){};
    ~CAPOLoggingFrameworkTest(){};

    BEGIN_TEST_CLASS(CAPOLoggingFrameworkTest)
        START_APPVERIFIFER
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
    BEGIN_TEST_METHOD(LoggingFrameworkTest)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"4FF3BA81-2539-4762-9F7D-40547D1DAD95")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"FEEC362D-673C-40B3-BB01-5063C4B33EA4")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO CAPX - Test the Logging Framework - LoggingFrameworkTest")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"30")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"60")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party CAPX APO Test: LoggingFrameworkTest")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

private:
    DWORD m_dwPlatform = DEVICEFAMILYINFOENUM_UAP;
    std::vector<wil::com_ptr<IMMDevice>> m_capxEndpoints;
};
