//
// SwapAPO.h -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//   Declaration of the CSwapAPO class.
//

#pragma once

#include <audioenginebaseapo.h>
#include <audioengineextensionapo.h>
#include <BaseAudioProcessingObject.h>
#include <SwapAPOInterface.h>
#include <SwapAPODll.h>

#include <commonmacros.h>
#include <devicetopology.h>
#include <rtworkq.h>

#include <wil\com.h>

_Analysis_mode_(_Analysis_code_type_user_driver_)

#define PK_EQUAL(x, y)  ((x.fmtid == y.fmtid) && (x.pid == y.pid))
#define GUID_FORMAT_STRING "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
#define GUID_FORMAT_ARGS(guidVal)  (guidVal).Data1, (guidVal).Data2, (guidVal).Data3, (guidVal).Data4[0], (guidVal).Data4[1], (guidVal).Data4[2], (guidVal).Data4[3], (guidVal).Data4[4], (guidVal).Data4[5], (guidVal).Data4[6], (guidVal).Data4[7]
#define NUM_OF_EFFECTS 1

//
// Define a GUID identifying the type of this APO's custom effect.
//
// APOs generally should not define new GUIDs for types of effects and instead
// should use predefined effect types. Only define a new GUID if the effect is
// truly very different from all predefined types of effects.
//
// {B8EC75BA-00ED-434C-A732-064A0F00788E}
DEFINE_GUID(SwapEffectId,       0xb8ec75ba, 0x00ed, 0x434c, 0xa7, 0x32, 0x06, 0x4a, 0x0f, 0x00, 0x78, 0x8e);

// {5DB5B4C8-6C37-450E-93F5-1E275AFDF87F}
DEFINE_GUID(SWAP_APO_MFX_CONTEXT, 0x5db5b4c8, 0x6c37, 0x450e, 0x93, 0xf5, 0x1e, 0x27, 0x5a, 0xfd, 0xf8, 0x7f);

// {99817AE5-E6DC-4074-B513-8A872178DA12}
DEFINE_GUID(SWAP_APO_SFX_CONTEXT, 0x99817ae5, 0xe6dc, 0x4074, 0xb5, 0x13, 0x8a, 0x87, 0x21, 0x78, 0xda, 0x12);

LONG GetCurrentEffectsSetting(IPropertyStore* properties, PROPERTYKEY pkeyEnable, GUID processingMode);

class SwapMFXApoAsyncCallback :
    public IRtwqAsyncCallback
{
private:
    DWORD m_queueId;
    volatile ULONG _refCount = 1;

public:
    SwapMFXApoAsyncCallback(DWORD queueId) : m_queueId(queueId)
    {
    }

    static HRESULT Create(_Outptr_ SwapMFXApoAsyncCallback** workItemOut, DWORD queueId);

    // IRtwqAsyncCallback
    STDMETHOD(GetParameters)(_Out_opt_ DWORD* pdwFlags, _Out_opt_ DWORD* pdwQueue)
    {
        *pdwFlags = 0;
        *pdwQueue = m_queueId;
        return S_OK;
    }
    STDMETHOD(Invoke)(_In_ IRtwqAsyncResult* asyncResult);

    // IUnknown (needed for IRtwqAsyncCallback)
    STDMETHOD(QueryInterface)(REFIID riid, __deref_out void** interfaceOut);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
};

#pragma AVRT_VTABLES_BEGIN
// Swap APO class - MFX
class CSwapAPOMFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSwapAPOMFX, &CLSID_SwapAPOMFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioProcessingObjectNotifications,
    public IAudioSystemEffects3,
    // IAudioSystemEffectsCustomFormats may be optionally supported
    // by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    public IAudioSystemEffectsCustomFormats, 
    public ISwapAPOMFX
{
public:
    // constructor
    CSwapAPOMFX()
    :   CBaseAudioProcessingObject(sm_RegProperties)
    ,   m_hEffectsChangedEvent(NULL)
    ,   m_AudioProcessingMode(AUDIO_SIGNALPROCESSINGMODE_DEFAULT)
    ,   m_fEnableSwapMFX(FALSE)
    {
        m_pf32Coefficients = NULL;
    }

    virtual ~CSwapAPOMFX();    // destructor

DECLARE_REGISTRY_RESOURCEID(IDR_SWAPAPOMFX)

BEGIN_COM_MAP(CSwapAPOMFX)
    COM_INTERFACE_ENTRY(ISwapAPOMFX)
    COM_INTERFACE_ENTRY(IAudioSystemEffects)
    COM_INTERFACE_ENTRY(IAudioSystemEffects2)
    COM_INTERFACE_ENTRY(IAudioSystemEffects3)
    // IAudioSystemEffectsCustomFormats may be optionally supported
    // by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    COM_INTERFACE_ENTRY(IAudioSystemEffectsCustomFormats)
    COM_INTERFACE_ENTRY(IMMNotificationClient)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectNotifications)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectRT)
    COM_INTERFACE_ENTRY(IAudioProcessingObject)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectConfiguration)
