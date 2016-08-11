//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"
#include "ihvsample_i.c"

extern HINSTANCE g_hInst;

LPWSTR g_IHVAuthFriendlyName[] = {
    L"IHVAuth V1", 
    L"IHVAuth V2",
    L"IHVAuth V3"
};

LPWSTR g_IHVCipherFriendlyName[] = {
    L"None",
    L"IHVCipher 1",
    L"IHVCipher 2",
    L"IHVCipher 3"
};

IHV_AUTH_CIPHER_CAPABILITY g_IHVOneXExtCapability = 
{
    3,
    {
        {
            IHVAuthV1,
            1,
            {IHVCipher1}
        },
        {
            IHVAuthV2,
            3,
            {None, IHVCipher1, IHVCipher2}
        },
        {
            IHVAuthV3,
            2,
            {IHVCipher2, IHVCipher3}
        }
    }
};


static const WCHAR c_szIhvUIRequest[] = L"_UI_Request";
static const WCHAR c_szIhvUIResponse[] = L"_UI_Response";

template<typename T>
T* GetThis(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR c_szThisPointer[]= L"_Win32_this_";
    UNREFERENCED_PARAMETER(wParam);

    T* pThis = NULL;
    if (uMsg == WM_INITDIALOG)
    {
        if (sizeof(PROPSHEETPAGE) == ((LPPROPSHEETPAGE)lParam)->dwSize)
        {
            // This corresponds to MSDN 
            pThis = (T *)((LPPROPSHEETPAGE)lParam)->lParam;
        }
        else
        {
            // TODO: Need to determine when this abnormality happens...
            pThis = (T *)lParam;
        }

        SetProp(hwnd, c_szThisPointer, (HANDLE)pThis);

    }
    else if (uMsg == WM_DESTROY)
    {
        RemoveProp(hwnd, c_szThisPointer);
    }
    else
    {
        pThis = (T *)GetProp(hwnd, c_szThisPointer);
    }

    return pThis;
}


CDot11SampleExtUI::CDot11SampleExtUI(): m_crefCount(0)
{
    InterlockedIncrement(&g_objRefCount);
    
    m_pUnkSite = NULL;
    m_hFirstPagePsp = NULL;
    m_hLastPagePsp = NULL;
    m_pUIRequest = NULL;
}

CDot11SampleExtUI::~CDot11SampleExtUI()
{
    InterlockedDecrement(&g_objRefCount);        

    if( m_pUIRequest)
    {
        delete m_pUIRequest;
    }
    
    if( m_pUnkSite)
    {
        m_pUnkSite ->Release();
        m_pUnkSite = NULL;
    }
}

// Used to get the IHV friendly name
STDMETHODIMP
CDot11SampleExtUI::GetDot11ExtUIFriendlyName( 
    BSTR* bstrFriendlyName)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != bstrFriendlyName)
    {
        *bstrFriendlyName = SysAllocString(IHV_SAMPLE_IHV_NAME);
        hr = S_OK;
    }

    return hr;
}


// Returns the requested property type
STDMETHODIMP
CDot11SampleExtUI::GetDot11ExtUIProperties(
    DOT11_EXT_UI_PROPERTY_TYPE ExtType,
    ULONG *pcExtensions,
    IDot11ExtUIProperty **ppDot11ExtUIProperty
    )
{
    HRESULT hr = S_OK;
    if (!pcExtensions || !ppDot11ExtUIProperty)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    // Initialize the out parameters
    *pcExtensions = 0;
    *ppDot11ExtUIProperty = NULL;

    switch(ExtType)
    {
        case DOT11_EXT_UI_CONNECTION:
            hr = CreateConnectionProperties(pcExtensions, ppDot11ExtUIProperty);
            break;

        case DOT11_EXT_UI_SECURITY:
            hr = CreateSecurityProperties(pcExtensions, ppDot11ExtUIProperty);
            break;

        case DOT11_EXT_UI_KEYEXTENSION:
            hr = CreateKeyProperties(pcExtensions, ppDot11ExtUIProperty);
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }
    
error:
    return hr;
}

#define IHV_BALLOON_TEXT L"Please enter key information"

