//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    OEMUI.cpp
//
//
//  PURPOSE:  Main file for OEM UI test module.
//

#include "precomp.h"
#include "resource.h"
#include "debug.h"
#include "globals.h"
#include "devmode.h"
#include "stringutils.h"
#include "helper.h"
#include "features.h"
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
    CUIHelper       *pHelper;
    POEMUIOBJ       poemuiobj;
    BOOL            bPermission;
    BOOL            bHidingStandardUI;

} CBUSERDATA, *PCBUSERDATA;



////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static HRESULT hrDocumentPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMPrinterUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUIItemCallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam);
LONG APIENTRY OEMDevUICallBack(PCPSUICBPARAM pCallbackParam);
INT_PTR CALLBACK DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
static void InitOptItems(POPTITEM pOptItems, DWORD dwOptItems);




////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM device or document property UI.
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
    HRESULT hrResult = S_OK;

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
        pOEMUIParam->OEMCUIPCallback = OEMDocUIItemCallBack;

        // Init OEMOptItmes.
        InitOptItems(pOEMUIParam->pOEMOptItems, pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.

        // New section.
        pOEMUIParam->pOEMOptItems[0].Level = 1;
        pOEMUIParam->pOEMOptItems[0].Flags = OPTIF_COLLAPSE;
        pOEMUIParam->pOEMOptItems[0].Sel = pOEMDev->dwAdvancedData;

        hrResult = GetStringResource(pOEMUIParam->hOEMHeap,
                                     (HMODULE) pOEMUIParam->hModule,
                                     IDS_ADV_SECTION,
                                     &pOEMUIParam->pOEMOptItems[0].pName);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("hrDocumentPropertyPage() failed to get section name. (hrResult = 0x%x)\r\n"),
                          hrResult);
            goto Exit;
        }

        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);

        pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;
    }


Exit:

    return hrResult;
}

////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM printer property UI.
//
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    HRESULT hrResult = S_OK;

    UNREFERENCED_PARAMETER(dwMode);

    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Fill in the number of OptItems to create for OEM printer property UI.
        pOEMUIParam->cOEMOptItems = 1;

        VERBOSE(DLLTEXT("hrPrinterPropertyPage() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
        DWORD   dwError;
        DWORD   dwDeviceValue;
        DWORD   dwType;
        DWORD   dwNeeded;


        VERBOSE(DLLTEXT("hrPrinterPropertyPage() fill out %d items.\r\n"), pOEMUIParam->cOEMOptItems);

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
        hrResult = GetStringResource(pOEMUIParam->hOEMHeap,
                                     (HMODULE) pOEMUIParam->hModule,
                                     IDS_DEV_SECTION,
                                     &pOEMUIParam->pOEMOptItems[0].pName);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("hrPrinterPropertyPage() failed to get section name. (hrResult = 0x%x)\r\n"),
                          hrResult);
            goto Exit;
        }

        pOEMUIParam->pOEMOptItems[0].Sel = dwDeviceValue;

        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);

        pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;
    }

Exit:

    return hrResult;
}


