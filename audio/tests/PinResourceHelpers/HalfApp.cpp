// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Module Name:
//
//  HalfApp.cpp
//
// Abstract:
//
//  Implementation for CHalfApp class
//
// -------------------------------------------------------------------------------












HRESULT
CHalfApp::InitializeEndpoint
(
    void
)
{
    HRESULT                                     hr = S_OK;



    wil::unique_prop_variant                    varActivationParameter;



    WAVEFORMATEX*                                 pWfx = m_pCurrentFormat.get();

    if (!VERIFY_IS_NOT_NULL(pWfx)) {
        hr = E_NOTFOUND;
        return hr;
    }









    // Allocate a structure for endpoint creation.
    varActivationParameter.vt = VT_BLOB;

    // Fill the endpoint creation structure...

















    // Activate the endpoint
    if (m_DataFlow == render) {
        wil::com_ptr_nothrow<IAudioOutputEndpointRT> spAudioOutputEndpointRT;
        if (!VERIFY_SUCCEEDED(hr = m_pDevice->Activate(
            __uuidof(IAudioOutputEndpointRT),
            CLSCTX_INPROC_SERVER,
            &varActivationParameter,
            reinterpret_cast<void**>(&spAudioOutputEndpointRT)))) {
            return hr;
        }
        spAudioOutputEndpointRT.query_to(&m_pAudioDeviceEndpoint);
        if (!VERIFY_IS_NOT_NULL(m_pAudioDeviceEndpoint)) {
            hr = E_FAIL;
            return hr;
        }
    }
    else {
        wil::com_ptr_nothrow<IAudioInputEndpointRT> spAudioInputEndpointRT;
        if (!VERIFY_SUCCEEDED(hr = m_pDevice->Activate(
            __uuidof(IAudioInputEndpointRT),
            CLSCTX_INPROC_SERVER,
            &varActivationParameter,
            reinterpret_cast<void**>(&spAudioInputEndpointRT)))) {
            return hr;
        }
        spAudioInputEndpointRT.query_to(&m_pAudioDeviceEndpoint);
        if (!VERIFY_IS_NOT_NULL(m_pAudioDeviceEndpoint)) {
            hr = E_FAIL;
            return hr;
        }
    }

    // Initialize other endpoint interfaces
    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint.query_to(&m_pAudioEndpointControl))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint.query_to(&m_pAudioEndpoint))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint.query_to(&m_pAudioEndpointRT))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint.query_to(&m_pAudioClock))) {
        return hr;
    }

    return hr;
}

HRESULT
CHalfApp::ReleaseEndpoint
(
    void
)
{
    HRESULT hr = S_OK;

    if (m_pAudioEndpointControl) {
        m_pAudioEndpointControl->Reset();
    }

    m_pAudioDeviceEndpoint.reset();
    m_pAudioEndpointControl.reset();
    m_pAudioEndpoint.reset();
    m_pAudioEndpointRT.reset();
    m_pAudioClock.reset();

    return hr;
}

HRESULT
CHalfApp::StartEndpoint
(
    void
)
{
    HRESULT hr = S_OK;
    auto lock = m_CritSec.lock();

    if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpointControl->Start())) {
        return hr;
    }

    return hr;
}

HRESULT
CHalfApp::StopEndpoint
(
    void
)
{
    HRESULT hr = S_OK;
    auto lock = m_CritSec.lock();

    if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpointControl->Stop())) {
        return hr;
    }

    return hr;
}

