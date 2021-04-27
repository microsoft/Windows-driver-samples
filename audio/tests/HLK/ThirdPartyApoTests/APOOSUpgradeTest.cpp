#include <stdafx.h>

#include <initguid.h>
#include "propkey.h"
#include <AudioClient.h>
#include <SpatialAudioClient.h>
#include <functiondiscoverykeys.h>
#include <DevPKey.h>
#include <AudioEngineBaseAPOP.h>
#include "TestMediaType.h"
#include <audioenginebaseapo.h>
#include <MMDeviceAPI.h>
#include <MMDeviceAPIP.h>

#include <WexTestClass.h>

#include <list>
#include <memory>
#include <ctime>
#include <thread>
#include <future>
#include <mutex>
#include <functional>
#include <atlbase.h>

#include "APOOSUpgradeTest.h"
#include "APOStressTest.h"
#include "sinewave.h"

using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

#define IF_FAILED_RETURN(hr) { HRESULT hrCode = hr; if(FAILED(hrCode)) { return hrCode; } }
#define IF_SUCCEEDED(hr) { HRESULT hrCode = hr; if (!(hrCode == S_OK || hrCode == AUDCLNT_E_WRONG_ENDPOINT_TYPE)) { VERIFY_SUCCEEDED(hrCode); } }

bool SetupSkipRTHeap();
bool CleanupSkipRTHeap();

bool CAPOUpgradeTest::setUpMethod()
{
    return SetupSkipRTHeap();
}

bool CAPOUpgradeTest::tearDownMethod()
{
    return CleanupSkipRTHeap();
}

void CAPOUpgradeTest::TestUpgrade()
{
    UINT cDevices = 0;
    com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    com_ptr_nothrow<IMMDeviceCollection> spDevices;

    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));
    VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &spDevices));
    VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

    for (UINT i = 0; i < cDevices; i++)
    {
        com_ptr_nothrow<IMMDevice> pDevice = nullptr;
        com_ptr_nothrow<IAudioClient2> pAudioClient = nullptr;
        com_ptr_nothrow<IMMEndpoint> pEndpoint = nullptr;
        BOOL bOffloadCapable = FALSE;
        EDataFlow eFlow = eAll;

        VERIFY_SUCCEEDED(spDevices->Item(i, &pDevice));

        VERIFY_SUCCEEDED(com_query_to_nothrow(pDevice, &pEndpoint));
        VERIFY_SUCCEEDED(pEndpoint->GetDataFlow(&eFlow));

        VERIFY_SUCCEEDED(pDevice->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, NULL, (void**)&pAudioClient));
        VERIFY_SUCCEEDED(pAudioClient->IsOffloadCapable(AudioCategory_Media, &bOffloadCapable));

        if (eFlow == eAll || eFlow == eRender)
        {
            VERIFY_SUCCEEDED(BasicAudioStreaming(pDevice.get()));

            VERIFY_SUCCEEDED(BasicSpatialAudio(pDevice.get()));

            VERIFY_SUCCEEDED(BasicAudioLoopback(pDevice.get()));

            if (bOffloadCapable)
            {
                VERIFY_SUCCEEDED(BasicOffloadStreaming(pDevice.get()));
            }
        }
        
        if (eFlow == eAll || eFlow == eCapture)
        {
            VERIFY_SUCCEEDED(BasicAudioCapture(pDevice.get()));
        }
    }
}
