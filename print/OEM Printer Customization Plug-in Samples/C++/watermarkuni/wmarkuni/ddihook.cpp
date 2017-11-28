//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    DDIHook.cpp
//
//
//  PURPOSE:  DDI Hook routines for User Mode COM Customization DLL.
//
//

#include "precomp.h"
#include "debug.h"
#include "wmarkuni.h"

// indicate to prefast that this is a user-mode component.
_Analysis_mode_(_Analysis_code_type_user_driver_);


// Loop limiter.
#define MAX_LOOP    10

void RenderWaterMark(PDEVOBJ     pdevobj);
static DWORD FormatResource(_In_ LPSTR pszResource, _Out_ LPSTR *ppszBuffer, ...);
//
// OEMBitBlt
//



BOOL APIENTRY
OEMBitBlt(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMBitBlt() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoTrg->dhpdev;

    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvBitBlt)(poempdev->pfnUnidrv[UD_DrvBitBlt])) (
           psoTrg,
           psoSrc,
           psoMask,
           pco,
           pxlo,
           prclTrg,
           pptlSrc,
           pptlMask,
           pbo,
           pptlBrush,
           rop4));

}

//
// OEMStretchBlt
//

BOOL APIENTRY
OEMStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStretchBlt() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;


    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStretchBlt)(poempdev->pfnUnidrv[UD_DrvStretchBlt])) (
            psoDest,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDest,
            prclSrc,
            pptlMask,
            iMode));

}

//
// OEMCopyBits
//

BOOL APIENTRY
OEMCopyBits(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDest,
    POINTL         *pptlSrc
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMCopyBits() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvCopyBits)(poempdev->pfnUnidrv[UD_DrvCopyBits])) (
            psoDest,
            psoSrc,
            pco,
            pxlo,
            prclDest,
            pptlSrc));

}

//
// OEMTextOut
//

BOOL APIENTRY
OEMTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMTextOut() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvTextOut)(poempdev->pfnUnidrv[UD_DrvTextOut])) (
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlOrg,
            mix));

}

//
// OEMStrokePath
//

BOOL APIENTRY
OEMStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStokePath() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStrokePath)(poempdev->pfnUnidrv[UD_DrvStrokePath])) (
            pso,
            ppo,
            pco,
            pxo,
            pbo,
            pptlBrushOrg,
            plineattrs,
            mix));

}

//
// OEMFillPath
//

BOOL APIENTRY
OEMFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMFillPath() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvFillPath)(poempdev->pfnUnidrv[UD_DrvFillPath])) (
            pso,
            ppo,
            pco,
            pbo,
            pptlBrushOrg,
            mix,
            flOptions));

}

//
// OEMStrokeAndFillPath
//

BOOL APIENTRY
OEMStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStrokeAndFillPath() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStrokeAndFillPath)(poempdev->pfnUnidrv[UD_DrvStrokeAndFillPath])) (
            pso,
            ppo,
            pco,
            pxo,
            pboStroke,
            plineattrs,
            pboFill,
            pptlBrushOrg,
            mixFill,
            flOptions));

}

//
// OEMRealizeBrush
//

BOOL APIENTRY
OEMRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMRealizeBrush() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvRealizeBrush)(poempdev->pfnUnidrv[UD_DrvRealizeBrush])) (
            pbo,
            psoTarget,
            psoPattern,
            psoMask,
            pxlo,
            iHatch));
}

//
// OEMStartPage
//

BOOL APIENTRY
OEMStartPage(
    SURFOBJ    *pso
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStartPage() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    // turn around to call Unidrv
    //

    return (((PFN_DrvStartPage)(poempdev->pfnUnidrv[UD_DrvStartPage]))(pso));

}

#define OEM_TESTSTRING  "The DDICMDCB DLL adds this line of text."

//
// OEMSendPage
//

