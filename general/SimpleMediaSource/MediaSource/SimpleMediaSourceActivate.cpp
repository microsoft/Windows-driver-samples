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
        m_spSimpleMediaSrc = winrt::make_self<winrt::WindowsSample::implementation::SimpleMediaSource>();
        RETURN_IF_FAILED(m_spSimpleMediaSrc->Initialize());
        RETURN_IF_FAILED(m_spSimpleMediaSrc->QueryInterface(riid, ppv));

        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::ShutdownObject()
    {
        return S_OK;
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DetachObject()
    {
        m_spSimpleMediaSrc = nullptr;
        return S_OK;
    }

    // IMFAttributes (inherits by IMFActivate)
    IFACEMETHODIMP SimpleMediaSourceActivate::GetItem(_In_ REFGUID guidKey, _Inout_opt_  PROPVARIANT* pValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetItem(guidKey, pValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetItemType(_In_ REFGUID guidKey, _Out_ MF_ATTRIBUTE_TYPE* pType)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetItemType(guidKey, pType);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::CompareItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value, _Out_ BOOL* pbResult)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->CompareItem(guidKey, Value, pbResult);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::Compare(_In_ IMFAttributes* pTheirs, _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType, _Out_ BOOL* pbResult)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->Compare(pTheirs, MatchType, pbResult);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUINT32(_In_ REFGUID guidKey, _Out_ UINT32* punValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetUINT32(guidKey, punValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUINT64(_In_ REFGUID guidKey, _Out_ UINT64* punValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetUINT64(guidKey, punValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetDouble(_In_ REFGUID guidKey, _Out_ double* pfValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetDouble(guidKey, pfValue);
    }
    IFACEMETHODIMP SimpleMediaSourceActivate::GetGUID(_In_ REFGUID guidKey, _Out_ GUID* pguidValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetGUID(guidKey, pguidValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetStringLength(_In_ REFGUID guidKey, _Out_ UINT32* pcchLength)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetStringLength(guidKey, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetString(_In_ REFGUID guidKey, _Out_writes_(cchBufSize) LPWSTR pwszValue, _In_ UINT32 cchBufSize, _Inout_opt_ UINT32* pcchLength)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetAllocatedString(_In_ REFGUID guidKey, _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue, _Inout_  UINT32* pcchLength)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetBlobSize(_In_ REFGUID guidKey, _Out_ UINT32* pcbBlobSize)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetBlob(_In_ REFGUID  guidKey, _Out_writes_(cbBufSize) UINT8* pBuf, UINT32 cbBufSize, _Inout_  UINT32* pcbBlobSize)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetAllocatedBlob(__RPC__in REFGUID guidKey, __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf, __RPC__out UINT32* pcbSize)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetUnknown(__RPC__in REFGUID guidKey, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID* ppv)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetUnknown(guidKey, riid, ppv);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetItem(guidKey, Value);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DeleteItem(_In_ REFGUID guidKey)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->DeleteItem(guidKey);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::DeleteAllItems()
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->DeleteAllItems();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUINT32(_In_ REFGUID guidKey, _In_ UINT32  unValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetUINT32(guidKey, unValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUINT64(_In_ REFGUID guidKey, _In_ UINT64  unValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetUINT64(guidKey, unValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetDouble(_In_ REFGUID guidKey, _In_ double  fValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetDouble(guidKey, fValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetGUID(_In_ REFGUID guidKey, _In_ REFGUID guidValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetGUID(guidKey, guidValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetString(_In_ REFGUID guidKey, _In_ LPCWSTR wszValue)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetString(guidKey, wszValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetBlob(_In_ REFGUID guidKey, _In_reads_(cbBufSize) const UINT8* pBuf, UINT32 cbBufSize)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::SetUnknown(_In_ REFGUID guidKey, _In_ IUnknown* pUnknown) 
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->SetUnknown(guidKey, pUnknown);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::LockStore() 
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->LockStore();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::UnlockStore() 
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->UnlockStore();
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetCount(_Out_ UINT32* pcItems) 
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetCount(pcItems);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::GetItemByIndex(UINT32 unIndex, _Out_ GUID* pguidKey, _Inout_ PROPVARIANT* pValue) 
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    IFACEMETHODIMP SimpleMediaSourceActivate::CopyAllItems(_In_ IMFAttributes* pDest)
    {
        if (!m_spActivateAttributes)
            RETURN_IF_FAILED(MFCreateAttributes(&m_spActivateAttributes, 1));
        return m_spActivateAttributes->CopyAllItems(pDest);
    }
}
