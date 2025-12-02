// Mft0.cpp : Implementation of CMft0

#include "stdafx.h"
#include "Mft0.h"
#include "SampleHelpers.h"
#include <WinString.h>

// CMft0
STDMETHODIMP CMft0::UpdateDsp(UINT32 uiPercentOfScreen)
{
    m_percentOfScreen = uiPercentOfScreen;
    return S_OK;
}

STDMETHODIMP CMft0::Enable(void)
{
    m_bEnableEffects = TRUE;
    return S_OK;
}

STDMETHODIMP CMft0::Disable(void)
{
    m_bEnableEffects = FALSE;
    return S_OK;
}

STDMETHODIMP CMft0::GetDspSetting(UINT* puiPercentOfScreen, BOOL* pIsEnabled)
{
    HRESULT hr = S_OK;
    do {
        CHK_NULL_PTR_BRK(puiPercentOfScreen);
        CHK_NULL_PTR_BRK(pIsEnabled);
        *puiPercentOfScreen = m_percentOfScreen;
        *pIsEnabled = m_bEnableEffects;
    } while (FALSE);

    return S_OK;
}

STDMETHODIMP CMft0::GetIids( 
    /* [out] */ _Out_ ULONG *iidCount,
    /* [size_is][size_is][out] */ _Outptr_result_buffer_maybenull_(*iidCount) IID **iids)
{
    HRESULT hr = S_OK;
    do {
        CHK_NULL_PTR_BRK(iidCount);
        CHK_NULL_PTR_BRK(iids);
        *iids = NULL;
        *iidCount = 0;
    } while (FALSE);

    return hr;
}

STDMETHODIMP CMft0::GetRuntimeClassName( 
    /* [out] */ _Outptr_result_maybenull_ HSTRING *className)
{
    HRESULT hr = S_OK;
    if(className != nullptr)
    {
        hr = WindowsCreateString(NULL, 0, className);
        if(FAILED(hr)){
            hr = E_OUTOFMEMORY;
        }
    }else {
        hr = E_INVALIDARG;
    }
    return  hr;
}

STDMETHODIMP CMft0::GetTrustLevel( 
    /* [out] */ _Out_ TrustLevel *trustLevel)
{
    HRESULT hr = S_OK;
    do {
        CHK_NULL_PTR_BRK(trustLevel);
        *trustLevel = TrustLevel::BaseTrust;
    } while (FALSE);

    return hr;
}


STDMETHODIMP CMft0::GetStreamLimits( 
    /* [out] */ _Out_ DWORD *pdwInputMinimum,
    /* [out] */ _Out_ DWORD *pdwInputMaximum,
    /* [out] */ _Out_ DWORD *pdwOutputMinimum,
    /* [out] */ _Out_ DWORD *pdwOutputMaximum)
{
    HRESULT hr = S_OK;
    do {
        if ((pdwInputMinimum == NULL) ||
            (pdwInputMaximum == NULL) ||
            (pdwOutputMinimum == NULL) ||
            (pdwOutputMaximum == NULL))
        {
            hr = E_POINTER;
            break;
        }

        // This MFT has a fixed number of streams.
        *pdwInputMinimum = 1;
        *pdwInputMaximum = 1;
        *pdwOutputMinimum = 1;
        *pdwOutputMaximum = 1;
    } while (FALSE);

    return hr;    
}


STDMETHODIMP CMft0::GetStreamCount( 
    /* [out] */ _Out_ DWORD *pcInputStreams,
    /* [out] */ _Out_ DWORD *pcOutputStreams)
{
    HRESULT hr = S_OK;

    do {
        if ((pcInputStreams == NULL) || (pcOutputStreams == NULL))

        {
            hr = E_POINTER;
            break;
        }

        // This MFT has a fixed number of streams.
        *pcInputStreams = 1;
        *pcOutputStreams = 1;
    } while (FALSE);

    return hr;
}
STDMETHODIMP CMft0::GetStreamIDs( 
    DWORD dwInputIDArraySize,
    /* [size_is][out] */ _Out_writes_(dwInputIDArraySize) DWORD *pdwInputIDs,
    DWORD dwOutputIDArraySize,
    /* [size_is][out] */ _Out_writes_(dwOutputIDArraySize) DWORD *pdwOutputIDs)
{
    dwOutputIDArraySize = 0;
    dwInputIDArraySize = 0;
    pdwInputIDs = NULL;
    pdwOutputIDs = NULL;
    return E_NOTIMPL; 
}

