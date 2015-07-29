//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    SwapPropPage.h
//
// Abstract:    Declaration of the CSwapPropPage class
//
// ----------------------------------------------------------------------------


#pragma once

// ----------------------------------------------------------------------
// CSwapPropPage class
//
class ATL_NO_VTABLE CSwapPropPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSwapPropPage, &CLSID_SwapPropPage>,
    public IDispatchImpl<ISwapPropPage, &__uuidof(ISwapPropPage), &LIBID_CplExtLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
    public IShellExtInit,
    public IShellPropSheetExt
{
public:
    CSwapPropPage();
    ~CSwapPropPage();

    DECLARE_REGISTRY_RESOURCEID(IDR_SWAP_PROP_PAGE)

    BEGIN_COM_MAP(CSwapPropPage)
        COM_INTERFACE_ENTRY(ISwapPropPage)
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
    AudioFXExtensionParams* m_pAudioFXExtParams;
    BOOL m_fDisableSysFX;
    BOOL m_fEnableSwapSFX;
    BOOL m_fEnableSwapMFX;
    BOOL m_fEnableDelaySFX;
    BOOL m_fEnableDelayMFX;
    BOOL m_fReset;

    HRESULT GetDeviceFriendlyName(_Outptr_result_maybenull_ LPWSTR* ppNameOut);
    HRESULT RetrieveSysFXState(BOOL *pfDisabled);
    HRESULT SetSysFXState();
    HRESULT RetrieveSwapSFXState(BOOL *pfEnabled);
    HRESULT SetSwapSFXState();
    HRESULT RetrieveSwapMFXState(BOOL *pfEnabled);
    HRESULT SetSwapMFXState();
    HRESULT RetrieveDelaySFXState(BOOL *pfEnabled);
    HRESULT SetDelaySFXState();
    HRESULT RetrieveDelayMFXState(BOOL *pfEnabled);
    HRESULT SetDelayMFXState();

    BOOL OnInitDialog(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    BOOL OnApply(HWND hwndDlg);
    BOOL OnCheckBoxClickedDisableSysFX(HWND hwndDlg);
    BOOL OnCheckBoxClickedEnableSwapSFX(HWND hwndDlg);
    BOOL OnCheckBoxClickedEnableSwapMFX(HWND hwndDlg);
    BOOL OnCheckBoxClickedEnableDelaySFX(HWND hwndDlg);
    BOOL OnCheckBoxClickedEnableDelayMFX(HWND hwndDlg);
};

OBJECT_ENTRY_AUTO(__uuidof(SwapPropPage), CSwapPropPage)
