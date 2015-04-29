/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   uictrl.cpp

Abstract:

   Implementation of the abstract UI control class and the base default
   UI controls for check boxe, list, combo, edit and spin controls.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "uictrl.h"

/*++

Routine Name:

    CUIControl::CUIControl

Routine Description:

    CUIControl class constructor

Arguments:

    None

Return Value:

    None

--*/
CUIControl::CUIControl() :
    m_pDriverUIHelp(NULL),
    m_pOemCUIPParam(NULL),
    m_pUIProperties(NULL)
{
}

/*++

Routine Name:

    CUIControl::~CUIControl

Routine Description:

    CUIControl class destructor

Arguments:

    None

Return Value:

    None

--*/
CUIControl::~CUIControl()
{
}

/*++

Routine Name:

    CUIControl::SetOemCUIPParam

Routine Description:

    Store a pointer to a OEMCUIPPARAM structure as a member in the class.

Arguments:

    pOemCUIPParam - Pointer to a OEMCUIPPARAM structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIControl::SetOemCUIPParam(
    _In_ CONST POEMCUIPPARAM pOemCUIPParam
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam, E_POINTER)))
    {
        m_pOemCUIPParam = pOemCUIPParam;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIControl::SetUIProperties

Routine Description:

    Store a pointer to an CUIProperties interface as a member in the class.

Arguments:

    pUIProperties - Pointer to an instance of the CUIProperties interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIControl::SetUIProperties(
    _In_ CUIProperties* pUIProperties
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pUIProperties, E_POINTER)))
    {
        m_pUIProperties = pUIProperties;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIControl::SetPrintOemDriverUI

Routine Description:

    Store a pointer to an IPrintOemDriverUI interface as a member in the class.

Arguments:

    pOEMDriverUI - Pointer to an instance of the IPrintOemDriverUI interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIControl::SetPrintOemDriverUI(
    _In_ CONST IPrintOemDriverUI* pOEMDriverUI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOEMDriverUI, E_POINTER)))
    {
        m_pDriverUIHelp = const_cast<IPrintOemDriverUI*>(pOEMDriverUI);
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Message handling stubs
//
/*++

Routine Name:

    CUIControl::OnInit

Routine Description:

    This is a default implementation that returns S_OK.
    Called from the property page handler on a WM_INITDIALOG message.

Arguments:

    None referenced.

Return Value:

    HRESULT
    S_OK

--*/
HRESULT
CUIControl::OnInit(
    _In_ CONST HWND
    )
{
    //
    // A sub class is required to implement this method if it requires intialisation. The
    // implementation is optional however so we return S_OK by default.
    //
    return S_OK;
}

/*++

Routine Name:

    CUIControl::OnCommand

Routine Description:

    This is a place holder method with no implementation.
    Called from the property page handler on a WM_COMMAND message.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CUIControl::OnCommand(
    _In_ CONST HWND ,
    _In_ INT
    )
{
    //
    // A sub class is required to implement this method if it can recieve command messages. The
    // implementation is optional however so we return S_OK by default.
    //
    return S_OK;
}

/*++

Routine Name:

    CUIControl::OnNotify

Routine Description:

    This is a place holder method with no implementation.
    Called from the property page handler on a WM_NOTIFY message.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CUIControl::OnNotify(
    _In_ CONST HWND,
    _In_ CONST NMHDR*
    )
{
    //
    // A sub class is required to implement this method if it can recieve notify messages. The
    // implementation is optional however so we return S_OK by default.
    //
    return S_OK;
}

//
// Check box control
//
/*++

Routine Name:

    CUICtrlDefaultCheck::CUICtrlDefaultCheck

Routine Description:

    CUICtrlDefaultCheck class constructor

Arguments:

    gpdString - Property name of the data associated with this control.
    iCheckResID - Resource id for the checkbox control.

Return Value:

    None

--*/
CUICtrlDefaultCheck::CUICtrlDefaultCheck(
    _In_ PCSTR gpdString,
    _In_ CONST INT   iCheckResID
    ) :
    m_szGPDString(gpdString),
    m_iCheckResID(iCheckResID)
{
}

