//
// DelayAPO.h -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//   Declaration of the CDelayAPO class.
//

#pragma once

#include <audioenginebaseapo.h>
#include <BaseAudioProcessingObject.h>
#include <DelayAPOInterface.h>
#include <DelayAPODll.h>

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
// {29FBFBB5-9002-4ABC-DCBC-DD45462478C8}
DEFINE_GUID(DelayEffectId,      0x29FBFBB5, 0x9002, 0x4ABC, 0xDC, 0xBC, 0xDD, 0x45, 0x46, 0x24, 0x78, 0xC8);

// 1000 ms of delay
#define HNS_DELAY HNS_PER_SECOND

#define FRAMES_FROM_HNS(hns) (ULONG)(1.0 * hns / HNS_PER_SECOND * GetFramesPerSecond() + 0.5)

LONG GetCurrentEffectsSetting(IPropertyStore* properties, PROPERTYKEY pkeyEnable, GUID processingMode);

#pragma AVRT_VTABLES_BEGIN
// Delay APO class - MFX
class CDelayAPOMFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDelayAPOMFX, &CLSID_DelayAPOMFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioSystemEffects2,
    // IAudioSystemEffectsCustomFormats may be optionally supported
    // by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    public IAudioSystemEffectsCustomFormats, 
    public IDelayAPOMFX
{
public:
    // constructor
    CDelayAPOMFX()
    :   CBaseAudioProcessingObject(sm_RegProperties)
    ,   m_hEffectsChangedEvent(NULL)
    ,   m_AudioProcessingMode(AUDIO_SIGNALPROCESSINGMODE_DEFAULT)
    ,   m_fEnableDelayMFX(FALSE)
    ,   m_nDelayFrames(0)
    ,   m_iDelayIndex(0)
    {
        m_pf32Coefficients = NULL;
    }

    virtual ~CDelayAPOMFX();    // destructor

DECLARE_REGISTRY_RESOURCEID(IDR_DELAYAPOMFX)

BEGIN_COM_MAP(CDelayAPOMFX)
    COM_INTERFACE_ENTRY(IDelayAPOMFX)
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
    LONG                                    m_fEnableDelayMFX;
    GUID                                    m_AudioProcessingMode;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties

    // Locked memory
    FLOAT32                                 *m_pf32Coefficients;

    CComHeapPtr<FLOAT32>                    m_pf32DelayBuffer;
    UINT32                                  m_nDelayFrames;
    UINT32                                  m_iDelayIndex;

private:
    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;

    HRESULT ProprietaryCommunicationWithDriver(APOInitSystemEffects2 *_pAPOSysFxInit2);

};
#pragma AVRT_VTABLES_END


#pragma AVRT_VTABLES_BEGIN
// Delay APO class - SFX
class CDelayAPOSFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDelayAPOSFX, &CLSID_DelayAPOSFX>,
    public CBaseAudioProcessingObject,
    public IMMNotificationClient,
    public IAudioSystemEffects2,
    public IDelayAPOSFX
{
public:
    // constructor
    CDelayAPOSFX()
    :   CBaseAudioProcessingObject(sm_RegProperties)
    ,   m_hEffectsChangedEvent(NULL)
    ,   m_AudioProcessingMode(AUDIO_SIGNALPROCESSINGMODE_DEFAULT)
    ,   m_fEnableDelaySFX(FALSE)
    ,   m_nDelayFrames(0)
    ,   m_iDelayIndex(0)
    {
    }

    virtual ~CDelayAPOSFX();    // destructor

DECLARE_REGISTRY_RESOURCEID(IDR_DELAYAPOSFX)

BEGIN_COM_MAP(CDelayAPOSFX)
    COM_INTERFACE_ENTRY(IDelayAPOSFX)
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
    LONG                                    m_fEnableDelaySFX;
    GUID                                    m_AudioProcessingMode;
    CComPtr<IPropertyStore>                 m_spAPOSystemEffectsProperties;
    CComPtr<IMMDeviceEnumerator>            m_spEnumerator;
    static const CRegAPOProperties<1>       sm_RegProperties;   // registration properties

    CCriticalSection                        m_EffectsLock;
    HANDLE                                  m_hEffectsChangedEvent;

    CComHeapPtr<FLOAT32>                    m_pf32DelayBuffer;
    UINT32                                  m_nDelayFrames;
    UINT32                                  m_iDelayIndex;
};
#pragma AVRT_VTABLES_END

OBJECT_ENTRY_AUTO(__uuidof(DelayAPOMFX), CDelayAPOMFX)
OBJECT_ENTRY_AUTO(__uuidof(DelayAPOSFX), CDelayAPOSFX)

//
//   Declaration of the ProcessDelay routine.
//
void ProcessDelay(
    _Out_writes_(u32ValidFrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32OutputFrames,
    _In_reads_(u32ValidFrameCount * u32SamplesPerFrame)
        const FLOAT32 *pf32InputFrames,
    UINT32       u32ValidFrameCount,
    UINT32       u32SamplesPerFrame,
    _Inout_updates_(u32DelayFrames * u32SamplesPerFrame)
        FLOAT32 *pf32DelayBuffer,
    UINT32       u32DelayFrames,
    _Inout_
        UINT32  *pu32DelayIndex );

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
