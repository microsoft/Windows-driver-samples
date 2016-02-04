//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

extern HINSTANCE g_hInst;

// used by the con prop extensions
CIhvSecurityProfile* pIhvKeyProfile;


CDot11SampleExtUIKeyProperty::CDot11SampleExtUIKeyProperty(): 
    m_crefCount(0), m_fInitialized(false), 
    m_ExtType(DOT11_EXT_UI_KEYEXTENSION), m_fModified(FALSE)
{
    InterlockedIncrement(&g_objRefCount);
    memset(
        &m_IHVAuthCiphers, 
        0,
        sizeof(IHV_AUTH_CIPHERS)
        );
    m_bstrFN = NULL;
}

CDot11SampleExtUIKeyProperty::~CDot11SampleExtUIKeyProperty()
{
    if(m_bstrFN != NULL) {
        SysFreeString(m_bstrFN);
    }
    
    InterlockedDecrement(&g_objRefCount);        
}


STDMETHODIMP 
CDot11SampleExtUIKeyProperty::GetDot11ExtUIPropertyFriendlyName(BSTR* bstrPropertyName)
{
    HRESULT hr = E_INVALIDARG; 
    if (false == m_fInitialized)
    {
        return hr;
    }

    if (NULL != g_IHVAuthFriendlyName[m_IHVAuthCiphers.IHVAuth])
    {
        *bstrPropertyName = SysAllocString(g_IHVAuthFriendlyName[m_IHVAuthCiphers.IHVAuth]);
        hr = S_OK;
    }

    return hr;
}

//Used to extend property
STDMETHODIMP 
CDot11SampleExtUIKeyProperty::DisplayDot11ExtUIProperty(
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
    pIhvKeyProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvKeyProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pIhvKeyProfile->LoadXml(bstrIHVProfile);
    m_fModified = FALSE;

    // Dialog will store the string in a member variable
    DialogBoxParam(
        g_hInst, 
        MAKEINTRESOURCE(IDD_PROPPAGE_SMALL), 
        hParent, 
        SimpleDialogProcKey,
        (LPARAM)pIhvKeyProfile
        );

    m_fModified = pIhvKeyProfile->GetModified();

    if (NULL != bstrModifiedIHVProfile)
    {
        if (m_fModified)
        {
            pIhvKeyProfile->EmitXml(bstrModifiedIHVProfile); 
        }
    }

    if (NULL != pbIsModified)
    {
        *pbIsModified = m_fModified;
    }

error:
    if(pIhvKeyProfile)
    {
        delete pIhvKeyProfile;
        pIhvKeyProfile = NULL;
    }

    return hr;
}


//Used to get the currently chosen entry to display as selected in the dropdown list
STDMETHODIMP 
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertyGetSelected(
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    BOOL* pfIsSelected // flag denoting if this is the selected profile
    )
{
    HRESULT hr = S_OK;
    IHV_AUTH_TYPE currentAuthType = IHVAuthInvalid;
    UNREFERENCED_PARAMETER(pIHVParams);

    if(pfIsSelected == NULL)
    {
        hr = E_INVALIDARG;
        goto error;
    }

    *pfIsSelected = FALSE;

    pIhvKeyProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvKeyProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    
    pIhvKeyProfile->LoadXml(bstrIHVProfile);

    hr = pIhvKeyProfile->GetAuthType(&currentAuthType);
    if(FAILED(hr))
    {
        // if it fails then choose a default selected auth
        hr = S_OK;
        if(IHVAuthV1 == m_IHVAuthCiphers.IHVAuth)
        {
            *pfIsSelected = TRUE;
        }
    }
    else if(currentAuthType == m_IHVAuthCiphers.IHVAuth)
    {
        *pfIsSelected = TRUE;
    }
    
error:
    if(pIhvKeyProfile != NULL)
    {
        delete pIhvKeyProfile;
        pIhvKeyProfile = NULL;
    }    
    return hr;
}

