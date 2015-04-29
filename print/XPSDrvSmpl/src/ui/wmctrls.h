/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmctrls.h

Abstract:

   Definition of the watermark specific UI controls.

--*/

#pragma once

#include "uictrl.h"

class CUICtrlWMTypeCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlWMTypeCombo();

    virtual ~CUICtrlWMTypeCombo();

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
    static PCSTR m_pszWMType;
};

class CUICtrlWMLayeringCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlWMLayeringCombo();

    virtual ~CUICtrlWMLayeringCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszWMLayering;
};

class CUICtrlWMTextEdit : public CUICtrlDefaultEditText
{
public:
    CUICtrlWMTextEdit();

    virtual ~CUICtrlWMTextEdit();

private:
    static PCSTR m_pszWMText;
};

class CUICtrlWMTransparencyEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMTransparencyEdit();

    virtual ~CUICtrlWMTransparencyEdit();

private:
    static PCSTR m_pszWMTransparency;
};

class CUICtrlWMTransparencySpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMTransparencySpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMTransparencySpin();

private:
    static PCSTR m_pszWMTransparency;
};

class CUICtrlWMAngleEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMAngleEdit();

    virtual ~CUICtrlWMAngleEdit();

private:
    static PCSTR m_pszWMAngle;
};

class CUICtrlWMAngleSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMAngleSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMAngleSpin();

private:
    static PCSTR m_pszWMAngle;
};

class CUICtrlWMOffsetXEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMOffsetXEdit();

    virtual ~CUICtrlWMOffsetXEdit();

private:
    static PCSTR m_pszWMOffsetX;
};

class CUICtrlWMOffsetXSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMOffsetXSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMOffsetXSpin();

private:
    static PCSTR m_pszWMOffsetX;
};

class CUICtrlWMOffsetYEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMOffsetYEdit();

    virtual ~CUICtrlWMOffsetYEdit();

private:
    static PCSTR m_pszWMOffsetY;
};

class CUICtrlWMOffsetYSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMOffsetYSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMOffsetYSpin();

private:
    static PCSTR m_pszWMOffsetY;
};

class CUICtrlWMWidthEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMWidthEdit();

    virtual ~CUICtrlWMWidthEdit();

private:
    static PCSTR m_pszWMWidth;
};

class CUICtrlWMWidthSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMWidthSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMWidthSpin();

private:
    static PCSTR m_pszWMWidth;
};

class CUICtrlWMHeightEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMHeightEdit();

    virtual ~CUICtrlWMHeightEdit();

private:
    static PCSTR m_pszWMHeight;
};

class CUICtrlWMHeightSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMHeightSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMHeightSpin();

private:
    static PCSTR m_pszWMHeight;
};

class CUICtrlWMFontSizeEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlWMFontSizeEdit();

    virtual ~CUICtrlWMFontSizeEdit();

private:
    static PCSTR m_pszWMFontSize;
};

class CUICtrlWMFontSizeSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlWMFontSizeSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlWMFontSizeSpin();

private:
    static PCSTR m_pszWMFontSize;
};

class CUICtrlColorBtn : public CUICtrlDefaultBtn
{
public:
    CUICtrlColorBtn();

    ~CUICtrlColorBtn();

    HRESULT
    OnBnClicked(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszWMFontColor;
};

