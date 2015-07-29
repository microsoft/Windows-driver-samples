//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    UIWidgets.h
//
// Abstract:    Declaration of CUIWidget and derived classes
//
// ----------------------------------------------------------------------------


#pragma once


// ----------------------------------------------------------------------------
// Class:
//      CUIWidget
//
// Description:
//      Base class for different types of widgets
//
// ----------------------------------------------------------------------------
class CUIWidget : public IControlChangeNotify
{
protected:
    HWND                m_hwndParentDlg;
    IID                 m_iid;
    CComPtr<IPart>      m_spPart;
    CComPtr<IUnknown>   m_spControl;
    DWORD               m_dwProcessId;
    LPWSTR              m_wstrLabel;
    BOOL                m_bRegisteredForCallbacks;
    GUID                m_guidEventContext;

public:
    CUIWidget(IPart* pPart, REFIID iid);
    virtual ~CUIWidget();

    virtual HRESULT Create(HWND hwndDlg, SRECT& rc, UINT& nCtrlIdBase);
    virtual HRESULT HandleControlChangeNotification() = 0;
    virtual BOOL    OwnsCtrlId(int ctrlId)  
    {   
        UNREFERENCED_PARAMETER(ctrlId);
        return FALSE;   
    }
    virtual HRESULT OnClick()   {   return S_OK;    }
    virtual HRESULT OnHScroll() {   return S_OK;    }
    virtual LPCGUID GetEventContext() {   return &m_guidEventContext;    }
    virtual HRESULT CommitValue()   {   return S_OK;    }

    // IControlChangeNotify
    STDMETHOD(OnNotify)(_In_ DWORD dwProcessId, _In_opt_ LPCGUID pguidEventContext);

    // IUnknown (since we implement IControlChangeNotify)
    HRESULT STDMETHODCALLTYPE   QueryInterface(const IID& iid, void** ppUnk)
    { 
        UNREFERENCED_PARAMETER(iid);

        *ppUnk = NULL; 
        return E_NOTIMPL; 
    }
    ULONG   STDMETHODCALLTYPE   AddRef(void)
                                { return 0; }
    ULONG   STDMETHODCALLTYPE   Release(void)
                                { return 0; }
};


// ----------------------------------------------------------------------------
// Class:
//      CBooleanWidget
//
// Description:
//      Handles IAudioLoudness, IAudioAutoGainControl and
//      IGenericBooleanProperty
//
// ----------------------------------------------------------------------------
class CBooleanWidget : public CUIWidget
{
protected:
    BOOL    m_bCurrentValue;
    HWND    m_hwndCheckbox;

public:
    CBooleanWidget(IPart* pPart, REFIID iid);

    HRESULT Create(HWND hwndDlg, SRECT& rc, UINT& nCtrlIdBase);     // virtual

    virtual HRESULT HandleControlChangeNotification();
    virtual BOOL    OwnsCtrlId(int ctrlId)
                    {   return (ctrlId == GetDlgCtrlID(m_hwndCheckbox)); }
    HRESULT CommitValue();
};


// ----------------------------------------------------------------------------
// TODO:  Underlying plumbing for the LONG and ULOG types aren't implemented
// yet in audiocore.  Once that's implemented and verified working, need to
// remove #if TEST stuff
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class:
//      CLongWidget
//
// Description:
//      Handles IDeviceSpecificProperty
// ----------------------------------------------------------------------------
template <class _TYPE_>
class CLongWidget : public CUIWidget
{
public:
    CLongWidget(IPart* pPart);
    ~CLongWidget();
    virtual HRESULT Create(HWND hwndDlg, SRECT& rc, UINT& ctrlIdBase);
    virtual HRESULT HandleControlChangeNotification();
    virtual HRESULT CommitValue();

private:
    HWND    m_hwnd;
    HWND    m_hwndLabel;
    HWND    m_hwndTrackbar;
    HWND    m_hwndValue;
    _TYPE_  m_Value;
    _TYPE_  m_Min;
    _TYPE_  m_Max;
    LONG    m_Stepping;
    CComQIPtr<IDeviceSpecificProperty>  m_spDevSpecControl;

