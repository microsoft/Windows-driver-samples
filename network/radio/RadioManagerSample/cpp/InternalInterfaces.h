#pragma once

interface __declspec(uuid("{931D7B9C-11FB-45B6-97DA-47894D597F39}")) ISampleRadioInstanceInternal : public IUnknown
{
    IFACEMETHOD(OnSysRadioChange)(_In_ SYSTEM_RADIO_STATE sysRadioState) = 0;
};

interface __declspec(uuid("{98595167-4F34-4247-A669-CA41ABBC479A}")) ISampleRadioManagerInternal : public IUnknown
{
    IFACEMETHOD(OnInstanceRadioChange)(_In_ BSTR bstrRadioInstanceID, _In_ DEVICE_RADIO_STATE radioState) = 0;
};


HRESULT CSampleRadioInstance_CreateInstance(
    _In_                            PCWSTR pszKeyName,
    _In_                            ISampleRadioManagerInternal *pParentManager,
    _COM_Outptr_                    ISampleRadioInstanceInternal **ppRadioInstance);

HRESULT CRadioInstanceCollection_CreateInstance(
    _In_                            DWORD cInstances,
    _In_reads_(cInstances)          IRadioInstance **rgpIRadioInstance,
    _COM_Outptr_                    IRadioInstanceCollection **ppInstanceCollection);

