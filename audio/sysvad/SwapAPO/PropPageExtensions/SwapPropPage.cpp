//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    SwapPropPage.cpp
//
// Abstract:    Implementation of the CSwapPropPage class
//
// ----------------------------------------------------------------------------


#include "stdafx.h"
#include <initguid.h>   // DEFINE_GUID
#include <mmdeviceapi.h>
#include <audioenginebaseapo.h>
#include <audioendpoints.h>
#include "SwapPropPage.h"
#include <functiondiscoverykeys.h>
#include <CustomPropKeys.h>
#include "cplext_i.c"

_Analysis_mode_(_Analysis_code_type_user_driver_)

// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::CSwapPropPage
//
// Description:
//      CSwapPropPage constructor
// ----------------------------------------------------------------------------
CSwapPropPage::CSwapPropPage()
:   m_pAudioFXExtParams(NULL)
,   m_fDisableSysFX(FALSE)
,   m_fEnableSwapSFX(FALSE)
,   m_fEnableSwapMFX(FALSE)
,   m_fEnableDelaySFX(FALSE)
,   m_fEnableDelayMFX(FALSE)
,   m_fReset(FALSE)
{
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::~CSwapPropPage
//
// Description:
//      CSwapPropPage destructor
// ----------------------------------------------------------------------------
CSwapPropPage::~CSwapPropPage()
{
    SAFE_RELEASE(m_pAudioFXExtParams->pFxProperties);
    SAFE_DELETE(m_pAudioFXExtParams);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::GetDeviceFriendlyName
//
// Description:
//      Retrieves the endpoint's friendly name
//
// Parameters:
//      ppNameOut - [out] The friendly name of the endpoint
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::GetDeviceFriendlyName
(
    _Outptr_result_maybenull_ LPWSTR* ppNameOut
)
{
    CComPtr<IMMDeviceEnumerator>    spEnumerator;
    CComPtr<IPropertyStore>         spProperties;
    CComPtr<IMMDevice>              spMMDevice;
    HRESULT                         hr = S_OK;
    PROPVARIANT                     var;

    IF_TRUE_ACTION_JUMP((ppNameOut == NULL), hr = E_POINTER, Exit);

    *ppNameOut = NULL;

    // Create device enumerator and get IMMDevice from the device ID
    hr = spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
    IF_FAILED_JUMP(hr, Exit);

    hr = spEnumerator->GetDevice(m_pAudioFXExtParams->pwstrEndpointID, &spMMDevice);
    IF_FAILED_JUMP(hr, Exit);

    // Open the PropertyStore for read access
    hr = spMMDevice->OpenPropertyStore(STGM_READ, &spProperties);
    IF_FAILED_JUMP(hr, Exit);

    PropVariantInit(&var);

    // Retrieve the friendly name of the endpoint
    hr = spProperties->GetValue(PKEY_Device_FriendlyName, &var);
    if (SUCCEEDED(hr) && (var.vt == VT_LPWSTR))
    {
        *ppNameOut = var.pwszVal;
    }
    else
    {
        PropVariantClear(&var);
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::RetrieveSysFXState
//
// Description:
//      Get the current state (enabled or disabled) of system effects
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::RetrieveSysFXState(BOOL *pfDisabled)
{
    HRESULT hr = E_POINTER;

    if ((m_pAudioFXExtParams != NULL) && (m_pAudioFXExtParams->pFxProperties != NULL))
    {
        PROPVARIANT var;
        PropVariantInit(&var);

        // Get the state of whether system effects are enabled or not
        hr = m_pAudioFXExtParams->pFxProperties->GetValue(PKEY_AudioEndpoint_Disable_SysFx, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_UI4))
        {
            *pfDisabled = (var.ulVal != 0L);
        }

        PropVariantClear(&var);
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::SetSysFXState
//
// Description:
//      Enable or disable system effects
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::SetSysFXState()
{
    BOOL fCurrentState = 0;
    HRESULT hr = RetrieveSysFXState(&fCurrentState);
    IF_FAILED_JUMP(hr, Exit);

    if (fCurrentState != m_fDisableSysFX)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        var.ulVal = (m_fDisableSysFX ? 1L : 0L);

        // Enable or disable SysFX
        hr = m_pAudioFXExtParams->pFxProperties->SetValue(PKEY_AudioEndpoint_Disable_SysFx, var);
    }
    
Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::RetrieveSwapSFXState
//
// Description:
//      Get the current state (enabled or disabled) of channel swap SFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::RetrieveSwapSFXState(BOOL *pfEnabled)
{
    HRESULT hr = E_POINTER;

    if ((m_pAudioFXExtParams != NULL) && (m_pAudioFXExtParams->pFxProperties != NULL))
    {
        PROPVARIANT var;
        PropVariantInit(&var);

        // Get the state of whether channel swap SFX is enabled or not
        hr = m_pAudioFXExtParams->pFxProperties->GetValue(PKEY_Endpoint_Enable_Channel_Swap_SFX, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_UI4))
        {
            *pfEnabled = (var.ulVal != 0L);
        }

        PropVariantClear(&var);
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::SetSwapSFXState
//
// Description:
//      Enable or disable channel swap SFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::SetSwapSFXState()
{
    BOOL fCurrentState = 0;
    HRESULT hr = RetrieveSwapSFXState(&fCurrentState);
    IF_FAILED_JUMP(hr, Exit);

    if (fCurrentState != m_fEnableSwapSFX)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        var.ulVal = (m_fEnableSwapSFX ? 1L : 0L);

        // Enable or disable channel swap SFX
        hr = m_pAudioFXExtParams->pFxProperties->SetValue(PKEY_Endpoint_Enable_Channel_Swap_SFX, var);
        
        // Enabling or disabling the swap effect can be done with the engine running
        // It does not invalidate anything which should be locked by IAudioProcessingObjectConfiguration::LockForProcess
        // In particular, it does not impact IAudioProcessingObject::GetLatency
        // So we do not need to call IAudioEndpointFormatControl::ResetToDefault here
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::RetrieveSwapMFXState
//
// Description:
//      Get the current state (enabled or disabled) of channel swap MFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::RetrieveSwapMFXState(BOOL *pfEnabled)
{
    HRESULT hr = E_POINTER;

    if ((m_pAudioFXExtParams != NULL) && (m_pAudioFXExtParams->pFxProperties != NULL))
    {
        PROPVARIANT var;
        PropVariantInit(&var);

        // Get the state of whether channel swap MFX is enabled or not
        hr = m_pAudioFXExtParams->pFxProperties->GetValue(PKEY_Endpoint_Enable_Channel_Swap_MFX, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_UI4))
        {
            *pfEnabled = (var.ulVal != 0L);
        }

        PropVariantClear(&var);
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::SetSwapMFXState
//
// Description:
//      Enable or disable channel swap MFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::SetSwapMFXState()
{
    BOOL fCurrentState = 0;
    HRESULT hr = RetrieveSwapMFXState(&fCurrentState);
    IF_FAILED_JUMP(hr, Exit);

    if (fCurrentState != m_fEnableSwapMFX)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        var.ulVal = (m_fEnableSwapMFX ? 1L : 0L);

        // Enable or disable channel swap MFX
        hr = m_pAudioFXExtParams->pFxProperties->SetValue(PKEY_Endpoint_Enable_Channel_Swap_MFX, var);

        // Enabling or disabling the swap effect can be done with the engine running
        // It does not invalidate anything which should be locked by IAudioProcessingObjectConfiguration::LockForProcess
        // In particular, it does not impact IAudioProcessingObject::GetLatency
        // So we do not need to call IAudioEndpointFormatControl::ResetToDefault here
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::RetrieveDelaySFXState
//
// Description:
//      Get the current state (enabled or disabled) of delay SFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::RetrieveDelaySFXState(BOOL *pfEnabled)
{
    HRESULT hr = E_POINTER;

    if ((m_pAudioFXExtParams != NULL) && (m_pAudioFXExtParams->pFxProperties != NULL))
    {
        PROPVARIANT var;
        PropVariantInit(&var);

        // Get the state of whether channel swap SFX is enabled or not
        hr = m_pAudioFXExtParams->pFxProperties->GetValue(PKEY_Endpoint_Enable_Delay_SFX, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_UI4))
        {
            *pfEnabled = (var.ulVal != 0L);
        }

        PropVariantClear(&var);
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::SetSwapSFXState
//
// Description:
//      Enable or disable channel swap SFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::SetDelaySFXState()
{
    BOOL fCurrentState = 0;
    HRESULT hr = RetrieveDelaySFXState(&fCurrentState);
    IF_FAILED_JUMP(hr, Exit);

    if (fCurrentState != m_fEnableDelaySFX)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        var.ulVal = (m_fEnableDelaySFX ? 1L : 0L);

        // Enable or disable delay SFX
        hr = m_pAudioFXExtParams->pFxProperties->SetValue(PKEY_Endpoint_Enable_Delay_SFX, var);
        IF_FAILED_JUMP(hr, Exit);

        // Enabling or disabling delay changes the value that should be returned by IAudioProcessingObject::GetLatency
        // This is one of the things that is locked by IAudioProcessingObjectConfiguration::LockForProcess
        // So we need to call IAudioEndpointFormatControl::ResetToDefault
        // This will prompt AudioDG to tear down the graph and build it up again
        m_fReset = TRUE;
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::RetrieveDelayMFXState
//
// Description:
//      Get the current state (enabled or disabled) of delay MFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::RetrieveDelayMFXState(BOOL *pfEnabled)
{
    HRESULT hr = E_POINTER;

    if ((m_pAudioFXExtParams != NULL) && (m_pAudioFXExtParams->pFxProperties != NULL))
    {
        PROPVARIANT var;
        PropVariantInit(&var);

        // Get the state of whether channel swap MFX is enabled or not
        hr = m_pAudioFXExtParams->pFxProperties->GetValue(PKEY_Endpoint_Enable_Delay_MFX, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_UI4))
        {
            *pfEnabled = (var.ulVal != 0L);
        }

        PropVariantClear(&var);
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::SetDelayMFXState
//
// Description:
//      Enable or disable delay MFX
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CSwapPropPage::SetDelayMFXState()
{
    BOOL fCurrentState = 0;
    HRESULT hr = RetrieveDelayMFXState(&fCurrentState);
    IF_FAILED_JUMP(hr, Exit);

    if (fCurrentState != m_fEnableDelayMFX)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        var.ulVal = (m_fEnableDelayMFX ? 1L : 0L);

        // Enable or disable delay MFX
        hr = m_pAudioFXExtParams->pFxProperties->SetValue(PKEY_Endpoint_Enable_Delay_MFX, var);
        IF_FAILED_JUMP(hr, Exit);
        
        // Enabling or disabling delay changes the value that should be returned by IAudioProcessingObject::GetLatency
        // This is one of the things that is locked by IAudioProcessingObjectConfiguration::LockForProcess
        // So we need to call IAudioEndpointFormatControl::ResetToDefault
        // This will prompt AudioDG to tear down the graph and build it up again
        m_fReset = TRUE;
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnInitDialog
//
// Description:
//      Dialog initialization routine
//
// Parameters:
//      hwndDlg - [in] Handle to dialog box
//      wParam - [in] Handle to control to receive the default keyboard focus
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE to direct the system to set the keyboard focus to the control
//      specified by wParam. Otherwise, it should return FALSE to prevent the
//      system from setting the default keyboard focus.
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnInitDialog
(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam
)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    HRESULT hr = S_OK;
    LPWSTR pwstrEndpointName = NULL;

    // Retrieve the endpoint's friendly name, system effects, and swap SFX and MFX states
    hr = GetDeviceFriendlyName(&pwstrEndpointName);
    IF_FAILED_JUMP(hr, Exit);

    hr = RetrieveSysFXState(&m_fDisableSysFX);
    IF_FAILED_JUMP(hr, Exit);

    hr = RetrieveSwapSFXState(&m_fEnableSwapSFX);
    IF_FAILED_JUMP(hr, Exit);

    hr = RetrieveSwapMFXState(&m_fEnableSwapMFX);
    IF_FAILED_JUMP(hr, Exit);

    hr = RetrieveDelaySFXState(&m_fEnableDelaySFX);
    IF_FAILED_JUMP(hr, Exit);

    hr = RetrieveDelayMFXState(&m_fEnableDelayMFX);
    IF_FAILED_JUMP(hr, Exit);

    // Update the property page with retrieved information
    SetWindowText(GetDlgItem(hwndDlg, IDC_SPP_ENDPOINT_NAME), pwstrEndpointName);

    // Based on the retrieved states, toggle the checkboxes to reflect them
    if (m_fDisableSysFX)
    {
        CheckDlgButton(hwndDlg, IDC_DISABLE_SYSFX, BST_CHECKED);

        // Disable APO toggling controls on the page
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_SFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_MFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_SFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_MFX), FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_DISABLE_SYSFX, BST_UNCHECKED);

        // Enable APO toggling controls on the page
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_SFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_MFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_SFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_MFX), TRUE);
    }

    if (m_fEnableSwapSFX)
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_SWAP_SFX, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_SWAP_SFX, BST_UNCHECKED);
    }

    if (m_fEnableSwapMFX)
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_SWAP_MFX, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_SWAP_MFX, BST_UNCHECKED);
    }

    if (m_fEnableDelaySFX)
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_DELAY_SFX, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_DELAY_SFX, BST_UNCHECKED);
    }

    if (m_fEnableDelayMFX)
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_DELAY_MFX, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_ENABLE_DELAY_MFX, BST_UNCHECKED);
    }

Exit:
    SAFE_COTASKMEMFREE(pwstrEndpointName);
    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnApply
//
// Description:
//      Handle the pressing of the apply button
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      TRUE to set keyboard focus on control
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnApply
(
    HWND hwndDlg
)
{
    HRESULT hr = S_OK;

    // Commit the settings
    hr = SetSysFXState();
    IF_FAILED_JUMP(hr, Exit);

    hr = SetSwapSFXState();
    IF_FAILED_JUMP(hr, Exit);

    hr = SetSwapMFXState();
    IF_FAILED_JUMP(hr, Exit);

    hr = SetDelaySFXState();
    IF_FAILED_JUMP(hr, Exit);

    hr = SetDelayMFXState();
    IF_FAILED_JUMP(hr, Exit);
    
    if (NULL != m_pAudioFXExtParams && NULL != m_pAudioFXExtParams->pFxProperties)
    {
        hr = m_pAudioFXExtParams->pFxProperties->Commit();
        IF_FAILED_JUMP(hr, Exit);

        if (m_fReset)
        {
            // something changed that forces us to reset the format support
            CComPtr<IMMDeviceEnumerator>    spEnumerator;
            CComPtr<IMMDevice>              spMMDevice;

            // Create device enumerator and get IMMDevice from the device ID
            hr = spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
            IF_FAILED_JUMP(hr, Exit);

            hr = spEnumerator->GetDevice(m_pAudioFXExtParams->pwstrEndpointID, &spMMDevice);
            IF_FAILED_JUMP(hr, Exit);

            CComPtr<IAudioEndpointFormatControl> spEndpointFormat;  
            hr = spMMDevice->Activate(__uuidof(IAudioEndpointFormatControl), CLSCTX_ALL, NULL, (void **)&spEndpointFormat);
            IF_FAILED_JUMP(hr, Exit);

            spEndpointFormat->ResetToDefault(ENDPOINT_FORMAT_RESET_MIX_ONLY);
            m_fReset = FALSE;
        }
    }
    
Exit:
    if (SUCCEEDED(hr))
    {
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
    }
    else
    {
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
    }

    return(TRUE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnCheckBoxClickedDisableSysFX
//
// Description:
//      Handle the clicking of the Disable System Effects check box
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnCheckBoxClickedDisableSysFX
(
    HWND hwndDlg
)
{
    // Check the state of the check box and update associated data member
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_DISABLE_SYSFX))
    {
        m_fDisableSysFX = TRUE;

        // Disable APO toggling controls on the page
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_SFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_MFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_SFX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_MFX), FALSE);
    }
    else
    {
        m_fDisableSysFX = FALSE;

        // Enable APO toggling controls on the page
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_SFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_SWAP_MFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_SFX), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DELAY_MFX), TRUE);
    }

    // If the user changes the check box, enable the Apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnCheckBoxClickedEnableSwapSFX
