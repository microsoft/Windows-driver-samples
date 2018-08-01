#include "SimpleMediaSource.h"
#include "SimpleMediaStream.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT SimpleMediaSource::RuntimeClassInitialize()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        hr = MFCreateAttributes(&_spAttributes, 10);
    }

    if (SUCCEEDED(hr))
    {
        hr = MFCreateEventQueue(&_spEventQueue);
    }

    if (SUCCEEDED(hr))
    {
        hr = MakeAndInitialize<SimpleMediaStream>(&_stream, this);
    }

    if (SUCCEEDED(hr))
    {
        ComPtr<IMFStreamDescriptor> streamDescriptor(_stream.Get()->_spStreamDesc.Get());
        hr = MFCreatePresentationDescriptor(NUM_STREAMS, streamDescriptor.GetAddressOf(), &_spPresentationDescriptor);
    }

    if (SUCCEEDED(hr))
    {
        _wasStreamPreviouslySelected = false;
        _sourceState = SourceState::Stopped;
    }

    return hr;
}

// IMFMediaEventGenerator methods.
///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->BeginGetEvent(pCallback, punkState);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->EndGetEvent(pResult, ppEvent);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
{
    // NOTE:
    // GetEvent can block indefinitely, so we don't hold the lock.
    // This requires some juggling with the event queue pointer.

    HRESULT hr = S_OK;

    ComPtr<IMFMediaEventQueue> spQueue;

    {
        auto lock = _critSec.Lock();

        hr = _CheckShutdownRequiresLock();
        if (SUCCEEDED(hr))
        {
            spQueue = _spEventQueue;
        }
    }

    // Now get the event.
    if (SUCCEEDED(hr))
    {
        hr = spQueue->GetEvent(dwFlags, ppEvent);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::QueueEvent(
    MediaEventType eventType,
    REFGUID guidExtendedType,
    HRESULT hrStatus,
    _In_opt_ PROPVARIANT const *pvValue)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->QueueEventParamVar(eventType, guidExtendedType, hrStatus, pvValue);
    }

    return hr;
}

