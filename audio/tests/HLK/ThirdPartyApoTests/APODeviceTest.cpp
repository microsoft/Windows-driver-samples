#include <stdafx.h>

#include <initguid.h>
#include "propkey.h"
#include <AudioClient.h>
#include <functiondiscoverykeys.h>
#include <DevPKey.h>
#include <AudioEngineBaseAPOP.h>
#include "TestMediaType.h"
#include "audioconnectionutil.h"
#include <audioenginebaseapo.h>

#include <WexTestClass.h>

#include <list>
#include <memory>

#include "APODeviceTest.h"
#include "Wrappers.h"
#include "FormatHelpers.h"


using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

#define REGKEY_AUDIOSERVICE L"Software\\Microsoft\\Windows\\CurrentVersion\\Audio"
#define REGVALUE_SKIPRTHEAP L"SkipRTHeap"

// Forward Declarations
bool SetupSkipRTHeap();
bool CleanupSkipRTHeap();
HRESULT VerifyCustomFormatSupport(CAPODevice * deviceUnderTest);
HRESULT VerifyValidFrameCount(CAPODevice * deviceUnderTest);
HRESULT VerifyAPODataStreaming(CAPODevice * deviceUnderTest);
HRESULT VerifyActivateDeactivate(CAPODevice * deviceUnderTest);
HRESULT FillConnection
(
    UNCOMPRESSEDAUDIOFORMAT* Format,
    UINT_PTR pBuffer,
    UINT32 u32ExtraFrameCount,
    UINT32 u32MaxFrameCount,
    APO_CONNECTION_DESCRIPTOR* pConnection
);

bool CApoDeviceTests::setUpMethod()
{
    // Populate a vector of each endpoint on the system and wrap it with
    // an APO test wrapper which will save necessary APO information for 
    // test cases
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);
    com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    com_ptr_nothrow<IMMDeviceCollection> spDevices;

    UINT32 cDevices;
    PROPVARIANT var;

    PropVariantInit(&var);

    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));
    VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &spDevices));
    VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

    // Initialize an APO device list for each device on the system
    for(UINT32 i = 0; i < cDevices; i++)
    {
        com_ptr_nothrow<IMMDevice> spEndpoint;
        com_ptr_nothrow<IPropertyStore> spPropertyStore;

        VERIFY_SUCCEEDED(spDevices->Item(i, &spEndpoint));
        VERIFY_SUCCEEDED(spEndpoint->OpenPropertyStore(STGM_READ, &spPropertyStore));
        VERIFY_SUCCEEDED(spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var));
        VERIFY_ARE_EQUAL(VT_LPWSTR, var.vt);
        LOG_OUTPUT(L"Setting Up Endpoint: %s", var.pwszVal);

        unique_ptr<CAPODeviceList> pTestModule(new CAPODeviceList());
        pTestModule->Initialize(wistd::move(spEndpoint), var.pwszVal);
        m_testDeviceWrapperList.push_back(move(pTestModule));

        PropVariantClear(&var);
    }

    return SetupSkipRTHeap();
}

bool CApoDeviceTests::tearDownMethod()
{
    m_testDeviceWrapperList.clear();
    return CleanupSkipRTHeap();
}

void CApoDeviceTests::TestAPOInitialize()
{
    // In the event that one initialization fails fully, we do not want to fail
    // the whole test. Instead, mark the test as failed and complete it on all
    // devices.
    bool testSucceeded = true;

    m_testDeviceWrapperList.remove_if([&](auto & deviceWrapper) {

        // Print Device Friendly name
        LOG_OUTPUT(L"Endpoint Under Test: %ls", deviceWrapper->m_deviceName);

        HRESULT hr = deviceWrapper->AddSysFxDevices();

        if (hr == S_FALSE)
        {
            // No cause for failure here. Just remove the device from our test list
            // as it appears to be inactive or has no third party APOs
            return true;
        }
        
        if (FAILED(hr))
        {
            LOG_ERROR(L"Device initialization failed with error: %x", hr);
            testSucceeded = false;
        }
        return false;

    });

    VERIFY_IS_TRUE(testSucceeded);
}

void CApoDeviceTests::TestCustomFormatSupport()
{
    bool testSucceeded = true;

    TestAPOInitialize();

    for(auto const& deviceWrapper : m_testDeviceWrapperList)
    {
        // Print Device Friendly name
        LOG_OUTPUT(L"Endpoint Under Test: %ls", deviceWrapper->m_deviceName);

        // Test over each SysFX device on the device
        for (auto const& apo : deviceWrapper->m_DeviceList)
        {
            if (apo->IsProxyApo())
            {
                GUID proxyGuid;
                unique_cotaskmem_string proxyString;

                apo->GetClsID(&proxyGuid);
                StringFromCLSID(proxyGuid, &proxyString);

                LOG_OUTPUT(L"Proxy APO found. Skipping: %ls", proxyString.get());
                continue;
            }

            HRESULT hr = VerifyCustomFormatSupport(apo.get());

            if (hr == S_FALSE)
            {
                LOG_OUTPUT(L"\tAPO does not support custom formats. This is not an error");
            }

            if (FAILED(hr))
            {
                LOG_ERROR(L"\tTest Custom Format Support failed with error: (0x%08lx)", hr);
                testSucceeded = false;
            }
        }
    }

    VERIFY_IS_TRUE(testSucceeded);
}

void CApoDeviceTests::TestValidFrameCount()
{
    bool testSucceeded = true;

    TestAPOInitialize();

    for(auto const& deviceWrapper : m_testDeviceWrapperList)
    {
        // Print Device Friendly name
        LOG_OUTPUT(L"Endpoint Under Test: %ls", deviceWrapper->m_deviceName);

        // Test over each SysFX device on the device
        for (auto const& apo : deviceWrapper->m_DeviceList)
        {
            if (apo->IsProxyApo())
            {
                GUID proxyGuid;
                unique_cotaskmem_string proxyString;

                apo->GetClsID(&proxyGuid);
                StringFromCLSID(proxyGuid, &proxyString);

                LOG_OUTPUT(L"\tProxy APO found. Skipping: %ls", proxyString.get());
                continue;
            }

            HRESULT hr = VerifyValidFrameCount(apo.get());

            if (FAILED(hr))
            {
                LOG_ERROR(L"\tTest Valid Frame Count failed with error: (0x%08lx)", hr);
                testSucceeded = false;
            }
        }
    }

    VERIFY_IS_TRUE(testSucceeded);
}

