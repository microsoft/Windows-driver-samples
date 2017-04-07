//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once
#include "Common.h"
class CSensorTransformMFTPin 
:   public IMFAttributes
,   public IKsControl
{
public:
    // This CreateInstance is called during Factory initialization.
    static HRESULT CreateInstance(_In_ DWORD PinId, 
                                  _In_ IMFSensorStream* pStream,
                                  _COM_Outptr_ CSensorTransformMFTPin** ppPin);

    // This is when we're about to be activated.
    static HRESULT CreateInstance(_In_ DWORD PinId,
                                  _In_ IMFMediaEventGenerator* pEventGenerator, 
                                  _In_ IMFTransform* pTransform,
                                  _COM_Outptr_ CSensorTransformMFTPin** ppPin);

    ///
    /// IUnknown
    IFACEMETHOD_(ULONG, AddRef)(void) override;
    IFACEMETHOD_(ULONG, Release)(void) override;
    IFACEMETHOD(QueryInterface)(_In_ REFIID riid, _Out_ void **ppvObject) override;

    ///
    /// IMFAttributes functions
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

    ///
    /// IKSControl Inferface function declarations
    IFACEMETHOD(KsEvent)(_In_reads_bytes_(ulEventLength) PKSEVENT pEvent, _In_ ULONG ulEventLength, _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned) override;
    IFACEMETHOD(KsProperty)(_In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty, _In_ ULONG ulPropertyLength, _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned) override;
    IFACEMETHOD(KsMethod)(_In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty, _In_ ULONG ulPropertyLength, _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, _In_ ULONG ulDataLength, _Inout_ ULONG* pBytesReturned) override;


    /// 
    /// Internal methods only invoked by CSensorTransformMFT. 
    IFACEMETHOD(GetAvailableMediaType)(_In_ DWORD dwIdx, _COM_Outptr_ IMFMediaType** ppMediaType);
    IFACEMETHOD(GetCurrentMediaType)(_COM_Outptr_ IMFMediaType** ppMediaType);
    IFACEMETHOD(GetStreamAttributes)(_COM_Outptr_ IMFAttributes** ppAttributes);
    IFACEMETHOD_(DWORD,GetStreamID)();
    IFACEMETHOD_(DeviceStreamState,GetStreamState)();
    IFACEMETHOD(SetStreamState)(_In_ DeviceStreamState eState, _In_opt_ IMFMediaType* pMediaType);
    IFACEMETHOD(SendInput)(_In_ IMFSample* pSample);
    IFACEMETHOD(GetOutput)(_COM_Outptr_ IMFSample** pSample);
    IFACEMETHOD_(void,Flush)();
    IFACEMETHOD_(void,Shutdown)();

protected:
    CSensorTransformMFTPin(_In_ DWORD PinId);
    virtual ~CSensorTransformMFTPin();

    virtual HRESULT                     InitializePin(_In_opt_ IMFSensorStream* pStream);
    virtual HRESULT                     InitializePinForActivation(_In_ IMFMediaEventGenerator* pEventGen, _In_ IMFTransform* pMFT);
    inline HRESULT CheckShutdown() 
    {
        if (m_fShutdown) 
            return MF_E_SHUTDOWN; 
        return S_OK; 
    }

    LONG                                m_lRef;
    CCritSec                            m_lock;
    bool                                m_fShutdown;
    ComPtr<IMFMediaEventGenerator>      m_spEventGenerator;
    ComPtr<IMFSensorStream>             m_spStream;
    std::vector<ComPtr<IMFMediaType> >  m_aryMediaTypes;
    ComPtr<IMFMediaType>                m_spCurrentMediaType;
    ComPtr<IMFAttributes>               m_spSourceAttributes;
    ComPtr<IKsControl>                  m_spKsControl;
    DWORD                               m_PinId;
    DeviceStreamState                   m_eStreamState;
    ComPtr<IMFSample>                   m_spSample;
};



