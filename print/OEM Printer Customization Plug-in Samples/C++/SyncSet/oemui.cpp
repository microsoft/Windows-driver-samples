//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    OEMUI.cpp
//
//  PURPOSE:  Main file for OEM UI test module.
//

#include "precomp.h"
#include "resource.h"
#include "debug.h"
#include "oemui.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      INTERNAL MACROS and DEFINES
////////////////////////////////////////////////////////

typedef struct _tagCBUserData
{
    HANDLE          hComPropSheet;
    HANDLE          hPropPage;
    POEMUIPSPARAM   pOEMUIParam;
    PFNCOMPROPSHEET pfnComPropSheet;

} CBUSERDATA, *PCBUSERDATA;


////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static HRESULT hrDocumentPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);

LONG APIENTRY OEMPrinterUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUICallBack2(PCPSUICBPARAM pCallbackParam);
INT_PTR CALLBACK DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

void SetOEMUiState (HWND hDlg, POEMSHEETDATA pOEMSheetData);
void SetOEMCommonDataIntoOptItem (POEMSHEETDATA pOEMSheetData, _In_ PSTR PName, LONG Sel);
void ChangeOpItem (IPrintOemDriverUI *pIPrintOEMDrvUI,  POEMUIOBJ pOEMObj, POPTITEM pOptItem, int iSel);
POPTITEM FindDrvOptItem (POPTITEM pDrvOptItems, DWORD dwItemCount, _In_ PSTR pKeyWordName, BYTE dmPubID = DMPUB_NONE);

static POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems);
static void InitOptItems(POPTITEM pOptItems, DWORD dwOptItems);
static POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams);
static PTSTR GetStringResource(HANDLE hHeap, HANDLE hModule, UINT uResource);

////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM device or document property UI.
// Called via IOemUI::CommonUIProp
//
HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    HRESULT hResult = S_OK;


    VERBOSE(DLLTEXT("hrOEMPropertyPage(%d) entry.\r\n"), dwMode);

    // Validate parameters.
    if( (OEMCUIP_DOCPROP != dwMode)
        &&
        (OEMCUIP_PRNPROP != dwMode)
      )
    {
        ERR(ERRORTEXT("hrOEMPropertyPage() ERROR_INVALID_PARAMETER.\r\n"));
        VERBOSE(DLLTEXT("\tdwMode = %d, pOEMUIParam = %#lx.\r\n"), dwMode, pOEMUIParam);

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        case OEMCUIP_DOCPROP:
            hResult = hrDocumentPropertyPage(dwMode, pOEMUIParam);
            break;

        case OEMCUIP_PRNPROP:
            hResult = hrPrinterPropertyPage(dwMode, pOEMUIParam);
            break;

        default:
            // Should never reach this!
            ERR(ERRORTEXT("hrOEMPropertyPage() Invalid dwMode, %d"), dwMode);
            SetLastError(ERROR_INVALID_PARAMETER);
            hResult = E_FAIL;
            break;
    }

    return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM document property UI.
//
static HRESULT hrDocumentPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    UNREFERENCED_PARAMETER(dwMode);

    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Fill in the number of OptItems to create for OEM document property UI.
        pOEMUIParam->cOEMOptItems = 1;
        VERBOSE(DLLTEXT("hrDocumentPropertyPage() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
        POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


        VERBOSE(DLLTEXT("hrDocumentPropertyPage() fill out %d items.\r\n"), pOEMUIParam->cOEMOptItems);

        // Init UI Callback reference.
        pOEMUIParam->OEMCUIPCallback = OEMDocUICallBack;

        // Init OEMOptItmes.
        InitOptItems(pOEMUIParam->pOEMOptItems, pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.

        // New section.
        pOEMUIParam->pOEMOptItems[0].Level  = 1;
        pOEMUIParam->pOEMOptItems[0].Flags  = OPTIF_COLLAPSE;
        pOEMUIParam->pOEMOptItems[0].pName  = GetStringResource(pOEMUIParam->hOEMHeap, pOEMUIParam->hModule, IDS_ADV_SECTION);
        pOEMUIParam->pOEMOptItems[0].Sel    = pOEMDev->dwAdvancedData;

        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);

        //
        //Setup the Optional Item
        //
        pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;
    }

    return S_OK;
}



