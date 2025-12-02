#include <stdafx.h>
#include <memory>
#include <wil\resultmacros.h>
#include <wil\com.h>
#include <audioclient.h>
#include <audioclientp.h>
#include <audiosessiontypesp.h>
#include <mmdeviceapi.h>
#include "etwlistener.h"
#include <functiondiscoverykeys.h>

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

#include "CAPXHelper.h"

#include <initguid.h>
#include <guiddef.h>
DEFINE_GUID(CLSID_TestCapxAPO, 0xA43110A0,0x4D2B,0x4C5B,0x99,0xB0,0xC6,0xF8,0x62,0x08,0x28,0xB1);

using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

#define POLICY_KERNEL_ONECORE_DEVICE_FAMILY_ID      L"Kernel-OneCore-DeviceFamilyID"

class CAPONotificationFrameworkTest
{
    CAPONotificationFrameworkTest(){};
    ~CAPONotificationFrameworkTest(){};

    BEGIN_TEST_CLASS(CAPONotificationFrameworkTest)
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
    BEGIN_TEST_METHOD(NotificationFrameworkTest)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"30F6BEE7-2AC9-4434-B1AD-642055B2E871")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"C53BE546-1188-4167-B426-E291524BCFD9")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO CAPX - Test the Notification Framework - NotificationFrameworkTest")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"30")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"60")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party CAPX APO Test: NotificationFrameworkTest")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

private:
    DWORD m_dwPlatform = DEVICEFAMILYINFOENUM_UAP;
    std::vector<wil::com_ptr<IMMDevice>> m_capxEndpoints;
};
