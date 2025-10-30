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

#include "APOStressTest.h"
#include "sinewave.h"
#include "util.h"

#include <psapi.h>
#include <processthreadsapi.h>

using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

#define NUMBER_OF_TESTS 8
#define THIRTY_MIN_IN_SEC 1800
#define SIXTY_SECONDS 60
#define ONE_SEC_IN_MS 1000
#define THIRTY_MIN_IN_MS 1800000
#define MAX_ALLOWED_PAGEFILE_USAGE_IN_BYTES 300000000

#define IF_FAILED_RETURN(hr) { HRESULT hrCode = hr; if(FAILED(hrCode)) { return hrCode; } }

#define REGKEY_AUDIOSERVICE L"Software\\Microsoft\\Windows\\CurrentVersion\\Audio"
#define REGVALUE_SKIPRTHEAP L"SkipRTHeap"

const DWORD WAIT_TIME_MIN_FOR_THREAD_OPERATION = 3u * 1000u;        //  3 seconds
const DWORD WAIT_TIME_MAX_FOR_THREAD_OPERATION = 10u * 1000u;       // 10 seconds

size_t maxAudioDGPagefileUsage = 0;
std::mutex maxAudioDGPagefileUsage_mutex; // protects maxAudioDGPagefileUsage_mutex 

template <typename T>
inline T RandInRange(T min, T max)
{
    // Treat the value span as a double right away.  Otherwise if min = 0, and max
    // is truly the max for the type, the value span could loop to zero.
    double valueSpan = 1.0f + (double) (max - min);

    // The following had a problem where doubles larger than 0x8000000000000000 were always truncating
    // to 0x8000000000000000 when cast to a DWORD64.  Multiplying by 0.5f and then (T) 2 seemed to
    // get around this problem.
    //return min + (T) ((double) (max - min + 1) * (double) rand() / (double) (RAND_MAX + 1));

    // Then the following had a problem where RandInRange(0, 1) could never return 1.
    //return min + (T) 2 * (T) (0.5f * valueSpan * ((double) rand() / (double) (RAND_MAX + 1)));

    double offset = valueSpan * ((double) rand() / (double) (RAND_MAX + 1));

    if (offset > (double) ((DWORD64) 0x8000000000000000))
    {
        return min + (T) 2 * (T) (0.5f * offset);
    }

    return min + (T) offset;
}

void WriteToAudioObjectBuffer(FLOAT* buffer, UINT frameCount, FLOAT frequency, UINT samplingRate)
{
    const double PI = 4 * atan2(1.0, 1.0);
    static double _radPhase = 0.0;
    double step = 2 * PI * frequency / samplingRate;

    for (UINT i = 0; i < frameCount; i++)
    {
        double sample = sin(_radPhase);
        buffer[i] = FLOAT(sample);
        _radPhase += step; // next frame phase

        if (_radPhase >= 2 * PI)
        {
            _radPhase -= 2 * PI;
        }
    }
}

bool SetupSkipRTHeap()
{
    DWORD dwValue = 1;
    DWORD cbData = sizeof(dwValue);

    DWORD result = RegSetKeyValueW(HKEY_LOCAL_MACHINE, REGKEY_AUDIOSERVICE, REGVALUE_SKIPRTHEAP, REG_DWORD, &dwValue, cbData);

    RestartAudioService();

    return (result == ERROR_SUCCESS);
}

bool CleanupSkipRTHeap()
{
    DWORD result = RegDeleteKeyValueW(HKEY_LOCAL_MACHINE, REGKEY_AUDIOSERVICE, REGVALUE_SKIPRTHEAP);

    RestartAudioService();

    return (result == ERROR_SUCCESS);
}

bool CAPOStressTest::setUpMethod()
{
    return SetupSkipRTHeap();
}

bool CAPOStressTest::tearDownMethod()
{
    return CleanupSkipRTHeap();
}