void CApoDeviceTests::TestAPODataStreaming()
{
    bool testSucceeded = true;

    TestAPOInitialize();

    for(auto const& deviceWrapper : m_testDeviceWrapperList)
    {
        // Print Device Friendly name
        LOG_OUTPUT(L"Endpoint Under Test: %ls", deviceWrapper->m_deviceName);

        // Test over each SysFX device on the device
        for (auto const& apo : deviceWrapper->m_DeviceList)
        {
            if (apo->IsProxyApo())
            {
                GUID proxyGuid;
                unique_cotaskmem_string proxyString;

                apo->GetClsID(&proxyGuid);
                StringFromCLSID(proxyGuid, &proxyString);

                LOG_OUTPUT(L"\tProxy APO found. Skipping: %ls", proxyString.get());
                continue;
            }

            HRESULT hr = VerifyAPODataStreaming(apo.get());

            if (FAILED(hr))
            {
                LOG_ERROR(L"\tTest APO Data Streaming failed with error: (0x%08lx)", hr);
                testSucceeded = false;
            }
        }
    }

    VERIFY_IS_TRUE(testSucceeded);
}

void CApoDeviceTests::TestActivateDeactivate()
{
    bool testSucceeded = true;

    TestAPOInitialize();

    for(auto const& deviceWrapper : m_testDeviceWrapperList)
    {
        // Print Device Friendly name
        LOG_OUTPUT(L"Endpoint Under Test: %ls", deviceWrapper->m_deviceName);

        // Test over each SysFX device on the device
        for (auto const& apo : deviceWrapper->m_DeviceList)
        {
            if (apo->IsProxyApo())
            {
                GUID proxyGuid;
                unique_cotaskmem_string proxyString;

                apo->GetClsID(&proxyGuid);
                StringFromCLSID(proxyGuid, &proxyString);

                LOG_OUTPUT(L"\tProxy APO found. Skipping: %ls", proxyString.get());
                continue;
            }

            HRESULT hr = VerifyActivateDeactivate(apo.get());

            if (FAILED(hr))
            {
                LOG_ERROR(L"\tTest Activate and Deactivate failed with error: (0x%08lx)", hr);
                testSucceeded = false;
            }
        }
    }

    VERIFY_IS_TRUE(testSucceeded);
}

HRESULT VerifyValidFrameCount(CAPODevice*  deviceUnderTest)
{
    wil::com_ptr<IAudioProcessingObject>                 pIAPO;
    wil::com_ptr<IAudioProcessingObjectRT>               pIAPORT;
    wil::com_ptr<IAudioProcessingObjectConfiguration>    pIAPOConfig;

    if (deviceUnderTest == nullptr)
    {
        LOG_ERROR(L"\tCould not instantiate SysFx object.\n"); 
        return E_NOINTERFACE;
    }

    LOG_RETURN_IF_FAILED(deviceUnderTest->GetAPOInterfaces(&pIAPO, &pIAPORT, &pIAPOConfig), L"\tCould not instantiate IAudioProcessingObject interface.");

    APO_CONNECTION_PROPERTY inputProperty, outputProperty;
    APO_CONNECTION_DESCRIPTOR inputDescriptor, outputDescriptor;
    APO_CONNECTION_PROPERTY *pInputConnProp = &inputProperty, *pOutputConnProp = &outputProperty;
    APO_CONNECTION_DESCRIPTOR *pInputConnDesc = &inputDescriptor, *pOutputConnDesc = &outputDescriptor;
    UNCOMPRESSEDAUDIOFORMAT defaultFormat {};

    FillFormat(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 2, sizeof(float), sizeof(float) * 8, static_cast<float>(cFrameRate), KSAUDIO_SPEAKER_STEREO, &defaultFormat);    

    LOG_RETURN_IF_FAILED(SetupConnection(pInputConnDesc, pOutputConnDesc, pInputConnProp, pOutputConnProp, &defaultFormat),
                        L"\tSetupConnection failed");

    LOG_RETURN_IF_FAILED(LockForProcess(pIAPOConfig.get(), 1, &pInputConnDesc, 1, &pOutputConnDesc),
                        L"\tLockForProcess failed");

    // setup vectors
    FLOAT32     *pf32Input, *pf32Output;
    pf32Input = (FLOAT32*)pInputConnProp->pBuffer;
    pf32Output = (FLOAT32*)pOutputConnProp->pBuffer;
    for (UINT32 index = 0; index < cFrameCountToProcess; index++)
    {
        pf32Input[index] = 0.5f;
        pf32Output[index] = 0.0f;
    }

    APOProcess(pIAPORT.get(), 1, &pInputConnProp, 1, &pOutputConnProp);

    LOG_RETURN_HR_IF(pOutputConnProp->u32ValidFrameCount != cFrameCountToProcess,
        E_FAIL,
        L"\tFAIL: APOProcess output(%d) and input(%d) buffer valid frame counts does not match.\n", pOutputConnProp->u32ValidFrameCount, cFrameCountToProcess);

    LOG_RETURN_IF_FAILED(UnlockForProcess(pIAPOConfig.get()),
                        L"\tUnlock for process failed");

    LOG_RETURN_IF_FAILED(DestroyConnection(pInputConnDesc), L"\tFailed to DestroyConnection");
    LOG_RETURN_IF_FAILED(DestroyConnection(pOutputConnDesc), L"\tFailed to DestroyConnection");

    return S_OK;
}