STDMETHODIMP
CDot11SampleExtUI::GetDot11ExtUIBalloonText(
    BSTR pIHVUIRequest, // the UI request structure from IHV
    BSTR* pwszBalloonText // the balloon text to be displayed
    ) 
{
    HRESULT hr = E_INVALIDARG;
    PDOT11EXT_IHV_UI_REQUEST pIhvUiRequest = (PDOT11EXT_IHV_UI_REQUEST) pIHVUIRequest;

    if (NULL != pwszBalloonText)
    {
        // Ihv could choose to parse the UI request data here ...
        UNREFERENCED_PARAMETER( pIhvUiRequest );

        *pwszBalloonText = SysAllocString( IHV_BALLOON_TEXT );
        hr = S_OK;
    }

    return hr;
}



HRESULT
CDot11SampleExtUI::CreateConnectionProperties(
    ULONG *pcExtensions,
    IDot11ExtUIProperty **ppDot11ExtUIProperty
    )
{
    HRESULT hr = ERROR_SUCCESS;
    BSTR strName = NULL;
    ULONG uCount = 0;

    IDot11SampleExtUIConProperty **pprgProps = NULL;

    uCount = PROP_COUNT_CONNECTION;
    pprgProps = (IDot11SampleExtUIConProperty**) 
                    CoTaskMemAlloc(sizeof(IDot11SampleExtUIConProperty*) * uCount);

    if (!pprgProps)
    {
        hr = E_UNEXPECTED;
        goto error;
    }

    // Since we just have one property of each, we'll 
    // create one interface first and initialize it separately
    IDot11SampleExtUIConProperty *pTempIProp = NULL;
    hr = CoCreateInstance(
                GUID_SAMPLE_IHVUI_CLSID,
                NULL,
                CLSCTX_INPROC,
                IID_IDot11SampleExtUIConProperty,
                (PVOID*)&pTempIProp
                );

    if (FAILED(hr))
    {
        goto error;
    }

    // this will probably never be displayed
    strName = SysAllocString(L"IHV Connection Settings");
    hr = pTempIProp->Initialize(strName);
    pprgProps[0] = pTempIProp;

    if (SUCCEEDED(hr))
    {
        *pcExtensions = uCount;
        *ppDot11ExtUIProperty = (IDot11ExtUIProperty*)pprgProps; 

        // Set the current pointer to NULL so it doesn't get freed at the bottom
        pprgProps = NULL;
    }
    
error:
    if (FAILED(hr) && pprgProps)
    {
        CoTaskMemFree(pprgProps);
        pprgProps = NULL;
    }
    SysFreeString(strName);
    return hr;
}

HRESULT
CDot11SampleExtUI::CreateSecurityProperties(
    ULONG *pcExtensions,
    IDot11ExtUIProperty **ppDot11ExtUIProperty
    )
{
    HRESULT hr = ERROR_SUCCESS;
    BSTR strName = NULL;
    ULONG uCount = 0;
    DWORD i = 0;
    WCHAR wbuf[128];

    IDot11SampleExtUISecProperty **pprgProps = NULL;

    uCount = PROP_COUNT_SECURITY;
    pprgProps = (IDot11SampleExtUISecProperty**) 
                    CoTaskMemAlloc(sizeof(IDot11SampleExtUISecProperty*) * uCount);

    if (!pprgProps)
    {
        hr = E_UNEXPECTED;
        goto error;
    }

    // Since we just have one property of each, we'll 
    // create one interface first and initialize it separately
    IDot11SampleExtUISecProperty *pTempIProp = NULL;

    for (i = 0; i < uCount; ++i)
    {
        ZeroMemory(
            wbuf,
            128
            );
        pTempIProp = NULL;
        SysFreeString(strName);
        strName = NULL;

        hr = CoCreateInstance(
                    GUID_SAMPLE_IHVUI_CLSID,
                    NULL,
                    CLSCTX_INPROC,
                    IID_IDot11SampleExtUISecProperty,
                    (PVOID*)&pTempIProp
                    );

        if (FAILED(hr))
        {
            continue;
        }

        StringCchPrintf(
            wbuf,
            128,
            wstrSecurityTypes[i]
            );
            
        strName = SysAllocString(wbuf);
        hr = pTempIProp->Initialize(strName, i);
        pprgProps[i] = pTempIProp;
    }
    
    if (SUCCEEDED(hr))
    {
        *pcExtensions = uCount;
        *ppDot11ExtUIProperty = (IDot11ExtUIProperty*)pprgProps; 

        // Set the current pointer to NULL so it doesn't get freed at the bottom
        pprgProps = NULL;
    }
    
error:
    if (FAILED(hr) && pprgProps)
    {
        CoTaskMemFree(pprgProps);
        pprgProps = NULL;
    }
    SysFreeString(strName);
    return hr;
}

