//*@@@+++@@@@******************************************************************
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//


#include "stdafx.h"


HRESULT
CSensorTransformMFTPin::CreateInstance(
    _In_ DWORD PinId, 
    _In_ IMFSensorStream* pStream,
    _COM_Outptr_ CSensorTransformMFTPin** ppPin
    )
{   
    HRESULT                         hr = S_OK;
    ComPtr<CSensorTransformMFTPin>  spPin;

    SDMFTCHECKNULL_GOTO (pStream, done, E_INVALIDARG);
    SDMFTCHECKNULL_GOTO (ppPin, done, E_POINTER);

    *ppPin = nullptr;
    spPin = new CSensorTransformMFTPin(PinId);
    SDMFTCHECKNULL_GOTO (spPin.Get(), done, E_OUTOFMEMORY);
    SDMFTCHECKHR_GOTO (spPin->InitializePin(pStream), done);
    SDMFTCHECKHR_GOTO (spPin->QueryInterface(__uuidof(IUnknown), reinterpret_cast<PVOID*>(ppPin)), done);
done:
    return hr;
}

HRESULT
CSensorTransformMFTPin::CreateInstance(
    _In_ DWORD PinId,
    _In_ IMFMediaEventGenerator* pEventGen, 
    _In_ IMFTransform* pMFT,
    _COM_Outptr_ CSensorTransformMFTPin** ppPin
    )
{
    HRESULT                         hr = S_OK;
    ComPtr<CSensorTransformMFTPin>  spPin;

    SDMFTCHECKNULL_GOTO (pEventGen, done, E_INVALIDARG);
    SDMFTCHECKNULL_GOTO (pMFT, done, E_INVALIDARG);
    SDMFTCHECKNULL_GOTO (ppPin, done, E_POINTER);

    *ppPin = nullptr;
    spPin = new CSensorTransformMFTPin(PinId);
    SDMFTCHECKNULL_GOTO (spPin.Get(), done, E_OUTOFMEMORY);
    SDMFTCHECKHR_GOTO (spPin->InitializePinForActivation(pEventGen, pMFT), done);
    SDMFTCHECKHR_GOTO(spPin->QueryInterface(__uuidof(IUnknown), reinterpret_cast<PVOID*>(ppPin)), done);
 done:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////// 
///  
CSensorTransformMFTPin::CSensorTransformMFTPin(
    _In_ DWORD PinId 
    )
:   m_lRef(0)
,   m_fShutdown(false)
,   m_PinId(PinId)
,   m_eStreamState(DeviceStreamState_Stop)
{

}

CSensorTransformMFTPin::~CSensorTransformMFTPin()
{
    m_aryMediaTypes.clear();
}
//////////////////////////////////////////////////////////////////////////
// IUnknown
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP_(ULONG)
CSensorTransformMFTPin::AddRef(
    )
{
    return InterlockedIncrement(&m_lRef);
}

IFACEMETHODIMP_(ULONG)
CSensorTransformMFTPin::Release(
    )
{
    LONG cRef = InterlockedDecrement(&m_lRef);
    if (0 == cRef)
    {
        delete this;
    }
    return(cRef);
}

IFACEMETHODIMP
CSensorTransformMFTPin::QueryInterface(
    _In_ REFIID iid,
    _Out_ LPVOID *ppv
    )
{
    HRESULT hr = S_OK;

    SDMFTCHECKNULL_GOTO (ppv, done, E_POINTER);
    *ppv = nullptr;

    if ((iid == __uuidof(IMFAttributes)) || (iid == __uuidof(IUnknown)))
    {
        *ppv = static_cast< IMFAttributes* >(this);
        AddRef();
    }
    else if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

done:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// IMFAttributes implementation
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFTPin::GetItem(
    _In_ REFGUID guidKey,
    _Inout_opt_  PROPVARIANT* pValue
    )
{
    return m_spStream->GetItem(guidKey, pValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetItemType(
    _In_ REFGUID guidKey,
    _Out_ MF_ATTRIBUTE_TYPE* pType
    )
{
    return m_spStream->GetItemType(guidKey, pType);
}

IFACEMETHODIMP
CSensorTransformMFTPin::CompareItem(
    _In_ REFGUID guidKey,
    _In_ REFPROPVARIANT Value,
    _Out_ BOOL* pbResult
    )
{
    return m_spStream->CompareItem(guidKey, Value, pbResult);
}

IFACEMETHODIMP
CSensorTransformMFTPin::Compare(
    _In_ IMFAttributes* pTheirs,
    _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType,
    _Out_ BOOL* pbResult
    )
{
    return m_spStream->Compare(pTheirs, MatchType, pbResult);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetUINT32(
    _In_ REFGUID guidKey,
    _Out_ UINT32* punValue
    )
{
    return m_spStream->GetUINT32(guidKey, punValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetUINT64(
    _In_ REFGUID guidKey,
    _Out_ UINT64* punValue
    )
{
    return m_spStream->GetUINT64(guidKey, punValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetDouble(
    _In_ REFGUID guidKey,
    _Out_ double* pfValue
    )
{
    return m_spStream->GetDouble(guidKey, pfValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetGUID(
    _In_ REFGUID guidKey,
    _Out_ GUID* pguidValue
    )
{
    return m_spStream->GetGUID(guidKey, pguidValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetStringLength(
    _In_ REFGUID guidKey,
    _Out_ UINT32* pcchLength
    )
{
    return m_spStream->GetStringLength(guidKey, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetString(
    _In_ REFGUID guidKey,
    _Out_writes_(cchBufSize) LPWSTR pwszValue,
    _In_ UINT32 cchBufSize,
    _Inout_opt_ UINT32* pcchLength
    )
{
    return m_spStream->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetAllocatedString(
    _In_ REFGUID guidKey,
    _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue,
    _Inout_  UINT32* pcchLength
    )
{
    return m_spStream->GetAllocatedString(guidKey, ppwszValue, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetBlobSize(
    _In_ REFGUID guidKey,
    _Out_ UINT32* pcbBlobSize
    )
{
    return m_spStream->GetBlobSize(guidKey, pcbBlobSize);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetBlob(
    _In_                    REFGUID  guidKey,
    _Out_writes_(cbBufSize) UINT8* pBuf,
    UINT32 cbBufSize,
    _Inout_  UINT32* pcbBlobSize
    )
{
    return m_spStream->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetAllocatedBlob(
    __RPC__in REFGUID guidKey,
    __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf,
    __RPC__out UINT32* pcbSize
    )
{
    return m_spStream->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetUnknown(
    __RPC__in REFGUID guidKey,
    __RPC__in REFIID riid,
    __RPC__deref_out_opt LPVOID *ppv
    )
{
    return m_spStream->GetUnknown(guidKey, riid, ppv);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetItem(
    _In_ REFGUID guidKey,
    _In_ REFPROPVARIANT Value
    )
{
    return m_spStream->SetItem(guidKey, Value);
}

IFACEMETHODIMP
CSensorTransformMFTPin::DeleteItem(
    _In_ REFGUID guidKey
    )
{
    return m_spStream->DeleteItem(guidKey);
}

IFACEMETHODIMP
CSensorTransformMFTPin::DeleteAllItems()
{
    return m_spStream->DeleteAllItems();
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetUINT32(
    _In_ REFGUID guidKey,
    _In_ UINT32  unValue
    )
{
    return m_spStream->SetUINT32(guidKey, unValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetUINT64(
    _In_ REFGUID guidKey,
    _In_ UINT64  unValue
    )
{
    return m_spStream->SetUINT64(guidKey, unValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetDouble(
    _In_ REFGUID guidKey,
    _In_ double  fValue
    )
{
    return m_spStream->SetDouble(guidKey, fValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetGUID(
    _In_ REFGUID guidKey,
    _In_ REFGUID guidValue
    )
{
    return m_spStream->SetGUID(guidKey, guidValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetString(
    _In_ REFGUID guidKey,
    _In_ LPCWSTR wszValue
    )
{
    return m_spStream->SetString(guidKey, wszValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetBlob(
    _In_ REFGUID guidKey,
    _In_reads_(cbBufSize) const UINT8* pBuf,
    UINT32 cbBufSize
    )
{
    return m_spStream->SetBlob(guidKey, pBuf, cbBufSize);
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetUnknown(
    _In_ REFGUID guidKey,
    _In_ IUnknown* pUnknown
    )
{
    return m_spStream->SetUnknown(guidKey, pUnknown);
}

IFACEMETHODIMP
CSensorTransformMFTPin::LockStore()
{
    return m_spStream->LockStore();
}

IFACEMETHODIMP
CSensorTransformMFTPin::UnlockStore()
{
    return m_spStream->UnlockStore();
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetCount(
    _Out_ UINT32* pcItems
    )
{
    return m_spStream->GetCount(pcItems);
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetItemByIndex(
    UINT32 unIndex,
    _Out_ GUID* pguidKey,
    _Inout_ PROPVARIANT* pValue
    )
{
    return m_spStream->GetItemByIndex(unIndex, pguidKey, pValue);
}

IFACEMETHODIMP
CSensorTransformMFTPin::CopyAllItems(
    _In_ IMFAttributes* pDest
    )
{
    return m_spStream->CopyAllItems(pDest);
}

//////////////////////////////////////////////////////////////////////////
// IKSControl Inferface function declarations
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFTPin::KsEvent(
    _In_reads_bytes_(ulEventLength) PKSEVENT pEvent, 
    _In_ ULONG ulEventLength, 
    _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsEvent(pEvent, ulEventLength, pEventData, ulDataLength, pBytesReturned), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::KsProperty(
    _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty, 
    _In_ ULONG ulPropertyLength, 
    _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned)
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsProperty(pProperty, ulPropertyLength, pPropertyData, ulDataLength, pBytesReturned), done)

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::KsMethod(
    _In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty, 
    _In_ ULONG ulPropertyLength, 
    _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsMethod(pProperty, ulPropertyLength, pPropertyData, ulDataLength, pBytesReturned), done);

done:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// Internal methods only invoked by CSensorTransformMFT. 
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFTPin::GetAvailableMediaType(
    _In_ DWORD dwIdx, 
    _COM_Outptr_ IMFMediaType** ppMediaType
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppMediaType, done, E_POINTER);
    *ppMediaType = nullptr;
    if (dwIdx >= m_aryMediaTypes.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_NO_MORE_TYPES, done);
    }

    SDMFTCHECKHR_GOTO (m_aryMediaTypes[dwIdx]->QueryInterface(IID_PPV_ARGS(ppMediaType)), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetCurrentMediaType(
    _COM_Outptr_ IMFMediaType** ppMediaType
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppMediaType, done, E_POINTER);
    if (m_spCurrentMediaType)
    {
        SDMFTCHECKHR_GOTO (m_spCurrentMediaType->QueryInterface(IID_PPV_ARGS(ppMediaType)), done);
    }
    else
    {
        SDMFTCHECKHR_GOTO (MF_E_TRANSFORM_TYPE_NOT_SET, done);
    }

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetStreamAttributes(
    _COM_Outptr_ IMFAttributes** ppAttributes
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppAttributes, done, E_POINTER);
    SDMFTCHECKHR_GOTO (m_spStream->QueryInterface(IID_PPV_ARGS(ppAttributes)), done);

done:
    return hr;
}

IFACEMETHODIMP_(DWORD)
CSensorTransformMFTPin::GetStreamID(
    )
{
    return m_PinId;
}

IFACEMETHODIMP_(DeviceStreamState)
CSensorTransformMFTPin::GetStreamState(
    )
{
    CAutoLock     lock( &m_lock );
    return m_eStreamState;
}

IFACEMETHODIMP
CSensorTransformMFTPin::SetStreamState(
    _In_ DeviceStreamState eState,
    _In_opt_ IMFMediaType* pMediaType
    )
{
    CAutoLock     lock( &m_lock );
    HRESULT         hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);

    m_eStreamState = eState;
    if (m_eStreamState == DeviceStreamState_Stop)
    {
        // Our version of "flush".
        m_spSample = nullptr;
        m_spCurrentMediaType = nullptr;
    }
    else
    {
        size_t   cCount = m_aryMediaTypes.size();
        bool    fFound = false;

        SDMFTCHECKNULL_GOTO (pMediaType, done, MF_E_TRANSFORM_TYPE_NOT_SET);
        for (DWORD i = 0; i < (DWORD)cCount; i++)
        {
            DWORD dwFlag = 0;

            if (m_aryMediaTypes[i]->IsEqual(pMediaType, &dwFlag) == S_OK)
            {
                m_spCurrentMediaType = pMediaType;
                fFound = true;
                break;
            }
        }

        if (!fFound)
        {
            SDMFTCHECKHR_GOTO (MF_E_INVALIDMEDIATYPE, done);
        }
    }

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::SendInput(
    _In_ IMFSample* pSample
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (m_eStreamState != DeviceStreamState_Run)
    {
        SDMFTCHECKHR_GOTO (MF_E_NOTACCEPTING, done);
    }
    if (nullptr != pSample)
    {
        m_spSample = pSample;
        (void)m_spEventGenerator->QueueEvent(METransformHaveOutput, GUID_NULL, S_OK, nullptr);
    }

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFTPin::GetOutput(
    _COM_Outptr_ IMFSample** ppSample
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (!m_spSample)
    {
        // This should never, ever happen...
        SDMFTCHECKHR_GOTO (MF_E_TRANSFORM_NEED_MORE_INPUT, done);
    }
    SDMFTCHECKHR_GOTO (m_spSample->QueryInterface(IID_PPV_ARGS(ppSample)), done);
    m_spSample = nullptr;

done:
    return hr;
}

IFACEMETHODIMP_(void)
CSensorTransformMFTPin::Flush(
    )
{
    CAutoLock lock( &m_lock );

    m_spSample = nullptr;
}

IFACEMETHODIMP_(void)
CSensorTransformMFTPin::Shutdown(
    )
{
    CAutoLock lock( &m_lock );

    m_fShutdown = true;
    m_spSample = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////// 
///  
HRESULT
CSensorTransformMFTPin::InitializePin(
    _In_ IMFSensorStream* pStream
    )
{
    HRESULT                     hr = S_OK;
    ComPtr<IMFSensorStream>    spStream;

    SDMFTCHECKNULL_GOTO (pStream, done, E_INVALIDARG);
    SDMFTCHECKHR_GOTO (pStream->CloneSensorStream(&spStream), done);
    m_spStream = spStream;

done:
    return hr;
}

HRESULT
CSensorTransformMFTPin::InitializePinForActivation(
    _In_ IMFMediaEventGenerator* pEventGen,
    _In_ IMFTransform*           pMFT
    )
{
    HRESULT                         hr = S_OK;
    ComPtr<IMFCollection>          spMTCollection;
    ComPtr<IMFAttributes>          spPinAttributes;
    ComPtr<IMFAttributes>          spSrcAttributes;
    ComPtr<IMFSensorStream>        spStream;
    ComPtr<IKsControl>             spKsControl;
    PFNCREATESENSORSTREAM pfnCreateStream = NULL;

    SDMFTCHECKNULL_GOTO (pEventGen, done, E_INVALIDARG);
    SDMFTCHECKNULL_GOTO (pMFT, done, E_INVALIDARG);
    SDMFTCHECKHR_GOTO (MFCreateCollection(&spMTCollection), done);

    for (DWORD i = 0; ;i++)
    {
        ComPtr<IUnknown>       spUnknown;
        ComPtr<IMFMediaType>   spMediaType;

        hr = pMFT->GetOutputAvailableType(m_PinId, i, &spMediaType);
        if (hr == MF_E_NO_MORE_TYPES)
        {
            hr = S_OK;
            break;
        }
        SDMFTCHECKHR_GOTO (spMediaType->QueryInterface(IID_PPV_ARGS(&spUnknown)), done);
        SDMFTCHECKHR_GOTO (spMTCollection->AddElement(spUnknown.Get()), done);
        hr = ExceptionBoundary([this, spMediaType]() { m_aryMediaTypes.push_back(spMediaType);});
        SDMFTCHECKHR_GOTO (hr, done);
    }
  
    SDMFTCHECKHR_GOTO (pMFT->GetAttributes(&spSrcAttributes), done);
    SDMFTCHECKHR_GOTO (pMFT->GetOutputStreamAttributes(m_PinId, &spPinAttributes), done);
    (VOID)spPinAttributes->GetUnknown(MF_DEVICESTREAM_PIN_KSCONTROL, IID_PPV_ARGS(&spKsControl));
    // Call into the sensor group API to create the stream
    pfnCreateStream = (PFNCREATESENSORSTREAM)GetProcAddress(CLoader::GetLoader()->CLoader::GetModule(), "MFCreateSensorStream");
    SDMFTCHECKNULL_GOTO(pfnCreateStream, done, E_NOTIMPL);
    SDMFTCHECKHR_GOTO (pfnCreateStream(m_PinId, spPinAttributes.Get(), spMTCollection.Get(), spStream.GetAddressOf()), done);
    SDMFTCHECKHR_GOTO (pMFT->GetOutputCurrentType(m_PinId, &m_spCurrentMediaType), done);

    m_spSourceAttributes = spSrcAttributes;
    m_spKsControl = spKsControl;
    m_spStream = spStream;
    m_spEventGenerator = pEventGen;

done:
    return hr;
}


