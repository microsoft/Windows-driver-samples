//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#include "pch.h"

#define NUM_IMAGE_ROWS 480
#define NUM_IMAGE_COLS 640
#define BYTES_PER_PIXEL 4
#define IMAGE_BUFFER_SIZE_BYTES (NUM_IMAGE_ROWS * NUM_IMAGE_COLS * BYTES_PER_PIXEL)
#define IMAGE_ROW_SIZE_BYTES (NUM_IMAGE_COLS * BYTES_PER_PIXEL)

namespace winrt::WindowsSample::implementation
{
    HRESULT SimpleMediaStream::Initialize(
            _In_ SimpleMediaSource* pSource,
            _In_ DWORD dwStreamId,
            _In_ MFSampleAllocatorUsage allocatorUsage
        )
    {
        wil::com_ptr_nothrow<IMFMediaTypeHandler> spTypeHandler;
        wil::com_ptr_nothrow<IMFAttributes> attrs;

        RETURN_HR_IF_NULL(E_INVALIDARG, pSource);
        m_parent = pSource;

        m_dwStreamId = dwStreamId;
        m_allocatorUsage = allocatorUsage;

        const uint32_t NUM_MEDIATYPES = 2;
        wil::unique_cotaskmem_array_ptr<wil::com_ptr_nothrow<IMFMediaType>> mediaTypeList = wilEx::make_unique_cotaskmem_array<wil::com_ptr_nothrow<IMFMediaType>>(NUM_MEDIATYPES);

        // Initialize media type and set the video output media type.
        wil::com_ptr_nothrow<IMFMediaType> spMediaType;
        RETURN_IF_FAILED(MFCreateMediaType(&spMediaType));
        spMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        spMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        spMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        spMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        MFSetAttributeSize(spMediaType.get(), MF_MT_FRAME_SIZE, NUM_IMAGE_COLS, NUM_IMAGE_ROWS);
        MFSetAttributeRatio(spMediaType.get(), MF_MT_FRAME_RATE, 30, 1);
        // frame size * pixle bit size * framerate
        uint32_t bitrate = (uint32_t)(NUM_IMAGE_COLS * 1.5 * NUM_IMAGE_ROWS * 8* 30);
        spMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
        MFSetAttributeRatio(spMediaType.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        mediaTypeList[0] = spMediaType.detach();

        RETURN_IF_FAILED(MFCreateMediaType(&spMediaType));
        spMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        spMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        spMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        spMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        MFSetAttributeSize(spMediaType.get(), MF_MT_FRAME_SIZE, NUM_IMAGE_COLS, NUM_IMAGE_ROWS);
        MFSetAttributeRatio(spMediaType.get(), MF_MT_FRAME_RATE, 30, 1);
        // frame size * pixle bit size * framerate
        bitrate = NUM_IMAGE_COLS * NUM_IMAGE_ROWS * 4 * 8* 30;
        spMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
        MFSetAttributeRatio(spMediaType.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        mediaTypeList[1] = spMediaType.detach();

        RETURN_IF_FAILED(MFCreateAttributes(&m_spAttributes, 10));
        RETURN_IF_FAILED(_SetStreamAttributes(m_spAttributes.get()));

        RETURN_IF_FAILED(MFCreateEventQueue(&m_spEventQueue));

        // Initialize stream descriptors
        RETURN_IF_FAILED(MFCreateStreamDescriptor(m_dwStreamId /*StreamId*/, NUM_MEDIATYPES /*MT count*/, mediaTypeList.get(), &m_spStreamDesc));

        RETURN_IF_FAILED(m_spStreamDesc->GetMediaTypeHandler(&spTypeHandler));
        RETURN_IF_FAILED(spTypeHandler->SetCurrentMediaType(mediaTypeList[0]));
        RETURN_IF_FAILED(_SetStreamDescriptorAttributes(m_spStreamDesc.get()));

        return S_OK;
    }

    // IMFMediaEventGenerator
    IFACEMETHODIMP SimpleMediaStream::BeginGetEvent(
            _In_ IMFAsyncCallback* pCallback,
            _In_ IUnknown* punkState
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->BeginGetEvent(pCallback, punkState));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::EndGetEvent(
            _In_ IMFAsyncResult* pResult,
            _COM_Outptr_ IMFMediaEvent** ppEvent
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->EndGetEvent(pResult, ppEvent));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::GetEvent(
            _In_ DWORD dwFlags,
            _COM_Outptr_ IMFMediaEvent** ppEvent
        )
    {
        // NOTE:
        // GetEvent can block indefinitely, so we don't hold the lock.
        // This requires some juggling with the event queue pointer.

        wil::com_ptr_nothrow<IMFMediaEventQueue> spQueue;

        {
            winrt::slim_lock_guard lock(m_Lock);

            RETURN_IF_FAILED(_CheckShutdownRequiresLock());
            spQueue = m_spEventQueue;
        }

        // Now get the event.
        RETURN_IF_FAILED(spQueue->GetEvent(dwFlags, ppEvent));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::QueueEvent(
            _In_ MediaEventType eventType,
            _In_ REFGUID guidExtendedType,
            _In_ HRESULT hrStatus,
            _In_opt_ PROPVARIANT const* pvValue
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(eventType, guidExtendedType, hrStatus, pvValue));

        return S_OK;
    }

    // IMFMediaStream
    IFACEMETHODIMP SimpleMediaStream::GetMediaSource(
            _COM_Outptr_ IMFMediaSource** ppMediaSource
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, ppMediaSource);
        *ppMediaSource = nullptr;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_parent.copy_to(ppMediaSource));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::GetStreamDescriptor(
            _COM_Outptr_ IMFStreamDescriptor** ppStreamDescriptor
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, ppStreamDescriptor);
        *ppStreamDescriptor = nullptr;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        if (m_spStreamDesc != nullptr)
        {
            RETURN_IF_FAILED(m_spStreamDesc.copy_to(ppStreamDescriptor));
        }
        else
        {
            return E_UNEXPECTED;
        }

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::RequestSample(
            _In_ IUnknown* pToken
        )
    {
        winrt::slim_lock_guard lock(m_Lock);
        wil::com_ptr_nothrow<IMFSample> sample;
        wil::com_ptr_nothrow<IMFMediaBuffer> outputBuffer;
        LONG pitch = 0;
        BYTE* bufferStart = nullptr; // not used
        DWORD bufferLength = 0;
        BYTE* pbuf = nullptr;
        wil::com_ptr_nothrow<IMF2DBuffer2> buffer2D;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        if (m_streamState != MF_STREAM_STATE_RUNNING)
        {
            RETURN_HR_MSG(MF_E_INVALIDREQUEST, "Stream is not in running state, state:%d", m_streamState);
        }

        RETURN_IF_FAILED(m_spSampleAllocator->AllocateSample(&sample));
        RETURN_IF_FAILED(sample->GetBufferByIndex(0, &outputBuffer));
        RETURN_IF_FAILED(outputBuffer->QueryInterface(IID_PPV_ARGS(&buffer2D)));
        RETURN_IF_FAILED(buffer2D->Lock2DSize(MF2DBuffer_LockFlags_Write,
            &pbuf,
            &pitch,
            &bufferStart,
            &bufferLength));
        

        RETURN_IF_FAILED(m_spFrameGenerator->CreateFrame(pbuf, bufferLength, pitch, m_rgbMask));
        RETURN_IF_FAILED(buffer2D->Unlock2D());

        RETURN_IF_FAILED(sample->SetSampleTime(MFGetSystemTime()));
        RETURN_IF_FAILED(sample->SetSampleDuration(333333));
        if (pToken != nullptr)
        {
            RETURN_IF_FAILED(sample->SetUnknown(MFSampleExtension_Token, pToken));
        }
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamUnk(MEMediaSample,
            GUID_NULL,
            S_OK,
            sample.get()));

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // IMFMediaStream2
    IFACEMETHODIMP SimpleMediaStream::SetStreamState(MF_STREAM_STATE state)
    {
        winrt::slim_lock_guard lock(m_Lock);
        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        switch (state)
        {
        case MF_STREAM_STATE_PAUSED:
            // because not supported
            break;

        case MF_STREAM_STATE_RUNNING:
            m_streamState = state;
            break;

        case MF_STREAM_STATE_STOPPED:
            m_streamState = state;

            break;

        default:
            return MF_E_INVALID_STATE_TRANSITION;
            break;
        }

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaStream::GetStreamState(
            _Out_ MF_STREAM_STATE* pState
        )
    {
        winrt::slim_lock_guard lock(m_Lock);
        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        *pState = m_streamState;

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Public methods
    HRESULT SimpleMediaStream::Start()
    {
        winrt::slim_lock_guard lock(m_Lock);

        // Create the allocator if one doesn't exist
        if (m_allocatorUsage == MFSampleAllocatorUsage_UsesProvidedAllocator)
        {
            RETURN_HR_IF_NULL_MSG(E_POINTER, m_spSampleAllocator, "Sample allocator is not set");
        }
        else
        {
            RETURN_IF_FAILED(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&m_spSampleAllocator)));
        }

        wil::com_ptr_nothrow<IMFMediaTypeHandler> spMTHandler;
        RETURN_IF_FAILED(m_spStreamDesc->GetMediaTypeHandler(&spMTHandler));

        wil::com_ptr_nothrow<IMFMediaType> spMediaType;
        UINT32 width, height;
        GUID subType;
        RETURN_IF_FAILED(spMTHandler->GetCurrentMediaType(&spMediaType));
        RETURN_IF_FAILED(spMediaType->GetGUID(MF_MT_SUBTYPE, &subType));
        MFGetAttributeSize(spMediaType.get(), MF_MT_FRAME_SIZE, &width, &height);

        DEBUG_MSG(L"Initialize sample allocator for mediatype: %s, %dx%d ", winrt::to_hstring(subType).data(), width, height);
        RETURN_IF_FAILED(m_spSampleAllocator->InitializeSampleAllocator(10, spMediaType.get()));
        if (m_spFrameGenerator == nullptr)
        {
            m_spFrameGenerator = wil::make_unique_nothrow<SimpleFrameGenerator>();
            RETURN_IF_NULL_ALLOC_MSG(m_spFrameGenerator, "Fail to create SimpleFrameGenerator");
        }
        RETURN_IF_FAILED(m_spFrameGenerator->Initialize(spMediaType.get()));

        // Post MEStreamStarted event to signal stream has started 
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(MEStreamStarted, GUID_NULL, S_OK, nullptr));

        return S_OK;
    }

