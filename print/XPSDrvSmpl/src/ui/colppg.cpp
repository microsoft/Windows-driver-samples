/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colppg.cpp

Abstract:

   Implementation of the color management property page. This class is
   responsible for initialising and registering the color management
   property page and its controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "resource.h"
#include "colppg.h"
#include "colctrls.h"

/*++

Routine Name:

    CColorPropPage::CColorPropPage

Routine Description:

    CColorPropPage class constructor.
    Creates a handler class object for every control on the color profile property page.
    Each of these handlers is stored in a collection.

Arguments:

    None

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CColorPropPage::CColorPropPage()
{
    HRESULT hr = S_OK;

    try
    {
        CUIControl* pControl = new(std::nothrow) CUICtrlPageColManCombo();
        if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
        {
            hr = AddUIControl(IDC_COMBO_COL_MANAGE, pControl);
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlColProfList();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_LIST_COLPROF, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlPageColIntentCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_COL_INTENT, pControl);
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

    CColorPropPage::~CColorPropPage

Routine Description:

    CColorPropPage class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorPropPage::~CColorPropPage()
{
}

/*++

Routine Name:

    CColorPropPage::InitDlgBox

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
CColorPropPage::InitDlgBox(
    _Out_ LPCTSTR* ppszTemplate,
    _Out_ LPCTSTR* ppszTitle
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppszTemplate, E_POINTER)) ||
        SUCCEEDED(hr = CHECK_POINTER(ppszTitle, E_POINTER)))
    {
        *ppszTemplate = MAKEINTRESOURCE(IDD_COL_MANAGE);
        *ppszTitle    = MAKEINTRESOURCE(IDS_COLMAN);
    }

    ERR_ON_HR(hr);
    return hr;
}

