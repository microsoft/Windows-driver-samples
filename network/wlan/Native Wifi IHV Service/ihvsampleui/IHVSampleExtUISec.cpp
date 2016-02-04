//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

extern HINSTANCE g_hInst;

// used by the con prop extensions
CIhvSecurityProfile* pIhvSecProfile;

CDot11SampleExtUISecProperty::CDot11SampleExtUISecProperty(): 
    m_crefCount(0), m_fInitialized(false), 
    m_ExtType(DOT11_EXT_UI_SECURITY), m_fModified(FALSE), 
    m_IhvSecurityType(IHVSecurityInvalid)
{
    m_bstrFN = NULL;
    InterlockedIncrement(&g_objRefCount);
}

CDot11SampleExtUISecProperty::~CDot11SampleExtUISecProperty()
{
    SysFreeString(m_bstrFN);
    InterlockedDecrement(&g_objRefCount);        
}


STDMETHODIMP 
CDot11SampleExtUISecProperty::GetDot11ExtUIPropertyFriendlyName(BSTR* bstrPropertyName)
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
CDot11SampleExtUISecProperty::DisplayDot11ExtUIProperty(
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
    pIhvSecProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvSecProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    pIhvSecProfile->LoadXml(bstrIHVProfile);
    m_fModified = FALSE;

    // Dialog will store the string in a member variable
    DialogBoxParam(
        g_hInst, 
        MAKEINTRESOURCE(IDD_PROPPAGE_SMALL), 
        hParent, 
        SimpleDialogProcSec,
        (LPARAM)pIhvSecProfile
        );

    m_fModified = pIhvSecProfile->GetModified();

    if (NULL != bstrModifiedIHVProfile)
    {
        if (m_fModified)
        {
            pIhvSecProfile->EmitXml(bstrModifiedIHVProfile); 
        }
    }

    if (NULL != pbIsModified)
    {
        *pbIsModified = m_fModified;
    }

error:
    if(pIhvSecProfile)
    {
        delete pIhvSecProfile;
        pIhvSecProfile = NULL;
    }

    return hr;
}


//Used to get the currently chosen entry to display as selected in the dropdown list
STDMETHODIMP 
CDot11SampleExtUISecProperty::Dot11ExtUIPropertyGetSelected(
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    BOOL* pfIsSelected // flag denoting if this is the selected profile
    )
{
    HRESULT hr = S_OK;
    IHV_SECURITY_TYPE currentSecurityType = IHVSecurityInvalid;

    UNREFERENCED_PARAMETER(pIHVParams);

    if(pfIsSelected == NULL)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    *pfIsSelected = FALSE;

    pIhvSecProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvSecProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    
    pIhvSecProfile->LoadXml(bstrIHVProfile);

    hr = pIhvSecProfile->GetSecurityType(&currentSecurityType);
    if(FAILED(hr))
    {
        hr = S_OK;
        if(IHVSecurityV1 == m_IhvSecurityType)
        {
            *pfIsSelected = TRUE;
        }
    }
    else if(currentSecurityType == m_IhvSecurityType)
    {
        *pfIsSelected = TRUE;
    }
    
error:
    if(pIhvSecProfile != NULL)
    {
        delete pIhvSecProfile;
        pIhvSecProfile = NULL;
    }    
    return hr;
}

//Used to set the current entry as chosen from the dropdown list
STDMETHODIMP 
CDot11SampleExtUISecProperty::Dot11ExtUIPropertySetSelected(
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

    pIhvSecProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvSecProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    
    pIhvSecProfile->LoadXml(bstrIHVProfile);

    pIhvSecProfile->SetSecurityType(m_IhvSecurityType);
    pIhvSecProfile->SetFullSecurityFlag(TRUE);

    pIhvSecProfile->EmitXml(bstrModifiedIHVProfile );
    *pbIsModified = pIhvSecProfile->GetModified();
    
error:
    if(pIhvSecProfile != NULL)
    {
        delete pIhvSecProfile;
        pIhvSecProfile = NULL;
    }
    return hr;    
}


STDMETHODIMP 
CDot11SampleExtUISecProperty::Dot11ExtUIPropertyHasConfigurationUI(
    BOOL *fHasConfigurationUI)
{
    // this page always wants to show a config UI
    *fHasConfigurationUI = TRUE;
    return S_OK;
}