//Used to set the current entry as chosen from the dropdown list
STDMETHODIMP 
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertySetSelected(
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

    pIhvKeyProfile = new(std::nothrow) CIhvSecurityProfile();
    if (pIhvKeyProfile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
        
    pIhvKeyProfile->LoadXml(bstrIHVProfile);

    pIhvKeyProfile->SetAuthType(m_IHVAuthCiphers.IHVAuth);
    pIhvKeyProfile->SetFullSecurityFlag(FALSE);

    pIhvKeyProfile->EmitXml(bstrModifiedIHVProfile);
    *pbIsModified = pIhvKeyProfile->GetModified();
    
error:
    if(pIhvKeyProfile != NULL)
    {
        delete pIhvKeyProfile;
        pIhvKeyProfile = NULL;
    }
    return hr;    
}


STDMETHODIMP 
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertyHasConfigurationUI(
    BOOL *fHasConfigurationUI)
{
    // this page always wants to show a config UI unless its of IHVAuthOpen1X auth
    *fHasConfigurationUI = (m_IHVAuthCiphers.IHVAuth == IHVAuthV3)?FALSE:TRUE;
    return S_OK;
}

//Used to get additional display data (ciphers for auth types)
STDMETHODIMP 
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertyGetDisplayInfo(
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
    DWORD dwDefaultSelection = 0;
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

    for(i = 0; i < m_IHVAuthCiphers.dwCipherCount; ++i)
    {
        ciphersInfoArray[m_IHVAuthCiphers.IHVAuth][i].dwDataKey = m_IHVAuthCiphers.IHVCiphers[i];
        ciphersInfoArray[m_IHVAuthCiphers.IHVAuth][i].dot11ExtUIDisplayInfoType = DOT11_EXT_UI_DISPLAY_INFO_CIPHER;
        ciphersInfoArray[m_IHVAuthCiphers.IHVAuth][i].bstrDisplayText = SysAllocString(g_IHVCipherFriendlyName[m_IHVAuthCiphers.IHVCiphers[i]]);

        if(m_IHVAuthCiphers.IHVCiphers[i] == cipherType)
        {
            dwDefaultSelection = i;
        }
    }
    
    // for the given auth type we want to return the list of compatible ciphers
    *ppDot11ExtUIProperty = ciphersInfoArray[m_IHVAuthCiphers.IHVAuth];
    *pcEntries = m_IHVAuthCiphers.dwCipherCount;
    *puDefaultSelection = dwDefaultSelection;

error:
    return hr;
}


STDMETHODIMP 
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertySetDisplayInfo(
    DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be modified
    BSTR bstrIHVProfile, // IHV data from the profile 
    PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
    DOT11_EXT_UI_PROPERTY_DISPLAY_INFO *pDot11ExtUIProperty, // selected info structure
    BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
    BOOL* pbIsModified // flag to denote if profile was modified
    )
{
    HRESULT hr = S_OK;

    CIhvSecurityProfile IhvSecurityProfile;

    UNREFERENCED_PARAMETER(pIHVParams);

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
CDot11SampleExtUIKeyProperty::Dot11ExtUIPropertyIsStandardSecurity(
    BOOL *fIsStandardSecurity, // if this interface is a standard auth method
    DOT11_EXT_UI_SECURITY_TYPE *dot11ExtUISecurityType  // which of the standard auth methods it is
    )
{
    *fIsStandardSecurity = FALSE;

    if (m_IHVAuthCiphers.IHVAuth == IHVAuthV1)
    {
        *fIsStandardSecurity = TRUE;
        *dot11ExtUISecurityType = DOT11_EXT_UI_SECURITY_8021X;
    }

    return S_OK;
}


STDMETHODIMP
CDot11SampleExtUIKeyProperty::Initialize(BYTE* pbData)
{
    HRESULT hr = E_INVALIDARG; 
    PIHV_AUTH_CIPHERS pIhvAuthCiphers = NULL;
    pIhvAuthCiphers = (PIHV_AUTH_CIPHERS) pbData;
    
    if (false == m_fInitialized)
    {
        // Set the FriendlyName
        m_fInitialized = true;
        memcpy(
            &m_IHVAuthCiphers,
            pIhvAuthCiphers,
            sizeof(IHV_AUTH_CIPHERS)
            );
        hr = S_OK;
    }
    return hr;
}

INT_PTR CALLBACK 
SimpleDialogProcKey(
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

    if(!pIhvKeyProfile)
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
                IDS_IHV_DEFAULT_KEY_TITLE,
                strDialogTitle,
                MAX_PATH
                );
                    
            SetWindowText(hwndDlg, strDialogTitle);
        }
        
        // check the checkbox if needed
        if(FAILED(pIhvKeyProfile->GetParamDWORD(&dwValue)))
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
        if(FAILED(pIhvKeyProfile->GetParamBSTR(&bstrText)))
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

                    pIhvKeyProfile->SetParamDWORD(dwNewValue);                    
                    pIhvKeyProfile->SetParamBSTR(szBuf);
                    pIhvKeyProfile->SetFullSecurityFlag(FALSE);

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