END_COM_MAP()

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:
    STDMETHOD_(void, APOProcess)(UINT32 u32NumInputConnections,
        APO_CONNECTION_PROPERTY** ppInputConnections, UINT32 u32NumOutputConnections,
        APO_CONNECTION_PROPERTY** ppOutputConnections);

    STDMETHOD(GetLatency)(HNSTIME* pTime);

    STDMETHOD(LockForProcess)(UINT32 u32NumInputConnections,
        APO_CONNECTION_DESCRIPTOR** ppInputConnections,  
        UINT32 u32NumOutputConnections, APO_CONNECTION_DESCRIPTOR** ppOutputConnections);

    STDMETHOD(Initialize)(UINT32 cbDataSize, BYTE* pbyData);

    // IAudioSystemEffects2
    STDMETHOD(GetEffectsList)(_Outptr_result_buffer_maybenull_(*pcEffects)  LPGUID *ppEffectsIds, _Out_ UINT *pcEffects, _In_ HANDLE Event);

    // IAudioSystemEffects3
    STDMETHOD(GetControllableSystemEffectsList)(_Outptr_result_buffer_maybenull_(*numEffects) AUDIO_SYSTEMEFFECT** effects, _Out_ UINT* numEffects, _In_opt_ HANDLE event);

    STDMETHOD(SetAudioSystemEffectState)(GUID effectId, AUDIO_SYSTEMEFFECT_STATE state);

    virtual HRESULT ValidateAndCacheConnectionInfo(
                                    UINT32 u32NumInputConnections, 
                                    APO_CONNECTION_DESCRIPTOR** ppInputConnections, 
                                    UINT32 u32NumOutputConnections, 
                                    APO_CONNECTION_DESCRIPTOR** ppOutputConnections);

    // IMMNotificationClient
    STDMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) 
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        UNREFERENCED_PARAMETER(dwNewState); 
        return S_OK; 
    }
    STDMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId)
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId)
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
    { 
        UNREFERENCED_PARAMETER(flow); 
        UNREFERENCED_PARAMETER(role); 
        UNREFERENCED_PARAMETER(pwstrDefaultDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);

    // IAudioProcessingObjectNotifications
    STDMETHODIMP GetApoNotificationRegistrationInfo(_Out_writes_(*count) APO_NOTIFICATION_DESCRIPTOR** apoNotifications, _Out_ DWORD* count);
    STDMETHODIMP_(void) HandleNotification(_In_ APO_NOTIFICATION* apoNotification);

    // IAudioSystemEffectsCustomFormats
    // This interface may be optionally supported by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    STDMETHODIMP GetFormatCount(UINT* pcFormats);
    STDMETHODIMP GetFormat(UINT nFormat, IAudioMediaType** ppFormat);
    STDMETHODIMP GetFormatRepresentation(UINT nFormat, _Outptr_ LPWSTR* ppwstrFormatRep);

    // IAudioProcessingObject
    STDMETHODIMP IsOutputFormatSupported(IAudioMediaType *pOppositeFormat, IAudioMediaType *pRequestedOutputFormat, IAudioMediaType **ppSupportedOutputFormat);

    STDMETHODIMP CheckCustomFormats(IAudioMediaType *pRequestedFormat);

    STDMETHODIMP DoWorkOnRealTimeThread();

    void HandleWorkItemCompleted(_In_ IRtwqAsyncResult* asyncResult);
    
public:
    LONG                                    m_fEnableSwapMFX;
    GUID                                    m_AudioProcessingMode;
    wil::com_ptr_nothrow<IMMDevice>         m_device;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties
    AUDIO_SYSTEMEFFECT                      m_effectInfos[NUM_OF_EFFECTS];

    // Locked memory
    FLOAT32                                 *m_pf32Coefficients;

private:
    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;
    BOOL m_bRegisteredEndpointNotificationCallback = FALSE;
 
    wil::com_ptr_nothrow<IPropertyStore> m_userStore;
    wil::com_ptr_nothrow<IAudioProcessingObjectLoggingService> m_apoLoggingService;

    DWORD m_queueId = 0;
    wil::com_ptr_nothrow<SwapMFXApoAsyncCallback> m_asyncCallback;

    HRESULT ProprietaryCommunicationWithDriver(IMMDeviceCollection *pDeviceCollection, UINT nSoftwareIoDeviceInCollection, UINT nSoftwareIoConnectorIndex);

};
#pragma AVRT_VTABLES_END


