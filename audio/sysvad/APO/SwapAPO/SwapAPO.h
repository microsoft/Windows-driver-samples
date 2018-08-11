//
// SwapAPO.h -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//   Declaration of the CSwapAPO class.
//

#pragma once

#include <audioenginebaseapo.h>
#include <BaseAudioProcessingObject.h>
#include <SwapAPOInterface.h>
#include <SwapAPODll.h>

#include <commonmacros.h>
#include <devicetopology.h>

_Analysis_mode_(_Analysis_code_type_user_driver_)

#define PK_EQUAL(x, y)  ((x.fmtid == y.fmtid) && (x.pid == y.pid))

//
// Define a GUID identifying the type of this APO's custom effect.
//
// APOs generally should not define new GUIDs for types of effects and instead
// should use predefined effect types. Only define a new GUID if the effect is
// truly very different from all predefined types of effects.
//
// {B8EC75BA-00ED-434C-A732-064A0F00788E}
DEFINE_GUID(SwapEffectId,       0xb8ec75ba, 0x00ed, 0x434c, 0xa7, 0x32, 0x06, 0x4a, 0x0f, 0x00, 0x78, 0x8e);

LONG GetCurrentEffectsSetting(IPropertyStore* properties, PROPERTYKEY pkeyEnable, GUID processingMode);

#pragma AVRT_VTABLES_BEGIN
// Swap APO class - MFX
class CSwapAPOMFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSwapAPOMFX, &CLSID_SwapAPOMFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioSystemEffects2,
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
    // IAudioSystemEffectsCustomFormats may be optionally supported
    // by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    COM_INTERFACE_ENTRY(IAudioSystemEffectsCustomFormats)
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

    // IAudioSystemEffectsCustomFormats
    // This interface may be optionally supported by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    STDMETHODIMP GetFormatCount(UINT* pcFormats);
    STDMETHODIMP GetFormat(UINT nFormat, IAudioMediaType** ppFormat);
    STDMETHODIMP GetFormatRepresentation(UINT nFormat, _Outptr_ LPWSTR* ppwstrFormatRep);

public:
    LONG                                    m_fEnableSwapMFX;
    GUID                                    m_AudioProcessingMode;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties

    // Locked memory
    FLOAT32                                 *m_pf32Coefficients;

private:
    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;

    HRESULT ProprietaryCommunicationWithDriver(APOInitSystemEffects2 *_pAPOSysFxInit2);

};
#pragma AVRT_VTABLES_END


#pragma AVRT_VTABLES_BEGIN
// Swap APO class - SFX
class CSwapAPOSFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSwapAPOSFX, &CLSID_SwapAPOSFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioSystemEffects2,
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

public:
    LONG                                    m_fEnableSwapSFX;
    LONG                                    m_fEnableDelaySFX;
    GUID                                    m_AudioProcessingMode;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties

    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;
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