HRESULT VerifyCustomFormatSupport(CAPODevice * deviceUnderTest)
{
    com_ptr_nothrow<IAudioProcessingObject>                 pIAPO = nullptr;
    com_ptr_nothrow<IAudioSystemEffectsCustomFormats>       pICustomFormats = nullptr;
    com_ptr_nothrow<IEndpointUtility>                       pIEPUtility = nullptr;
    IMMDevice *                                             pIMMDevice = nullptr;
    UINT                                                    iFormat;

    if (deviceUnderTest == nullptr)
    {
        LOG_ERROR(L"\tCould not instantiate SysFx object.");
        return E_NOINTERFACE;
    }
    
    if (nullptr == (pIMMDevice = deviceUnderTest->GetEndpoint()))
    {
        LOG_ERROR(L"\tUnable to get endpoint for device.");
        return E_NOINTERFACE;
    }
    
    LOG_RETURN_IF_FAILED(pIMMDevice->Activate(__uuidof(IEndpointUtility), CLSCTX_INPROC_SERVER, NULL, (void**)&pIEPUtility),
                        L"\tUnable to activate utility interface");
    
    LOG_RETURN_IF_FAILED(deviceUnderTest->GetAPOInterfaces(&pIAPO, nullptr, nullptr), L"\tCould not instantiate IAudioProcessingObject interface.");
    
    if (pIAPO == nullptr)
    {
        LOG_ERROR(L"\tFAIL: Could not instantiate IAudioProcessingObject interface.");
        return E_NOINTERFACE;
    }

    HRESULT hrQi = QIInternal(pIAPO.get(), __uuidof(IAudioSystemEffectsCustomFormats), (void**)&pICustomFormats);

    if (E_NOINTERFACE == hrQi)
    {
        //  This GFX does not support custom formats...
        LOG_OUTPUT(L"\tSysFx does not support custom formats (QueryInterface for "
                    "IAudioSystemEffectsCustomFormats returns E_NOINTERFACE).");
        return S_FALSE;
    }

    LOG_RETURN_IF_FAILED(hrQi, L"\tQueryInterface for IAudioSystemEffectsCustomFormats returned error.");


    //  Custom formats are available and supported
    LOG_OUTPUT(L"\tIAudioSystemEffectsCustomFormats available on SysFx.");

    LOG_RETURN_IF_FAILED(pICustomFormats->GetFormatCount(&iFormat),
                        L"\tIAudioSystemEffectsCustomFormats::GetFormatCount failed");
    
    // No matter what we want to test all formats. However if one fails, make sure we fail
    // the test as well
    bool allFormatsSucceeded = true;

    for (; iFormat; --iFormat)
    {
        com_ptr_nothrow<IAudioMediaType>        pIFormat = nullptr;
        com_ptr_nothrow<IAudioMediaType>        pIFormatSuggested = nullptr;
        LPWSTR                                  strFormatName = NULL;        
        com_ptr_nothrow<IPartsList>             pIParts = nullptr;
        KSDATAFORMAT_WAVEFORMATEX *             pKsFormat = NULL;
        HRESULT                                 hrFormatSupport = S_OK;
        
        if (FAILED(hrFormatSupport = pICustomFormats->GetFormat(iFormat - 1, &pIFormat)))
        {
            LOG_ERROR(L"\tFAIL: IAudioSystemEffectsCustomFormats::GetFormat returned "
                    "error (0x%08lx).",
                hrFormatSupport);
            allFormatsSucceeded = false;

            continue;
        }
        
        if (FAILED(hrFormatSupport = pICustomFormats->GetFormatRepresentation(iFormat - 1, &strFormatName)))
        {
            LOG_ERROR(L"\tFAIL: IAudioSystemEffectsCustomFormats::"
                    "GetFormatRepresentation returned error (0x%08lx)",
                hrFormatSupport);
            allFormatsSucceeded = false;

            continue;
        }
        
        LOG_OUTPUT(L"\tVerifying format support for: [%ls]...", strFormatName);
        
        //  Custom format, must be supported on the output side of SysFx...
        hrFormatSupport = IsOutputFormatSupported(pIAPO.get(), NULL, pIFormat.get(), &pIFormatSuggested);
        const WAVEFORMATEX* original = pIFormat->GetAudioFormat();
        const WAVEFORMATEX* suggested = pIFormatSuggested->GetAudioFormat();

        LOG_OUTPUT(L"\tWaveFormatEx pIFormat - tag: %u, channels: %u, samplespersec: %u, avgbytespersec: %u, blockalign: %u, bitspersample: %u, cbsize: %u", 
                    original->wFormatTag, original->nChannels, original->nSamplesPerSec, original->nAvgBytesPerSec, original->nBlockAlign, 
                    original->wBitsPerSample, original->cbSize);

        LOG_OUTPUT(L"\tWaveFormatEx pIFormatSuggested - tag: %u, channels: %u, samplespersec: %u, avgbytespersec: %u, blockalign: %u, bitspersample: %u, cbsize: %u", 
                    suggested->wFormatTag, suggested->nChannels, suggested->nSamplesPerSec, suggested->nAvgBytesPerSec, suggested->nBlockAlign, 
                    suggested->wBitsPerSample, suggested->cbSize);
        
        if (S_FALSE == hrFormatSupport)
        {
            LOG_ERROR(L"\tFAIL: IsOutputFormatSupported returns S_FALSE with a suggested format");
            allFormatsSucceeded = false;            
        }
        else if (S_OK != hrFormatSupport)
        {
            LOG_ERROR(L"\tFAIL: IsOutputFormatSupported returns error (0x%08lx)",
                hrFormatSupport);
            allFormatsSucceeded = false;
        }
        else
        {
            if (pIFormatSuggested == nullptr)
            {
                LOG_ERROR(L"\tFAIL: IsOutputFormatSupported returns S_OK, but does not "
                        "suggest format.");
                allFormatsSucceeded = false;
            }
            else
            {
                if (!IsEqualFormat(pIFormat.get(), pIFormatSuggested.get()))
                {
                    LOG_ERROR(L"\tFAIL: IsOutputFormatSupported returns S_OK, but "
                            "returns a different format.");
                    allFormatsSucceeded = false;
                }
                else
                {
                    LOG_OUTPUT(L"\tIsOutputFormatSupported supports custom format.");
                }
            }
        }
        
        pKsFormat = CreateKSDataFromWFX((WAVEFORMATEX*)pIFormat->GetAudioFormat());
        
        if (pKsFormat == NULL)
        {
            LOG_ERROR(L"\tFAIL: Unable to create KSDATAFORMAT from PWAVEFORMATEX.");
            allFormatsSucceeded = false;

            continue;
        }
        
        {
            unique_cotaskmem_ptr<KSDATAFORMAT_WAVEFORMATEX> formatBuffer(pKsFormat);
            
            //  Custom format, must be supported by endpoint...
            hrFormatSupport = pIEPUtility->FindHostConnector(
                    (KSDATAFORMAT*)formatBuffer.get(),
                    formatBuffer->DataFormat.FormatSize,
                    FALSE,
                    &pIParts);
            
            if (S_OK != hrFormatSupport)
            {
                LOG_ERROR(L"\tFAIL: Endpoint does not support custom format with error: (0x%08lx)", hrFormatSupport);
                allFormatsSucceeded = false;
            }
            else
            {
                LOG_OUTPUT(L"\tEndpoint supports custom format.");
            }
        }
    }
    
    return allFormatsSucceeded;
}