void CAPOStressTest::AudioAPOStressTest()
{

    UINT cDevices = 0;
    com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    com_ptr_nothrow<IMMDeviceCollection> spDevices;
    vector<com_ptr<IMMDevice>> sEndpoints;

    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));
    VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &spDevices));
    VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

    for (UINT i = 0; i < cDevices; i++)
    {
        wil::com_ptr<IMMDevice> pEndpoint = nullptr;

        VERIFY_SUCCEEDED(spDevices->Item(i, &pEndpoint));

        sEndpoints.push_back(pEndpoint);
    }

    RunStressTest(THIRTY_MIN_IN_SEC, sEndpoints);
}

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

DWORD GetAudioDGProcessID()
{
    // Get the list of process identifiers.

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return 0;
    }

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Look for AudioDG name and process identifier on each process.

    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 && 0 == _wcsicmp(QueryProcessName(aProcesses[i]).c_str(), L"AUDIODG.EXE"))
        {
            return aProcesses[i];
        }
    }

    return 0;
}

void GetAudioDGMemoryUsageSample()
{
    // Get the AudioDG process ID
    DWORD audioDGProcessID = GetAudioDGProcessID();

    if (audioDGProcessID != 0)
    {
        std::lock_guard<std::mutex> autoLock(maxAudioDGPagefileUsage_mutex);

        // Memory PagefileUsage snapshot 
        PROCESS_MEMORY_COUNTERS pmc;

        HANDLE hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                        PROCESS_VM_READ,
                                        FALSE, audioDGProcessID );

        if (hProcess != NULL && GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
        {
            maxAudioDGPagefileUsage = max(maxAudioDGPagefileUsage, pmc.PagefileUsage);
        }

        VERIFY_IS_TRUE(maxAudioDGPagefileUsage < MAX_ALLOWED_PAGEFILE_USAGE_IN_BYTES);

        CloseHandle( hProcess );
    }
    else
    {
        Log::Warning(L"Unable to find AudioDG PID");
    }
}

HRESULT BasicAudioStreaming(IMMDevice* pEndpoint)
{
    com_ptr_nothrow<IAudioClient2> pAudioClient;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    IF_FAILED_RETURN(pEndpoint->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, NULL, (void**)&pAudioClient));

    AudioClientProperties clientProperties = { 0 };
    clientProperties.cbSize = sizeof(AudioClientProperties);
    clientProperties.bIsOffload = false;
    clientProperties.eCategory = (AUDIO_STREAM_CATEGORY) RandInRange(0, 11);

    IF_FAILED_RETURN(pAudioClient->SetClientProperties(&clientProperties));

    IF_FAILED_RETURN(pAudioClient->GetMixFormat(wil::out_param_ptr<WAVEFORMATEX **>(mixFormat)));

    IF_FAILED_RETURN(pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0,
        0,
        mixFormat.get(),
        nullptr));

    com_ptr_nothrow<IAudioRenderClient> pRenderClient;
    IF_FAILED_RETURN(pAudioClient->GetService(_uuidof(IAudioRenderClient), wil::out_param_ptr<void **>(pRenderClient)));

    wil::unique_event bufferCompleteEvent;
    bufferCompleteEvent.create();
    IF_FAILED_RETURN(pAudioClient->SetEventHandle(bufferCompleteEvent.get()));

    UINT32 bufferFrameCount;
    REFERENCE_TIME duration = RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION);
    IF_FAILED_RETURN(pAudioClient->GetBufferSize(&bufferFrameCount));

    SineWave sw(mixFormat->nChannels, mixFormat->nSamplesPerSec);
    for (unsigned i = 0; i < mixFormat->nChannels; ++i)
    {
        sw.SetChannel(i, 400, 0.25f);
    }

    BYTE* data{};
    IF_FAILED_RETURN(pRenderClient->GetBuffer(bufferFrameCount, &data));

    sw.FillFrames(data, bufferFrameCount);
    IF_FAILED_RETURN(pRenderClient->ReleaseBuffer(bufferFrameCount, 0));

    IF_FAILED_RETURN(pAudioClient->Start());

    unsigned c = 0;
    while (c++ < duration)
    {
        if (WaitForSingleObject(bufferCompleteEvent.get(), ONE_SEC_IN_MS) != WAIT_OBJECT_0)
        {
            break;
        }

        unsigned numFramesPadding{};
        IF_FAILED_RETURN(pAudioClient->GetCurrentPadding(&numFramesPadding));
        auto numFramesAvailable = bufferFrameCount - numFramesPadding;

        IF_FAILED_RETURN(pRenderClient->GetBuffer(numFramesAvailable, &data));
        sw.FillFrames(data, numFramesAvailable);
        IF_FAILED_RETURN(pRenderClient->ReleaseBuffer(numFramesAvailable, 0));
    }

    // Get a sample of the current AudioDG memory usage
    GetAudioDGMemoryUsageSample();

    IF_FAILED_RETURN(pAudioClient->Stop());

    return S_OK;
}