HRESULT
CDot11SampleExtUI::CreateKeyProperties(
    ULONG *pcExtensions,
    IDot11ExtUIProperty **ppDot11ExtUIProperty
    )
{
    HRESULT hr = ERROR_SUCCESS;
    BSTR strName = NULL;
    ULONG uCount = 0;
    DWORD i = 0;
    WCHAR wbuf[128];
    IDot11SampleExtUIKeyProperty **pprgProps = NULL;

    uCount = g_IHVOneXExtCapability.dwAuthCount;

    pprgProps = (IDot11SampleExtUIKeyProperty**) 
                    CoTaskMemAlloc(sizeof(IDot11SampleExtUIKeyProperty*) * uCount);

    if (!pprgProps)
    {
        hr = E_UNEXPECTED;
        goto error;
    }

    // Since we just have one property of each, we'll 
    // create one interface first and initialize it separately
    IDot11SampleExtUIKeyProperty *pTempIProp = NULL;

    for (i = 0; i < uCount; ++i)
    {
        ZeroMemory(
            wbuf,
            128
            );
        pTempIProp = NULL;
        SysFreeString(strName);
        strName = NULL;

        hr = CoCreateInstance(
                    GUID_SAMPLE_IHVUI_CLSID,
                    NULL,
                    CLSCTX_INPROC,
                    IID_IDot11SampleExtUIKeyProperty,
                    (PVOID*)&pTempIProp
                    );

        if (FAILED(hr))
        {
            continue;
        }

        StringCchPrintf(
            wbuf,
            128,
            g_IHVAuthFriendlyName[g_IHVOneXExtCapability.IhvAuthCiphers[i].IHVAuth]
            );

        strName = SysAllocString(wbuf);
        hr = pTempIProp->Initialize((BYTE *) &(g_IHVOneXExtCapability.IhvAuthCiphers[i]));
        pprgProps[i] = pTempIProp;
    }
    
    if (SUCCEEDED(hr))
    {
        *pcExtensions = uCount;
        *ppDot11ExtUIProperty = (IDot11ExtUIProperty*)pprgProps; 

        // Set the current pointer to NULL so it doesn't get freed at the bottom
        pprgProps = NULL;
    }
    
error:
    if (FAILED(hr) && pprgProps)
    {
        CoTaskMemFree(pprgProps);
        pprgProps = NULL;
    }
    SysFreeString(strName);
    return hr;
}


HRESULT 
CDot11SampleExtUI::FinalConstruct()
{
    m_pUnkSite = NULL;
    m_hFirstPagePsp = NULL;
    m_hLastPagePsp = NULL;
    m_pUIRequest = NULL;

    return S_OK;
}

VOID 
CDot11SampleExtUI::FinalRelease()
{
    if( m_pUnkSite)
    {
        m_pUnkSite ->Release();
        m_pUnkSite = NULL;
    }

    if( m_pUIRequest)
    {
        delete m_pUIRequest;
        m_pUIRequest = NULL;
    }

}

// IObjectWithSite
STDMETHODIMP 
CDot11SampleExtUI::SetSite (
                             IUnknown* pUnkSite
                             )
{
    if( m_pUnkSite)
        m_pUnkSite ->Release();

    m_pUnkSite = pUnkSite;

    if( m_pUnkSite)
        m_pUnkSite ->AddRef();

    return S_OK;
}

STDMETHODIMP 
CDot11SampleExtUI::GetSite (
                             REFIID riid, 
                             void** ppvSite
                             )
{
    *ppvSite = NULL;

    if( m_pUnkSite == NULL)
        return E_FAIL;

    return m_pUnkSite ->QueryInterface(riid, ppvSite);
}