STDMETHODIMP CMft0::GetInputStreamInfo( 
    DWORD dwInputStreamID,
    /* [out] */ _Out_ MFT_INPUT_STREAM_INFO *pStreamInfo)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_critSec);
    
    do {
        CHK_NULL_BRK(pStreamInfo);
        if (!IsValidInputStream(dwInputStreamID))
        {
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        }
        if(m_pInputType) {
            pStreamInfo->cbAlignment = 0;
            pStreamInfo->cbSize = 0;
            pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER ;
            pStreamInfo->hnsMaxLatency = 0;
        } else {
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        }
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    return hr;
}


STDMETHODIMP CMft0::GetOutputStreamInfo( 
    DWORD dwOutputStreamID,
    /* [out] */ _Out_ MFT_OUTPUT_STREAM_INFO *pStreamInfo)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_critSec);

    do {
        CHK_NULL_BRK(pStreamInfo);
        if (!IsValidInputStream(dwOutputStreamID))
        {
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        }
        if(m_pOutputType) {
            pStreamInfo->cbAlignment = 0;
            pStreamInfo->cbSize = 0;
            pStreamInfo->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES  | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE; 
        } else {
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        }

    } while (FALSE);
    
    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::GetAttributes( 
    /* [out] */ _Outptr_result_maybenull_ IMFAttributes **ppAttributes)
{
    HRESULT hr = S_OK;

    do {
        CHK_NULL_PTR_BRK(ppAttributes);
        if(!m_pGlobalAttributes) {
            CHK_LOG_BRK(MFCreateAttributes(&m_pGlobalAttributes, 3));
            CHK_LOG_BRK(m_pGlobalAttributes->SetUINT32(MF_TRANSFORM_ASYNC, FALSE));
            CHK_LOG_BRK(m_pGlobalAttributes->SetString(MFT_ENUM_HARDWARE_URL_Attribute, L"Sample_CameraExtensionMft"));
            CHK_LOG_BRK(m_pGlobalAttributes->SetUINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE));
        }
        *ppAttributes = m_pGlobalAttributes;
        (*ppAttributes)->AddRef();
    } while (FALSE);
    
    return hr;
}

STDMETHODIMP CMft0::GetInputStreamAttributes( 
    DWORD dwInputStreamID,
    /* [out] */ _Outptr_result_maybenull_ IMFAttributes **ppAttributes)
{
    HRESULT hr = S_OK;

    do {
        if(dwInputStreamID > 0) {
            hr = MF_E_INVALIDSTREAMNUMBER;
            break;
        }
        CHK_NULL_PTR_BRK(ppAttributes);
        if(!m_pInputAttributes){
            CHK_LOG_BRK(MFCreateAttributes(&m_pInputAttributes, 2));
            CHK_LOG_BRK(m_pInputAttributes->SetUINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE));
            CHK_LOG_BRK(m_pInputAttributes->SetString(MFT_ENUM_HARDWARE_URL_Attribute, L"Sample_CameraExtensionMft"));
        }
        *ppAttributes = m_pInputAttributes;
        (*ppAttributes)->AddRef();
    } while (FALSE);
    
    return hr;
}

STDMETHODIMP CMft0::GetOutputStreamAttributes( 
    DWORD dwOutputStreamID,
    /* [out] */ _Outptr_result_maybenull_ IMFAttributes **ppAttributes)
{
    HRESULT hr = S_OK;

    do {
        if(dwOutputStreamID > 0) {
            hr = MF_E_INVALIDSTREAMNUMBER;
            break;
        }
        CHK_NULL_PTR_BRK(ppAttributes);
        CHK_NULL_PTR_BRK(m_pInputAttributes);

        *ppAttributes = m_pInputAttributes;
        (*ppAttributes)->AddRef();
    } while (FALSE);
    
    return hr;
}

