#pragma once

// Represents a simple media source that only has one stream and outputs a static image

#include "stdafx.h"
#include "SimpleMediaStream.h"

#define  BREAK_ON_FAIL(value)       if FAILED(value) break;

#if !defined(_IKsControl_)
#define _IKsControl_
interface DECLSPEC_UUID("28F54685-06FD-11D2-B27A-00A0C9223196") IKsControl;
#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
    STDMETHOD(KsProperty)(
        THIS_
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsMethod)(
        THIS_
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsEvent)(
        THIS_
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
};
#endif //!defined(_IKsControl_)

class SimpleMediaStream;

class __declspec(uuid("{9812588D-5CE9-4E4C-ABC1-049138D10DCE}"))
SimpleMediaSource : public RuntimeClass<
    RuntimeClassFlags<WinRtClassicComMix>,
    IMFMediaEventGenerator,
    IMFMediaSource,
    IMFMediaSourceEx,
    IMFGetService,
    IKsControl>
{

    enum class SourceState
    {
        Invalid, Stopped, Started, Shutdown
    };

public:
    // IMFMediaEventGenerator
    IFACEMETHOD(BeginGetEvent)(_In_ IMFAsyncCallback *pCallback, _In_ IUnknown *punkState);
    IFACEMETHOD(EndGetEvent)(_In_ IMFAsyncResult *pResult, _Out_ IMFMediaEvent **ppEvent);
    IFACEMETHOD(GetEvent)(DWORD dwFlags, _Out_ IMFMediaEvent **ppEvent);
    IFACEMETHOD(QueueEvent)(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, _In_ const PROPVARIANT *pvValue);

    // IMFMediaSource
    IFACEMETHOD(CreatePresentationDescriptor)(_Out_ IMFPresentationDescriptor **ppPresentationDescriptor);
    IFACEMETHOD(GetCharacteristics)(_Out_ DWORD *pdwCharacteristics);
    IFACEMETHOD(Pause)();
    IFACEMETHOD(Shutdown)();
    IFACEMETHOD(Start)(
        _In_ IMFPresentationDescriptor *pPresentationDescriptor,
        _In_ const GUID *pguidTimeFormat,
        _In_ const PROPVARIANT *pvarStartPosition);
    IFACEMETHOD(Stop)();

    // IMFMediaSourceEx
    IFACEMETHOD(GetSourceAttributes)(_Outptr_ IMFAttributes **ppAttributes);
    IFACEMETHOD(GetStreamAttributes)(DWORD dwStreamIdentifier, _Outptr_ IMFAttributes **ppAttributes);
    IFACEMETHOD(SetD3DManager)(_In_opt_ IUnknown *pManager);

    // IMFGetService
    IFACEMETHOD(GetService)(REFGUID guidService, REFIID riid, _Out_opt_ LPVOID *ppvObject);

    // IKsControl
    IFACEMETHOD(KsProperty)(
        _In_ PKSPROPERTY pProperty,
        ULONG ulPropertyLength,
        _Inout_ LPVOID pPropertyData,
        ULONG ulDataLength,
        _Out_ ULONG* pBytesReturned);

    IFACEMETHOD(KsMethod)(
        _In_ PKSMETHOD pMethod,
        ULONG ulMethodLength,
        _Inout_ LPVOID pMethodData,
        ULONG ulDataLength,
        _Out_ ULONG* pBytesReturned
        );

    IFACEMETHOD(KsEvent)(
        _In_opt_ PKSEVENT pEvent,
        ULONG ulEventLength,
        _Inout_opt_ LPVOID pEventData,
        ULONG ulDataLength,
        _Out_opt_ ULONG* pBytesReturned
        );
public:
    HRESULT RuntimeClassInitialize();

private:
    HRESULT _CheckShutdownRequiresLock();
    HRESULT _ValidatePresentationDescriptor(IMFPresentationDescriptor *pPresentationDescriptor);

    CriticalSection _critSec;
    SourceState _sourceState{ SourceState::Invalid };
    ComPtr<IMFMediaEventQueue> _spEventQueue;
    ComPtr<IMFPresentationDescriptor> _spPresentationDescriptor;
    ComPtr<IMFAttributes> _spAttributes;

    bool _wasStreamPreviouslySelected; // maybe makes more sense as a property of the stream
    const DWORD NUM_STREAMS = 1;
    ComPtr<SimpleMediaStream> _stream;
};

CoCreatableClass(SimpleMediaSource);