HRESULT DisableEnableEndpoint(IMMDevice* pEndpoint)
{
    com_ptr_nothrow<IMMEndpointManager> pManager = nullptr;

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    IF_FAILED_RETURN(CoCreateInstance(__uuidof(MMEndpointManager), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pManager)));

    IF_FAILED_RETURN(pManager->SetEndpointState(pEndpoint, DEVICE_STATE_DISABLED));

    Sleep(1000);

    IF_FAILED_RETURN(pManager->SetEndpointState(pEndpoint, DEVICE_STATE_ACTIVE));

    return S_OK;
}

HRESULT BasicAudioCapture(IMMDevice* pEndpoint)
{
    com_ptr_nothrow<IAudioClient> pAudioClient;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    IF_FAILED_RETURN(pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient));

    IF_FAILED_RETURN(pAudioClient->GetMixFormat(wil::out_param_ptr<WAVEFORMATEX **>(mixFormat)));

    IF_FAILED_RETURN(pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0,
        0,
        mixFormat.get(),
        nullptr));

    com_ptr_nothrow<IAudioCaptureClient> pCaptureClient;
    IF_FAILED_RETURN(pAudioClient->GetService(_uuidof(IAudioCaptureClient), wil::out_param_ptr<void **>(pCaptureClient)));

    wil::unique_event bufferCompleteEvent;
    bufferCompleteEvent.create();
    IF_FAILED_RETURN(pAudioClient->SetEventHandle(bufferCompleteEvent.get()));

    UINT32 bufferFrameCount;
    REFERENCE_TIME duration = RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION);
    IF_FAILED_RETURN(pAudioClient->GetBufferSize(&bufferFrameCount));

    BYTE* data{};
    DWORD dwFlags = 0;
    UINT32 numFramesInNextPacket = 0;

    IF_FAILED_RETURN(pCaptureClient->GetNextPacketSize(&numFramesInNextPacket));
    IF_FAILED_RETURN(pCaptureClient->GetBuffer(&data, &numFramesInNextPacket, &dwFlags, NULL, NULL)); 

    IF_FAILED_RETURN(pAudioClient->Start());

    unsigned c = 0;
    while (c++ < duration)
    {
       if (WaitForSingleObject(bufferCompleteEvent.get(), ONE_SEC_IN_MS) != WAIT_OBJECT_0)
        {
            break;
        }

        IF_FAILED_RETURN(pCaptureClient->ReleaseBuffer(numFramesInNextPacket));

        IF_FAILED_RETURN(pCaptureClient->GetBuffer(&data, &numFramesInNextPacket, &dwFlags, NULL, NULL));
    }

    // Get a sample of the current AudioDG memory usage
    GetAudioDGMemoryUsageSample();
    
    IF_FAILED_RETURN(pAudioClient->Stop());

    return S_OK;
}

