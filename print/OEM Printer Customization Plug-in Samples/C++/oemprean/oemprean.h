//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   OEMUNI.H
//    
//
//  PURPOSE:    Define the COemPDEV class which stores the private
//          PDEV for the driver.
//
//
//  Functions:
//          COemPDEV constructor and destructor
//          COemPDEV::InitializeDDITable
//          COemPDEV::_SavePFN
//          COemPDEV::_ResetPointers
//
//  History: 
//          06/24/03    xxx created.
//
//

#pragma once 

#include "debug.h"

#define OEM_SIGNATURE   'MSFT'
#define OEM_VERSION     0x00000001L

#define DLLTEXT(s)      TEXT("OEMUNI:  ") TEXT(s)
#define ERRORTEXT(s)    TEXT("ERROR ") DLLTEXT(s)

class COemPDEV
{
public:
    __stdcall COemPDEV(void)
    {

        // Note that since our parent has AddRef'd the UNIDRV interface,
        // we don't do so here since our scope is identical to our
        // parent.
        //

        VERBOSE(L"In COemPDEV constructor...");

        _ResetPointers();
    }

    __stdcall ~COemPDEV(void)
    {
        VERBOSE(L"In COemPDEV destructor...");
    }

    void __stdcall InitializeDDITable(DRVENABLEDATA* pded)
    {
        VERBOSE(L"COemPDEV::InitializeDDITable entry.");

        UINT   iDrvFn;
        UINT   cDrvFn = pded->c;
        PDRVFN pDrvFn = pded->pdrvfn;

        for (iDrvFn = 0; iDrvFn < cDrvFn; ++iDrvFn, ++pDrvFn)
        {
            _SavePFN(pDrvFn->iFunc, pDrvFn->pfn);
        }
    }

private:

    void __stdcall _SavePFN(ULONG uFunc, PFN pfn)
    {

        // Note: if a "pass-through" driver is being constructed, then
        // there is nothing for this method to do. So, to keep the 
        // compiler from complaining about a switch with no case
        // statements, but which has a default statement, we surround
        // the entire construct with preprocessor controls.
        //
        switch(uFunc)
        {
        // The following are the page/band related DDI hooks.
        //

        case INDEX_DrvStartDoc:
            m_pfnDrvStartDoc = (PFN_DrvStartDoc)pfn;
            break;
        case INDEX_DrvEndDoc:
            m_pfnDrvEndDoc = (PFN_DrvEndDoc)pfn;
            break;
        case INDEX_DrvStartPage:
            m_pfnDrvStartPage = (PFN_DrvStartPage)pfn;
            break;
        case INDEX_DrvSendPage:
            m_pfnDrvSendPage = (PFN_DrvSendPage)pfn;
            break;
        case INDEX_DrvStartBanding:
            m_pfnDrvStartBanding = (PFN_DrvStartBanding)pfn;
            break;
        case INDEX_DrvNextBand:
            m_pfnDrvNextBand = (PFN_DrvNextBand)pfn;
            break;
        case INDEX_DrvEscape:
            m_pfnDrvEscape = (PFN_DrvEscape)pfn;
            break;

        // The following are the drawing related DDI hooks.
        //
        case INDEX_DrvAlphaBlend:
            m_pfnDrvAlphaBlend  = (PFN_DrvAlphaBlend)pfn;
            break;
        case INDEX_DrvBitBlt:
            m_pfnDrvBitBlt  = (PFN_DrvBitBlt)pfn;
            break;
        case INDEX_DrvCopyBits:
            m_pfnDrvCopyBits = (PFN_DrvCopyBits)pfn;
            break;
        case INDEX_DrvDitherColor:
            m_pfnDrvDitherColor = (PFN_DrvDitherColor)pfn;
            break;
        case INDEX_DrvFillPath:
            m_pfnDrvFillPath = (PFN_DrvFillPath)pfn;
            break;
        case INDEX_DrvFontManagement:
            m_pfnDrvFontManagement = (PFN_DrvFontManagement)pfn;
            break;
        case INDEX_DrvGetGlyphMode:
            m_pfnDrvGetGlyphMode = (PFN_DrvGetGlyphMode)pfn;
            break;
        case INDEX_DrvGradientFill:
            m_pfnDrvGradientFill = (PFN_DrvGradientFill)pfn;
            break;
        case INDEX_DrvLineTo:
            m_pfnDrvLineTo = (PFN_DrvLineTo)pfn;
            break;
        case INDEX_DrvPaint:
            m_pfnDrvPaint = (PFN_DrvPaint)pfn;
            break;
        case INDEX_DrvPlgBlt:
            m_pfnDrvPlgBlt = (PFN_DrvPlgBlt)pfn;
            break;
        case INDEX_DrvQueryAdvanceWidths:
            m_pfnDrvQueryAdvanceWidths = (PFN_DrvQueryAdvanceWidths)pfn;
            break;
        case INDEX_DrvQueryFont:
            m_pfnDrvQueryFont = (PFN_DrvQueryFont)pfn;
            break;
        case INDEX_DrvQueryFontData:
            m_pfnDrvQueryFontData = (PFN_DrvQueryFontData)pfn;
            break;
        case INDEX_DrvQueryFontTree:
            m_pfnDrvQueryFontTree = (PFN_DrvQueryFontTree)pfn;
            break;
        case INDEX_DrvRealizeBrush:
            m_pfnDrvRealizeBrush = (PFN_DrvRealizeBrush)pfn;
            break;
        case INDEX_DrvStretchBlt:
            m_pfnDrvStretchBlt = (PFN_DrvStretchBlt)pfn;
            break;
        case INDEX_DrvStretchBltROP:
            m_pfnDrvStretchBltROP = (PFN_DrvStretchBltROP)pfn;
            break;
        case INDEX_DrvStrokeAndFillPath:
            m_pfnDrvStrokeAndFillPath = (PFN_DrvStrokeAndFillPath)pfn;
            break;
        case INDEX_DrvStrokePath:
            m_pfnDrvStrokePath = (PFN_DrvStrokePath)pfn;
            break;
        case INDEX_DrvTextOut:
            m_pfnDrvTextOut = (PFN_DrvTextOut)pfn;
            break;
        case INDEX_DrvTransparentBlt:
            m_pfnDrvTransparentBlt = (PFN_DrvTransparentBlt)pfn;
            break;

        } // switch(uFunc)
    }