STDMETHODIMP CMft0::DeleteInputStream( 
    DWORD dwStreamID)
{
    HRESULT hr = S_OK;
    if(dwStreamID > 0) {
        hr = MF_E_INVALIDSTREAMNUMBER;
    } else {
        hr = E_NOTIMPL;
    }
    return hr; 
}

STDMETHODIMP CMft0::AddInputStreams( 
    DWORD cStreams,
    /* [in] */ _In_ DWORD *adwStreamIDs)
{
    HRESULT hr = S_OK;
    
    if( !adwStreamIDs || cStreams > 0) {
        hr = E_INVALIDARG;
    } else {
        hr = E_NOTIMPL;
    }
    return hr; 
}

STDMETHODIMP CMft0::GetInputAvailableType( 
    DWORD dwInputStreamID,
    DWORD dwTypeIndex,
    /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType)
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    IMFAttributes *pSourceAttributes = NULL;
    UINT32 uiSourceStreamId = 0;
    wchar_t *pszName = NULL;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_BOOL_BRK(IsValidInputStream(dwInputStreamID));

        if(!m_pSourceTransform && m_pInputAttributes) {
            CHK_LOG_BRK(m_pInputAttributes->GetUnknown(MFT_CONNECTED_STREAM_ATTRIBUTE, IID_PPV_ARGS(&pSourceAttributes)));
            CHK_LOG_BRK(pSourceAttributes->GetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT, IID_PPV_ARGS(&pUnk)));
            CHK_LOG_BRK(pUnk->QueryInterface(__uuidof(IMFTransform), (void**)&m_pSourceTransform));
            CHK_LOG_BRK(pSourceAttributes->GetGUID( MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType));
            CHK_LOG_BRK(pSourceAttributes->GetUINT32( MF_DEVICESTREAM_STREAM_ID, &uiSourceStreamId));

            if(m_stStreamType == PINNAME_VIDEO_CAPTURE) {
                wprintf(L"Stream type: PINNAME_VIDEO_CAPTURE\n");
            } else if(m_stStreamType == PINNAME_VIDEO_PREVIEW) {
                wprintf(L"Stream type: PINNAME_VIDEO_PREVIEW\n");
            } else if(m_stStreamType == PINNAME_VIDEO_STILL) {
                wprintf(L"Stream type: PINNAME_VIDEO_STILL\n");
            } else if(m_stStreamType == PINNAME_IMAGE) {
                wprintf(L"Stream type: PINNAME_IMAGE\n");
            } else {
                if(SUCCEEDED(StringFromCLSID(m_stStreamType, &pszName)) && pszName){
                    wprintf(L"Stream type: %s\n", pszName);
                    CoTaskMemFree(pszName);
                }
            }
            CHK_LOG_BRK((m_stStreamType == PINNAME_VIDEO_PREVIEW || m_stStreamType == PINNAME_VIDEO_CAPTURE) ? S_OK : E_UNEXPECTED);
            CHK_LOG_BRK(GenerateMFMediaTypeListFromDevice(uiSourceStreamId));
        }

        CHK_LOG_BRK(GetMediaType(dwInputStreamID, dwTypeIndex, ppType));
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    SAFERELEASE(pUnk);
    SAFERELEASE(pSourceAttributes);
    return hr;
}

