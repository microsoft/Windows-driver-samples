/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   docppg.h

Abstract:

   Definition of the document property page class. This is an abstract
   class that provides common functionality for all property pages including
   accessors and the dialog proc.

--*/

#pragma once

#include "uictrl.h"

#define ID_XDSMPL_DLG_ATOM 1000

typedef std::map<INT_PTR, CUIControl*> UIControlMap;

class CDocPropPage
{
public:
    CDocPropPage();

    virtual ~CDocPropPage();

    virtual HRESULT
    InitDlgBox(
        _Out_ LPCTSTR* ppszTemplate,
        _Out_ LPCTSTR* ppszTitle
        ) = 0;

    //
    // Implementation
    //
    virtual HRESULT
    PropPageInit(
        _In_ CONST PPROPSHEETUI_INFO pPSUIInfo
        );

    virtual HRESULT
    SetPrintOemDriverUI(
        _In_ CONST IPrintOemDriverUI* pOEMDriverUI
        );

    virtual HRESULT
    SetOemCUIPParam(
        _In_ CONST POEMCUIPPARAM pOemCUIParam
        );

    virtual HRESULT
    SetUIProperties(
        _In_ CUIProperties* pUIProperties
    );

    static INT_PTR CALLBACK
    DlgProc(
        _In_ HWND   hDlg,
        _In_ UINT   uiMessage,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
        );

protected:
    HRESULT
    StoreThis(
        _In_ CONST HWND hDlg
        );

    static HRESULT
    RetrieveThis(
        _In_        CONST HWND     hDlg,
        _Outptr_ CDocPropPage** pDocPropPage
        );

    HRESULT
    RemoveThis(
        _In_ CONST HWND hDlg
        );

    HRESULT
    SetComPropSheetFunc(
        _In_ CONST PFNCOMPROPSHEET pfnComPropSheet
        );

    HRESULT
    SetPageHandle(
        _In_ CONST HANDLE hPage
        );

    HRESULT
    SetComPropSheetHandle(
        _In_ CONST HANDLE hComPropSheet
        );

    HRESULT
    GetComPropSheetFunc(
        _Outptr_ PFNCOMPROPSHEET* ppfnComPropSheet
        );

    HANDLE
    GetPageHandle(
        VOID
        );

    HANDLE
    GetComPropSheetHandle(
        VOID
        );

    HRESULT
    AddUIControl(
        _In_ CONST INT iCtrlID,
        _In_ CUIControl* pUIControl
        );

    HRESULT
    GetUIComponents(
        _Outptr_ UIControlMap** ppUIComponents
        );

    HRESULT
    GetOemCUIPParam(
        _Outptr_ POEMCUIPPARAM* ppOemCUIPParam
        );

    HRESULT
    PublishHelpToControls(
        VOID
        );

    HRESULT
    SendCommand(
        _In_ CONST HWND   hDlg,
        _In_ CONST WPARAM wParam
        );

    HRESULT
    SendSetActive(
        _In_ CONST HWND   hDlg
        );

    HRESULT
    SendInit(
        _In_ CONST HWND   hDlg
        );

    HRESULT
    SendNotify(
        _In_ CONST HWND   hDlg,
        _In_ CONST NMHDR* pNMhdr
        );

    HRESULT
    DestroyUIComponents(
        VOID
        );

private:
    UIControlMap                 m_UIControls;

    CComPtr<IPrintOemDriverUI>   m_pDriverUIHelp;

    HANDLE                       m_hPage;

    HANDLE                       m_hComPropSheet;

    PFNCOMPROPSHEET              m_pfnComPropSheet;

    POEMCUIPPARAM                m_pOemCUIPParam;

    CUIProperties*               m_pUIProperties;
};

