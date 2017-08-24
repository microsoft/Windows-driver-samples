//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    uniuirep.cpp
//    
//  PURPOSE:  This module handles COMPSTUI initialization and 
//            callback behavior.
//
//--------------------------------------------------------------------------


#include "precomp.h"

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
    IPrintCoreHelper* pHelper;
    BOOL bPermission;

} CBUSERDATA, *PCBUSERDATA;


////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

LONG APIENTRY 
OEMCommonUICallBack(
    _In_ PCPSUICBPARAM pCallbackParam
    );

POPTITEM
CreateOptItems(
    HANDLE hHeap,
    DWORD dwOptItems
    );


//+---------------------------------------------------------------------------
//
//  Member:
//      ::hrCommonPropSheetMethod
//
//  Synopsis:
//      Adds property page to printer property sheet.
//
//  Returns:
//      S_OK on success, E_FAIL on failure.  Sets LastError.
//
//
//----------------------------------------------------------------------------
HRESULT
hrCommonPropSheetMethod(
    PPROPSHEETUI_INFO pPSUIInfo,
        // Property sheet information, including
        // the reason for calling this function.
    LPARAM lParam,
        // Optional parameter.  Value is determined
        // by the reason member of pPSUIInfo.
    IPrintCoreHelper* pHelper,
        // Pointer to the helper interface used to control
        // settings described in the GPD
    CFeatureCollection* pFeatures,
        // Collection of all features that we're supporting
        // in our custom UI.
    DWORD dwMode
        // Should our sheet show device or document features?
    )
{
    LONG_PTR lResult = TRUE;
        // result supplied to COMPSTUI via pPSUIInfo

    HRESULT hrResult = S_OK;
        // Result returned to the immediate caller.

    VERBOSE(DLLTEXT("hrCommonPropSheetMethod entry."));

    //
    // Validate parameters.
    //
    if ((NULL == pPSUIInfo) ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version))
    {
        ERR(ERRORTEXT("hrCommonPropSheetMethod() ERROR_INVALID_PARAMETER.\r\n"));

        //
        // Return invalid parameter error.
        // 
        SetLastError(ERROR_INVALID_PARAMETER);
        hrResult = E_FAIL;
        goto Exit;
    }

    Dump(pPSUIInfo);

    //
    // Do action.
    // 
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                WORD wFeatures = 0;
                DWORD dwSheets = 0;
                PCBUSERDATA pUserData = NULL;
                POEMUIPSPARAM pOEMUIParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;
                BOOL bPermission = 0;
                HANDLE hHeap = pOEMUIParam->hOEMHeap;
                COMPROPSHEETUI Sheet;

                //
                // If the user doesn't have permission to change settings,
                // make a note of it so we can disable controls later.
                //
                if (dwMode == OEMCUIP_PRNPROP)
                {
                    bPermission = ((pOEMUIParam->dwFlags & DPS_NOPERMISSION)==0);
                }
                else
                {
                    bPermission = ((pOEMUIParam->dwFlags & DM_NOPERMISSION)==0);
                }

                //
                // Make sure that we have the core driver features
                // 
                hrResult = pFeatures->Acquire(hHeap, pHelper);
                if (!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to load core driver features! (hrResult = 0x%x)\r\n"),
                                hrResult);
                    goto Exit;
                }

                //
                // Wait until after the call to Acquire to read the number of features.
                //
                wFeatures = (WORD)pFeatures->GetCount(dwMode);

                //
                // Init device settings replacement page
                // 
                memset(&Sheet, 0, sizeof(COMPROPSHEETUI));
                Sheet.cbSize            = sizeof(COMPROPSHEETUI);
                Sheet.Flags             = bPermission ? CPSUIF_UPDATE_PERMISSION : 0;
                Sheet.hInstCaller       = ghInstance;
                Sheet.pHelpFile         = NULL;
                Sheet.pfnCallBack       = OEMCommonUICallBack;
                Sheet.cOptItem          = wFeatures;
                Sheet.IconID            = IDI_CPSUI_PRINTER;
                Sheet.CallerVersion     = 0x100;
                Sheet.OptItemVersion    = 0x100;

                //
                // Initialize mode-specific fields
                // 
                if (dwMode == OEMCUIP_PRNPROP)
                {
                    // 
                    // Use a DlgPage structure to set the text in the tab
                    // in the printer propertues view.
                    //
                    DLGPAGE DlgPage;
                    memset(&DlgPage, 0, sizeof(DLGPAGE));
                    DlgPage.cbSize = sizeof(DLGPAGE);
                    DlgPage.DlgTemplateID = DP_STD_TREEVIEWPAGE;

                    //
                    // Get device settings display name
                    // 
                    hrResult = GetStringResource(hHeap, ghInstance, IDS_DEVICE_SETTINGS_NAME, &DlgPage.pTabName);

                    if (!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to get tab name! (hrResult = 0x%x)\r\n"),
                                    hrResult);
                        goto Exit;
                    }
                    Sheet.pDlgPage          = &DlgPage;
                    Sheet.cDlgPage          = 1;
                }
                else
                {
                    //
                    // printing preferences settings
                    //
                    Sheet.pDlgPage          = CPSUI_PDLGPAGE_TREEVIEWONLY;
                }

                //
                // Get caller name
                // 
                hrResult = GetStringResource(hHeap, ghInstance, IDS_NAME, &Sheet.pCallerName);
                if (!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to get caller name! (hrResult = 0x%x)\r\n"), hrResult);
                    goto Exit;
                }

                //
                // Get section name
                // 
                hrResult = GetStringResource(hHeap, ghInstance, IDS_SECTION, &Sheet.pOptItemName);
                if (!SUCCEEDED(hrResult))
                {
                    ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to get section name! (hrResult = 0x%x)\r\n"), hrResult);
                    goto Exit;
                }

                //
                // Init user data.
                // 
                pUserData = (PCBUSERDATA)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CBUSERDATA));

                if (pUserData == NULL)
                {
                    ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to allocate user data!\r\n"));
                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                pUserData->hComPropSheet    = pPSUIInfo->hComPropSheet;
                pUserData->pfnComPropSheet  = pPSUIInfo->pfnComPropSheet;
                pUserData->pOEMUIParam      = pOEMUIParam;
                pUserData->pHelper          = pHelper;
                pUserData->bPermission      = bPermission;
                Sheet.UserData = (ULONG_PTR)pUserData;

                //
                // Create OptItems for page.
                // 
                Sheet.pOptItem = CreateOptItems(hHeap, Sheet.cOptItem);

                if (Sheet.pOptItem == NULL)
                {
                    ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to allocate OPTITEMS!\r\n"));
                    hrResult = E_OUTOFMEMORY;
                    goto Exit;
                }

                //
                // Add core driver features
                // 
                for (DWORD wIndex = 0; wIndex < wFeatures; wIndex++)
                {
                    //
                    // Initialize level and basic state for feature
                    // 
                    Sheet.pOptItem[wIndex].Level = 1;
                    Sheet.pOptItem[wIndex].Flags = OPTIF_COLLAPSE;
                    CFeature *pFeature = pFeatures->GetFeature(wIndex, dwMode);

                    if (pFeature == NULL)
                    {
                        ERR(ERRORTEXT("hrCommonPropSheetMethod: Feature not found or features not initialized!\r\n"));
                        hrResult = E_UNEXPECTED;
                        goto Exit;
                    }

                    //
                    // Get OPTITEM for this feature
                    // 
                    hrResult = pFeature->InitOptItem(hHeap, Sheet.pOptItem + wIndex);

                    if (!SUCCEEDED(hrResult))
                    {
                        ERR(ERRORTEXT("hrCommonPropSheetMethod: Failed to get OPTITEM for feature %s\r\n"), pFeature->GetKeyword());
                        goto Exit;
                    }

                }

                //
                // Add property sheets.
                // 
                lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                     CPSFUNC_ADD_PCOMPROPSHEETUI,
                                                     (LPARAM)&Sheet,
                                                     (LPARAM)&dwSheets);


                if (!lResult)
                {
                    ERR(DLLTEXT("hrCommonPropSheetMethod() pfnComPropSheet returned %d.\r\n"), lResult);
                    ERR(ERRORTEXT("ComPropSheet failed! Failed to add device settings tab!\n"));
                }
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                //
                // Set header title
                // 
                PPROPSHEETUI_INFO_HEADER pHeader = (PPROPSHEETUI_INFO_HEADER)lParam;
                pHeader->pTitle = (LPTSTR)PROP_TITLE;
                lResult = TRUE;
            }
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            //
            // No icon
            // 
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO)lParam;
                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            break;
    }

    pPSUIInfo->Result = lResult;

