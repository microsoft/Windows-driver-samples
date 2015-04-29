/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmctrls.cpp

Abstract:

   Implementation of the watermark specific UI controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "resource.h"
#include "wmctrls.h"
#include "privatedefs.h"

PCSTR CUICtrlWMTypeCombo::m_pszWMType         = "PageWatermarkType";

PCSTR CUICtrlWMLayeringCombo::m_pszWMLayering = "PageWatermarkLayering";
PCSTR CUICtrlWMTransparencyEdit::m_pszWMTransparency    = "PageWatermarkTransparency";
PCSTR CUICtrlWMTransparencySpin::m_pszWMTransparency    = "PageWatermarkTransparency";
PCSTR CUICtrlWMAngleEdit::m_pszWMAngle        = "PageWatermarkTextAngle";
PCSTR CUICtrlWMAngleSpin::m_pszWMAngle        = "PageWatermarkTextAngle";
PCSTR CUICtrlWMOffsetXEdit::m_pszWMOffsetX    = "PageWatermarkOriginWidth";
PCSTR CUICtrlWMOffsetXSpin::m_pszWMOffsetX    = "PageWatermarkOriginWidth";
PCSTR CUICtrlWMOffsetYEdit::m_pszWMOffsetY    = "PageWatermarkOriginHeight";
PCSTR CUICtrlWMOffsetYSpin::m_pszWMOffsetY    = "PageWatermarkOriginHeight";

PCSTR CUICtrlWMWidthEdit::m_pszWMWidth        = "PageWatermarkSizeWidth";
PCSTR CUICtrlWMWidthSpin::m_pszWMWidth        = "PageWatermarkSizeWidth";
PCSTR CUICtrlWMHeightEdit::m_pszWMHeight      = "PageWatermarkSizeHeight";
PCSTR CUICtrlWMHeightSpin::m_pszWMHeight      = "PageWatermarkSizeHeight";

PCSTR CUICtrlWMTextEdit::m_pszWMText          = "PageWatermarkTextText";

PCSTR CUICtrlWMFontSizeEdit::m_pszWMFontSize  = "PageWatermarkTextFontSize";
PCSTR CUICtrlWMFontSizeSpin::m_pszWMFontSize  = "PageWatermarkTextFontSize";

PCSTR CUICtrlColorBtn::m_pszWMFontColor       = "PageWatermarkTextColor";

#define WMTYPE_NONE_SEL 0
#define WMTYPE_TEXT_SEL 1
#define WMTYPE_RAST_SEL 2
#define WMTYPE_VECT_SEL 3

