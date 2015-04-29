/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   docppg.cpp

Abstract:

   Implementation of the document property page class. This is an abstract
   class that provides common functionality for all property pages including
   accessors and the dialog proc.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "docppg.h"

/*++

Routine Name:

    CDocPropPage::CDocPropPage

Routine Description:

    CDocPropPage class constructor

Arguments:

    None

Return Value:

    None

--*/
CDocPropPage::CDocPropPage() :
    m_hPage(NULL),
    m_hComPropSheet(NULL),
    m_pOemCUIPParam(NULL),
    m_pfnComPropSheet(NULL),
    m_pDriverUIHelp(NULL),
    m_pUIProperties(NULL)
{
}

/*++

Routine Name:

    CDocPropPage::~CDocPropPage

Routine Description:

    CDocPropPage class destructor.

Arguments:

    None

Return Value:

    None

--*/
CDocPropPage::~CDocPropPage()
{
    HRESULT hr = S_OK;
    hr = DestroyUIComponents();

    ASSERTMSG(SUCCEEDED(hr), "Error Deleting Property Pages/n");
}

/*++

Routine Name:

    CDocPropPage::PropPageInit

Routine Description:

    Adds an additional Feature property page to the existing Unidrv supplied property pages.
    Called from the Unidrv UI Plug-in entry point for DocumentPropertySheets(),
    on the PROPSHEETUI_REASON_INIT message.

    This base class implementation provides common property page intialisation functionality for
    all property pages. Derived property page classes are required to provide the dialog box template
    and the dialog box title.

Arguments:

    pPSUIInfo - Pointer to a PPROPSHEETUI_INFO structure.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::PropPageInit(
    _In_ CONST PPROPSHEETUI_INFO pPSUIInfo
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPSUIInfo, E_POINTER)))
    {
        //
        // Only proceed if we have effectively published the helper interfaces to the UI
        // control objects.
        //
        // We also need to retrieve the dialog template resource and dialog title from the
        // derived property page class.
        //
        PROPSHEETPAGE page = {0};
        if (SUCCEEDED(hr = PublishHelpToControls()) &&
            SUCCEEDED(hr = InitDlgBox(&page.pszTemplate, &page.pszTitle)))
        {
            page.dwSize = sizeof(PROPSHEETPAGE);
            page.dwFlags = PSP_DEFAULT | PSP_USETITLE;
            page.hInstance = g_hInstance;

            page.pfnDlgProc = CDocPropPage::DlgProc;
            page.lParam = reinterpret_cast<LPARAM>(this);

            pPSUIInfo->Result = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                           CPSFUNC_ADD_PROPSHEETPAGE,
                                                           reinterpret_cast<LPARAM>(&page),
                                                           0);

            if (SUCCEEDED(hr = SetComPropSheetFunc(pPSUIInfo->pfnComPropSheet)) &&
                SUCCEEDED(hr = SetPageHandle(reinterpret_cast<HANDLE>(pPSUIInfo->Result))))
            {
                hr = SetComPropSheetHandle(pPSUIInfo->hComPropSheet);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::StoreThis

Routine Description:

    Stores a pointer to this instance of the CDocPropPage class
    that will be associated with the windows handle provided.

Arguments:

    hDlg - Handle of the property page window associated with this class.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::StoreThis(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    if (!SetProp(hDlg, MAKEINTATOM(ID_XDSMPL_DLG_ATOM), reinterpret_cast<HANDLE>(this)))
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::RetrieveThis

Routine Description:

    Obtains the instance of the CDocPropPage class that is associated with a windows handle.

Arguments:

    hDlg - Handle of the property page Window associated with this class.
    pDocPropPage - Address of a pointer to be filled out with the instance of this class.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/

HRESULT
CDocPropPage::RetrieveThis(
    _In_        CONST HWND     hDlg,
    _Outptr_ CDocPropPage** pDocPropPage
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_HANDLE(hDlg, E_HANDLE)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDocPropPage, E_POINTER)))
    {
        *pDocPropPage = reinterpret_cast<CDocPropPage*>(GetProp(hDlg, MAKEINTATOM(ID_XDSMPL_DLG_ATOM)));
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::RemoveThis

Routine Description:

    Removes the class pointer from the associated windows handle.

Arguments:

    hDlg - Handle of the property page Window associated with this class.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::RemoveThis(
    _In_ CONST HWND hDlg
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_HANDLE(hDlg, E_HANDLE)))
    {
        RemoveProp(hDlg, MAKEINTATOM(ID_XDSMPL_DLG_ATOM));
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SetComPropSheetFunc

Routine Description:

    Store the pointer to the PFNCOMPROPSHEET function.

Arguments:

    pfnComPropSheet - pointer to the PFNCOMPROPSHEET function.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetComPropSheetFunc(
    _In_ CONST PFNCOMPROPSHEET pfnComPropSheet
    )
{
    HRESULT hr = S_OK;

    ASSERTMSG(pfnComPropSheet != NULL, "NULL pointer to common propert sheet functions.\n");

    if (SUCCEEDED(hr = CHECK_POINTER(pfnComPropSheet, E_POINTER)))
    {
        m_pfnComPropSheet = pfnComPropSheet;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SetPageHandle

Routine Description:

    Store the handle of a property page window.

Arguments:

    hPage - Handle of a property page window.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetPageHandle(
    _In_ CONST HANDLE hPage
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_HANDLE(hPage, E_HANDLE)))
    {
        m_hPage = hPage;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SetComPropSheetHandle

Routine Description:

    Store the handle of the Common Property Sheet window.

Arguments:

    hComPropSheet - Handle of the common property sheet window.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetComPropSheetHandle(
    _In_ CONST HANDLE hComPropSheet
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_HANDLE(hComPropSheet, E_HANDLE)))
    {
        m_hComPropSheet = hComPropSheet;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::GetComPropSheetFunc

Routine Description:

    Retrieve the pointer of the PFNCOMPROPSHEET function.

Arguments:

    ppfnComPropSheet - Address of a pointer that will be filled out with the PFNCOMPROPSHEET function.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::GetComPropSheetFunc(
    _Outptr_ PFNCOMPROPSHEET* ppfnComPropSheet
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppfnComPropSheet, E_POINTER)))
    {
        *ppfnComPropSheet = m_pfnComPropSheet;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::GetPageHandle

Routine Description:

    Retrieves the property page handle.

Arguments:

    None

Return Value:

    Handle to the property page

--*/
HANDLE
CDocPropPage::GetPageHandle(
    VOID
    )
{
    return m_hPage;
}

