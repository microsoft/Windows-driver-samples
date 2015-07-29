//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    UIWidgets.cpp
//
// Abstract:    Implementation of CUIWidget and derived classes
//
// ----------------------------------------------------------------------------


#include "stdafx.h"
#include <DeviceTopology.h>
#include "UIWidgets.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)

// ----------------------------------------------------------------------
// CUIWidget base class


// ----------------------------------------------------------------------------
// Function:
//      CUIWidget::CUIWidget
//
// Description:
//      CUIWidget constructor
// ----------------------------------------------------------------------------
CUIWidget::CUIWidget
(
    IPart*  pPart,
    REFIID  iid
)
:   m_hwndParentDlg(NULL),
    m_iid(iid),
    m_wstrLabel(NULL),
    m_bRegisteredForCallbacks(NULL)
{
    // This is done here rather than in the initializer list to avoid a prefix error
    if (pPart != NULL)
    {
        m_spPart = pPart;
    }

    m_dwProcessId = GetCurrentProcessId();
}


// ----------------------------------------------------------------------------
// Function:
//      CUIWidget::~CUIWidget
//
// Description:
//      CUIWidget destructor
// ----------------------------------------------------------------------------
CUIWidget::~CUIWidget()
{
    SAFE_COTASKMEMFREE(m_wstrLabel);

    if (m_bRegisteredForCallbacks)
    {
        m_spPart->UnregisterControlChangeCallback(this);
    }
}


// ----------------------------------------------------------------------------
// Function:
//      CUIWidget::Create
//
// Description:
//      -- Activates the control interface on the specified part
//      -- Registers for control change callbacks
//
// Parameters:
//      hwndDlg - [in] Parent dialog of the widget
//      rc - [in] Not used
//      ctrlIdBase - [in] Not used
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CUIWidget::Create
(
    HWND    hwndDlg,
    SRECT&  /*rc*/,
    UINT&   /*ctrlIdBase*/
)
{
    HRESULT hr;

    m_hwndParentDlg = hwndDlg;

    RPC_STATUS rpcs = UuidCreate(&m_guidEventContext);
    if (rpcs != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcs);
        goto Exit;
    }

    hr = m_spPart->Activate(CLSCTX_INPROC_SERVER, m_iid, (void**)&m_spControl);
    IF_FAILED_JUMP(hr, Exit);

    hr = m_spPart->RegisterControlChangeCallback(m_iid, this);
    IF_FAILED_JUMP(hr, Exit);

    m_bRegisteredForCallbacks = TRUE;

    hr = m_spPart->GetName(&m_wstrLabel);

Exit:
    return hr;
};


