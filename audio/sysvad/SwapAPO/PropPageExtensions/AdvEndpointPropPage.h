//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    AdvEndpointPropPage.h
//
// Abstract:    Declaration of the CAdvEndpointPropPage class
//
// ----------------------------------------------------------------------------


#pragma once


class ATL_NO_VTABLE CAdvEndpointPropPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAdvEndpointPropPage, &CLSID_AdvEndpointPropPage>,
    public IDispatchImpl<IAdvEndpointPropPage, &__uuidof(IAdvEndpointPropPage), &LIBID_CplExtLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
    public IShellExtInit,
    public IShellPropSheetExt
{
public:
    CAdvEndpointPropPage();
    ~CAdvEndpointPropPage();

    DECLARE_REGISTRY_RESOURCEID(IDR_ADV_ENDPOINT_PROP_PAGE)

    BEGIN_COM_MAP(CAdvEndpointPropPage)
        COM_INTERFACE_ENTRY(IAdvEndpointPropPage)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IShellExtInit)
        COM_INTERFACE_ENTRY(IShellPropSheetExt)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct()
    {
        return S_OK;
    }

    void FinalRelease()
    {
    }

    static INT_PTR CALLBACK DialogProcPage1(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

    // IShellExtInit
    STDMETHOD(Initialize)(_In_opt_ LPCITEMIDLIST pidlFolder, _In_opt_ IDataObject* pdtobj, _In_opt_ HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)(_In_ LPFNADDPROPSHEETPAGE lpfnAddPage, _In_ LPARAM lParam);
    STDMETHOD(ReplacePage)(_In_ UINT uPageID, _In_ LPFNSVADDPROPSHEETPAGE lpfnReplaceWith, _In_ LPARAM lParam);

private:
    AudioExtensionParams* m_pAudioExtParams;
    TList<CUIWidget> m_lstWidgets;

    HRESULT FindHostConnector(PKSDATAFORMAT pKsFormat, ULONG cbFormat, BOOL bRejectMixedPaths, IPartsList** ppPath);
    HRESULT Search(IPart* pStart, DataFlow flowStart, IExaminer* pExaminer, CPartsList* pPath, UINT cDepth, BOOL bRejectMixedPaths);
    HRESULT InitializeWidgets();
    HRESULT GetDeviceFriendlyName(_Outptr_result_maybenull_ LPWSTR* ppNameOut);
    HRESULT MakeNewWidget(IPart* pPart, REFIID iid, UINT nCtrlId);
    BOOL OnInitDialog (HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    BOOL OnApply(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    BOOL OnCheckBoxClicked(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
};

OBJECT_ENTRY_AUTO(__uuidof(AdvEndpointPropPage), CAdvEndpointPropPage)
