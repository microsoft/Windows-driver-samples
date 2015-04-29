/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmppg.cpp

Abstract:

   Implementation of the watermark property page. This class is
   responsible for initialising and registering the color management
   property page and its controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "resource.h"
#include "wmppg.h"
#include "wmctrls.h"

/*++

Routine Name:

    CWatermarkPropPage::CWatermarkPropPage

Routine Description:

    CWatermarkPropPage class constructor.
    Creates a handler class object for every control on the watermark property page.
    Each of these handlers is stored in a collection.

Arguments:

    None

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWatermarkPropPage::CWatermarkPropPage()
{
    HRESULT hr = S_OK;

    try
    {
        CUIControl* pControl = new(std::nothrow) CUICtrlWMTypeCombo();

        if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
        {
            hr = AddUIControl(IDC_COMBO_WMTYPE, pControl);
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMLayeringCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_WMLAYERING, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMTextEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_EDIT_WMTEXT, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMTransparencyEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMTRANSPARENCY, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMTransparencySpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMTRANSPARENCY, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMAngleEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMANGLE, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMAngleSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMANGLE, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMOffsetXEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMOFFX, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMOffsetXSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMOFFX, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMOffsetYEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMOFFY, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMOffsetYSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMOFFY, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMWidthEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMWIDTH, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMWidthSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMWIDTH, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMHeightEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMHEIGHT, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMHeightSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMHEIGHT, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlWMFontSizeEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_WMSIZE, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlWMFontSizeSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_WMSIZE, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlColorBtn();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_BUTTON_WMCOLOR, pControl);
            }
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    if (FAILED(hr))
    {
        DestroyUIComponents();
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CWatermarkPropPage::~CWatermarkPropPage

Routine Description:

    CWatermarkPropPage class destructor

Arguments:

    None

Return Value:

    None

--*/
CWatermarkPropPage::~CWatermarkPropPage()
{
}

/*++

Routine Name:

    CWatermarkPropPage::InitDlgBox

Routine Description:

    Provides the base class with the data required to intialise the dialog box.

Arguments:

    ppszTemplate - Pointer to dialog box template to be intialised.
    ppszTitle    - Pointer to dialog box title to be intialised.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkPropPage::InitDlgBox(
    _Out_ LPCTSTR* ppszTemplate,
    _Out_ LPCTSTR* ppszTitle
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppszTemplate, E_POINTER)) ||
        SUCCEEDED(hr = CHECK_POINTER(ppszTitle, E_POINTER)))
    {
        *ppszTemplate = MAKEINTRESOURCE(IDD_WATERMARK);
        *ppszTitle    = MAKEINTRESOURCE(IDS_WMARK);
    }

    ERR_ON_HR(hr);
    return hr;
}

