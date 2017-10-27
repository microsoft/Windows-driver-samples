//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Devmode.h
//    
//
//  PURPOSE:    Define common data types, and external function prototypes
//              for devmode functions.
//
//  History: 
//          06/24/03    xxx created.
//
//

#pragma once


////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER       dmOEMExtra;
    BOOL                    dwDriverData;
} OEMDEV, *POEMDEV;

typedef const OEMDEV *PCOEMDEV;


/////////////////////////////////////////////////////////
//      Prototypes
/////////////////////////////////////////////////////////

HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam);
BOOL bConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut);
BOOL bMakeOEMDevmodeValid(POEMDEV pOEMDevmode);