    static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    void                    UpdateValueText();
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Implementation of CLongWidget


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::CLongWidget
//
// Description:
//      CLongWidget<_TYPE_> constructor
// ----------------------------------------------------------------------------
template <class _TYPE_>
CLongWidget<_TYPE_>::CLongWidget(IPart* pPart)
:   CUIWidget(pPart, __uuidof(IDeviceSpecificProperty)),
    m_hwndTrackbar(NULL),
    m_hwndLabel(NULL),
    m_Value(0),
    m_Min(0),
    m_Max(0),
    m_Stepping(1)
{
}


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::CLongWidget
//
// Description:
//      CLongWidget<_TYPE_> destructor
// ----------------------------------------------------------------------------
template <class _TYPE_>
CLongWidget<_TYPE_>::~CLongWidget()
{
    DestroyWindow(m_hwnd);
}


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::Create
//
// Description:
//      Creates the widget which is a dialog
//
// Parameters:
//      hwndDlg - [in] Parent dialog of the widget
//      rc - [in] Position of the widget
//      ctrlIdBase - [in] Not used
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
template <class _TYPE_>
HRESULT CLongWidget<_TYPE_>::Create
(
    HWND hwndDlg,
    SRECT& rc,
    UINT& ctrlIdBase
)
{
    HRESULT hr = S_OK;
    HFONT   hFont;
    RECT    rcClient;
    LONG    lMin, lMax;

    // Activate the control interface and register for callbacks
    hr = CUIWidget::Create(hwndDlg, rc, ctrlIdBase);
    IF_FAILED_JUMP(hr, Exit);

    // QI for specific interface
    m_spDevSpecControl = m_spControl;

    // Create dialog
    m_hwnd = CreateDialogParam( g_hInstance,
                                MAKEINTRESOURCE(IDD_WIDGET_GEN_LONG),
                                hwndDlg,
                                CLongWidget<_TYPE_>::DlgProc,
                                (LPARAM)this);

    IF_TRUE_ACTION_JUMP((m_hwnd == NULL), hr = HRESULT_FROM_WIN32(GetLastError()), Exit);

    SetWindowPos(m_hwnd, NULL, rc.x, rc.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    GetClientRect(m_hwnd, &rcClient);
    rc.w = rcClient.right - rcClient.left;
    rc.h = rcClient.bottom - rcClient.top;

    // Make the new controls use the same font as the dialog
    hFont = Window_GetFont(hwndDlg);

    m_hwndLabel    = GetDlgItem(m_hwnd, IDC_GEN_LABEL);
    m_hwndTrackbar = GetDlgItem(m_hwnd, IDC_GEN_SLIDER);
    m_hwndValue    = GetDlgItem(m_hwnd, IDC_GEN_VALUE);

    Window_SetFont(m_hwndValue, hFont);
    Window_SetFont(m_hwndLabel, hFont);
    SetWindowText(m_hwndLabel, m_wstrLabel);

    // Set value text
    hr = m_spDevSpecControl->Get4BRange(&lMin, &lMax, &m_Stepping);
    IF_FAILED_JUMP(hr, Exit);

    m_Min = (_TYPE_)lMin;
    m_Max = (_TYPE_)lMax;

    // Set trackbar range and ticks
    TrackBar_SetRange(m_hwndTrackbar, 0, 100);
    TrackBar_SetTickFrequency(m_hwndTrackbar, 20);    // 5 ticks

    // Set trackbar position
    HandleControlChangeNotification();

    // Update the numeric representation of the trackbar
    UpdateValueText();

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::HandleControlChangeNotification
//
// Description:
//      Handles widget change notifications
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
template <class _TYPE_>
HRESULT CLongWidget<_TYPE_>::HandleControlChangeNotification()
{
    ATLASSERT(m_spDevSpecControl);
    ATLASSERT(m_hwndTrackbar);

    HRESULT hr = S_OK;
    DWORD   cbValue = sizeof(m_Value);

    // Get current value
    hr = m_spDevSpecControl->GetValue(&m_Value, &cbValue);

    if (SUCCEEDED(hr))
    {
        float fPos = (float)(m_Value - m_Min) / (float)(m_Max - m_Min);

        // Update trackbar position
        TrackBar_SetPos(m_hwndTrackbar, (int)(fPos * 100.f));
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::CommitValue
//
// Description:
//      Commit the value set on the control
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
template <class _TYPE_>
HRESULT CLongWidget<_TYPE_>::CommitValue()
{
    ATLASSERT(m_spDevSpecControl);
    ATLASSERT(m_hwndTrackbar);

    HRESULT hr = S_OK;
    float   fPos;
    DWORD   cbValue = sizeof(m_Value);

    // Calculate new value from trackbar position
    fPos = (float)TrackBar_GetPos(m_hwndTrackbar) / 100.f;
    m_Value = (_TYPE_)((float)(m_Max - m_Min) * fPos) + m_Min;

    // Commit current setting of trackbar
    hr = m_spDevSpecControl->SetValue(&m_Value, cbValue, GetEventContext());

    return(hr);
};


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::DlgProc
//
// Description:
//      Callback for widget dialog
//
// Parameters:
//      hwndDlg - [in] Handle to the widget dialog
//      uMsg - [in] Specifies the message
//      wParam - [in] Specifies additional message-specific information
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE if it processed the message, FALSE if not
// ----------------------------------------------------------------------------
template <class _TYPE_>
INT_PTR CALLBACK CLongWidget<_TYPE_>::DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    CLongWidget<_TYPE_>* pthis = (CLongWidget<_TYPE_>*)
        (LONG_PTR)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    BOOL fRet = FALSE;

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
            fRet = FALSE;   // don't set focus
            break; 
        }

        case WM_DESTROY:
        {
            // Don't delete.  The Widget destructor is what posts the WM_DESTROY
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
            fRet = TRUE;
            break;
        }

        case WM_HSCROLL:

            if (pthis)
            {
                pthis->m_Value = TrackBar_GetPos(pthis->m_hwndTrackbar);
                pthis->UpdateValueText();

                // Enable the Apply button upon user changing the control
                SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)(GetParent(hwndDlg)), 0);
            }
            fRet = TRUE;
            break;
    }

    return(fRet);
}


// ----------------------------------------------------------------------------
// Function:
//      CLongWidget<_TYPE_>::UpdateValueText
//
// Description:
//      Updates the numeric representation of the trackbar
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
template <class _TYPE_>
void CLongWidget<_TYPE_>::UpdateValueText()
{
    WCHAR wsz[12];

    StringCbPrintfW(wsz, sizeof(wsz), L"0x%08x", m_Value);

    // Set new text
    SetWindowText(m_hwndValue, wsz);
}
