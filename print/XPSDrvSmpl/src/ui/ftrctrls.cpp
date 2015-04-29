/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ftrctrls.cpp

Abstract:

   Implementation of the features property page UI controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "resource.h"
#include "ftrctrls.h"
#include "privatedefs.h"

PCSTR CUICtrlFeatPgScaleCombo::m_pszFeatPgScale         = "PageScaling";

PCSTR CUICtrlFeatScaleOffsetCombo::m_pszFeatScaleOffset = "ScaleOffsetAlignment";

PCSTR CUICtrlFeatPgScaleXEdit::m_pszFeatPgScaleX        = "PageScalingScaleWidth";
PCSTR CUICtrlFeatPgScaleXSpin::m_pszFeatPgScaleX        = "PageScalingScaleWidth";
PCSTR CUICtrlFeatPgScaleYEdit::m_pszFeatPgScaleY        = "PageScalingScaleHeight";
PCSTR CUICtrlFeatPgScaleYSpin::m_pszFeatPgScaleY        = "PageScalingScaleHeight";
PCSTR CUICtrlFeatPgOffsetXEdit::m_pszFeatPgOffsetX      = "PageScalingOffsetWidth";
PCSTR CUICtrlFeatPgOffsetXSpin::m_pszFeatPgOffsetX      = "PageScalingOffsetWidth";
PCSTR CUICtrlFeatPgOffsetYEdit::m_pszFeatPgOffsetY      = "PageScalingOffsetHeight";
PCSTR CUICtrlFeatPgOffsetYSpin::m_pszFeatPgOffsetY      = "PageScalingOffsetHeight";

PCSTR CUICtrlFeatNUpCombo::m_pszFeatNUp                 = "DocumentNUp";
PCSTR CUICtrlFeatNUpOrderCombo::m_pszFeatNUpOrder       = "DocumentNUpPresentationOrder";

PCSTR CUICtrlFeatDocDuplexCombo::m_pszFeatDocDuplex     = "DocumentDuplex";
PCSTR CUICtrlFeatPhotIntCombo::m_pszFeatPhotInt         = "PagePhotoPrintingIntent";
PCSTR CUICtrlFeatBordersCheck::m_pszFeatBorders         = "PageBorderless";

PCSTR CUICtrlFeatJobBindCombo::m_pszFeatJobBind         = "JobBindAllDocuments";
PCSTR CUICtrlFeatDocBindCombo::m_pszFeatDocBind         = "DocumentBinding";

#define PGSCALE_NONE_SEL            0
#define PGSCALE_CUSTOM_SEL          1
#define PGSCALE_CUSTSQUARE_SEL      2
#define PGSCALE_FITBLEED_SEL        3
#define PGSCALE_FITCONTENT_SEL      4
#define PGSCALE_FITPAGE_SEL         5
#define PGSCALE_SCALEPAGETOPAGE_SEL 6

#define JOBBIND_NONE_SEL            0