BOOL APIENTRY
OEMSendPage(
    SURFOBJ    *pso
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMSendPage() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    POEMDEV pOemDevmode = (POEMDEV) pdevobj->pOEMDM;
    if(pOemDevmode && pOemDevmode->bWmarkEnabled)
    {
        RenderWaterMark(pdevobj);
    }
    //
    // print a line of text, just for testing
    //
    if (pso->iType == STYPE_BITMAP)
    {
        pdevobj->pDrvProcs->DrvXMoveTo(pdevobj, 0, 0);
        pdevobj->pDrvProcs->DrvYMoveTo(pdevobj, 0, 0);
        pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj, OEM_TESTSTRING,
                                             sizeof(OEM_TESTSTRING));
    }

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvSendPage)(poempdev->pfnUnidrv[UD_DrvSendPage]))(pso));

}

//
// OEMEscape
//

ULONG APIENTRY
OEMEscape(
    SURFOBJ                   *pso,
    ULONG                      iEsc,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMEscape() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvEscape)(poempdev->pfnUnidrv[UD_DrvEscape])) (
            pso,
            iEsc,
            cjIn,
            pvIn,
            cjOut,
            pvOut));

}

//
// OEMStartDoc
//

BOOL APIENTRY
OEMStartDoc(
    SURFOBJ     *pso,
    _In_ PWSTR  pwszDocName,
    DWORD       dwJobId
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStartDoc() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStartDoc)(poempdev->pfnUnidrv[UD_DrvStartDoc])) (
            pso,
            pwszDocName,
            dwJobId));

}

//
// OEMEndDoc
//

BOOL APIENTRY
OEMEndDoc(
    SURFOBJ    *pso,
    FLONG       fl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMEndDoc() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvEndDoc)(poempdev->pfnUnidrv[UD_DrvEndDoc])) (
            pso,
            fl));

}

////////
// NOTE:
// OEM DLL needs to hook out the following six font related DDI calls only
// if it enumerates additional fonts beyond what's in the GPD file.
// And if it does, it needs to take care of its own fonts for all font DDI
// calls and DrvTextOut call.
///////

//
// OEMQueryFont
//

PIFIMETRICS APIENTRY
OEMQueryFont(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG_PTR  *pid
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMQueryFont() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvQueryFont)(poempdev->pfnUnidrv[UD_DrvQueryFont])) (
            dhpdev,
            iFile,
            iFace,
            pid));

}

//
// OEMQueryFontTree
//

PVOID APIENTRY
OEMQueryFontTree(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR  *pid
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMQueryFontTree() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvQueryFontTree)(poempdev->pfnUnidrv[UD_DrvQueryFontTree])) (
            dhpdev,
            iFile,
            iFace,
            iMode,
            pid));

}

//
// OEMQueryFontData
//

LONG APIENTRY
OEMQueryFontData(
    DHPDEV                      dhpdev,
    FONTOBJ                    *pfo,
    ULONG                       iMode,
    HGLYPH                      hg,
    GLYPHDATA                  *pgd,
    _Out_writes_bytes_(cjSize) PVOID  pv,
    ULONG                       cjSize
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMQueryFontData() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvQueryFontData)(poempdev->pfnUnidrv[UD_DrvQueryFontData])) (
            dhpdev,
            pfo,
            iMode,
            hg,
            pgd,
            pv,
            cjSize));

}

//
// OEMQueryAdvanceWidths
//

BOOL APIENTRY
OEMQueryAdvanceWidths(
    DHPDEV                                       dhpdev,
    FONTOBJ                                     *pfo,
    ULONG                                        iMode,
    _In_reads_(cGlyphs) HGLYPH                 *phg,
    _Out_writes_bytes_(cGlyphs*sizeof(USHORT)) PVOID   pvWidths,
    ULONG                                        cGlyphs
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMQueryAdvanceWidths() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvQueryAdvanceWidths)
             (poempdev->pfnUnidrv[UD_DrvQueryAdvanceWidths])) (
                   dhpdev,
                   pfo,
                   iMode,
                   phg,
                   pvWidths,
                   cGlyphs));

}

//
// OEMFontManagement
//