// ----------------------------------------------------------------------
// Function:
//      CUIWidget::OnNotify
//
// Description:
//      Implementation of IControlChangeNotify::OnNotify
//      Called when someone calls a "Set" method on a control interface.
//
// Parameters:
//      dwProcessId - [in] Process ID
//      pguidEventContext - Event context guid
//
// Return:
//      S_OK always
// ----------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CUIWidget::OnNotify
(
    DWORD       dwProcessId,
    LPCGUID     pguidEventContext
)
{
    UNREFERENCED_PARAMETER(dwProcessId);

    if ((pguidEventContext == NULL) || 
        (*pguidEventContext != m_guidEventContext))
    {
        // Delegate to derived class
        HandleControlChangeNotification();
    }

    return S_OK;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// CBooleanWidget


// ----------------------------------------------------------------------------
// Function:
//      CBooleanWidget::CBooleanWidget
//
// Description:
//      CBooleanWidget constructor
// ----------------------------------------------------------------------------
CBooleanWidget::CBooleanWidget
(
    IPart*  pPart,
    REFIID  iid
)
:   CUIWidget(pPart, iid),
    m_hwndCheckbox(NULL),
    m_bCurrentValue(FALSE)
{
}


// ----------------------------------------------------------------------------
// Function:
//      CBooleanWidget::Create
//
// Description:
//      Creates the widget which is a dialog
//
// Parameters:
//      hwndDlg - [in] Parent dialog of the widget
//      rc - [in] Position of the widget
//      nCtrlId - [in] Child-window identifier
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CBooleanWidget::Create
(
    HWND    hwndDlg,
    SRECT&  rc,
    UINT&   nCtrlId
)
{
    HRESULT hr;
    HFONT   hFont;
    DWORD   dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX;

    // Activate the control interface and register for callbacks
    hr = CUIWidget::Create(hwndDlg, rc, nCtrlId);
    IF_FAILED_JUMP(hr, Exit);

    // Create a check box
    rc.w = 300;
    rc.h = 20;
    m_hwndCheckbox = CreateWindowEx(
                            0,  //WS_EX_STATICEDGE,
                            WC_BUTTON,
                            m_wstrLabel,
                            dwStyle,
                            rc.x, rc.y,         // put at (x,y)
                            rc.w, rc.h,
                            m_hwndParentDlg,
                            (HMENU)(UINT_PTR)nCtrlId,
                            NULL,
                            0);
    IF_TRUE_ACTION_JUMP((m_hwndCheckbox == NULL), hr = HRESULT_FROM_WIN32(GetLastError()), Exit);

    // Make the new control use the same font as the dialog
    hFont = Window_GetFont(hwndDlg);
    Window_SetFont(m_hwndCheckbox, hFont);

    // Get the current value
    hr = HandleControlChangeNotification();
    IF_FAILED_JUMP(hr, Exit);

Exit:
    return hr;
}


// ----------------------------------------------------------------------
// Function:
//      CBooleanWidget::HandleControlChangeNotification()
//
// Description:
//      Handles widget change notifications
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------
HRESULT CBooleanWidget::HandleControlChangeNotification()
{
    HRESULT hr = S_OK;

    ATLASSERT(m_spControl);
    ATLASSERT(m_hwndCheckbox);

    // Set value on DeviceTopology control interface
    if (m_iid == __uuidof(IAudioLoudness))
    {
        CComQIPtr<IAudioLoudness>   spLoudness = m_spControl;
        hr = spLoudness->GetEnabled(&m_bCurrentValue);
    }
    else if (m_iid == __uuidof(IAudioAutoGainControl))
    {
        CComQIPtr<IAudioAutoGainControl>   spAGC = m_spControl;
        hr = spAGC->GetEnabled(&m_bCurrentValue);
    }
    else if (m_iid == __uuidof(IDeviceSpecificProperty))
    {
        CComQIPtr<IDeviceSpecificProperty>  spBoolCtrl = m_spControl;
        DWORD                               cbValue = sizeof(m_bCurrentValue);

        hr = spBoolCtrl->GetValue(&m_bCurrentValue, &cbValue);
    }

    // Set the check box to reflect the current value
    SendMessage(m_hwndCheckbox, BM_SETCHECK, (WPARAM)(int)(m_bCurrentValue), 0L);

    return hr;
}


// ----------------------------------------------------------------------------
// Function:
//      CBooleanWidget::CommitValue
//
// Description:
//      Commit the value set on the control
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CBooleanWidget::CommitValue()
{
    HRESULT hr = S_OK;

    ATLASSERT(m_spControl);
    ATLASSERT(m_hwndCheckbox);

    // Get current value from check box
    m_bCurrentValue = ((int)(DWORD)SendMessage(m_hwndCheckbox, BM_GETCHECK, 0L, 0L));

    // Set value on DeviceTopology control interface
    if (m_iid == __uuidof(IAudioLoudness))
    {
        CComQIPtr<IAudioLoudness>   spLoudness = m_spControl;
        hr = spLoudness->SetEnabled(m_bCurrentValue, GetEventContext());
    }
    else if (m_iid == __uuidof(IAudioAutoGainControl))
    {
        CComQIPtr<IAudioAutoGainControl>   spAGC = m_spControl;
        hr = spAGC->SetEnabled(m_bCurrentValue, GetEventContext());
    }
    else if (m_iid == __uuidof(IDeviceSpecificProperty))
    {
        CComQIPtr<IDeviceSpecificProperty>   spBoolCtrl = m_spControl;

        hr = spBoolCtrl->SetValue(&m_bCurrentValue, sizeof(m_bCurrentValue),
                                  GetEventContext());
    }

    return hr;
};