HRESULT
CHalfApp::ResetEndpoint
(
    void
)
{
    HRESULT hr = S_OK;
    auto lock = m_CritSec.lock();

    if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpointControl->Reset())) {
        return hr;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////
//
// Creates a pre-load data buffer used by the stream render and capture routines.
//
// If the current endpoint is a render one, the data buffer consists of a sine tone
// wich gets loaded into a stream render routine.
//
// In the case of a capture endpoint, the pre-load data consists of silence.
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT
CHalfApp::CreateSineToneDataBuffer
(
    WAVEFORMATEX* pWfx
)
{
    HRESULT hr = S_OK;

    if (!VERIFY_IS_NOT_NULL(pWfx)) {
        hr = E_FAIL;
        return hr;
    }

    m_dwSineToneDataBufferSize = (DWORD)(((ULONGLONG)BUF_LEN_IN_MS * pWfx->nSamplesPerSec / 1000) * pWfx->nBlockAlign);
    m_pbSineToneDataBuffer.reset((BYTE*)LocalAlloc(LPTR, m_dwSineToneDataBufferSize));
    if (!VERIFY_IS_NOT_NULL(m_pbSineToneDataBuffer)) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    if (m_DataFlow == render) // Sine tone for render
    {
        if (!VERIFY_SUCCEEDED(hr = FillBufferWithSineSignal(
            g_pBasicLog, XFAIL,
            pWfx,
            TEST_AMPLITUDE, // amplitude
            TEST_FREQUENCY, // frequency
            0.0, // initial phase,
            0.0, // dc
            Method_NoDithering,
            m_pbSineToneDataBuffer.get(),
            m_dwSineToneDataBufferSize / pWfx->nBlockAlign,
            m_dwSineToneDataBufferSize
        ))) {
            return hr;
        }

        m_dwSineToneDataBufferPosition = 0;
    }
    else // Silence for capture
    {
        memset(m_pbSineToneDataBuffer.get(), 0, m_dwSineToneDataBufferSize);
        m_dwSineToneDataBufferPosition = 0;
    }

    return hr;
}

//////////////////////////////////
//
// Clears the current data buffer.
//
//////////////////////////////////
HRESULT
CHalfApp::ReleaseSineToneDataBuffer
(
    void
)
{
    m_pbSineToneDataBuffer = nullptr;
    m_dwSineToneDataBufferSize = 0;
    m_dwSineToneDataBufferPosition = 0;

    return S_OK;
}







































HRESULT
CHalfApp::InitializeAndSetBuffer
(
    HNSTIME requestedPeriodicity,
    UINT32 u32LatencyCoefficient
)
{
    HRESULT hr = S_OK;
    BOOL bIsEventCapable = false;

    // If no requested periodicity is provided, use default.
    if (requestedPeriodicity == 0) {
        m_hnsPeriod = FRAMES_TO_HNSTIME_DOUBLE(m_u32CurrentDefaultPeriodicityInFrames, m_pCurrentFormat->nSamplesPerSec);
    }
    else {
        m_hnsPeriod = requestedPeriodicity;
    }

    if (m_ConnectorType == eOffloadConnector) {
        bIsEventCapable = true;
    }
    else {
        if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint->GetEventDrivenCapable(&bIsEventCapable))) {
            return hr;
        }
    }

    if (bIsEventCapable)
    {
        if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpoint->SetStreamFlags(AUDCLNT_STREAMFLAGS_EVENTCALLBACK))) {
            return hr;
        }
    }

    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint->SetBuffer(m_hnsPeriod, u32LatencyCoefficient))) {
        return hr;
    }

    return hr;
}