HRESULT VerifyAPODataStreaming(CAPODevice* deviceUnderTest)
{
    wil::com_ptr<IAudioProcessingObject>                 pIAPO;
    wil::com_ptr<IAudioProcessingObjectRT>               pIAPORT;
    wil::com_ptr<IAudioProcessingObjectConfiguration>    pIAPOConfig;
    APO_REG_PROPERTIES *pRegProperties = NULL;    

    if (deviceUnderTest == nullptr)
    {
        LOG_ERROR(L"\tCould not instantiate SysFx object.\n"); 
        return E_NOINTERFACE;
    }

    LOG_RETURN_IF_FAILED(deviceUnderTest->GetAPOInterfaces(&pIAPO, &pIAPORT, &pIAPOConfig), L"\tCould not instantiate IAudioProcessingObject interface.");

    APO_CONNECTION_PROPERTY inputProperty, outputProperty;
    APO_CONNECTION_DESCRIPTOR inputDescriptor, outputDescriptor;
    APO_CONNECTION_PROPERTY *pInputConnProp = &inputProperty, *pOutputConnProp = &outputProperty;
    APO_CONNECTION_DESCRIPTOR *pInputConnDesc = &inputDescriptor, *pOutputConnDesc = &outputDescriptor;
    UNCOMPRESSEDAUDIOFORMAT defaultFormat {};

    FillFormat(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 2, sizeof(float), sizeof(float) * 8, static_cast<float>(cFrameRate), KSAUDIO_SPEAKER_STEREO, &defaultFormat);    

    LOG_RETURN_IF_FAILED(SetupConnection(pInputConnDesc, pOutputConnDesc, pInputConnProp, pOutputConnProp, &defaultFormat),
                        L"\tSetupConnection failed");

    LOG_RETURN_IF_FAILED(LockForProcess(pIAPOConfig.get(), 1, &pInputConnDesc, 1, &pOutputConnDesc),
                        L"\tLockForProcess failed");

    // setup vectors
    FLOAT32     *pf32Input, *pf32Output;
    pf32Input = (FLOAT32*)pInputConnProp->pBuffer;
    pf32Output = (FLOAT32*)pOutputConnProp->pBuffer;
    for (UINT32 index = 0; index < cFrameCountToProcess; index++)
    {
        pf32Input[index] = 0.5f;
        pf32Output[index] = INFINITY;
    }

    APOProcess(pIAPORT.get(), 1, &pInputConnProp, 1, &pOutputConnProp);

    GetRegistrationProperties(pIAPO.get(), &pRegProperties);

    // Tests to make sure that in place APO have the correct data.
    for (UINT32 index = 0; index < cFrameCountToProcess; index++)
    {
        // This will also check NaN along with INF
        LOG_RETURN_HR_IF(!isfinite(pf32Output[index]), E_FAIL,
            L"\tFAIL: APOProcess output(%f) is not a number or infinite.\n", pf32Output[index]);
    }

    LOG_RETURN_IF_FAILED(UnlockForProcess(pIAPOConfig.get()),
                        L"\tUnlock for process failed");

    if (APO_FLAG_INPLACE == (pRegProperties->Flags & APO_FLAG_INPLACE))
    {
        LOG_RETURN_IF_FAILED(LockForProcess(pIAPOConfig.get(), 1, &pInputConnDesc, 1, &pInputConnDesc),
                             L"\tLockForProcess failed");

        // setup vector
        pf32Input = (FLOAT32 *)pInputConnProp->pBuffer;
        for (UINT32 index = 0; index < cFrameCountToProcess; index++)
        {
            pf32Input[index] = 0.5f;
        }

        APOProcess(pIAPORT.get(), 1, &pInputConnProp, 1, &pInputConnProp);

        LOG_RETURN_IF_FAILED(UnlockForProcess(pIAPOConfig.get()),
                        L"\tUnlock for process failed");
    }

    LOG_RETURN_IF_FAILED(DestroyConnection(pInputConnDesc), L"\tFailed to DestroyConnection");
    LOG_RETURN_IF_FAILED(DestroyConnection(pOutputConnDesc), L"\tFailed to DestroyConnection");

    return S_OK;
}

