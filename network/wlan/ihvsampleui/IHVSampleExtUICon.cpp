//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

extern HINSTANCE g_hInst;

// used by the con prop extensions
CIhvConnectivityProfile* pIhvConProfile;


CDot11SampleExtUIConProperty::CDot11SampleExtUIConProperty(): 
    m_crefCount(0), m_fInitialized(false), 
    m_ExtType(DOT11_EXT_UI_CONNECTION), m_fModified(FALSE)
{
    m_bstrFN = NULL;
    InterlockedIncrement(&g_objRefCount);
}

CDot11SampleExtUIConProperty::~CDot11SampleExtUIConProperty()
{
    SysFreeString(m_bstrFN);
    InterlockedDecrement(&g_objRefCount);        
}


STDMETHODIMP 
CDot11SampleExtUIConProperty::GetDot11ExtUIPropertyFriendlyName(BSTR* bstrPropertyName)
{
    HRESULT hr = E_INVALIDARG; 
    if (false == m_fInitialized)
    {
        return hr;
    }

    if (NULL != bstrPropertyName)
    {
        *bstrPropertyName = SysAllocString(m_bstrFN);
        hr = S_OK;
    }

    return hr;
}

//Used to extend property
STDMETHODIMP 
CDot11SampleExtUIConProperty::DisplayDot11ExtUIProperty(
    HWND hParent, // Parent Window Handle 
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
    BOOL* pbIsModified // flag to denote if profile was modified
    )
{
    HRESULT hr = S_OK; 
    UNREFERENCED_PARAMETER(pIHVParams);
    
    if (!m_fInitialized)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    // Store the passed-in string in a member variable so dialog box can display it
    pIhvConProfile = new(std::nothrow) CIhvConnectivityProfile();
    if (pIhvConProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pIhvConProfile->LoadXml(bstrIHVProfile);
    m_fModified = FALSE;

    // Dialog will store the string in a member variable
    DialogBoxParam(
        g_hInst, 
        MAKEINTRESOURCE(IDD_PROPPAGE_SMALL), 
        hParent, 
        SimpleDialogProcCon,
        (LPARAM)pIhvConProfile
        );

    m_fModified = pIhvConProfile->GetModified();

    if (NULL != bstrModifiedIHVProfile)
            {
        if (m_fModified)
        {
            pIhvConProfile->EmitXml(bstrModifiedIHVProfile); 
        }
    }

    if (NULL != pbIsModified)
    {
        *pbIsModified = m_fModified;
    }

error:
    if(pIhvConProfile)
    {
        delete pIhvConProfile;
        pIhvConProfile = NULL;
    }

    return hr;
}


//Used to get the currently chosen entry to display as selected in the dropdown list
STDMETHODIMP 
CDot11SampleExtUIConProperty::Dot11ExtUIPropertyGetSelected(
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    BOOL* pfIsSelected // flag denoting if this is the selected profile
    )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(bstrIHVProfile);
    UNREFERENCED_PARAMETER(pIHVParams);

    // since there is only one connection profile always set it to true
    *pfIsSelected = TRUE;
    
    return hr;
}

//Used to set the current entry as chosen from the dropdown list
STDMETHODIMP 
CDot11SampleExtUIConProperty::Dot11ExtUIPropertySetSelected(
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
    BOOL* pbIsModified // flag to denote if profile was modified
    )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pIHVParams);

    if(bstrModifiedIHVProfile == NULL || pbIsModified == NULL)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    // in case the profile is NULL this will supply the default
    pIhvConProfile = new(std::nothrow) CIhvConnectivityProfile();
    if (pIhvConProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pIhvConProfile->LoadXml(bstrIHVProfile);

    pIhvConProfile->EmitXml(bstrModifiedIHVProfile);
    *pbIsModified = pIhvConProfile->GetModified();
    
error:
    if(pIhvConProfile != NULL)
    {
        delete pIhvConProfile;
        pIhvConProfile = NULL;
    }
    return hr;    
}


STDMETHODIMP 
CDot11SampleExtUIConProperty::Dot11ExtUIPropertyHasConfigurationUI(
    BOOL *fHasConfigurationUI
    )
{
    // this page always wants to show a config UI
    *fHasConfigurationUI = TRUE;
    return S_OK;
}