HRESULT
CHalfApp::GetCurrentAvailiablePinInstanceCount
(
    UINT32 *pAvailablePinInstanceCount
)
{
    HRESULT                         hr = S_OK;

    // Read the current pin instance counts
    if (!VERIFY_SUCCEEDED(hr = GetAvailiablePinInstanceCount(m_pDevice.get(), m_uConnectorId, pAvailablePinInstanceCount))) {
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// Gets a CHalfApp instance based on the current one, but with a different processing mode.
//
// The processing mode for the new instance is specified through the secondMode parameter.
//
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT
CHalfApp::GetSecondHalfApp
(
    AUDIO_SIGNALPROCESSINGMODE secondMode,
    CHalfApp** ppSecondHalfApp
)
{
    HRESULT                                     hr = S_OK;
    ULONG                                       cFormatRecords = 0;
    CComHeapPtr<FORMAT_RECORD>                  spFormatRecords;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX>     pPreferredFormat;
    UINT32                                      u32DefaultPeriodicityInFrames;
    UINT32                                      u32FundamentalPeriodicityInFrames;
    UINT32                                      u32MinPeriodicityInFrames;
    UINT32                                      u32MaxPeriodicityInFrames;
    UINT32                                      u32MaxPeriodicityInFramesExtended;
    DeviceDescriptor                            descriptor = { 0 };

    if (!VERIFY_SUCCEEDED(hr = GetSupportedFormatRecordsForConnector(m_pDevice.get(), m_uConnectorId, m_ConnectorType, secondMode, m_DataFlow, &cFormatRecords, &spFormatRecords))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = GetPreferredFormatForConnector(m_pDevice.get(), m_uConnectorId, m_ConnectorType, secondMode, wil::out_param(pPreferredFormat)))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = GetPreferredFormatPeriodicityCharacteristicsForConnector(m_pDevice.get(), m_ConnectorType, secondMode, m_DataFlow, pPreferredFormat.get(), cFormatRecords, spFormatRecords, &u32DefaultPeriodicityInFrames, &u32FundamentalPeriodicityInFrames, &u32MinPeriodicityInFrames, &u32MaxPeriodicityInFrames, &u32MaxPeriodicityInFramesExtended))) {
        return hr;
    }

    descriptor.pDevice = m_pDevice.get();
    descriptor.pwstrAudioEndpointId = m_pwstrDeviceId.get();
    descriptor.pwstrAudioEndpointFriendlyName = m_pwstrDeviceFriendlyName.get();
    descriptor.dataFlow = m_DataFlow;
    descriptor.eConnectorType = m_ConnectorType;
    descriptor.uConnectorId = m_uConnectorId;
    descriptor.mode = secondMode;
    descriptor.cModes = m_cModes;
    descriptor.pModes = m_pModes;
    descriptor.cFormatRecords = cFormatRecords;
    descriptor.pFormatRecords = spFormatRecords;
    descriptor.pPreferredFormat = pPreferredFormat.get();
    descriptor.u32DefaultPeriodicityInFrames = u32DefaultPeriodicityInFrames;
    descriptor.u32FundamentalPeriodicityInFrames = u32FundamentalPeriodicityInFrames;
    descriptor.u32MinPeriodicityInFrames = u32MinPeriodicityInFrames;
    descriptor.u32MaxPeriodicityInFrames = u32MaxPeriodicityInFrames;
    descriptor.bIsAVStream = m_bIsAVStream;
    descriptor.bIsBluetooth = m_bIsBluetooth;
    descriptor.bIsSideband = m_bIsSideband;
    *ppSecondHalfApp = new CHalfApp(descriptor);
    if (*ppSecondHalfApp == NULL) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets a CHalfApp instance corresponding to the host pin of the current audio endpoint.
//
// This function is usually called on a test method that involves a loopback pin,
// in order to be able to start and control a stream in the host pin and get loopback data.
//
////////////////////////////////////////////////////////////////////////////////////////////
HRESULT
CHalfApp::GetHostHalfApp
(
    CHalfApp** ppHostHalfApp
)
{
    HRESULT                                     hr = S_OK;
    bool                                        bHasConnector = false;
    UINT                                        uConnectorId;
    ULONG                                       cModes = 0;
    CComHeapPtr<AUDIO_SIGNALPROCESSINGMODE>     spModes;
    bool                                        bRawSupport = false;
    bool                                        bDefaultSupport = false;
    AUDIO_SIGNALPROCESSINGMODE                  mode;
    ULONG                                       cFormatRecords = 0;
    CComHeapPtr<FORMAT_RECORD>                  spFormatRecords;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX>     pPreferredFormat;
    UINT32                                      u32DefaultPeriodicityInFrames;
    UINT32                                      u32FundamentalPeriodicityInFrames;
    UINT32                                      u32MinPeriodicityInFrames;
    UINT32                                      u32MaxPeriodicityInFrames;
    UINT32                                      u32MaxPeriodicityInFramesExtended;
    DeviceDescriptor                            descriptor = { 0 };

    // It is only used when testing on loopback pin. When preparing loopback pin, we also need prepare the host pin.
    VERIFY_IS_TRUE(m_ConnectorType == eLoopbackConnector);

    // Get host connector id
    if (!VERIFY_SUCCEEDED(hr = GetConnectorId(m_pDevice.get(), eHostProcessConnector, &bHasConnector, &uConnectorId))) {
        return hr;
    }
    if (!bHasConnector) {
        return hr;
    }

    // Get all signal processing modes for host connector
    if (!VERIFY_SUCCEEDED(hr = GetProcessingModesForConnector(m_pDevice.get(), uConnectorId, eHostProcessConnector, &cModes, &spModes))) {
        return hr;
    }

    // It is possible that host pin support multiple processing modes so we pick up one mode. If host pin support default, use default mode. If host pin support raw, use raw mode.
    for (ULONG i = 0; i < cModes; i++) {
        if (spModes[i] == AUDIO_SIGNALPROCESSINGMODE_RAW)
            bRawSupport = true;
        if (spModes[i] == AUDIO_SIGNALPROCESSINGMODE_DEFAULT)
            bDefaultSupport = true;
    }
    if (!VERIFY_IS_TRUE(bRawSupport || bDefaultSupport)) {
        hr = E_FAIL;
        return hr;
    }
    mode = bDefaultSupport ? AUDIO_SIGNALPROCESSINGMODE_DEFAULT : AUDIO_SIGNALPROCESSINGMODE_RAW;

    if (!VERIFY_SUCCEEDED(hr = GetSupportedFormatRecordsForConnector(m_pDevice.get(), uConnectorId, eHostProcessConnector, mode, render, &cFormatRecords, &spFormatRecords))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = GetPreferredFormatForConnector(m_pDevice.get(), uConnectorId, eHostProcessConnector, mode, wil::out_param(pPreferredFormat)))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = GetPreferredFormatPeriodicityCharacteristicsForConnector(m_pDevice.get(), eHostProcessConnector, mode, render, pPreferredFormat.get(), cFormatRecords, spFormatRecords, &u32DefaultPeriodicityInFrames, &u32FundamentalPeriodicityInFrames, &u32MinPeriodicityInFrames, &u32MaxPeriodicityInFrames, &u32MaxPeriodicityInFramesExtended))) {
        return hr;
    }

    descriptor.pDevice = m_pDevice.get();
    descriptor.pwstrAudioEndpointId = m_pwstrDeviceId.get();
    descriptor.pwstrAudioEndpointFriendlyName = m_pwstrDeviceFriendlyName.get();
    descriptor.dataFlow = render; 
    descriptor.eConnectorType = eHostProcessConnector;
    descriptor.uConnectorId = uConnectorId;
    descriptor.mode = mode;
    descriptor.cModes = cModes;
    descriptor.pModes = spModes;
    descriptor.cFormatRecords = cFormatRecords;
    descriptor.pFormatRecords = spFormatRecords;
    descriptor.pPreferredFormat = pPreferredFormat.get();
    descriptor.u32DefaultPeriodicityInFrames = u32DefaultPeriodicityInFrames;
    descriptor.u32FundamentalPeriodicityInFrames = u32FundamentalPeriodicityInFrames;
    descriptor.u32MinPeriodicityInFrames = u32MinPeriodicityInFrames;
    descriptor.u32MaxPeriodicityInFrames = u32MaxPeriodicityInFrames;
    descriptor.bIsAVStream = m_bIsAVStream;
    descriptor.bIsBluetooth = m_bIsBluetooth;
    descriptor.bIsSideband = m_bIsSideband;
    *ppHostHalfApp = new CHalfApp(descriptor);
    if (*ppHostHalfApp == NULL) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    return hr;
}