//
// Description:
//      Handle the clicking of the Enable Channel Swap SFX check box
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnCheckBoxClickedEnableSwapSFX
(
    HWND hwndDlg
)
{
    // Check the state of the check box and update associated data member
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_ENABLE_SWAP_SFX))
    {
        m_fEnableSwapSFX = TRUE;
    }
    else
    {
        m_fEnableSwapSFX = FALSE;
    }

    // If the user changes the check box, enable the Apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnCheckBoxClickedEnableSwapMFX
//
// Description:
//      Handle the clicking of the Enable Channel Swap MFX check box
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnCheckBoxClickedEnableSwapMFX
(
    HWND hwndDlg
)
{
    // Check the state of the check box and update associated data member
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_ENABLE_SWAP_MFX))
    {
        m_fEnableSwapMFX = TRUE;
    }
    else
    {
        m_fEnableSwapMFX = FALSE;
    }

    // If the user changes the check box, enable the Apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnCheckBoxClickedEnableDelaySFX
//
// Description:
//      Handle the clicking of the Enable Delay SFX check box
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnCheckBoxClickedEnableDelaySFX
(
    HWND hwndDlg
)
{
    // Check the state of the check box and update associated data member
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_ENABLE_DELAY_SFX))
    {
        m_fEnableDelaySFX = TRUE;
    }
    else
    {
        m_fEnableDelaySFX = FALSE;
    }

    // If the user changes the check box, enable the Apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::OnCheckBoxClickedEnableDelayMFX
