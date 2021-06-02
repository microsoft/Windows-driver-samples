//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
#include "pch.h"

namespace winrt::WindowsSample::implementation
{
    /////////////////////////////////////////////////////////////////////////////////
    HRESULT SimpleMediaSource::Initialize()
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CreateSourceAttributes());
        RETURN_IF_FAILED(MFCreateEventQueue(&m_spEventQueue));

        m_streamList = wilEx::make_unique_cotaskmem_array<wil::com_ptr_nothrow<SimpleMediaStream>>(NUM_STREAMS);
        RETURN_IF_NULL_ALLOC(m_streamList.get());

        wil::unique_cotaskmem_array_ptr<wil::com_ptr_nothrow<IMFStreamDescriptor>> streamDescriptorList = wilEx::make_unique_cotaskmem_array<wil::com_ptr_nothrow<IMFStreamDescriptor>>(NUM_STREAMS);

        // This example showcase one stream (i.e. NUM_STREAMS == 1),
        // it can be extended to multiple streams by changing NUM_STREAMS
        for (unsigned int i = 0; i < NUM_STREAMS; i++)
        {
            auto ptr = winrt::make_self<SimpleMediaStream>();
            m_streamList[i] = ptr.detach();
            RETURN_IF_FAILED(m_streamList[i]->Initialize(this, i, MFSampleAllocatorUsage_UsesProvidedAllocator));

            RETURN_IF_FAILED(m_streamList[i]->GetStreamDescriptor(&streamDescriptorList[i]));
        }

        RETURN_IF_FAILED(MFCreatePresentationDescriptor(m_streamList.size(), streamDescriptorList.get(), &m_spPresentationDescriptor));

        m_sourceState = SourceState::Stopped;

        return S_OK;
    }

    // IMFMediaEventGenerator methods.
    IFACEMETHODIMP SimpleMediaSource::BeginGetEvent(
            _In_ IMFAsyncCallback* pCallback,
            _In_ IUnknown* punkState )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->BeginGetEvent(pCallback, punkState));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::EndGetEvent(
            _In_ IMFAsyncResult* pResult,
            _COM_Outptr_ IMFMediaEvent** ppEvent
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->EndGetEvent(pResult, ppEvent));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::GetEvent(
            DWORD dwFlags,
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

    IFACEMETHODIMP SimpleMediaSource::QueueEvent(
            MediaEventType eventType,
            REFGUID guidExtendedType,
            HRESULT hrStatus,
            _In_opt_ PROPVARIANT const* pvValue
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(eventType, guidExtendedType, hrStatus, pvValue));

        return S_OK;
    }

    // IMFMediaSource methods
    IFACEMETHODIMP SimpleMediaSource::CreatePresentationDescriptor(
            _COM_Outptr_ IMFPresentationDescriptor** ppPresentationDescriptor
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, ppPresentationDescriptor);
        *ppPresentationDescriptor = nullptr;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        RETURN_IF_FAILED(m_spPresentationDescriptor->Clone(ppPresentationDescriptor));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::GetCharacteristics(
            _Out_ DWORD* pdwCharacteristics
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, pdwCharacteristics);
        *pdwCharacteristics = 0;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());
        *pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::Pause()
    {
        // Pause() not required/needed
        return MF_E_INVALID_STATE_TRANSITION;
    }


    IFACEMETHODIMP SimpleMediaSource::Shutdown()
    {
        winrt::slim_lock_guard lock(m_Lock);

        m_sourceState = SourceState::Shutdown;

        m_spAttributes.reset();
        m_spPresentationDescriptor.reset();

        if (m_spEventQueue != nullptr)
        {
            m_spEventQueue->Shutdown();
            m_spEventQueue.reset();
        }

        for(unsigned int i = 0; i < m_streamList.size(); i++)
        {
            m_streamList[i]->Shutdown();
        }
        m_streamList.reset();

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::Start(
            _In_ IMFPresentationDescriptor* pPresentationDescriptor,
            _In_opt_ const GUID* pguidTimeFormat,
            _In_ const PROPVARIANT* pvarStartPos
        )
    {
        winrt::slim_lock_guard lock(m_Lock);
        DWORD count = 0;
        wil::unique_prop_variant startTime;

        if (pPresentationDescriptor == nullptr || pvarStartPos == nullptr)
        {
            return E_INVALIDARG;
        }

        if (pguidTimeFormat != nullptr && *pguidTimeFormat != GUID_NULL)
        {
            return MF_E_UNSUPPORTED_TIME_FORMAT;
        }

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        if (!(m_sourceState != SourceState::Stopped || m_sourceState != SourceState::Shutdown))
        {
            return MF_E_INVALID_STATE_TRANSITION;
        }
        m_sourceState = SourceState::Started;

        // This checks the passed in PresentationDescriptor matches the member of streams we
        // have defined internally and that at least one stream is selected
        RETURN_IF_FAILED(_ValidatePresentationDescriptor(pPresentationDescriptor));
        RETURN_IF_FAILED(pPresentationDescriptor->GetStreamDescriptorCount(&count));
        RETURN_IF_FAILED(InitPropVariantFromInt64(MFGetSystemTime(), &startTime));

        // We're hardcoding this to the first descriptor
        // since this sample is a single stream sample.  For
        // multiple streams, we need to walk the list of streams
        // and for each selected stream, send the MEUpdatedStream
        // or MENewStream event along with the MEStreamStarted
        // event.
        for (unsigned int i = 0; i < count; i++)
        {
            BOOL selected = false;
            wil::com_ptr_nothrow<IMFStreamDescriptor> streamDesc;
            RETURN_IF_FAILED(pPresentationDescriptor->GetStreamDescriptorByIndex(
                i,
                &selected,
                &streamDesc));

            DWORD streamId = 0;
            RETURN_IF_FAILED(streamDesc->GetStreamIdentifier(&streamId));

            DWORD streamIdx = 0;
            bool wasSelected = false;
            wil::com_ptr_nothrow<IMFStreamDescriptor> spLocalStreamDescriptor;
            RETURN_IF_FAILED(_GetStreamDescriptorByStreamId(streamId, &streamIdx, &wasSelected, &spLocalStreamDescriptor));
            
            if (selected)
            {
                // Update our internal PresentationDescriptor
                RETURN_IF_FAILED(m_spPresentationDescriptor->SelectStream(streamIdx));

                // Update stream state
                RETURN_IF_FAILED(m_streamList[streamIdx]->SetStreamState(MF_STREAM_STATE_RUNNING));
                RETURN_IF_FAILED(m_streamList[streamIdx]->Start());

                // Send the MEUpdatedStream/MENewStream to our source event
                // queue.
                wil::com_ptr_nothrow<IUnknown> spunkStream;
                MediaEventType met = (wasSelected ? MEUpdatedStream : MENewStream);
                RETURN_IF_FAILED(m_streamList[streamIdx]->QueryInterface(IID_PPV_ARGS(&spunkStream)));
                RETURN_IF_FAILED(m_spEventQueue->QueueEventParamUnk(
                    met,
                    GUID_NULL,
                    S_OK,
                    spunkStream.get()));
            }
            else if(wasSelected)
            {
                // stream was previously selected but not selected this time.
                RETURN_IF_FAILED(m_spPresentationDescriptor->DeselectStream(streamIdx));
            }
        }

        // Send event that the source started. Include error code in case it failed.
        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(
            MESourceStarted,
            GUID_NULL,
            S_OK,
            &startTime));

        return S_OK;
    }

    HRESULT SimpleMediaSource::_GetStreamDescriptorByStreamId(DWORD dwStreamId, DWORD* pdwStreamIdx, bool* pSelected, IMFStreamDescriptor** ppStreamDescriptor)
    {
        RETURN_HR_IF_NULL(E_POINTER, ppStreamDescriptor);
        *ppStreamDescriptor = nullptr;

        RETURN_HR_IF_NULL(E_POINTER, pdwStreamIdx);
        *pdwStreamIdx = 0;

        RETURN_HR_IF_NULL(E_POINTER, pSelected);
        *pSelected = false;

        DWORD streamCount = 0;
        RETURN_IF_FAILED(m_spPresentationDescriptor->GetStreamDescriptorCount(&streamCount));
        for (unsigned int i = 0; i < streamCount; i++)
        {
            wil::com_ptr_nothrow<IMFStreamDescriptor> spStreamDescriptor;
            BOOL selected = FALSE;

            RETURN_IF_FAILED(m_spPresentationDescriptor->GetStreamDescriptorByIndex(i, &selected, &spStreamDescriptor));

            DWORD id = 0;
            RETURN_IF_FAILED(spStreamDescriptor->GetStreamIdentifier(&id));

            if (dwStreamId == id)
            {
                // Found the streamDescriptor with matching streamId
                *pdwStreamIdx = i;
                *pSelected = !!selected;
                RETURN_IF_FAILED(spStreamDescriptor.copy_to(ppStreamDescriptor));
                return S_OK;
            }
        }

        return MF_E_NOT_FOUND;
    }

    IFACEMETHODIMP SimpleMediaSource::Stop()
    {
        winrt::slim_lock_guard lock(m_Lock);

        if (m_sourceState != SourceState::Started)
        {
            return MF_E_INVALID_STATE_TRANSITION;
        }
        m_sourceState = SourceState::Stopped;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        wil::unique_prop_variant stopTime;
        RETURN_IF_FAILED(InitPropVariantFromInt64(MFGetSystemTime(), &stopTime));

        // Deselect the streams and send the stream stopped events.
        for (unsigned int i = 0; i < m_streamList.size(); i++)
        {
            RETURN_IF_FAILED(m_streamList[i]->SetStreamState(MF_STREAM_STATE_STOPPED));
            RETURN_IF_FAILED(m_streamList[i]->Stop());
            RETURN_IF_FAILED(m_spPresentationDescriptor->DeselectStream(i));
        }

        RETURN_IF_FAILED(m_spEventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, &stopTime));

        return S_OK;
    }

    // IMFMediaSourceEx
    IFACEMETHODIMP SimpleMediaSource::GetSourceAttributes(_COM_Outptr_ IMFAttributes** sourceAttributes)
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, sourceAttributes);
        *sourceAttributes = nullptr;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        RETURN_IF_FAILED(MFCreateAttributes(sourceAttributes, 1));
        RETURN_IF_FAILED(m_spAttributes->CopyAllItems(*sourceAttributes));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::GetStreamAttributes(
            _In_ DWORD dwStreamIdentifier,
            _COM_Outptr_ IMFAttributes** ppAttributes
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_HR_IF_NULL(E_POINTER, ppAttributes);
        *ppAttributes = nullptr;

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        wil::com_ptr_nothrow<SimpleMediaStream> spStream;
        RETURN_IF_FAILED(_GetMediaStreamById(dwStreamIdentifier, &spStream));
        RETURN_IF_FAILED(spStream->m_spAttributes.copy_to(ppAttributes));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::SetD3DManager(
            _In_opt_ IUnknown* /*pManager*/
        )
    {
        // Return code is ignored by the frame work, this is a
        // best effort attempt to inform the media source of the
        // DXGI manager to use if DX surface support is available.

        return E_NOTIMPL;
    }

    // IMFGetService methods
    _Use_decl_annotations_
    IFACEMETHODIMP SimpleMediaSource::GetService(
            _In_ REFGUID guidService,
            _In_ REFIID riid,
            _Out_ LPVOID* ppvObject
        )
    {
        winrt::slim_lock_guard lock(m_Lock);

        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        RETURN_HR_IF_NULL(E_POINTER, ppvObject);
        *ppvObject = nullptr;

        // We have no supported service, just return
        // MF_E_UNSUPPORTED_SERVICE for all calls.
        return MF_E_UNSUPPORTED_SERVICE;
    }

    // IKsControl methods
    _Use_decl_annotations_
    IFACEMETHODIMP SimpleMediaSource::KsProperty(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_to_(ulDataLength, *pBytesReturned) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Out_ ULONG* pBytesReturned
        )
    {
        if (ulPropertyLength < sizeof(KSPROPERTY))
        {
            return E_INVALIDARG;
        }
        
        // ERROR_SET_NOT_FOUND is the standard error code returned
        // by the AV Stream driver framework when a miniport
        // driver does not register a handler for a KS operation.
        // We want to mimic the driver behavior here if we don't
        // support controls.
        if ((pProperty->Set != PROPSETID_SIMPLEMEDIASOURCE_CUSTOMCONTROL) || (pProperty->Id >= KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_END))
        {
            return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
        }


        if (pPropertyData == NULL && ulDataLength == 0)
        {
            //
            // If both PropertyData and DataLength, this function needs 
            // to return set BytesReturns to buffer size and
            // return HRESULT_FROM_WIN32(ERROR_MORE_DATA) 
            // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ks/nf-ks-ikscontrol-ksproperty
            //
            RETURN_HR_IF_NULL(E_POINTER, pBytesReturned);

            switch (pProperty->Id)
            {
            case KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE:
                *pBytesReturned = sizeof(KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S);
                break;

            default:
                return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
                break;
            }
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
        else
        {
            RETURN_HR_IF_NULL(E_POINTER, pPropertyData);
            RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), ulDataLength == 0);
            RETURN_HR_IF_NULL(E_POINTER, pBytesReturned);

            // validate properyData length
            switch (pProperty->Id)
            {
            case KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE:
                if (ulDataLength < sizeof(KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S))
                {
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }

                // Set operation 
                if (0 != (pProperty->Flags & (KSPROPERTY_TYPE_SET)))
                {
                    DEBUG_MSG(L"Set filter level KSProperty");
                    *pBytesReturned = 0;
                    for (size_t i = 0; i < m_streamList.size(); i++)
                    {
                        m_streamList[i]->SetRGBMask(((PKSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S)pPropertyData)->ColorMode);
                        *pBytesReturned = sizeof(KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S);
                    }
                }
                // Get operation
                else if (0 != (pProperty->Flags & (KSPROPERTY_TYPE_GET)))
                {
                    DEBUG_MSG(L"Get filter level KSProperty");
                    ((PKSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S)pPropertyData)->ColorMode = m_streamList[0]->GetRGBMask();
                    *pBytesReturned = sizeof(KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_S);
                }
                else
                {
                    return E_INVALIDARG;
                }
                break;

            default:
                break;
            }
        }

        return S_OK;
    }

    _Use_decl_annotations_
    IFACEMETHODIMP SimpleMediaSource::KsMethod(
        _In_reads_bytes_(ulMethodLength) PKSMETHOD pMethod,
        _In_ ULONG ulMethodLength,
        _Inout_updates_to_(ulDataLength, *pBytesReturned) LPVOID pMethodData,
        _In_ ULONG ulDataLength,
        _Out_ ULONG* pBytesReturned
        )
    {
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    _Use_decl_annotations_
    IFACEMETHODIMP SimpleMediaSource::KsEvent(
        _In_reads_bytes_opt_(ulEventLength) PKSEVENT pEvent,
        _In_ ULONG ulEventLength,
        _Inout_updates_to_(ulDataLength, *pBytesReturned) LPVOID pEventData,
        _In_ ULONG ulDataLength,
        _Out_opt_ ULONG* pBytesReturned
        )
    {
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    IFACEMETHODIMP SimpleMediaSource::SetDefaultAllocator(
        _In_  DWORD dwOutputStreamID,
        _In_  IUnknown* pAllocator)
    {
        winrt::slim_lock_guard lock(m_Lock);
        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        wil::com_ptr_nothrow<IMFVideoSampleAllocator> spAllocator;
        RETURN_IF_FAILED(pAllocator->QueryInterface(IID_PPV_ARGS(&spAllocator)));

        wil::com_ptr_nothrow<SimpleMediaStream> spStream;
        RETURN_IF_FAILED(_GetMediaStreamById(dwOutputStreamID, &spStream));
        RETURN_IF_FAILED(spStream->SetSampleAllocator(spAllocator.get()));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSource::GetAllocatorUsage(
        _In_  DWORD dwOutputStreamID,
        _Out_  DWORD* pdwInputStreamID,
        _Out_  MFSampleAllocatorUsage* peUsage)
    {
        winrt::slim_lock_guard lock(m_Lock);
        RETURN_IF_FAILED(_CheckShutdownRequiresLock());

        RETURN_HR_IF_NULL(E_POINTER, pdwInputStreamID);
        RETURN_HR_IF_NULL(E_POINTER, peUsage);

        wil::com_ptr_nothrow<SimpleMediaStream> spStream;
        RETURN_IF_FAILED(_GetMediaStreamById(dwOutputStreamID, &spStream));

        *peUsage = spStream->SampleAlloactorUsage();
        *pdwInputStreamID = dwOutputStreamID;

        return S_OK;
    }

    /// Internal methods.
    HRESULT SimpleMediaSource::_CheckShutdownRequiresLock()
    {
        if (m_sourceState == SourceState::Shutdown)
        {
            return MF_E_SHUTDOWN;
        }

        if (m_spEventQueue == nullptr || m_streamList.get() == nullptr)
        {
            return E_UNEXPECTED;
        }

        return S_OK;
    }

    HRESULT SimpleMediaSource::_ValidatePresentationDescriptor(_In_ IMFPresentationDescriptor* pPD)
    {
        DWORD cStreams = 0;
        bool anySelected = false;

        RETURN_HR_IF_NULL(E_INVALIDARG, pPD);

        // The caller's PD must have the same number of streams as ours.
        RETURN_IF_FAILED(pPD->GetStreamDescriptorCount(&cStreams));
        if (cStreams != m_streamList.size())
        {
            return E_INVALIDARG;
        }

        // The caller must select at least one stream.
        for (UINT32 i = 0; i < cStreams; ++i)
        {
            wil::com_ptr_nothrow<IMFStreamDescriptor> spSD;
            BOOL fSelected = FALSE;

            RETURN_IF_FAILED(pPD->GetStreamDescriptorByIndex(i, &fSelected, &spSD));
            anySelected |= !!fSelected;
        } 

        if (!anySelected)
        {
            return E_INVALIDARG;
        }

        return S_OK;
    }

    HRESULT SimpleMediaSource::_CreateSourceAttributes()
    {
        if (m_spAttributes.get() == nullptr)
        {
            // Create our source attribute store.
            RETURN_IF_FAILED(MFCreateAttributes(&m_spAttributes, 1));

            wil::com_ptr_nothrow<IMFSensorProfileCollection>  profileCollection;
            wil::com_ptr_nothrow<IMFSensorProfile> profile;

            // Create an empty profile collection...
            RETURN_IF_FAILED(MFCreateSensorProfileCollection(&profileCollection));

            // In this example since we have just one stream, we only have one
            // pin to add:  Pin = STREAM_ID.

            // Legacy profile is mandatory.  This is to ensure non-profile
            // aware applications can still function, but with degraded
            // feature sets.
            const DWORD STREAM_ID = 0;
            RETURN_IF_FAILED(MFCreateSensorProfile(KSCAMERAPROFILE_Legacy, 0 /*ProfileIndex*/, nullptr,
                &profile));
            RETURN_IF_FAILED(profile->AddProfileFilter(STREAM_ID, L"((RES==;FRT<=30,1;SUT==))"));
            RETURN_IF_FAILED(profileCollection->AddProfile(profile.get()));

            // High Frame Rate profile will only allow >=60fps.
            RETURN_IF_FAILED(MFCreateSensorProfile(KSCAMERAPROFILE_HighFrameRate, 0 /*ProfileIndex*/, nullptr,
                &profile));
            RETURN_IF_FAILED(profile->AddProfileFilter(STREAM_ID, L"((RES==;FRT>=60,1;SUT==))"));
            RETURN_IF_FAILED(profileCollection->AddProfile(profile.get()));


            // Se the profile collection to the attribute store of the IMFTransform.
            RETURN_IF_FAILED(m_spAttributes->SetUnknown(
                MF_DEVICEMFT_SENSORPROFILE_COLLECTION,
                profileCollection.get()));
        }

        return S_OK;
    }

    HRESULT SimpleMediaSource::_GetMediaStreamById(_In_ DWORD dwStreamId, _COM_Outptr_ SimpleMediaStream** ppStream)
    {
        RETURN_HR_IF_NULL(E_POINTER, ppStream);
        *ppStream = nullptr;

        for (unsigned int i = 0; i < m_streamList.size(); i++)
        {
            if (m_streamList[i]->Id() == dwStreamId)
            {
                *ppStream = m_streamList[i];
                (*ppStream)->AddRef();
                return S_OK;
            }
        }
        return MF_E_NOT_FOUND;
    }
}
