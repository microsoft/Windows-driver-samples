/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   uictrl.h

Abstract:

   Definition of the UI control interface called by the containing property page,
   the abstract UI control class and the base default UI controls for check boxe,
   list, combo, edit and spin controls.

--*/

#pragma once

#include "UIProperties.h"

class CUIControl
{
public:
    CUIControl();

    virtual ~CUIControl();

    virtual HRESULT
    SetPrintOemDriverUI(
        _In_ CONST IPrintOemDriverUI* pOEMDriverUI
        );

    virtual HRESULT
    SetOemCUIPParam(
        _In_ CONST POEMCUIPPARAM pOemCUIPParam
        );

    virtual HRESULT
    SetUIProperties(
        _In_ CUIProperties* pUIProperties
        );

    //
    // Message handling stubs
    //
    virtual HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        ) = 0;

    virtual HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

    virtual HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

    virtual HRESULT
    OnNotify(
        _In_ CONST HWND hDlg,
        _In_ CONST NMHDR* pNmhdr
        );

protected:
    CComPtr<IPrintOemDriverUI> m_pDriverUIHelp;

    POEMCUIPPARAM              m_pOemCUIPParam;

    CUIProperties *            m_pUIProperties;
};

class CUICtrlDefaultCheck : public CUIControl
{
public:
    CUICtrlDefaultCheck(
        _In_ PCSTR gpdString,
        _In_ INT   iCheckResID
        );

    virtual ~CUICtrlDefaultCheck();

    HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        );

    HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

private:
    virtual HRESULT
    OnBnClicked(
        _In_ CONST HWND hDlg
        );

    virtual HRESULT
    EnableDependentCtrls(
        _In_ CONST HWND hDlg,
        _In_ CONST LONG lSel
        );

    CUICtrlDefaultCheck& operator = (CONST CUICtrlDefaultCheck&);

private:
    PCSTR     m_szGPDString;

    CONST INT m_iCheckResID;
};

class CUICtrlDefaultList : public CUIControl
{
public:
    CUICtrlDefaultList(
        _In_ PCSTR gpdString,
        _In_ INT   iListResID
        );

    virtual ~CUICtrlDefaultList();

    HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        );

    HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

protected:
    HRESULT
    AddString(
        _In_ CONST HWND      hDlg,
        _In_ CONST HINSTANCE hStringResDLL,
        _In_ CONST INT       idString
        );

private:
    virtual HRESULT
    EnableDependentCtrls(
        _In_ CONST HWND hDlg,
        _In_ CONST LONG lSel
        );

    virtual HRESULT
    OnSelChange(
        _In_ CONST HWND hDlg
        );

    CUICtrlDefaultList& operator = (CONST CUICtrlDefaultList&);

private:
    PCSTR     m_szGPDString;

    CONST INT m_iListResID;
};

class CUICtrlDefaultCombo : public CUIControl
{
public:
    CUICtrlDefaultCombo(
        _In_ PCSTR gpdString,
        _In_ INT   iComboResID
        );

    virtual ~CUICtrlDefaultCombo();

    HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        );

    HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

    virtual HRESULT
    OnSelChange(
        _In_ CONST HWND hDlg
        );

protected:
    HRESULT
    AddString(
        _In_ CONST HWND      hDlg,
        _In_ CONST HINSTANCE hStringResDLL,
        _In_ CONST INT       idString
        );

private:
    virtual HRESULT
    EnableDependentCtrls(
        _In_ CONST HWND hDlg,
        _In_ CONST LONG lSel
        );

    CUICtrlDefaultCombo& operator = (CONST CUICtrlDefaultCombo&);

private:
    PCSTR     m_szGPDString;

    CONST INT m_iComboResID;
};

class CUICtrlDefaultSpin;

class CUICtrlDefaultEditNum : public CUIControl
{
public:
    CUICtrlDefaultEditNum(
        _In_ PCSTR     propString,
        _In_ CONST INT iEditResID,
        _In_ CONST INT iPropMin,
        _In_ CONST INT iPropMax,
        _In_ CONST INT iSpinResID
        );

    virtual ~CUICtrlDefaultEditNum();

    friend class CUICtrlDefaultSpin;

    HRESULT
    virtual OnActivate(
        _In_ CONST HWND hDlg
        );

    virtual HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT iCommand
        );

private:
    CUICtrlDefaultEditNum() :
        m_iEditResID(0),
        m_iPropMin(0),
        m_iPropMax(0),
        m_iSpinResID(0)
    {}

    virtual HRESULT
    CheckInRange(
        _In_ PINT pValue
        );

    virtual HRESULT
    OnEnChange(
        _In_ CONST HWND hDlg
        );

    CUICtrlDefaultEditNum& operator = (CONST CUICtrlDefaultEditNum&);


private:
    PCSTR     m_szPropString;

    CONST INT m_iPropMin;

    CONST INT m_iPropMax;

    CONST INT m_iEditResID;

    CONST INT m_iSpinResID;
};

class CUICtrlDefaultSpin : public CUIControl
{

public:
    CUICtrlDefaultSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlDefaultSpin();

    virtual HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        );

    virtual HRESULT
    OnNotify(
        _In_ CONST HWND hDlg,
        _In_ CONST NMHDR* pNmhdr
        );


private:
    virtual HRESULT
    CheckInRange(
        _In_ PINT pValue
        );

    virtual HRESULT
    OnDeltaPos(
        _In_ CONST HWND hDlg,
        _In_ CONST NMUPDOWN* pNmud
        );

    CUICtrlDefaultSpin& operator = (CONST CUICtrlDefaultSpin&);

private:
    PCSTR     m_szPropString;

    INT m_iPropMin;

    INT m_iPropMax;

    INT m_iSpinResID;

    INT m_iEditResID;
};

class CUICtrlDefaultEditText : public CUIControl
{
public:
    CUICtrlDefaultEditText(
        _In_ PCSTR     propString,
        _In_ CONST INT iEditResID,
        _In_ CONST INT cbMaxLength
        );

    virtual ~CUICtrlDefaultEditText();

    HRESULT
    virtual OnActivate(
        _In_ CONST HWND hDlg
        );

    virtual HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

private:
    virtual HRESULT
    OnEnChange(
        _In_ CONST HWND hDlg
        );

    CUICtrlDefaultEditText& operator = (CONST CUICtrlDefaultEditText&);


private:
    PCSTR     m_szPropString;

    CONST INT m_cbMaxLength;

    CONST INT m_iEditResID;
};

class CUICtrlDefaultBtn : public CUIControl
{
public:
    CUICtrlDefaultBtn(
        _In_ PCSTR propertyString,
        _In_ INT   iCheckResID
        );

    virtual ~CUICtrlDefaultBtn();

    HRESULT
    OnActivate(
        _In_ CONST HWND hDlg
        );

    HRESULT
    OnCommand(
        _In_ CONST HWND hDlg,
        _In_ INT        iCommand
        );

private:
    virtual HRESULT
    OnBnClicked(
        _In_ CONST HWND hDlg
        ) = 0;

    CUICtrlDefaultBtn& operator = (CONST CUICtrlDefaultCheck&);

protected:
    PCSTR     m_szPropertyString;

private:
    CONST INT m_iBtnResID;
};

