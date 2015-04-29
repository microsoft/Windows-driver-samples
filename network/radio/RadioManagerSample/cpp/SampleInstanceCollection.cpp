#include "precomp.h"
#pragma hdrstop

HRESULT CRadioInstanceCollection_CreateInstance(
    _In_                            DWORD cInstances,
    _In_reads_(cInstances)          IRadioInstance **rgpIRadioInstance,
    _COM_Outptr_                    IRadioInstanceCollection **ppInstanceCollection)
{
    return CRadioInstanceCollection::CreateInstance(cInstances, rgpIRadioInstance, ppInstanceCollection);
}

void CRadioInstanceCollection::FinalRelease()
{
    POSITION p;
    while (nullptr != (p = _listRadioInstances.GetHeadPosition()))
    {
        IRadioInstance *pRadioInstance = _listRadioInstances.GetAt(p);
        pRadioInstance->Release();
        _listRadioInstances.SetAt(p, nullptr);
        _listRadioInstances.RemoveHeadNoReturn();
    }
}

// static
HRESULT CRadioInstanceCollection::CreateInstance(
    _In_                            DWORD cInstances,
    _In_reads_(cInstances)       IRadioInstance **rgpIRadioInstance,
    _COM_Outptr_                    IRadioInstanceCollection **ppInstanceCollection)
{
    HRESULT hr;
    CComObject<CRadioInstanceCollection> *pInstanceCollection;
    CComPtr<IRadioInstanceCollection> spInstanceCollection;

    *ppInstanceCollection = nullptr;

    hr = CComObject<CRadioInstanceCollection>::CreateInstance(&pInstanceCollection);

    if (SUCCEEDED(hr) && (pInstanceCollection != nullptr))
    {
        hr = pInstanceCollection->QueryInterface(IID_PPV_ARGS(&spInstanceCollection));
    }

    if (SUCCEEDED(hr) && (pInstanceCollection != nullptr))
    {
        for (DWORD dwIndex = 0; dwIndex < cInstances; dwIndex++)
        {
            _ATLTRY
            {
                pInstanceCollection->_listRadioInstances.AddTail(rgpIRadioInstance[dwIndex]);
                rgpIRadioInstance[dwIndex]->AddRef();
            }
            _ATLCATCH(e)
            {
                hr = e;
                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppInstanceCollection = spInstanceCollection.Detach();
    }

    return hr;
}

IFACEMETHODIMP CRadioInstanceCollection::GetCount(_Out_ UINT32 *pcInstance)
{
    if (nullptr == pcInstance)
    {
        return E_INVALIDARG;
    }

    *pcInstance = static_cast<UINT32>(_listRadioInstances.GetCount());
    return S_OK;
}

IFACEMETHODIMP CRadioInstanceCollection::GetAt(_In_ UINT32 uIndex, _COM_Outptr_ IRadioInstance **ppRadioInstance)
{
    if (nullptr == ppRadioInstance)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    *ppRadioInstance = nullptr;

    POSITION p = _listRadioInstances.FindIndex(uIndex);
    *ppRadioInstance = _listRadioInstances.GetAt(p);
    if (nullptr != *ppRadioInstance)
    {
        (*ppRadioInstance)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
