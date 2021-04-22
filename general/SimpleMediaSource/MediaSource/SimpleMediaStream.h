//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#ifndef SIMPLEMEDIASTREAM_H
#define SIMPLEMEDIASTREAM_H

#include "SimpleMediaSource.h"
//#include "VirtualCameraWin32.h"

namespace winrt::WindowsSample::implementation
{
    struct SimpleMediaStream : winrt::implements<SimpleMediaStream, IMFMediaStream2>
    {
        friend struct SimpleMediaSource;

    public:
        // IMFMediaEventGenerator
        IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback* pCallback, IUnknown* punkState);
        IFACEMETHOD(EndGetEvent)(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
        IFACEMETHOD(GetEvent)(DWORD dwFlags, IMFMediaEvent** ppEvent);
        IFACEMETHOD(QueueEvent)(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

        // IMFMediaStream
        IFACEMETHOD(GetMediaSource)(IMFMediaSource** ppMediaSource);
        IFACEMETHOD(GetStreamDescriptor)(IMFStreamDescriptor** ppStreamDescriptor);
        IFACEMETHOD(RequestSample)(IUnknown* pToken);

        // IMFMediaStream2
        IFACEMETHOD(SetStreamState)(_In_ MF_STREAM_STATE state);
        IFACEMETHOD(GetStreamState)(_Out_ MF_STREAM_STATE* pState);

        // Non-interface methods.
        HRESULT Initialize(_In_ SimpleMediaSource* pSource, _In_ DWORD streamId, _In_ MFSampleAllocatorUsage allocatorUsage);
        HRESULT Start();
        HRESULT Stop();
        HRESULT Shutdown();
        HRESULT SetSampleAllocator(_In_ IMFVideoSampleAllocator* pAllocator);

        DWORD Id() const { return m_dwStreamId; }
        MFSampleAllocatorUsage SampleAlloactorUsage() const { return m_allocatorUsage; }

        void SetRGBMask(ULONG rgbMask) { winrt::slim_lock_guard lock(m_Lock);  m_rgbMask = rgbMask; }
        ULONG GetRGBMask() { winrt::slim_lock_guard lock(m_Lock);  return m_rgbMask; }

    protected:
        HRESULT _CheckShutdownRequiresLock();
        HRESULT _SetStreamAttributes(IMFAttributes* pAttributeStore);
        HRESULT _SetStreamDescriptorAttributes(IMFAttributes* pAttributeStore);

    private:
        winrt::slim_mutex  m_Lock;

        wil::com_ptr_nothrow<IMFMediaSource> m_parent;
        wil::com_ptr_nothrow<IMFMediaEventQueue> m_spEventQueue;
        wil::com_ptr_nothrow<IMFAttributes> m_spAttributes;
        wil::com_ptr_nothrow<IMFStreamDescriptor> m_spStreamDesc;
        wil::com_ptr_nothrow<IMFVideoSampleAllocator> m_spSampleAllocator;
       wistd::unique_ptr<SimpleFrameGenerator> m_spFrameGenerator;

        bool m_bIsShutdown = false;
        MF_STREAM_STATE m_streamState = MF_STREAM_STATE_STOPPED;
        ULONG m_rgbMask = KSPROPERTY_SIMPLEMEDIASOURCE_CUSTOMCONTROL_COLORMODE_BLUE;

        DWORD m_dwStreamId;
        MFSampleAllocatorUsage m_allocatorUsage;
    };
}

#endif
