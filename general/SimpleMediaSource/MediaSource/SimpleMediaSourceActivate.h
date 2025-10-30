//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
#pragma once
#ifndef SIMPLEMEDIASOURCEACTIVATE_H
#define SIMPLEMEDIASOURCEACTIVATE_H

namespace winrt::WindowsSample::implementation
{
    struct __declspec(uuid("9812588D-5CE9-4E4C-ABC1-049138D10DCE"))
        SimpleMediaSourceActivate : winrt::implements<SimpleMediaSourceActivate, IMFActivate>
    {
        SimpleMediaSourceActivate() = default;

    public:
        // IMFActivate
        IFACEMETHODIMP ActivateObject(REFIID riid, void** ppv) override;
        IFACEMETHODIMP ShutdownObject() override;
        IFACEMETHODIMP DetachObject() override;

        // IMFAttributes (inherits by IMFActivate)
        IFACEMETHODIMP GetItem(_In_ REFGUID guidKey, _Inout_opt_  PROPVARIANT* pValue) override;
        IFACEMETHODIMP GetItemType(_In_ REFGUID guidKey, _Out_ MF_ATTRIBUTE_TYPE* pType) override;
        IFACEMETHODIMP CompareItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value, _Out_ BOOL* pbResult) override;
        IFACEMETHODIMP Compare(_In_ IMFAttributes* pTheirs, _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType, _Out_ BOOL* pbResult) override;
        IFACEMETHODIMP GetUINT32(_In_ REFGUID guidKey, _Out_ UINT32* punValue) override;
        IFACEMETHODIMP GetUINT64(_In_ REFGUID guidKey, _Out_ UINT64* punValue) override;
        IFACEMETHODIMP GetDouble(_In_ REFGUID guidKey, _Out_ double* pfValue) override;
        IFACEMETHODIMP GetGUID(_In_ REFGUID guidKey, _Out_ GUID* pguidValue) override;
        IFACEMETHODIMP GetStringLength(_In_ REFGUID guidKey, _Out_ UINT32* pcchLength) override;
        IFACEMETHODIMP GetString(_In_ REFGUID guidKey, _Out_writes_(cchBufSize) LPWSTR pwszValue, _In_ UINT32 cchBufSize, _Inout_opt_ UINT32* pcchLength) override;
        IFACEMETHODIMP GetAllocatedString(_In_ REFGUID guidKey, _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue, _Inout_  UINT32* pcchLength) override;
        IFACEMETHODIMP GetBlobSize(_In_ REFGUID guidKey, _Out_ UINT32* pcbBlobSize) override;
        IFACEMETHODIMP GetBlob(_In_ REFGUID  guidKey, _Out_writes_(cbBufSize) UINT8* pBuf, UINT32 cbBufSize, _Inout_  UINT32* pcbBlobSize) override;
        IFACEMETHODIMP GetAllocatedBlob(__RPC__in REFGUID guidKey, __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf, __RPC__out UINT32* pcbSize) override;
        IFACEMETHODIMP GetUnknown(__RPC__in REFGUID guidKey, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID* ppv) override;
        IFACEMETHODIMP SetItem(_In_ REFGUID guidKey, _In_ REFPROPVARIANT Value) override;
        IFACEMETHODIMP DeleteItem(_In_ REFGUID guidKey) override;
        IFACEMETHODIMP DeleteAllItems() override;
        IFACEMETHODIMP SetUINT32(_In_ REFGUID guidKey, _In_ UINT32  unValue) override;
        IFACEMETHODIMP SetUINT64(_In_ REFGUID guidKey, _In_ UINT64  unValue) override;
        IFACEMETHODIMP SetDouble(_In_ REFGUID guidKey, _In_ double  fValue) override;
        IFACEMETHODIMP SetGUID(_In_ REFGUID guidKey, _In_ REFGUID guidValue) override;
        IFACEMETHODIMP SetString(_In_ REFGUID guidKey, _In_ LPCWSTR wszValue) override;
        IFACEMETHODIMP SetBlob(_In_ REFGUID guidKey, _In_reads_(cbBufSize) const UINT8* pBuf, UINT32 cbBufSize) override;
        IFACEMETHODIMP SetUnknown(_In_ REFGUID guidKey, _In_ IUnknown* pUnknown) override;
        IFACEMETHODIMP LockStore() override;
        IFACEMETHODIMP UnlockStore() override;
        IFACEMETHODIMP GetCount(_Out_ UINT32* pcItems) override;
        IFACEMETHODIMP GetItemByIndex(UINT32 unIndex, _Out_ GUID* pguidKey, _Inout_ PROPVARIANT* pValue) override;
        IFACEMETHODIMP CopyAllItems(_In_ IMFAttributes* pDest) override;

    private:
        wil::com_ptr_nothrow <IMFAttributes> m_spActivateAttributes;
        winrt::com_ptr<winrt::WindowsSample::implementation::SimpleMediaSource> m_spSimpleMediaSrc;
    };
}

//
// cocreatable class
//
struct SimpleMediaSourceActivateFactory : public winrt::implements<SimpleMediaSourceActivateFactory, IClassFactory>
{
public:
    STDMETHODIMP CreateInstance(_In_ IUnknown*, _In_ REFIID riid, _COM_Outptr_ void** result) noexcept final try
    {
        RETURN_HR_IF_NULL(E_POINTER, result);
        *result = nullptr;

        winrt::com_ptr<winrt::WindowsSample::implementation::SimpleMediaSourceActivate> ptr = winrt::make_self<winrt::WindowsSample::implementation::SimpleMediaSourceActivate>();
        return ptr->QueryInterface(riid, result);
    } CATCH_RETURN()


    STDMETHODIMP LockServer(BOOL) noexcept final
    {
        return S_OK;
    }
};


#endif