HRESULT VerifyActivateDeactivate(CAPODevice* deviceUnderTest)
{
    GUID clsid;
    WCHAR ComRegistrationPath[1024];
    WCHAR file[1024];
    PWSTR end;
    size_t cchRemaining;
    DWORD dwSize = sizeof(WCHAR) * 1024;

    if (deviceUnderTest == nullptr)
    {
        LOG_ERROR(L"\tCould not instantiate SysFx object.\n"); 
        return E_NOINTERFACE;
    }

    deviceUnderTest->GetClsID(&clsid);

    LOG_RETURN_IF_FAILED(StringCchPrintfEx(ComRegistrationPath, ARRAYSIZE(ComRegistrationPath), &end, &cchRemaining, 0, L"CLSID\\{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\\InprocServer32",
        clsid.Data1, clsid.Data2, clsid.Data3, clsid.Data4[0], clsid.Data4[1], clsid.Data4[2], clsid.Data4[3], clsid.Data4[4], clsid.Data4[5], clsid.Data4[6], clsid.Data4[7]), 
        L"\tFailed to create string to find file name for DLL.");

    LONG error = RegGetValue(HKEY_CLASSES_ROOT, ComRegistrationPath, nullptr, RRF_RT_REG_SZ, nullptr, file, &dwSize);

    if (error != ERROR_SUCCESS)
    {
        LOG_RETURN_IF_FAILED(HRESULT_FROM_WIN32(error), L"\tFAILED to get the registry value.");
    }

    auto loadAPO = LoadLibrary(file);

    if (loadAPO == NULL)
    {
        DWORD err = GetLastError();
        HRESULT hr = HRESULT_FROM_WIN32(err);
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        LOG_RETURN_IF_FAILED(hr, L"\tFAILED to load the APO.");
    }

    BOOL bFreed = FreeLibrary(loadAPO);

    if (!bFreed)
    {
        DWORD err = GetLastError();
        HRESULT hr = HRESULT_FROM_WIN32(err);
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        LOG_RETURN_IF_FAILED(hr, L"\tFAILED to free the loaded APO.");
    }

    return S_OK;
}

struct APO_PROPERTIES { 
    PROPERTYKEY key; 
    LPARAM apoType;
    LPCWSTR apoTypeName;
    DWORD dwMajorVersion; 
    DWORD dwMinorVersion;
    } PKEY_FX_EFFECTS[] = {
        {PKEY_FX_PostMixEffectClsid,  DT_GFX, L"GFX " ,6, 0},
        {PKEY_FX_PreMixEffectClsid,   DT_LFX, L"LFX ", 6, 0},
        {PKEY_FX_StreamEffectClsid,   DT_SFX, L"SFX ", 6, 3},
        {PKEY_FX_ModeEffectClsid,     DT_MFX, L"MFX ", 6, 3},
        {PKEY_FX_EndpointEffectClsid, DT_EFX, L"EFX ", 6, 3},
        // Composite effects
        {PKEY_CompositeFX_StreamEffectClsid,    DT_SFX, L"Composite SFX", 10, 0},
        {PKEY_CompositeFX_ModeEffectClsid,      DT_MFX, L"Composite MFX", 10, 0},
        {PKEY_CompositeFX_EndpointEffectClsid,  DT_EFX, L"Composite EFX", 10, 0},
        // Offload effects
        {PKEY_FX_Offload_StreamEffectClsid,    DT_SFX, L"Offload SFX", 10, 0},
        {PKEY_FX_Offload_ModeEffectClsid,      DT_MFX, L"Offload MFX", 10, 0},
        {PKEY_CompositeFX_Offload_StreamEffectClsid,    DT_SFX, L"Composite Offload SFX", 10, 0},
        {PKEY_CompositeFX_Offload_ModeEffectClsid,      DT_MFX, L"Composite Offload MFX", 10, 0},
        // Keyword Detector effects
        {PKEY_FX_KeywordDetector_StreamEffectClsid,    DT_SFX, L"KeywordDetector SFX", 10, 0},
        {PKEY_FX_KeywordDetector_ModeEffectClsid,      DT_MFX, L"KeywordDetector MFX", 10, 0},
        {PKEY_FX_KeywordDetector_EndpointEffectClsid,  DT_EFX, L"KeywordDetector EFX", 10, 0},
        {PKEY_CompositeFX_KeywordDetector_StreamEffectClsid,    DT_SFX, L"Composite KeywordDetector SFX", 10, 0},
        {PKEY_CompositeFX_KeywordDetector_ModeEffectClsid,      DT_MFX, L"Composite KeywordDetector MFX", 10, 0},
        {PKEY_CompositeFX_KeywordDetector_EndpointEffectClsid,  DT_EFX, L"Composite KeywordDetector EFX", 10, 0},
    };