/*++

Routine Name:

    CUICtrlDefaultCheck::~CUICtrlDefaultCheck

Routine Description:

    CUICtrlDefaultCheck class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultCheck::~CUICtrlDefaultCheck()
{
}

/*++

Routine Name:

    CUICtrlDefaultCheck::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the Unidrv GPD settings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCheck::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr   = S_OK;
    POPTITEM pOptItem = NULL;

    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
    {

        //
        // Convert from a selection to a checkbox state
        // and initialise the controls state.
        //

        LONG lSel = pOptItem->Sel;

        UINT uChecked = (lSel == 0) ? BST_UNCHECKED : BST_CHECKED;

        if (CheckDlgButton(hDlg, m_iCheckResID, uChecked) > 0)
        {
            hr = EnableDependentCtrls(hDlg, lSel);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCheck::OnCommand

Routine Description:

    Called from the property page handler when a WM_COMMAND message is recieved.
    Filters out any button click (BN_CLICKED) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultCheck::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case BN_CLICKED:
        {
            hr = OnBnClicked(hDlg);
        }
        break;

        default:
            hr = E_NOTIMPL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCheck::OnBnClicked

Routine Description:

    This rountine handles the event of a button press. The state of the check box control is read which
    can be either checked and unchecked, the result is communicated through the Unidrv helper functions
    informing the Unidrv core that a change has occured.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCheck::OnBnClicked(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");
    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pOemCUIPParam->poemuiobj != NULL, "NULL pointer to OEMCUIPPARAM->poemuiobj structure.\n");
    ASSERTMSG(m_pDriverUIHelp != NULL, "NULL pointer to driver UI help interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam->poemuiobj, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pDriverUIHelp, E_PENDING)))
    {
        //
        // Retrieve the controls current selection
        // and convert from checkbox state to a selection
        //
        UINT uChecked = IsDlgButtonChecked(hDlg, m_iCheckResID);

        LONG lSel = (uChecked == BST_UNCHECKED) ? 0 : 1;

        //
        // Check against the optitem
        //
        POPTITEM pOptItem = NULL;

        if (SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
            SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
        {
            if (pOptItem->Sel != lSel)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);

                pOptItem->Sel = lSel;
                pOptItem->Flags |= OPTIF_CHANGED;

                if (SUCCEEDED(hr = m_pDriverUIHelp->DrvUpdateUISetting(m_pOemCUIPParam->poemuiobj, pOptItem, 0, OEMCUIP_DOCPROP)))
                {
                    hr = EnableDependentCtrls(hDlg, lSel);
                }
            }
            else
            {
                PropSheet_UnChanged(GetParent(hDlg), hDlg);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCheck::EnableDependentCtrls

Routine Description:

    This is a default implementation simply returns S_OK

Arguments:

    None referenced.

Return Value:

    HRESULT
    S_OK - Success

--*/
HRESULT
CUICtrlDefaultCheck::EnableDependentCtrls(
    _In_ CONST HWND,
    _In_ CONST LONG
    )
{
    return S_OK;
}


//
// List box control
//
/*++

Routine Name:

    CUICtrlDefaultList::CUICtrlDefaultList

Routine Description:

    CUICtrlDefaultList class construtor

Arguments:

    gpdString - Property name of the data associated with this control.
    iListResID - Resource id for the List Box control.

Return Value:

    None

--*/
CUICtrlDefaultList::CUICtrlDefaultList(
    _In_ PCSTR     gpdString,
    _In_ CONST INT iListResID
    ) :
    m_szGPDString(gpdString),
    m_iListResID(iListResID)
{
}