HRESULT
CHalfApp::InitializeStream
(
    HNSTIME requestedPeriodicity,
    UINT32 u32LatencyCoefficient
)
{
    HRESULT hr = S_OK;
    BOOL    bIsEventCapable;
    wil::unique_cotaskmem_ptr<WAVEFORMATEX> pDeviceFormat;

    if (!VERIFY_IS_TRUE(!m_bStreamInitialized)) {
        hr = AEERR_ALREADY_INITIALIZED;
    }

    if (!VERIFY_SUCCEEDED(hr = InitializeAndSetBuffer(requestedPeriodicity, u32LatencyCoefficient))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpoint->GetFrameFormat(out_param(pDeviceFormat)))) {
        return hr;
    }
    m_f32EndpointFrameRate = (FLOAT32)pDeviceFormat->nAvgBytesPerSec / (FLOAT32)pDeviceFormat->nBlockAlign;
    if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpoint->GetLatency(&m_hnsEndpointLatency))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = m_hProcessThreadStartedEvent.create())) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = m_hTerminate.create(wil::EventOptions::ManualReset))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = m_pAudioDeviceEndpoint->GetEventDrivenCapable(&bIsEventCapable))) {
        return hr;
    }
   
    if (bIsEventCapable || m_ConnectorType == eOffloadConnector) {
        m_bIsEventCapable = true;
    }
  
    if (m_bIsEventCapable)
    {
        if (!VERIFY_SUCCEEDED(hr = m_hEndpointBufferCompleteEvent.create())) {
            return hr;
        }
        if (!VERIFY_SUCCEEDED(hr = m_pAudioEndpoint->SetEventHandle(m_hEndpointBufferCompleteEvent.get()))) {
            return hr;
        }
    }

    if (!m_bIsEventCapable) {
        m_hTimer.reset(CreateWaitableTimerEx(NULL, NULL, 0, TIMER_ALL_ACCESS));
    }

    if (m_DataFlow == render) {
        m_pStreamRoutine = RenderStreamRoutine;
    }
    else {
        m_pStreamRoutine = CaptureStreamRoutine;
    }

    // To-do: Adjust for offload streaming

    m_bStreamInitialized = true;

    return hr;
}

