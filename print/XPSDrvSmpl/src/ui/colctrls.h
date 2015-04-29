/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colctrls.h

Abstract:

   Definition of the color management specific UI controls. These are the
   combo box used to select the PageColorManagement option, a list box
   for selecting the PageSourceColorProfile option and a combo box for
   selecting the PageSourceColorProfile option.

--*/

#pragma once

#include "uictrl.h"

class CUICtrlPageColManCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlPageColManCombo();

    virtual ~CUICtrlPageColManCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    HRESULT
    EnableDependentCtrls(
        _In_ CONST HWND hDlg,
        _In_ CONST LONG lSel
        );

private:
    static PCSTR m_pszPageColManName;
};

class CUICtrlColProfList : public CUICtrlDefaultList
{
public:
    CUICtrlColProfList();

    virtual ~CUICtrlColProfList();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszDestColProf;
};

class CUICtrlPageColIntentCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlPageColIntentCombo();

    virtual ~CUICtrlPageColIntentCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszPageIntentName;
};