    HRESULT SimpleMediaStream::Stop()
    {
        winrt::slim_lock_guard lock(m_Lock);

        // Post MEStreamStopped event to signal stream has stopped
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(MEStreamStopped, GUID_NULL, S_OK, nullptr));
        return S_OK;
    }

    HRESULT SimpleMediaStream::Shutdown()
    {
        winrt::slim_lock_guard lock(m_Lock);

        m_bIsShutdown = true;
        m_parent.reset();

        if (m_spEventQueue != nullptr)
        {
            m_spEventQueue->Shutdown();
            m_spEventQueue.reset();
        }

        m_spAttributes.reset();
        m_spStreamDesc.reset();

        m_streamState = MF_STREAM_STATE_STOPPED;

        return S_OK;
    }

    HRESULT SimpleMediaStream::SetSampleAllocator(IMFVideoSampleAllocator* pAllocator)
    {
        winrt::slim_lock_guard lock(m_Lock);

        if (m_streamState == MF_STREAM_STATE_RUNNING)
        {
            RETURN_HR_MSG(MF_E_INVALIDREQUEST, "Cannot update allocator when the stream is streaming");
        }
        m_spSampleAllocator.reset();
        m_spSampleAllocator = pAllocator;

        return S_OK;
    }

    
    //////////////////////////////////////////////////////////////////////////////////////////
    // Private methods

    HRESULT SimpleMediaStream::_CheckShutdownRequiresLock()
    {
        if (m_bIsShutdown)
        {
            return MF_E_SHUTDOWN;
        }

        if (m_spEventQueue == nullptr)
        {
            return E_UNEXPECTED;

        }
        return S_OK;
    }

    HRESULT SimpleMediaStream::_SetStreamAttributes(
            _In_ IMFAttributes* pAttributeStore
        )
    {
        RETURN_HR_IF_NULL(E_INVALIDARG, pAttributeStore);

        RETURN_IF_FAILED(pAttributeStore->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_STREAM_ID, m_dwStreamId));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes::MFFrameSourceTypes_Color));

        return S_OK;
    }

    HRESULT SimpleMediaStream::_SetStreamDescriptorAttributes(
            _In_ IMFAttributes* pAttributeStore
        )
    {
        RETURN_HR_IF_NULL(E_INVALIDARG, pAttributeStore);

        RETURN_IF_FAILED(pAttributeStore->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_STREAM_ID, m_dwStreamId));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
        RETURN_IF_FAILED(pAttributeStore->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes::MFFrameSourceTypes_Color));

        return S_OK;
    }


}
