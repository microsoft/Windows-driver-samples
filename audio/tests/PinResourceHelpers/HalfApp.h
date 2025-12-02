// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// File Name:
//
//  HalfApp.h
//
// Abstract:
//
//  CHalfApp declarations
//
// -------------------------------------------------------------------------------
#pragma once

#define BUF_LEN_IN_MS   1000 // 1 second

// ------------------------------------------------------------------------------
// Data flow
enum STACKWISE_DATAFLOW { render, capture };


// ------------------------------------------------------------------------------
// Structs used to hold results of format records
typedef struct
{
    WAVEFORMATEXTENSIBLE wfxEx;
    UINT32 fundamentalPeriodInFrames;
    UINT32 defaultPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;
    UINT32 maxPeriodInFramesExtended;
} FORMAT_RECORD, * PFORMAT_RECORD;

struct DeviceDescriptor
{
    IMMDevice*              pDevice;
    LPWSTR                  pwstrAudioEndpointId;
    LPWSTR                  pwstrAudioEndpointFriendlyName;
    STACKWISE_DATAFLOW      dataFlow;
    EndpointConnectorType   eConnectorType;
    UINT                    uConnectorId;
    GUID                    mode;
    ULONG                   cModes;
    GUID*                   pModes;
    ULONG                   cFormatRecords;
    PFORMAT_RECORD          pFormatRecords;
    PWAVEFORMATEX           pPreferredFormat;
    UINT32                  u32DefaultPeriodicityInFrames;
    UINT32                  u32FundamentalPeriodicityInFrames;
    UINT32                  u32MinPeriodicityInFrames;
    UINT32                  u32MaxPeriodicityInFrames;
    bool                    bIsPortCls;
    bool                    bIsAVStream;
    bool                    bIsBluetooth;
    bool                    bIsSideband;
    bool                    bIsMVA;
};

class CHalfApp
{
public:
    // Basic pin infos
    wil::com_ptr_nothrow<IMMDevice>             m_pDevice;
    wil::unique_cotaskmem_string                m_pwstrDeviceId;
    wil::unique_cotaskmem_string                m_pwstrDeviceFriendlyName;
    STACKWISE_DATAFLOW                          m_DataFlow;
    EndpointConnectorType                       m_ConnectorType;
    UINT                                        m_uConnectorId;
    GUID                                        m_Mode;
    ULONG                                       m_cModes;
    GUID*                                       m_pModes; // All processing modes
    bool                                        m_bIsPortCls;
    bool                                        m_bIsAVStream;
    bool                                        m_bIsBluetooth;
    bool                                        m_bIsSideband;
    bool                                        m_bIsMVA;

    // Formats
    wil::unique_cotaskmem_ptr<WAVEFORMATEX>     m_pPreferredFormat; 
    UINT32                                      m_u32DefaultPeriodicityInFrames;
    UINT32                                      m_u32FundamentalPeriodicityInFrames;
    UINT32                                      m_u32MinPeriodicityInFrames;
    UINT32                                      m_u32MaxPeriodicityInFrames;

    PFORMAT_RECORD                              m_pFormatRecords;
    ULONG                                       m_cFormatRecords;

    wil::unique_cotaskmem_ptr<WAVEFORMATEX>     m_pCurrentFormat; 
    UINT32                                      m_u32CurrentDefaultPeriodicityInFrames;
    UINT32                                      m_u32CurrentFundamentalPeriodicityInFrames;
    UINT32                                      m_u32CurrentMinPeriodicityInFrames;
    UINT32                                      m_u32CurrentMaxPeriodicityInFrames;

    // Audio endpoint interfaces
    wil::com_ptr_nothrow<IAudioDeviceEndpoint>  m_pAudioDeviceEndpoint;
    wil::com_ptr_nothrow<IAudioEndpoint>        m_pAudioEndpoint;
    wil::com_ptr_nothrow<IAudioEndpointControl> m_pAudioEndpointControl;
    wil::com_ptr_nothrow<IAudioEndpointRT>      m_pAudioEndpointRT;
    wil::com_ptr_nothrow<IAudioClock>           m_pAudioClock;

    // Data buffer
    wil::unique_hlocal_ptr<BYTE>                m_pbSineToneDataBuffer;
    DWORD                                       m_dwSineToneDataBufferSize;
    DWORD                                       m_dwSineToneDataBufferPosition;

    // Stream
    HNSTIME                                     m_hnsPeriod = 0;
    FLOAT32                                     m_f32EndpointFrameRate = 0.F;
    HNSTIME                                     m_hnsEndpointLatency = 0;
    bool                                        m_bIsEventCapable = false;
    bool                                        m_bStreamInitialized = false;
    volatile bool                               m_bStreamThreadTerminate = false;
    bool                                        m_bYieldActive = false;
    bool                                        m_bIsBackupTimerRequired = true;

    wil::unique_handle                          m_hStreamThread;

    LPTHREAD_START_ROUTINE                      m_pStreamRoutine = nullptr;
    wil::unique_event_nothrow                   m_hProcessThreadStartedEvent;
    wil::unique_event_nothrow                   m_hTerminate;
    wil::unique_event_nothrow                   m_hEndpointBufferCompleteEvent;
    wil::unique_handle                          m_hTimer;

    wil::critical_section                       m_CritSec;

    typedef enum
    {
        ET_DO_NOTHING = 0x00,
        ET_TERMINATE = 0x01,
        ET_OBJECT_BUFFER_COMPLETE = 0x02,
        ET_CLIENT_RELEASE_BUFFER = 0x04,
        ET_TIMER = 0x08,
        ET_PAUSE_PUMP = 0x10,
        ET_RESUME_PUMP = 0x20,
    } TEventType;

