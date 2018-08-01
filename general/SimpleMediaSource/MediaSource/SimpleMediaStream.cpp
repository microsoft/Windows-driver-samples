#include "SimpleMediaStream.h"

#define NUM_IMAGE_ROWS 240
#define NUM_IMAGE_COLS 320
#define BYTES_PER_PIXEL 4
#define IMAGE_BUFFER_SIZE_BYTES (NUM_IMAGE_ROWS * NUM_IMAGE_COLS * BYTES_PER_PIXEL)
#define IMAGE_ROW_SIZE_BYTES (NUM_IMAGE_COLS * BYTES_PER_PIXEL)

#define CHECKHR_GOTO( val, label )  \
hr = (val); \
if( FAILED( hr ) ) { \
    goto label; \
}

HRESULT SimpleMediaStream::RuntimeClassInitialize(_In_ SimpleMediaSource *pSource)
{
    HRESULT hr = S_OK;
    ComPtr<IMFMediaTypeHandler> spTypeHandler;
    ComPtr<IMFAttributes> attrs;

    AsWeak(pSource, &_wpSource);

    // Initialize media type and set the video output media type.
    CHECKHR_GOTO(MFCreateMediaType(&_spMediaType), done);
    _spMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    _spMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    _spMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    _spMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    MFSetAttributeSize(_spMediaType.Get(), MF_MT_FRAME_SIZE, NUM_IMAGE_COLS, NUM_IMAGE_ROWS);
    MFSetAttributeRatio(_spMediaType.Get(), MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(_spMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    CHECKHR_GOTO(MFCreateAttributes(&_spAttributes, 10), done);
    CHECKHR_GOTO(this->_SetStreamAttributes(_spAttributes.Get()), done);
    CHECKHR_GOTO(MFCreateEventQueue(&_spEventQueue), done);

    // Initialize stream descriptors
    CHECKHR_GOTO(MFCreateStreamDescriptor(0, 1, _spMediaType.GetAddressOf(), &_spStreamDesc), done);

    CHECKHR_GOTO(_spStreamDesc->GetMediaTypeHandler(&spTypeHandler), done);
    CHECKHR_GOTO(spTypeHandler->SetCurrentMediaType(_spMediaType.Get()), done);
    CHECKHR_GOTO(this->_SetStreamDescriptorAttributes(_spStreamDesc.Get()), done);

done:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IMFMediaEventGenerator
IFACEMETHODIMP SimpleMediaStream::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->BeginGetEvent(pCallback, punkState);
    }

    return hr;
}

IFACEMETHODIMP SimpleMediaStream::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->EndGetEvent(pResult, ppEvent);
    }

    return hr;
}

