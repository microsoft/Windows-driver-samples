/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ftrppg.cpp

Abstract:

   Implementation of the feature property page. This class is
   responsible for initialising and registering the features
   property page and its controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "resource.h"
#include "ftrppg.h"
#include "ftrctrls.h"

/*++

Routine Name:

    CFeaturePropPage::CFeaturePropPage

Routine Description:

    CFeaturePropPage class constructor.
    Creates a handler class object for every control on the feature property page.
    Each of these handlers is stored in a collection.

Arguments:

    None

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CFeaturePropPage::CFeaturePropPage()
{
    HRESULT hr = S_OK;

    try
    {
        CUIControl* pControl = new(std::nothrow) CUICtrlFeatPgScaleCombo();
        if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
        {
            hr = AddUIControl(IDC_COMBO_PGSCALE, pControl);
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatScaleOffsetCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_SCALEOFF, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatPgScaleXEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_PGSCALEX, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlFeatPgScaleXSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_PGSCALEX, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatPgOffsetXEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_PGOFFX, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlFeatPgOffsetXSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_PGOFFX, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatPgScaleYEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_PGSCALEY, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlFeatPgScaleYSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_PGSCALEY, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatPgOffsetYEdit();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = AddUIControl(IDC_EDIT_PGOFFY, pControl)))
            {
                pControl = new(std::nothrow) CUICtrlFeatPgOffsetYSpin(reinterpret_cast<CUICtrlDefaultEditNum *>(pControl));
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
                {
                    hr = AddUIControl(IDC_SPIN_PGOFFY, pControl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatNUpCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_NUP, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatNUpOrderCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_NUP_ORDER, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatPhotIntCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_PHOTO_INTENT, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatDocDuplexCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_DOCDUPLEX, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatBordersCheck();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_CHECK_BORDERLESS, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatJobBindCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_JOBBIND, pControl);
            }
        }

        if (SUCCEEDED(hr))
        {
            pControl = new(std::nothrow) CUICtrlFeatDocBindCombo();
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_OUTOFMEMORY)))
            {
                hr = AddUIControl(IDC_COMBO_DOCBIND, pControl);
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

    CFeaturePropPage::~CFeaturePropPage

Routine Description:

    CFeaturePropPage class destructor

Arguments:

    None

Return Value:

    None

--*/
CFeaturePropPage::~CFeaturePropPage()
{
}

/*++

Routine Name:

    CFeaturePropPage::InitDlgBox

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
CFeaturePropPage::InitDlgBox(
    _Out_ LPCTSTR* ppszTemplate,
    _Out_ LPCTSTR* ppszTitle
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppszTemplate, E_POINTER)) ||
        SUCCEEDED(hr = CHECK_POINTER(ppszTitle, E_POINTER)))
    {
        *ppszTemplate = MAKEINTRESOURCE(IDD_FEATURES);
        *ppszTitle    = MAKEINTRESOURCE(IDS_FEATURE);
    }

    ERR_ON_HR(hr);
    return hr;
}

