#pragma once

#define HNSTIME_PER_MILLISECOND 10000

class CStreamEngine
{
public:
    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Run();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Pause();
    
    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AssignDrmContentId(
        _In_ ULONG          DrmContentId,
        _In_ PACXDRMRIGHTS  DrmRights
        );

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    GetHWLatency(
        _Out_   ULONG         * FifoSize, 
        _Out_   ULONG         * Delay
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CStreamEngine(
        _In_    ACXSTREAM       Stream, 
        _In_    ACXDATAFORMAT   StreamFormat
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    virtual
    #pragma code_seg()
    ~CStreamEngine();

protected:
    ACX_STREAM_STATE    m_CurrentState;
    ACXSTREAM           m_Stream;
    ACXDATAFORMAT       m_StreamFormat;
    LARGE_INTEGER       m_PerformanceCounterFrequency;
};

class CRenderStreamEngine : public CStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CRenderStreamEngine(
        _In_    ACXSTREAM       Stream,
        _In_    ACXDATAFORMAT   StreamFormat
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    ~CRenderStreamEngine();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();

protected:
    // data section.
};

class CCaptureStreamEngine : public CStreamEngine
{
public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    CCaptureStreamEngine(
        _In_    ACXSTREAM       Stream,
        _In_    ACXDATAFORMAT   StreamFormat
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    ~CCaptureStreamEngine();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware();

    virtual
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReleaseHardware();

protected:
    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReadRegistrySettings();
};

