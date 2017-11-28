//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMPS.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for oemps.cpp.
//
#pragma once


///////////////////////////////////////////////////////
// Warning: the following enum order must match the 
//          order in OEMHookFuncs[].
///////////////////////////////////////////////////////
typedef enum tag_Hooks {
    UD_DrvRealizeBrush,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvIcmCreateColorTransform,
    UD_DrvIcmDeleteColorTransform,
    UD_DrvQueryDeviceSupport,

    MAX_DDI_HOOKS,

} ENUMHOOKS;


typedef struct _OEMPDEV {
    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // This test DLL hooks out every drawing DDI. So it needs to remember
    // PS's hook function pointer so it call back.
    //
    PFN     pfnPS[MAX_DDI_HOOKS];

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    DWORD     dwReserved[1];

} OEMPDEV, *POEMPDEV;