/*++

Routine Name:

    CUICtrlDefaultList::~CUICtrlDefaultList

Routine Description:

    CUICtrlDefaultList class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultList::~CUICtrlDefaultList()
{
}

/*++

Routine Name:

    CUICtrlDefaultList::AddString

Routine Description:

    Loads a string from the specified resource and adds it to the end of the list box.

Arguments:

    hDlg - handle to the parent window
    hStringResDLL - handle of the resource DLL that contains the string table.
    idString - identifer of the string resource to be added

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultList::AddString(
    _In_ CONST HWND      hDlg,
    _In_ CONST HINSTANCE hStringResDLL,
    _In_ CONST INT       idString
    )
{
    HRESULT hr = S_OK;

    TCHAR szItem[MAX_UISTRING_LEN];
    if (LoadString(hStringResDLL, idString, szItem, countof(szItem)) > 0)
    {
        LRESULT lResult = SendDlgItemMessage(hDlg, m_iListResID, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szItem));

        if (lResult == LB_ERRSPACE ||
            lResult == LB_ERR)
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultList::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the Unidrv GPD settings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultList::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr   = S_OK;
    POPTITEM pOptItem = NULL;

    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
    {
        LONG lSel = pOptItem->Sel;
        if (SendDlgItemMessage(hDlg, m_iListResID, LB_SETCURSEL, lSel, 0) != LB_ERR)
        {
            hr = EnableDependentCtrls(hDlg, lSel);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultList::OnCommand

Routine Description:

    Called from the property page handler when a WM_COMMAND message is recieved.
    Filters out any list box selection change (LBN_SELCHANGE) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultList::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT        iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case LBN_SELCHANGE:
        {
            hr = OnSelChange(hDlg);
        }
        break;

        default:
            hr = E_NOTIMPL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultList::OnSelChange

Routine Description:

    This rountine handles the event of a change of selection in the list box. The selection of the
    list box control is read and the result is communicated through the Unidrv helper functions
    informing the Unidrv core that a change has occured.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultList::OnSelChange(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");
    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pOemCUIPParam->poemuiobj != NULL, "NULL pointer to OEMCUIPPARAM->poemuiobj structure.\n");
    ASSERTMSG(m_pDriverUIHelp != NULL, "NULL pointer to driver UI help interface.\n");


    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam->poemuiobj, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pDriverUIHelp, E_PENDING)))
    {
        //
        // Retrieve the controls current selection
        //
        LONG lSel = static_cast<LONG>(SendDlgItemMessage(hDlg, m_iListResID, LB_GETCURSEL, 0, 0));

        if (lSel != LB_ERR)
        {
            //
            // Check against the optitem
            //
            POPTITEM pOptItem = NULL;

            if (SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
                SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
            {
                if (pOptItem->Sel != lSel)
                {
                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    pOptItem->Sel = lSel;
                    pOptItem->Flags |= OPTIF_CHANGED;

                    if (SUCCEEDED(hr = m_pDriverUIHelp->DrvUpdateUISetting(m_pOemCUIPParam->poemuiobj, pOptItem, 0, OEMCUIP_DOCPROP)))
                    {
                        hr = EnableDependentCtrls(hDlg, lSel);
                    }
                }
                else
                {
                    PropSheet_UnChanged(GetParent(hDlg), hDlg);
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultList::EnableDependentCtrls

Routine Description:

    This is a default implementation that returns S_OK.
    Called when a selection is changed in the list box to show/enable any dependent controls.

Arguments:

    None referenced.

Return Value:

    HRESULT
    S_OK - Success

--*/
HRESULT
CUICtrlDefaultList::EnableDependentCtrls(
    _In_ CONST HWND,
    _In_ CONST LONG
    )
{
    return S_OK;
}

//
// Combo box control
//
/*++

Routine Name:

    CUICtrlDefaultCombo::CUICtrlDefaultCombo

Routine Description:

    CUICtrlDefaultCombo class constructor.

Arguments:

    gpdString - Property name of the data associated with this control.
    iComboResID - Resource id for the Combo Box control.

Return Value:

    None

--*/
CUICtrlDefaultCombo::CUICtrlDefaultCombo(
    _In_ PCSTR     gpdString,
    _In_ CONST INT iComboResID
    ) :
    m_szGPDString(gpdString),
    m_iComboResID(iComboResID)
{
}