// IMFMediaSource methods
///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::CreatePresentationDescriptor(
    IMFPresentationDescriptor **ppPresentationDescriptor)
{
    if (ppPresentationDescriptor == nullptr)
    {
        return E_POINTER;
    }

    *ppPresentationDescriptor = nullptr;

    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        hr = _spPresentationDescriptor->Clone(ppPresentationDescriptor);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::GetCharacteristics(DWORD *pdwCharacteristics)
{
    *pdwCharacteristics = 0;

    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        *pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::Pause()
{
    // Pause() not required/needed for live sources
    HRESULT hr = MF_E_INVALID_STATE_TRANSITION;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::Shutdown()
{
    HRESULT hr = S_OK;
    {
        auto lock = _critSec.Lock();

        _sourceState = SourceState::Shutdown;

        _spAttributes.Reset();
        _spPresentationDescriptor.Reset();

        if (_spEventQueue != nullptr)
        {
            _spEventQueue->Shutdown();
            _spEventQueue.Reset();
        }

        if (_stream != nullptr)
        {
            _stream.Get()->Shutdown();
            _stream.Reset();
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::Start(
    _In_ IMFPresentationDescriptor *pPresentationDescriptor,
    _In_opt_ const GUID *pguidTimeFormat,
    _In_ const PROPVARIANT *pvarStartPos)
{
    HRESULT hr = S_OK;

    if (pPresentationDescriptor == nullptr || pvarStartPos == nullptr)
    {
        hr = E_INVALIDARG;
    }
    else if (pguidTimeFormat != nullptr && *pguidTimeFormat != GUID_NULL)
    {
        hr = MF_E_UNSUPPORTED_TIME_FORMAT;
    }

    do
    {
        BREAK_ON_FAIL(hr);

        auto lock = _critSec.Lock();
        BREAK_ON_FAIL(hr = _CheckShutdownRequiresLock());

        if (_sourceState != SourceState::Stopped)
        {
            hr = MF_E_INVALID_STATE_TRANSITION;
            break;
        }

        _sourceState = SourceState::Started;

        // This checks the passed in PresentationDescriptor matches the member of streams we
        // have defined internally and that at least one stream is selected
        BREAK_ON_FAIL(hr = _ValidatePresentationDescriptor(pPresentationDescriptor));

        DWORD count = 0;
        BREAK_ON_FAIL(hr = pPresentationDescriptor->GetStreamDescriptorCount(&count));

        PROPVARIANT startTime;
        BREAK_ON_FAIL(hr = InitPropVariantFromInt64(MFGetSystemTime(), &startTime));

        // Send event that the source started. Include error code in case it failed.
        BREAK_ON_FAIL(hr = _spEventQueue->QueueEventParamVar(MESourceStarted, GUID_NULL, hr, &startTime));

        // Open and un-pause the selected stream(s)
        BOOL selected = false;
        ComPtr<IMFStreamDescriptor> spStreamDescriptor;
        BREAK_ON_FAIL(hr = pPresentationDescriptor->GetStreamDescriptorByIndex(0, &selected, &spStreamDescriptor));

        DWORD streamIndex = 0;
        BREAK_ON_FAIL(hr = spStreamDescriptor->GetStreamIdentifier(&streamIndex));

        if (streamIndex >= NUM_STREAMS)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
            break;
        }

        if (selected)
        {
            // Update our internal PresentationDescriptor
            BREAK_ON_FAIL(hr = _spPresentationDescriptor->SelectStream(streamIndex));
            BREAK_ON_FAIL(hr = _stream.Get()->SetStreamState(MF_STREAM_STATE_RUNNING));

            ComPtr<IUnknown> spunkStream;
            BREAK_ON_FAIL(hr = _stream.As(&spunkStream));

            BREAK_ON_FAIL(hr = _spEventQueue->QueueEventParamUnk(_wasStreamPreviouslySelected ? MEUpdatedStream : MENewStream,
                GUID_NULL, S_OK, spunkStream.Get()));
            BREAK_ON_FAIL(hr = _stream.Get()->QueueEvent(MEStreamStarted, GUID_NULL, S_OK, &startTime));
        }
        _wasStreamPreviouslySelected = selected;

    } while (false);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::Stop()
{
    HRESULT hr = S_OK;

    do
    {
        auto lock = _critSec.Lock();

        if (_sourceState != SourceState::Started)
        {
            BREAK_ON_FAIL(hr = MF_E_INVALID_STATE_TRANSITION);
        }

        BREAK_ON_FAIL(hr = _CheckShutdownRequiresLock());

        PROPVARIANT stopTime;
        BREAK_ON_FAIL(hr = InitPropVariantFromInt64(MFGetSystemTime(), &stopTime));

        DWORD count = 0;
        BREAK_ON_FAIL(hr = _spPresentationDescriptor->GetStreamDescriptorCount(&count));
        // Deselect the streams and send the stream stopped events.
        MF_STREAM_STATE state;
        hr = _stream.Get()->GetStreamState(&state);
        if (FAILED(hr))
        {
            continue;
        }
        _wasStreamPreviouslySelected = (state == MF_STREAM_STATE_RUNNING);
        hr = _stream.Get()->SetStreamState(MF_STREAM_STATE_STOPPED);
        if (FAILED(hr))
        {
            continue;
        }
        _spPresentationDescriptor->DeselectStream(0);
        hr = _stream.Get()->QueueEvent(MEStreamStopped, GUID_NULL, hr, &stopTime);
        if (FAILED(hr))
        {
            continue;
        }

        BREAK_ON_FAIL(hr = _spEventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, hr, &stopTime));

    } while (false);

    return hr;
}

// IMFMediaSourceEx
///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::GetSourceAttributes(_Outptr_ IMFAttributes **ppAttributes)
{
    if (ppAttributes == nullptr)
    {
        return E_POINTER;
    }

    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        *ppAttributes = _spAttributes.Get();
        (*ppAttributes)->AddRef();
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::GetStreamAttributes(DWORD dwStreamIdentifier, _Outptr_ IMFAttributes **ppAttributes)
{
    if (ppAttributes == nullptr)
    {
        return E_POINTER;
    }

    auto lock = _critSec.Lock();

    *ppAttributes = nullptr;

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        if (dwStreamIdentifier >= NUM_STREAMS)
        {
            hr = MF_E_INVALIDSTREAMNUMBER;
        }
        else
        {
            *ppAttributes = _stream.Get()->_spAttributes.Get();
            (*ppAttributes)->AddRef();
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP SimpleMediaSource::SetD3DManager(_In_opt_ IUnknown* /*pManager*/)
{
    // No need to implement this method in our case.
    HRESULT hr = E_NOTIMPL;
    return hr;
}

// IMFGetService methods
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
IFACEMETHODIMP SimpleMediaSource::GetService(REFGUID guidService, REFIID riid, LPVOID * ppvObject)
{
    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        if (!ppvObject)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            *ppvObject = NULL;
        }

        hr = MF_E_UNSUPPORTED_SERVICE;
    }

    return hr;
}

// IKsControl methods
_Use_decl_annotations_
IFACEMETHODIMP SimpleMediaSource::KsProperty(
    PKSPROPERTY pProperty,
    ULONG ulPropertyLength,
    LPVOID pPropertyData,
    ULONG ulDataLength,
    ULONG* pBytesReturned)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
IFACEMETHODIMP SimpleMediaSource::KsMethod(
    PKSMETHOD pMethod,
    ULONG ulMethodLength,
    LPVOID pMethodData,
    ULONG ulDataLength,
    ULONG* pBytesReturned)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
IFACEMETHODIMP SimpleMediaSource::KsEvent(
    _In_opt_ PKSEVENT pEvent,
    _In_ ULONG ulEventLength,
    _Inout_opt_ LPVOID pEventData,
    _In_ ULONG ulDataLength,
    _Out_opt_ ULONG* pBytesReturned)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT SimpleMediaSource::_CheckShutdownRequiresLock()
{
    if (_sourceState == SourceState::Shutdown)
    {
        return MF_E_SHUTDOWN;
    }

    if (_spEventQueue == nullptr || _stream == nullptr)
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT SimpleMediaSource::_ValidatePresentationDescriptor(IMFPresentationDescriptor *pPD)
{
    if (pPD == nullptr)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    DWORD cStreams = 0;
    bool anySelected = false;

    // The caller's PD must have the same number of streams as ours.
    hr = pPD->GetStreamDescriptorCount(&cStreams);

    if (SUCCEEDED(hr) && (cStreams != NUM_STREAMS))
    {
        hr = E_INVALIDARG;
    }

    // The caller must select at least one stream.
    for (UINT32 i = 0; SUCCEEDED(hr) && i < cStreams; ++i)
    {
        ComPtr<IMFStreamDescriptor> spSD;
        BOOL fSelected = FALSE;
        hr = pPD->GetStreamDescriptorByIndex(i, &fSelected, &spSD);

        if (SUCCEEDED(hr))
        {
            anySelected |= !!fSelected;

            DWORD dwId = 0;
            hr = spSD->GetStreamIdentifier(&dwId);

            if (SUCCEEDED(hr) && dwId >= NUM_STREAMS)
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if (!anySelected)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}