////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM printer property UI.
//
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    UNREFERENCED_PARAMETER(dwMode);

    //
    //This is null the first time the interface is called.
    //
    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Fill in the number of OptItems to create for OEM printer property UI.
        pOEMUIParam->cOEMOptItems = 1;
        VERBOSE(DLLTEXT("hrPrinterPropertyPage() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
        //
        //This is the second time we are called Now setup the optional items.
        //
        DWORD   dwError;
        DWORD   dwDeviceValue;
        DWORD   dwType;
        DWORD   dwNeeded;

        VERBOSE(DLLTEXT("hrPrinterPropertyPage() fill out %d items.\r\n"), pOEMUIParam->cOEMOptItems);

        //
        //Add OEM Optitem to the Device
        //

        // Get device settings value from printer.
        dwError = GetPrinterData(pOEMUIParam->hPrinter, OEMUI_VALUE, &dwType, (PBYTE) &dwDeviceValue,
                                   sizeof(dwDeviceValue), &dwNeeded);
        if( (ERROR_SUCCESS != dwError)
            ||
            (dwDeviceValue > 100)
          )
        {
            // Failed to get the device value or value is invalid, just use the default.
            dwDeviceValue = 0;
        }

        // Init UI Callback reference.
        pOEMUIParam->OEMCUIPCallback = OEMPrinterUICallBack;


        // Init OEMOptItmes.
        InitOptItems(pOEMUIParam->pOEMOptItems, pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.
        // New section.
        pOEMUIParam->pOEMOptItems[0].Level = 1;
        pOEMUIParam->pOEMOptItems[0].Flags = OPTIF_COLLAPSE;
        pOEMUIParam->pOEMOptItems[0].pName = GetStringResource(pOEMUIParam->hOEMHeap, pOEMUIParam->hModule, IDS_DEV_SECTION);
        pOEMUIParam->pOEMOptItems[0].Sel = dwDeviceValue;

        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);

        //
        //Setup the Optional Item
        //
        pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;


    }
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// Adds property page to Document property sheet. Called via IOemUI::DocumentPropertySheets
//
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam,
                                    IPrintOemDriverUI*  pOEMHelp)
{
    LONG_PTR    lResult = 0;
    HRESULT     hrResult = S_OK;

    VERBOSE(DLLTEXT("hrOEMDocumentPropertySheets() entry.\r\n"));

    UNREFERENCED_PARAMETER(pOEMHelp);

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR(ERRORTEXT("hrOEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return  E_FAIL;
    }

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                DWORD           dwSheets    = 0;
                PCBUSERDATA     pUserData;
                POEMUIPSPARAM   pOEMUIParam = (POEMUIPSPARAM) pPSUIInfo->lParamInit;
                HANDLE          hHeap       = pOEMUIParam->hOEMHeap;
                POEMDEV         pOEMDev     = (POEMDEV) pOEMUIParam->pOEMDM;
                COMPROPSHEETUI  Sheet;


                // Init property page.
                memset(&Sheet, 0, sizeof(COMPROPSHEETUI));
                Sheet.cbSize            = sizeof(COMPROPSHEETUI);
                Sheet.Flags             = CPSUIF_UPDATE_PERMISSION;
                Sheet.hInstCaller       = ghInstance;
                Sheet.pCallerName       = GetStringResource(hHeap, ghInstance, IDS_NAME);
                Sheet.pHelpFile         = NULL;
                Sheet.pfnCallBack       = OEMDocUICallBack2;
                Sheet.pDlgPage          = CPSUI_PDLGPAGE_TREEVIEWONLY;
                Sheet.cOptItem          = 1;
                Sheet.IconID            = IDI_CPSUI_PRINTER;
                Sheet.pOptItemName      = GetStringResource(hHeap, ghInstance, IDS_SECTION);
                Sheet.CallerVersion     = 0x100;
                Sheet.OptItemVersion    = 0x100;

                // Init user data.
                pUserData = (PCBUSERDATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CBUSERDATA));
                if (NULL == pUserData)
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to allocate user data.\r\n"));

                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                pUserData->hComPropSheet    = pPSUIInfo->hComPropSheet;
                pUserData->pfnComPropSheet  = pPSUIInfo->pfnComPropSheet;
                pUserData->pOEMUIParam      = pOEMUIParam;
                Sheet.UserData              = (ULONG_PTR) pUserData;

                // Create OptItems for page.
                Sheet.pOptItem = CreateOptItems(hHeap, Sheet.cOptItem);
                if (NULL == Sheet.pOptItem)
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to allocate OPTITEMs.\r\n"));

                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                // Initialize OptItems
                Sheet.pOptItem[0].Level = 1;
                Sheet.pOptItem[0].Flags = OPTIF_COLLAPSE;
                Sheet.pOptItem[0].pName = GetStringResource(hHeap, ghInstance, IDS_SECTION);
                Sheet.pOptItem[0].Sel   = pOEMDev->dwDriverData;

                Sheet.pOptItem[0].pOptType = CreateOptType(hHeap, 2);
                if (NULL == Sheet.pOptItem[0].pOptType)
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to allocate OPTTYPE.\r\n"));

                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                //
                //Set the UI prop of this OPTYPE item.
                //
                Sheet.pOptItem[0].pOptType->Type = TVOT_UDARROW;
                Sheet.pOptItem[0].pOptType->pOptParam[1].IconID = 0;
                Sheet.pOptItem[0].pOptType->pOptParam[1].lParam = 100;

                // Adds the  property sheets.
                lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PCOMPROPSHEETUI,
                                                     (LPARAM)&Sheet, (LPARAM)&dwSheets);
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER    pHeader = (PPROPSHEETUI_INFO_HEADER) lParam;

                pHeader->pTitle = (LPTSTR)PROP_TITLE;
                lResult = TRUE;
            }
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            break;
    }

