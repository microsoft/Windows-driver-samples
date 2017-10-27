//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    UNIUIREP.H
//
//  PURPOSE:    Define common data types, and external function prototypes
//                for UI replacement sample.
//
//--------------------------------------------------------------------------

#pragma once

#define DLLTEXT(s)      TEXT("[UIREPPLUG]  ") TEXT(s)
#define FUNCTEXT(s)     _T(__FUNCTION__) TEXT(s)
#define ERRORTEXT(s)    TEXT("ERROR ") DLLTEXT(s)

//
// OEM Signature is used to identify the 
// private DEVMODE portion to mitigate against
// data corruption.  Replace this with a unique string
// for your organization.
//
#define OEM_SIGNATURE               'UIRP'
#define OEM_VERSION                 0x00000001L

//
// OEM Signature and version.
// 
#define PROP_TITLE                  L"UIRepPlug Page"

//
// Printer registry keys where OEM data is stored.
// 
#define UIREPPLUG_VALUE             TEXT("UIREPPLUG_VALUE")
#define UIREPPLUG_DEVICE_VALUE      TEXT("UIREPPLUG_DEVICE_VALUE")

//
// Method prototypes
//

HRESULT
hrCommonPropSheetMethod(
    PPROPSHEETUI_INFO pPSUIInfo,
    LPARAM lParam,
    IPrintCoreHelper* pHelper,
    CFeatureCollection* pFeatures,
    DWORD dwMode
    );