STDMETHODIMP CMft0::GetOutputAvailableType( 
    DWORD dwOutputStreamID,
    DWORD dwTypeIndex,
    /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType)
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    IMFAttributes *pSourceAttributes = NULL;
    UINT32 uiSourceStreamId = 0;
    wchar_t *pszName = NULL;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_BOOL_BRK(IsValidOutputStream(dwOutputStreamID));
        if(!m_pSourceTransform && m_pInputAttributes) {
            CHK_LOG_BRK(m_pInputAttributes->GetUnknown(MFT_CONNECTED_STREAM_ATTRIBUTE, IID_PPV_ARGS(&pSourceAttributes)));
            CHK_LOG_BRK(pSourceAttributes->GetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT, IID_PPV_ARGS(&pUnk)));
            CHK_LOG_BRK(pUnk->QueryInterface(__uuidof(IMFTransform), (void**)&m_pSourceTransform));
            CHK_LOG_BRK(pSourceAttributes->GetGUID( MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType));
            CHK_LOG_BRK(pSourceAttributes->GetUINT32( MF_DEVICESTREAM_STREAM_ID, &uiSourceStreamId));
            if(m_stStreamType == PINNAME_VIDEO_CAPTURE) {
                wprintf(L"Stream type: PINNAME_VIDEO_CAPTURE\n");
            } else if(m_stStreamType == PINNAME_VIDEO_PREVIEW) {
                wprintf(L"Stream type: PINNAME_VIDEO_PREVIEW\n");
            } else if(m_stStreamType == PINNAME_VIDEO_STILL) {
                wprintf(L"Stream type: PINNAME_VIDEO_STILL\n");
            } else if(m_stStreamType == PINNAME_IMAGE) {
                wprintf(L"Stream type: PINNAME_IMAGE\n");
            } else {
                if (SUCCEEDED(StringFromCLSID(m_stStreamType, &pszName)) && pszName) {
                    wprintf(L"Stream type: %s\n", pszName);
                    CoTaskMemFree(pszName);
                }
            }
            CHK_LOG_BRK((m_stStreamType == PINNAME_VIDEO_PREVIEW || m_stStreamType == PINNAME_VIDEO_CAPTURE) ? S_OK : E_UNEXPECTED);
            CHK_LOG_BRK(GenerateMFMediaTypeListFromDevice(uiSourceStreamId));
        }

        CHK_LOG_BRK(GetMediaType(dwOutputStreamID, dwTypeIndex, ppType));
    } while (FALSE);
    
    LeaveCriticalSection(&m_critSec);
    SAFERELEASE(pUnk);
    SAFERELEASE(pSourceAttributes);
    return hr;
}

STDMETHODIMP CMft0::SetInputType( 
    DWORD dwInputStreamID,
    /* [in] */ _In_opt_ IMFMediaType *pType,
    DWORD dwFlags)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);
    // Validate flags.
    do {
        if(bReallySet) {
            CComPtr<IMFMediaType> pFullType;
            CHK_LOG_BRK(IsMediaTypeSupported(dwInputStreamID, pType, &pFullType));
            SAFERELEASE(m_pInputType);
            SAFERELEASE(m_pOutputType);
            m_pOutputType = pFullType;
            m_pOutputType->AddRef();
            m_pInputType = pFullType;
            m_pInputType->AddRef();
        } else {
            CHK_LOG_BRK(IsMediaTypeSupported(dwInputStreamID, pType));
        }
    } while(FALSE);
    
    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::SetOutputType( 
    DWORD dwOutputStreamID,
    /* [in] */ _In_opt_ IMFMediaType *pType,
    DWORD dwFlags)
{
    HRESULT hr = S_OK;

    do {
        CHK_LOG_BRK(SetInputType(dwOutputStreamID, pType, dwFlags));
    } while (FALSE);

    return hr;
}

STDMETHODIMP CMft0::GetInputCurrentType( 
    DWORD dwInputStreamID,
    /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_NULL_BRK(ppType);

        if (!IsValidInputStream(dwInputStreamID))
        {
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        }
        else if(m_pInputType)
        {
            *ppType = m_pInputType;
            (*ppType)->AddRef();
        }
        else 
        {
            CHK_LOG_BRK(MF_E_TRANSFORM_TYPE_NOT_SET);
        }
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::GetOutputCurrentType( 
    DWORD dwOutputStreamID,
    /* [out] */ _Outptr_result_maybenull_ IMFMediaType **ppType)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_NULL_BRK(ppType);

        if (!IsValidOutputStream(dwOutputStreamID))
        {
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        }
        else if(m_pOutputType) 
        {
            *ppType = m_pOutputType;
            (*ppType)->AddRef();
        }
        else
        {
            CHK_LOG_BRK(MF_E_TRANSFORM_TYPE_NOT_SET);
        }
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::GetInputStatus( 
    DWORD dwInputStreamID,
    /* [out] */ _Out_ DWORD *pdwFlags)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_BOOL_BRK(pdwFlags);

        if (!IsValidInputStream(dwInputStreamID))
        {
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        }

        // If we already have an input sample, we don't accept
        // another one until the client calls ProcessOutput or Flush.
        if (m_pSample == NULL)
        {
            *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
        }
        else
        {
            *pdwFlags = 0;
        }
    } while (FALSE);
    
    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::GetOutputStatus( 
    /* [out] */ _Out_ DWORD *pdwFlags)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_NULL_BRK(pdwFlags);
        // We can produce an output sample if (and only if)
        // we have an input sample.
        if (m_pSample != NULL)
        {
            *pdwFlags = MFT_OUTPUT_STATUS_SAMPLE_READY;
        }
        else
        {
            *pdwFlags = 0;
        }
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::SetOutputBounds( 
    LONGLONG hnsLowerBound,
    LONGLONG hnsUpperBound)
{
    UNREFERENCED_PARAMETER(hnsLowerBound);
    UNREFERENCED_PARAMETER(hnsUpperBound);
    return S_OK;
}

