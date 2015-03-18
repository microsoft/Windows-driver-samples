// Mft0.h : Declaration of the CMft0

#pragma once
#include "resource.h"       // main symbols
#include "SampleMft0.h"
#include "SampleHelpers.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CMft0

class ATL_NO_VTABLE CMft0 :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CMft0, &CLSID_Mft0>,
    public IMft0, 
    public IMFTransform,
    public IInspectable
{
public:
    CMft0() :   m_pSample(NULL),
        m_pInputType(NULL),
        m_pOutputType(NULL),
        m_pInputAttributes(0),
        m_pGlobalAttributes(0),
        m_pSourceTransform(0),
        m_bEnableEffects(TRUE),
        m_uiStreamId(0),
        m_percentOfScreen(50),
        m_nRefCount(1)
    {
        InitializeCriticalSection(&m_critSec);
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_MFT0)

    DECLARE_NOT_AGGREGATABLE(CMft0)

    BEGIN_COM_MAP(CMft0)
        COM_INTERFACE_ENTRY(IMft0)
        COM_INTERFACE_ENTRY(IMFTransform)
        COM_INTERFACE_ENTRY(IInspectable)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct()
    {
        return S_OK;
    }

    void FinalRelease()
    {
    }

public:
    STDMETHOD(UpdateDsp)(UINT32 uiPercentOfScreen);
    STDMETHOD(Enable)(void);
    STDMETHOD(Disable)(void);
    STDMETHOD(GetDspSetting)(UINT* puiPercentOfScreen, BOOL* pIsEnabled);

    STDMETHODIMP GetIids( 
        /* [out] */ _Out_ ULONG *iidCount,
        /* [size_is][size_is][out] */ _Outptr_result_buffer_maybenull_(*iidCount) IID **iids);

    STDMETHODIMP GetRuntimeClassName( 
        /* [out] */ _Outptr_result_maybenull_ HSTRING *className);

    STDMETHODIMP GetTrustLevel( 
        /* [out] */ _Out_ TrustLevel *trustLevel);

    STDMETHODIMP GetStreamLimits( 
        /* [out] */ _Out_ DWORD *pdwInputMinimum,
        /* [out] */ _Out_ DWORD *pdwInputMaximum,
        /* [out] */ _Out_ DWORD *pdwOutputMinimum,
        /* [out] */ _Out_ DWORD *pdwOutputMaximum);

    STDMETHODIMP GetStreamCount( 
        /* [out] */ _Out_ DWORD *pcInputStreams,
        /* [out] */ _Out_ DWORD *pcOutputStreams);

    STDMETHODIMP GetStreamIDs( 
        DWORD dwInputIDArraySize,
        /* [size_is][out] */ _Out_writes_(dwInputIDArraySize) DWORD *pdwInputIDs,
        DWORD dwOutputIDArraySize,
        /* [size_is][out] */ _Out_writes_(dwOutputIDArraySize) DWORD *pdwOutputIDs);

    STDMETHODIMP GetInputStreamInfo( 
        DWORD dwInputStreamID,
        /* [out] */ _Out_ MFT_INPUT_STREAM_INFO *pStreamInfo);

    STDMETHODIMP GetOutputStreamInfo( 
        DWORD dwOutputStreamID,
        /* [out] */ _Out_ MFT_OUTPUT_STREAM_INFO *pStreamInfo);

    STDMETHODIMP GetAttributes( 
        /* [out] */ _Outptr_result_maybenull_ IMFAttributes **pAttributes);

    STDMETHODIMP GetInputStreamAttributes( 
        DWORD dwInputStreamID,
        /* [out] */ _Outptr_result_maybenull_ IMFAttributes **pAttributes);

    STDMETHODIMP GetOutputStreamAttributes( 
        DWORD dwOutputStreamID,
        /* [out] */ _Outptr_result_maybenull_ IMFAttributes **pAttributes);

    STDMETHODIMP DeleteInputStream( 
        DWORD dwStreamID);

    STDMETHODIMP AddInputStreams( 
        DWORD cStreams,
        /* [in] */ _In_ DWORD *adwStreamIDs);

    STDMETHODIMP GetInputAvailableType( 
        DWORD dwInputStreamID,
        DWORD dwTypeIndex,
        /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType);

    STDMETHODIMP GetOutputAvailableType( 
        DWORD dwOutputStreamID,
        DWORD dwTypeIndex,
        /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType);

    STDMETHODIMP SetInputType( 
        DWORD dwInputStreamID,
        /* [in] */ _In_opt_ IMFMediaType *pType,
        DWORD dwFlags);

    STDMETHODIMP SetOutputType( 
        DWORD dwOutputStreamID,
        /* [in] */ _In_opt_ IMFMediaType *pType,
        DWORD dwFlags);

    STDMETHODIMP GetInputCurrentType( 
        DWORD dwInputStreamID,
        /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType);

    STDMETHODIMP GetOutputCurrentType( 
        DWORD dwOutputStreamID,
        /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType);

    STDMETHODIMP GetInputStatus( 
        DWORD dwInputStreamID,
        /* [out] */ _Out_ DWORD *pdwFlags);

    STDMETHODIMP GetOutputStatus( 
        /* [out] */ _Out_ DWORD *pdwFlags);

    STDMETHODIMP SetOutputBounds( 
        LONGLONG hnsLowerBound,
        LONGLONG hnsUpperBound);

    STDMETHODIMP ProcessEvent( 
        DWORD dwInputStreamID,
        /* [in] */ _In_opt_ IMFMediaEvent *pEvent);

    STDMETHODIMP ProcessMessage( 
        MFT_MESSAGE_TYPE eMessage,
        ULONG_PTR ulParam);

    STDMETHODIMP ProcessInput( 
        DWORD dwInputStreamID,
        IMFSample *pSample,
        DWORD dwFlags);

    STDMETHODIMP ProcessOutput( 
        DWORD dwFlags,
        DWORD cOutputBufferCount,
        /* [size_is][out][in] */ MFT_OUTPUT_DATA_BUFFER *pOutputSamples,
        /* [out] */ DWORD *pdwStatus);

protected:
    virtual ~CMft0()
    {
        SAFERELEASE(m_pInputAttributes);
        SAFERELEASE(m_pGlobalAttributes);
        SAFERELEASE(m_pSourceTransform);
        SAFERELEASE(m_pInputType);
        SAFERELEASE(m_pOutputType);
        SAFERELEASE(m_pSample);
        DeleteCriticalSection(&m_critSec);
    }

    STDMETHODIMP GetMediaType(DWORD  dwStreamID, DWORD dwTypeIndex, IMFMediaType **ppType);
    STDMETHODIMP IsMediaTypeSupported(UINT uiStreamId, IMFMediaType *pIMFMediaType, IMFMediaType **ppIMFMediaTypeFull = NULL);
    STDMETHODIMP GenerateMFMediaTypeListFromDevice(UINT uiStreamId);

    // HasPendingOutput: Returns TRUE if the MFT is holding an input sample.
    BOOL HasPendingOutput() const { return m_pSample != NULL; }

    // IsValidInputStream: Returns TRUE if dwInputStreamID is a valid input stream identifier.
    BOOL IsValidInputStream(DWORD dwInputStreamID);

    // IsValidOutputStream: Returns TRUE if dwOutputStreamID is a valid output stream identifier.
    BOOL IsValidOutputStream(DWORD dwOutputStreamID);

    STDMETHODIMP GetDefaultStride(LONG *plStride);

    STDMETHODIMP OnProcessOutput(IMFMediaBuffer *pIn, IMFMediaBuffer *pOut);

    STDMETHODIMP OnFlush();

    CRITICAL_SECTION            m_critSec;

    IMFSample                   *m_pSample;                 // Input sample.
    IMFMediaType                *m_pInputType;              // Input media type.
    IMFMediaType                *m_pOutputType;             // Output media type.

    // Image transform function. (Changes based on the media type.)
    IMFAttributes               *m_pInputAttributes;
    IMFAttributes               *m_pGlobalAttributes;
    GUID                        m_stStreamType;
    IMFTransform                *m_pSourceTransform;
    volatile BOOL               m_bEnableEffects;
    UINT                        m_uiStreamId;
    UINT                        m_percentOfScreen;
    UINT                        m_nRefCount;
    CAtlMap<DWORD, CComPtr<IMFMediaType>> m_listOfMediaTypes;
};

OBJECT_ENTRY_AUTO(__uuidof(Mft0), CMft0)