HRESULT
CHalfApp::CleanupStream
(
    void
)
{   
    HRESULT hr = S_OK;

    // If the thread thread is still active, stop it
    if (m_hStreamThread)
    {
        SignalAndWaitForThread();
    }

    m_hnsPeriod = 0;
    m_f32EndpointFrameRate = 0.F;
    m_hnsEndpointLatency = 0;

    m_pStreamRoutine = nullptr;

    m_hTerminate.reset();
    m_hEndpointBufferCompleteEvent.reset();
    m_hProcessThreadStartedEvent.reset();
    m_hTimer.reset();

    m_bIsEventCapable = false;
    m_bStreamInitialized = false;
    m_bStreamThreadTerminate = false;
    m_bYieldActive = false;
  
    return hr;
}

DWORD CALLBACK
CHalfApp::RenderStreamRoutine
(
    PVOID lpParameter
)











































































































DWORD CALLBACK
CHalfApp::CaptureStreamRoutine
(
    PVOID lpParameter
)


















































































HRESULT
CHalfApp::StartStream
(
    void
)
{
    HRESULT hr = S_OK;

    if (!VERIFY_IS_TRUE(m_bStreamInitialized)) {
        hr = AEERR_NOT_INITIALIZED;
        return hr;
    }
    if (!VERIFY_IS_TRUE(!m_hStreamThread)) {
        hr = AEERR_ALREADY_RUNNING;
        return hr;
    }

    m_bStreamThreadTerminate = false;
    m_hTerminate.ResetEvent();
    m_hProcessThreadStartedEvent.ResetEvent();

    DWORD   dwThreadID;
    m_hStreamThread.reset(CreateThread(NULL, 0, m_pStreamRoutine, this, 0, &dwThreadID));
    if (!VERIFY_IS_NOT_NULL(m_hStreamThread)) {
        DWORD   dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        return hr;
    }
    if (m_hStreamThread)
    {
        SetThreadPriority(m_hStreamThread.get(), THREAD_PRIORITY_HIGHEST);
    }

    // Wait for the processing thread to be started or to error out. To avoid the Start call from being blocked forever, give up waiting after 10 seconds, which is a duration quite unlikely to hit -- something must have gone wrong in the thread creation.
    WaitForSingleObjectEx(m_hProcessThreadStartedEvent.get(), 10000, FALSE);
   
    //  Set timer
    if (!m_bIsEventCapable) {
        SetTimer(m_hTimer.get(), m_hnsPeriod, true);
    }
    
    if (!VERIFY_SUCCEEDED(hr = StartEndpoint())) {
        return hr;
    }

    return hr;
}