////////////////////////////////////////////////////////////////////////////////
//
// Adds property page to Document property sheet.
//
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo,
                                    LPARAM lParam,
                                    CUIHelper &Helper,
                                    CFeatures *pFeatures,
                                    BOOL bHidingStandardUI)
{
    LONG_PTR    lResult = 0;
    HRESULT     hrResult = S_OK;

    VERBOSE(DLLTEXT("OEMDocumentPropertySheets() entry.\r\n"));

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR(ERRORTEXT("OEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return  E_FAIL;
    }

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                WORD            wFeatures       = 0;
                WORD            wIndex          = 0;
                DWORD           dwSheets        = 0;
                PCBUSERDATA     pUserData       = NULL;
                POEMUIPSPARAM   pOEMUIParam     = (POEMUIPSPARAM) pPSUIInfo->lParamInit;
                BOOL            bPermission     = ((pOEMUIParam->dwFlags & DM_NOPERMISSION) == 0);
                HANDLE          hHeap           = pOEMUIParam->hOEMHeap;
                POEMDEV         pOEMDev         = (POEMDEV) pOEMUIParam->pOEMDM;
                COMPROPSHEETUI  Sheet;


                // Make sure that we have the Core Driver Features.
                // Only get features if we are hiding the standard
                // document property sheets.
                if(bHidingStandardUI)
                {
                    pFeatures->Acquire(hHeap, Helper, pOEMUIParam->poemuiobj);
                    wFeatures = pFeatures->GetCount(OEMCUIP_DOCPROP);
                }

                // Init property page.
                memset(&Sheet, 0, sizeof(COMPROPSHEETUI));
                Sheet.cbSize            = sizeof(COMPROPSHEETUI);
                Sheet.Flags             = bPermission ? CPSUIF_UPDATE_PERMISSION : 0;
                Sheet.hInstCaller       = ghInstance;
                Sheet.pHelpFile         = NULL;
                Sheet.pfnCallBack       = OEMDocUICallBack;
                Sheet.pDlgPage          = CPSUI_PDLGPAGE_TREEVIEWONLY;
                Sheet.cOptItem          = wFeatures + 1;
                Sheet.IconID            = IDI_CPSUI_PRINTER;
                Sheet.CallerVersion     = 0x100;
                Sheet.OptItemVersion    = 0x100;

                // Get Caller's name.
                hrResult = GetStringResource(hHeap, ghInstance, IDS_NAME, &Sheet.pCallerName);
                if(!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to get caller name. (hrResult = 0x%x)\r\n"),
                                  hrResult);
                    goto Exit;
                }

                // Get section name.
                hrResult = GetStringResource(hHeap, ghInstance, IDS_SECTION, &Sheet.pOptItemName);
                if(!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to get section name. (hrResult = 0x%x)\r\n"),
                                  hrResult);
                    goto Exit;
                }

                // Init user data.
                pUserData = (PCBUSERDATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CBUSERDATA));
                if(NULL == pUserData)
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to allocate user data.\r\n"));

                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }
                pUserData->hComPropSheet        = pPSUIInfo->hComPropSheet;
                pUserData->pfnComPropSheet      = pPSUIInfo->pfnComPropSheet;
                pUserData->pOEMUIParam          = pOEMUIParam;
                pUserData->pHelper              = &Helper;
                pUserData->poemuiobj            = pOEMUIParam->poemuiobj;
                pUserData->bPermission          = bPermission;
                pUserData->bHidingStandardUI    = bHidingStandardUI;
                Sheet.UserData                  = (ULONG_PTR) pUserData;

                // Create OptItems for page.
                Sheet.pOptItem = CreateOptItems(hHeap, Sheet.cOptItem);
                if(NULL == Sheet.pOptItem)
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to allocate OPTITEMs.\r\n"));

                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                // Add Core Driver features.
                for(wIndex = 0; wIndex < wFeatures; ++wIndex)
                {
                    // Initialize level and basic state for feature.
                    Sheet.pOptItem[wIndex].Level   = 1;
                    Sheet.pOptItem[wIndex].Flags   = OPTIF_COLLAPSE;

                    // Get the OPTITEM for this feature.
                    hrResult = pFeatures->InitOptItem(hHeap,
                                                      Sheet.pOptItem + wIndex,
                                                      wIndex,
                                                      OEMCUIP_DOCPROP);
                    if(!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to get OPTITEM for feature %hs.\r\n"),
                                      pFeatures->GetKeyword(wIndex, OEMCUIP_DOCPROP));

                        goto Exit;
                    }
                }

                // Initialize Plug-in OptItems
                Sheet.pOptItem[wIndex].Level   = 1;
                Sheet.pOptItem[wIndex].Flags   = OPTIF_COLLAPSE;
                Sheet.pOptItem[wIndex].Sel     = pOEMDev->dwDriverData;

                // get optitem name.
                hrResult = GetStringResource(hHeap, ghInstance, IDS_SECTION, &Sheet.pOptItem[wIndex].pName);
                if(!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrOEMDocumentPropertySheets() failed to get OptItem %d name. (hrResult = 0x%x)\r\n"),
                                  wIndex,
                                  hrResult);
                    goto Exit;
                }

                Sheet.pOptItem[wIndex].pOptType = CreateOptType(hHeap, 2);

                Sheet.pOptItem[wIndex].pOptType->Type                  = TVOT_UDARROW;
                Sheet.pOptItem[wIndex].pOptType->pOptParam[1].IconID   = 0;
                Sheet.pOptItem[wIndex].pOptType->pOptParam[1].lParam   = 100;


                // Add property sheets.
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
// Adds property page to printer property sheet.
//
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo,
                                  LPARAM lParam,
                                  CUIHelper &Helper,
                                  CFeatures *pFeatures,
                                  BOOL bHidingStandardUI)
{
    LONG_PTR    lResult = 0;
    HRESULT     hrResult = S_OK;


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

                // If hiding standard UI, then
                // need to add Device Settings page, too.
                if(bHidingStandardUI)
                {
                    POEMUIPSPARAM   pOEMUIParam     = (POEMUIPSPARAM) pPSUIInfo->lParamInit;
                    BOOL            bPermission     = ((pOEMUIParam->dwFlags & DPS_NOPERMISSION) == 0);
                    HANDLE          hHeap           = pOEMUIParam->hOEMHeap;
                    WORD            wFeatures       = 0;
                    WORD            wIndex          = 0;
                    DWORD           dwSheets        = 0;
                    DLGPAGE         DlgPage;
                    PCBUSERDATA     pUserData       = NULL;
                    COMPROPSHEETUI  Sheet;


                    // Make sure that we have the Core Driver Features.
                    pFeatures->Acquire(hHeap, Helper, pOEMUIParam->poemuiobj);
                    wFeatures = pFeatures->GetCount(OEMCUIP_PRNPROP);

                    // Init DlgPage struct for Device Settings replacement page.
                    memset(&DlgPage, 0, sizeof(DLGPAGE));
                    DlgPage.cbSize          = sizeof(DLGPAGE);
                    DlgPage.DlgTemplateID   = DP_STD_TREEVIEWPAGE;

                    // Get Device Settings display name.
                    hrResult = GetStringResource(hHeap, ghInstance, IDS_DEVICE_SETTINGS_NAME, &DlgPage.pTabName);
                    if(!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to get Device Settings display name. (hrResult = 0x%x)\r\n"),
                                      hrResult);
                        goto Exit;
                    }

                    // Init Device Settings replacement page.
                    memset(&Sheet, 0, sizeof(COMPROPSHEETUI));
                    Sheet.cbSize            = sizeof(COMPROPSHEETUI);
                    Sheet.Flags             = bPermission ? CPSUIF_UPDATE_PERMISSION : 0;
                    Sheet.hInstCaller       = ghInstance;
                    Sheet.pHelpFile         = NULL;
                    Sheet.pfnCallBack       = OEMDevUICallBack;
                    Sheet.pDlgPage          = &DlgPage; //CPSUI_PDLGPAGE_TREEVIEWONLY;
                    Sheet.cOptItem          = wFeatures;
                    Sheet.cDlgPage          = 1;
                    Sheet.IconID            = IDI_CPSUI_PRINTER;
                    Sheet.CallerVersion     = 0x100;
                    Sheet.OptItemVersion    = 0x100;

                    // Get Caller's name.
                    hrResult = GetStringResource(hHeap, ghInstance, IDS_NAME, &Sheet.pCallerName);
                    if(!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to get caller name. (hrResult = 0x%x)\r\n"),
                                      hrResult);
                        goto Exit;
                    }

                    // Get section name.
                    hrResult = GetStringResource(hHeap, ghInstance, IDS_SECTION, &Sheet.pOptItemName);
                    if(!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to get section name. (hrResult = 0x%x)\r\n"),
                                      hrResult);
                        goto Exit;
                    }

                    // Allocate and init User data used in our callback for this page.
                    pUserData = (PCBUSERDATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CBUSERDATA));
                    if(NULL == pUserData)
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to allocate user data.\r\n"));

                        hrResult = E_OUTOFMEMORY;
                        goto Exit;
                    }
                    pUserData->hComPropSheet        = pPSUIInfo->hComPropSheet;
                    pUserData->pfnComPropSheet      = pPSUIInfo->pfnComPropSheet;
                    pUserData->pOEMUIParam          = pOEMUIParam;
                    pUserData->pHelper              = &Helper;
                    pUserData->poemuiobj            = pOEMUIParam->poemuiobj;
                    pUserData->bPermission          = bPermission;
                    pUserData->bHidingStandardUI    = bHidingStandardUI;
                    Sheet.UserData                  = (ULONG_PTR) pUserData;

                    // Create OptItems for page.
                    Sheet.pOptItem = CreateOptItems(hHeap, Sheet.cOptItem);
                    if(NULL == Sheet.pOptItem)
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to allocate OPTITEMs.\r\n"));

                        hrResult = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    // Add Core Driver features.
                    for(wIndex = 0; wIndex < wFeatures; ++wIndex)
                    {
                        // Initialize level and basic state for feature.
                        Sheet.pOptItem[wIndex].Level   = 1;
                        Sheet.pOptItem[wIndex].Flags   = OPTIF_COLLAPSE;

                        // Get the OPTITEM for this feature.
                        hrResult = pFeatures->InitOptItem(hHeap,
                                                          Sheet.pOptItem + wIndex,
                                                          wIndex,
                                                          OEMCUIP_PRNPROP);
                        if(!SUCCEEDED(hrResult))
                        {
                            ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to get OPTITEM for feature %hs.\r\n"),
                                          pFeatures->GetKeyword(wIndex, OEMCUIP_PRNPROP));

                            goto Exit;
                        }
                    }

                    // Add property sheets.
                    lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PCOMPROPSHEETUI,
                                                         (LPARAM)&Sheet, (LPARAM)&dwSheets);
                    if(!SUCCEEDED(lResult))
                    {
                        ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to add Device Settings replacement page. (lResult = 0x%x)\r\n"),
                                      lResult);
                        goto Exit;
                    }
                }

                // Init our property page.
                memset(&Page, 0, sizeof(PROPSHEETPAGE));
                Page.dwSize         = sizeof(PROPSHEETPAGE);
                Page.dwFlags        = PSP_DEFAULT;
                Page.hInstance      = ghInstance;
                Page.pszTemplate    = MAKEINTRESOURCE(IDD_DEVICE_PROPPAGE);
                Page.pfnDlgProc     = DevicePropPageProc;

                // Add property sheets.
                lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PROPSHEETPAGE, (LPARAM)&Page, 0);
                if(!SUCCEEDED(lResult))
                {
                    ERR(ERRORTEXT("hrOEMDevicePropertySheets() failed to add our Device Property page. (lResult = 0x%x)\r\n"),
                                  lResult);
                    goto Exit;
                }

                VERBOSE(DLLTEXT("hrOEMDevicePropertySheets() pfnComPropSheet returned %d.\r\n"), lResult);
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

                // Store OptItems state in printer data.
                SetPrinterData(pOEMUIParam->hPrinter, OEMUI_VALUE, REG_DWORD, (PBYTE) &dwDriverValue, sizeof(DWORD));
            }
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
#pragma warning(push)
// 2 MAX_PATH strings just slightly exceed the default threshold for the prefast stack usage warning
#pragma warning( disable : 6262 ) 
INT_PTR CALLBACK DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDC_CALIBRATE:
                            // Just display a message that the printer is calibrated,
                            // since we don't acutally calibrate anything.
                            {
                                TCHAR   szName[MAX_PATH];
                                TCHAR   szCalibrated[MAX_PATH];


                                LoadString(ghInstance, IDS_NAME, szName, sizeof(szName)/sizeof(szName[0]));
                                LoadString(ghInstance, IDS_CALIBRATED, szCalibrated, sizeof(szCalibrated)/sizeof(szCalibrated[0]));
                                MessageBox(hDlg, szCalibrated, szName, MB_OK);
                            }
                            break;
                    }
                    break;

                default:
                    return FALSE;
            }
            return TRUE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)  // type of notification message
                {
                    case PSN_SETACTIVE:
                        break;

                    case PSN_KILLACTIVE:
                        break;

                    case PSN_APPLY:
                        break;

                    case PSN_RESET:
                        break;
                }
            }
            break;
    }

    return FALSE;
}
#pragma warning(pop)