//Used to get additional display data (ciphers for auth types)
STDMETHODIMP 
CDot11SampleExtUIConProperty::Dot11ExtUIPropertyGetDisplayInfo(
	DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be described
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    ULONG *pcEntries, // number of dependent strings
    ULONG *puDefaultSelection, // the entry in the array to be selected by default
  	DOT11_EXT_UI_PROPERTY_DISPLAY_INFO **ppDot11ExtUIProperty // array of returned info structure
    )
{
    UNREFERENCED_PARAMETER(dot11ExtUIDisplayInfoType);
    UNREFERENCED_PARAMETER(pIHVParams);
    UNREFERENCED_PARAMETER(bstrIHVProfile);
    // we have no additional data to display
    *pcEntries = 0;
    *puDefaultSelection = 0;
    *ppDot11ExtUIProperty = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP 
CDot11SampleExtUIConProperty::Dot11ExtUIPropertySetDisplayInfo(
    DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be modified
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    DOT11_EXT_UI_PROPERTY_DISPLAY_INFO *pDot11ExtUIProperty, // selected info structure
    BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
    BOOL* pbIsModified // flag to denote if profile was modified
    )
{
    UNREFERENCED_PARAMETER(dot11ExtUIDisplayInfoType);
    UNREFERENCED_PARAMETER(bstrIHVProfile);
    UNREFERENCED_PARAMETER(pIHVParams);
    UNREFERENCED_PARAMETER(pDot11ExtUIProperty);
    UNREFERENCED_PARAMETER(bstrModifiedIHVProfile);
    UNREFERENCED_PARAMETER(pbIsModified);
    return E_NOTIMPL;
}

STDMETHODIMP
CDot11SampleExtUIConProperty::Dot11ExtUIPropertyIsStandardSecurity(
    BOOL *fIsStandardSecurity, // if this interface is a standard auth method
    DOT11_EXT_UI_SECURITY_TYPE *dot11ExtUISecurityType  // which of the standard auth methods it is
    )
{
    UNREFERENCED_PARAMETER(dot11ExtUISecurityType);
    *fIsStandardSecurity = FALSE;
    return E_NOTIMPL;    
}

STDMETHODIMP
CDot11SampleExtUIConProperty::Initialize(BSTR bstrPropertyName)
{
    HRESULT hr = E_INVALIDARG; 
    if (false == m_fInitialized)
    {
        // Set the FriendlyName
        m_bstrFN = SysAllocString(bstrPropertyName);
        m_fInitialized = true;
        hr = S_OK;
    }
    return hr;
}

INT_PTR CALLBACK 
SimpleDialogProcCon(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL fRetVal = FALSE;
    WCHAR szBuf[256] = {0};
    DWORD dwValue = 0;
    BSTR bstrText = NULL;

    UNREFERENCED_PARAMETER(lParam);

    if(!pIhvConProfile)
    {
        goto error;
    }
    
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            // Dialog title
            WCHAR strDialogTitle[MAX_PATH] = {0};
            (VOID)::LoadString(
                g_hInst,
                IDS_IHV_DEFAULT_CON_TITLE,
                strDialogTitle,
                MAX_PATH
                );
                    
            SetWindowText(hwndDlg, strDialogTitle);
        }
    
        // check the checkbox if needed
        if(FAILED(pIhvConProfile->GetParamDWORD(&dwValue)))
        {
            dwValue = 0;
        }
        ::SendMessage(
            GetDlgItem(hwndDlg, IDC_USE_FASTHANDOFF), 
            BM_SETCHECK, 
            (WPARAM)(int)dwValue, 
            0L
            );
        
        // Set text in the textbox
        if(FAILED(pIhvConProfile->GetParamBSTR(&bstrText)))
        {
            bstrText = NULL;
        }
        SetWindowText(GetDlgItem(hwndDlg, IDC_PARAM_BOX), bstrText);
        
        fRetVal = TRUE;
        break;
        
    case WM_COMMAND: 
        switch (LOWORD(wParam)) 
        { 
            case ID_OK: 
                GetWindowText(GetDlgItem(hwndDlg, IDC_PARAM_BOX), szBuf, 255);
                if(szBuf)
                {
                    DWORD dwNewValue = 0;
                    
                    // get the button state and record it
                    dwNewValue = (int)::SendMessage(
                                            GetDlgItem(hwndDlg, IDC_USE_FASTHANDOFF), 
                                            BM_GETCHECK, 
                                            0L, 
                                            0L
                                            );

                    pIhvConProfile->SetParamDWORD(dwNewValue);                    
                    pIhvConProfile->SetParamBSTR(szBuf);
                    
                    // Notify the owner window to carry out the task. 
                    EndDialog(hwndDlg, 1);
                    fRetVal = TRUE; 
                }
                break;

            case ID_CANCEL: 
                EndDialog(hwndDlg, 0);
                fRetVal = TRUE;
                break;
        }
        break;
    }

error:
    return fRetVal;
}





