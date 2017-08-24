//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMUI.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for OEMUI Test Module.
//
//
#pragma once 

#include "OEM.H"
#include "DEVMODE.H"
#include "globals.h"

////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////


// OEM Signature and version.
#define PROP_TITLE      L"OEM UI Page"
#define DLLTEXT(s)      __TEXT("UI:  ") __TEXT(s)

// OEM UI Misc defines.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


// Printer registry keys where OEM data is stored.
#define OEMUI_VALUE             TEXT("OEMUI_VALUE")
#define OEMUI_DEVICE_VALUE      TEXT("OEMUI_DEVICE_VALUE")

//
//KeyWordNames for OPTITEMS (These are nonlocalised keywords assocaited with driver features. in the OPTITEMS)
//
const char DUPLEXUNIT[]		= "DuplexUnit";
const char ENVFEEDER[]		= "EnvFeeder";
const char PRINTERHDISK[]	= "PrinterHardDisk";

//
//This structure is used to link the common pages and the OEM plugin page.
//
typedef struct _OEMSHEETDATA {
	
	//
	//Any other OEM Data you may want to store.
	//

	
	//
	//Hold Pointers you may need.
	//
	HANDLE				hComPropSheet;		//Handle to the Prop Sheet Parent Page Used when calling ComPropSheet
	HANDLE				hmyPlugin;			//Handle to the OEM Plugin page. Page Used when calling ComPropSheet
	PFNCOMPROPSHEET		pfnComPropSheet;    //Is the Pointer to ComPropSheet This is needed when APPLY is clicked (CPSFUNC_SET_RESULT)

	POEMCUIPPARAM		pOEMCUIParam;		//Holds links to the DEVMODE and OPTITEMS needed.
	IPrintOemDriverUI*	pOEMHelp;

} OEMSHEETDATA, *POEMSHEETDATA;

////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, IPrintOemDriverUI*  pOEMHelp);
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, POEMSHEETDATA pOemSheetData);