ULONG APIENTRY
OEMFontManagement(
    SURFOBJ                   *pso,
    FONTOBJ                   *pfo,
    ULONG                      iMode,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMFontManagement() entry.\r\n"));

    //
    // Note that Unidrv will not call OEM DLL for iMode==QUERYESCSUPPORT.
    // So pso is not NULL for sure.
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvFontManagement)(poempdev->pfnUnidrv[UD_DrvFontManagement])) (
            pso,
            pfo,
            iMode,
            cjIn,
            pvIn,
            cjOut,
            pvOut));

}

//
// OEMGetGlyphMode
//

ULONG APIENTRY
OEMGetGlyphMode(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMGetGlyphMode() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvGetGlyphMode)(poempdev->pfnUnidrv[UD_DrvGetGlyphMode])) (
            dhpdev,
            pfo));

}

BOOL APIENTRY
OEMNextBand(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMNextBand() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvNextBand)(poempdev->pfnUnidrv[UD_DrvNextBand])) (
            pso,
            pptl));

}

BOOL APIENTRY
OEMStartBanding(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStartBanding() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    POEMDEV pOemDevmode = (POEMDEV) pdevobj->pOEMDM;
    if(pOemDevmode->bWmarkEnabled)
    {
    RenderWaterMark(pdevobj);
    }

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStartBanding)(poempdev->pfnUnidrv[UD_DrvStartBanding])) (
            pso,
            pptl));


}

ULONG APIENTRY
OEMDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMDitherColor() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvDitherColor)(poempdev->pfnUnidrv[UD_DrvDitherColor])) (
            dhpdev,
            iMode,
            rgbColor,
            pulDither));

}

BOOL APIENTRY
OEMPaint(
    SURFOBJ         *pso,
    CLIPOBJ         *pco,
    BRUSHOBJ        *pbo,
    POINTL          *pptlBrushOrg,
    MIX             mix
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMPaint() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvPaint)(poempdev->pfnUnidrv[UD_DrvPaint])) (
            pso,
            pco,
            pbo,
            pptlBrushOrg,
            mix));

}

BOOL APIENTRY
OEMLineTo(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    LONG        x1,
    LONG        y1,
    LONG        x2,
    LONG        y2,
    RECTL      *prclBounds,
    MIX         mix
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMLineTo() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvLineTo)(poempdev->pfnUnidrv[UD_DrvLineTo])) (
            pso,
            pco,
            pbo,
            x1,
            y1,
            x2,
            y2,
            prclBounds,
            mix));

}


//
// OEMStretchBltROP
//

BOOL APIENTRY
OEMStretchBltROP(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMStretchBltROP() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvStretchBltROP)(poempdev->pfnUnidrv[UD_DrvStretchBltROP])) (
            psoDest,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDest,
            prclSrc,
            pptlMask,
            iMode,
            pbo,
            rop4
            ));


}

//
// OEMPlgBlt
//

BOOL APIENTRY
OEMPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMPlgBlt() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvPlgBlt)(poempdev->pfnUnidrv[UD_DrvPlgBlt])) (
            psoDst,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlBrushOrg,
            pptfixDest,
            prclSrc,
            pptlMask,
            iMode));

}

//
// OEMAlphaBlend
//

BOOL APIENTRY
OEMAlphaBlend(
    SURFOBJ    *psoDest,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDest,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMAlphaBlend() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvAlphaBlend)(poempdev->pfnUnidrv[UD_DrvAlphaBlend])) (
            psoDest,
            psoSrc,
            pco,
            pxlo,
            prclDest,
            prclSrc,
            pBlendObj
            ));

}

//
// OEMGradientFill
//

BOOL APIENTRY
OEMGradientFill(
        SURFOBJ    *psoDest,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        TRIVERTEX  *pVertex,
        ULONG       nVertex,
        PVOID       pMesh,
        ULONG       nMesh,
        RECTL      *prclExtents,
        POINTL     *pptlDitherOrg,
        ULONG       ulMode
    )
{
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMGradientFill() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvGradientFill)(poempdev->pfnUnidrv[UD_DrvGradientFill])) (
            psoDest,
            pco,
            pxlo,
            pVertex,
            nVertex,
            pMesh,
            nMesh,
            prclExtents,
            pptlDitherOrg,
            ulMode
            ));

}

