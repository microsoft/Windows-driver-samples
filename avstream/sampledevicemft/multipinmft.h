//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once
#include "common.h"
#include "mftpeventgenerator.h"
#include "basepin.h"
#include "custompin.h"
#include "multipinmfthelpers.h"

//
// The Below GUID is needed to transfer photoconfirmation sample successfully in the pipeline
// It is used to propagate the mediatype of the sample to the pipeline which will consume the sample
// This attribute is known to the OS, but not publicly defined.
//

DEFINE_GUID(MFSourceReader_SampleAttribute_MediaType_priv,
    0x0ea5c1e8, 0x9845, 0x41e0, 0xa2, 0x43, 0x72, 0x32, 0x07, 0xfc, 0x78, 0x1f);


interface IDirect3DDeviceManager9;

//
// Forward declarations
//
class CMFAttributes;
class CPinCreationFactory;
//
// CMultipinMft class:
// Implements a device proxy MFT.
//
class CMultipinMft :
    public IMFDeviceTransform
#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
    , public IMFTransform
#endif
    , public IMFShutdown
    , public CMediaEventGenerator
    , public IMFRealTimeClientEx
    , public IKsControl
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    , public IMFCapturePhotoConfirmation
    , public IMFGetService
#endif
    , public CDMFTModuleLifeTimeManager
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    , public IMFSampleAllocatorControl
#endif
{
    friend class CPinCreationFactory;
public:
    CMultipinMft(
        void );

    virtual ~CMultipinMft();

    //
    // IUnknown
    //
    STDMETHOD_(ULONG, AddRef)(
        void );

    STDMETHOD_(ULONG, Release)(
        void );

    STDMETHOD(QueryInterface)(
        _In_ REFIID iid,
        _COM_Outptr_ void** ppv);


    //
    // IMFDeviceTransform functions
    //
    STDMETHOD(GetStreamCount)(
        _Inout_ DWORD   *pdwInputStreams,
        _Inout_ DWORD   *pdwOutputStreams);


    STDMETHOD(GetStreamIDs)(
        _In_                                    DWORD  dwInputIDArraySize,
        _When_(dwInputIDArraySize >= m_InputPinCount, _Out_writes_(dwInputIDArraySize))  DWORD* pdwInputIDs,
        _In_                                    DWORD  dwOutputIDArraySize,
        _When_(dwOutputIDArraySize >= m_OutputPinCount && (pdwInputIDs && (dwInputIDArraySize > 0)),
        _Out_writes_(dwOutputIDArraySize)) _On_failure_(_Valid_) DWORD* pdwOutputIDs
        );

    STDMETHOD(GetInputStreamAttributes)(
        _In_        DWORD           dwInputStreamID,
        _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes);

    STDMETHOD(GetOutputStreamAttributes)(
        _In_        DWORD           dwOutputStreamID,
        _Out_ IMFAttributes** ppAttributes);

    STDMETHOD(GetInputAvailableType)(
        _In_        DWORD           dwInputStreamID,
        _In_        DWORD           dwTypeIndex,
        _Out_ IMFMediaType**  ppType);

    STDMETHOD(GetOutputAvailableType)(
        _In_        DWORD           dwOutputStreamID,
        _In_        DWORD           dwTypeIndex,
        _Out_ IMFMediaType**  ppMediaType);

    STDMETHOD(GetInputCurrentType)(
        _In_        DWORD           dwInputStreamID,
        _COM_Outptr_result_maybenull_ IMFMediaType**  ppMediaType);

    STDMETHOD(GetOutputCurrentType)(
        _In_        DWORD           dwOutputStreamID,
        _Out_       IMFMediaType**  ppMediaType);

    STDMETHOD(ProcessMessage)(
        _In_    MFT_MESSAGE_TYPE    eMessage,
        _In_    ULONG_PTR           ulParam );

    STDMETHOD(ProcessEvent)(
        _In_  DWORD dwInputStreamID,
        _In_  IMFMediaEvent *pEvent);


    STDMETHOD(ProcessInput)(
        _In_    DWORD       dwInputStreamID,
        _In_    IMFSample*  pSample,
        _In_    DWORD       dwFlags );

    STDMETHOD(ProcessOutput)(
        _In_    DWORD                       dwFlags,
        _In_    DWORD                       cOutputBufferCount,
        _Inout_updates_(cOutputBufferCount)  MFT_OUTPUT_DATA_BUFFER  *pOutputSamples,
        _Out_   DWORD                       *pdwStatus );

    //
    // IMFRealTimeClientEx
    //
    STDMETHOD(RegisterThreadsEx)(
        _Inout_ DWORD* pdwTaskIndex,
        _In_ LPCWSTR wszClassName,
        _In_ LONG lBasePriority )
    {
        UNREFERENCED_PARAMETER(pdwTaskIndex);
        UNREFERENCED_PARAMETER(wszClassName);
        UNREFERENCED_PARAMETER(lBasePriority);
        return S_OK;
    }

    STDMETHOD(UnregisterThreads)()
    {
        return S_OK;
    }

    STDMETHOD(SetWorkQueueEx)(
        _In_ DWORD dwWorkQueueId,
        _In_ LONG lWorkItemBasePriority );

    //
    // IMFShutdown
    //
    STDMETHOD(Shutdown)(
        void );

    STDMETHOD(GetShutdownStatus)(
        MFSHUTDOWN_STATUS *pStatus)
    {
        UNREFERENCED_PARAMETER(pStatus);
        return(m_eShutdownStatus);
    };

    //
    // IMFDeviceTransform function declarations
    //
    STDMETHODIMP InitializeTransform(
        _In_ IMFAttributes *pAttributes );

    _Requires_no_locks_held_
    STDMETHODIMP SetInputStreamState(
        _In_ DWORD dwStreamID,
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags );

    STDMETHODIMP GetInputStreamState(
        _In_ DWORD dwStreamID,
        _Out_ DeviceStreamState *value );

    STDMETHODIMP SetOutputStreamState(
        _In_ DWORD dwStreamID,
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags );

    STDMETHODIMP GetOutputStreamState(
        _In_ DWORD dwStreamID,
        _Out_ DeviceStreamState *value );

    STDMETHODIMP GetInputStreamPreferredState(
        _In_              DWORD                             dwStreamID,
        _Inout_           DeviceStreamState                 *value,
        _Outptr_opt_result_maybenull_ IMFMediaType          **ppMediaType );

    STDMETHODIMP FlushInputStream(
        _In_ DWORD dwStreamIndex,
        _In_ DWORD dwFlags );

    STDMETHODIMP FlushOutputStream(
        _In_ DWORD dwStreamIndex,
        _In_ DWORD dwFlags );

    STDMETHODIMP_(VOID) FlushAllStreams(
        VOID
        );

    //
    //IKSControl Inferface function declarations
    //
    STDMETHOD(KsEvent)(
        _In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
        _In_ ULONG ulEventLength,
        _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );
    STDMETHOD(KsProperty)(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );
    STDMETHOD(KsMethod)(
        _In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    //
    // The Below functions are needed for photoconfirmation
    //
    //
    STDMETHOD(GetService)(
        __in REFGUID guidService,
        __in REFIID riid,
        __deref_out LPVOID* ppvObject
        );
    //
    //IMFCapturePhotoConfirmation Inferface function declarations
    //
    STDMETHOD(SetPhotoConfirmationCallback)(
        _In_ IMFAsyncCallback* pNotificationCallback
        );
    STDMETHOD(SetPixelFormat)(
        _In_ GUID subtype
        );
    STDMETHOD(GetPixelFormat)(
        _Out_ GUID* subtype
        );

    __inline BOOL IsPhotoConfirmationEnabled()
    {
        return (m_spPhotoConfirmationCallback != nullptr);
    }

    STDMETHODIMP ProcessCapturePhotoConfirmationCallBack(
        _In_ IMFMediaType* pMediaType,
        _In_ IMFSample*    pSample
        );

    __inline VOID SetPhotoConfirmationCallBack(_In_ IMFAsyncCallback *Callback)
    {
        m_spPhotoConfirmationCallback = Callback;
    }

#endif
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    //
    // IMFSampleAllocatorControl Inferface function declarations
    //

    STDMETHOD(SetDefaultAllocator)(
        _In_ DWORD dwOutputStreamID,
        _In_ IUnknown *pAllocator
    );

    STDMETHOD(GetAllocatorUsage)(
        _In_ DWORD dwOutputStreamID,
        _Out_  DWORD* pdwInputStreamID,
        _Out_ MFSampleAllocatorUsage* peUsage
    );
#endif
    static STDMETHODIMP CreateInstance(
        REFIID iid, void **ppMFT);
 
    __inline BOOL isPhotoModePhotoSequence()
    {
        return m_PhotoModeIsPhotoSequence;
    }

    _inline DWORD GetQueueId()
    {
        return m_dwWorkQueueId;
    }

    //
    //Will be used from Pins to get the D3D manager once set!!!
    //
    __inline STDMETHODIMP_(VOID) GetD3DDeviceManager(
        IUnknown** ppDeviceManagerUnk
        )
    {
        m_spDeviceManagerUnk.CopyTo( ppDeviceManagerUnk );
    }

    HRESULT SendEventToManager(
        _In_ MediaEventType,
        _In_ REFGUID,
        _In_ UINT32
        );

protected:

    //
    //Helper functions
    //

    HRESULT SetStreamingStateCustomPins(
        _In_ DeviceStreamState
        );

    CInPin* GetInPin(
        _In_ DWORD dwStreamID
        );

    COutPin* GetOutPin(
        _In_ DWORD dwStreamID
        );
    
    HRESULT GetConnectedInpin(_In_ ULONG ulOutpin, _Out_ ULONG &ulInPin);

    __requires_lock_held(m_critSec)
    HRESULT ChangeMediaTypeEx(
        _In_ ULONG pinId,
        _In_opt_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState newState
        );
    HRESULT BridgeInputPinOutputPin(
        _In_ CInPin* pInPin,
        _In_ COutPin* pOutPin);
#if defined (MF_DEVICEMFT_WARMSTART_HANDLING)
    HRESULT CMultipinMft::WarmStartHandler(
        _In_    PKSPROPERTY Property,
        _In_    ULONG       ulPropertyLength,
        _In_    LPVOID      pData,
        _In_    ULONG       ulOutputBufferLength,
        _Inout_   PULONG      pulBytesReturned
        );
#endif
#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
    STDMETHODIMP CMultipinMft::GetAttributes(
        _COM_Outptr_opt_result_maybenull_ IMFAttributes** ppAttributes
        );
#endif

#if (defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES))
    _DEFINE_DEVICEMFT_MFT0HELPER_IMPL__
#endif
    
    

    //
    //Inline functions
    //

    __inline IMFDeviceTransform* Parent()
    {
        return m_spSourceTransform.Get();
    }

    __inline VOID SetStreamingState(DeviceStreamState state)
    {
        InterlockedExchange((LONG*)&m_StreamingState, state);
    }
    __inline DeviceStreamState GetStreamingState()
    {
        return (DeviceStreamState)InterlockedCompareExchange((LONG*)&m_StreamingState, 0L, 0L);
    }
    __inline BOOL IsStreaming()
    {
        return (InterlockedCompareExchange((LONG*)&m_StreamingState, DeviceStreamState_Run, DeviceStreamState_Run) == DeviceStreamState_Run);
    }

private:
    ULONG                        m_InputPinCount;
    ULONG                        m_OutputPinCount;
    ULONG                        m_CustomPinCount;
    DeviceStreamState            m_StreamingState;
    CBasePinArray                m_OutPins;
    CBasePinArray                m_InPins;
    BOOL                         m_PhotoModeIsPhotoSequence;  // used to store if the filter is in photo sequence or not
    long                         m_nRefCount;                 // Reference count
    CCritSec                     m_critSec;                   // Control lock.. taken only durign state change operations   
    ComPtr <IUnknown>            m_spDeviceManagerUnk;        // D3D Manager set, when MFT_MESSAGE_SET_D3D_MANAGER is called through ProcessMessage
    ComPtr<IMFDeviceTransform>   m_spSourceTransform;         // The sources transform. This is the pipeline DevProxy
    MFSHUTDOWN_STATUS            m_eShutdownStatus;
    DWORD                        m_dwWorkQueueId;
    LONG                         m_lWorkQueuePriority;
    UINT32                       m_punValue;
    ComPtr<IKsControl>           m_spIkscontrol;
    ComPtr<IMFAttributes>        m_spAttributes;
    map<int, int>                m_outputPinMap;                      // How output pins are connected to input pins i-><0..outpins>
    CDMFTEventHandler            m_eventHandler;
    PWCHAR                       m_SymbolicLink;

#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    ComPtr<IMFAsyncCallback>    m_spPhotoConfirmationCallback;  //Photo Confirmation related definitions
    GUID                        m_guidPhotoConfirmationSubtype;
    BOOL                        m_firePhotoConfirmation;
#endif
#if defined (MF_DEVICEMFT_WARMSTART_HANDLING)
    VOID SetWarmStart(DWORD dwPinId, BOOL state)
    {
        m_dwWarmStartMask ^= (-state ^ m_dwWarmStartMask) & (1UL << dwPinId);
    }
    BOOLEAN GetWarmStart(_In_ DWORD dwPinId)
    {
        return ((m_dwWarmStartMask >> dwPinId) & 1UL);
    }
    DWORD                      m_dwWarmStartMask;
#endif

};



inline HRESULT MFT_CreateInstance(REFIID riid, void **ppv)
{
    return CMultipinMft::CreateInstance(riid, ppv);
}