HRESULT
CHalfApp::StopStream
(
    void
)
{
    HRESULT hr = S_OK;

    m_bStreamThreadTerminate = true;

    if (!VERIFY_IS_TRUE(m_bStreamInitialized)) {
        hr = AEERR_NOT_INITIALIZED;
        return hr;
    }
    if (!VERIFY_IS_NOT_NULL(m_hStreamThread)) {
        hr = AEERR_NOT_RUNNING;
        return hr;
    }

    SignalAndWaitForThread();

    //Cancel timer
    if (!m_bIsEventCapable) {
        CancelTimer(m_hTimer.get());
    }

    if (!VERIFY_SUCCEEDED(hr = StopEndpoint())) {
        return hr;
    }

    return hr;
}

void
CHalfApp::SignalAndWaitForThread
(
    void
)
{
    DWORD dwReturn;

    m_bStreamThreadTerminate = true;
    m_hTerminate.SetEvent();

    dwReturn = WaitForSingleObjectEx(m_hStreamThread.get(), INFINITE, FALSE);

    VERIFY_IS_TRUE(WAIT_OBJECT_0 == dwReturn);

    m_hStreamThread.reset();
}































































inline HRESULT CHalfApp::SetTimer(HANDLE timer, HNSTIME timeDuration, bool fireImmediatley)
{
    if (timer)
    {
        LARGE_INTEGER DueTime;

        if (fireImmediatley)
        {
            DueTime.QuadPart = 0;
        }
        else
        {
            // Negate this to mean relative in call to SetWaitableTimer
            DueTime.QuadPart = -timeDuration;
        }

        SetWaitableTimer(timer, &DueTime,
            static_cast<UINT32>(MF_TO_MS(timeDuration)),
            nullptr, nullptr, FALSE);
    }
    return S_OK;
}

inline void CHalfApp::CancelTimer(HANDLE timer)
{
    if (timer)
    {
        // Cancel the waitable timer 
        CancelWaitableTimer(timer);

        // There is a race condition where the timer could have fired before it was canceled, which would leave the handle set to fire the next time events are waited upon.  CancelWaitableTimer will not reset the state of the handle so
        // wait for 0 ms for the timer to fire to clear it.
        WaitForSingleObject(timer, 0);
    }
}

HRESULT CHalfApp::GetPosition(UINT64* pu64Position, UINT64* pu64hnsQPCPosition)
{
    HRESULT hr = S_OK;
    auto lock = m_CritSec.lock();

    if (!VERIFY_SUCCEEDED(m_pAudioClock->GetPosition(pu64Position, pu64hnsQPCPosition))) {
        return hr;
    }

    return S_OK;
}
