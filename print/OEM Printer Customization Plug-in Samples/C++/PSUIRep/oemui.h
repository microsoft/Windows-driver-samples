//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMUI.H
//    
#pragma once


////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////

// Printer registry keys where OEM data is stored.
#define OEMUI_VALUE             TEXT("OEMUI_VALUE")
#define OEMUI_DEVICE_VALUE      TEXT("OEMUI_DEVICE_VALUE")


////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, CUIHelper &Helper, CFeatures *pFeatures,
                                    BOOL bHidingStandardUI);
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, CUIHelper &Helper, CFeatures *pFeatures,
                                  BOOL bHidingStandardUI);

POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems);
POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams);