CAPODevice::CAPODevice
(
    IMMDevice          *pIEndpoint,
    LPCWSTR             pszClassId,
    LPCWSTR             pszEndpoint,
    IPropertyStore     *pIStoreDevice,
    IPropertyStore     *pIStoreFx,
    LPARAM              apoType,
    LPCWSTR             apoTypeName,
    LPCWSTR             pszAttachedDevice,
    BOOL                bProxyAPO
)
: m_pRegProps(NULL), m_fValid(FALSE)
{
    {
        //  Getting PnpId from property store...
        PROPVARIANT                     pv;
        com_ptr_nothrow<IMMDeviceEnumerator>    pIDeviceEnumerator;
        com_ptr_nothrow<IMMDevice>              pIMMDevNode;
        com_ptr_nothrow<IPropertyStore>         pIDNPropStore;
    
        LOG_RETURN_VOID_IF_FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pIDeviceEnumerator)), 
                                  L"\tCoCreateInstance for MMDeviceEnumerator returned error");
           
        PropVariantInit(&pv);
        auto clearPropVariantOnScopeExit = scope_exit([&]
        {
            PropVariantClear(&pv);
        });
    
        LOG_RETURN_VOID_IF_FAILED(pIStoreDevice->GetValue(PKEY_Endpoint_Devnode, &pv), L"\tFailed to get endpoint device");
    
        if (VT_LPWSTR != pv.vt)
        {
            LOG_ERROR(L"\tEnpoint Devnode property key is of the incorrect type");
            return;
        }
        
        LOG_RETURN_VOID_IF_FAILED(pIDeviceEnumerator->GetDevice(pv.pwszVal, &pIMMDevNode), L"\tGetting device (%s) from the enumerator failed", pv.pwszVal);
        
        PropVariantClear(&pv);
        
        LOG_RETURN_VOID_IF_FAILED(pIMMDevNode->OpenPropertyStore(STGM_READ, &pIDNPropStore), L"\tFailed to open device property store");
        
        PropVariantInit(&pv);
        
        LOG_RETURN_VOID_IF_FAILED(pIDNPropStore->GetValue((PROPERTYKEY&)DEVPKEY_Device_InstanceId, &pv), L"\tFailed to get device instance id from device prop store");
        
        if (VT_LPWSTR != pv.vt)
        {
            LOG_ERROR(L"\tDevice instance ID is of the incorrect type");
            return;
        }
        
        m_szPnPId = wil::make_cotaskmem_string_nothrow(pv.pwszVal, sizeof(m_szPnPId));
    }
    
    LOG_RETURN_VOID_IF_FAILED(CLSIDFromString((LPOLESTR)pszClassId, &m_gClsID), L"\tCLSIDFromString failed");
    
    m_szAttachedDevice = wil::make_cotaskmem_string_nothrow(pszAttachedDevice, wcslen(pszAttachedDevice));
    m_szEndpoint = wil::make_cotaskmem_string_nothrow(pszEndpoint, wcslen(pszEndpoint));

    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(pIStoreFx->GetValue(PKEY_ItemNameDisplay, &value)))
    {
        if (value.vt == VT_LPWSTR)
        {
            m_sApoName = wil::make_cotaskmem_string_nothrow(value.pwszVal, wcslen(value.pwszVal));
        }
        PropVariantClear(&value);
    }
    
        
    LOG_RETURN_VOID_IF_FAILED(CoCreateInstance(m_gClsID, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_pIUnknown)), L"\tCoCreateInstance for IUnknown returned error");


    LOG_RETURN_VOID_IF_FAILED(m_pIUnknown->QueryInterface(__uuidof(IAudioProcessingObject), (void**)&m_pIAPO), 
                                L"\tCoCreateInstance for IAudioProcessingObject returned error");


    //  Getting DeviceCollection
    {
        PROPVARIANT                 pvFormat;
        
        // the standard KSDATAFORMAT_WAVEFORMATEX only contains a 
        // WAVEFORMATEX, but we need the extensible format so
        // we define this type here:
        typedef struct {
            KSDATAFORMAT            DataFormat;
            WAVEFORMATEXTENSIBLE    WFXtensible;
        } KS_WFExtensible;
        
        KS_WFExtensible             ksWfx;
        
        // initialize the structure
        PropVariantInit(&pvFormat);

        // Clear the prop variant on scope exit
        auto clearFormatOnExit = scope_exit([&] {
            PropVariantClear(&pvFormat);
        });
        
        LOG_RETURN_VOID_IF_FAILED(pIStoreDevice->GetValue(PKEY_AudioEngine_DeviceFormat, &pvFormat), L"\tGet PKEY_AudioEngine_Device format returned error");
        
        if (pvFormat.vt == VT_BLOB)
        {
            if (pvFormat.blob.cbSize != sizeof(WAVEFORMATEXTENSIBLE))
            {
                LOG_ERROR(L"\tpvFormat.blob.cbSize (%zu) != sizeof(WAVEFORMATEXTENSIBLE) (%zu)\n", pvFormat.blob.cbSize, sizeof(WAVEFORMATEXTENSIBLE));
                return;
            }
        }
        else
        {
            LOG_ERROR(L"\tFormat is not VT_BLOB, Q != %c\n", VARTYPE(pvFormat.vt));
            return;
        }


        LPWAVEFORMATEX              pbWfxDevice = NULL;
        ULONG                       cbWfxDevice = 0;
        
        pbWfxDevice = (WAVEFORMATEX*)pvFormat.blob.pBlobData;
        cbWfxDevice = pvFormat.blob.cbSize;

        ksWfx.DataFormat.FormatSize  = sizeof(KSDATAFORMAT) + cbWfxDevice;
        ksWfx.DataFormat.Flags       = 0;
        ksWfx.DataFormat.SampleSize  = 0;
        ksWfx.DataFormat.Reserved    = 0;
        ksWfx.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        ksWfx.DataFormat.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
        ksWfx.DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
        CopyMemory( &(ksWfx.WFXtensible), pbWfxDevice, cbWfxDevice );
        
        com_ptr_nothrow<IEndpointUtility2>  pIEPUtil;
        LOG_RETURN_VOID_IF_FAILED(pIEndpoint->Activate(__uuidof(IEndpointUtility2), CLSCTX_ALL, NULL, (void**)&pIEPUtil), L"\tpIMMDevice->Activate returned error");
        
        LOG_RETURN_VOID_IF_FAILED(pIEPUtil->GetHostConnectorDeviceCollection2((PKSDATAFORMAT)&ksWfx, sizeof(ksWfx), TRUE, 
                                  eHostProcessConnector, &m_pIDevCollection, &m_nSoftwareIoDeviceInCollection, &m_nSoftwareIoConnectorIndex),
                                  L"\tGetting device collection returned error");
    }
    
    //  Saved off for InitializeAPO()
    m_pIEndpoint = pIEndpoint;
    m_pIEPStore  = pIStoreDevice;
    m_pIFXStore  = pIStoreFx;
    m_lpType     = apoType;
    m_sApoTypeName =  wil::make_cotaskmem_string_nothrow(apoTypeName, wcslen(apoTypeName));;
    m_bProxyAPO  = bProxyAPO;
    
    //  Initialize SysFx!!
    LOG_RETURN_VOID_IF_FAILED(InitializeAPO(m_pIAPO.get()), L"\tpIAPO->Initialize returned error");

    m_fValid = TRUE;
}

CAPODevice::~CAPODevice(void)
{
    if (NULL != m_pRegProps)
    {
        CoTaskMemFree(m_pRegProps);
        m_pRegProps = NULL;
    }
}