//
// Description:
//      Handle the clicking of the Enable Delay MFX check box
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CSwapPropPage::OnCheckBoxClickedEnableDelayMFX
(
    HWND hwndDlg
)
{
    // Check the state of the check box and update associated data member
    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_ENABLE_DELAY_MFX))
    {
        m_fEnableDelayMFX = TRUE;
    }
    else
    {
        m_fEnableDelayMFX = FALSE;
    }

    // If the user changes the check box, enable the Apply button
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::DialogProcPage1
//
// Description:
//      Callback for property page
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//      uMsg - [in] Specifies the message
//      wParam - [in] Specifies additional message-specific information
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE if it processed the message, FALSE if not
// ----------------------------------------------------------------------------
INT_PTR CALLBACK CSwapPropPage::DialogProcPage1
(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CSwapPropPage* pthis = (CSwapPropPage*)(LONG_PTR)GetWindowLongPtr(
                                hwndDlg, GWLP_USERDATA);
    BOOL fRet = FALSE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Extract the context data from PROPSHEETPAGE::lParam
            PROPSHEETPAGE*  pSheetDesc = (PROPSHEETPAGE*)lParam;

            // Create the property page factory class
#pragma warning(push)
#pragma warning(disable: 28197)
            pthis = new CComObject<CSwapPropPage>();
#pragma warning(pop)
            if (pthis == NULL)
            {
                return(FALSE);
            }

            // Save this object in lParam
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pthis);

            // Keep audio FX extension parameters passed by the control panel
            pthis->m_pAudioFXExtParams = (AudioFXExtensionParams*)pSheetDesc->lParam;

            fRet = pthis->OnInitDialog(hwndDlg, wParam, lParam);
            break;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                    if (pthis)
                    {
                        // Apply button pressed
                        fRet = pthis->OnApply(hwndDlg);
                    }
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                // Handle the clicking of the check boxes
                case IDC_DISABLE_SYSFX:
                    if (pthis)
                    {
                        fRet = pthis->OnCheckBoxClickedDisableSysFX(hwndDlg);
                    }
                    break;

                case IDC_ENABLE_SWAP_SFX:
                    if (pthis)
                    {
                        fRet = pthis->OnCheckBoxClickedEnableSwapSFX(hwndDlg);
                    }
                    break;

                case IDC_ENABLE_SWAP_MFX:
                    if (pthis)
                    {
                        fRet = pthis->OnCheckBoxClickedEnableSwapMFX(hwndDlg);
                    }
                    break;
                    
                case IDC_ENABLE_DELAY_SFX:
                    if (pthis)
                    {
                        fRet = pthis->OnCheckBoxClickedEnableDelaySFX(hwndDlg);
                    }
                    break;

                case IDC_ENABLE_DELAY_MFX:
                    if (pthis)
                    {
                        fRet = pthis->OnCheckBoxClickedEnableDelayMFX(hwndDlg);
                    }
                    break;
            }
            break;
        }

        case WM_DESTROY:
        {
            SAFE_DELETE(pthis);
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
            fRet = TRUE;
            break;
        }
    }

    return(fRet);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::PropSheetPageProc
