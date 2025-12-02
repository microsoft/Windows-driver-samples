//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

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

class CTestWasapiUsageFromAudioDg
{
    BEGIN_TEST_CLASS(CTestWasapiUsageFromAudioDg)
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

protected:
    BEGIN_TEST_METHOD(TestStreamCreation)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"50B80D35-5E61-4D91-B201-5F89D67E4A5A")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"216ADA42-25DC-4504-9032-16D63704FB08")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Test WASAPI usage from APOs - TestStreamCreation")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"30")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"60")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: Test WASAPI usage")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()
};

using malloca_deleter = wil::function_deleter<decltype(&::_freea), ::_freea>;
template <typename T>
using unique_malloca_ptr = wistd::unique_ptr<T, malloca_deleter>;

class CEventParser : public IEtwEventHandler
{
public:
    int GetCountOfAudioClientInits() { return m_countOfAudioClientInits; }
    int GetCountOfAudioClientInitsFromAudioDg() { return m_countOfAudioClientInitsFromAudioDg; }

private:

    int m_countOfAudioClientInits = 0;
    int m_countOfAudioClientInitsFromAudioDg = 0;

    std::wstring QueryProcessName(const DWORD pid)
    {
        wil::unique_handle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if(NULL == hProcess)
        {
            return L"err " + std::to_wstring(GetLastError());
        }
        else
        {
            WCHAR szProcessName[1024];
            DWORD cchSize = ARRAYSIZE(szProcessName);
            if (0 == QueryFullProcessImageNameW(hProcess.get(), 0, szProcessName, &cchSize))
            {
                return L"err " + std::to_wstring(GetLastError());
            }

            PWSTR fileName = szProcessName + cchSize;
            while (fileName != szProcessName && *fileName != L'\\') fileName--;
            
            return (*fileName == L'\\') ? fileName + 1 : fileName;
        }
    }

    void OnEvent(PEVENT_RECORD pEventRecord)
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);
        [&]() {
            ULONG bufferSize = 0;
            if (TdhGetEventInformation(pEventRecord, 0, nullptr, nullptr, &bufferSize) == ERROR_INSUFFICIENT_BUFFER)
            {
                unique_malloca_ptr<TRACE_EVENT_INFO> eventInfo(reinterpret_cast<PTRACE_EVENT_INFO>(_malloca(bufferSize)));
                VERIFY_IS_NOT_NULL(eventInfo);
                RETURN_IF_WIN32_ERROR(TdhGetEventInformation(pEventRecord, 0, nullptr, eventInfo.get(), &bufferSize));
     
                // LOG_OUTPUT(L"Received event: %s", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE*>(eventInfo.get()) + eventInfo->EventNameOffset));
                if (_wcsicmp(L"AudioClientInitialize", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE*>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    m_countOfAudioClientInits++;
                    
                    try
                    {
                        // LOG_OUTPUT(L"Process name: %s", QueryProcessName(pEventRecord->EventHeader.ProcessId).c_str());
                        if (0 == _wcsicmp(QueryProcessName(pEventRecord->EventHeader.ProcessId).c_str(), L"audiodg.exe"))
                        {
                            m_countOfAudioClientInitsFromAudioDg++;
                        }
                    }
                    CATCH_LOG()
                }
            }
            return S_OK;
        }();
    }
};

// {6e7b1892-5288-5fe5-8f34-e3b0dc671fd2}
static const GUID AudioSesTelemetryProvider =
{ 0x6e7b1892, 0x5288, 0x5fe5, 0x8f, 0x34, 0xe3, 0xb0, 0xdc, 0x67, 0x1f, 0xd2 };