Exit:

    pPSUIInfo->Result = lResult;
    return hrResult;
}


////////////////////////////////////////////////////////////////////////////////
//
// Adds property page to printer property sheet. Called via IOemUI::DevicePropertySheets
//
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, POEMSHEETDATA pOemSheetData)
{
    LONG_PTR    lResult = 0;


    VERBOSE(DLLTEXT("hrOEMDevicePropertySheets(%#x, %#x) entry\r\n"), pPSUIInfo, lParam);

    // Validate parameters.
    if( (NULL == pPSUIInfo)

        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR(ERRORTEXT("hrOEMDevicePropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    Dump(pPSUIInfo);


    // Do action.
    switch(pPSUIInfo->Reason)
    {

        case PROPSHEETUI_REASON_INIT:
            {
                PROPSHEETPAGE   Page;

                // Init property page.
                memset(&Page, 0, sizeof(PROPSHEETPAGE));
                Page.dwSize = sizeof(PROPSHEETPAGE);
                Page.dwFlags = PSP_DEFAULT;
                Page.hInstance = ghInstance;
                Page.pszTemplate = MAKEINTRESOURCE(IDD_DEVICE_PROPPAGE);
                Page.pfnDlgProc = DevicePropPageProc;

                //
                //This is the OEMPlugIN Sheets Shared Data. Use this Pointer to gain access to shared driver data
                //
                pOemSheetData->hComPropSheet = pPSUIInfo->hComPropSheet;
                pOemSheetData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
                Page.lParam = (LPARAM)pOemSheetData;

                // Add property sheets.
                pOemSheetData->hmyPlugin = (HANDLE)(pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PROPSHEETPAGE, (LPARAM)&Page, 0));

                VERBOSE(DLLTEXT("hrOEMDevicePropertySheets() pfnComPropSheet returned %d.\r\n"), pOemSheetData->hmyPlugin);
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER    pHeader = (PPROPSHEETUI_INFO_HEADER) lParam;

                pHeader->pTitle = (LPTSTR)PROP_TITLE;
                lResult = TRUE;
            }
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            break;
    }

    pPSUIInfo->Result = lResult;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
// OptItems call back for OEM printer property UI.
//
LONG APIENTRY OEMPrinterUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    lReturn = CPSUICB_ACTION_NONE;

    VERBOSE(DLLTEXT("OEMPrinterUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            {
                DWORD   dwDriverValue = pOEMUIParam->pOEMOptItems[0].Sel;

                //
                // Store OptItems state in printer data.
                //
                SetPrinterData(pOEMUIParam->hPrinter, OEMUI_VALUE, REG_DWORD, (PBYTE) &dwDriverValue, sizeof(DWORD));
            }
            break;

        //
        //Because the plugin page is changing items in this CPSUI page we need to rinitalise the data on the page.
        //
        case CPSUICB_REASON_SETACTIVE:
            return CPSUICB_ACTION_OPTIF_CHANGED;
            break;

        default:
            break;
    }

    return lReturn;
}



////////////////////////////////////////////////////////////////////////////////
//
// Call back for OEM device property UI.
//
// This OEM plugin page is using the OPTITEMS in the Device tab to store data. (Installabale Options)
// If you hide these option in the device tab and only access them from your UI you dont have to worry about sync the views.
// If you want the Options in both tabs you need to have a CPSUICALLBACK function defined for the Device tab. (OEMPrinterUICallBack)
// OEMPrinterUICallBack will return CPSUICB_ACTION_OPTIF_CHANGED; to indicate that some of the controls have change. Resulting in CPSUI to refresh the view.
//
INT_PTR CALLBACK DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{

    VERBOSE(DLLTEXT("DevicePropPageProc() entry, Reason is %d.\r\n"), uiMsg);

    POEMSHEETDATA   pOEMSheetData   = NULL;
    LONG            lState          = 0;
    DWORD           fDataValid      = 0;
    WORD            dlgControl      = 0;
    PSTR            pControlName    = NULL;


    if ( uiMsg != WM_INITDIALOG )
    {
        //
        //Retrieves Application data storded by the call to SetWindowLongPtr.
        //
        pOEMSheetData = (POEMSHEETDATA)GetWindowLongPtr( hDlg, DWLP_USER);


        if (!pOEMSheetData)
        {
            //
            //Failed to get the OEM Prop Page Data Pointer.
            //
            ERR(ERRORTEXT("DevicePropPageProc : GetWindowLongPtr Failed (%d)\r\n"), GetLastError());
            return FALSE;
        }
    }

    switch (uiMsg)
    {
        case WM_INITDIALOG:
            //
            //PROPSHEETPAGE structure is passed to the dialog box procedure with a WM_INITDIALOG message.
            //The lParam member is provided to allow you to pass application-specific information to the dialog box procedure
            //
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR)((LPPROPSHEETPAGE)lParam)->lParam);
            break;

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {

                case BN_CLICKED:

                    //
                    //Change the UI to activate the apply button Send the prop dailog a change message.
                    //
                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    switch(LOWORD(wParam))
                    {

                        //
                        //USER modified OEM Controls on the plugin page.
                        //
                        case IDC_CHECK_DUPLPEX:
                            dlgControl = IDC_CHECK_DUPLPEX;
                            fDataValid = TRUE;
                            pControlName = (PSTR)DUPLEXUNIT;
                            break;

                        case IDC_CHECK_HDRIVE:
                            dlgControl = IDC_CHECK_HDRIVE;
                            fDataValid = TRUE;
                            pControlName = (PSTR)PRINTERHDISK;
                            break;

                        case IDC_CHECK_ENVFEEDER:
                            dlgControl = IDC_CHECK_ENVFEEDER;
                            fDataValid = TRUE;
                            pControlName = (PSTR)ENVFEEDER;
                            break;

                        default:
                            fDataValid = 0;
                            break;

                    }

                    //
                    //If the Control is valid Get the ctrl state and UPdate the OPTITEM.
                    //
                    if ( fDataValid )
                    {
                        lState = (LONG)(SendDlgItemMessage (hDlg, dlgControl, BM_GETSTATE, 0, 0) & 0x0003);
                        VERBOSE(DLLTEXT("DevicePropPageProc  :  Clicked dlgControl (%d)\r\n"), lState);

                        //
                        //Saves the Data to the OPTITEMS in the device settings page.
                        //
                        SetOEMCommonDataIntoOptItem (pOEMSheetData, pControlName, lState);
                    }
                    break;


                default:
                    return FALSE;
            }
            return TRUE;

        //
        //Dailog is going to paint set up the UI controls to match the relavent OPTITEMS Data.
        //
        case WM_CTLCOLORDLG:
            {
                VERBOSE(DLLTEXT("DevicePropPageProc : WM_CTLCOLORDLG \r\n"));
                SetOEMUiState (hDlg, pOEMSheetData);
            }
            return TRUE;


        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)  // type of notification message
                {
                    case PSN_SETACTIVE:
                        return TRUE;
                        break;

                    case PSN_KILLACTIVE:
                        return TRUE;
                        break;

                    case PSN_APPLY:

                        //
                        //Calls ComPropSheet
                        //causes the ComPropSheet function to pass a specified result value to all PFNPROPSHEETUI-typed functions associated with a specified page and its parents
                        //
                        pOEMSheetData->pfnComPropSheet(pOEMSheetData->hComPropSheet,
                                                       CPSFUNC_SET_RESULT,
                                                       (LPARAM) pOEMSheetData->hmyPlugin,
                                                       (LPARAM)CPSUI_OK);

                        //
                        //Accept the apply if the data was invalid and you are checking for this set PSNRET_INVALID | PSNRET_INVALID_NOCHANGEPAGE
                        //Disables the APPLY button and saves the Data.
                        //
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,PSNRET_NOERROR);
                        return TRUE;
                        break;

                    //
                    // Need To undo the changes
                    //
                    case PSN_RESET:
                         break;
                }
            }
            break;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Update the OEM page controls to match the Device settings page