BOOL APIENTRY
OEMTransparentBlt(
        SURFOBJ    *psoDst,
        SURFOBJ    *psoSrc,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        RECTL      *prclDst,
        RECTL      *prclSrc,
        ULONG      iTransColor,
        ULONG      ulReserved
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    VERBOSE(DLLTEXT("OEMTransparentBlt() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvTransparentBlt)(poempdev->pfnUnidrv[UD_DrvTransparentBlt])) (
            psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            prclSrc,
            iTransColor,
            ulReserved
            ));

}

void RenderWaterMark(PDEVOBJ  pdevobj)
{
    POEMDEV pOemDevmode = (POEMDEV) pdevobj->pOEMDM;
       // We have a programmable palette. See GPD file for PaletteProgrammable option.
    // We are programming color index 8 to the new color and selectiing it with the PJL code "\xv%da%db%dc8I\xib*v8S".
    // In the  font selection code, we are selecting the arial font (4148)"\x1b(19U\x1b(s4148t0b0s%dv1P"
    // Cursor position is hard coded to <x,y> but you it can also be made dependent on user input.
    // PCL5 can print in 0,90,180 and 270 degrees. So Vertical can also be supported.
    // The \x1b*v7S after the watermark sets the foreground color back to black

    /*
    //Note we can push and pop the cursor position if needed.
    Push/Pop Cursor position
    \x1b&f0S        //push
    \x1b&f1S        //pop
    */

    PSTR    pszBuffer   = NULL;
    char str_wmark[]="\x1b*v%da%db%dc8I\x1b*v8S\x1b(19U\x1b(s4148t0b0s%dv1P\x1b*p1000Y\x1b*p750X%ls\x1b*v7S";
    DWORD dwLen= FormatResource (str_wmark,
                                 &pszBuffer,
                                 GetRValue(pOemDevmode->crWmarkTextColor),
                                 GetGValue(pOemDevmode->crWmarkTextColor),
                                 GetBValue(pOemDevmode->crWmarkTextColor),
                                 pOemDevmode->dwWmarkFontSize,pOemDevmode->szWaterMark);

    if ( dwLen != 0 )
    {
        pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj, pszBuffer, dwLen);
    }

    if ( pszBuffer )
    {
        delete[]pszBuffer;
        pszBuffer = NULL;
    }
}


DWORD
FormatResource(
        _In_ LPSTR  pszResource,
        _Out_ LPSTR *ppszBuffer,
        ...)
{
    DWORD   dwBufferSize  = 0;
    DWORD   dwLoop  = 0;
    va_list vaList;
    HRESULT hResult = S_OK;

    if ( NULL == pszResource ||
         NULL == ppszBuffer )
    {
        hResult = E_INVALIDARG;
        return 0;
    }


    if ( SUCCEEDED(hResult) )
    {
        dwBufferSize = (DWORD)strlen(pszResource) + MAX_PATH;

        va_start(vaList, ppszBuffer);

        // *ppszBuffer should be NULL when passed in.
        *ppszBuffer = NULL;

        // Allocate and format the string.
        do {

            if(NULL != *ppszBuffer)
            {
                delete[] *ppszBuffer;
            }
            *ppszBuffer = new CHAR[dwBufferSize];
            if(NULL == *ppszBuffer)
            {
                hResult = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hResult = StringCbVPrintfA(*ppszBuffer, dwBufferSize, pszResource, vaList);

            if(STRSAFE_E_INSUFFICIENT_BUFFER == hResult)
            {
                dwBufferSize *= 2;
            }

        } while ( FAILED(hResult) && (dwLoop++ < MAX_LOOP));
    }

Cleanup:

    // Check to see if we hit error.
    if(FAILED(hResult))
    {
        if(NULL != *ppszBuffer)
        {
            delete[] *ppszBuffer;
            *ppszBuffer = NULL;
        }
    }

    va_end(vaList);

    if ( *ppszBuffer )
    {
        DWORD dwToReturn = 0;

        if (SUCCEEDED(SIZETToDWord(strlen(*ppszBuffer), &dwToReturn)))
        {
            return dwToReturn;
        }
    }
    return 0;
}