//Used to get additional display data (ciphers for auth types)
STDMETHODIMP 
CDot11SampleExtUISecProperty::Dot11ExtUIPropertyGetDisplayInfo(
    DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be described
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    ULONG *pcEntries, // number of dependent strings
    ULONG *puDefaultSelection, // the entry in the array to be selected by default
    DOT11_EXT_UI_PROPERTY_DISPLAY_INFO **ppDot11ExtUIProperty // array of returned info structure
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    CIhvSecurityProfile IhvSecurityProfile;
    IHV_CIPHER_TYPE cipherType = IHVCipherInvalid;

    UNREFERENCED_PARAMETER(pIHVParams);

    if(dot11ExtUIDisplayInfoType != DOT11_EXT_UI_DISPLAY_INFO_CIPHER)
    {
        hr = E_NOTIMPL;
        goto error;
    }

    IhvSecurityProfile.LoadXml(bstrIHVProfile);
    hr = IhvSecurityProfile.GetCipherType(&cipherType);
    if(FAILED(hr))
    {
        goto error;
    }

    for(i = 0; i < PROP_COUNT_SEC_CIPHERS; ++i)
    {
        ciphersInfoArray[m_IhvSecurityType][i].dwDataKey = i;
        ciphersInfoArray[m_IhvSecurityType][i].dot11ExtUIDisplayInfoType = DOT11_EXT_UI_DISPLAY_INFO_CIPHER;
        ciphersInfoArray[m_IhvSecurityType][i].bstrDisplayText = SysAllocString(g_IHVCipherFriendlyName[i]);
    }
    
    // for the given auth type we want to return the list of compatible ciphers
    *ppDot11ExtUIProperty = ciphersInfoArray[m_IhvSecurityType];
    *pcEntries = PROP_COUNT_SEC_CIPHERS;
    *puDefaultSelection = cipherType >= IHVCipherInvalid ? 0 : cipherType;

error:
    return hr;
}

STDMETHODIMP 
CDot11SampleExtUISecProperty::Dot11ExtUIPropertySetDisplayInfo(
    DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be modified
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    DOT11_EXT_UI_PROPERTY_DISPLAY_INFO *pDot11ExtUIProperty, // selected info structure
    BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
    BOOL* pbIsModified // flag to denote if profile was modified
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pIHVParams);

    CIhvSecurityProfile IhvSecurityProfile;

    if(dot11ExtUIDisplayInfoType != DOT11_EXT_UI_DISPLAY_INFO_CIPHER)
    {
        hr = E_NOTIMPL;
        goto error;
    }

    if(pDot11ExtUIProperty == NULL)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    hr = IhvSecurityProfile.LoadXml(bstrIHVProfile);
    if(FAILED(hr))
    {
        goto error;
    }
    hr = IhvSecurityProfile.SetCipherType((IHV_CIPHER_TYPE)pDot11ExtUIProperty->dwDataKey);
    if(FAILED(hr))
    {
        goto error;
    }

    hr = IhvSecurityProfile.EmitXml(bstrModifiedIHVProfile);
    *pbIsModified = IhvSecurityProfile.GetModified();
    
error:
    return hr;
}

STDMETHODIMP
CDot11SampleExtUISecProperty::Dot11ExtUIPropertyIsStandardSecurity(
    BOOL *fIsStandardSecurity, // if this interface is a standard auth method
    DOT11_EXT_UI_SECURITY_TYPE *dot11ExtUISecurityType  // which of the standard auth methods it is
    )
{
    UNREFERENCED_PARAMETER(dot11ExtUISecurityType);
    *fIsStandardSecurity = FALSE;
    return S_OK;    
}

STDMETHODIMP
CDot11SampleExtUISecProperty::Initialize(BSTR bstrPropertyName, DWORD dwIhvSecurity)
{
    HRESULT hr = E_INVALIDARG; 
    if (false == m_fInitialized)
    {
        // Set the FriendlyName
        m_bstrFN = SysAllocString(bstrPropertyName);
        m_IhvSecurityType = (IHV_SECURITY_TYPE)dwIhvSecurity;
        m_fInitialized = true;
        hr = S_OK;
    }
    return hr;
}

INT_PTR CALLBACK 
SimpleDialogProcSec(
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

    if(!pIhvSecProfile)
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
                IDS_IHV_DEFAULT_SEC_TITLE,
                strDialogTitle,
                MAX_PATH
                );
                    
            SetWindowText(hwndDlg, strDialogTitle);
        }
    
        // check the checkbox if needed
        if(FAILED(pIhvSecProfile->GetParamDWORD(&dwValue)))
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
        if(FAILED(pIhvSecProfile->GetParamBSTR(&bstrText)))
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

                    pIhvSecProfile->SetParamDWORD(dwNewValue);                    
                    pIhvSecProfile->SetParamBSTR(szBuf);
                    pIhvSecProfile->SetFullSecurityFlag(TRUE);

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


