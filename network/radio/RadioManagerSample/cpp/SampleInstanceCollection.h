#pragma once

class ATL_NO_VTABLE CRadioInstanceCollection :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRadioInstanceCollection>,
    public IRadioInstanceCollection
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CRadioInstanceCollection)

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CRadioInstanceCollection)
        COM_INTERFACE_ENTRY(IRadioInstanceCollection)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    void    FinalRelease();

    static HRESULT CreateInstance(
        _In_                            DWORD cInstances,
        _In_reads_(cInstances)          IRadioInstance **rgpIRadioInstance,
        _COM_Outptr_                    IRadioInstanceCollection **ppInstanceCollection);

    IFACEMETHOD(GetCount)(_Out_ UINT32 *pcInstance);
    IFACEMETHOD(GetAt)(_In_ UINT32 uIndex, _COM_Outptr_ IRadioInstance **ppRadioInstance);

private:
    CAtlList<IRadioInstance *> _listRadioInstances;
};