//
// Watermark type combo box control
//
/*++

Routine Name:

    CUICtrlWMTypeCombo::CUICtrlWMTypeCombo

Routine Description:

    CUICtrlWMTypeCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTypeCombo::CUICtrlWMTypeCombo() :
    CUICtrlDefaultCombo(m_pszWMType, IDC_COMBO_WMTYPE)
{
}

/*++

Routine Name:

    CUICtrlWMTypeCombo::~CUICtrlWMTypeCombo

Routine Description:

    CUICtrlWMTypeCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTypeCombo::~CUICtrlWMTypeCombo()
{
}

/*++

Routine Name:

    CUICtrlWMTypeCombo::OnInit

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
CUICtrlWMTypeCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_NONE)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_TEXT)) &&
        SUCCEEDED(hr = AddString(hDlg, g_hInstance, IDS_GPD_RASTERIMAGE)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_VECTORIMAGE);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlWMTypeCombo::EnableDependentCtrls

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
CUICtrlWMTypeCombo::EnableDependentCtrls(
    _In_ CONST HWND hDlg,
    _In_ CONST LONG lSel
    )
{
    HRESULT hr = S_OK;
    HWND hWnd = NULL;

    BOOL bWatermarkEnabled = (lSel != WMTYPE_NONE_SEL);
    BOOL bRasterType       = (lSel == WMTYPE_RAST_SEL);
    BOOL bVectorType       = (lSel == WMTYPE_VECT_SEL);
    BOOL bTextType         = (lSel == WMTYPE_TEXT_SEL);

    //
    // Common Watermark Properties
    //
    // Here we are enabling/disabling common watermark controls based off the current
    // watermark option.
    //
    // If a watermark is selected we enable the layering, transparentcy offset and angle controls
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_COMBO_WMLAYERING), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMLAYERING), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMTRANSPARENCY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMTRANSPARENCY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMTRANSPARENCY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMANGLE), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMANGLE), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMANGLE), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMOFFX), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMOFFY), E_HANDLE)))
    {
        EnableWindow(hWnd, bWatermarkEnabled);
    }

    //
    // Vector / Bitmap Watermark Properties
    //
    // Here we are enabling/disabling vector/bitmap watermark controls based off the current
    // watermark option.
    //
    // If a vector or bitmap watermark is selected we enable the width and height controls
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMWIDTH), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMWIDTH), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMWIDTH), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMHEIGHT), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMHEIGHT), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMHEIGHT), E_HANDLE)))
    {
        EnableWindow(hWnd, (bRasterType || bVectorType));
        ShowWindow(hWnd, (bRasterType || bVectorType) ? SW_SHOW : SW_HIDE);
    }

    //
    // Text Watermark Properties
    //
    // Here we are enabling/disabling text watermark controls based off the current
    // watermark option.
    //
    // If a text watermark is selected we enable the text and font controls
    //
    if (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMTEXT), E_HANDLE)))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMTEXT), E_HANDLE))))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

     if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_EDIT_WMSIZE), E_HANDLE))))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_SPIN_WMSIZE), E_HANDLE))))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_TXT_WMSIZE), E_HANDLE))))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

    if (SUCCEEDED(hr) &&
        (SUCCEEDED(hr = CHECK_HANDLE(hWnd = GetDlgItem(hDlg, IDC_BUTTON_WMCOLOR), E_HANDLE))))
    {
       EnableWindow(hWnd, bTextType);
       ShowWindow(hWnd, bTextType ? SW_SHOW : SW_HIDE);
    }

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Watermark Layering combo box control
//
/*++

Routine Name:

    CUICtrlWMLayeringCombo::CUICtrlWMLayeringCombo

Routine Description:

    CUICtrlWMLayeringCombo class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMLayeringCombo::CUICtrlWMLayeringCombo() :
    CUICtrlDefaultCombo(m_pszWMLayering, IDC_COMBO_WMLAYERING)
{
}

/*++

Routine Name:

    CUICtrlWMLayeringCombo::~CUICtrlWMLayeringCombo

Routine Description:

    CUICtrlWMLayeringCombo class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMLayeringCombo::~CUICtrlWMLayeringCombo()
{
}

/*++

Routine Name:

    CUICtrlWMLayeringCombo::OnInit

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
CUICtrlWMLayeringCombo::OnInit(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    //
    // Populate the combo box
    //
    if (SUCCEEDED (hr = AddString(hDlg, g_hInstance, IDS_GPD_OVERLAYED)))
    {
        hr = AddString(hDlg, g_hInstance, IDS_GPD_UNDERLAYED);
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Watermark text combo box control
//
/*++

Routine Name:

    CUICtrlWMTextEdit::CUICtrlWMTextEdit

Routine Description:

    CUICtrlWMTextEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTextEdit::CUICtrlWMTextEdit() :
    CUICtrlDefaultEditText(m_pszWMText,
                           IDC_EDIT_WMTEXT,
                           MAX_WATERMARK_TEXT)
{
}

/*++

Routine Name:

    CUICtrlWMTextEdit::~CUICtrlWMTextEdit

Routine Description:

    CUICtrlWMTextEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTextEdit::~CUICtrlWMTextEdit()
{
}

//
// Page Watermark Transparency
//

/*++

Routine Name:

    CUICtrlWMTransparencyEdit::CUICtrlWMTransparencyEdit

Routine Description:

    CUICtrlWMTransparencyEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTransparencyEdit::CUICtrlWMTransparencyEdit() :
    CUICtrlDefaultEditNum(m_pszWMTransparency,
                          IDC_EDIT_WMTRANSPARENCY,
                          wmParamDefIntegers[ePageWatermarkTransparency].min_length,
                          wmParamDefIntegers[ePageWatermarkTransparency].max_length,
                          IDC_SPIN_WMTRANSPARENCY)
{
}

/*++

Routine Name:

    CUICtrlWMTransparencyEdit::~CUICtrlWMTransparencyEdit

Routine Description:

    CUICtrlWMTransparencyEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTransparencyEdit::~CUICtrlWMTransparencyEdit()
{
}

/*++

Routine Name:

    CUICtrlWMTransparencySpin::CUICtrlWMTransparencySpin

Routine Description:

    CUICtrlWMTransparencySpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTransparencySpin::CUICtrlWMTransparencySpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMTransparencySpin::~CUICtrlWMTransparencySpin

Routine Description:

    CUICtrlWMTransparencySpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMTransparencySpin::~CUICtrlWMTransparencySpin()
{
}

//
// Page Watermark Angle
//

/*++

Routine Name:

    CUICtrlWMAngleEdit::CUICtrlWMAngleEdit

Routine Description:

    CUICtrlWMAngleEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMAngleEdit::CUICtrlWMAngleEdit() :
    CUICtrlDefaultEditNum(m_pszWMAngle,
                          IDC_EDIT_WMANGLE,
                          wmParamDefIntegers[ePageWatermarkAngle].min_length,
                          wmParamDefIntegers[ePageWatermarkAngle].max_length,
                          IDC_SPIN_WMANGLE)
{
}

/*++

Routine Name:

    CUICtrlWMAngleEdit::~CUICtrlWMAngleEdit

Routine Description:

    CUICtrlWMAngleEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMAngleEdit::~CUICtrlWMAngleEdit()
{
}

/*++

Routine Name:

    CUICtrlWMAngleSpin::CUICtrlWMAngleSpin

Routine Description:

    CUICtrlWMAngleSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMAngleSpin::CUICtrlWMAngleSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMAngleSpin::~CUICtrlWMAngleSpin

Routine Description:

    CUICtrlWMAngleSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMAngleSpin::~CUICtrlWMAngleSpin()
{
}

//
// Page Watermark Offset X
//

/*++

Routine Name:

    CUICtrlWMOffsetXEdit::CUICtrlWMOffsetXEdit

Routine Description:

    CUICtrlWMOffsetXEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetXEdit::CUICtrlWMOffsetXEdit() :
    CUICtrlDefaultEditNum(m_pszWMOffsetX,
                          IDC_EDIT_WMOFFX,
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].max_length),
                          IDC_SPIN_WMOFFX)
{
}

/*++

Routine Name:

    CUICtrlWMOffsetXEdit::~CUICtrlWMOffsetXEdit

Routine Description:

    CUICtrlWMOffsetXEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetXEdit::~CUICtrlWMOffsetXEdit()
{
}

/*++

Routine Name:

    CUICtrlWMOffsetXSpin::CUICtrlWMOffsetXSpin

Routine Description:

    CUICtrlWMOffsetXSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetXSpin::CUICtrlWMOffsetXSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMOffsetXSpin::~CUICtrlWMOffsetXSpin

Routine Description:

    CUICtrlWMOffsetXSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetXSpin::~CUICtrlWMOffsetXSpin()
{
}

//
// Page Watermark Offset Y
//

/*++

Routine Name:

    CUICtrlWMOffsetYEdit::CUICtrlWMOffsetYEdit

Routine Description:

    CUICtrlWMOffsetYEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetYEdit::CUICtrlWMOffsetYEdit() :
    CUICtrlDefaultEditNum(m_pszWMOffsetY,
                          IDC_EDIT_WMOFFY,
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].max_length),
                          IDC_SPIN_WMOFFY)
{
}

/*++

Routine Name:

    CUICtrlWMOffsetYEdit::~CUICtrlWMOffsetYEdit

Routine Description:

    CUICtrlWMOffsetYEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetYEdit::~CUICtrlWMOffsetYEdit()
{
}

/*++

Routine Name:

    CUICtrlWMOffsetYSpin::CUICtrlWMOffsetYSpin

Routine Description:

    CUICtrlWMOffsetYSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetYSpin::CUICtrlWMOffsetYSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMOffsetYSpin::~CUICtrlWMOffsetYSpin

Routine Description:

    CUICtrlWMOffsetYSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMOffsetYSpin::~CUICtrlWMOffsetYSpin()
{
}

//
// Page Watermark Width
//

/*++

Routine Name:

    CUICtrlWMWidthEdit::CUICtrlWMWidthEdit

Routine Description:

    CUICtrlWMWidthEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMWidthEdit::CUICtrlWMWidthEdit() :
    CUICtrlDefaultEditNum(m_pszWMWidth,
                          IDC_EDIT_WMWIDTH,
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].max_length),
                          IDC_SPIN_WMWIDTH)
{
}

/*++

Routine Name:

    CUICtrlWMWidthEdit::~CUICtrlWMWidthEdit

Routine Description:

    CUICtrlWMWidthEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMWidthEdit::~CUICtrlWMWidthEdit()
{
}

/*++

Routine Name:

    CUICtrlWMWidthSpin::CUICtrlWMWidthSpin

Routine Description:

    CUICtrlWMWidthSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMWidthSpin::CUICtrlWMWidthSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMWidthSpin::~CUICtrlWMWidthSpin

Routine Description:

    CUICtrlWMWidthSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMWidthSpin::~CUICtrlWMWidthSpin()
{
}

//
// Page Watermark Height
//

/*++

Routine Name:

    CUICtrlWMHeightEdit::CUICtrlWMHeightEdit

Routine Description:

    CUICtrlWMHeightEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMHeightEdit::CUICtrlWMHeightEdit() :
    CUICtrlDefaultEditNum(m_pszWMHeight,
                          IDC_EDIT_WMHEIGHT,
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].min_length),
                          MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].max_length),
                          IDC_SPIN_WMHEIGHT)
{
}

/*++

Routine Name:

    CUICtrlWMHeightEdit::~CUICtrlWMHeightEdit

Routine Description:

    CUICtrlWMHeightEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMHeightEdit::~CUICtrlWMHeightEdit()
{
}

/*++

Routine Name:

    CUICtrlWMHeightSpin::CUICtrlWMHeightSpin

Routine Description:

    CUICtrlWMHeightSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMHeightSpin::CUICtrlWMHeightSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMHeightSpin::~CUICtrlWMHeightSpin

Routine Description:

    CUICtrlWMHeightSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMHeightSpin::~CUICtrlWMHeightSpin()
{
}

//
// Page Watermark Font Size
//

/*++

Routine Name:

    CUICtrlWMFontSizeEdit::CUICtrlWMFontSizeEdit

Routine Description:

    CUICtrlWMFontSizeEdit class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMFontSizeEdit::CUICtrlWMFontSizeEdit() :
    CUICtrlDefaultEditNum(m_pszWMFontSize,
                          IDC_EDIT_WMSIZE,
                          wmParamDefIntegers[ePageWatermarkTextFontSize].min_length,
                          wmParamDefIntegers[ePageWatermarkTextFontSize].max_length,
                          IDC_SPIN_WMSIZE)
{
}

/*++

Routine Name:

    CUICtrlWMFontSizeEdit::~CUICtrlWMFontSizeEdit

Routine Description:

    CUICtrlWMFontSizeEdit class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMFontSizeEdit::~CUICtrlWMFontSizeEdit()
{
}

/*++

Routine Name:

    CUICtrlWMHeightSpin::CUICtrlWMHeightSpin

Routine Description:

    CUICtrlWMHeightSpin class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMFontSizeSpin::CUICtrlWMFontSizeSpin(
    _In_ CUICtrlDefaultEditNum* pEdit
    ) :
    CUICtrlDefaultSpin(pEdit)
{
}

/*++

Routine Name:

    CUICtrlWMFontSizeSpin::~CUICtrlWMFontSizeSpin

Routine Description:

    CUICtrlWMFontSizeSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlWMFontSizeSpin::~CUICtrlWMFontSizeSpin()
{
}

/*++

Routine Name:

    CUICtrlWMFontSizeSpin::~CUICtrlWMFontSizeSpin

Routine Description:

    CUICtrlWMFontSizeSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlColorBtn::CUICtrlColorBtn() :
    CUICtrlDefaultBtn(m_pszWMFontColor, IDC_BUTTON_WMCOLOR)
{
}

/*++

Routine Name:

    CUICtrlColorBtn::~CUICtrlColorBtn

Routine Description:

    CUICtrlColorBtn class constructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlColorBtn::~CUICtrlColorBtn()
{
}

/*++

Routine Name:

    CUICtrlColorBtn::OnBnClicked

Routine Description:

    Color button clicked handler

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlColorBtn::OnBnClicked(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    DWORD colorText = 0;
    if (SUCCEEDED(hr = CHECK_HANDLE(hDlg, E_HANDLE)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropertyString, reinterpret_cast<UIProperty*>(&colorText), sizeof(colorText))))
    {
        PBYTE pChannels = reinterpret_cast<PBYTE>(&colorText);

        COLORREF colorsCust[16] = {0};
        COLORREF rgbIn = RGB(pChannels[2], pChannels[1], pChannels[0]);

        colorsCust[0] = rgbIn;

        CHOOSECOLOR chooseColor = {
            sizeof(CHOOSECOLOR),
            hDlg,
            NULL,
            colorText,
            colorsCust,
            CC_RGBINIT | CC_SOLIDCOLOR,
            NULL,
            NULL,
            NULL
        };

        if (ChooseColor(&chooseColor) &&
            chooseColor.rgbResult != rgbIn)
        {
            pChannels[2] = GetRValue(chooseColor.rgbResult);
            pChannels[1] = GetGValue(chooseColor.rgbResult);
            pChannels[0] = GetBValue(chooseColor.rgbResult);

            if (SUCCEEDED(hr = m_pUIProperties->SetItem(m_szPropertyString, reinterpret_cast<UIProperty*>(&colorText), sizeof(colorText))))
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