//
// Page scale selection combo box control
//
/*++

Routine Name:

    CUICtrlFeatPgScaleCombo::CUICtrlFeatPgScaleCombo

Routine Description:

    CUICtrlFeatPgScaleCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleCombo::CUICtrlFeatPgScaleCombo() :
    CUICtrlDefaultCombo(m_pszFeatPgScale, IDC_COMBO_PGSCALE)
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleCombo::~CUICtrlFeatPgScaleCombo

Routine Description:

    CUICtrlFeatPgScaleCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleCombo::~CUICtrlFeatPgScaleCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleCombo::OnInit

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
CUICtrlFeatPgScaleCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_CUSTOM)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_CUSTSQUARE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_FITBLEED)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_FITCONTENT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_FITPAGE)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALEPAGETOPAGE);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlFeatPgScaleCombo::EnableDependentCtrls

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
CUICtrlFeatPgScaleCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    BOOL bNone         = (lSel == PGSCALE_NONE_SEL);
    BOOL bCustom       = (lSel == PGSCALE_CUSTOM_SEL);
    BOOL bCustomSquare = (lSel == PGSCALE_CUSTSQUARE_SEL);
    BOOL bFit          = !(bCustom | bCustomSquare) && !bNone;

    //
    // Here we are enabling/disabling and showing/hiding the page scale controls
    // based on the currently selected options.
    //
    // If custom scaling is selected we need to enable and show the X/Y offset and X/Y
    // scaling controls and hide the "fit to" options. If we custom square scaling is
    // selected we show the same controls as custom scaling but disable the Y scaling
    // control so the user can only apply the scale in one dimension.
    //
    // If one of the "fit to" options (FitApplicationBleedSizeToPageImageableSize etc.)
    // is selected we disable and hide the custom controls (X/Y offset and scale) and enable
    // the offset alignment option control.
    //
    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_PGSCALEX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_PGSCALEX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_PGSCALEX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_PGSCALEY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_PGSCALEY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_PGSCALEY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_PGOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_PGOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_PGOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_PGOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_PGOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_PGOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bCustom | bCustomSquare);
        ShowWindow(hWnd, (bCustom | bCustomSquare | bNone) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_SCALEOFF), E_HANDLE)))
    {
        EnableWindow(hWnd, bFit);
        ShowWindow(hWnd, bFit ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_SCALEOFF), E_HANDLE)))
    {
        EnableWindow(hWnd, bFit);
        ShowWindow(hWnd, bFit ? SW_SHOW : SW_HIDE);
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Page scale selection combo box control
//
/*++

Routine Name:

    CUICtrlFeatScaleOffsetCombo::CUICtrlFeatScaleOffsetCombo

Routine Description:

    CUICtrlFeatScaleOffsetCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatScaleOffsetCombo::CUICtrlFeatScaleOffsetCombo() :
    CUICtrlDefaultCombo(m_pszFeatScaleOffset, IDC_COMBO_SCALEOFF)
{
}

/*++

Routine Name:

    CUICtrlFeatScaleOffsetCombo::~CUICtrlFeatScaleOffsetCombo

Routine Description:

    CUICtrlFeatScaleOffsetCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatScaleOffsetCombo::~CUICtrlFeatScaleOffsetCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatScaleOffsetCombo::OnInit

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
CUICtrlFeatScaleOffsetCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_BC)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_BL)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_BR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_CC)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_LC)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_CR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_CT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_TL)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_SCALE_ALIGN_TR);
    }

    ERR_ON_HR(hr);
    return hr;
}

#define NUP_1PPS_SEL 0

//
// NUp page per sheet combo box control
//
/*++

Routine Name:

    CUICtrlFeatNUpCombo::CUICtrlFeatNUpCombo

Routine Description:

    CUICtrlFeatNUpCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatNUpCombo::CUICtrlFeatNUpCombo() :
    CUICtrlDefaultCombo(m_pszFeatNUp, IDC_COMBO_NUP)
{
}

/*++

Routine Name:

    CUICtrlFeatNUpCombo::~CUICtrlFeatNUpCombo

Routine Description:

    CUICtrlFeatNUpCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatNUpCombo::~CUICtrlFeatNUpCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatNUpCombo::OnInit

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
CUICtrlFeatNUpCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_1PPS)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_2PPS)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_4PPS)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_6PPS)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_8PPS)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_9PPS)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_16PPS);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlFeatNUpCombo::EnableDependentCtrls

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
CUICtrlFeatNUpCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    //
    // Here we are enabling/disabling the NUp and Binding controls depending on the current
    // NUp selection.
    //
    // When NUp is more than 1 page per sheet we enable the NUp order controls and disable
    // binding option controls.
    //
    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_NUP_ORDER), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel > NUP_1PPS_SEL);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_NUP_ORDER), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel > NUP_1PPS_SEL);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_JOBBIND), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel == NUP_1PPS_SEL);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_JOBBIND), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel == NUP_1PPS_SEL);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_DOCBIND), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel == NUP_1PPS_SEL);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_DOCBIND), E_HANDLE)))
    {
       EnableWindow(hWnd, lSel == NUP_1PPS_SEL);
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// NUp presentation order combo box control
//
/*++

Routine Name:

    CUICtrlFeatNUpOrderCombo::CUICtrlFeatNUpOrderCombo

Routine Description:

    CUICtrlFeatNUpOrderCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatNUpOrderCombo::CUICtrlFeatNUpOrderCombo() :
    CUICtrlDefaultCombo(m_pszFeatNUpOrder, IDC_COMBO_NUP_ORDER)
{
}

/*++

Routine Name:

    CUICtrlFeatNUpOrderCombo::~CUICtrlFeatNUpOrderCombo

Routine Description:

    CUICtrlFeatNUpOrderCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatNUpOrderCombo::~CUICtrlFeatNUpOrderCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatNUpOrderCombo::OnInit

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
CUICtrlFeatNUpOrderCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_LTORTTOB)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_TTOBLTOR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RTOLTTOB)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_TTOBRTOL)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_LTORBTOT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_BTOTLTOR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RTOLBTOT)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_BTOTRTOL);
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Binding combo box control
//
/*++

Routine Name:

    CUICtrlFeatJobBindCombo::CUICtrlFeatJobBindCombo

Routine Description:

    CUICtrlFeatJobBindCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatJobBindCombo::CUICtrlFeatJobBindCombo() :
    CUICtrlDefaultCombo(m_pszFeatJobBind, IDC_COMBO_JOBBIND)
{
}

/*++

Routine Name:

    CUICtrlFeatJobBindCombo::~CUICtrlFeatJobBindCombo

Routine Description:

    CUICtrlFeatJobBindCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatJobBindCombo::~CUICtrlFeatJobBindCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatJobBindCombo::OnInit

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
CUICtrlFeatJobBindCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;
    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_LTOR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RTOL)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_TTOB)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_BTOT);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlFeatJobBindCombo::EnableDependentCtrls

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
CUICtrlFeatJobBindCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    //
    // Here we are enabling/disabling binding and NUp controls based off the current
    // binding option.
    //
    // If JobBindAllDocuments option is selected we disable DocumentBinding and NUp controls.
    //
    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_DOCBIND), E_HANDLE)))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_DOCBIND), E_HANDLE))))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_NUP), E_HANDLE)))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_NUP), E_HANDLE))))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_NUP_ORDER), E_HANDLE)))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_NUP_ORDER), E_HANDLE))))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Binding Direction combo box control
//
/*++

Routine Name:

    CUICtrlFeatDocBindCombo::CUICtrlFeatDocBindCombo

Routine Description:

    CUICtrlFeatDocBindCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatDocBindCombo::CUICtrlFeatDocBindCombo() :
    CUICtrlDefaultCombo(m_pszFeatDocBind, IDC_COMBO_DOCBIND)
{
}

/*++

Routine Name:

    CUICtrlFeatDocBindCombo::~CUICtrlFeatDocBindCombo

Routine Description:

    CUICtrlFeatDocBindCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatDocBindCombo::~CUICtrlFeatDocBindCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatDocBindCombo::OnInit

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
CUICtrlFeatDocBindCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;
    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_LTOR)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RTOL)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_TTOB)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_BTOT);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlFeatDocBindCombo::EnableDependentCtrls

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
CUICtrlFeatDocBindCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    //
    // Here we are enabling/disabling binding and NUp controls based off the current
    // DocumentBinding option.
    //
    // If DocumentBinding option is selected we disable JobBindAllDocuments and NUp controls.
    //
    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_NUP), E_HANDLE)))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_NUP), E_HANDLE))))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_NUP_ORDER), E_HANDLE)))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_NUP_ORDER), E_HANDLE))))
    {
       EnableWindow(hWnd, (lSel == JOBBIND_NONE_SEL));
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Document photo printing intent combo box control
//
/*++

Routine Name:

    CUICtrlFeatPhotIntCombo::CUICtrlFeatPhotIntCombo

Routine Description:

    CUICtrlFeatPhotIntCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPhotIntCombo::CUICtrlFeatPhotIntCombo() :
    CUICtrlDefaultCombo(m_pszFeatPhotInt, IDC_COMBO_PHOTO_INTENT)
{
}

/*++

Routine Name:

    CUICtrlFeatPhotIntCombo::~CUICtrlFeatPhotIntCombo

Routine Description:

    CUICtrlFeatPhotIntCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPhotIntCombo::~CUICtrlFeatPhotIntCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatPhotIntCombo::OnInit

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
CUICtrlFeatPhotIntCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_BEST)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_DRAFT)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_STANDARD);
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Document duplex combo box control
//
/*++

Routine Name:

    CUICtrlFeatDocDuplexCombo::CUICtrlFeatDocDuplexCombo

Routine Description:

    CUICtrlFeatDocDuplexCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatDocDuplexCombo::CUICtrlFeatDocDuplexCombo() :
    CUICtrlDefaultCombo(m_pszFeatDocDuplex, IDC_COMBO_DOCDUPLEX)
{
}

/*++

Routine Name:

    CUICtrlFeatDocDuplexCombo::~CUICtrlFeatDocDuplexCombo

Routine Description:

    CUICtrlFeatDocDuplexCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatDocDuplexCombo::~CUICtrlFeatDocDuplexCombo()
{
}

/*++

Routine Name:

    CUICtrlFeatDocDuplexCombo::OnInit

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
CUICtrlFeatDocDuplexCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;
    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE))&&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_HORIZONTAL)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_VERTICAL);
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Document duplex combo box control
//
/*++

Routine Name:

    CUICtrlFeatPgScaleXEdit::CUICtrlFeatPgScaleXEdit

Routine Description:

    CUICtrlFeatPgScaleXEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleXEdit::CUICtrlFeatPgScaleXEdit() :
    CUICtrlDefaultEditNum(m_pszFeatPgScaleX,
                          IDC_EDIT_PGSCALEX,
                          pgscParamDefIntegers[ePageScalingScaleWidth].min_length,
                          pgscParamDefIntegers[ePageScalingScaleWidth].max_length,
                          IDC_SPIN_PGSCALEX)
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleXEdit::~CUICtrlFeatPgScaleXEdit

Routine Description:

    CUICtrlFeatPgScaleXEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleXEdit::~CUICtrlFeatPgScaleXEdit()
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleXSpin::CUICtrlFeatPgScaleXSpin

Routine Description:

    CUICtrlFeatPgScaleXSpin class constructor

Arguments:

    pEdit - Pointer to the edit num buddy control

Return Value:

    None

--*/
CUICtrlFeatPgScaleXSpin::CUICtrlFeatPgScaleXSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleXSpin::~CUICtrlFeatPgScaleXSpin