//

void SetOEMUiState (HWND hDlg, POEMSHEETDATA pOEMSheetData)
{
    LRESULT         lstate = 0;
    POEMCUIPPARAM   pOEMCUIParam = NULL;
    PUSERDATA       pUserData = NULL;
    POPTITEM        pOptItem = NULL;
    DWORD           cDrvOptItems = 0;


    if (pOEMSheetData->pOEMCUIParam)
    {
        pOEMCUIParam = pOEMSheetData->pOEMCUIParam;
        cDrvOptItems = pOEMCUIParam->cDrvOptItems;
    }

    //
    //loop down the OPITEMS and retrieve the data that is needed from the control
    //(all Optitesm are ref from DMPubID of the UserData->pKeyWordName)
    //
    for (DWORD i=0; i < cDrvOptItems; i++)
    {
        pOptItem = &(pOEMCUIParam->pDrvOptItems[i]);
        if (pOptItem->UserData)
        {
            pUserData = (PUSERDATA)(pOptItem->UserData);
            if ( pUserData->pKeyWordName )
            {
                //
                //Get the Selection of the OPITEM
                //
                if (pOptItem->Sel)
                {
                    lstate = BST_CHECKED;
                }
                else
                {
                    lstate = BST_UNCHECKED;
                }

                //
                //Update the Data in OEM Page to mactch the data in the Device Settings TAB.
                //Send a message to the Cntrol to update its State.
                //

                //Set the Duplex Option
                if ( strcmp(pUserData->pKeyWordName, DUPLEXUNIT ) == 0 )
                {
                    SendDlgItemMessage (hDlg, IDC_CHECK_DUPLPEX, BM_SETCHECK, lstate, 0);
                }

                //Set HardDrive Option
                if ( strcmp (pUserData->pKeyWordName, PRINTERHDISK ) == 0 )
                {
                    SendDlgItemMessage (hDlg, IDC_CHECK_HDRIVE, BM_SETCHECK, lstate, 0);
                }

                //Set the Env Feeder Option
                if ( strcmp (pUserData->pKeyWordName, ENVFEEDER ) == 0 )
                {
                    SendDlgItemMessage (hDlg, IDC_CHECK_ENVFEEDER, BM_SETCHECK, lstate, 0);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Save the OEM change into the correct OPTITEM.
// The UI data is been stored in the driver supplied OPTITEM
//
void SetOEMCommonDataIntoOptItem (POEMSHEETDATA pOEMSheetData, _In_ PSTR PName, LONG Sel)
{

    VERBOSE(DLLTEXT("SetOEMCommonDataIntoOptItem (%S) \r\n"), PName);

    POEMCUIPPARAM pOEMCUIParam = pOEMSheetData->pOEMCUIParam;
    POPTITEM pOptItem = NULL;

    //
    //Find the OPTITEM that needs to be Updated.
    //
    pOptItem = FindDrvOptItem (pOEMCUIParam->pDrvOptItems,
                               pOEMCUIParam->cDrvOptItems,
                               PName,
                               DMPUB_NONE);

    if (pOptItem)
    {
        ChangeOpItem (pOEMSheetData->pOEMHelp,
                      pOEMCUIParam->poemuiobj,
                      pOptItem,
                      Sel);
    }
    else
    {
        ERR(ERRORTEXT("SetOEMCommonDataIntoOptItem Item Not Found OPTITEM(%S) \r\n"), PName);
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Help Change the data set in the OPTITEM
//
void ChangeOpItem (IPrintOemDriverUI *pIPrintOEMDrvUI,  POEMUIOBJ pOEMUIObj, POPTITEM pOptItem, int iSel)
{
    HRESULT hRestult = S_OK;

    //
    //Change the Item (Change which OPTPARAM is selected fot the required OPTTYPE)
    //
    pOptItem->Sel = iSel;

    //
    //Set the Flag indicating it is changed
    //
    pOptItem->Flags |= OPTIF_CHANGED;

    //
    //Update the Driver UI. (this updates the UI but it does not cause CPSUI to update the its pages),
    //Look at OEMPrinterUICallBack to see how CPSUI pages are updated
    //The IPrintOemDriverUI::DrvUpdateUISetting method is provided by the
    //Unidrv and Pscript5 minidrivers so that user interface plug-ins can
    //notify the driver of a modified user interface option.
    //
    hRestult = pIPrintOEMDrvUI->DrvUpdateUISetting((PVOID)pOEMUIObj, (PVOID)pOptItem, 0, OEMCUIP_PRNPROP);

    if (hRestult != S_OK)
    {
        ERR(ERRORTEXT("ChangeOpItem DrvUpdateUISetting FAILED \r\n") );
    }

}


////////////////////////////////////////////////////////////////////////////////
//
// Help Find a OPTITEM from the array based on USERDATA->pKeyWordName or DMPubID
//
POPTITEM FindDrvOptItem (POPTITEM pDrvOptItems, DWORD dwItemCount, _In_ PSTR pKeyWordName, BYTE dmPubID )
{
    VERBOSE(DLLTEXT("FindDrvOptItem Looking (%S) (%d) \r\n"), pKeyWordName, dmPubID);

    POPTITEM  pOptItem  = NULL;
    PUSERDATA pUserData = NULL;

    for (DWORD i=0; i < dwItemCount; i++)
    {
        //
        //OPTITEM has valid data compare these now (Search on DMPubID)
        //
        if (pKeyWordName == NULL)
        {
            if (dmPubID == pDrvOptItems[i].DMPubID)
            {
                pOptItem = &(pDrvOptItems[i]);
                break;
            }
        }
        else
        {
            //
            //(Search a printer freature pKeyWordName)
            //
            //For Common Printer Features the key name is set to a common non localised string
            //Note the these are only ansi strings ie PSTR
            //
            if (pDrvOptItems[i].UserData)
            {
                pUserData = (PUSERDATA)pDrvOptItems[i].UserData;
                if (pUserData->pKeyWordName)
                {
                    //
                    //OPTITEM has valid data compare these now
                    //
                    if ( strcmp (pUserData->pKeyWordName, pKeyWordName) == 0)
                    {
                        pOptItem = &(pDrvOptItems[i]);
                        break;
                    }
                }
            }
        }
    }

    if (!pOptItem)
    {
        ERR(ERRORTEXT("FindDrvOptItem OPTITEM NOT Found \r\n") );
    }

    return pOptItem;
}


////////////////////////////////////////////////////////////////////////////////
//
// OptItems call back for OEM document property UI.
//
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    lReturn = CPSUICB_ACTION_NONE;
    POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            // Store OptItems state in DEVMODE.
            pOEMDev->dwAdvancedData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        case CPSUICB_REASON_KILLACTIVE:
            pOEMDev->dwAdvancedData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        //
        //Refress the Data in the view and tell CPUSI that the data has changed by ret CPSUICB_ACTION_OPTIF_CHANGED
        //
        case CPSUICB_REASON_SETACTIVE:
            if((DWORD)(pOEMUIParam->pOEMOptItems[0].Sel) != pOEMDev->dwAdvancedData)
            {
                pOEMUIParam->pOEMOptItems[0].Sel = pOEMDev->dwAdvancedData;
                pOEMUIParam->pOEMOptItems[0].Flags |= OPTIF_CHANGED;
                lReturn = CPSUICB_ACTION_OPTIF_CHANGED;
            }
            break;

        default:
            break;
    }

    return lReturn;
}


LONG APIENTRY OEMDocUICallBack2(PCPSUICBPARAM pCallbackParam)
{
    LONG            lReturn = CPSUICB_ACTION_NONE;
    PCBUSERDATA     pUserData = (PCBUSERDATA) pCallbackParam->UserData;
    POEMDEV         pOEMDev = (POEMDEV) pUserData->pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUICallBack2() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            pOEMDev->dwDriverData = pCallbackParam->pOptItem[0].Sel;
            pUserData->pfnComPropSheet(pUserData->hComPropSheet, CPSFUNC_SET_RESULT,
                                       (LPARAM)pUserData->hPropPage,
                                       (LPARAM)CPSUI_OK);
            break;

        case CPSUICB_REASON_KILLACTIVE:
            pOEMDev->dwDriverData = pCallbackParam->pOptItem[0].Sel;
            break;

        case CPSUICB_REASON_SETACTIVE:
            if((DWORD)(pCallbackParam->pOptItem[0].Sel) != pOEMDev->dwDriverData)
            {
                pCallbackParam->pOptItem[0].Sel = pOEMDev->dwDriverData;
                pCallbackParam->pOptItem[0].Flags |= OPTIF_CHANGED;
                lReturn = CPSUICB_ACTION_OPTIF_CHANGED;
            }
            break;

        default:
            break;
    }

    return lReturn;
}


////////////////////////////////////////////////////////////////////////////////
//
// Creates and Initializes OptItems.
//
static POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems)
{
    POPTITEM pOptItems      = NULL;
    SIZE_T   sizeOfOptItems = 0;

    // Compute the size of memory needed for the OptItems:
    if (SUCCEEDED(SIZETMult(sizeof(OPTITEM), (SIZE_T)dwOptItems, &sizeOfOptItems)))
    {
        // Allocate memory for OptItems;
        pOptItems = (POPTITEM) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeOfOptItems);
        if(NULL != pOptItems)
        {
            InitOptItems(pOptItems, dwOptItems);
        }
        else
        {
            ERR(ERRORTEXT("CreateOptItems() failed to allocate memory for OPTITEMs!\r\n"));
        }
    }
    else
    {
        ERR(ERRORTEXT("CreateOptItems() failed to compute necessary memory size for OPTITEMs!\r\n"));
    }

    return pOptItems;
}


////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems.
//
static void InitOptItems(POPTITEM pOptItems, DWORD dwOptItems)
{
    VERBOSE(DLLTEXT("InitOptItems() entry.\r\n"));

    // Zero out memory.
    memset(pOptItems, 0, sizeof(OPTITEM) * dwOptItems);

    // Set each OptItem's size, and Public DM ID.
    for(DWORD dwCount = 0; dwCount < dwOptItems; dwCount++)
    {
        pOptItems[dwCount].cbSize = sizeof(OPTITEM);
        pOptItems[dwCount].DMPubID = DMPUB_NONE;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Allocates and initializes OptType for OptItem.
//
static POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams)
{
    POPTTYPE    pOptType = NULL;


    VERBOSE(DLLTEXT("CreateOptType() entry.\r\n"));

    // Allocate memory from the heap for the OPTTYPE; the driver will take care of clean up.
    pOptType = (POPTTYPE) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OPTTYPE));
    if(NULL != pOptType)
    {
        // Initialize OPTTYPE.
        pOptType->cbSize = sizeof(OPTTYPE);
        pOptType->Count = wOptParams;

        // Allocate memory from the heap for the OPTPARAMs for the OPTTYPE.
        pOptType->pOptParam = (POPTPARAM) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, wOptParams * sizeof(OPTPARAM));
        if(NULL != pOptType->pOptParam)
        {
            // Initialize the OPTPARAMs.
            for(WORD wCount = 0; wCount < wOptParams; wCount++)
            {
                pOptType->pOptParam[wCount].cbSize = sizeof(OPTPARAM);
            }
        }
        else
        {
            ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTPARAMs!\r\n"));

            // Free allocated memory and return NULL.
            HeapFree(hHeap, 0, pOptType);
            pOptType = NULL;
        }
    }
    else
    {
        ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTTYPE!\r\n"));
    }

    return pOptType;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Retrieves pointer to a String resource.
//
static PTSTR GetStringResource(HANDLE hHeap, HANDLE hModule, UINT uResource)
{
    int     nResult;
    DWORD   dwSize = MAX_PATH;
    PTSTR   pszString = NULL;


    VERBOSE(DLLTEXT("GetStringResource(%#x, %#x, %d) entered.\r\n"), hHeap, hModule, uResource);

    // Allocate buffer for string resource from heap; let the driver clean it up.
    pszString = (PTSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(TCHAR));
    if(NULL != pszString)
    {
        // Load string resource; resize after loading so as not to waste memory.
        nResult = LoadString((HMODULE)hModule, uResource, pszString, dwSize);
        if(nResult > 0)
        {
            PTSTR   pszTemp;


            VERBOSE(DLLTEXT("LoadString() returned %d!\r\n"), nResult);
            VERBOSE(DLLTEXT("String load was \"%s\".\r\n"), pszString);

            pszTemp = (PTSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pszString, (nResult + 1) * sizeof(TCHAR));
            if(NULL != pszTemp)
            {
                pszString = pszTemp;
            }
            else
            {
                ERR(ERRORTEXT("GetStringResource() HeapReAlloc() of string retrieved failed! (Last Error was %d)\r\n"), GetLastError());
            }
        }
        else
        {
            ERR(ERRORTEXT("LoadString() returned %d! (Last Error was %d)\r\n"), nResult, GetLastError());
            ERR(ERRORTEXT("GetStringResource() failed to load string resource %d!\r\n"), uResource);

            pszString = NULL;
        }
    }
    else
    {
        ERR(ERRORTEXT("GetStringResource() failed to allocate string buffer!\r\n"));
    }

    return pszString;
}


