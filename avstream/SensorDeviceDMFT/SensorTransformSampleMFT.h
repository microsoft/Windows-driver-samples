//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once
#include "common.h"
//////////////////////////////////////////////////////////////////////////
// Definition needed to communicate the aggregate device type to the camera pipeline
//////////////////////////////////////////////////////////////////////////
DEFINE_GUID(MF_DEVSOURCE_ATTRIBUTE_DEVICETYPE_priv, 0x4D8C34F7, 0x1178, 0x4EBD, 0x92, 0x78, 0x8C, 0xBB, 0x1E, 0x89, 0x95, 0xFB);


class CSensorTransformMFT 
:   public IMFDeviceTransform
,   public IMFRealTimeClientEx
,   public IMFShutdown
,   public IKsControl
,   public CMediaEventGenerator
,   public IMFAttributes
,   public IMFSensorTransformFactory
,   public CDMFTModuleLifeTimeManager
{
public:
    static HRESULT CreateInstance( _In_ REFIID riid, _COM_Outptr_ PVOID* ppCSensorTransformMFT);

    ///
    /// IUnknown
    IFACEMETHOD_(ULONG, AddRef)(void) override;
    IFACEMETHOD_(ULONG, Release)(void) override;
    IFACEMETHOD(QueryInterface)(_In_ REFIID riid, _Out_ void **ppvObject) override;

    ////////////////////////////////////////////////////////////////////////////
    // IMFDeviceTransform functions
    ///////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(InitializeTransform)(_In_ IMFAttributes *pAttributes );
    IFACEMETHOD(GetInputAvailableType)(_In_ DWORD dwInputStreamID, _In_ DWORD dwTypeIndex, _Out_ IMFMediaType** ppType);
    IFACEMETHOD(GetInputCurrentType)(_In_ DWORD dwInputStreamID, _COM_Outptr_result_maybenull_ IMFMediaType**  ppMediaType);
    IFACEMETHOD(GetInputStreamAttributes)(_In_ DWORD dwInputStreamID, _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes);
    IFACEMETHOD(GetOutputAvailableType)(_In_ DWORD dwOutputStreamID, _In_ DWORD dwTypeIndex, _Out_ IMFMediaType**  ppMediaType);
    IFACEMETHOD(GetOutputCurrentType)(_In_ DWORD dwOutputStreamID, _COM_Outptr_ IMFMediaType**  ppMediaType);
    IFACEMETHOD(GetOutputStreamAttributes)(_In_ DWORD dwOutputStreamID, _Out_ IMFAttributes** ppAttributes);
    IFACEMETHOD(GetStreamCount)(_Inout_ DWORD   *pdwInputStreams, _Inout_ DWORD   *pdwOutputStreams);
    IFACEMETHOD(GetStreamIDs)(_In_ DWORD  dwInputIDArraySize, _Out_writes_(dwInputIDArraySize) DWORD* pdwInputIDs, _In_ DWORD  dwOutputIDArraySize, _Out_writes_(dwOutputIDArraySize) DWORD* pdwOutputIDs);
    IFACEMETHOD(ProcessEvent)(_In_ DWORD dwInputStreamID, _In_ IMFMediaEvent *pEvent);
    IFACEMETHOD(ProcessInput)(_In_ DWORD dwInputStreamID, _In_ IMFSample* pSample, _In_ DWORD dwFlags );
    IFACEMETHOD(ProcessMessage)(_In_ MFT_MESSAGE_TYPE eMessage, _In_ ULONG_PTR ulParam);
    IFACEMETHOD(ProcessOutput)(_In_ DWORD dwFlags, _In_ DWORD cOutputBufferCount, _Inout_updates_(cOutputBufferCount) MFT_OUTPUT_DATA_BUFFER *pOutputSamples, _Out_ DWORD *pdwStatus );
    IFACEMETHOD(SetInputStreamState)(_In_ DWORD dwStreamID, _In_ IMFMediaType *pMediaType, _In_ DeviceStreamState value, _In_ DWORD dwFlags );
    IFACEMETHOD(GetInputStreamState)(_In_ DWORD dwStreamID, _Out_ DeviceStreamState *value );
    IFACEMETHOD(SetOutputStreamState)(_In_ DWORD dwStreamID, _In_ IMFMediaType *pMediaType, _In_ DeviceStreamState value, _In_ DWORD dwFlags );
    IFACEMETHOD(GetOutputStreamState)(_In_ DWORD dwStreamID, _Out_ DeviceStreamState *value );
    IFACEMETHOD(GetInputStreamPreferredState)(_In_ DWORD dwStreamID, _Inout_ DeviceStreamState *value, _Outptr_opt_result_maybenull_ IMFMediaType **ppMediaType );
    IFACEMETHOD(FlushInputStream)(_In_ DWORD dwStreamIndex, _In_ DWORD dwFlags );
    IFACEMETHOD(FlushOutputStream)(_In_ DWORD dwStreamIndex, _In_ DWORD dwFlags );     



    //////////////////////////////////////////////////////////////////////////
    // IMFRealTimeClientEx
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(RegisterThreadsEx)(_Inout_ DWORD* pdwTaskIndex, _In_ LPCWSTR wszClassName, _In_ LONG lBasePriority );
    IFACEMETHOD(UnregisterThreads)();
    IFACEMETHOD(SetWorkQueueEx)(_In_ DWORD dwWorkQueueId, _In_ LONG lWorkItemBasePriority);

    //////////////////////////////////////////////////////////////////////////
    // IMFShutdown
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(Shutdown)();
    IFACEMETHOD(GetShutdownStatus)(_Out_ MFSHUTDOWN_STATUS *pStatus);

    //////////////////////////////////////////////////////////////////////////
    // IKSControl Inferface function declarations
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(KsEvent)(_In_reads_bytes_(ulEventLength) PKSEVENT pEvent, _In_ ULONG ulEventLength, _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned);
    IFACEMETHOD(KsProperty)(_In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty, _In_ ULONG ulPropertyLength, _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned);
    IFACEMETHOD(KsMethod)(_In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty, _In_ ULONG ulPropertyLength, _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned);

    //////////////////////////////////////////////////////////////////////////
    // IMFAttributes functions
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(GetItem)(_In_ REFGUID guidKey, _Inout_opt_  PROPVARIANT* pValue) override;
    IFACEMETHOD(GetItemType)(_In_ REFGUID guidKey, _Out_ MF_ATTRIBUTE_TYPE* pType) override;
    IFACEMETHOD(CompareItem)(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value, _Out_ BOOL* pbResult) override;
    IFACEMETHOD(Compare)(_In_ IMFAttributes* pTheirs, _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType, _Out_ BOOL* pbResult) override;
    IFACEMETHOD(GetUINT32)(_In_ REFGUID guidKey, _Out_ UINT32* punValue) override;
    IFACEMETHOD(GetUINT64)(_In_ REFGUID guidKey, _Out_ UINT64* punValue) override;
    IFACEMETHOD(GetDouble)(_In_ REFGUID guidKey, _Out_ double* pfValue) override;
    IFACEMETHOD(GetGUID)(_In_ REFGUID guidKey, _Out_ GUID* pguidValue) override;
    IFACEMETHOD(GetStringLength)(_In_ REFGUID guidKey, _Out_ UINT32* pcchLength) override;
    IFACEMETHOD(GetString)(_In_ REFGUID guidKey, _Out_writes_(cchBufSize) LPWSTR pwszValue, _In_ UINT32 cchBufSize, _Inout_opt_ UINT32* pcchLength) override;
    IFACEMETHOD(GetAllocatedString)(_In_ REFGUID guidKey, _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue, _Inout_  UINT32* pcchLength) override;
    IFACEMETHOD(GetBlobSize)(_In_ REFGUID guidKey, _Out_ UINT32* pcbBlobSize) override;
    IFACEMETHOD(GetBlob)(_In_ REFGUID  guidKey, _Out_writes_(cbBufSize) UINT8* pBuf, UINT32 cbBufSize, _Inout_  UINT32* pcbBlobSize) override;
    IFACEMETHOD(GetAllocatedBlob)(__RPC__in REFGUID guidKey, __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf, __RPC__out UINT32* pcbSize) override;
    IFACEMETHOD(GetUnknown)(__RPC__in REFGUID guidKey, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID *ppv) override;
    IFACEMETHOD(SetItem)(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value) override;
    IFACEMETHOD(DeleteItem)(_In_ REFGUID guidKey) override;
    IFACEMETHOD(DeleteAllItems)() override;
    IFACEMETHOD(SetUINT32)(_In_ REFGUID guidKey, _In_ UINT32  unValue) override;
    IFACEMETHOD(SetUINT64)(_In_ REFGUID guidKey, _In_ UINT64  unValue) override;
    IFACEMETHOD(SetDouble)(_In_ REFGUID guidKey, _In_ double  fValue) override;
    IFACEMETHOD(SetGUID)(_In_ REFGUID guidKey, _In_ REFGUID guidValue) override;
    IFACEMETHOD(SetString)(_In_ REFGUID guidKey, _In_ LPCWSTR wszValue) override;
    IFACEMETHOD(SetBlob)(_In_ REFGUID guidKey, _In_reads_(cbBufSize) const UINT8* pBuf, UINT32 cbBufSize) override;
    IFACEMETHOD(SetUnknown)(_In_ REFGUID guidKey, _In_ IUnknown* pUnknown) override;
    IFACEMETHOD(LockStore)() override;
    IFACEMETHOD(UnlockStore)() override;
    IFACEMETHOD(GetCount)(_Out_ UINT32* pcItems) override;
    IFACEMETHOD(GetItemByIndex)(UINT32 unIndex, _Out_ GUID* pguidKey, _Inout_ PROPVARIANT* pValue) override;
    IFACEMETHOD(CopyAllItems)(_In_ IMFAttributes* pDest) override;

    //////////////////////////////////////////////////////////////////////////
    // IMFSensorTransformFactory
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(GetFactoryAttributes)(_COM_Outptr_ IMFAttributes** ppAttributes) override;
    IFACEMETHOD(InitializeFactory)(_In_ DWORD dwMaxTransformCount, _In_ IMFCollection* pSensorDevice, _In_opt_ IMFAttributes* pAttributes) override;
    IFACEMETHOD(GetTransformCount)(_Out_ DWORD* pdwCount) override;
    IFACEMETHOD(GetTransformInformation)(_In_ DWORD TransformIndex, _Out_ GUID* pguidTransformId, _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes, _COM_Outptr_ IMFCollection** ppStreamInformation) override;
    IFACEMETHOD(CreateTransform)(_In_ REFGUID TransformID, _In_opt_ IMFAttributes* pAttributes, _COM_Outptr_ IMFDeviceTransform** ppDeviceMFT) override;


protected:
    CSensorTransformMFT();
    virtual ~CSensorTransformMFT();

    virtual HRESULT InitializeDMFT();
    inline HRESULT  CheckShutdown() 
    {
        if (m_fShutdown) 
            return MF_E_SHUTDOWN; 
        return S_OK; 
    }

    LONG                               m_lRef;
    CCritSec                           m_lock;
    bool                               m_fShutdown;
    DWORD                              m_dwWorkQueueId;
    LONG                               m_lWorkItemBasePriority;
    ComPtr<IMFAttributes>              m_spDMFTInitAttributes;
    ComPtr<IMFAttributes>              m_spDMFTAttributes;
    vector<ComPtr<CSensorTransformMFTPin> >    m_aryInputPins;
    vector<ComPtr<CSensorTransformMFTPin> >    m_aryOutputPins;
    ComPtr<IKsControl>                 m_spKsControl;
    CTransformStateOperation           m_pStateOp;
};