HRESULT BasicAudioLoopback(IMMDevice* pEndpoint)
{
    com_ptr_nothrow<IAudioClient> pAudioClient;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    IF_FAILED_RETURN(pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient));

    IF_FAILED_RETURN(pAudioClient->GetMixFormat(wil::out_param_ptr<WAVEFORMATEX **>(mixFormat)));

    IF_FAILED_RETURN(pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
        0,
        0,
        mixFormat.get(),
        nullptr));

    com_ptr_nothrow<IAudioCaptureClient> pCaptureClient;
    IF_FAILED_RETURN(pAudioClient->GetService(__uuidof(IAudioCaptureClient), wil::out_param_ptr<void **>(pCaptureClient)));

    wil::unique_event bufferCompleteEvent;
    bufferCompleteEvent.create();
    IF_FAILED_RETURN(pAudioClient->SetEventHandle(bufferCompleteEvent.get()));

    UINT32 bufferFrameCount;
    IF_FAILED_RETURN(pAudioClient->GetBufferSize(&bufferFrameCount));

    BYTE* data{};
    DWORD dwFlags = 0;
    UINT32 numFramesInNextPacket = 0;

    IF_FAILED_RETURN(pCaptureClient->GetNextPacketSize(&numFramesInNextPacket));

    IF_FAILED_RETURN(pAudioClient->Start());

    while (numFramesInNextPacket != 0)
    {
        IF_FAILED_RETURN(pCaptureClient->GetBuffer(&data, &numFramesInNextPacket, &dwFlags, NULL, NULL));

        IF_FAILED_RETURN(pCaptureClient->ReleaseBuffer(numFramesInNextPacket));

        IF_FAILED_RETURN(pCaptureClient->GetNextPacketSize(&numFramesInNextPacket));
    }

    // Get a sample of the current AudioDG memory usage
    GetAudioDGMemoryUsageSample();

    IF_FAILED_RETURN(pAudioClient->Stop());

    return S_OK;
}

HRESULT BasicSpatialAudio(IMMDevice* pEndpoint)
{
#ifdef LNM
    EnableSpatialAudio();
#endif

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    com_ptr_nothrow<ISpatialAudioClient> client;
    IF_FAILED_RETURN(pEndpoint->Activate(__uuidof(ISpatialAudioClient), CLSCTX_ALL, nullptr, wil::out_param_ptr<void **>(client)));

    com_ptr_nothrow<IAudioFormatEnumerator> enumer;
    IF_FAILED_RETURN(client->GetSupportedAudioObjectFormatEnumerator(&enumer));

    wil::unique_cotaskmem_ptr<WAVEFORMATEX> fmt;
    IF_FAILED_RETURN(enumer->GetFormat(0, wil::out_param_ptr<WAVEFORMATEX **>(fmt)));

    unique_event event;
    event.create();
    if (event == nullptr)
    {
        HRESULT_FROM_WIN32(GetLastError());
    }

    unique_prop_variant pv;
    auto params = reinterpret_cast<SpatialAudioObjectRenderStreamActivationParams *>(CoTaskMemAlloc(sizeof(SpatialAudioObjectRenderStreamActivationParams)));
    *params = SpatialAudioObjectRenderStreamActivationParams{fmt.get(), (AudioObjectType)AudioObjectType_FrontLeft, 0, 0, AudioCategory_Other, event.get(), nullptr};
    pv.vt = VT_BLOB;
    pv.blob.cbSize = sizeof(*params);
    pv.blob.pBlobData = reinterpret_cast<BYTE *>(params);

    com_ptr_nothrow<ISpatialAudioObjectRenderStream> stream;
    IF_FAILED_RETURN(client->ActivateSpatialAudioStream(&pv, __uuidof(ISpatialAudioObjectRenderStream), wil::out_param_ptr<void **>(stream)));

    com_ptr_nothrow<ISpatialAudioObject> audioObjectFrontLeft;
    IF_FAILED_RETURN(stream->ActivateSpatialAudioObject(AudioObjectType_FrontLeft, &audioObjectFrontLeft));

    IF_FAILED_RETURN(stream->Start());

    UINT totalFrameCount = fmt->nSamplesPerSec * 2;

    bool isRendering = true;
    while (isRendering)
    {
        // Wait for a signal from the audio-engine to start the next processing pass
        if (WaitForSingleObject(event.get(), 100) != WAIT_OBJECT_0)
        {
            IF_FAILED_RETURN(stream->Reset());
        }
    
        UINT32 availableDynamicObjectCount = 0;
        UINT32 frameCount = 0;
    
        // Begin the process of sending object data and metadata
        // Get the number of dynamic objects that can be used to send object-data
        // Get the frame count that each buffer will be filled with 
        IF_FAILED_RETURN(stream->BeginUpdatingAudioObjects(&availableDynamicObjectCount, &frameCount));
    
        BYTE* buffer = nullptr;
        UINT32 bufferLength = 0;
    
        if (audioObjectFrontLeft == nullptr)
        {
            IF_FAILED_RETURN(stream->ActivateSpatialAudioObject(AudioObjectType::AudioObjectType_FrontLeft, &audioObjectFrontLeft));
        }
    
        // Get the buffer to write audio data
        IF_FAILED_RETURN(audioObjectFrontLeft->GetBuffer(&buffer, &bufferLength));
    
        if (totalFrameCount >= frameCount)
        {
            // Write audio data to the buffer
            WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), frameCount, 200.0f, fmt->nSamplesPerSec);
    
            totalFrameCount -= frameCount;
        }
        else
        {
            // Write audio data to the buffer
            WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), totalFrameCount, 750.0f, fmt->nSamplesPerSec);
    
            // Get a sample of the current AudioDG memory usage
            GetAudioDGMemoryUsageSample();

            // Set end of stream for the last buffer 
            IF_FAILED_RETURN(audioObjectFrontLeft->SetEndOfStream(totalFrameCount));
    
            audioObjectFrontLeft = nullptr; // Release the object
    
            isRendering = false;
        }
    
        // Let the audio engine know that the object data are available for processing now
        IF_FAILED_RETURN(stream->EndUpdatingAudioObjects());
    }

    return S_OK;
}