void CTestWasapiUsageFromAudioDg::TestStreamCreation()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    CEtwListener listener;
    CEventParser parser;

    VERIFY_SUCCEEDED(listener.StartTraceSession(L"TestWasapiUsageFromAudioDg", static_cast<IEtwEventHandler *>(&parser)));

    // Enable Microsoft.Windows.Audio.Client
    VERIFY_SUCCEEDED(listener.EnableProvider(AudioSesTelemetryProvider, TRACE_LEVEL_INFORMATION, 8));

    wil::com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));

    // Test Render endpoints - all stream categories are valid except FarFieldSpeech, UniformSpeech, and VoiceTyping
    {
        wil::com_ptr_nothrow<IMMDeviceCollection> spDevices;
        VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &spDevices));

        UINT cDevices = 0;
        VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

        for (UINT i = 0; i < cDevices; i++)
        {
            wil::com_ptr_nothrow<IMMDevice> pEndpoint;
            VERIFY_SUCCEEDED(spDevices->Item(i, &pEndpoint));

            wil::unique_prop_variant var;
            wil::com_ptr_nothrow<IPropertyStore> spPropertyStore;
            VERIFY_SUCCEEDED(pEndpoint->OpenPropertyStore(STGM_READ, &spPropertyStore));
            VERIFY_SUCCEEDED(spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var));
            VERIFY_ARE_EQUAL(VT_LPWSTR, var.vt);
            LOG_OUTPUT(L"Testing endpoint: %s", var.pwszVal);

            for (int category = ExtendedAudioCategory_Other; category < ExtendedAudioCategory_enum_count; category++)
            {
                if (!(category == ExtendedAudioCategory_FarFieldSpeech ||
                      category == ExtendedAudioCategory_UniformSpeech ||
                      category == ExtendedAudioCategory_VoiceTyping))
                {
                    wil::com_ptr_nothrow<IAudioClient2> ac;
                    VERIFY_SUCCEEDED(pEndpoint->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, NULL, (void **)&ac));

                    AudioClientPropertiesPrivate acpp = {0};
                    acpp.ClientProps.cbSize = sizeof(acpp);
                    acpp.extCategory = (AUDIO_STREAM_EXTENDED_CATEGORY)category;

                    VERIFY_SUCCEEDED(ac->SetClientProperties(&acpp.ClientProps));

                    wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;
                    VERIFY_SUCCEEDED(ac->GetMixFormat(wil::out_param(mixFormat)));

                    LOG_OUTPUT(L"Testing category: %d", acpp.extCategory);
                    VERIFY_SUCCEEDED(ac->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, mixFormat.get(), nullptr));

                    Sleep(1000);
                }
            }
        }
    }

    // Test Capture endpoints - only some stream categories are valid
    {
        constexpr AUDIO_STREAM_EXTENDED_CATEGORY captureCategories[] = {ExtendedAudioCategory_Other, 
                                                                        ExtendedAudioCategory_Communications,
                                                                        ExtendedAudioCategory_Media,
                                                                        ExtendedAudioCategory_GameChat,
                                                                        ExtendedAudioCategory_Speech,
                                                                        ExtendedAudioCategory_PersonalAssistant,
                                                                        ExtendedAudioCategory_FarFieldSpeech,
                                                                        ExtendedAudioCategory_UniformSpeech,
                                                                        ExtendedAudioCategory_VoiceTyping,
                                                                        };

        wil::com_ptr_nothrow<IMMDeviceCollection> spDevices;
        VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &spDevices));

        UINT cDevices = 0;
        VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

        for (UINT i = 0; i < cDevices; i++)
        {
            wil::com_ptr_nothrow<IMMDevice> pEndpoint;
            VERIFY_SUCCEEDED(spDevices->Item(i, &pEndpoint));

            wil::unique_prop_variant var;
            wil::com_ptr_nothrow<IPropertyStore> spPropertyStore;
            VERIFY_SUCCEEDED(pEndpoint->OpenPropertyStore(STGM_READ, &spPropertyStore));
            VERIFY_SUCCEEDED(spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var));
            VERIFY_ARE_EQUAL(VT_LPWSTR, var.vt);
            LOG_OUTPUT(L"Testing endpoint: %s", var.pwszVal);

            for (int idx = 0; idx < ARRAYSIZE(captureCategories); idx++)
            {
                wil::com_ptr_nothrow<IAudioClient2> ac;
                VERIFY_SUCCEEDED(pEndpoint->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, NULL, (void**)&ac));

                AudioClientPropertiesPrivate acpp = { 0 };
                acpp.ClientProps.cbSize = sizeof(acpp);
                acpp.extCategory = captureCategories[idx];

                VERIFY_SUCCEEDED(ac->SetClientProperties(&acpp.ClientProps));

                wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;
                VERIFY_SUCCEEDED(ac->GetMixFormat(wil::out_param(mixFormat)));

                LOG_OUTPUT(L"Testing category: %d", acpp.extCategory);
                VERIFY_SUCCEEDED(ac->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, mixFormat.get(), nullptr));

                Sleep(1000);
            }
        }
    }

    // Sleep a while to ensure we receive all the events from the above streams
    Sleep(5000);

    LOG_OUTPUT(L"Number of audio streams created: %d", parser.GetCountOfAudioClientInits());
    LOG_OUTPUT(L"Number of audio streams created from AudioDg: %d", parser.GetCountOfAudioClientInitsFromAudioDg());
    VERIFY_ARE_EQUAL(0, parser.GetCountOfAudioClientInitsFromAudioDg());

    // Stop the trace
    listener.StopTraceSession();

}
