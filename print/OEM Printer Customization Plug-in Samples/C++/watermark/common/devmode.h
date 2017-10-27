//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Devmode.h
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for devmode functions.
//
#pragma once 

////////////////////////////////////////////////////////
//      OEM Devmode Defines
////////////////////////////////////////////////////////

#define WATER_MARK_TEXT_SIZE            128
#define WATER_MARK_DEFAULT_ENABLED      TRUE
#define WATER_MARK_DEFAULT_ROTATION     30
#define WATER_MARK_DEFAULT_FONTSIZE     28
#define WATER_MARK_DEFAULT_COLOR        RGB(230, 230, 230)
#define WATER_MARK_DEFAULT_TEXT         L"WaterMark"



////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER   dmOEMExtra;
    BOOL                bEnabled;
    DOUBLE              dfRotate;
    DWORD               dwFontSize;
    COLORREF            crTextColor;
    WCHAR               szWaterMark[WATER_MARK_TEXT_SIZE];

} OEMDEV, *POEMDEV;

typedef const OEMDEV *PCOEMDEV;



/////////////////////////////////////////////////////////
//		ProtoTypes
/////////////////////////////////////////////////////////

HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam);
BOOL ConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut);
BOOL MakeOEMDevmodeValid(POEMDEV pOEMDevmode);
BOOL IsValidFontSize(DWORD dwFontSize);