/*++

Routine Name:

    CUICtrlDefaultCombo::~CUICtrlDefaultCombo

Routine Description:

    CUICtrlDefaultCombo class destructor.

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultCombo::~CUICtrlDefaultCombo()
{
}

/*++

Routine Name:

    CUICtrlDefaultCombo::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the Unidrv GPD settings.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCombo::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT  hr       = S_OK;
    POPTITEM pOptItem = NULL;

    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
    {
        LONG lSel = pOptItem->Sel;

        LRESULT lResult = SendDlgItemMessage(hDlg, m_iComboResID, CB_SETCURSEL, lSel, 0);

        if (lResult != CB_ERR &&
            lResult != CB_ERRSPACE)
        {
            hr = EnableDependentCtrls(hDlg, lSel);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCombo::OnCommand

Routine Description:

    Called from the property page handler when a WM_COMMAND message is recieved.
    Filters out any Combo Box selection change (CBN_SELCHANGE) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultCombo::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case CBN_SELCHANGE:
        {
            hr = OnSelChange(hDlg);
        }
        break;

        default:
        {
            hr = E_NOTIMPL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCombo::OnSelChange

Routine Description:

    This rountine handles the event of a change of selection in the combo box. The selection of the
    combo box control is read and the result is communicated through the Unidrv helper functions
    informing the Unidrv core that a change has occured.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCombo::OnSelChange(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");
    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pOemCUIPParam->poemuiobj != NULL, "NULL pointer to OEMCUIPPARAM->poemuiobj structure.\n");
    ASSERTMSG(m_pDriverUIHelp != NULL, "NULL pointer to driver UI help interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam->poemuiobj, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pDriverUIHelp, E_PENDING)))
    {
        //
        // Retrieve the controls current selection
        //

        LONG lSel = static_cast<LONG>(SendDlgItemMessage(hDlg, m_iComboResID, CB_GETCURSEL, 0, 0));

        if (lSel == CB_ERR)
        {
            hr = E_FAIL;
        }

        POPTITEM pOptItem = NULL;

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = m_pUIProperties->GetOptItem(m_pOemCUIPParam, m_szGPDString, &pOptItem)) &&
            SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
        {
            if (pOptItem->Sel != lSel)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);

                pOptItem->Sel = lSel;
                pOptItem->Flags |= OPTIF_CHANGED;

                if (SUCCEEDED(hr = m_pDriverUIHelp->DrvUpdateUISetting(m_pOemCUIPParam->poemuiobj, pOptItem, 0, OEMCUIP_DOCPROP)))
                {
                    hr = EnableDependentCtrls(hDlg, lSel);
                }
            }
            else
            {
                PropSheet_UnChanged(GetParent(hDlg), hDlg);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultCombo::EnableDependentCtrls

Routine Description:

    This is a default implementation that returns S_OK.
    Called when a selection is changed in the combo box to show/enable any dependent controls.

Arguments:

    None referenced

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCombo::EnableDependentCtrls(
    _In_ CONST HWND,
    _In_ CONST LONG
    )
{
    return S_OK;
}

/*++

Routine Name:

    CUICtrlDefaultCombo::AddString

Routine Description:

    Loads a string from the specified resource and adds it to the end of the Combo Box.

Arguments:

    hDlg - handle to the parent window
    hStringResDLL - handle of the resource DLL that contains the string table.
    idString - identifer of the string resource to be added

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultCombo::AddString(
    _In_ CONST HWND      hDlg,
    _In_ CONST HINSTANCE hStringResDLL,
    _In_ CONST INT       idString
    )
{
    HRESULT hr = S_OK;

    TCHAR szItem[MAX_UISTRING_LEN];
    if (LoadString(hStringResDLL, idString, szItem, countof(szItem)) > 0)
    {
        LRESULT lResult = SendDlgItemMessage(hDlg, m_iComboResID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szItem));

        if (lResult == CB_ERRSPACE ||
            lResult == CB_ERR)
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// EditNum control
//
/*++

Routine Name:

    CUICtrlDefaultEditNum::CUICtrlDefaultEditNum

Routine Description:

    CUICtrlDefaultEditNum class constructor

Arguments:

    gpdString - Property name of the data associated with this control.
    iEditResID - Resource id for the Combo Box control.
    iPropMin - Minimum integer value that is valid.
    iPropMax - Maximum integer value that is valid.
    iSpinResID - Resource id for the associated Up/Down spinner control.

Return Value:

    None

--*/
CUICtrlDefaultEditNum::CUICtrlDefaultEditNum(
    _In_ PCSTR     propString,
    _In_ CONST INT iEditResID,
    _In_ CONST INT iPropMin,
    _In_ CONST INT iPropMax,
    _In_ CONST INT iSpinResID
    ) :
    m_szPropString(propString),
    m_iEditResID(iEditResID),
    m_iPropMin(iPropMin),
    m_iPropMax(iPropMax),
    m_iSpinResID(iSpinResID)