Exit:

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      OEMCommonUICallBack
//
//  Synopsis:
//      Callback function to handle changes to the feature list panel
//      added by this plug-in to the document properties UI.
//
//  Returns:
//      CPSUICB_ACTION_NONE if nothing further needs done.
//      CPSUICB_ACTION_NO_APPLY_EXIT if settings changes were aborted
//
//
//----------------------------------------------------------------------------
LONG APIENTRY 
OEMCommonUICallBack(
    _In_ PCPSUICBPARAM pCallbackParam
        // struct containing all of the relevant callback parameters,
        // including the reason that the callback is being made.
    )
{
    VERBOSE(DLLTEXT("OEMCommonUICallBack entry."));

    WORD wItems = pCallbackParam->cOptItem;
    LONG lReturn = CPSUICB_ACTION_NONE;
    POPTITEM pOptItem = pCallbackParam->pOptItem;
    PCBUSERDATA pUserData = (PCBUSERDATA) pCallbackParam->UserData;
    HANDLE hHeap = pUserData->pOEMUIParam->hOEMHeap;
    HRESULT hrResult = S_OK;

    VERBOSE(DLLTEXT("Reason = %d"), pCallbackParam->Reason);

    //
    // If user has no permission to change anything, then simply 
    // return without any action
    // 
    if (!pUserData->bPermission && 
        (pCallbackParam->Reason != CPSUICB_REASON_ABOUT))
    {
        WARNING(DLLTEXT("OEMCommonUICallBack: User does not have appropriate permission!\r\n"));
        lReturn = CPSUICB_ACTION_NONE;
    }
    else
    {
        switch(pCallbackParam->Reason)
        {
            case CPSUICB_REASON_APPLYNOW:
                if (wItems > 0)
                {
                    //
                    // Save features if hiding std UI
                    // 
                    hrResult = SaveFeatureOptItems(hHeap,
                                        pUserData->pHelper,
                                        pCallbackParam->hDlg,
                                        pOptItem,
                                        wItems);

                    if (!SUCCEEDED(hrResult))
                    {
                        //
                        // Return that we didn't save changes.
                        // NOTE: It is upto SaveFeatureOptItems() to display any 
                        // UI for failure
                        // 
                        lReturn = CPSUICB_ACTION_NO_APPLY_EXIT;
                    }
                    else
                    {
                        pUserData->pfnComPropSheet(pUserData->hComPropSheet,
                                                CPSFUNC_SET_RESULT,
                                                (LPARAM)pUserData->hPropPage,
                                                (LPARAM)CPSUI_OK);
                    }
                }
                break;

            case CPSUICB_REASON_KILLACTIVE:
                //
                // This active property sheet page is changing, and this page is 
                // going inactive.
                //
                break;

            case CPSUICB_REASON_SETACTIVE:
                //
                // This property sheet page is about to become the active page.
                //
            default:
                break;
        }
    }

    return lReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//      Creates and Initializes OptItems.
//
//  Returns:
//      Pointer to an array of optitems on success, else
//      NULL on failure.
//
//
//----------------------------------------------------------------------------
POPTITEM 
CreateOptItems(
    HANDLE hHeap, 
        // Heap to allocate data from
    DWORD dwOptItems
        // Number of opt items to allocate.
    )
{
    VERBOSE(DLLTEXT("CreateOptItems entry."));
       
    POPTITEM pOptItems = NULL;
        // OPTITEMs to be returned to caller.

    ULONG cbOptItems = 0;
        // Number of bytes needed for optitems.

    // Allocate memory for OptItems.
    if (SUCCEEDED(ULongMult( sizeof(OPTITEM), dwOptItems, &cbOptItems)) &&
        (pOptItems = (POPTITEM)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbOptItems)) != NULL)
    {
        // Zero out memory.
        memset(pOptItems, 0, sizeof(OPTITEM) * dwOptItems);

        // Set each OptItem's size, and Public DM ID.
        for (DWORD dwCount = 0; dwCount < dwOptItems; dwCount++)
        {
            pOptItems[dwCount].cbSize = sizeof(OPTITEM);
            pOptItems[dwCount].DMPubID = DMPUB_NONE;
        }
    }
    else
    {
        ERR(ERRORTEXT("CreateOptItems() failed to allocate memory for OPTITEMs!\r\n"));
    }

    return pOptItems;
}