HRESULT CAPODevice::InitializeAPO
(
    IAudioProcessingObject     *pIAPO
)
{
    LOG_RETURN_HR_IF(pIAPO == nullptr, E_POINTER, L"IAudioProcessingObject is null");

    if(DT_SMEFX & GetDeviceType())
    {
        APOInitSystemEffects2                        sysfxInitParams2 = {0};
        // Setup initialization struct
        sysfxInitParams2.APOInit.cbSize                = sizeof(APOInitSystemEffects2);
        sysfxInitParams2.APOInit.clsid                 = m_gClsID;
        sysfxInitParams2.pReserved                     = NULL;
        sysfxInitParams2.pDeviceCollection             = m_pIDevCollection.get();
        sysfxInitParams2.pAPOEndpointProperties        = m_pIEPStore.get();
        sysfxInitParams2.pAPOSystemEffectsProperties   = m_pIFXStore.get();
        sysfxInitParams2.InitializeForDiscoveryOnly    = TRUE;
        sysfxInitParams2.AudioProcessingMode           = AUDIO_SIGNALPROCESSINGMODE_DEFAULT;
        sysfxInitParams2.nSoftwareIoDeviceInCollection = m_nSoftwareIoDeviceInCollection;
        sysfxInitParams2.nSoftwareIoConnectorIndex     = m_nSoftwareIoConnectorIndex;
        
        return pIAPO->Initialize(sysfxInitParams2.APOInit.cbSize, (BYTE*)(&sysfxInitParams2));
    }
    else if (DT_LGFX & GetDeviceType())
    {
        APOInitSystemEffects                        sysfxInitParams = {0};
        // Setup initialization struct
        sysfxInitParams.APOInit.cbSize              = sizeof(sysfxInitParams);
        sysfxInitParams.APOInit.clsid               = m_gClsID;
        sysfxInitParams.pAPOEndpointProperties      = m_pIEPStore.get();
        sysfxInitParams.pAPOSystemEffectsProperties = m_pIFXStore.get();
        sysfxInitParams.pDeviceCollection           = m_pIDevCollection.get();
        sysfxInitParams.pReserved                   = NULL;
        
        return (pIAPO->Initialize(sizeof(sysfxInitParams), (BYTE*)&sysfxInitParams));
    }

    //  We should never get here unless we are initializing a first party APO (which we shouldn't be)
    LOG_ERROR(L"SFX is of unknown type. Type is: %d", GetDeviceType());
    return E_FAIL;
}

HRESULT CAPODevice::GetAPOInterfaces
(
    IAudioProcessingObject                **ppIAPO,
    IAudioProcessingObjectRT              **ppIAPORT,
    IAudioProcessingObjectConfiguration   **ppIAPOConfig
)
{
    if (ppIAPO != nullptr)
    {
        LOG_RETURN_IF_FAILED(UnknownQIInternal(m_pIUnknown.get(), __uuidof(IAudioProcessingObject), (void**)ppIAPO),
                            L"\tCoCreateInstance for IAudioProcessingObject failed");
    }

    if (ppIAPORT != nullptr)
    {
        LOG_RETURN_IF_FAILED(UnknownQIInternal(m_pIUnknown.get(), __uuidof(IAudioProcessingObjectRT), (void**)ppIAPORT),
                            L"\tCoCreateInstance for IAudioProcessingObjectRT failed");
    }
    
    if (ppIAPOConfig != nullptr)
    {
        LOG_RETURN_IF_FAILED(UnknownQIInternal(m_pIUnknown.get(), __uuidof(IAudioProcessingObjectConfiguration), (void**)ppIAPOConfig),
                            L"\tCoCreateInstance for IAudioProcessingObjectConfiguration failed");
    }
    
    return S_OK;
}

//--------------------------------------------------------------------------
//  CAPODeviceList constructor
CAPODeviceList::CAPODeviceList ()
{
}

//--------------------------------------------------------------------------
//  ~CAPODeviceList destructor

CAPODeviceList::~CAPODeviceList(VOID)
{
    m_DeviceList.clear();

    RegDeleteKeyValue(HKEY_LOCAL_MACHINE,
    REGKEY_AUDIO_SOFTWARE,
    PREVENT_APOTEST_CRASH_OR_REPORT_ON_APO_EXCEPTION);
}