{
}

/*++

Routine Name:

    CUICtrlDefaultEditNum::~CUICtrlDefaultEditNum

Routine Description:

    CUICtrlDefaultEditNum class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultEditNum::~CUICtrlDefaultEditNum()
{
}

/*++

Routine Name:

    CUICtrlDefaultEditNum::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the OEM private devmode.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultEditNum::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)))
    {
        INT iPos = 0;
        TCHAR szItem[MAX_UISTRING_LEN];

        if (SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iPos), sizeof(iPos))) &&
            SUCCEEDED(hr = StringCchPrintf(szItem, MAX_UISTRING_LEN, TEXT("%d"), iPos)))
        {
            if (SetDlgItemText(hDlg, m_iEditResID, reinterpret_cast<LPCTSTR>(szItem)) == 0)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultEditNum::OnCommand

Routine Description:

    Called from the property page handler when a WM_COMMAND message is recieved.
    Filters out any Edit Text change (EN_CHANGE) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultEditNum::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case EN_CHANGE:
        {
            hr = OnEnChange(hDlg);
        }
        break;

        default:
            hr = E_NOTIMPL;

    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultEditNum::CheckInRange

Routine Description:

    Ensures that a value is within range of the controls min/max extents.

Arguments:

    pValue - pointer to the integer value to check.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultEditNum::CheckInRange(
    _In_ PINT pValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (*pValue > m_iPropMax)
        {
            *pValue = m_iPropMax;
        }

        if (*pValue < m_iPropMin)
        {
            *pValue = m_iPropMin;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultEditNum::OnEnChange

Routine Description:

    This rountine handles the event of a change in the text box. The text in the control is read then
    validated and the result is stored in the OEM private devmode. In addition the buddy spinner
    control is updated to reflect the change in value.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultEditNum::OnEnChange(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    BOOL bTranslated;

    INT iPos = static_cast<INT>(GetDlgItemInt(hDlg, m_iEditResID, &bTranslated, TRUE));

    if (bTranslated)
    {
        INT iOrgPos = 0;

        if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
            SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iOrgPos), sizeof(iOrgPos))) &&
            SUCCEEDED(hr = CheckInRange(&iPos)) &&
            iOrgPos != iPos)
        {
            SendDlgItemMessage(hDlg, m_iSpinResID, UDM_SETPOS32, 0, static_cast<LPARAM>(iPos));

            if (SUCCEEDED(hr = m_pUIProperties->SetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iPos), sizeof(iPos))))
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Edit control
//
/*++

Routine Name:

    CUICtrlDefaultSpin::CUICtrlDefaultSpin

Routine Description:

    CUICtrlDefaultSpin class constructor.

    Requires a Text Box buddy control class.
    Settings for this Spin control are taken from the buddy class.

Arguments:

    pEdit - Pointer to the buddy Text Box control.

Return Value:

    None

--*/
CUICtrlDefaultSpin::CUICtrlDefaultSpin(
    _In_ CUICtrlDefaultEditNum * pEdit
    ):
        m_szPropString(0),
        m_iSpinResID(0),
        m_iPropMin(0),
        m_iPropMax(0),
        m_iEditResID(0)
{
    if (pEdit !=NULL)
    {
        m_szPropString = pEdit->m_szPropString;
        m_iSpinResID   = pEdit->m_iSpinResID;
        m_iPropMin     = pEdit->m_iPropMin;
        m_iPropMax     = pEdit->m_iPropMax;
        m_iEditResID   = pEdit->m_iEditResID;
    }
}