/*++

Routine Name:

    CDocPropPage::GetComPropSheetHandle

Routine Description:

    Retrieves the common property sheet handle

Arguments:

    None

Return Value:

    Handle to the common property sheet

--*/
HANDLE
CDocPropPage::GetComPropSheetHandle(
    VOID
    )
{
    return m_hComPropSheet;
}

/*++

Routine Name:

    CDocPropPage::DestroyUIComponents

Routine Description:

    Destroy all control handler classes that have been added into the collection.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::DestroyUIComponents(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        UIControlMap::iterator iterUIComponents = m_UIControls.begin();

        while (iterUIComponents != m_UIControls.end())
        {
            if (iterUIComponents->second != NULL)
            {
                delete iterUIComponents->second;
                iterUIComponents->second = NULL;
            }
            iterUIComponents++;
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::AddUIControl

Routine Description:

    Adds a control handler class into the collection.

Arguments:

    iCtrlID - Resource Identifier of control.
    pUIControl - Pointer to a control handler class.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::AddUIControl(
    _In_ CONST INT   iCtrlID,
    _In_ CUIControl* pUIControl
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pUIControl, E_POINTER)))
    {
        try
        {
            m_UIControls[iCtrlID] = pUIControl;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::GetUIComponents

Routine Description:

    Obtains the collection of control handlers.

Arguments:

    ppUIComponents - Address of the pointer that will be filled out with the contain handler collection.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::GetUIComponents(
    _Outptr_ UIControlMap** ppUIComponents
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppUIComponents, E_POINTER)))
    {
        *ppUIComponents = &m_UIControls;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SetOemCUIPParam

Routine Description:

    Store the pointer to the POEMCUIPPARAM function.

Arguments:

    pOemCUIPParam - pointer to the POEMCUIPPARAM function.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetOemCUIPParam(
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

    CDocPropPage::SetUIProperties

Routine Description:

    Store the pointer to an CUIProperties interface.

Arguments:

    pUIProperties - pointer to an CUIProperties interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetUIProperties(
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

    CDocPropPage::GetOemCUIPParam

Routine Description:

    Retrieve the pointer to the OEMCUIPPARAM function.

Arguments:

    ppOemCUIPParam - Address of a pointer that will be filled out with the POEMCUIPPARAM function.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::GetOemCUIPParam(
    _Outptr_ POEMCUIPPARAM* ppOemCUIPParam
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppOemCUIPParam, E_POINTER)))
    {
        *ppOemCUIPParam = m_pOemCUIPParam;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SetPrintOemDriverUI

Routine Description:

    Store a pointer to the IPrintOemDriverUI interface.

Arguments:

    pOEMDriverUI - pointer to the IPrintOemDriverUI interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SetPrintOemDriverUI(
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

/*++

Routine Name:

    CDocPropPage::PublishHelpToControls

Routine Description:

    Propagate any useful interfaces used in this class down to the collection of control handlers.
    This allows access to these helper interfaces in the control handlers classes.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::PublishHelpToControls(
    VOID
    )
{
    ASSERTMSG(m_pDriverUIHelp != NULL, "NULL pointer to driver UI help interface.\n");
    ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");
    ASSERTMSG(m_pUIProperties != NULL, "NULL pointer to CUIProperties.\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pDriverUIHelp, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOemCUIPParam, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_PENDING)))
    {
        try
        {
            //
            // Iterate over all controls and report the helper interfaces
            //
            if (!m_UIControls.empty())
            {
                UIControlMap::iterator iterUIComponents = m_UIControls.begin();

                while (iterUIComponents != m_UIControls.end())
                {
                    CUIControl* pControl = iterUIComponents->second;

                    if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_POINTER)) &&
                        SUCCEEDED(hr = pControl->SetPrintOemDriverUI(m_pDriverUIHelp)) &&
                        SUCCEEDED(hr = pControl->SetUIProperties(m_pUIProperties)) &&
                        SUCCEEDED(hr = pControl->SetOemCUIPParam(m_pOemCUIPParam)))
                    {
                        iterUIComponents++;
                    }
                }

            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SendCommand

Routine Description:

    Call the OnCommand() method in the relevant control handler in the collection.

Arguments:

    hDlg - Handle of property page.
    wParam - Windows WPARAM value passed with the windows message WM_COMMAND.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SendCommand(
    _In_ CONST HWND   hDlg,
    _In_ CONST WPARAM wParam
    )
{
    //
    // Use the wParam as the index into the control map and
    // inform the control that generated the command
    //
    HRESULT hr = S_OK;

    UIControlMap* pComponents = NULL;

    if (SUCCEEDED(hr = GetUIComponents(&pComponents)) &&
        SUCCEEDED(hr = CHECK_POINTER(pComponents, E_POINTER)))
    {
        try
        {
            CUIControl* pControl = (*pComponents)[LOWORD(wParam)];
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_POINTER)))
            {
                hr = pControl->OnCommand(hDlg, HIWORD(wParam));
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SendNotify

Routine Description:

    Call the OnNotify() method in the relevant control handler in the collection.

Arguments:

    hDlg - Handle of property page.
    pNMhdr - Windows Notify structure that was passed with the windows message WM_NOTIFY.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SendNotify(
    _In_ CONST HWND   hDlg,
    _In_ CONST NMHDR* pNMhdr
    )
{
    //
    // Use the wParam as the index into the control map and
    // inform the control of an activation
    //
    HRESULT hr = S_OK;

    UIControlMap* pComponents = NULL;

    if (SUCCEEDED(hr = GetUIComponents(&pComponents)) &&
        SUCCEEDED(hr = CHECK_POINTER(pComponents, E_POINTER)))
    {
        try
        {
            CUIControl* pControl = (*pComponents)[pNMhdr->idFrom];
            if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_POINTER)))
            {
                hr = pControl->OnNotify(hDlg, pNMhdr);
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SendSetActive

Routine Description:

    Call the OnActivate() method in all control handlers in the collection.

Arguments:

    hDlg - Handle of property page.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SendSetActive(
    _In_ CONST HWND   hDlg
    )
{
    //
    // Use the wParam as the index into the control map and
    // inform the control of an activation
    //
    HRESULT hr = S_OK;

    UIControlMap* pComponents = NULL;

    if (SUCCEEDED(hr = GetUIComponents(&pComponents)) &&
        SUCCEEDED(hr = CHECK_POINTER(pComponents, E_POINTER)))
    {
        try
        {
            UIControlMap::iterator iterUIComponents = pComponents->begin();

            while (iterUIComponents != pComponents->end())
            {
                CUIControl* pControl = iterUIComponents->second;
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_POINTER)))
                {
                    if (FAILED(hr = pControl->OnActivate(hDlg)))
                    {
                        break;
                    }
                }

                iterUIComponents++;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::SendInit

Routine Description:

    Call the OnInit() method in all control handlers in the collection.

Arguments:

    hDlg - Handle of property page.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CDocPropPage::SendInit(
    _In_ CONST HWND   hDlg
    )
{
    //
    // Use the wParam as the index into the control map and
    // inform the control of an activation
    //
    HRESULT hr = S_OK;

    UIControlMap* pComponents = NULL;

    if (SUCCEEDED(hr = GetUIComponents(&pComponents)) &&
        SUCCEEDED(hr = CHECK_POINTER(pComponents, E_POINTER)))
    {
        try
        {
            UIControlMap::iterator iterUIComponents = pComponents->begin();

            while (iterUIComponents != pComponents->end())
            {
                CUIControl* pControl = iterUIComponents->second;
                if (SUCCEEDED(hr = CHECK_POINTER(pControl, E_POINTER)))
                {
                    if (FAILED(hr = pControl->OnInit(hDlg)))
                    {
                        break;
                    }
                }

                iterUIComponents++;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CDocPropPage::DlgProc

Routine Description:

    Dialog proccedure for the property page.
    This handles all windows messages that are sent to the property page.

Arguments:

    hDlg - Handle of the property page.
    uiMessage - Specifies the message.
    wParam - Specifies additional message-specific information.
    lParam - Specifies additional message-specific information.

Return Value:

    The return value is the result of the message processing and depends on the message sent.

--*/
INT_PTR CALLBACK
CDocPropPage::DlgProc(
    _In_ CONST HWND   hDlg,
    _In_ CONST UINT   uiMessage,
    _In_ CONST WPARAM wParam,
    _In_ CONST LPARAM lParam
    )
{
    HRESULT hr = S_OK;
    BOOL retVal = FALSE;

    switch (uiMessage)
    {
        case WM_INITDIALOG:
        {
            //
            // Store the class instance
            //
            PROPSHEETPAGE* pPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);

            if (SUCCEEDED(hr = CHECK_POINTER(pPage, E_POINTER)))
            {
                if (pPage->lParam != NULL)
                {
                    CDocPropPage* thisInst = reinterpret_cast<CDocPropPage*>(pPage->lParam);

                    if (SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)))
                    {
                        if (SUCCEEDED(hr = thisInst->StoreThis(hDlg)))
                        {
                            hr = thisInst->SendInit(hDlg);
                        }
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            //
            // Set the keyboard focus to the control specified by wParam
            //
            retVal = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
                case EN_CHANGE:
                case BN_CLICKED:
                case CBN_SELCHANGE:
                // case LBN_SELCHANGE: CBN_SELCHANGE=LBN_SELCHANGE
                {
                    CDocPropPage* thisInst;

                    if (SUCCEEDED(hr = RetrieveThis(hDlg, &thisInst)))
                    {
                        if (SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)))
                        {
                            hr = thisInst->SendCommand(hDlg, wParam);
                        }
                    }

                    //
                    // Set to FALSE to indiate that the message has been handled.
                    //
                    retVal = FALSE;
                }
                break;

                default:
                {
                    //
                    // Unhandled command so return TRUE
                    //
                    retVal = TRUE;
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            NMHDR* pHdr = reinterpret_cast<NMHDR*>(lParam);
            if (SUCCEEDED(hr = CHECK_POINTER(pHdr, E_POINTER)))
            {
                switch (pHdr->code)
                {
                    case PSN_SETACTIVE:
                    {
                        CDocPropPage* thisInst;

                        if (SUCCEEDED(hr = RetrieveThis(hDlg, &thisInst)) &&
                            SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)))
                        {
                            hr = thisInst->SendSetActive(hDlg);
                        }

                        //
                        // Return FALSE to accept the page activation
                        //
                        retVal = FALSE;
                    }
                    break;

                    case PSN_KILLACTIVE:
                    {
                        //
                        // Return FALSE to allow the page to lose activation
                        //
                        retVal = FALSE;
                    }
                    break;

                    case PSN_APPLY:
                    {
                        PFNCOMPROPSHEET pfnComPropSheet = NULL;

                        CDocPropPage* thisInst;

                        if (SUCCEEDED(hr = RetrieveThis(hDlg, &thisInst)) &&
                            SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)) &&
                            SUCCEEDED(hr = thisInst->GetComPropSheetFunc(&pfnComPropSheet)) &&
                            SUCCEEDED(hr = CHECK_POINTER(pfnComPropSheet, E_FAIL)))
                        {
                            //
                            // Ensure that the last error is in a known state.
                            //
                            SetLastError(0);

                            //
                            // We do not need to validate any settings so set PSNRET_NOERROR
                            //
                            if (SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR) == 0)
                            {
                                //
                                // A return value of 0 does not necessarily indicate a failure.
                                //
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }

                            if (SUCCEEDED(hr))
                            {
                                //
                                // We have applied the change...
                                //
                                PropSheet_UnChanged(GetParent(hDlg), hDlg);

                                //
                                // Inform the propsheet
                                //
                                pfnComPropSheet(thisInst->GetComPropSheetHandle(),
                                                CPSFUNC_SET_RESULT,
                                                reinterpret_cast<LPARAM>(thisInst->GetPageHandle()),
                                                (LPARAM)CPSUI_OK);
                            }
                        }

                        retVal = TRUE;
                    }
                    break;

                    case UDN_DELTAPOS:
                    {
                        CDocPropPage* thisInst;

                        if (SUCCEEDED(hr = RetrieveThis(hDlg, &thisInst)) &&
                            SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)))
                        {
                            hr = thisInst->SendNotify(hDlg, pHdr);
                        }

                        //
                        // Set return to FALSE to allow the control value
                        //
                        retVal = FALSE;
                    }
                    break;
                }
            }
        }
        break;

        case WM_NCDESTROY:
        {
            CDocPropPage* thisInst;

            if (SUCCEEDED(hr = CDocPropPage::RetrieveThis(hDlg, &thisInst)) &&
                SUCCEEDED(hr = CHECK_POINTER(thisInst, E_FAIL)))
            {
                hr = thisInst->RemoveThis(hDlg);
            }

            //
            // Set return to FALSE to indicate that the message was processed
            //
            retVal = FALSE;
        }
        break;
    }

    ERR_ON_HR(hr);
    return retVal;
}