STDMETHODIMP CMft0::ProcessEvent( 
    DWORD dwInputStreamID,
    /* [in] */ _In_opt_ IMFMediaEvent *pEvent)
{
    UNREFERENCED_PARAMETER(dwInputStreamID);
    UNREFERENCED_PARAMETER(pEvent);
    return E_NOTIMPL;
}

STDMETHODIMP CMft0::ProcessMessage( 
    MFT_MESSAGE_TYPE eMessage,
    ULONG_PTR ulParam)
{
    UNREFERENCED_PARAMETER(ulParam);
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    switch (eMessage)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        // Flush the MFT.
        hr = OnFlush();
        break;

    case MFT_MESSAGE_COMMAND_DRAIN:
        // Drain: Tells the MFT not to accept any more input until
        // all of the pending output has been processed. That is our
        // default behevior already, so there is nothing to do.
        break;

    case MFT_MESSAGE_SET_D3D_MANAGER:
        // The pipeline should never send this message unless the MFT
        // has the MF_SA_D3D_AWARE attribute set to TRUE. However, if we
        // do get this message, it's invalid and we don't implement it.
        hr = E_NOTIMPL;
        break;

        // The remaining messages do not require any action from this MFT.
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        break;
    }

    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::ProcessInput( 
    DWORD dwInputStreamID,
    IMFSample *pSample,
    DWORD dwFlags)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_NULL_BRK(pSample);

        CHK_BOOL_BRK(dwFlags == 0)

            DWORD dwBufferCount = 0;

        if (!IsValidInputStream(dwInputStreamID))
        {
            CHK_LOG_BRK(hr = MF_E_INVALIDSTREAMNUMBER);
        }

        if (!m_pInputType || !m_pOutputType)
        {
            CHK_LOG_BRK(MF_E_NOTACCEPTING);   // Client must set input and output types.
        }

        if (m_pSample != NULL)
        {
            CHK_LOG_BRK(MF_E_NOTACCEPTING);   // We already have an input sample.
        }

        // Validate the number of buffers. There should only be a single buffer to hold the video frame.
        CHK_LOG_BRK(pSample->GetBufferCount(&dwBufferCount));

        if (dwBufferCount == 0)
        {
            CHK_LOG_BRK(E_FAIL);
        }
        if (dwBufferCount > 1)
        {
            CHK_LOG_BRK(MF_E_SAMPLE_HAS_TOO_MANY_BUFFERS);
        }

        // Cache the sample. We do the actual work in ProcessOutput.
        m_pSample = pSample;
        pSample->AddRef();  // Hold a reference count on the sample.
    } while (FALSE);

    LeaveCriticalSection(&m_critSec);
    return hr;
}

