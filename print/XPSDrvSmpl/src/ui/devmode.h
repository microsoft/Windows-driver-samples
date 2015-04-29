/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   devmode.h

Abstract:

   Definition of the OEM devmode structure.

--*/

#pragma once


////////////////////////////////////////////////////////
//      OEM Devmode Defines
////////////////////////////////////////////////////////

#define MAX_WATERMARK_TEXT 24

////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

//
//Can add info to the private devmode bellow here.
//Note :
//      This structure must be prefixed by OEM_DMEXTRAHEADER
//      Your plug-in must implement the IPrintOemUI::DevMode method
//
typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER dmOEMExtra;

    //
    //Private DevMode Members
    //

    //
    // Page Scaling Members
    //

    DWORD   dwPgScaleX;
    DWORD   dwPgScaleY;
    INT     iPgOffsetX;
    INT     iPgOffsetY;

    //
    // Watermark Members
    //
    INT     iWMTransparency;
    INT     iWMAngle;
    INT     iWMOffsetX;
    INT     iWMOffsetY;

    //
    // Text Watermark Members
    //
    INT     iWMFontSize;
    DWORD   dwColText;
    TCHAR   strWMText[MAX_WATERMARK_TEXT];

    //
    // Bitmap / Vector Members
    //
    INT     iWMWidth;
    INT     iWMHeight;

} OEMDEV, *POEMDEV;

typedef CONST OEMDEV *PCOEMDEV;