IFACEMETHODIMP SimpleMediaStream::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent)
{
    // NOTE:
    // GetEvent can block indefinitely, so we don't hold the lock.
    // This requires some juggling with the event queue pointer.

    HRESULT hr = S_OK;

    ComPtr<IMFMediaEventQueue> spQueue;

    {
        auto lock = _critSec.Lock();

        // Check shutdown
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

IFACEMETHODIMP SimpleMediaStream::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();
    if (SUCCEEDED(hr))
    {
        hr = _spEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// IMFMediaStream
IFACEMETHODIMP SimpleMediaStream::GetMediaSource(IMFMediaSource **ppMediaSource)
{
    if (ppMediaSource == nullptr)
    {
        return E_POINTER;
    }
    *ppMediaSource = nullptr;

    auto lock = _critSec.Lock();
    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        ComPtr<SimpleMediaSource> spSource;
        _wpSource.As(&spSource);
        if (spSource != nullptr)
        {
            hr = spSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

IFACEMETHODIMP SimpleMediaStream::GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor)
{
    if (ppStreamDescriptor == nullptr)
    {
        return E_POINTER;
    }

    *ppStreamDescriptor = nullptr;

    auto lock = _critSec.Lock();
    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        if (_spStreamDesc != nullptr)
        {
            *ppStreamDescriptor = _spStreamDesc.Get();
            (*ppStreamDescriptor)->AddRef();
        }
        else
        {
            return E_UNEXPECTED;
        }
    }

    return hr;
}

/*
   Writes to a buffer representing a 2D image.
   Writes a different constant to each line based on row number and current time.

   Assumes top down image, no negative stride and pBuf points to the begnning of the buffer of length len.

   Param:
   pBuf - pointer to beginning of buffer
   pitch - line length in bytes
   len - length of buffer in bytes
*/
HRESULT WriteSampleData(BYTE *pBuf, LONG pitch, DWORD len)
{
    if (pBuf == nullptr)
    {
        return E_POINTER;
    }

    const int NUM_ROWS = len / abs(pitch);

    LONGLONG curSysTimeInS = MFGetSystemTime() / (MFTIME)10000000;
    int offset = curSysTimeInS % NUM_ROWS;

    for (int r = 0; r < NUM_ROWS; r++)
    {
        int grayColor = r + offset;
        memset(pBuf + (pitch * r), (BYTE)(grayColor % NUM_ROWS), pitch);
    }

    return S_OK;
}

IFACEMETHODIMP SimpleMediaStream::RequestSample(IUnknown *pToken)
{
    auto lock = _critSec.Lock();

    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        ComPtr<IMFSample> spSample;

        ComPtr<IMFMediaBuffer> spOutputBuffer;
        hr = MFCreate2DMediaBuffer(NUM_IMAGE_COLS, NUM_IMAGE_ROWS, D3DFMT_X8R8G8B8, false, &spOutputBuffer);

        LONG pitch = IMAGE_ROW_SIZE_BYTES;
        BYTE *bufferStart = nullptr; // not used, since in this example we know the image will be top down, so bufferStart == pBuf
        DWORD bufferLength = IMAGE_BUFFER_SIZE_BYTES;

        BYTE *pBuf = nullptr;
        ComPtr<IMF2DBuffer2> spBuffer2D;
        if (SUCCEEDED(hr))
        {
            hr = spOutputBuffer.As(&spBuffer2D);
        }

        if (SUCCEEDED(hr))
        {
            if (spBuffer2D != nullptr)
            {
                hr = spBuffer2D->Lock2DSize(MF2DBuffer_LockFlags_Write, &pBuf, &pitch, &bufferStart, &bufferLength);
            }
            else
            {
                hr = spOutputBuffer->Lock(&pBuf, nullptr, nullptr);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = WriteSampleData(pBuf, pitch, bufferLength);
        }

        if (SUCCEEDED(hr))
        {
            hr = spOutputBuffer->SetCurrentLength(bufferLength);
        }

        if (SUCCEEDED(hr))
        {
            hr = spBuffer2D->Unlock2D();
        }

        if (SUCCEEDED(hr))
        {
            hr = MFCreateSample(&spSample);
        }

        if (SUCCEEDED(hr))
        {
            hr = spSample->AddBuffer(spOutputBuffer.Get());
        }

        // set timestamp
        if (SUCCEEDED(hr))
        {
            hr = spSample->SetSampleTime(MFGetSystemTime());
        }
        if (SUCCEEDED(hr))
        {
            hr = spSample->SetSampleDuration(330000);
        }

        if (pToken != nullptr)
        {
            hr = spSample->SetUnknown(MFSampleExtension_Token, pToken);
        }

        if (SUCCEEDED(hr))
        {
            hr = _spEventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, spSample.Get());
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// IMFMediaStream2
IFACEMETHODIMP SimpleMediaStream::SetStreamState(MF_STREAM_STATE state)
{
    bool runningState = false;
    auto lock = _critSec.Lock();

    HRESULT hr = S_OK;

    CHECKHR_GOTO(_CheckShutdownRequiresLock(), done);

    switch (state)
    {
    case MF_STREAM_STATE_PAUSED:
        goto done; // because not supported
    case MF_STREAM_STATE_RUNNING:
        runningState = true;
        break;
    case MF_STREAM_STATE_STOPPED:
        runningState = false;
        break;
    default:
        hr = MF_E_INVALID_STATE_TRANSITION;
        break;
    }

    _isSelected = runningState;

done:
    return hr;
}

IFACEMETHODIMP SimpleMediaStream::GetStreamState(_Out_ MF_STREAM_STATE *pState)
{
    auto lock = _critSec.Lock();
    BOOLEAN pauseState = false;

    HRESULT hr = _CheckShutdownRequiresLock();

    if (SUCCEEDED(hr))
    {
        *pState = (_isSelected ? MF_STREAM_STATE_RUNNING : MF_STREAM_STATE_STOPPED);
    }

    return hr;
}

HRESULT SimpleMediaStream::Shutdown()
{
    HRESULT hr = S_OK;
    auto lock = _critSec.Lock();

    _isShutdown = true;

    if (_spEventQueue != nullptr)
    {
        hr = _spEventQueue->Shutdown();
        _spEventQueue.Reset();
    }

    _spAttributes.Reset();
    _spMediaType.Reset();
    _spStreamDesc.Reset();

    _isSelected = false;

    _wpSource.Reset();

    return hr;
}

HRESULT SimpleMediaStream::_CheckShutdownRequiresLock()
{
    if (_isShutdown)
    {
        return MF_E_SHUTDOWN;
    }

    if (_spEventQueue == nullptr)
    {
        return E_UNEXPECTED;

    }
    return S_OK;
}

HRESULT SimpleMediaStream::_SetStreamAttributes(IMFAttributes *pAttributeStore)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE); }
    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_STREAM_ID, STREAMINDEX); }
    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1); }

    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, _MFFrameSourceTypes::MFFrameSourceTypes_Color); }

    return hr;
}

HRESULT SimpleMediaStream::_SetStreamDescriptorAttributes(IMFAttributes *pAttributeStore)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE); }
    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_STREAM_ID, STREAMINDEX); }
    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1); }

    if (SUCCEEDED(hr)) { hr = pAttributeStore->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, _MFFrameSourceTypes::MFFrameSourceTypes_Color); }

    return hr;
}