//IWizardExtension
STDMETHODIMP CDot11SampleExtUI::AddPages (
    HPROPSHEETPAGE* aPages, 
    UINT cPages, 
    UINT *pnPagesAdded
    )
{
    IPropertyBag *pIPropertyBag = NULL;

    UNREFERENCED_PARAMETER(cPages);

    HRESULT hr = m_pUnkSite->QueryInterface(IID_IPropertyBag,
        (VOID **)&pIPropertyBag);
    if (SUCCEEDED(hr))
    {
        VARIANT v;
        VariantInit(&v);

        WCHAR ihvKeyName[IHV_KEY_LENGTH];
        GetClsidPropertyName (
            & GUID_SAMPLE_IHVUI_CLSID, 
            (LPWSTR) c_szIhvUIRequest,
            ihvKeyName,
            IHV_KEY_LENGTH
            );
        hr = pIPropertyBag->Read(
                 ihvKeyName,
                 &v,
                 NULL);

        if (SUCCEEDED(hr) && (VT_BSTR == V_VT(&v)))
        {
            if( m_pUIRequest == NULL)
            {
                m_pUIRequest = new(std::nothrow) IHV_UI_REQUEST;
                if (m_pUIRequest == NULL)
                {
                     VariantClear(&v);
                     pIPropertyBag->Release();
                     return E_OUTOFMEMORY;
                }
            }
            memcpy(m_pUIRequest, v.bstrVal, sizeof(IHV_UI_REQUEST));
        }

        VariantClear(&v);

        pIPropertyBag->Release();
    }

    ////////////////

    *pnPagesAdded = 0;

    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof( psp);
    psp.hInstance = g_hInst;
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE;
    psp.lParam = (LPARAM) this;

    psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_SHOWHELP);
    psp.pfnDlgProc = (DLGPROC) CDot11SampleExtUI::HelpDlgProc;
    psp.pszHeaderTitle = MAKEINTRESOURCE( IDS_TITLE_SHOWHELP);
    m_hFirstPagePsp = CreatePropertySheetPage(& psp);

    psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_GETKEY);
    psp.pfnDlgProc = (DLGPROC) CDot11SampleExtUI::GetKeyDlgProc;
    psp.pszHeaderTitle = MAKEINTRESOURCE( IDS_TITLE_GETKEY);
    HPROPSHEETPAGE hPsp= CreatePropertySheetPage(& psp);

    psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_LASTPAGE);
    psp.pfnDlgProc = (DLGPROC) CDot11SampleExtUI::LastPageDlgProc;
    psp.pszHeaderTitle = MAKEINTRESOURCE( IDS_TITLE_LASTPAGE);
    m_hLastPagePsp = CreatePropertySheetPage(& psp);

    if( m_hFirstPagePsp
        && hPsp
        && m_hLastPagePsp)
    {
        aPages[0] = m_hFirstPagePsp;
        aPages[1] = hPsp;
        aPages[2] = m_hLastPagePsp;

        *pnPagesAdded = 3;

        return S_OK;
    }
    else
    {
        if(m_hFirstPagePsp)
        {
            DestroyPropertySheetPage(m_hFirstPagePsp);
        }

        if(hPsp)
        {
            DestroyPropertySheetPage(hPsp);
        }
        
        if(m_hLastPagePsp)
        {
            DestroyPropertySheetPage(m_hLastPagePsp);
        }

        m_hFirstPagePsp = hPsp = m_hLastPagePsp = NULL;

        return E_FAIL;       
    }
}

STDMETHODIMP CDot11SampleExtUI::GetFirstPage (
    HPROPSHEETPAGE *phpage
    )
{
    *phpage = m_hFirstPagePsp;
    return S_OK;
}

STDMETHODIMP 
CDot11SampleExtUI::GetLastPage (HPROPSHEETPAGE *phpage)
{
    * phpage = m_hLastPagePsp;
    return S_OK;
}