HRESULT BasicOffloadStreaming(IMMDevice* pEndpoint)
{
    com_ptr_nothrow<IAudioClient2> pAudioClient;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;
    BOOL bOffloadCapable = FALSE;

    if (pEndpoint == nullptr)
    {
        return E_INVALIDARG;
    }

    IF_FAILED_RETURN(pEndpoint->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, NULL, (void**)&pAudioClient));
    IF_FAILED_RETURN(pAudioClient->IsOffloadCapable(AudioCategory_Media, &bOffloadCapable));

    AudioClientProperties clientProperties = { 0 };
    clientProperties.cbSize = sizeof(AudioClientProperties);
    clientProperties.bIsOffload = bOffloadCapable ? true : false;
    clientProperties.eCategory = AudioCategory_Media;

    if (bOffloadCapable)
    {
        clientProperties.bIsOffload = true;
    }

    IF_FAILED_RETURN(pAudioClient->SetClientProperties(&clientProperties));

    IF_FAILED_RETURN(pAudioClient->GetMixFormat(wil::out_param_ptr<WAVEFORMATEX **>(mixFormat)));

    IF_FAILED_RETURN(pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0,
        0,
        mixFormat.get(),
        nullptr));

    com_ptr_nothrow<IAudioRenderClient> pRenderClient;
    IF_FAILED_RETURN(pAudioClient->GetService(_uuidof(IAudioRenderClient), wil::out_param_ptr<void **>(pRenderClient)));

    wil::unique_event bufferCompleteEvent;
    bufferCompleteEvent.create();
    IF_FAILED_RETURN(pAudioClient->SetEventHandle(bufferCompleteEvent.get()));

    UINT32 bufferFrameCount;
    REFERENCE_TIME duration = RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION);
    IF_FAILED_RETURN(pAudioClient->GetBufferSize(&bufferFrameCount));

    SineWave sw(mixFormat->nChannels, mixFormat->nSamplesPerSec);
    for (unsigned i = 0; i < mixFormat->nChannels; ++i)
    {
        sw.SetChannel(i, 400, 0.25f);
    }

    BYTE* data{};
    IF_FAILED_RETURN(pRenderClient->GetBuffer(bufferFrameCount, &data));

    sw.FillFrames(data, bufferFrameCount);
    IF_FAILED_RETURN(pRenderClient->ReleaseBuffer(bufferFrameCount, 0));

    IF_FAILED_RETURN(pAudioClient->Start());

    unsigned c = 0;
    while (c++ < duration)
    {
        if (WaitForSingleObject(bufferCompleteEvent.get(), ONE_SEC_IN_MS) != WAIT_OBJECT_0)
        {
            break;
        }

        unsigned numFramesPadding{};
        IF_FAILED_RETURN(pAudioClient->GetCurrentPadding(&numFramesPadding));
        auto numFramesAvailable = bufferFrameCount - numFramesPadding;

        IF_FAILED_RETURN(pRenderClient->GetBuffer(numFramesAvailable, &data));
        sw.FillFrames(data, numFramesAvailable);
        IF_FAILED_RETURN(pRenderClient->ReleaseBuffer(numFramesAvailable, 0));
    }

    // Get a sample of the current AudioDG memory usage
    GetAudioDGMemoryUsageSample();

    IF_FAILED_RETURN(pAudioClient->Stop());

    return S_OK;
}