#pragma AVRT_VTABLES_BEGIN
// Swap APO class - SFX
class CSwapAPOSFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSwapAPOSFX, &CLSID_SwapAPOSFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioProcessingObjectNotifications,
    public IAudioSystemEffects3,
    public ISwapAPOSFX
{
public:
    // constructor
    CSwapAPOSFX()
    :   CBaseAudioProcessingObject(sm_RegProperties)
    ,   m_hEffectsChangedEvent(NULL)
    ,   m_AudioProcessingMode(AUDIO_SIGNALPROCESSINGMODE_DEFAULT)
    ,   m_fEnableSwapSFX(FALSE)
    ,   m_fEnableDelaySFX(FALSE)
    {
    }

    virtual ~CSwapAPOSFX();    // destructor

DECLARE_REGISTRY_RESOURCEID(IDR_SWAPAPOSFX)

BEGIN_COM_MAP(CSwapAPOSFX)
    COM_INTERFACE_ENTRY(ISwapAPOSFX)
    COM_INTERFACE_ENTRY(IAudioSystemEffects)
    COM_INTERFACE_ENTRY(IAudioSystemEffects2)
    COM_INTERFACE_ENTRY(IAudioSystemEffects3)
    COM_INTERFACE_ENTRY(IMMNotificationClient)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectNotifications)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectRT)
    COM_INTERFACE_ENTRY(IAudioProcessingObject)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectConfiguration)
END_COM_MAP()

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:
    STDMETHOD_(void, APOProcess)(UINT32 u32NumInputConnections,
        APO_CONNECTION_PROPERTY** ppInputConnections, UINT32 u32NumOutputConnections,
        APO_CONNECTION_PROPERTY** ppOutputConnections);

    STDMETHOD(GetLatency)(HNSTIME* pTime);

    STDMETHOD(LockForProcess)(UINT32 u32NumInputConnections,
        APO_CONNECTION_DESCRIPTOR** ppInputConnections,  
        UINT32 u32NumOutputConnections, APO_CONNECTION_DESCRIPTOR** ppOutputConnections);

    STDMETHOD(Initialize)(UINT32 cbDataSize, BYTE* pbyData);

    // IAudioSystemEffects2
    STDMETHOD(GetEffectsList)(_Outptr_result_buffer_maybenull_(*pcEffects)  LPGUID *ppEffectsIds, _Out_ UINT *pcEffects, _In_ HANDLE Event);

    // IAudioSystemEffects3
    STDMETHOD(GetControllableSystemEffectsList)(_Outptr_result_buffer_maybenull_(*numEffects) AUDIO_SYSTEMEFFECT** effects, _Out_ UINT* numEffects, _In_opt_ HANDLE event);

    STDMETHOD(SetAudioSystemEffectState)(GUID effectId, AUDIO_SYSTEMEFFECT_STATE state);
    // IMMNotificationClient
    STDMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) 
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        UNREFERENCED_PARAMETER(dwNewState); 
        return S_OK; 
    }
    STDMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId)
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId)
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
    { 
        UNREFERENCED_PARAMETER(flow); 
        UNREFERENCED_PARAMETER(role); 
        UNREFERENCED_PARAMETER(pwstrDefaultDeviceId); 
        return S_OK; 
    }
    STDMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
    
    // IAudioProcessingObjectNotifications
    STDMETHODIMP GetApoNotificationRegistrationInfo(_Out_writes_(*count) APO_NOTIFICATION_DESCRIPTOR** apoNotifications, _Out_ DWORD* count);
    STDMETHODIMP_(void) HandleNotification(_In_ APO_NOTIFICATION* apoNotification);

public:
    LONG                                    m_fEnableSwapSFX;
    LONG                                    m_fEnableDelaySFX;
    GUID                                    m_AudioProcessingMode;
    wil::com_ptr_nothrow<IMMDevice>         m_device;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties
    AUDIO_SYSTEMEFFECT                      m_effectInfos[NUM_OF_EFFECTS];

    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;

private:
    wil::com_ptr_nothrow<IPropertyStore> m_userStore;
    wil::com_ptr_nothrow<IAudioProcessingObjectLoggingService> m_apoLoggingService;
    BOOL m_bRegisteredEndpointNotificationCallback = FALSE;
};
#pragma AVRT_VTABLES_END

OBJECT_ENTRY_AUTO(__uuidof(SwapAPOMFX), CSwapAPOMFX)
OBJECT_ENTRY_AUTO(__uuidof(SwapAPOSFX), CSwapAPOSFX)

//
//   Declaration of the ProcessSwap routine.
//
void ProcessSwap(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    UINT32   u32SamplesPerFrame);

//
//   Declaration of the ProcessSwapScale routine.
//
void ProcessSwapScale(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    UINT32   u32SamplesPerFrame,
    FLOAT32 *pf32Coefficients );

//
//   Convenience methods
//

void WriteSilence(
    _Out_writes_(u32FrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32Frames,
    UINT32 u32FrameCount,
    UINT32 u32SamplesPerFrame );

void CopyFrames(
    _Out_writes_(u32FrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32OutFrames,
    _In_reads_(u32FrameCount * u32SamplesPerFrame)
        const FLOAT32 *pf32InFrames,
    UINT32 u32FrameCount,
    UINT32 u32SamplesPerFrame );