/*++

Routine Name:

    CUICtrlDefaultSpin::~CUICtrlDefaultSpin

Routine Description:

    CUICtrlDefaultSpin class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultSpin::~CUICtrlDefaultSpin()
{
}

/*++

Routine Name:

    CUICtrlDefaultSpin::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the OEM private devmode.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultSpin::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr   = S_OK;
    INT     iPos = 0;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iPos), sizeof(iPos))))
    {
        SendDlgItemMessage(hDlg, m_iSpinResID, UDM_SETRANGE32, m_iPropMin, m_iPropMax);
        SendDlgItemMessage(hDlg, m_iSpinResID, UDM_SETPOS32, 0, iPos);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultSpin::OnNotify

Routine Description:

    Called from the property page handler when a WM_NOTIFY message is recieved.
    Filters out any Up/Down position (UDN_DELTAPOS) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    pNmhdr - pointer to the notification message

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultSpin::OnNotify(
    _In_ CONST HWND hDlg,
    _In_ CONST NMHDR* pNmhdr
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNmhdr, E_POINTER)))
    {
        switch (pNmhdr->code)
        {
            case UDN_DELTAPOS:
            {
                hr = OnDeltaPos(hDlg, reinterpret_cast<CONST NMUPDOWN*>(pNmhdr));
            }
            break;

            default:
                hr = E_NOTIMPL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultSpin::CheckInRange

Routine Description:

    Ensures that a value is within range of the controls min/max extents.

Arguments:

    pValue - Pointer to the integer value to check.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultSpin::CheckInRange(
    _In_ PINT pValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (*pValue > m_iPropMax)
        {
            *pValue = m_iPropMax;
        }

        if (*pValue < m_iPropMin)
        {
            *pValue = m_iPropMin;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultSpin::OnDeltaPos

Routine Description:

    This rountine handles the event of a change in value of the Up/Down control. The new value
    of the control is validated and stored in the OEM private devmode. In addition the buddy text
    box control is updated to reflect the change.

Arguments:

    hDlg - handle to the parent window
    pNmud - this structure contains information specific to up-down control messages

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultSpin::OnDeltaPos(
    _In_ CONST HWND hDlg,
    _In_ CONST NMUPDOWN* pNmud
    )
{
    HRESULT hr      = S_OK;
    INT     iOrgPos = 0;
    INT     iPos    = 0;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(pNmud, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
        SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iOrgPos), sizeof(iOrgPos))))
    {
        iPos = iOrgPos + pNmud->iDelta;
        TCHAR szItem[MAX_UISTRING_LEN];

        if (SUCCEEDED(hr = CheckInRange(&iPos)) &&
            iOrgPos != iPos &&
            SUCCEEDED(hr = StringCchPrintf(szItem, MAX_UISTRING_LEN, TEXT("%d"), iPos)))
        {
            if (SetDlgItemText(hDlg, m_iEditResID, reinterpret_cast<LPCTSTR>(szItem)) > 0)
            {
                if (SUCCEEDED(hr = m_pUIProperties->SetItem(m_szPropString, reinterpret_cast<UIProperty*>(&iPos), sizeof(iPos))))
                {
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// EditText control
//
/*++

Routine Name:

    CUICtrlDefaultEditText::CUICtrlDefaultEditText

Routine Description:

    CUICtrlDefaultEditText class constructor

Arguments:

    gpdString - Property name of the data associated with this control.
    iEditResID - Resource id for the Combo Box control.
    cbMaxLength - Maximum number of characters allowed.

Return Value:

    None

--*/
CUICtrlDefaultEditText::CUICtrlDefaultEditText(
    _In_ PCSTR     propString,
    _In_ CONST INT iEditResID,
    _In_ CONST INT cbMaxLength
    ) :
    m_szPropString(propString),
    m_iEditResID(iEditResID),
    m_cbMaxLength(cbMaxLength)

{
}

/*++

Routine Name:

    CUICtrlDefaultEditText::~CUICtrlDefaultEditText

Routine Description:

    CUICtrlDefaultEditText class destructor

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultEditText::~CUICtrlDefaultEditText()
{
}

/*++

Routine Name:

    CUICtrlDefaultEditText::OnActivate

Routine Description:

    Called when the parent property page becomes active.
    This method initialises the state of the control to reflect the OEM private devmode.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultEditText::OnActivate(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)))
    {
        SIZE_T  cbBuffer = sizeof(TCHAR) * m_cbMaxLength;
        LPTSTR lpBuffer  = new(std::nothrow) TCHAR[m_cbMaxLength];

        if (SUCCEEDED(hr = CHECK_POINTER(lpBuffer, E_OUTOFMEMORY)))
        {
            if (SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(lpBuffer), cbBuffer)) &&
                SUCCEEDED(hr = CHECK_POINTER(lpBuffer, E_FAIL)))
            {
                if (SetDlgItemText(hDlg, m_iEditResID, reinterpret_cast<LPCTSTR>(lpBuffer)) == 0)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }

            delete[] lpBuffer;
            lpBuffer = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        SendDlgItemMessage(hDlg, m_iEditResID, EM_LIMITTEXT, m_cbMaxLength - 1, 0L);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultEditText::OnCommand

Routine Description:

    Called from the property page handler when a WM_COMMAND message is recieved.
    Filters out any Edit Text change (EN_CHANGE) messages intended for this control.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK      - On success
    E_NOTIMPL - Command not implemented
    E_*       - On error

--*/
HRESULT
CUICtrlDefaultEditText::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT        iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case EN_CHANGE:
        {
            hr = OnEnChange(hDlg);
        }
        break;

        default:
            hr = E_NOTIMPL;

    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CUICtrlDefaultEditNum::OnEnChange

