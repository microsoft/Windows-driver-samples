//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once
#include "common.h"
#include "mftpeventgenerator.h"
#include "basepin.h"
#include <optional>

//
// The Below GUID is needed to transfer photoconfirmation sample successfully in the pipeline
// It is used to propagate the mediatype of the sample to the pipeline which will consume the sample
// This attribute is known to the OS, but not publicly defined.
//

DEFINE_GUID(MFSourceReader_SampleAttribute_MediaType_priv,
    0x0ea5c1e8, 0x9845, 0x41e0, 0xa2, 0x43, 0x72, 0x32, 0x07, 0xfc, 0x78, 0x1f);


interface IDirect3DDeviceManager9;

constexpr int kMAX_WAIT_TIME_DRIVER_PROFILE_KSEVENT = 3000;// ms, amount of time to wait for the profile DDI KsEvent sent to the driver

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
    , public IMFShutdown
    , public CMediaEventGenerator
    , public IMFRealTimeClientEx
    , public IKsControl
    , public CDMFTModuleLifeTimeManager
{
    friend class CPinCreationFactory;
public:
    CMultipinMft(
        void );

    virtual ~CMultipinMft();

    //
    // IUnknown
    //
    IFACEMETHODIMP_(ULONG) AddRef(
        void );

    IFACEMETHODIMP_(ULONG) Release(
        void );

    IFACEMETHODIMP QueryInterface(
        _In_ REFIID iid,
        _COM_Outptr_ void** ppv);


    //
    // IMFDeviceTransform functions
    //
    IFACEMETHODIMP GetStreamCount (
        _Inout_ DWORD   *pdwInputStreams,
        _Inout_ DWORD   *pdwOutputStreams);


    IFACEMETHODIMP GetStreamIDs (
        _In_                                    DWORD  dwInputIDArraySize,
        _When_(dwInputIDArraySize >= m_InputPinCount, _Out_writes_(dwInputIDArraySize))  DWORD* pdwInputIDs,
        _In_                                    DWORD  dwOutputIDArraySize,
        _When_(dwOutputIDArraySize >= m_OutputPinCount && (pdwInputIDs && (dwInputIDArraySize > 0)),
        _Out_writes_(dwOutputIDArraySize)) _On_failure_(_Valid_) DWORD* pdwOutputIDs
        );

    IFACEMETHODIMP GetInputStreamAttributes(
        _In_        DWORD           dwInputStreamID,
        _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes);

    IFACEMETHODIMP GetOutputStreamAttributes(
        _In_        DWORD           dwOutputStreamID,
        _Out_ IMFAttributes** ppAttributes);

    IFACEMETHODIMP GetInputAvailableType(
        _In_        DWORD           dwInputStreamID,
        _In_        DWORD           dwTypeIndex,
        _Out_ IMFMediaType**  ppType);

    IFACEMETHODIMP GetOutputAvailableType(
        _In_        DWORD           dwOutputStreamID,
        _In_        DWORD           dwTypeIndex,
        _Out_ IMFMediaType**  ppMediaType);

    IFACEMETHODIMP GetInputCurrentType(
        _In_        DWORD           dwInputStreamID,
        _COM_Outptr_result_maybenull_ IMFMediaType**  ppMediaType);

    IFACEMETHODIMP GetOutputCurrentType(
        _In_        DWORD           dwOutputStreamID,
        _Out_       IMFMediaType**  ppMediaType);

    IFACEMETHODIMP ProcessMessage(
        _In_    MFT_MESSAGE_TYPE    eMessage,
        _In_    ULONG_PTR           ulParam );

    IFACEMETHODIMP ProcessEvent(
        _In_  DWORD dwInputStreamID,
        _In_  IMFMediaEvent *pEvent);


    IFACEMETHODIMP ProcessInput(
        _In_    DWORD       dwInputStreamID,
        _In_    IMFSample*  pSample,
        _In_    DWORD       dwFlags );

    IFACEMETHODIMP ProcessOutput(
        _In_    DWORD                       dwFlags,
        _In_    DWORD                       cOutputBufferCount,
        _Inout_updates_(cOutputBufferCount)  MFT_OUTPUT_DATA_BUFFER  *pOutputSamples,
        _Out_   DWORD                       *pdwStatus );

    //
    // IMFRealTimeClientEx
    //
    IFACEMETHODIMP RegisterThreadsEx(
        _Inout_ DWORD* pdwTaskIndex,
        _In_ LPCWSTR wszClassName,
        _In_ LONG lBasePriority )
    {
        UNREFERENCED_PARAMETER(pdwTaskIndex);
        UNREFERENCED_PARAMETER(wszClassName);
        UNREFERENCED_PARAMETER(lBasePriority);
        return S_OK;
    }

    IFACEMETHODIMP UnregisterThreads()
    {
        return S_OK;
    }

    IFACEMETHODIMP SetWorkQueueEx(
        _In_ DWORD dwWorkQueueId,
        _In_ LONG lWorkItemBasePriority );

    //
    // IMFShutdown
    //
    IFACEMETHODIMP Shutdown(
        void );

    IFACEMETHODIMP GetShutdownStatus(
        MFSHUTDOWN_STATUS *pStatus)
    {
        UNREFERENCED_PARAMETER(pStatus);
        return(m_eShutdownStatus);
    };

    //
    // IMFDeviceTransform function declarations
    //
    IFACEMETHODIMP InitializeTransform(
        _In_ IMFAttributes *pAttributes );

    _Requires_no_locks_held_
    IFACEMETHODIMP SetInputStreamState(
        _In_ DWORD dwStreamID,
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags );

    IFACEMETHODIMP GetInputStreamState(
        _In_ DWORD dwStreamID,
        _Out_ DeviceStreamState *value );

    IFACEMETHODIMP SetOutputStreamState(
        _In_ DWORD dwStreamID,
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags );

    IFACEMETHODIMP GetOutputStreamState(
        _In_ DWORD dwStreamID,
        _Out_ DeviceStreamState *value );

    IFACEMETHODIMP GetInputStreamPreferredState(
        _In_              DWORD                             dwStreamID,
        _Inout_           DeviceStreamState                 *value,
        _Outptr_opt_result_maybenull_ IMFMediaType          **ppMediaType );

    IFACEMETHODIMP FlushInputStream(
        _In_ DWORD dwStreamIndex,
        _In_ DWORD dwFlags );

    IFACEMETHODIMP FlushOutputStream(
        _In_ DWORD dwStreamIndex,
        _In_ DWORD dwFlags );

    IFACEMETHODIMP_(VOID) FlushAllStreams(
        VOID
        );

    //
    //IKSControl Inferface function declarations
    //
    IFACEMETHODIMP KsEvent(
        _In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
        _In_ ULONG ulEventLength,
        _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );
    IFACEMETHODIMP KsProperty(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );
    IFACEMETHODIMP KsMethod(
        _In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned
        );

    static HRESULT CreateInstance(
        REFIID iid, void **ppMFT);
 
    __inline BOOL isPhotoModePhotoSequence()
    {
        return m_PhotoModeIsPhotoSequence;
    }

    __inline DWORD GetQueueId()
    {
        return m_dwWorkQueueId;
    }

    //
    //Will be used from Pins to get the D3D manager once set!!!
    //
    __inline IFACEMETHODIMP_(VOID) GetD3DDeviceManager(
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

    HRESULT ProfilePropertyHandler(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_       ULONG       ulPropertyLength,
        _Inout_updates_to_(ulDataLength, *pulBytesReturned) LPVOID pPropertyData,
        _In_       ULONG       ulDataLength,
        _Inout_    PULONG      pulBytesReturned);

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
    PWCHAR                       m_SymbolicLink;
    HANDLE                       m_hSelectedProfileKSEvent;
    HANDLE                       m_hSelectedProfileKSEventSentToDriver;
    std::optional<bool>          m_isProfileDDISupportedInBaseDriver;
    SENSORPROFILEID              m_selectedProfileId;

};



inline HRESULT MFT_CreateInstance(REFIID riid, void **ppv)
{
    return CMultipinMft::CreateInstance(riid, ppv);
}