HRESULT CAPODeviceList::AddSysFxDevices()
{
    HRESULT                                 hr = S_OK;
    bool                                    allApoDevicesValid = true; 
    com_ptr_nothrow<IMMEndpoint>            pIMMEndpoint;
    com_ptr_nothrow<IMMEndpointInternal>    pIMMEndpointInt;
    com_ptr_nothrow<IPropertyStore>         pIPropertyStoreDevice;
    com_ptr_nothrow<IPropertyStore>         pIPropertyStoreFX;
    com_ptr_nothrow<IDeviceTopology>        pIDeviceTopology;
    com_ptr_nothrow<IConnector>             pIConnector;
    LPCWSTR                                 strConnectedDevice = NULL;
    PROPVARIANT                             pvName;
    PROPVARIANT                             pv;
    PROPVARIANT                             pvProxy;
    BOOL                                    bAPOProxy;

    
    if (FAILED(hr = m_spDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_INPROC_SERVER, NULL, (void**)&pIDeviceTopology)))
    {
        if (E_NOINTERFACE == hr)
        {
            LOG_OUTPUT(L"\tDevice is not active. Removing from test cases");
            //  There's the possibility that this legit if device is not active.
            return S_FALSE;
        }
            
        return hr;
    }
    
    LOG_RETURN_IF_FAILED(pIDeviceTopology->GetConnector(0, &pIConnector), 
                        L"\tFailed to get device connector");
    
    LOG_RETURN_IF_FAILED(pIConnector->GetDeviceIdConnectedTo((LPWSTR *)&strConnectedDevice),
                        L"\tFailed to get connected device ID");
    
    LOG_RETURN_IF_FAILED(m_spDevice->QueryInterface(__uuidof(IMMEndpoint), (void**)&pIMMEndpoint),
                        L"\tQueryInterfacce for IMMEndpoint failed");
    
    LOG_RETURN_IF_FAILED(pIMMEndpoint->QueryInterface(__uuidof(IMMEndpointInternal), (void**)&pIMMEndpointInt),
                        L"\tQueryInterface for IMMEndpointInternal failed");
    
    LOG_RETURN_IF_FAILED(m_spDevice->OpenPropertyStore(STGM_READ, &pIPropertyStoreDevice),
                        L"\tFailed to open property store for device");
    
    LOG_RETURN_IF_FAILED(pIMMEndpointInt->TryOpenFXPropertyStore(STGM_READ, &pIPropertyStoreFX),
                        L"\tFailed to Open SysFX property store for device");

    if (!pIPropertyStoreFX)
    {
        // This device doesn't appear to have any SysFX
        LOG_OUTPUT(L"\tThis device probably doesn't have any SysFx. Skipping and removing from test");    
        return S_FALSE;
    }
    
    PropVariantInit(&pvName);
    if (SUCCEEDED(pIPropertyStoreDevice->GetValue(PKEY_Device_FriendlyName, &pvName)))
    {
        if (VT_LPWSTR != V_VT(&pvName))
        {
            LOG_OUTPUT(L"\tFriendlyName value is not a string.");
        }
    }
    else
    {
        LOG_OUTPUT(L"\tIPropertyStore::GetValue returned error.");
    }
    

    OSVERSIONINFO ver = {0};
    ver.dwOSVersionInfoSize = sizeof(ver);

    GetVersionEx(&ver);

    for(int i = 0; i < ARRAYSIZE(PKEY_FX_EFFECTS); i++)
    {
        if (ver.dwMajorVersion < PKEY_FX_EFFECTS[i].dwMajorVersion || 
           (ver.dwMajorVersion == PKEY_FX_EFFECTS[i].dwMajorVersion &&
            ver.dwMinorVersion < PKEY_FX_EFFECTS[i].dwMinorVersion))
        {
            continue;
        }

        PropVariantInit(&pv);
        hr = pIPropertyStoreFX->GetValue(PKEY_FX_EFFECTS[i].key, &pv);
        LOG_OUTPUT(L"\tPKEY_FX_EFFECTS[%i] - GetValue PKEY_FX_EFFECTS[%i].key - 0x%x", i, i, hr);
        if (S_OK == hr)
        {
            LOG_OUTPUT(L"\tPKEY_FX_EFFECTS[%i] - V_VT(&pv) - %i", i, V_VT(&pv));
            if (VT_LPWSTR == V_VT(&pv))
            {
                bAPOProxy = FALSE;
                PropVariantInit(&pvProxy);
                
                // Check if mode effect is proxy
                if (PKEY_FX_EFFECTS[i].apoType & DT_MFX)
                {
                    hr = pIPropertyStoreFX->GetValue(PKEY_MFX_ProcessingModes_Supported_For_Streaming, &pvProxy);
                    LOG_OUTPUT(L"\tPKEY_FX_EFFECTS[%i] - GetValue MFX - 0x%x", i, hr);
                    if (S_OK == hr)
                    {
                        bAPOProxy = (pvProxy.vt != (VT_VECTOR | VT_LPWSTR)) ? TRUE : FALSE;
                    }
                }
                
                // Check if endpoint effect is proxy
                if (PKEY_FX_EFFECTS[i].apoType & DT_EFX)
                {
                    hr = pIPropertyStoreFX->GetValue(PKEY_EFX_ProcessingModes_Supported_For_Streaming, &pvProxy);
                    LOG_OUTPUT(L"\tPKEY_FX_EFFECTS[%i] - GetValue EFX - 0x%x", i, hr);
                    if (S_OK == hr)
                    {
                        bAPOProxy = (pvProxy.vt != (VT_VECTOR | VT_LPWSTR)) ? TRUE : FALSE;
                    }
                }

                LOG_OUTPUT(L"\tLooking at Audio Processing Object %ls\n", pv.pwszVal);

                unique_ptr<CAPODevice> pSysFxDevice(new CAPODevice(m_spDevice.get(), pv.pwszVal, pvName.pwszVal, pIPropertyStoreDevice.get(), 
                                                        pIPropertyStoreFX.get(), PKEY_FX_EFFECTS[i].apoType, PKEY_FX_EFFECTS[i].apoTypeName, 
                                                        strConnectedDevice, bAPOProxy));
    
                if (NULL != pSysFxDevice)
                {
                    if (pSysFxDevice->IsValid())
                    {
                        pSysFxDevice->m_fSelected = TRUE;
                        m_DeviceList.push_back(move(pSysFxDevice));
                    }
                    else
                    {
                        LOG_ERROR(L"\tInvalid sysFX device encountered");
                    }
                }

                PropVariantClear(&pvProxy);
            }
            else if ((VT_LPWSTR | VT_VECTOR) == V_VT(&pv) && pv.calpwstr.cElems > 0)
            {
                UINT32 cClsids = pv.calpwstr.cElems;
                bAPOProxy = FALSE;

                for (UINT32 j = 0; j < cClsids; j++)
                {
                    LOG_OUTPUT(L"\tLooking at Audio Processing Object %ls\n", pv.calpwstr.pElems[j]);

                    unique_ptr<CAPODevice> pSysFxDevice(new CAPODevice(m_spDevice.get(), pv.calpwstr.pElems[j], pvName.pwszVal, pIPropertyStoreDevice.get(),
                                                                       pIPropertyStoreFX.get(), PKEY_FX_EFFECTS[i].apoType, PKEY_FX_EFFECTS[i].apoTypeName,
                                                                       strConnectedDevice, bAPOProxy));

                    if (NULL != pSysFxDevice)
                    {
                        if (pSysFxDevice->IsValid())
                        {
                            pSysFxDevice->m_fSelected = TRUE;
                            m_DeviceList.push_back(move(pSysFxDevice));
                        }
                        else
                        {
                            LOG_ERROR(L"\tInvalid sysFX device encountered");
                        }
                    }
                }
            }
        }
        PropVariantClear(&pv);
    }

    return allApoDevicesValid ? S_OK : E_FAIL;
}

HRESULT CAPODeviceList::Initialize(wil::com_ptr_nothrow<IMMDevice> pDevice, LPWSTR deviceName)
{
    if (pDevice == nullptr || deviceName == nullptr)
    {
        LOG_ERROR(L"\tIMMDevice is null");
        return E_INVALIDARG;
    }

    m_spDevice = pDevice;

    wcscat(m_deviceName, deviceName);

    DWORD dwValue = 1;
    // Set registry entry so AudioDG won't crash during IAudioProcessingObjectRT function calls
    RETURN_IF_WIN32_ERROR(RegSetKeyValue(HKEY_LOCAL_MACHINE,
        REGKEY_AUDIO_SOFTWARE,
        PREVENT_APOTEST_CRASH_OR_REPORT_ON_APO_EXCEPTION,
        REG_DWORD,
        &dwValue,
        sizeof(dwValue)));

    return S_OK;
}