BOOL CALLBACK 
CDot11SampleExtUI::HelpDlgProc (
    HWND hwndDlg,  
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam 
    )
{
    CDot11SampleExtUI* pthis = NULL; 

    switch (uMsg)
    {

    case WM_INITDIALOG:
        {
            pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam); 
            if(pthis && pthis->m_pUIRequest)
            {
                //
                // Convert the ANSI string into a WCHAR string and display it
                //
                int iBufferSize = MultiByteToWideChar(CP_ACP, 0, pthis->m_pUIRequest->title, -1, NULL, 0);
                if (iBufferSize > 0)
                {
                    WCHAR *pwszBuffer = new WCHAR[iBufferSize];
                    
                    if (NULL != pwszBuffer)
                    {
                        pwszBuffer[0] = 0;

                        (VOID)MultiByteToWideChar(CP_ACP, 0, pthis->m_pUIRequest->title, -1, pwszBuffer, iBufferSize);

                        SetDlgItemText(hwndDlg, IDC_EDIT_HELPER, pwszBuffer);

                        delete[] pwszBuffer;
                    }
                }
            }
        }
        return TRUE;

    case WM_DESTROY: 
        {
            // Don't release our properties here, wait
            // rather for Abort or Commit event notifications.
            // Then we will have the same values when resurrected.
        }
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {

            case PSN_SETACTIVE  :
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                return TRUE;

            case PSN_QUERYCANCEL:
                {
                    IWizardSite *pIWizardSite = NULL;
                    HRESULT hr = S_OK;

                    pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam); 
                    if(pthis != NULL)
                    {
                        hr = pthis->m_pUnkSite->QueryInterface(IID_IWizardSite, (VOID **)&pIWizardSite);
                        if (SUCCEEDED(hr))
                        {
                            HPROPSHEETPAGE hpage = NULL;

                            hr = pIWizardSite->GetCancelledPage(&hpage);
                            if (SUCCEEDED(hr))
                            {
                                PropSheet_SetCurSel(GetParent(hwndDlg), hpage, 0);
                            }
                            pIWizardSite->Release();
                        }
                    }
                }
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;

}

BOOL CALLBACK 
CDot11SampleExtUI::GetKeyDlgProc (
    HWND hwndDlg,  
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

{
    CDot11SampleExtUI* pthis = NULL; 

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            (VOID)GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam); 
        }
        return TRUE;

    case WM_DESTROY: 
        {
            // Don't release our properties here, wait
            // rather for Abort or Commit event notifications.
            // Then we will have the same values when resurrected.
        }
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE  :

                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;

            case PSN_WIZNEXT :
                {
                    WCHAR szBuffer[50 + 1] = {0};
                    HRESULT hr = S_OK;
                    IPropertyBag *pIPropertyBag = NULL;

                    GetDlgItemText(hwndDlg, IDC_EDIT_KEY, szBuffer, 50);
                    pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam);

                    if(pthis)
                    {
                        hr = pthis->m_pUnkSite->QueryInterface(IID_IPropertyBag, (VOID **)&pIPropertyBag);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT v;
                            VariantInit(&v);

                            WCHAR ihvKeyName[IHV_KEY_LENGTH] = {0};
                            pthis->GetClsidPropertyName(
                                &GUID_SAMPLE_IHVUI_CLSID, 
                                (LPWSTR) c_szIhvUIResponse,
                                ihvKeyName,
                                IHV_KEY_LENGTH
                                );              

                            // Make sure we remove the previous property if any
                            hr = pIPropertyBag->Read(ihvKeyName, &v, NULL);

                            VariantClear(&v);
                            V_VT(&v) = VT_BSTR;
                            v.bstrVal = SysAllocStringByteLen((LPCSTR)szBuffer, IHV_KEY_LENGTH);

                            // Write the updated property if any
                            hr = pIPropertyBag->Write(ihvKeyName, &v);
                            pIPropertyBag->Release();
                            
                            //
                            // hr is not used below
                            //
                            hr;
                        }
                    }
                }
                return TRUE;

            case PSN_QUERYCANCEL:
                {
                    IWizardSite *pIWizardSite = NULL;
                    HRESULT hr = S_OK;

                    pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam);

                    if(pthis)
                    {
                        hr = pthis->m_pUnkSite->QueryInterface(IID_IWizardSite,(VOID **)&pIWizardSite);
                        if (SUCCEEDED(hr))
                        {
                            HPROPSHEETPAGE hpage = NULL;
                            hr = pIWizardSite->GetCancelledPage(&hpage);
                            if (SUCCEEDED(hr))
                            {
                                PropSheet_SetCurSel(GetParent(hwndDlg), hpage, 0);
                            }
                            pIWizardSite->Release();
                        }
                    }
                }
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;

}