Routine Description:

    This rountine handles the event of a change in the text box.
    The text in the control is read and the result is stored in the OEM private devmode.

Arguments:

    hDlg - handle to the parent window

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultEditText::OnEnChange(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to UI properties interface.\n");

    SIZE_T  cbBuffer = sizeof(TCHAR) * m_cbMaxLength;
    LPTSTR lpBuffer = new(std::nothrow) TCHAR[m_cbMaxLength];

    if (SUCCEEDED(hr = CHECK_POINTER(lpBuffer, E_OUTOFMEMORY)))
    {
        if (GetDlgItemText(hDlg, m_iEditResID, lpBuffer, m_cbMaxLength) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            LPTSTR lpOrgBuffer = new(std::nothrow) TCHAR[m_cbMaxLength];

            if (SUCCEEDED(hr = CHECK_POINTER(lpOrgBuffer, E_OUTOFMEMORY)))
            {
                if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)) &&
                    SUCCEEDED(hr = m_pUIProperties->GetItem(m_szPropString, reinterpret_cast<UIProperty*>(lpOrgBuffer), cbBuffer)) &&
                    SUCCEEDED(hr = CHECK_POINTER(lpOrgBuffer, E_FAIL)) &&
                    wcsncmp(lpBuffer, lpOrgBuffer, m_cbMaxLength) != 0 &&
                    SUCCEEDED(hr = m_pUIProperties->SetItem(m_szPropString, reinterpret_cast<UIProperty*>(lpBuffer), cbBuffer)))
                {
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }

                delete[] lpOrgBuffer;
                lpOrgBuffer = NULL;
            }
        }

        delete[] lpBuffer;
        lpBuffer = NULL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUICtrlDefaultBtn::CUICtrlDefaultBtn

Routine Description:

    CUICtrlDefaultBtn class constructor.

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultBtn::CUICtrlDefaultBtn(
    _In_ PCSTR propertyString,
    _In_ INT   iBtnResID
    ) :
    m_szPropertyString(propertyString),
    m_iBtnResID(iBtnResID)
{
}

/*++

Routine Name:

    CUICtrlDefaultBtn::CUICtrlDefaultBtn

Routine Description:

    CUICtrlDefaultBtn class constructor.

Arguments:

    None

Return Value:

    None

--*/
CUICtrlDefaultBtn::~CUICtrlDefaultBtn()
{
}

/*++

Routine Name:

    CUICtrlDefaultBtn::CUICtrlDefaultBtn

Routine Description:

    Default button OnActivate handler.

Arguments:

    None

Return Value:

    S_OK

--*/
HRESULT
CUICtrlDefaultBtn::OnActivate(
    _In_ CONST HWND
    )
{
    return S_OK;
}

/*++

Routine Name:

    CUICtrlDefaultBtn::OnCommand

Routine Description:

    Default button OnCommand handler.

Arguments:

    hDlg - handle to the parent window
    iCommand - specifies the notification code

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUICtrlDefaultBtn::OnCommand(
    _In_ CONST HWND hDlg,
    _In_ INT        iCommand
    )
{
    HRESULT hr = S_OK;

    switch (iCommand)
    {
        case BN_CLICKED:
        {
            hr = OnBnClicked(hDlg);
        }
        break;

        default:
            hr = E_NOTIMPL;
    }

    ERR_ON_HR(hr);
    return hr;
}