Routine Description:

    CUICtrlFeatPgScaleXSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleXSpin::~CUICtrlFeatPgScaleXSpin()
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleYEdit::CUICtrlFeatPgScaleYEdit

Routine Description:

    CUICtrlFeatPgScaleYEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleYEdit::CUICtrlFeatPgScaleYEdit() :
    CUICtrlDefaultEditNum(m_pszFeatPgScaleY,
                          IDC_EDIT_PGSCALEY,
                          pgscParamDefIntegers[ePageScalingScaleHeight].min_length,
                          pgscParamDefIntegers[ePageScalingScaleHeight].max_length,
                          IDC_SPIN_PGSCALEY)
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleYEdit::~CUICtrlFeatPgScaleYEdit

Routine Description:

    CUICtrlFeatPgScaleYEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleYEdit::~CUICtrlFeatPgScaleYEdit()
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleYSpin::CUICtrlFeatPgScaleYSpin

Routine Description:

    CUICtrlFeatPgScaleYSpin class constructor

Arguments:

    pEdit - Pointer to the edit num buddy control

Return Value:

    None

--*/
CUICtrlFeatPgScaleYSpin::CUICtrlFeatPgScaleYSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlFeatPgScaleYSpin::~CUICtrlFeatPgScaleYSpin

Routine Description:

    CUICtrlFeatPgScaleYSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgScaleYSpin::~CUICtrlFeatPgScaleYSpin()
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetXEdit::CUICtrlFeatPgOffsetXEdit