STDMETHODIMP CMft0::ProcessOutput( 
    DWORD dwFlags,
    DWORD cOutputBufferCount,
    /* [size_is][out][in] */ MFT_OUTPUT_DATA_BUFFER *pOutputSamples,
    /* [out] */ DWORD *pdwStatus)
{
    HRESULT hr = S_OK;

    IMFMediaBuffer *pMediaBufInput = NULL,
        *pMediaBufOutput = NULL;
    IMFSample *pOutputIMFSample = NULL;
    LONGLONG hnsDuration = 0,
        hnsTime = 0;

    EnterCriticalSection(&m_critSec);

    do {
        CHK_BOOL_BRK(dwFlags == 0);
        CHK_NULL_BRK(pOutputSamples);
        CHK_NULL_BRK(pdwStatus);

        // Must be exactly one output buffer.
        CHK_BOOL_BRK(cOutputBufferCount == 1);

        // If we don't have an input sample, we need some input before
        // we can generate any output.
        if (m_pSample == NULL)
        {
            hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
            break;
        }

        CHK_LOG_BRK(m_pSample->ConvertToContiguousBuffer(&pMediaBufInput));

        MFCreateSample(&pOutputIMFSample);

        if(!pOutputSamples[0].pSample){
            CHK_LOG_BRK(m_pSample->ConvertToContiguousBuffer(&pMediaBufOutput));

        } else {
            CHK_LOG_BRK(pOutputSamples[0].pSample->ConvertToContiguousBuffer(&pMediaBufOutput));
        }

        pOutputIMFSample->AddBuffer(pMediaBufOutput);

        CHK_LOG_BRK(OnProcessOutput(pMediaBufInput, pMediaBufOutput));
        pOutputSamples[0].pSample  =  pOutputIMFSample;
        pOutputIMFSample->AddRef();

        pOutputSamples[0].dwStatus = 0;
        *pdwStatus = 0;
        if (SUCCEEDED(m_pSample->GetSampleDuration(&hnsDuration)))
        {
            CHK_LOG_BRK(pOutputSamples[0].pSample->SetSampleDuration(hnsDuration));
        }

        if (SUCCEEDED(m_pSample->GetSampleTime(&hnsTime)))
        {
            CHK_LOG_BRK(pOutputSamples[0].pSample->SetSampleTime(hnsTime));
        }
    } while (FALSE);

    SAFERELEASE(pOutputIMFSample);
    SAFERELEASE(pMediaBufInput);
    SAFERELEASE(pMediaBufOutput);
    SAFERELEASE(m_pSample);   // Release our input sample.
    LeaveCriticalSection(&m_critSec);
    return hr;
}

BOOL CMft0::IsValidInputStream(DWORD dwInputStreamID)
{
    return dwInputStreamID == 0;
}

// IsValidOutputStream: Returns TRUE if dwOutputStreamID is a valid output stream identifier.
BOOL CMft0::IsValidOutputStream(DWORD dwOutputStreamID)
{
    //update
    return dwOutputStreamID == 0;
}


