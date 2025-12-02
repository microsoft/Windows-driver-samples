#include "CAPXThreadingFrameworkTests.h"


bool CAPOThreadingFrameworkTest::setUpMethod()
{
    SLGetWindowsInformationDWORD(POLICY_KERNEL_ONECORE_DEVICE_FAMILY_ID, &m_dwPlatform);

    EndpointsWithCapx(m_capxEndpoints);

    return SetupSkipRTHeap();
}

bool CAPOThreadingFrameworkTest::tearDownMethod()
{
    return CleanupSkipRTHeap();
}

using malloca_deleter = wil::function_deleter<decltype(&::_freea), ::_freea>;
template <typename T>
using unique_malloca_ptr = wistd::unique_ptr<T, malloca_deleter>;
int g_mmThreadCount = 0;

class CEventParser : public IEtwEventHandler
{
public:
    int GetCountOfAPOThreads() { return m_countOfAPOThreads; }

private:

    int m_countOfAPOThreads = 0;

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
     
                // Threading implemented is the temporary name that the threading framework will be called until the final logs are created
                if (_wcsicmp(L"Threading implemented", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE*>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    m_countOfAPOThreads++;
                }
                auto mmThreadMock = Mock10::Mock::Function(&AvSetMmThreadCharacteristics, [](LPCWSTR taskName, LPDWORD taskIndex) {
                    g_mmThreadCount++;
                    return AvSetMmThreadCharacteristics(taskName, taskIndex);
                });
                if (_wcsicmp(L"CreateThread", reinterpret_cast<PWSTR>(reinterpret_cast<BYTE*>(eventInfo.get()) + eventInfo->EventNameOffset)) == 0)
                {
                    g_mmThreadCount++;
                }
            }
            return S_OK;
        }();
    }
};

// {6e7b1892-5288-5fe5-8f34-e3b0dc671fd2}
static const GUID AudioSesTelemetryProvider =
{ 0x6e7b1892, 0x5288, 0x5fe5, 0x8f, 0x34, 0xe3, 0xb0, 0xdc, 0x67, 0x1f, 0xd2 };

void CAPOThreadingFrameworkTest::ThreadingFrameworkTest()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    if (m_dwPlatform != DEVICEFAMILYINFOENUM_WINDOWS_CORE && m_capxEndpoints.size() == 0)
    {
        Log::Result(TestResults::Skipped, L"There are no CAPX APOs to test");
        return;
    }

    CEtwListener listener;
    CEventParser parser;

    VERIFY_SUCCEEDED(listener.StartTraceSession(L"ThreadingFrameworkTest", static_cast<IEtwEventHandler *>(&parser)));

    // Enable Microsoft.Windows.Audio.Client
    VERIFY_SUCCEEDED(listener.EnableProvider(AudioSesTelemetryProvider, TRACE_LEVEL_INFORMATION, 8));

    auto stopListener = scope_exit([&] {
        listener.StopTraceSession();
    });

    wil::com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));

    RunStressTest(FIVE_MIN_IN_SEC, m_capxEndpoints);

    LOG_OUTPUT(L"Number of APO threads created: %d", parser.GetCountOfAPOThreads());
    VERIFY_IS_TRUE(g_mmThreadCount == 0);
}