BOOL CALLBACK 
CDot11SampleExtUI::LastPageDlgProc(
    HWND hwndDlg,  
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam 
    )
{
    CDot11SampleExtUI* pthis = NULL; 

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            (VOID)GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam); 
        }
        return TRUE;

    case WM_DESTROY: 
        {
            (VOID)GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam);
        }
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE  :
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;

            case PSN_WIZNEXT :
                {
                    IWizardSite *pIWizardSite = NULL;
                    HRESULT hr = S_OK;

                    pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam);
                    if(pthis)
                    {
                        hr = pthis->m_pUnkSite->QueryInterface(IID_IWizardSite,(VOID **)&pIWizardSite);
                        if (SUCCEEDED(hr))
                        {
                            HPROPSHEETPAGE hpage = NULL;

                            hr = pIWizardSite->GetNextPage(&hpage);
                            if (SUCCEEDED(hr))
                            {
                                PropSheet_SetCurSel(GetParent(hwndDlg), hpage, 0);
                            }
                            pIWizardSite->Release();
                        }
                    }
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)-1);
                }
                return TRUE;

            case PSN_QUERYCANCEL:
                {
                    IWizardSite *pIWizardSite = NULL;
                    HRESULT hr = S_OK;

                    pthis = GetThis<CDot11SampleExtUI>(hwndDlg, uMsg, wParam, lParam);
                    if(pthis)
                    {
                        hr = pthis->m_pUnkSite->QueryInterface(IID_IWizardSite,(VOID **)&pIWizardSite);
                        if (SUCCEEDED(hr))
                        {
                            HPROPSHEETPAGE hpage = NULL;
                            hr = pIWizardSite->GetCancelledPage(&hpage);
                            if (SUCCEEDED(hr))
                            {
                                PropSheet_SetCurSel(GetParent(hwndDlg), hpage, 0);
                            }
                            pIWizardSite->Release();
                        }
                    }
                }
                return TRUE;

            }
        }
        return FALSE;
    }
    return FALSE;

}

HRESULT CDot11SampleExtUI::GetClsidPropertyName (
                      _In_ const CLSID* pCLSID, 
                      _In_opt_ PCWSTR pwszPropertyName,
                      _Out_writes_(maxResultLen) PWSTR pwszResultStr,
                      _In_ UINT  maxResultLen)
{
    #define MIN_BUFFER_SIZE 50

    wchar_t *pwszCLSID = NULL;
    wchar_t *pwszKeyName = NULL;
    HRESULT hRetCode = S_OK;
    size_t  iCharCount = 0;
    
    // Sanity
    //=======
    
    if( pCLSID == NULL ||
        pwszResultStr == NULL ||
        maxResultLen < MIN_BUFFER_SIZE
        )
    {
        return E_INVALIDARG;
    }
    
    // Convert CLSID to string
    //========================
    
    hRetCode = StringFromCLSID(*pCLSID, &pwszCLSID);
    if(FAILED(hRetCode))
    {
        goto Done;
    }
    
    // Allocate buffer for entire CLSID\PropertyName string
    //=====================================================
    
    iCharCount = wcslen(pwszCLSID) + 1;
    if(pwszPropertyName)
    {
        iCharCount += wcslen(pwszPropertyName);
    }
    
    pwszKeyName = new(std::nothrow) wchar_t[iCharCount];
    if(pwszKeyName == NULL)
    {
        hRetCode = E_OUTOFMEMORY;
        goto Done;
    }
    
    swprintf_s(pwszKeyName, iCharCount, L"%s%s", pwszCLSID, pwszPropertyName ? pwszPropertyName : L"");
    
    // Copy as much as we can to the target buffer
    //============================================
    
    wcsncpy_s(pwszResultStr, maxResultLen, pwszKeyName, _TRUNCATE);
    
Done:

    if(pwszKeyName != NULL)
    {
        delete [] pwszKeyName;
    }
    
    if(pwszCLSID != NULL)
    {
        CoTaskMemFree(pwszCLSID);
    }

    return hRetCode;
}    



