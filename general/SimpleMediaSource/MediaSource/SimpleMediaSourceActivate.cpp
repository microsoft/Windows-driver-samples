//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
#include "pch.h"

namespace winrt::WindowsSample::implementation
{
    // IMFActivate
    IFACEMETHODIMP SimpleMediaSourceActivate::ActivateObject(REFIID riid, void** ppv)
    {
        RETURN_HR_IF_NULL(E_POINTER, ppv);
        *ppv = nullptr;

        wil::com_ptr_nothrow<IMFAttributes> spAttributes;
        RETURN_IF_FAILED(MFCreateAttributes(&spAttributes, 1));

        DEBUG_MSG(L"Activate SimpleMediaSource");
        m_ptr = winrt::make_self<winrt::WindowsSample::implementation::SimpleMediaSource>();
        RETURN_IF_FAILED(m_ptr->Initialize());
        RETURN_IF_FAILED(m_ptr->QueryInterface(riid, ppv));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::ShutdownObject()
    {
        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DetachObject()
    {
        if (m_ptr)
        {
            m_ptr.detach();
        }
        return S_OK;
    }

    // IMFAttributes (inherits by IMFActivate)
    IFACEMETHODIMP SimpleMediaSourceActivate::GetItem(_In_ REFGUID guidKey, _Inout_opt_  PROPVARIANT* pValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetItem(guidKey, pValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetItemType(_In_ REFGUID guidKey, _Out_ MF_ATTRIBUTE_TYPE* pType)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetItemType(guidKey, pType);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::CompareItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value, _Out_ BOOL* pbResult)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->CompareItem(guidKey, Value, pbResult);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::Compare(_In_ IMFAttributes* pTheirs, _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType, _Out_ BOOL* pbResult)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->Compare(pTheirs, MatchType, pbResult);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUINT32(_In_ REFGUID guidKey, _Out_ UINT32* punValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetUINT32(guidKey, punValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUINT64(_In_ REFGUID guidKey, _Out_ UINT64* punValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetUINT64(guidKey, punValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetDouble(_In_ REFGUID guidKey, _Out_ double* pfValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetDouble(guidKey, pfValue);
    }
    IFACEMETHODIMP SimpleMediaSourceActivate::GetGUID(_In_ REFGUID guidKey, _Out_ GUID* pguidValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetGUID(guidKey, pguidValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetStringLength(_In_ REFGUID guidKey, _Out_ UINT32* pcchLength)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetStringLength(guidKey, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetString(_In_ REFGUID guidKey, _Out_writes_(cchBufSize) LPWSTR pwszValue, _In_ UINT32 cchBufSize, _Inout_opt_ UINT32* pcchLength)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetAllocatedString(_In_ REFGUID guidKey, _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue, _Inout_  UINT32* pcchLength)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetBlobSize(_In_ REFGUID guidKey, _Out_ UINT32* pcbBlobSize)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetBlob(_In_ REFGUID  guidKey, _Out_writes_(cbBufSize) UINT8* pBuf, UINT32 cbBufSize, _Inout_  UINT32* pcbBlobSize)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetAllocatedBlob(__RPC__in REFGUID guidKey, __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf, __RPC__out UINT32* pcbSize)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUnknown(__RPC__in REFGUID guidKey, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID* ppv)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetUnknown(guidKey, riid, ppv);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetItem(guidKey, Value);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DeleteItem(_In_ REFGUID guidKey)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->DeleteItem(guidKey);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DeleteAllItems()
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->DeleteAllItems();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUINT32(_In_ REFGUID guidKey, _In_ UINT32  unValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetUINT32(guidKey, unValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUINT64(_In_ REFGUID guidKey, _In_ UINT64  unValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetUINT64(guidKey, unValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetDouble(_In_ REFGUID guidKey, _In_ double  fValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetDouble(guidKey, fValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetGUID(_In_ REFGUID guidKey, _In_ REFGUID guidValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetGUID(guidKey, guidValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetString(_In_ REFGUID guidKey, _In_ LPCWSTR wszValue)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetString(guidKey, wszValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetBlob(_In_ REFGUID guidKey, _In_reads_(cbBufSize) const UINT8* pBuf, UINT32 cbBufSize)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUnknown(_In_ REFGUID guidKey, _In_ IUnknown* pUnknown) 
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->SetUnknown(guidKey, pUnknown);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::LockStore() 
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->LockStore();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::UnlockStore() 
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->UnlockStore();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetCount(_Out_ UINT32* pcItems) 
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetCount(pcItems);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetItemByIndex(UINT32 unIndex, _Out_ GUID* pguidKey, _Inout_ PROPVARIANT* pValue) 
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::CopyAllItems(_In_ IMFAttributes* pDest)
    {
        if (!_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&_spActivateAttributes, 1));
        return _spActivateAttributes->CopyAllItems(pDest);
    }
}
