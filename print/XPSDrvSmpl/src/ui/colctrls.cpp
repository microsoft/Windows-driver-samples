/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colctrls.cpp

Abstract:

   Implementation of the color management specific UI controls. These are the
   combo box used to select the PageColorManagement option, a list box
   for selecting the PageSourceColorProfile option and a combo box for
   selecting the PageSourceColorProfile option.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "resource.h"
#include "colctrls.h"

PCSTR CUICtrlPageColManCombo::m_pszPageColManName    = "PageColorManagement";
PCSTR CUICtrlColProfList::m_pszDestColProf           = "PageSourceColorProfile";
PCSTR CUICtrlPageColIntentCombo::m_pszPageIntentName = "PageICMRenderingIntent";

#define DRIVER_COL_MAN_SEL 2

/*++

Routine Name:

    CUICtrlPageColManCombo::CUICtrlPageColManCombo

Routine Description:

    CUICtrlPageColManCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlPageColManCombo::CUICtrlPageColManCombo() :
    CUICtrlDefaultCombo(m_pszPageColManName, IDC_COMBO_COL_MANAGE)
{
}

/*++

Routine Name:

    CUICtrlPageColManCombo::~CUICtrlPageColManCombo

Routine Description:

    CUICtrlPageColManCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlPageColManCombo::~CUICtrlPageColManCombo()
{
}

/*++

Routine Name:

    CUICtrlPageColManCombo::OnInit

Routine Description:

    This is responsible for initialising the control and is called when
    the WM_INITDIALOG message is recieved. This method populates the combo
    box with the appropriate option strings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlPageColManCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;
    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_DEVICE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_DRIVER)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_SYSTEM);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlPageColManCombo::EnableDependentCtrls

Routine Description:

    This method is used to enable or disable other controls in the UI based on the
    current combo box selection.

Arguments:

    hDlg - handle to the parent window
    lSel - current combo box selection

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlPageColManCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TEXT_COLPROF), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel == DRIVER_COL_MAN_SEL);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_LIST_COLPROF), E_HANDLE))))
    {
       EnableWindow(hWnd, lSel == DRIVER_COL_MAN_SEL);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_COL_INTENT), E_HANDLE))))
    {
       EnableWindow(hWnd, lSel == DRIVER_COL_MAN_SEL);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_COL_INTENT), E_HANDLE))))
    {
       EnableWindow(hWnd, lSel == DRIVER_COL_MAN_SEL);
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlColProfList::CUICtrlColProfList

Routine Description:

    CUICtrlColProfList class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlColProfList::CUICtrlColProfList() :
    CUICtrlDefaultList(m_pszDestColProf, IDC_LIST_COLPROF)
{
}

/*++

Routine Name:

    CUICtrlColProfList::~CUICtrlColProfList

Routine Description:

    CUICtrlColProfList class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlColProfList::~CUICtrlColProfList()
{
}

/*++

Routine Name:

    CUICtrlColProfList::OnInit

Routine Description:

    This is responsible for initialising the control and is called when
    the WM_INITDIALOG message is recieved. This method populates the list
    box with the appropriate option strings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlColProfList::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;
    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_CMYK)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_SCRGB);
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CUICtrlPageColIntentCombo::CUICtrlPageColIntentCombo

Routine Description:

    CUICtrlPageColIntentCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlPageColIntentCombo::CUICtrlPageColIntentCombo() :
    CUICtrlDefaultCombo(m_pszPageIntentName, IDC_COMBO_COL_INTENT)
{
}

/*++

Routine Name:

    CUICtrlPageColIntentCombo::~CUICtrlPageColIntentCombo

Routine Description:

    CUICtrlPageColIntentCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlPageColIntentCombo::~CUICtrlPageColIntentCombo()
{
}

/*++

Routine Name:

    CUICtrlPageColIntentCombo::OnInit

Routine Description:

    This is responsible for initialising the control and is called when
    the WM_INITDIALOG message is recieved. This method populates the combo
    box with the appropriate option strings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlPageColIntentCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_ABSCOLINTENT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RELCOLINTENT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_PHOTOINTENT)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_BIZINTENT);
    }

    ERR_ON_HR(hr);
    return hr;
}