    void __stdcall _ResetPointers(void)
    {
        m_pfnDrvAlphaBlend          = NULL;
        m_pfnDrvBitBlt              = NULL;
        m_pfnDrvCopyBits            = NULL;
        m_pfnDrvDitherColor         = NULL;
        m_pfnDrvEndDoc              = NULL;
        m_pfnDrvEscape              = NULL;
        m_pfnDrvFillPath            = NULL;
        m_pfnDrvFontManagement      = NULL;
        m_pfnDrvGetGlyphMode        = NULL;
        m_pfnDrvGradientFill        = NULL;
        m_pfnDrvLineTo              = NULL;
        m_pfnDrvNextBand            = NULL;
        m_pfnDrvPaint               = NULL;
        m_pfnDrvPlgBlt              = NULL;
        m_pfnDrvQueryAdvanceWidths  = NULL;
        m_pfnDrvQueryFont           = NULL;
        m_pfnDrvQueryFontData       = NULL;
        m_pfnDrvQueryFontTree       = NULL;
        m_pfnDrvRealizeBrush        = NULL;
        m_pfnDrvSendPage            = NULL;
        m_pfnDrvStartBanding        = NULL;
        m_pfnDrvStartDoc            = NULL;
        m_pfnDrvStartPage           = NULL;
        m_pfnDrvStretchBlt          = NULL;
        m_pfnDrvStretchBltROP       = NULL;
        m_pfnDrvStrokeAndFillPath   = NULL;
        m_pfnDrvStrokePath          = NULL;
        m_pfnDrvTextOut             = NULL;
        m_pfnDrvTransparentBlt      = NULL;
    }

public:

    // Unidrv function pointers so that we can punt
    // back from the DDI hooks
    //
    PFN_DrvAlphaBlend           m_pfnDrvAlphaBlend;
    PFN_DrvBitBlt               m_pfnDrvBitBlt;
    PFN_DrvCopyBits             m_pfnDrvCopyBits;
    PFN_DrvDitherColor          m_pfnDrvDitherColor;
    PFN_DrvEndDoc               m_pfnDrvEndDoc;
    PFN_DrvEscape               m_pfnDrvEscape;
    PFN_DrvFillPath             m_pfnDrvFillPath;
    PFN_DrvFontManagement       m_pfnDrvFontManagement;
    PFN_DrvGetGlyphMode         m_pfnDrvGetGlyphMode;
    PFN_DrvGradientFill         m_pfnDrvGradientFill;
    PFN_DrvLineTo               m_pfnDrvLineTo;
    PFN_DrvNextBand             m_pfnDrvNextBand;
    PFN_DrvPaint                m_pfnDrvPaint;
    PFN_DrvPlgBlt               m_pfnDrvPlgBlt;
    PFN_DrvQueryAdvanceWidths   m_pfnDrvQueryAdvanceWidths;
    PFN_DrvQueryFont            m_pfnDrvQueryFont;
    PFN_DrvQueryFontData        m_pfnDrvQueryFontData;
    PFN_DrvQueryFontTree        m_pfnDrvQueryFontTree;
    PFN_DrvRealizeBrush         m_pfnDrvRealizeBrush;
    PFN_DrvSendPage             m_pfnDrvSendPage;
    PFN_DrvStartBanding         m_pfnDrvStartBanding;
    PFN_DrvStartDoc             m_pfnDrvStartDoc;
    PFN_DrvStartPage            m_pfnDrvStartPage;
    PFN_DrvStretchBlt           m_pfnDrvStretchBlt;
    PFN_DrvStretchBltROP        m_pfnDrvStretchBltROP;
    PFN_DrvStrokeAndFillPath    m_pfnDrvStrokeAndFillPath;
    PFN_DrvStrokePath           m_pfnDrvStrokePath;
    PFN_DrvTextOut              m_pfnDrvTextOut;
    PFN_DrvTransparentBlt       m_pfnDrvTransparentBlt;

    /*******************************************/
    /*                                           */
    /*      Add any custom PDEV members here         */
    /*                                           */
    /*******************************************/

    BOOL bPreAnalysis;      // Indicates if a current pass is the pre-analysis pass
    
};
typedef COemPDEV* POEMPDEV;