Routine Description:

    CUICtrlFeatPgOffsetXEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetXEdit::CUICtrlFeatPgOffsetXEdit() :
    CUICtrlDefaultEditNum(m_pszFeatPgOffsetX,
                          IDC_EDIT_PGOFFX,
                          MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].max_length),
                          IDC_SPIN_PGOFFX)
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetXEdit::~CUICtrlFeatPgOffsetXEdit

Routine Description:

    CUICtrlFeatPgOffsetXEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetXEdit::~CUICtrlFeatPgOffsetXEdit()
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetXSpin::CUICtrlFeatPgOffsetXSpin

Routine Description:

    CUICtrlFeatPgOffsetXSpin class constructor

Arguments:

    pEdit - Pointer to the edit num buddy control

Return Value:

    None

--*/
CUICtrlFeatPgOffsetXSpin::CUICtrlFeatPgOffsetXSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetXSpin::~CUICtrlFeatPgOffsetXSpin

Routine Description:

    CUICtrlFeatPgOffsetXSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetXSpin::~CUICtrlFeatPgOffsetXSpin()
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetYEdit::CUICtrlFeatPgOffsetYEdit

Routine Description:

    CUICtrlFeatPgOffsetYEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetYEdit::CUICtrlFeatPgOffsetYEdit() :
    CUICtrlDefaultEditNum(m_pszFeatPgOffsetY,
                          IDC_EDIT_PGOFFY,
                          MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].max_length),
                          IDC_SPIN_PGOFFY)
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetYEdit::~CUICtrlFeatPgOffsetYEdit

Routine Description:

    CUICtrlFeatPgOffsetYEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetYEdit::~CUICtrlFeatPgOffsetYEdit()
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetYSpin::CUICtrlFeatPgOffsetYSpin

Routine Description:

    CUICtrlFeatPgOffsetYSpin class constructor

Arguments:

    pEdit - Pointer to the edit num buddy control

Return Value:

    None

--*/
CUICtrlFeatPgOffsetYSpin::CUICtrlFeatPgOffsetYSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlFeatPgOffsetYSpin::~CUICtrlFeatPgOffsetYSpin

Routine Description:

    CUICtrlFeatPgOffsetYSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatPgOffsetYSpin::~CUICtrlFeatPgOffsetYSpin()
{
}

/*++

Routine Name:

    CUICtrlFeatBordersCheck::CUICtrlFeatBordersCheck

Routine Description:

    CUICtrlFeatBordersCheck class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatBordersCheck::CUICtrlFeatBordersCheck() :
    CUICtrlDefaultCheck(m_pszFeatBorders, IDC_CHECK_BORDERLESS)
{
}

/*++

Routine Name:

    CUICtrlFeatBordersCheck::~CUICtrlFeatBordersCheck

Routine Description:

    CUICtrlFeatBordersCheck class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlFeatBordersCheck::~CUICtrlFeatBordersCheck()
{
}

