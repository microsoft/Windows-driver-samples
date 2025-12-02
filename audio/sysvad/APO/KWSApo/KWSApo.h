//
// KWSApo.h -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//   Declaration of the CKWSApoEFX class.
//

#pragma once

#include <audioenginebaseapo.h>
#include <BaseAudioProcessingObject.h>
#include <KWSApoInterface.h>
#include <KWSApoDll.h>

#include <commonmacros.h>
#include <devicetopology.h>

#include <audioengineextensionapo.h>

#include <wil\com.h>

_Analysis_mode_(_Analysis_code_type_user_driver_)

#pragma AVRT_VTABLES_BEGIN
// KWS APO class - EFX
class CKWSApoEFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CKWSApoEFX, &CLSID_KWSApoEFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioSystemEffects3,
    public IAudioProcessingObjectNotifications,
    public IKWSApoEFX
{
public:
    // constructor
    CKWSApoEFX()
    :   CBaseAudioProcessingObject(sm_RegProperties)
    {
    }

    virtual ~CKWSApoEFX();    // destructor

DECLARE_REGISTRY_RESOURCEID(IDR_KWSAPOEFX)

BEGIN_COM_MAP(CKWSApoEFX)
    COM_INTERFACE_ENTRY(IKWSApoEFX)
    COM_INTERFACE_ENTRY(IAudioSystemEffects)
    COM_INTERFACE_ENTRY(IAudioSystemEffects2)
    COM_INTERFACE_ENTRY(IAudioSystemEffects3)
    COM_INTERFACE_ENTRY(IAudioProcessingObjectNotifications)
    COM_INTERFACE_ENTRY(IMMNotificationClient)
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

    // IAudioProcessingObject
    STDMETHOD(IsInputFormatSupported)(IAudioMediaType *pOutputFormat, IAudioMediaType *pRequestedInputFormat, IAudioMediaType **ppSupportedInputFormat);
    STDMETHOD(IsOutputFormatSupported)(IAudioMediaType *pInputFormat, IAudioMediaType *pRequestedOutputFormat, IAudioMediaType **ppSupportedOutputFormat);
    STDMETHOD(GetInputChannelCount)(UINT32 *pu32ChannelCount);

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
    STDMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
    { 
        UNREFERENCED_PARAMETER(pwstrDeviceId);
        UNREFERENCED_PARAMETER(key);
        return S_OK; 
    }

    // IAudioSystemEffects3
    STDMETHOD(GetControllableSystemEffectsList)(_Outptr_result_buffer_maybenull_(*numEffects) AUDIO_SYSTEMEFFECT** effects, _Out_ UINT* numEffects, _In_opt_ HANDLE event) override;
    STDMETHODIMP SetAudioSystemEffectState(GUID, AUDIO_SYSTEMEFFECT_STATE) override {return S_OK;}

    // IAudioProcessingObjectNotifications
    STDMETHODIMP GetApoNotificationRegistrationInfo(_Out_writes_(*count) APO_NOTIFICATION_DESCRIPTOR** apoNotifications, _Out_ DWORD* count) override
    {
        UNREFERENCED_PARAMETER(apoNotifications);
        UNREFERENCED_PARAMETER(count);
        return S_OK; 
    }

    STDMETHODIMP_(void) HandleNotification(_In_ APO_NOTIFICATION* apoNotification) override
    {
        UNREFERENCED_PARAMETER(apoNotification);
    }

public:
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties
    INTERLEAVED_AUDIO_FORMAT_INFORMATION    m_FormatInfo;

private:
    BOOL                                    m_bRegisteredEndpointNotificationCallback = FALSE;
    wil::com_ptr_nothrow<IAudioProcessingObjectLoggingService> m_apoLoggingService;
};
#pragma AVRT_VTABLES_END

OBJECT_ENTRY_AUTO(__uuidof(KWSApoEFX), CKWSApoEFX)

//
//   Declaration of the ProcessBuffer routine.
//
void ProcessBuffer(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    INTERLEAVED_AUDIO_FORMAT_INFORMATION *formatInfo);

//
//   Convenience methods
//

void WriteSilence(
    _Out_writes_(u32FrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32Frames,
    UINT32 u32FrameCount,
    UINT32 u32SamplesPerFrame );