    CHalfApp(
        _In_ DeviceDescriptor descriptor
    )
        : m_DataFlow(descriptor.dataFlow)
        , m_ConnectorType(descriptor.eConnectorType)
        , m_uConnectorId(descriptor.uConnectorId)
        , m_cModes(descriptor.cModes)
        , m_cFormatRecords(descriptor.cFormatRecords)
        , m_u32DefaultPeriodicityInFrames(descriptor.u32DefaultPeriodicityInFrames)
        , m_u32FundamentalPeriodicityInFrames(descriptor.u32FundamentalPeriodicityInFrames)
        , m_u32MinPeriodicityInFrames(descriptor.u32MinPeriodicityInFrames)
        , m_u32MaxPeriodicityInFrames(descriptor.u32MaxPeriodicityInFrames)
        , m_u32CurrentDefaultPeriodicityInFrames(descriptor.u32DefaultPeriodicityInFrames)
        , m_u32CurrentFundamentalPeriodicityInFrames(descriptor.u32FundamentalPeriodicityInFrames)
        , m_u32CurrentMinPeriodicityInFrames(descriptor.u32MinPeriodicityInFrames)
        , m_u32CurrentMaxPeriodicityInFrames(descriptor.u32MaxPeriodicityInFrames)
        , m_bIsPortCls(descriptor.bIsPortCls)
        , m_bIsAVStream(descriptor.bIsAVStream)
        , m_bIsBluetooth(descriptor.bIsBluetooth)
        , m_bIsSideband(descriptor.bIsSideband)
        , m_bIsMVA(descriptor.bIsMVA)
    {
        m_pDevice = descriptor.pDevice;
        m_pAudioDeviceEndpoint = nullptr;
        m_pAudioEndpointControl = nullptr;

        m_pwstrDeviceId = wil::make_cotaskmem_string_nothrow(descriptor.pwstrAudioEndpointId, wcslen(descriptor.pwstrAudioEndpointId));
        m_pwstrDeviceFriendlyName = wil::make_cotaskmem_string_nothrow(descriptor.pwstrAudioEndpointFriendlyName, wcslen(descriptor.pwstrAudioEndpointFriendlyName));
        m_Mode = descriptor.mode;

        m_pModes = new GUID[descriptor.cModes];
        memcpy(m_pModes, descriptor.pModes, sizeof(GUID)*(descriptor.cModes));
        m_pFormatRecords = new FORMAT_RECORD[descriptor.cFormatRecords];
        memcpy(m_pFormatRecords, descriptor.pFormatRecords, sizeof(FORMAT_RECORD)*(descriptor.cFormatRecords));
        
        CloneWaveFormat(descriptor.pPreferredFormat, wil::out_param(m_pPreferredFormat));
        CloneWaveFormat(descriptor.pPreferredFormat, wil::out_param(m_pCurrentFormat)); // Use preferred format as current format
    }

    ~CHalfApp(void)
    {
        if (m_pModes)
            delete[] m_pModes;
        if (m_pFormatRecords)
            delete[] m_pFormatRecords;
    }

    // Audio endpoint interfaces
    HRESULT    InitializeEndpoint();
    HRESULT    ReleaseEndpoint();
    HRESULT    StartEndpoint();
    HRESULT    StopEndpoint();
    HRESULT    ResetEndpoint();

    // Sine tone data buffer
    HRESULT    CreateSineToneDataBuffer(WAVEFORMATEX* pWfx);
    HRESULT    ReleaseSineToneDataBuffer();




    // Stream
    HRESULT    InitializeAndSetBuffer(HNSTIME hnsPeriod, UINT32 u32LatencyCoefficient);
    HRESULT    InitializeStream(HNSTIME requestedPeriodicity, UINT32 u32LatencyCoefficient);
    HRESULT    CleanupStream();
    HRESULT    StartStream();
    HRESULT    StopStream();
    static DWORD CALLBACK   RenderStreamRoutine(PVOID lpParameter);
    static DWORD CALLBACK   CaptureStreamRoutine(PVOID lpParameter);
    void        SignalAndWaitForThread();



    HRESULT SetTimer(HANDLE timer, HNSTIME timeDuration, bool fireImmediatley);
    void CancelTimer(HANDLE timer);
    HRESULT     GetPosition(UINT64* pu64Position, UINT64* pu64hnsQPCPosition);

    // Multiple pin instances
    HRESULT    GetCurrentAvailiablePinInstanceCount(UINT32* pAvailablePinInstanceCount);
    HRESULT    GetSecondHalfApp(AUDIO_SIGNALPROCESSINGMODE secondMode, CHalfApp** ppSecondHalfApp);

    // Only for loopback testcases
    HRESULT    GetHostHalfApp(CHalfApp** ppHostHalfApp);
};

// {571BF784-A9D6-420A-BCA0-C579A8710236}
DEFINE_GUID(IID_IHalfAppContainer,
    0x571bf784, 0xa9d6, 0x420a, 0xbc, 0xa0, 0xc5, 0x79, 0xa8, 071, 0x02, 0x36);

DECLARE_INTERFACE_IID_(
IHalfAppContainer,
IUnknown,
"571BF784-A9D6-420A-BCA0-C579A8710236")
{
    STDMETHOD(GetHalfApp)(THIS_ CHalfApp ** ppHalfApp) PURE;
};