//
// Description:
//      Callback that gets invoked right after page creation or right before
//      before page destruction
//
// Parameters:
//      hwnd - Reserved; must be NULL
//      uMsg - [in] Action flag. PSPCB_ADDREF, PSPCB_CREATE, or PSPCB_RELEASE
//      ppsp - [in, out] Pointer to a PROPSHEETPAGE structure that defines
//             the page being created or destroyed.
//
// Return values:
//      Depends on the value of the uMsg parameter
// ----------------------------------------------------------------------------
UINT CALLBACK CSwapPropPage::PropSheetPageProc
(
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp
)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(ppsp);

    // if (uMsg == PSPCB_CREATE) ...
    return(1);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::Initialize
//
// Description:
//      Implementation of IShellExtInit::Initialize. Initializes a property
//      sheet extension, shortcut menu extension, or drag-and-drop handler.
//
// Parameters:
//      pidlFolder - [in] Address of an ITEMIDLIST structure that uniquely
//                   identifies a folder. For property sheet extensions,
//                   this parameter is NULL.
//      pdtobj - [out] Address of an IDataObject interface object that can be
//               used to retrieve the objects being acted upon. 
//      hkeyProgID - [in] Registry key for the file object or folder type.
//
// Return values:
//      Returns NOERROR if successful, or an OLE-defined error value otherwise
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CSwapPropPage::Initialize
(
    LPCITEMIDLIST   pidlFolder,
    IDataObject*    pdtobj,
    HKEY            hkeyProgID
)
{
    UNREFERENCED_PARAMETER(pidlFolder);
    UNREFERENCED_PARAMETER(pdtobj);
    UNREFERENCED_PARAMETER(hkeyProgID);

    return(S_OK);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::AddPages
//
// Description:
//      Implementation of IShellPropSheetExt::AddPages. Adds one or more pages
//      to a property sheet that the Shell displays for a file object.
//
// Parameters:
//      lpfnAddPage - [in] Address of a function that the property sheet
//                    handler calls to add a page to the property sheet. The
//                    function takes a property sheet handle returned by the
//                    CreatePropertySheetPage function and the lParam parameter
//                    passed to the AddPages method. 
//      lParam - [in] Parameter to pass to the function specified by the
//               lpfnAddPage method.
//
// Return values:
//      Returns S_OK if successful. If the method fails, an OLE-defined error
//      code is returned
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE CSwapPropPage::AddPages
(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,    // See PrSht.h
    LPARAM                  lParam          // Used by caller, don't modify
)
{
    HRESULT                 hr = S_OK;
    PROPSHEETPAGE           psp;
    HPROPSHEETPAGE          hPage1 = NULL;
    AudioFXExtensionParams* pAudioFXParams = (AudioFXExtensionParams*)lParam;
#pragma warning(push)
#pragma warning(disable: 28197)
    AudioFXExtensionParams* pAudioFXParamsCopy = new AudioFXExtensionParams;
#pragma warning(pop)

    if (pAudioFXParamsCopy == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Make a copy of the params
    CopyMemory(pAudioFXParamsCopy, pAudioFXParams, sizeof(AudioFXExtensionParams));
    SAFE_ADDREF(pAudioFXParamsCopy->pFxProperties);

    // Initialize property page params and create page
    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance     = _AtlBaseModule.GetModuleInstance();
    psp.hIcon         = 0;
    psp.pcRefParent   = (UINT*)&m_dwRef;
    psp.lParam        = (LPARAM)pAudioFXParamsCopy;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_SWAP_PROP_PAGE);
    psp.pfnDlgProc    = (DLGPROC)DialogProcPage1;
    psp.pfnCallback   = PropSheetPageProc;

    // Create the property sheet page and add the page
    hPage1 = CreatePropertySheetPage(&psp);
    if (hPage1)
    {
        if (!lpfnAddPage(hPage1, pAudioFXParams->AddPageParam))
        {
            hr = E_FAIL;
            delete pAudioFXParamsCopy;
            DestroyPropertySheetPage(hPage1);
        }
        else
        {
            // Add ref for page
            this->AddRef();
        }
    }
    else
    {
        delete pAudioFXParamsCopy;
        hr = E_OUTOFMEMORY;
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CSwapPropPage::ReplacePage
//
// Description:
//      Implementation of IShellPropSheetExt::ReplacePage. Replaces a page in
//      a property sheet for a Control Panel object.
//
// Parameters:
//      uPageID - [in] Identifier of the page to replace 
//      lpfnReplacePage - [in] Address of a function that the property sheet
//                        handler calls to replace a page to the property
//                        sheet. The function takes a property sheet handle
//                        returned by the CreatePropertySheetPage function and
//                        the lParam parameter passed to the ReplacePage
//                        method.
//      lParam - [in] Parameter to pass to the function specified by the
//               lpfnReplacePage parameter. 
//
// Return values:
//      Returns NOERROR if successful, or an OLE-defined error value otherwise
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE CSwapPropPage::ReplacePage
(
    UINT                    uPageID,
    LPFNSVADDPROPSHEETPAGE  lpfnReplaceWith,
    LPARAM                  lParam
)
{
    UNREFERENCED_PARAMETER(uPageID);
    UNREFERENCED_PARAMETER(lpfnReplaceWith);
    UNREFERENCED_PARAMETER(lParam);

    return(S_FALSE);
}
