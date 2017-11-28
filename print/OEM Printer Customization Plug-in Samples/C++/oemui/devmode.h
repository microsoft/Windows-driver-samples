//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Devmode.h
//    
//  PURPOSE:    Define common data types, and external function prototypes
//              for devmode functions.
//

#pragma once

////////////////////////////////////////////////////////
//      OEM Devmode Defines
////////////////////////////////////////////////////////

#define OEM_SIGNATURE   'MSFT'
#define OEM_VERSION     0x00000001L

////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

//
// This structure must be prefixed by OEM_DMEXTRAHEADER
//    Plug-ins must implement the IPrintOemUI::DevMode method
//
typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER   dmOEMExtra;
    DWORD               dwDriverData;
    DWORD               dwAdvancedData;

    //
    //Private DevMode Members
    //

} OEMDEV, *POEMDEV;

typedef const OEMDEV *PCOEMDEV;


/////////////////////////////////////////////////////////
//        ProtoTypes
/////////////////////////////////////////////////////////

HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam);
BOOL ConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut);
BOOL MakeOEMDevmodeValid(POEMDEV pOEMDevmode);