////////////////////////////////////////////////////////////////////////////////
//
// OptItems call back for OEM document property UI.
//
LONG APIENTRY OEMDocUIItemCallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    lReturn = CPSUICB_ACTION_NONE;
    POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUIItemCallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            // Store OptItems state in DEVMODE.
            pOEMDev->dwAdvancedData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        case CPSUICB_REASON_KILLACTIVE:
            pOEMDev->dwAdvancedData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        case CPSUICB_REASON_SETACTIVE:
            if((DWORD)(pOEMUIParam->pOEMOptItems[0].Sel) != pOEMDev->dwAdvancedData)
            {
                pOEMUIParam->pOEMOptItems[0].Sel    = pOEMDev->dwAdvancedData;
                pOEMUIParam->pOEMOptItems[0].Flags |= OPTIF_CHANGED;
                lReturn                             = CPSUICB_ACTION_OPTIF_CHANGED;
            }
            break;

        default:
            break;
    }

    return lReturn;
}


LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam)
{
    WORD            wItems      = pCallbackParam->cOptItem;
    LONG            lReturn     = CPSUICB_ACTION_NONE;
    POPTITEM        pOptItem    = pCallbackParam->pOptItem;
    PCBUSERDATA     pUserData   = (PCBUSERDATA) pCallbackParam->UserData;
    HANDLE          hHeap       = pUserData->pOEMUIParam->hOEMHeap;
    POEMDEV         pOEMDev     = (POEMDEV) pUserData->pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    //
    // If user has no permission to change anything, then
    // simply return without taking any action.
    //

    if (!pUserData->bPermission && (pCallbackParam->Reason != CPSUICB_REASON_ABOUT))
        return CPSUICB_ACTION_NONE;

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            if(wItems > 0)
            {
                // Save feature options if hidig standard UI.
                if(pUserData->bHidingStandardUI)
                {
                    HRESULT hrResult;


                    // Save feature OPTITEMs.
                    hrResult = SaveFeatureOptItems(hHeap,
                                                   pUserData->pHelper,
                                                   pUserData->poemuiobj,
                                                   pCallbackParam->hDlg,
                                                   pOptItem,
                                                   wItems);
                    if(!SUCCEEDED(hrResult))
                    {
                        // Return that we didn't save changes.
                        // NOTE: it is up to SaveFeatureOptItems() to display
                        //       any UI for failure.
                        return CPSUICB_ACTION_NO_APPLY_EXIT;
                    }
                }

                // Save OPTITEM that we explicitly added.
                pOEMDev->dwDriverData = pOptItem[wItems - 1].Sel;
                pUserData->pfnComPropSheet(pUserData->hComPropSheet, CPSFUNC_SET_RESULT,
                                           (LPARAM)pUserData->hPropPage,
                                           (LPARAM)CPSUI_OK);

            }
            break;

        case CPSUICB_REASON_KILLACTIVE:
            if(wItems > 0)
            {
                // Update OPTITEM that we explicitly added.
                pOEMDev->dwDriverData = pOptItem[wItems - 1].Sel;
            }
            break;

        case CPSUICB_REASON_SETACTIVE:
            if(wItems > 0)
            {
                if((DWORD)(pOptItem[wItems - 1].Sel) != pOEMDev->dwDriverData)
                {
                    // Update OPTITEM that we explicitly added.
                    pOptItem[wItems - 1].Sel     = pOEMDev->dwDriverData;
                    pOptItem[wItems - 1].Flags  |= OPTIF_CHANGED;
                }
            }
            lReturn = CPSUICB_ACTION_OPTIF_CHANGED;
            break;

        default:
            break;
    }

    return lReturn;
}

