#pragma once

#include "stdafx.h"

class SimpleMediaSource;

class SimpleMediaStream : public RuntimeClass<
    RuntimeClassFlags<ClassicCom>,
    IMFMediaEventGenerator,
    IMFMediaStream,
    IMFMediaStream2>
{
    friend class SimpleMediaSource;

public:
    // IMFMediaEventGenerator
    IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback *pCallback, IUnknown *punkState);
    IFACEMETHOD(EndGetEvent)(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
    IFACEMETHOD(GetEvent)(DWORD dwFlags, IMFMediaEvent **ppEvent);
    IFACEMETHOD(QueueEvent)(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue);

    // IMFMediaStream
    IFACEMETHOD(GetMediaSource)(IMFMediaSource **ppMediaSource);
    IFACEMETHOD(GetStreamDescriptor)(IMFStreamDescriptor **ppStreamDescriptor);
    IFACEMETHOD(RequestSample)(IUnknown *pToken);

    // IMFMediaStream2
    IFACEMETHOD(SetStreamState)(MF_STREAM_STATE state);
    IFACEMETHOD(GetStreamState)(_Out_ MF_STREAM_STATE *pState);

    // Non-interface methods.
    HRESULT RuntimeClassInitialize(_In_ SimpleMediaSource* pSource);
    HRESULT Shutdown();


protected:
    HRESULT _CheckShutdownRequiresLock();
    HRESULT _SetStreamAttributes(IMFAttributes* pAttributeStore);
    HRESULT _SetStreamDescriptorAttributes(IMFAttributes* pAttributeStore);


    CriticalSection _critSec;

    ComPtr<IMFMediaSource> _parent;
    ComPtr<IMFMediaEventQueue> _spEventQueue;
    ComPtr<IMFAttributes> _spAttributes;
    ComPtr<IMFMediaType> _spMediaType;
    ComPtr<IMFStreamDescriptor> _spStreamDesc;
    bool _isShutdown = false;
    bool _isSelected = false;

    const DWORD STREAMINDEX = 0; // since there is only one stream
};

