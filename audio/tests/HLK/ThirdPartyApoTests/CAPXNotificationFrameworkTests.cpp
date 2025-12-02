#include "CAPXNotificationFrameworkTests.h"


bool CAPONotificationFrameworkTest::setUpMethod()
{
    SLGetWindowsInformationDWORD(POLICY_KERNEL_ONECORE_DEVICE_FAMILY_ID, &m_dwPlatform);

    EndpointsWithCapx(m_capxEndpoints);

    return true;
}

bool CAPONotificationFrameworkTest::tearDownMethod()
{
    return true;
}

using malloca_deleter = wil::function_deleter<decltype(&::_freea), ::_freea>;
template <typename T>
using unique_malloca_ptr = wistd::unique_ptr<T, malloca_deleter>;

class CEventParser : public IEtwEventHandler
{
public:
    int GetCountOfAPONotifications() { return m_countOfAudioProcessingNotifications; }
    bool WereOldNotificationsDetected() { return m_bOldNotificationsDetected; }

private:

    int m_countOfAudioProcessingNotifications = 0;
    DWORD m_dLastPid = 0;
    DWORD m_dLastThreadId = 0;
    bool m_bOldNotificationsDetected = false;

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

                // Check for old notification types
                if (_wcsicmp(L"IAudioProcessingObjectNotifications implemented", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    m_countOfAudioProcessingNotifications++;
                    m_dLastPid = pEventRecord->EventHeader.ProcessId;
                    m_dLastThreadId = pEventRecord->EventHeader.ThreadId;
                }
                if (_wcsicmp(L"OnPropertyValueChanged", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    if (m_dLastPid == pEventRecord->EventHeader.ProcessId &&
                        m_dLastThreadId == pEventRecord->EventHeader.ThreadId)
                    {
                        m_bOldNotificationsDetected = true;
                    }
                }
                if (_wcsicmp(L"OnEndpointVolumeChanged", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    if (m_dLastPid == pEventRecord->EventHeader.ProcessId &&
                        m_dLastThreadId == pEventRecord->EventHeader.ThreadId)
                    {
                        m_bOldNotificationsDetected = true;
                    }
                }
                if (_wcsicmp(L"RegisterEndpointNotificationCallback", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0 ||
                    _wcsicmp(L"RegisterEndpointNotificationCallbackWithVirtualEnumerator", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    if (m_dLastPid == pEventRecord->EventHeader.ProcessId &&
                        m_dLastThreadId == pEventRecord->EventHeader.ThreadId)
                    {
                        m_bOldNotificationsDetected = true;
                    }
                }
                if (_wcsicmp(L"UnregisterEndpointNotificationCallback", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0 ||
                    _wcsicmp(L"UnregisterEndpointNotificationCallbackWithVirtualEnumerator", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE *>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    if (m_dLastPid == pEventRecord->EventHeader.ProcessId &&
                        m_dLastThreadId == pEventRecord->EventHeader.ThreadId)
                    {
                        m_bOldNotificationsDetected = true;
                    }
                }
            }
            return S_OK;
        }();
    }
};

// {6e7b1892-5288-5fe5-8f34-e3b0dc671fd2}
static const GUID AudioSesTelemetryProvider =
{ 0x6e7b1892, 0x5288, 0x5fe5, 0x8f, 0x34, 0xe3, 0xb0, 0xdc, 0x67, 0x1f, 0xd2 };

void CAPONotificationFrameworkTest::NotificationFrameworkTest()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

     if (m_dwPlatform != DEVICEFAMILYINFOENUM_WINDOWS_CORE && m_capxEndpoints.size() == 0)
    {
        Log::Result(TestResults::Skipped, L"There are no CAPX APOs to test");
        return;
    }

    CEtwListener listener;
    CEventParser parser;

    VERIFY_SUCCEEDED(listener.StartTraceSession(L"NotificationFrameworkTest", static_cast<IEtwEventHandler *>(&parser)));

    // Enable Microsoft.Windows.Audio.Client
    VERIFY_SUCCEEDED(listener.EnableProvider(AudioSesTelemetryProvider, TRACE_LEVEL_INFORMATION, 8));

    auto stopListener = scope_exit([&] {
        listener.StopTraceSession();
    });

    {
        for (UINT i = 0; i < m_capxEndpoints.size(); i++)
        {
            wil::com_ptr_nothrow<IMMDevice> pEndpoint = m_capxEndpoints.at(i);

            wil::unique_prop_variant var;
            wil::com_ptr_nothrow<IPropertyStore> spPropertyStore;
            VERIFY_SUCCEEDED(pEndpoint->OpenPropertyStore(STGM_READ, &spPropertyStore));
            VERIFY_SUCCEEDED(spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var));
            VERIFY_ARE_EQUAL(VT_LPWSTR, var.vt);
            LOG_OUTPUT(L"Testing endpoint: %s", var.pwszVal);

            wil::com_ptr_nothrow<IAudioEndpointVolume> aev;
            VERIFY_SUCCEEDED(pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&aev));

            VERIFY_SUCCEEDED(aev->SetMasterVolumeLevelScalar((0.7f), NULL));

            Sleep(500);

            VERIFY_SUCCEEDED(aev->SetMasterVolumeLevelScalar((1.0f), NULL));

            LOG_OUTPUT(L"Number of APO notifications detected: %d", parser.GetCountOfAPONotifications());  
        }

        VERIFY_IS_FALSE(parser.WereOldNotificationsDetected());
    }
}