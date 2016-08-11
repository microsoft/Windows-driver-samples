/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        Mft0Impl.cpp

    Abstract:

        This is the implementation of CSocMft0.
        
        CSocMft0 is the MFT0 for the AvsCamera sample driver.  This MFT0
        performs the following key tasks:
            1. Filters overscan media types supplied by the driver, and
            2. Parses private metadata and attaches an IMFAttribute to the sample.

    History:

        created 5/15/2014

**************************************************************************/


#include "stdafx.h"
#include "Mft0Impl.h"
#include "SampleHelpers.h"
#include "metadataInternal.h"
#include <WinString.h>
#include <ks.h>
#include <ksproxy.h>
#include <initguid.h>

#include "CustomProperties.h"
#include "macros.h"

/////////////////////////////////////////////////////////////////////////////////
//
// These two functions demostrate that OEM application can communicate with MFT0
//
STDMETHODIMP CSocMft0::SetState(UINT32 state)
{
    // OEM can use similar function to update the status of MFT
    // From their own application
    m_uiInternalState = state;
    return S_OK;
}

STDMETHODIMP CSocMft0::GetState(UINT32 *pState)
{
    HRESULT hr = S_OK;

    // OEM can use similar function to get the status of MFT
    // From their own application
    if(!pState)
    {
        return E_POINTER;
    }
    *pState = m_uiInternalState;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
//
// This initializes the CSocMFT0 for Com
//
//
HRESULT CSocMft0::CreateInstance(
    REFIID iid,
    void **ppMFT
)
{
    HRESULT hr = S_OK;
    ComPtr<IMFTransform> spMFT;
    UNREFERENCED_PARAMETER(iid);

    auto sObject = Microsoft::WRL::Make<CSocMft0>();

    if(!sObject)
    {
        return E_OUTOFMEMORY;
    }
    hr = sObject->FinalConstruct();

    if(SUCCEEDED(hr))
    {
        hr = sObject.As(&spMFT);
        *ppMFT = spMFT.Detach();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////
//
// IInspectable interface enable COM interface to talk with other programming environments
//
// Gets the interfaces that are implemented by the current Windows Runtime class.
//
STDMETHODIMP CSocMft0::GetIids(
    _Out_ ULONG *iidCount,
    _Outptr_result_buffer_maybenull_(*iidCount) IID **iids
)
{
    HRESULT hr = S_OK;

    if(!iidCount)
    {
        return E_POINTER;
    }

    if(!iids)
    {
        return E_POINTER;
    }

    *iids = NULL;
    *iidCount = 0;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  Gets the fully qualified name of the current Windows Runtime object.
//
STDMETHODIMP CSocMft0::GetRuntimeClassName(
    _Outptr_result_maybenull_ HSTRING *pClassName
)
{

    if(!pClassName)
    {
        return E_POINTER;
    }
    return WindowsCreateString(NULL, 0, pClassName);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  Gets the trust level of the current Windows Runtime object.
//
STDMETHODIMP CSocMft0::GetTrustLevel(
    _Out_ TrustLevel *trustLevel
)
{
    HRESULT hr = S_OK;

    if(trustLevel)
    {
        return E_POINTER;
    }
    *trustLevel = TrustLevel::BaseTrust;

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
// Final Construct
//
HRESULT CSocMft0::FinalConstruct()
{
    HRESULT hr = S_OK;
    hr = MFCreateAttributes(&m_spGlobalAttributes, 3);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spGlobalAttributes->SetUINT32(MF_TRANSFORM_ASYNC, FALSE);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spGlobalAttributes->SetString(
             MFT_ENUM_HARDWARE_URL_Attribute,
             L"Sample_CameraExtensionMft");
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spGlobalAttributes->SetUINT32(
             MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE,
             TRUE);

done:
    if(FAILED(hr))
    {
        m_spGlobalAttributes.Reset();
    }
    return hr;
}

// IMFTransform methods. Refer to the Media Foundation SDK documentation for details.

////////////////////////////////////////////////////////////////////////
//
// GetStreamLimits
// Returns the minimum and maximum number of streams.
//
STDMETHODIMP CSocMft0::GetStreamLimits(
    _Out_ DWORD *pdwInputMinimum,
    _Out_ DWORD *pdwInputMaximum,
    _Out_ DWORD *pdwOutputMinimum,
    _Out_ DWORD *pdwOutputMaximum
)
{
    if( (!pdwInputMinimum) ||
            (!pdwInputMaximum) ||
            (!pdwOutputMinimum) ||
            (!pdwOutputMaximum))
    {
        return E_POINTER;
    }

    // This MFT has a fixed number of streams.
    *pdwInputMinimum = 1;
    *pdwInputMaximum = 1;
    *pdwOutputMinimum = 1;
    *pdwOutputMaximum = 1;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// GetStreamCount
// Returns the actual number of streams.
//
STDMETHODIMP CSocMft0::GetStreamCount(
    _Out_ DWORD *pcInputStreams,
    _Out_ DWORD *pcOutputStreams
)
{
    if(!pcInputStreams || !pcOutputStreams)
    {
        return E_POINTER;
    }

    // This MFT has a fixed number of streams.
    *pcInputStreams = 1;
    *pcOutputStreams = 1;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// GetStreamIDs
// Returns stream IDs for the input and output streams.
//
STDMETHODIMP CSocMft0::GetStreamIDs(
    DWORD dwInputIDArraySize,
    _Out_writes_(dwInputIDArraySize) DWORD *pdwInputIDs,
    DWORD dwOutputIDArraySize,
    _Out_writes_(dwOutputIDArraySize) DWORD *pdwOutputIDs
)
{
    UNREFERENCED_PARAMETER(dwInputIDArraySize);
    UNREFERENCED_PARAMETER(dwOutputIDArraySize);
    UNREFERENCED_PARAMETER(pdwInputIDs);
    UNREFERENCED_PARAMETER(pdwOutputIDs);

    // It is not required to implement this method if the MFT has a fixed number of
    // streams AND the stream IDs are numbered sequentially from zero (that is, the
    // stream IDs match the stream indexes).

    // In that case, it is OK to return E_NOTIMPL.
    return (E_NOTIMPL);
}

////////////////////////////////////////////////////////////////////////
//
// GetInputStreamInfo
// Returns information about an input stream.
//
STDMETHODIMP CSocMft0::GetInputStreamInfo(
    DWORD dwInputStreamID,
    _Out_ MFT_INPUT_STREAM_INFO *pStreamInfo
)
{
    CAutoLock lock(&m_critSec);

    if(!pStreamInfo)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(!m_spInputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    pStreamInfo->hnsMaxLatency  = 0;
    pStreamInfo->cbAlignment = 0;
    pStreamInfo->cbSize = 0;
    pStreamInfo->dwFlags =
        MFT_INPUT_STREAM_WHOLE_SAMPLES |
        MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
        MFT_INPUT_STREAM_PROCESSES_IN_PLACE ;
    pStreamInfo->hnsMaxLatency = 0;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// GetOutputStreamInfo
// Returns information about an output stream.
//
STDMETHODIMP CSocMft0::GetOutputStreamInfo(
    DWORD dwOutputStreamID,
    _Out_ MFT_OUTPUT_STREAM_INFO *pStreamInfo
)
{
    CAutoLock lock(&m_critSec);

    if(!pStreamInfo)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(!m_spOutputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    pStreamInfo->cbAlignment = 0;
    pStreamInfo->cbSize = 0;
    pStreamInfo->dwFlags =
        MFT_OUTPUT_STREAM_WHOLE_SAMPLES  |
        MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER |
        MFT_OUTPUT_STREAM_PROVIDES_SAMPLES |
        MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// GetAttributes
// Returns the global attributes for the MFT.
//
STDMETHODIMP CSocMft0::GetAttributes(
    _Outptr_result_maybenull_ IMFAttributes **ppAttributes
)
{
    if(!ppAttributes)
    {
        return E_POINTER;
    }

    return m_spGlobalAttributes.CopyTo(ppAttributes);
}

////////////////////////////////////////////////////////////////////////
//
// GetInputStreamAttributes
// Returns stream-level attributes for an input stream.
//
STDMETHODIMP CSocMft0::GetInputStreamAttributes(
    DWORD dwInputStreamID,
    _Outptr_result_maybenull_ IMFAttributes **ppAttributes
)
{
    HRESULT hr = S_OK;

    if(dwInputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }
    if(!ppAttributes)
    {
        return E_POINTER;
    }

    if(!m_spInputAttributes)
    {
        hr = MFCreateAttributes(&m_spInputAttributes, 2);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = m_spInputAttributes->SetUINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = m_spInputAttributes->SetString(MFT_ENUM_HARDWARE_URL_Attribute, L"Sample_CameraExtensionMft");
        if (FAILED(hr))
        {
            goto done;
        }
    }
    hr = m_spInputAttributes.CopyTo(ppAttributes);

done:
    if(FAILED(hr))
    {
        m_spInputAttributes.Reset();
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////
//
// GetOutputStreamAttributes
// Returns stream-level attributes for an output stream.
//
STDMETHODIMP CSocMft0::GetOutputStreamAttributes(
    DWORD dwOutputStreamID,
    _Outptr_result_maybenull_ IMFAttributes **ppAttributes
)
{
    if(dwOutputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(!ppAttributes)
    {
        return E_POINTER;
    }

    if(!m_spInputAttributes)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    return m_spInputAttributes.CopyTo(ppAttributes);
}

////////////////////////////////////////////////////////////////////////
//
// DeleteInputStream
//
STDMETHODIMP CSocMft0::DeleteInputStream(
    DWORD dwStreamID
)
{
    UNREFERENCED_PARAMETER(dwStreamID);

    // This MFT has a fixed number of input streams, so the method is not supported.
    return (E_NOTIMPL);
}

////////////////////////////////////////////////////////////////////////
//
// AddInputStreams
//
STDMETHODIMP CSocMft0::AddInputStreams(
    DWORD cStreams,
    _In_ DWORD *adwStreamIDs
)
{
    UNREFERENCED_PARAMETER(cStreams);
    UNREFERENCED_PARAMETER(adwStreamIDs);

    // This MFT has a fixed number of output streams, so the method is not supported.
    return (E_NOTIMPL);
}

////////////////////////////////////////////////////////////////////////
//
// GetInputAvailableType
// Returns a preferred input type.
//
STDMETHODIMP CSocMft0::GetInputAvailableType(
    DWORD dwInputStreamID,
    DWORD dwTypeIndex,
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    HRESULT hr = S_OK;
    ComPtr<IUnknown> spUnknown;
    ComPtr<IMFAttributes> spSourceAttributes;

    CAutoLock lock(&m_critSec);
    if(!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(!m_spSourceTransform && m_spInputAttributes)
    {
        hr = m_spInputAttributes->GetUnknown(MFT_CONNECTED_STREAM_ATTRIBUTE, IID_PPV_ARGS(spSourceAttributes.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT, IID_PPV_ARGS(spUnknown.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spUnknown.As(&m_spSourceTransform);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetUINT32(MF_DEVICESTREAM_STREAM_ID, &m_uiSourceStreamId);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = GenerateMFMediaTypeListFromDevice();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetMediaType(dwInputStreamID, dwTypeIndex, ppType);

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
// GetOutputAvailableType
// Returns a preferred output type.
//
STDMETHODIMP CSocMft0::GetOutputAvailableType(
    DWORD dwOutputStreamID,
    DWORD dwTypeIndex,
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    HRESULT hr = S_OK;
    ComPtr<IUnknown> spUnknown;
    ComPtr<IMFAttributes> spSourceAttributes;

    CAutoLock lock(&m_critSec);

    if (!IsValidInputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_spSourceTransform && m_spInputAttributes)
    {
        hr = m_spInputAttributes->GetUnknown(MFT_CONNECTED_STREAM_ATTRIBUTE, IID_PPV_ARGS(spSourceAttributes.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT, IID_PPV_ARGS(spUnknown.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spUnknown.As(&m_spSourceTransform);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = spSourceAttributes->GetUINT32(MF_DEVICESTREAM_STREAM_ID, &m_uiSourceStreamId);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // This is the first function get called after the MFT0 object get instantiated,
        // Here we can generate the supported media type from the connected input pin,
        // Also, we can selectively plug in MFT0 only on pins requires processing
        // by returning error on the unwanted pin types. Example below if we did not
        // want MFT0 for Record

        //if(m_stStreamType != PINNAME_IMAGE && m_stStreamType != PINNAME_VIDEO_PREVIEW)
        //{
        //    return E_UNEXPECTED;
        //}

        hr = GenerateMFMediaTypeListFromDevice();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return GetMediaType(dwOutputStreamID, dwTypeIndex, ppType);
}

////////////////////////////////////////////////////////////////////////
//
// SetInputType
//
STDMETHODIMP CSocMft0::SetInputType(
    DWORD dwInputStreamID,
    _In_opt_ IMFMediaType *pType,
    DWORD dwFlags
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

    // Validate flags.
    // Only MFT_SET_TYPE_TEST_ONLY is supported, if any other bit flags
    // get set, it should return error.
    if(dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
    {
        return E_INVALIDARG;
    }

    if(!bReallySet)
    {
        return IsMediaTypeSupported(dwInputStreamID, pType);
    }

    ComPtr<IMFMediaType> spFullType;
    hr = IsMediaTypeSupported(dwInputStreamID, pType, spFullType.ReleaseAndGetAddressOf());
    if(FAILED(hr))
    {
        return hr;
    }

    m_spInputType = spFullType;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// SetOutputType
// This function set the output media type and at the same time, it should
// also choose a compatible input type
//
STDMETHODIMP CSocMft0::SetOutputType(
    DWORD dwOutputStreamID,
    _In_opt_ IMFMediaType *pType,
    DWORD dwFlags
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);
    BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);
    BOOL bUseModifiedInputType = FALSE;

    // Validate flags.
    if(dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
    {
        return E_INVALIDARG;
    }

    if(!bReallySet)
    {
        return IsMediaTypeSupported(dwOutputStreamID, pType);
    }

    ComPtr<IMFMediaType> spFullType;
    ComPtr<IMFMediaType> spFullInputType;
    hr = IsMediaTypeSupported(dwOutputStreamID, pType, spFullType.ReleaseAndGetAddressOf());
    if(FAILED(hr))
    {
        return hr;
    }

    if (m_stStreamType == PINNAME_CAPTURE)
    {
        hr = GetVideoStabilizationEnabled();
        if (FAILED(hr))
        {
            return hr;
        }

        // Hardware Video Stabilization is enabled
        if (m_bEnableVideoStabilization)
        {
            UINT32 uiIndex = 0;
            //This shouldn't fail, since it is derived from IsMediaTypeSupported
            // and that just passed.
            hr = FindMediaIndex(dwOutputStreamID, pType, &uiIndex);
            if (FAILED(hr))
            {
                return hr;
            }
            //Device Driver contains a static list of mediatypes on the pin
            // We have ordered them in such a way so that all odd mediatypes are the
            // overscan media of the even mediatype immediatley proceeding it, ie.
            //  -normalVGA
            //  -overscanVGA
            //  -normalQVGA
            //  -overscanQVGA
            //  etc
            // The following command determines if we are dealing with an overscan or normal mediatype
            //  if it is overscan, we fail (cannot overscan an overscan mediatype)
            //  otherwise, we set our input mediatype to the overscan type associated with it.
            if (uiIndex % 2 != 0) //odd means overscan
            {
                return MF_E_INVALIDMEDIATYPE;
            }
            else //even
            {
                hr = GetMediaType(dwOutputStreamID, uiIndex + 1, spFullInputType.ReleaseAndGetAddressOf());
                if (FAILED(hr))
                {
                    return hr;
                }

                bUseModifiedInputType = TRUE;
            }
        }
    }
    m_spOutputType = spFullType;

    // Here, we choose a compatible input type which is the same type
    // as output. If OEM choose to encode sample here, it should choose
    // an input which is supported by encoder.
    if (bUseModifiedInputType)
    {
        m_spInputType = spFullInputType;
    }
    else
    {
        m_spInputType = spFullType;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// GetInputCurrentType
// Returns the current input type.
//
STDMETHODIMP CSocMft0::GetInputCurrentType(
    DWORD dwInputStreamID,
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    if(!ppType)
    {
        return E_POINTER;
    }
    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(!m_spInputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    ComPtr<IMFMediaType> spMediaType;
    hr = MFCreateMediaType(spMediaType.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_spInputType->CopyAllItems(spMediaType.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    return spMediaType.CopyTo(ppType);
}

////////////////////////////////////////////////////////////////////////
//
// GetOutputCurrentType
// Returns the current output type.
//
STDMETHODIMP CSocMft0::GetOutputCurrentType(
    DWORD dwOutputStreamID,
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    if(!ppType)
    {
        return E_POINTER;
    }

    if (!IsValidOutputStream(dwOutputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }
    if(!m_spOutputType)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    ComPtr<IMFMediaType> spMediaType;
    hr = MFCreateMediaType(spMediaType.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_spOutputType->CopyAllItems(spMediaType.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    return spMediaType.CopyTo(ppType);
}

////////////////////////////////////////////////////////////////////////
//
// GetInputStatus
// Query if the MFT is accepting more input.
//
STDMETHODIMP CSocMft0::GetInputStatus(
    DWORD dwInputStreamID,
    _Out_ DWORD *pdwFlags
)
{
    CAutoLock lock(&m_critSec);

    if(!pdwFlags)
    {
        return E_POINTER;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    // If we already have an input sample, we don't accept
    // another one until the client calls ProcessOutput or Flush.
    if (m_spSample == NULL)
    {
        *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
    }
    else
    {
        *pdwFlags = 0;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// GetOutputStatus
// Query if the MFT can produce output.
//
STDMETHODIMP CSocMft0::GetOutputStatus(
    _Out_ DWORD *pdwFlags
)
{
    if(!pdwFlags)
    {
        return E_POINTER;
    }

    CAutoLock lock(&m_critSec);

    // We can produce an output sample if (and only if)
    // we have an input sample.
    if (m_spSample != NULL)
    {
        *pdwFlags = MFT_OUTPUT_STATUS_SAMPLE_READY;
    }
    else
    {
        *pdwFlags = 0;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// SetOutputBounds
// Sets the range of time stamps that the MFT will output.
//
STDMETHODIMP CSocMft0::SetOutputBounds(
    LONGLONG hnsLowerBound,
    LONGLONG hnsUpperBound
)
{
    UNREFERENCED_PARAMETER(hnsLowerBound);
    UNREFERENCED_PARAMETER(hnsUpperBound);

    // Implementation of this method is optional.
    return (E_NOTIMPL);
}

////////////////////////////////////////////////////////////////////////
//
// ProcessEvent
// Sends an event to an input stream.
//
STDMETHODIMP CSocMft0::ProcessEvent(
    DWORD dwInputStreamID,
    _In_opt_ IMFMediaEvent *pEvent
)
{
    UNREFERENCED_PARAMETER(dwInputStreamID);
    UNREFERENCED_PARAMETER(pEvent);
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//
// ProcessMessage
//
STDMETHODIMP CSocMft0::ProcessMessage(
    MFT_MESSAGE_TYPE eMessage,
    ULONG_PTR ulParam
)
{
    UNREFERENCED_PARAMETER(ulParam);
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    switch (eMessage)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        // Flush the MFT.
        hr = OnFlush();
        break;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// ProcessInput
// Process an input sample. The sample is cached and real work is done
// in ProcessOutput
//
STDMETHODIMP CSocMft0::ProcessInput(
    DWORD dwInputStreamID,
    IMFSample *pSample,
    DWORD dwFlags
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    if(!pSample)
    {
        return E_POINTER;
    }
    if(dwFlags != 0)
    {
        return E_INVALIDARG;
    }

    if (!IsValidInputStream(dwInputStreamID))
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if (!m_spInputType || !m_spOutputType)
    {
        return MF_E_NOTACCEPTING;   // Client must set input and output types.
    }

    if (m_spSample != NULL)
    {
        return MF_E_NOTACCEPTING;   // We already have an input sample.
    }

    // Validate the number of buffers. There should only be a single buffer to hold the video frame.
    DWORD dwBufferCount = 0;
    hr = pSample->GetBufferCount(&dwBufferCount);
    if(FAILED(hr))
    {
        return hr;
    }

    if (dwBufferCount == 0)
    {
        return E_FAIL;
    }

    if (dwBufferCount > 1)
    {
        return MF_E_SAMPLE_HAS_TOO_MANY_BUFFERS;
    }

    // Cache the sample. We do the actual work in ProcessOutput.
    m_spSample = pSample;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//
// ProcessOutput
// Process an output sample.
//
STDMETHODIMP CSocMft0::ProcessOutput(
    DWORD dwFlags,
    DWORD cOutputBufferCount,
    MFT_OUTPUT_DATA_BUFFER *pOutputSamples,
    DWORD *pdwStatus
)
{
    HRESULT hr = S_OK;

    CAutoLock lock(&m_critSec);

    if(dwFlags != 0)
    {
        return E_INVALIDARG;
    }

    if(!pOutputSamples || !pdwStatus)
    {
        return E_POINTER;
    }

    if( pOutputSamples[0].pSample)
    {
        pOutputSamples[0].pSample->Release();
        pOutputSamples[0].pSample = nullptr;
    }

    // Must be exactly one output buffer.
    if(cOutputBufferCount != 1)
    {
        return E_INVALIDARG;
    }

    // If we don't have an input sample, we need some input before
    // we can generate any output.
    if(m_spSample == nullptr)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    hr = ProcessMetadata();
    if(FAILED(hr))
    {
        //Log the failure
        hr = S_OK;
    }    
    hr = CreateOutputSample(&pOutputSamples[0].pSample);
    if(FAILED(hr))
    {
        return hr;
    }
    pOutputSamples[0].dwStatus = 0;
    *pdwStatus = 0;

    // if createOutputSample actually make a copy of the sample,
    // (for example, JPEG encoding case), we need to copy the
    // attribute from m_spSample to spOutputIMFSample
    m_spSample.Reset();
    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// The actual processing is here, required operation is demux
// Other operation is optional
//
HRESULT
CSocMft0::
CreateOutputSample(
    _Outptr_result_maybenull_
    IMFSample **ppSample
)
{
    HRESULT hr = S_OK;

    //Do nothing for now
    if(!ppSample)
    {
        return E_POINTER;
    }

    m_spSample.CopyTo(ppSample);

    return hr;
}
/////////////////////////////////////////////////////////////////////
//
// Flush the MFT.
//
STDMETHODIMP CSocMft0::OnFlush()
{
    // For this MFT, flushing just means releasing the input sample.
    CAutoLock lock(&m_critSec);
    m_spSample.Reset();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//
// Get the mediaType of selected stream.
//
STDMETHODIMP CSocMft0::GetMediaType(
    _In_ DWORD dwStreamId,
    _In_ DWORD dwTypeIndex,
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    HRESULT hr = S_OK;
    ComPtr<IMFMediaType> spMediaType;

    if(!ppType)
    {
        return E_POINTER;
    }

    if(dwStreamId != 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(dwTypeIndex >= m_listOfMediaTypes.size())
    {
        return MF_E_NO_MORE_TYPES;
    }

    hr = MFCreateMediaType(spMediaType.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_listOfMediaTypes[dwTypeIndex]->CopyAllItems(spMediaType.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    return spMediaType.CopyTo(ppType);
}

/////////////////////////////////////////////////////////////////////
//
// Check if the media type is supported by MFT0.
//
STDMETHODIMP CSocMft0::IsMediaTypeSupported(
    _In_ UINT uiStreamId,
    _In_ IMFMediaType *pIMFMediaType,
    _Outptr_result_maybenull_ IMFMediaType **ppIMFMediaTypeFull
)
{
    HRESULT hr = S_OK;

    if(!pIMFMediaType)
    {
        return E_POINTER;
    }

    if(uiStreamId != 0)
    {
        return MF_E_INVALIDINDEX;
    }

    BOOL bFound =FALSE;


    for(UINT i = 0; i< m_listOfMediaTypes.size(); i++)
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
            if(ppIMFMediaTypeFull)
            {
                m_listOfMediaTypes[i].CopyTo(ppIMFMediaTypeFull);
            }
            break;
        }
        else if(FAILED(hr))
        {
            return hr;
        }
    }

    if(bFound == FALSE)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//
// Check if the media type is supported by MFT0.
//
_Success_(return == 0)
STDMETHODIMP CSocMft0::FindMediaIndex(
    _In_ UINT uiStreamId,
    _In_ IMFMediaType *pIMFMediaType,
    _Out_ UINT *puiMediaIndex
)
{
    HRESULT hr = S_OK;

    if(!pIMFMediaType || !puiMediaIndex)
    {
        return E_INVALIDARG;
    }

    if(uiStreamId != 0)
    {
        return MF_E_INVALIDINDEX;
    }

    BOOL bFound = FALSE;

    for(UINT i = 0; i< m_listOfMediaTypes.size(); i++)
    {
        DWORD   dwResult = 0;
        hr = m_listOfMediaTypes[i]->IsEqual(pIMFMediaType, &dwResult);
        if(hr == S_FALSE)
        {
            if((dwResult & MF_MEDIATYPE_EQUAL_MAJOR_TYPES) &&
                    (dwResult & MF_MEDIATYPE_EQUAL_FORMAT_TYPES) &&
                    (dwResult & MF_MEDIATYPE_EQUAL_FORMAT_DATA))
            {
                hr = S_OK;
            }
        }
        if(hr == S_OK)
        {
            bFound = TRUE;
            *puiMediaIndex = i;
            break;
        }
        else if(FAILED(hr))
        {
            return hr;
        }
    }

    if(bFound == FALSE)
    {
        return MF_E_INVALIDMEDIATYPE;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//
// Generate the supported input media type from device source media type
//
STDMETHODIMP CSocMft0::GenerateMFMediaTypeListFromDevice()
{
    HRESULT hr = S_OK;

    if(!m_spSourceTransform)
    {
        return MF_E_NOT_FOUND;
    }

    m_listOfMediaTypes.clear();
    UINT iMediaType = 0;
    while(SUCCEEDED(hr))
    {
        ComPtr<IMFMediaType> spMediaType;
        hr = m_spSourceTransform->GetOutputAvailableType(m_uiSourceStreamId, iMediaType, spMediaType.ReleaseAndGetAddressOf());
        if(FAILED(hr))
        {
            if(hr == MF_E_NO_MORE_TYPES)
            {
                hr = S_OK;
            }
            break;
        }

        if(m_stStreamType == PINNAME_IMAGE || m_stStreamType == PINNAME_VIDEO_STILL)
        {
            GUID guidPhotoSubType = {};
            spMediaType->GetGUID(MF_MT_SUBTYPE, &guidPhotoSubType);
            if (IsEqualGUID(guidPhotoSubType, MFVideoFormat_NV12))
            {
                // Need to set MF_MT_VIDEO_NOMINAL_RANGE on NV12 or the pipeline ends up with a copy due to mismatched nominal range
                hr = spMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);
                if (FAILED(hr))
                {
                    return hr;
                }
            }

            
            m_listOfMediaTypes.push_back(spMediaType);
        }

        else if (m_stStreamType == PINNAME_CAPTURE)
        {
            UINT32 width = 0;
            UINT32 height = 0;
            hr = MFGetAttributeSize(spMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);
            if (FAILED(hr))
            {
                return hr;
            }

            MFVideoArea validVideo;
            validVideo.OffsetX.value = 0;
            validVideo.OffsetX.fract = 0;
            validVideo.OffsetY.value = 0;
            validVideo.OffsetY.fract = 0;
            validVideo.Area.cx = width;
            validVideo.Area.cy = height;
            hr = spMediaType->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&validVideo, sizeof(validVideo));
            if (FAILED(hr))
            {
                return hr;
            }

            m_listOfMediaTypes.push_back(spMediaType);
        }

        else
        {
            m_listOfMediaTypes.push_back(spMediaType);
        }

        iMediaType++;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Get the current media type in order to generate photo confirmation
//
HRESULT CSocMft0::GetPreviewMediaType(
    _Outptr_result_maybenull_ IMFMediaType **ppType
)
{
    CAutoLock lock(&m_critSec);
    if(!m_spSourceTransform)
    {
        return MF_E_NOT_FOUND;
    }

    DWORD dwInStreamCount = 0, dwOutStreamCount = 0;
    HRESULT hr = m_spSourceTransform->GetStreamCount(
                     &dwInStreamCount,
                     &dwOutStreamCount);
    if (FAILED(hr))
    {
        return hr;
    }

    for(UINT32 i = 0 ; i < dwOutStreamCount; i++)
    {
        ComPtr<IMFAttributes> spAttributes;
        hr = m_spSourceTransform->GetOutputStreamAttributes(i, spAttributes.ReleaseAndGetAddressOf());
        if(FAILED(hr))
        {
            continue;
        }

        GUID guidPinCategory = GUID_NULL;
        hr = spAttributes->GetGUID(
                 MF_DEVICESTREAM_STREAM_CATEGORY,
                 &guidPinCategory);
        if(FAILED(hr))
        {
            continue;
        }
        if (IsEqualGUID(guidPinCategory, PINNAME_VIDEO_PREVIEW))
        {
            ComPtr<IMFMediaType> spMediaType;
            hr =  m_spSourceTransform->GetOutputCurrentType(i, spMediaType.ReleaseAndGetAddressOf());
            if(FAILED(hr))
            {
                return MF_E_NOT_FOUND;
            }
            return spMediaType.CopyTo(ppType);
        }
    }

    return MF_E_NOT_FOUND;
}

/////////////////////////////////////////////////////////////////////
//
// Handle the Metadata with the buffer
//
HRESULT CSocMft0::ProcessMetadata()
{
    ComPtr<IMFAttributes>  spMetadata;
    HRESULT hr = m_spSample->GetUnknown(MFSampleExtension_CaptureMetadata, IID_PPV_ARGS(spMetadata.ReleaseAndGetAddressOf()));
    if(FAILED(hr))
    {
        return hr;
    }

    ComPtr<IMFMediaBuffer> spBuffer;
    hr = spMetadata->GetUnknown(MF_CAPTURE_METADATA_FRAME_RAWSTREAM, IID_PPV_ARGS(spBuffer.ReleaseAndGetAddressOf()));
    if(FAILED(hr))
    {
        return hr;
    }

    MediaBufferLock bufferLock(spBuffer.Get());
    BYTE *pData = NULL;
    DWORD dwLength = 0;
    hr = bufferLock.LockBuffer(&pData, NULL, &dwLength);
    if(FAILED(hr))
    {
        return hr;
    }

    // OEM put meta data passing logic here,
    if(FAILED(hr))
    {
        return hr;
    }

    LONG lBufferLeft = static_cast<LONG>(dwLength);
    if(lBufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
    {
        return E_UNEXPECTED;
    }

    PKSCAMERA_METADATA_ITEMHEADER pItem = (PKSCAMERA_METADATA_ITEMHEADER) pData;

    while(lBufferLeft > 0)
    {
        switch (pItem->MetadataId)
        {
        case MetadataId_Custom_PreviewAggregation:
            hr = ParseMetadata_PreviewAggregation(pItem, spMetadata.Get());
            if(FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_ImageAggregation:
            hr = ParseMetadata_ImageAggregation(pItem, spMetadata.Get());
            if(FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_Histogram:
            hr = ParseMetadata_Histogram(pItem, spMetadata.Get());
            if(FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_FaceDetection:
            hr = ParseMetadata_FaceDetection(pItem, spMetadata.Get());
            if (FAILED(hr))
            {
                return hr;
            }
        }

        if(!pItem->Size)
        {
            // 0 size item will cause the loop to break and
            // we will report buffer malformated
            break;
        }
        lBufferLeft -= (LONG)pItem->Size;
        if(lBufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
        {
            break;
        }
        pItem = reinterpret_cast<PKSCAMERA_METADATA_ITEMHEADER>
                (reinterpret_cast<PBYTE>(pItem) + pItem->Size);
    }

    if(lBufferLeft != 0)
    {
        //Check and log for malformated data
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT CSocMft0::ParseMetadata_PreviewAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes *pMetaDataAttributes
)
{
    HRESULT hr = S_OK;
    if(pItem->Size < sizeof(CAMERA_METADATA_PREVIEWAGGREGATION))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_PREVIEWAGGREGATION pFixedStruct =
        (PCAMERA_METADATA_PREVIEWAGGREGATION)pItem;

    hr = pMetaDataAttributes->SetUINT32(
             MF_CAPTURE_METADATA_FOCUSSTATE,
             pFixedStruct->Data.FocusState);
    if(FAILED(hr))
    {
        return hr;
    }

    if(pFixedStruct->Data.ExposureTime.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
                 MF_CAPTURE_METADATA_EXPOSURE_TIME,
                 pFixedStruct->Data.ExposureTime.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.ISOSpeed.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_ISO_SPEED,
                 pFixedStruct->Data.ISOSpeed.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.LensPosition.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_LENS_POSITION,
                 pFixedStruct->Data.LensPosition.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.FlashOn.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_FLASH,
                 pFixedStruct->Data.FlashOn.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.WhiteBalanceMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_WHITEBALANCE,
                 pFixedStruct->Data.WhiteBalanceMode.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.EVCompensation.Set)
    {
        CapturedMetadataExposureCompensation EVCompensation;

        EVCompensation.Flags = pFixedStruct->Data.EVCompensation.Flags;
        EVCompensation.Value = pFixedStruct->Data.EVCompensation.Value;

        hr = pMetaDataAttributes->SetBlob(
                 MF_CAPTURE_METADATA_EXPOSURE_COMPENSATION,
                 (const UINT8 *)&EVCompensation,
                 sizeof(EVCompensation));
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.SensorFrameRate.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
                 MF_CAPTURE_METADATA_SENSORFRAMERATE,
                 pFixedStruct->Data.SensorFrameRate.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if( pFixedStruct->Data.IsoAnalogGain.Set ||
            pFixedStruct->Data.IsoDigitalGain.Set )
    {
        CapturedMetadataISOGains IsoGains;

        if( pFixedStruct->Data.IsoAnalogGain.Set )
        {
            IsoGains.AnalogGain =
                FLOAT( pFixedStruct->Data.IsoAnalogGain.Numerator ) /
                FLOAT( pFixedStruct->Data.IsoAnalogGain.Denominator );
        }
        if( pFixedStruct->Data.IsoDigitalGain.Set )
        {
            IsoGains.DigitalGain =
                FLOAT( pFixedStruct->Data.IsoDigitalGain.Numerator ) /
                FLOAT( pFixedStruct->Data.IsoDigitalGain.Denominator );
        }

        hr = pMetaDataAttributes->SetBlob(
                 MF_CAPTURE_METADATA_ISO_GAINS,
                 (const UINT8 *)&IsoGains,
                 sizeof(IsoGains));
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if( pFixedStruct->Data.WhiteBalanceGain_R.Set ||
            pFixedStruct->Data.WhiteBalanceGain_G.Set ||
            pFixedStruct->Data.WhiteBalanceGain_B.Set )
    {
        CapturedMetadataWhiteBalanceGains WhiteBalanceGains;

        if( pFixedStruct->Data.WhiteBalanceGain_R.Set )
        {
            WhiteBalanceGains.R =
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_R.Numerator ) /
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_R.Denominator );
        }
        if( pFixedStruct->Data.WhiteBalanceGain_G.Set )
        {
            WhiteBalanceGains.G =
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_G.Numerator ) /
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_G.Denominator );
        }
        if( pFixedStruct->Data.WhiteBalanceGain_B.Set )
        {
            WhiteBalanceGains.B =
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_B.Numerator ) /
                FLOAT( pFixedStruct->Data.WhiteBalanceGain_B.Denominator );
        }

        hr = pMetaDataAttributes->SetBlob(
                 MF_CAPTURE_METADATA_WHITEBALANCE_GAINS,
                 (const UINT8 *)&WhiteBalanceGains,
                 sizeof(WhiteBalanceGains));
        if(FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT CSocMft0::ParseMetadata_ImageAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes *pMetaDataAttributes
)
{
    HRESULT hr = S_OK;
    if(pItem->Size < sizeof(CAMERA_METADATA_IMAGEAGGREGATION))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_IMAGEAGGREGATION pFixedStruct =
        (PCAMERA_METADATA_IMAGEAGGREGATION)pItem;

    if(pFixedStruct->Data.FrameId.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_REQUESTED_FRAME_SETTING_ID,
                 pFixedStruct->Data.FrameId.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.ExposureTime.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
                 MF_CAPTURE_METADATA_EXPOSURE_TIME,
                 pFixedStruct->Data.ExposureTime.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.ISOSpeed.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_ISO_SPEED,
                 pFixedStruct->Data.ISOSpeed.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.LensPosition.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_LENS_POSITION,
                 pFixedStruct->Data.LensPosition.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.SceneMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
                 MF_CAPTURE_METADATA_SCENE_MODE,
                 pFixedStruct->Data.SceneMode.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.FlashOn.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_FLASH,
                 pFixedStruct->Data.FlashOn.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.FlashPower.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_FLASH_POWER,
                 pFixedStruct->Data.FlashPower.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.WhiteBalanceMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_WHITEBALANCE,
                 pFixedStruct->Data.WhiteBalanceMode.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.ZoomFactor.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_ZOOMFACTOR,
                 pFixedStruct->Data.ZoomFactor.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(pFixedStruct->Data.EVCompensation.Set)
    {
        CapturedMetadataExposureCompensation EVCompensation;

        EVCompensation.Flags = pFixedStruct->Data.EVCompensation.Flags;
        EVCompensation.Value = pFixedStruct->Data.EVCompensation.Value;

        hr = pMetaDataAttributes->SetBlob(
                 MF_CAPTURE_METADATA_EXPOSURE_COMPENSATION,
                 (const UINT8 *)&EVCompensation,
                 sizeof(EVCompensation));
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if( pFixedStruct->Data.FocusState.Set )
    {
        hr = pMetaDataAttributes->SetUINT32(
                 MF_CAPTURE_METADATA_FOCUSSTATE,
                 pFixedStruct->Data.FocusState.Value);
        if(FAILED(hr))
        {
            return hr;
        }
    }
    return S_OK;
}

struct HistogramData
{
    HistogramDataHeader     Header;
    ULONG                   Color[256];

    HistogramData()
    {
        Header.Size = sizeof(*this);
        Header.ChannelMask = 0;
        Header.Linear = 1;
        RtlZeroMemory( Color, sizeof(Color) );
    }
};

//  This is the blob we pass back every time.
template <ULONG I>
struct Histogram
{
    HistogramBlobHeader     Blob;
    //  RGB or YUV Histograms.
    HistogramHeader         Header;
    HistogramData           Data[I];

    Histogram(
        _In_    ULONG   Width=0,
        _In_    ULONG   Height=0
    )
    {
        Blob.Histograms = 1;
        Blob.Size = sizeof(*this);

        Header.Size = sizeof(Header) + sizeof(Data);
        Header.Bins = 256;
        Header.Grid.Height = Height;
        Header.Grid.Width = Width;
        Header.Grid.Region.top = 0;
        Header.Grid.Region.left = 0;
        Header.Grid.Region.bottom = Height-1;
        Header.Grid.Region.right = Width-1;
    }
};

#define MF_HISTOGRAM_RGB    (MF_HISTOGRAM_CHANNEL_R | MF_HISTOGRAM_CHANNEL_G  | MF_HISTOGRAM_CHANNEL_B )
#define MF_HISTOGRAM_YCrCb  (MF_HISTOGRAM_CHANNEL_Y | MF_HISTOGRAM_CHANNEL_Cr | MF_HISTOGRAM_CHANNEL_Cb)

HRESULT CSocMft0::ParseMetadata_Histogram(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes *pMetaDataAttributes
)
{
    if(pItem->Size < sizeof(CAMERA_METADATA_HISTOGRAM))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_HISTOGRAM pHistogram = (PCAMERA_METADATA_HISTOGRAM) pItem;

    if( (pHistogram->Data.ChannelMask & MF_HISTOGRAM_RGB) == MF_HISTOGRAM_RGB )
    {
        Histogram<4>    Blob( pHistogram->Data.Width, pHistogram->Data.Height );

        Blob.Header.FourCC = pHistogram->Data.FourCC;
        Blob.Header.ChannelMasks = pHistogram->Data.ChannelMask;

        //  For a RGB Histogram, we fake the Y channel by copying the G channel.
        Blob.Data[0].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Y;
        RtlCopyMemory( Blob.Data[0].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[0].Color) );

        //  Now just copy the RGB channels normally.
        Blob.Data[1].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_R;
        RtlCopyMemory( Blob.Data[1].Color, pHistogram->Data.P0Data, sizeof(Blob.Data[1].Color) );
        Blob.Data[2].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_G;
        RtlCopyMemory( Blob.Data[2].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[2].Color) );
        Blob.Data[3].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_B;
        RtlCopyMemory( Blob.Data[3].Color, pHistogram->Data.P2Data, sizeof(Blob.Data[3].Color) );

        return pMetaDataAttributes->SetBlob(
                   MF_CAPTURE_METADATA_HISTOGRAM,
                   (PBYTE) &Blob,
                   sizeof(Blob)
               );
    }
    else if( (pHistogram->Data.ChannelMask & MF_HISTOGRAM_YCrCb) == MF_HISTOGRAM_YCrCb )
    {
        Histogram<3>    Blob( pHistogram->Data.Width, pHistogram->Data.Height );

        Blob.Header.FourCC = pHistogram->Data.FourCC;
        Blob.Header.ChannelMasks = pHistogram->Data.ChannelMask;

        Blob.Data[0].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Y;
        RtlCopyMemory( Blob.Data[0].Color, pHistogram->Data.P0Data, sizeof(Blob.Data[0].Color) );
        Blob.Data[1].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Cr;
        RtlCopyMemory( Blob.Data[1].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[1].Color) );
        Blob.Data[2].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Cb;
        RtlCopyMemory( Blob.Data[2].Color, pHistogram->Data.P2Data, sizeof(Blob.Data[2].Color) );

        //TODO:
        return pMetaDataAttributes->SetBlob(
                   MF_CAPTURE_METADATA_HISTOGRAM,
                   (PBYTE) &Blob,
                   sizeof(Blob)
               );
    }
    return E_UNEXPECTED;
}

HRESULT CSocMft0::ParseMetadata_FaceDetection(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes *pMetaDataAttributes
)
{
    HRESULT hr = S_OK;

    if (pItem->Size < sizeof(CAMERA_METADATA_FACEHEADER))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_FACEHEADER pFaceHeader = (PCAMERA_METADATA_FACEHEADER)pItem;

    if( pItem->Size < (sizeof(CAMERA_METADATA_FACEHEADER) + (sizeof(METADATA_FACEDATA) * pFaceHeader->Count)) )
    {
        return E_UNEXPECTED;
    }
    PMETADATA_FACEDATA  pFaceData = (PMETADATA_FACEDATA) (pFaceHeader+1);
    UINT32 cbRectSize = sizeof(FaceRectInfoBlobHeader) + (sizeof(FaceRectInfo) * (pFaceHeader->Count));
    BYTE *pRectBuf = new BYTE [cbRectSize];
    if (pRectBuf == NULL)
    {
        return E_OUTOFMEMORY;
    }

    UINT32 cbCharSize = sizeof(FaceCharacterizationBlobHeader) + (sizeof(FaceCharacterization) * (pFaceHeader->Count));
    BYTE *pCharBuf = new BYTE[cbCharSize];
    if (pCharBuf == NULL)
    {
        delete [] pRectBuf;
        return E_OUTOFMEMORY;
    }

    FaceRectInfoBlobHeader *pFaceRectHeader = (FaceRectInfoBlobHeader *)pRectBuf;
    pFaceRectHeader->Size = cbRectSize;
    pFaceRectHeader->Count = pFaceHeader->Count;

    FaceCharacterizationBlobHeader *pFaceCharHeader = (FaceCharacterizationBlobHeader *)pCharBuf;
    pFaceCharHeader->Size = cbCharSize;
    pFaceCharHeader->Count = pFaceHeader->Count;

    FaceRectInfo           *FaceRegions = (FaceRectInfo *)(pFaceRectHeader + 1);
    FaceCharacterization   *FaceChars   = (FaceCharacterization *) (pFaceCharHeader + 1);

    for (UINT i = 0; i < pFaceHeader->Count; i++)
    {
        FaceRegions[i].Region = pFaceData[i].Region;
        FaceRegions[i].confidenceLevel = pFaceData[i].confidenceLevel;

        FaceChars[i].BlinkScoreLeft  = pFaceData[i].BlinkScoreLeft;
        FaceChars[i].BlinkScoreRight = pFaceData[i].BlinkScoreRight;
        FaceChars[i].FacialExpression      = (pFaceData[i].FacialExpression==EXPRESSION_SMILE)?MF_METADATAFACIALEXPRESSION_SMILE:0;
        FaceChars[i].FacialExpressionScore = pFaceData[i].FacialExpressionScore;
    }

    hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROIS, pRectBuf, cbRectSize);
    if( FAILED(hr) )
    {
        goto done;
    }

    MetadataTimeStamps timestamp;
    timestamp.Flags = MF_METADATATIMESTAMPS_DEVICE;
    timestamp.Device = pFaceHeader->Timestamp;

    hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROITIMESTAMPS, (const UINT8 *)&timestamp, sizeof(MetadataTimeStamps));
    if( FAILED(hr) )
    {
        goto done;
    }

    //  Include characterization data if any of the associated bits were set.
    if( pFaceHeader->Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_ADVANCED_MASK )
    {
        hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROICHARACTERIZATIONS , pCharBuf, cbCharSize);
    }

done:
    delete [] pRectBuf;
    delete [] pCharBuf;

    return hr;
}

HRESULT CSocMft0::FillBufferLengthFromMediaType(
    _In_ IMFMediaType *pPreviewType,
    _Inout_ IMFMediaBuffer *pBuffer
)
{
    if(!pPreviewType || !pBuffer)
    {
        return E_INVALIDARG;
    }

    UINT32 uiWidth = 0, uiHeight = 0, uiImageSize = 0;
    GUID guidSubType= {};

    HRESULT hr = MFGetAttributeSize(pPreviewType, MF_MT_FRAME_SIZE, &uiWidth, &uiHeight);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pPreviewType->GetGUID(MF_MT_SUBTYPE, &guidSubType);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = MFCalculateImageSize(guidSubType, uiWidth, uiHeight, &uiImageSize);
    if (FAILED(hr))
    {
        return hr;
    }

    return pBuffer->SetCurrentLength(uiImageSize);
}

HRESULT CSocMft0::GetVideoStabilizationEnabled()
{
    HRESULT hr = S_OK;
    ComPtr<IKsControl> spKsControl;
    KSPROPERTY ksProperty = { 0 };
    KSPROPERTY_CUSTOM_CAMERACONTROL_MFT0_VIDEOSTABILIZATION_S ksData = { 0 };
    ULONG bytesReturned = 0;

    if (m_spSourceTransform != NULL)
    {
        hr = m_spSourceTransform.As(&spKsControl);
        if (SUCCEEDED(hr))
        {
            ksProperty.Set = PROPSETID_VIDCAP_CUSTOM_CAMERACONTROL_MFT0;
            ksProperty.Id = KSPROPERTY_CUSTOM_CAMERACONTROL_MFT0_VIDEOSTABILIZATION;
            ksProperty.Flags = KSPROPERTY_TYPE_GET;

            hr = spKsControl->KsProperty(&ksProperty, sizeof(ksProperty), &ksData, sizeof(ksData), &bytesReturned);
            if (SUCCEEDED(hr))
            {
                m_bEnableVideoStabilization = ksData.VideoStabilizationEnabled;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}