STDMETHODIMP CMft0::OnProcessOutput(IMFMediaBuffer *pIn, IMFMediaBuffer *pOut)
{
    HRESULT hr = S_OK;
    LONG lDefaultStride = 0,
        lSrcStride = 0,
        lDestStride = 0;
    UINT uiWidth = 0,
         uiHeight = 0;
    BYTE *pDest = NULL,
        *pSrc = NULL;
    //BOOL bCompressed = TRUE;
    GUID stSubType = {0};

    do {
        CHK_NULL_PTR_BRK(m_pSample);
        CHK_NULL_PTR_BRK(m_pInputType);

        CHK_LOG_BRK(m_pInputType->GetGUID(MF_MT_SUBTYPE, &stSubType));

        if(((stSubType == MFVideoFormat_RGB8) || (stSubType == MFVideoFormat_RGB555) ||
            (stSubType == MFVideoFormat_RGB565) || (stSubType == MFVideoFormat_RGB24) ||
            (stSubType == MFVideoFormat_RGB32)  || (stSubType == MFVideoFormat_ARGB32) ||
            (stSubType == MFVideoFormat_AI44)   || (stSubType == MFVideoFormat_AYUV) ||
            (stSubType == MFVideoFormat_I420)   || (stSubType == MFVideoFormat_IYUV) || 
            (stSubType == MFVideoFormat_NV11)   || (stSubType == MFVideoFormat_NV12) ||
            (stSubType == MFVideoFormat_UYVY)   || (stSubType == MFVideoFormat_Y41P) ||
            (stSubType == MFVideoFormat_Y41T)   || (stSubType == MFVideoFormat_Y42T) ||
            (stSubType == MFVideoFormat_YUY2)   || (stSubType == MFVideoFormat_YV12) ||
            (stSubType == MFVideoFormat_P010)   || (stSubType == MFVideoFormat_P016) ||
            (stSubType == MFVideoFormat_P210)   || (stSubType == MFVideoFormat_P216) ||
            (stSubType == MFVideoFormat_v210)   || (stSubType == MFVideoFormat_v216) ||
            (stSubType == MFVideoFormat_v410)   || (stSubType == MFVideoFormat_Y210) ||
            (stSubType == MFVideoFormat_Y216)   || (stSubType == MFVideoFormat_Y410) ||
            (stSubType == MFVideoFormat_Y416))  && m_bEnableEffects)
        {
            CHK_LOG_BRK(GetDefaultStride(&lDefaultStride));
            CHK_LOG_BRK(MFGetAttributeSize(m_pInputType, MF_MT_FRAME_SIZE, &uiWidth, &uiHeight));
            VideoBufferLock inputLock(pIn);
            VideoBufferLock outputLock(pOut);

            // Lock the input buffer.
            CHK_LOG_BRK(inputLock.LockBuffer(lDefaultStride, uiHeight, &pSrc, &lSrcStride));

            // Lock the output buffer.
            CHK_LOG_BRK(outputLock.LockBuffer(lDefaultStride, uiHeight, &pDest, &lDestStride));
            long lines =  uiHeight;
            if(m_percentOfScreen != -1 && m_percentOfScreen != 0) {
                lines = (UINT)(uiHeight * (1.0- m_percentOfScreen/100.00));
            }
            for(long i = 0; i < (long)uiHeight; i++) {
                if(lDestStride < 0) {
                    if(i >= lines) {
                        memset(pDest+i*lDestStride, 0, abs(lDefaultStride));
                    } else {
                        memcpy(pDest+i*lDestStride, pSrc+i*lDefaultStride, abs(lDefaultStride));
                    }
                } else {
                    if(i >= lines) {
                        memset(pDest+i*lDestStride, 0, lDestStride);
                    } else {
                        memcpy(pDest+i*lDestStride, pSrc+i*lDestStride, lDestStride);
                    }
                }
            }
        } 
    } while (FALSE);

    return hr;
}

STDMETHODIMP CMft0::GetDefaultStride(LONG *plStride)
{
    LONG lStride = 0;

    // Try to get the default stride from the media type.
    HRESULT hr = m_pInputType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride);
    if (FAILED(hr))
    {
        // Attribute not set. Try to calculate the default stride.
        GUID subtype = GUID_NULL;
        UINT32 width = 0;
        UINT32 height = 0;

        // Get the subtype and the image size.
        hr = m_pInputType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr))
        {
            hr = MFGetAttributeSize(m_pInputType, MF_MT_FRAME_SIZE, &width, &height);
        }
        if (SUCCEEDED(hr))
        {
            hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &lStride);
        }

        // Set the attribute for later reference.
        if (SUCCEEDED(hr))
        {
            (void)m_pInputType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
        }
    }

    if (SUCCEEDED(hr))
    {
        *plStride = lStride;
    }

    return hr;
}

STDMETHODIMP CMft0::OnFlush()
{
    HRESULT hr = S_OK;

    // For this MFT, flushing just means releasing the input sample.
    SAFERELEASE(m_pSample);

    return hr;
}

STDMETHODIMP CMft0::GetMediaType( DWORD dwStreamId, DWORD dwTypeIndex, IMFMediaType **ppType)
{
    HRESULT hr = S_OK;

    do {
        CHK_NULL_PTR_BRK(ppType);
        if(dwStreamId != 0)
            CHK_LOG_BRK(MF_E_INVALIDSTREAMNUMBER);
        if(dwTypeIndex >= m_listOfMediaTypes.GetCount())
            CHK_LOG_BRK(MF_E_NO_MORE_TYPES);

        *ppType = m_listOfMediaTypes[dwTypeIndex];
        (*ppType)->AddRef();
    } while (FALSE);

    return hr;
}