void RunStressTest(int timeInSeconds, std::vector<wil::com_ptr<IMMDevice>>& pEndpoints)
{
    const UINT num = static_cast<UINT>(pEndpoints.size()) * NUMBER_OF_TESTS;

    std::vector<std::thread> threads(num);

    for (UINT i = 0; i < pEndpoints.size(); i++)
    {
        threads[i * NUMBER_OF_TESTS + 0] = std::thread([=](){
            BasicAudioStreaming(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 1] = std::thread([=](){
            BasicAudioStreaming(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 2] = std::thread([=](){
            BasicAudioStreaming(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 3] = std::thread([=](){
            BasicAudioCapture(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 4] = std::thread([=](){
            BasicAudioLoopback(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 5] = std::thread([=](){
            DisableEnableEndpoint(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 6] = std::thread([=](){
            BasicSpatialAudio(pEndpoints[i].get());
        });

        threads[i * NUMBER_OF_TESTS + 7] = std::thread([=](){
            BasicOffloadStreaming(pEndpoints[i].get());
        });
    }

    time_t timer;
    time(&timer);

    time_t endTime = timer + timeInSeconds;

    while (timer < endTime) 
    {
        for (UINT i = 0; i < num; i++)
        {
            if (threads[i].joinable())
            {
                UINT testNumber = i % NUMBER_OF_TESTS;
                UINT k = i / NUMBER_OF_TESTS;

                threads[i].join();

                switch (testNumber)
                {
                case 0:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicAudioStreaming(pEndpoints[k].get());
                    });
                    break;
                case 1:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicAudioStreaming(pEndpoints[k].get());
                    });
                    break;
                case 2:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicAudioStreaming(pEndpoints[k].get());
                    });
                    break;
                case 3:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicAudioCapture(pEndpoints[k].get());
                    });
                    break;
                case 4:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicAudioLoopback(pEndpoints[k].get());
                    });
                    break;
                case 5:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        DisableEnableEndpoint(pEndpoints[k].get());
                    });
                    break;
                case 6:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicSpatialAudio(pEndpoints[k].get());
                    });
                    break;
                case 7:
                    threads[i] = std::thread([=]() {
                        Sleep(RandInRange(WAIT_TIME_MIN_FOR_THREAD_OPERATION, WAIT_TIME_MAX_FOR_THREAD_OPERATION));
                        BasicOffloadStreaming(pEndpoints[k].get());
                    });
                    break;
                }
            }
        }

        time(&timer);
    }

    for (UINT j = 0; j < num; j++)
    {
        if (threads[j].joinable())
        {
            threads[j].join();
        }
    }

    std::lock_guard<std::mutex> autoLock(maxAudioDGPagefileUsage_mutex);

    // If we were not able to open the AudioDG handle process we should have 0 as memory usage
    VERIFY_IS_TRUE(maxAudioDGPagefileUsage > 0);

    // Adding the maxPagefileUsageValue to the VERIFY as a string
    wchar_t maxAudioDGPagefileUsageString[32];
    swprintf_s(maxAudioDGPagefileUsageString, L"%u", maxAudioDGPagefileUsage);

    VERIFY_IS_TRUE(maxAudioDGPagefileUsage < MAX_ALLOWED_PAGEFILE_USAGE_IN_BYTES, maxAudioDGPagefileUsageString);
}