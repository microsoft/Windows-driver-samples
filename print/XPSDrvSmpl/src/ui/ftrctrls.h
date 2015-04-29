/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ftrctrls.h

Abstract:

   Definition of the features property page UI controls.

--*/

#pragma once

#include "uictrl.h"

class CUICtrlFeatPgScaleCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatPgScaleCombo();

    virtual ~CUICtrlFeatPgScaleCombo();

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
    static PCSTR m_pszFeatPgScale;
};

class CUICtrlFeatScaleOffsetCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatScaleOffsetCombo();

    virtual ~CUICtrlFeatScaleOffsetCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszFeatScaleOffset;
};

class CUICtrlFeatPgScaleXEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlFeatPgScaleXEdit();

    virtual ~CUICtrlFeatPgScaleXEdit();

private:
    static PCSTR m_pszFeatPgScaleX;
};

class CUICtrlFeatPgScaleXSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlFeatPgScaleXSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlFeatPgScaleXSpin();

private:
    static PCSTR m_pszFeatPgScaleX;
};

class CUICtrlFeatPgOffsetXEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlFeatPgOffsetXEdit();

    virtual ~CUICtrlFeatPgOffsetXEdit();

private:
    static PCSTR m_pszFeatPgOffsetX;
};

class CUICtrlFeatPgOffsetXSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlFeatPgOffsetXSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlFeatPgOffsetXSpin();

private:
    static PCSTR m_pszFeatPgOffsetX;
};

class CUICtrlFeatPgScaleYEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlFeatPgScaleYEdit();

    virtual ~CUICtrlFeatPgScaleYEdit();

private:
    static PCSTR m_pszFeatPgScaleY;
};

class CUICtrlFeatPgScaleYSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlFeatPgScaleYSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlFeatPgScaleYSpin();

private:
    static PCSTR m_pszFeatPgScaleY;
};

class CUICtrlFeatPgOffsetYEdit : public CUICtrlDefaultEditNum
{
public:
    CUICtrlFeatPgOffsetYEdit();

    virtual ~CUICtrlFeatPgOffsetYEdit();

private:
    static PCSTR m_pszFeatPgOffsetY;
};

class CUICtrlFeatPgOffsetYSpin : public CUICtrlDefaultSpin
{
public:
    CUICtrlFeatPgOffsetYSpin(
        _In_ CUICtrlDefaultEditNum* pEdit
        );

    virtual ~CUICtrlFeatPgOffsetYSpin();

private:
    static PCSTR m_pszFeatPgOffsetY;
};

class CUICtrlFeatNUpCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatNUpCombo();

    virtual ~CUICtrlFeatNUpCombo();

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
    static PCSTR m_pszFeatNUp;
};

class CUICtrlFeatNUpOrderCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatNUpOrderCombo();

    virtual ~CUICtrlFeatNUpOrderCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszFeatNUpOrder;
};

class CUICtrlFeatJobBindCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatJobBindCombo();

    virtual ~CUICtrlFeatJobBindCombo();

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
    static PCSTR m_pszFeatJobBind;
};

class CUICtrlFeatDocBindCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatDocBindCombo();

    virtual ~CUICtrlFeatDocBindCombo();

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
    static PCSTR m_pszFeatDocBind;
};

class CUICtrlFeatPhotIntCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatPhotIntCombo();

    virtual ~CUICtrlFeatPhotIntCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszFeatPhotInt;
};

class CUICtrlFeatDocDuplexCombo : public CUICtrlDefaultCombo
{
public:
    CUICtrlFeatDocDuplexCombo();

    virtual ~CUICtrlFeatDocDuplexCombo();

    HRESULT
    OnInit(
        _In_ CONST HWND hDlg
        );

private:
    static PCSTR m_pszFeatDocDuplex;
};

class CUICtrlFeatBordersCheck : public CUICtrlDefaultCheck
{
public:
    CUICtrlFeatBordersCheck();

    virtual ~CUICtrlFeatBordersCheck();

private:
    static PCSTR m_pszFeatBorders;
};