STDMETHODIMP CMft0::IsMediaTypeSupported(UINT uiStreamId, IMFMediaType *pIMFMediaType, IMFMediaType **ppIMFMediaTypeFull)
{
    HRESULT hr = S_OK;

    do {
        CHK_NULL_PTR_BRK(pIMFMediaType);

        if(uiStreamId != 0)
        {
            CHK_LOG_BRK(MF_E_INVALIDINDEX);
        }
        BOOL bFound =FALSE;
        for(UINT i = 0; i< m_listOfMediaTypes.GetCount(); i++)
        {
            DWORD   dwResult = 0;
            hr = m_listOfMediaTypes[i]->IsEqual(pIMFMediaType, &dwResult);
            if(hr == S_FALSE)
            {

                if((dwResult & MF_MEDIATYPE_EQUAL_MAJOR_TYPES) && 
                    (dwResult& MF_MEDIATYPE_EQUAL_FORMAT_TYPES) && 
                    (dwResult& MF_MEDIATYPE_EQUAL_FORMAT_DATA))
                {
                    hr = S_OK;
                }
            }
            if(hr == S_OK)
            {
                bFound = TRUE;
                if(ppIMFMediaTypeFull) {
                    *ppIMFMediaTypeFull = m_listOfMediaTypes[i];
                    (*ppIMFMediaTypeFull)->AddRef();
                }
                break;
            }
            else if(FAILED(hr))
            {
                CHK_LOG_BRK(hr);
            }

        }
        if(bFound == FALSE)
        {
            CHK_LOG_BRK(MF_E_INVALIDMEDIATYPE);
        }
    } while (FALSE);

    return hr;
}

STDMETHODIMP CMft0::GenerateMFMediaTypeListFromDevice(UINT uiStreamId)
{
    HRESULT hr = S_OK;
    GUID stSubType = {0};

    do {
        CHK_NULL_PTR_BRK(m_pSourceTransform);

        m_listOfMediaTypes.RemoveAll();
        for(UINT iMediaType = 0; TRUE; iMediaType++) 
        {
            CComPtr<IMFMediaType> pMediaType;
            hr = m_pSourceTransform->GetOutputAvailableType(uiStreamId, iMediaType, &pMediaType);
            if(hr != S_OK)
                break;
            CHK_LOG_BRK(pMediaType->GetGUID(MF_MT_SUBTYPE, &stSubType));

            if(((stSubType == MFVideoFormat_RGB8)   || (stSubType == MFVideoFormat_RGB555) ||
                (stSubType == MFVideoFormat_RGB565) || (stSubType == MFVideoFormat_RGB24) ||
                (stSubType == MFVideoFormat_RGB32)  || (stSubType == MFVideoFormat_ARGB32) ||
                (stSubType == MFVideoFormat_AI44)   || (stSubType == MFVideoFormat_AYUV) ||
                (stSubType == MFVideoFormat_I420)   || (stSubType == MFVideoFormat_IYUV) || 
                (stSubType == MFVideoFormat_NV11)   || (stSubType == MFVideoFormat_NV12) ||
                (stSubType == MFVideoFormat_UYVY)   || (stSubType == MFVideoFormat_Y41P) ||
                (stSubType == MFVideoFormat_Y41T)   || (stSubType == MFVideoFormat_Y42T) ||
                (stSubType == MFVideoFormat_YUY2)   || (stSubType == MFVideoFormat_YV12) ||
                (stSubType == MFVideoFormat_P010)   || (stSubType == MFVideoFormat_P016) ||
                (stSubType == MFVideoFormat_P210)   || (stSubType == MFVideoFormat_P216) ||
                (stSubType == MFVideoFormat_v210)   || (stSubType == MFVideoFormat_v216) ||
                (stSubType == MFVideoFormat_v410)   || (stSubType == MFVideoFormat_Y210) ||
                (stSubType == MFVideoFormat_Y216)   || (stSubType == MFVideoFormat_Y410) ||
                (stSubType == MFVideoFormat_Y416))  && m_bEnableEffects)
            {
                m_listOfMediaTypes[(ULONG)(m_listOfMediaTypes.GetCount())] = pMediaType;
            }
        }        

    } while (FALSE);

    if(hr == MF_E_NO_MORE_TYPES) {
        hr = S_OK;
    }

    return hr;
}