LONG APIENTRY OEMDevUICallBack(PCPSUICBPARAM pCallbackParam)
{
    WORD            wItems      = pCallbackParam->cOptItem;
    LONG            lReturn     = CPSUICB_ACTION_NONE;
    POPTITEM        pOptItem    = pCallbackParam->pOptItem;
    PCBUSERDATA     pUserData   = (PCBUSERDATA) pCallbackParam->UserData;
    HANDLE          hHeap       = pUserData->pOEMUIParam->hOEMHeap;


    VERBOSE(DLLTEXT("OEMDevUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    //
    // If user has no permission to change anything, then
    // simply return without taking any action.
    //

    if (!pUserData->bPermission && (pCallbackParam->Reason != CPSUICB_REASON_ABOUT))
        return CPSUICB_ACTION_NONE;

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            if(wItems > 0)
            {
                // Save feature options if hidig standard UI.
                if(pUserData->bHidingStandardUI)
                {
                    HRESULT hrResult;


                    // Save feature OPTITEMs.
                    hrResult = SaveFeatureOptItems(hHeap,
                                                   pUserData->pHelper,
                                                   pUserData->poemuiobj,
                                                   pCallbackParam->hDlg,
                                                   pOptItem,
                                                   wItems);
                    if(!SUCCEEDED(hrResult))
                    {
                        // Return that we didn't save changes.
                        // NOTE: it is up to SaveFeatureOptItems() to display
                        //       any UI for failure.
                        return CPSUICB_ACTION_NO_APPLY_EXIT;
                    }
                }


                pUserData->pfnComPropSheet(pUserData->hComPropSheet, CPSFUNC_SET_RESULT,
                                           (LPARAM)pUserData->hPropPage,
                                           (LPARAM)CPSUI_OK);
            }
            break;

        case CPSUICB_REASON_KILLACTIVE:
            break;

        case CPSUICB_REASON_SETACTIVE:
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
POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems)
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
POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams)
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


