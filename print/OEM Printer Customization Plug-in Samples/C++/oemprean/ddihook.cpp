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
//            This module implements the following functions defined in printoem.h:
//              OEMCopyBits
//              OEMBitBlt
//              OEMStretchBlt
//              OEMDitherColor
//              OEMStretchBltROP
//              OEMPlgBlt
//              OEMPaint
//              OEMRealizeBrush
//              OEMStrokePath
//              OEMFillPath
//              OEMStrokeAndFillPath
//              OEMLineTo
//              OEMAlphaBlend
//              OEMGradientFill
//              OEMTransparentBlt
//              OEMTextOut
//              OEMQueryFont
//              OEMQueryFontTree
//              OEMQueryFontData
//              OEMFontManagement
//              OEMQueryAdvanceWidths
//              OEMGetGlyphMode
//              OEMStartDoc
//              OEMEndDoc
//              OEMStartPage
//              OEMSendPage
//              OEMStartBanding
//              OEMNextBand
//              OEMEscape
//
//  History:
//          06/24/03    xxx created.
//
//

#include "precomp.h"
#include "oemprean.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


BOOL APIENTRY
OEMCopyBits(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclDst,
    POINTL      *pptlSrc
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvCopyBits.

    DrvCopyBits translates between device-managed
    raster surfaces and GDI standard-format bitmaps.
    GDI will call DrvCopyBits when it needs to copy from
    one surface to another and at least one of the
    surfaces is device-managed.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Points to the Dstination surface
    psoSrc - Points to the source surface
    pxlo - XLATEOBJ provided by the engine
    pco - Defines a clipping region on the Dstination surface
    pxlo - Defines the translation of color indices
        between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper-left corner of the source rectangle

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMCopyBits entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMCopyBits: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMCopyBits: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvCopyBits)(psoDst,
                                        psoSrc,
                                        pco,
                                        pxlo,
                                        prclDst,
                                        pptlSrc);
}

BOOL APIENTRY
OEMBitBlt(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclDst,
    POINTL      *pptlSrc,
    POINTL      *pptlMask,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrush,
    ROP4        rop4
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvBitBlt.

    DrvBitBlt provides general bit-block transfer capabilities
    between device-managed surfaces, between GDI-managed
    standard-format bitmaps, or between a device-managed
    surface and a GDI-managed standard-format bitmap.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Describes the target surface
    psoSrc - Describes the source surface
    psoMask - Describes the mask for rop4
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated
        between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper left corner of the source rectangle
    pptlMask - Defines which pixel in the mask corresponds to
            the upper left corner of the source rectangle
    pbo - Defines the pattern for bitblt
    pptlBrush - Defines the origin of the brush in the Dstination surface
    rop4 - ROP code that defines how the mask, pattern, source, and
        Dstination pixels are combined to write to the Dstination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMBitBlt entry.");

    DBG_SURFOBJ(DBG_VERBOSE, L"psoDst", psoDst);
    DBG_SURFOBJ(DBG_VERBOSE, L"psoSrc", psoSrc);
    DBG_SURFOBJ(DBG_VERBOSE, L"psoMask", psoMask);
    DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);
    DBG_XLATEOBJ(DBG_VERBOSE, L"pxlo", pxlo);
    DBG_RECTL(DBG_VERBOSE, L"prclDst", prclDst);
    DBG_POINTL(DBG_VERBOSE, L"pptlSrc", pptlSrc);
    DBG_POINTL(DBG_VERBOSE, L"pptlMask", pptlMask);
    DBG_BRUSHOBJ(DBG_VERBOSE, L"pbo", pbo);
    DBG_POINTL(DBG_VERBOSE, L"pptlBrush", pptlBrush);
    VERBOSE(TEXT("\nrop4: %#x\r\n\n"), rop4);

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMBitBlt: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMBitBlt: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvBitBlt)(psoDst,
                                    psoSrc,
                                    psoMask,
                                    pco,
                                    pxlo,
                                    prclDst,
                                    pptlSrc,
                                    pptlMask,
                                    pbo,
                                    pptlBrush,
                                    rop4);
}

BOOL APIENTRY
OEMStretchBlt(
    SURFOBJ             *psoDst,
    SURFOBJ             *psoSrc,
    SURFOBJ             *psoMask,
    CLIPOBJ             *pco,
    XLATEOBJ            *pxlo,
    COLORADJUSTMENT *pca,
    POINTL              *pptlHTOrg,
    RECTL               *prclDst,
    RECTL               *prclSrc,
    POINTL              *pptlMask,
    ULONG               iMode
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStretchBlt.

    DrvStretchBlt provides stretching bit-block transfer
    capabilities between any combination of device-managed
    and GDI-managed surfaces. DrvStretchBlt enables the
    device driver to write to GDI bitmaps, especially when the
    driver can do halftoning. This function allows the same
    halftoning algorithm to be applied to GDI bitmaps and
    device surfaces. This function can be provided to handle
    only certain forms of stretching, such as by integer multiples.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    pca     - Defines color adjustment values to be applied to the source bitmap
    pptlHTOrg - Specifies the origin of the halftone brush
    prclDst - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
            the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMStretchBlt entry.");

    DBG_SURFOBJ(DBG_VERBOSE, L"psoDst", psoDst);
    DBG_SURFOBJ(DBG_VERBOSE, L"psoSrc", psoSrc);
    DBG_SURFOBJ(DBG_VERBOSE, L"psoMask", psoMask);
    DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);
    DBG_XLATEOBJ(DBG_VERBOSE, L"pxlo", pxlo);
    DBG_COLORADJUSTMENT(DBG_VERBOSE, L"pca", pca);
    DBG_POINTL(DBG_VERBOSE, L"pptlHTOrg", pptlHTOrg);
    DBG_RECTL(DBG_VERBOSE, L"prclDst", prclDst);
    DBG_RECTL(DBG_VERBOSE, L"prclSrc", prclSrc);
    DBG_POINTL(DBG_VERBOSE, L"pptlMask", pptlMask);
    VERBOSE(TEXT("\niMode: %#x\r\n\n"), iMode);

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMStretchBlt: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMStretchBlt: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStretchBlt)(psoDst,
                                        psoSrc,
                                        psoMask,
                                        pco,
                                        pxlo,
                                        pca,
                                        pptlHTOrg,
                                        prclDst,
                                        prclSrc,
                                        pptlMask,
                                        iMode);
}

ULONG APIENTRY
OEMDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG   *pulDither
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvDitherColor.

    DrvDitherColor requests the device to create a
    brush dithered against a device palette.

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - DHPDEV passed, it is our pDEV
    iMode - Not used
    rgbColor - Solid rgb color to be used
    pulDither - buffer to put the halftone brush.

Return Value:

    Returns halftone method.

    DCR_DRIVER if the dither values have been calculated by the driver.
    DCR_SOLID if the engine should use the best solid color approximation of the color.
    DCR_HALFTONE if the engine should create a halftone approximation for the driver.

--*/

{
    VERBOSE(L"OEMDitherColor entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvDitherColor)(dhpdev,
                                        iMode,
                                        rgbColor,
                                        pulDither);
}

BOOL APIENTRY
OEMStretchBltROP(
    SURFOBJ             *psoDst,
    SURFOBJ             *psoSrc,
    SURFOBJ             *psoMask,
    CLIPOBJ             *pco,
    XLATEOBJ            *pxlo,
    COLORADJUSTMENT *pca,
    POINTL              *pptlHTOrg,
    RECTL               *prclDst,
    RECTL               *prclSrc,
    POINTL              *pptlMask,
    ULONG               iMode,
    BRUSHOBJ            *pbo,
    ROP4                rop4
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStretchBltROP.

    DrvStretchBltROP performs a stretching bit-block transfer
    using a ROP.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Specifies the target surface
    psoSrc - Specifies the source surface
    psoMask - Specifies the mask surface
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlHTOrg - Specifies the halftone origin
    prclDst - Area to be modified on the destination surface
    prclSrc - Rectangle area on the source surface
    pptlMask - Defines which pixel in the mask corresponds to
        the upper left corner of the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels
    pbo - Defines the pattern for bitblt
    rop4 - ROP code that defines how the mask, pattern, source, and
        destination pixels are combined on the destination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMStretchBltROP entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMStretchBltROP: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMStretchBltROP: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStretchBltROP)(psoDst,
                                            psoSrc,
                                            psoMask,
                                            pco,
                                            pxlo,
                                            pca,
                                            pptlHTOrg,
                                            prclDst,
                                            prclSrc,
                                            pptlMask,
                                            iMode,
                                            pbo,
                                            rop4);
}

BOOL APIENTRY
OEMPlgBlt(
    SURFOBJ             *psoDst,
    SURFOBJ             *psoSrc,
    SURFOBJ             *psoMask,
    CLIPOBJ             *pco,
    XLATEOBJ            *pxlo,
    COLORADJUSTMENT *pca,
    POINTL              *pptlBrushOrg,
    POINTFIX                *pptfixDest,
    RECTL               *prclSrc,
    POINTL              *pptlMask,
    ULONG               iMode
    )
/*++

Routine Description:

    Implementation of DDI hook for DrvPlgBlt.

    DrvPlgBlt provides rotate bit-block transfer capabilities between
    combinations of device-managed and GDI-managed surfaces.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlBrushOrg - Specifies the origin of the halftone brush
    pptfixDest - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
            the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMPlgBlt entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMPlgBlt: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMPlgBlt: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvPlgBlt)(psoDst,
                                    psoSrc,
                                    psoMask,
                                    pco,
                                    pxlo,
                                    pca,
                                    pptlBrushOrg,
                                    pptfixDest,
                                    prclSrc,
                                    pptlMask,
                                    iMode);
}

BOOL APIENTRY
OEMPaint(
    SURFOBJ     *pso,
    CLIPOBJ     *pco,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrushOrg,
    MIX         mix
    )
/*++

Routine Description:

    Implementation of DDI hook for DrvPaint.

    DrvPaint is an obsolete function.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to draw
    pco - Limits the area to be modified on the Dstination
    pbo - Points to a BRUSHOBJ which defined the pattern and colors to fill with
    pptlBrushOrg - Specifies the origin of the halftone brush
    mix - Defines the foreground and background raster operations to use for
        the brush

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMPaint entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMPaint: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMPaint: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvPaint)(pso,
                                    pco,
                                    pbo,
                                    pptlBrushOrg,
                                    mix);
}

BOOL APIENTRY
OEMRealizeBrush(
    BRUSHOBJ    *pbo,
    SURFOBJ     *psoTarget,
    SURFOBJ     *psoPattern,
    SURFOBJ     *psoMask,
    XLATEOBJ    *pxlo,
    ULONG       iHatch
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvRealizeBrush.

    DrvRealizeBrush requests that the driver realize a specified brush
    for a specified surface. To realize a brush, the driver converts a
    GDI brush into a form that can be used internally. A realized brush
    contains device-specific information needed by the device to
    accelerate drawing using the brush. The driver's realization of a brush
    is written into the buffer allocated by a call to BRUSHOBJ_pvAllocRBrush.

    Please refer to DDK documentation for more details.

Arguments:

    pbo - BRUSHOBJ to be realized
    psoTarget - Defines the surface for which the brush is to be realized
    psoPattern - Defines the pattern for the brush
    psoMask - Transparency mask for the brush
    pxlo - Defines the interpretration of colors in the pattern
    iHatch - Specifies whether psoPattern is one of the hatch brushes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMRealizeBrush entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoTarget->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvRealizeBrush)(pbo,
                                        psoTarget,
                                        psoPattern,
                                        psoMask,
                                        pxlo,
                                        iHatch);
}

BOOL APIENTRY
OEMStrokePath(
    SURFOBJ     *pso,
    PATHOBJ     *ppo,
    CLIPOBJ     *pco,
    XFORMOBJ    *pxo,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrushOrg,
    LINEATTRS   *plineattrs,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStrokePath.

    DrvStrokePath strokes a path when called by GDI.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Identifies the surface on which to draw
    ppo - Defines the path to be stroked
    pco - Defines the clipping path
    pxo - Specifies the world to device coordinate transformation
    pbo - Specifies the brush to be used when drawing the path
    pptlBrushOrg - Defines the brush origin
    plineattrs - Defines the line attributes
    mix - Specifies how to combine the brush with the destination

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    VERBOSE(L"OEMStrokePath entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMStrokePath: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMStrokePath: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStrokePath)(pso,
                                    ppo,
                                    pco,
                                    pxo,
                                    pbo,
                                    pptlBrushOrg,
                                    plineattrs,
                                    mix);
}

BOOL APIENTRY
OEMFillPath(
    SURFOBJ     *pso,
    PATHOBJ     *ppo,
    CLIPOBJ     *pco,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvFillPath.

    DrvFillPath is an optional entry point to handle the filling of closed paths.
    GDI can call DrvFillPath to fill a path on a device-managed surface.
    When deciding whether to call this function, GDI compares the fill
    requirements with the following flags in the flGraphicsCaps member of
    the DEVINFO structure: GCAPS_BEZIERS, GCAPS_ALTERNATEFILL,
    and GCAPS_WINDINGFILL.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to draw.
    ppo - Defines the path to be filled
    pco - Defines the clipping path
    pbo - Defines the pattern and colors to fill with
    pptlBrushOrg - Defines the brush origin
    mix - Defines the foreground and background ROPs to use for the brush
    flOptions - Whether to use zero-winding or odd-even rule

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    VERBOSE(L"OEMFillPath entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMFillPath: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMFillPath: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvFillPath)(pso,
                                    ppo,
                                    pco,
                                    pbo,
                                    pptlBrushOrg,
                                    mix,
                                    flOptions);
}

BOOL APIENTRY
OEMStrokeAndFillPath(
    SURFOBJ     *pso,
    PATHOBJ     *ppo,
    CLIPOBJ     *pco,
    XFORMOBJ    *pxo,
    BRUSHOBJ    *pboStroke,
    LINEATTRS   *plineattrs,
    BRUSHOBJ    *pboFill,
    POINTL      *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStrokeAndFillPath.

    DrvStrokeAndFillPath strokes and fills a path concurrently.
    The driver can return FALSE if the path or the clipping is too
    complex for the device to handle; in that case, GDI converts
    to a simpler call. For example, if the device driver has set the
    GCAPS_BEZIERS flag in the flGraphicsCaps member of the
    DEVINFO structure and then receives a path with Bezier curves,
    it can return FALSE; GDI will then convert the Bezier curves to
    lines and call DrvStrokeAndFillPath again. If the device driver
    returns FALSE again, GDI will further simplify the call, making
    calls to DrvStrokePath and DrvFillPath, or to DrvBitBlt, depending
    on the mix and width of the lines making up the path.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Describes the surface on which to draw
    ppo - Describes the path to be filled
    pco - Defines the clipping path
    pxo - Specifies the world to device coordinate transformation
    pboStroke - Specifies the brush to use when stroking the path
    plineattrs - Specifies the line attributes
    pboFill - Specifies the brush to use when filling the path
    pptlBrushOrg - Specifies the brush origin for both brushes
    mixFill - Specifies the foreground and background ROPs to use
            for the fill brush
    flOptions - Whether to use zero-winding or odd-even rule

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    VERBOSE(L"OEMStrokeAndFillPath entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMStrokeAndFillPath: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMStrokeAndFillPath: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStrokeAndFillPath)(pso,
                                            ppo,
                                            pco,
                                            pxo,
                                            pboStroke,
                                            plineattrs,
                                            pboFill,
                                            pptlBrushOrg,
                                            mixFill,
                                            flOptions);
}

BOOL APIENTRY
OEMLineTo(
    SURFOBJ     *pso,
    CLIPOBJ     *pco,
    BRUSHOBJ    *pbo,
    LONG        x1,
    LONG        y1,
    LONG        x2,
    LONG        y2,
    RECTL       *prclBounds,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvLineTo.

    DrvLineTo draws a single, solid, integer-only cosmetic line.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Describes the surface on which to draw
    pco - Defines the clipping path
    pbo - Defines the brush used to draw the line
    x1,y1 - Specifies the line's starting point
    x2,y2 - Specifies the line's ending point
    prclBounds - Defines a rectangle that bounds the unclipped line.
    mix - Specifies the foreground and background ROP

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    VERBOSE(L"OEMLineTo entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMLineTo: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMLineTo: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvLineTo)(pso,
                                    pco,
                                    pbo,
                                    x1,
                                    y1,
                                    x2,
                                    y2,
                                    prclBounds,
                                    mix);
}

BOOL APIENTRY
OEMAlphaBlend(
    SURFOBJ     *psoDest,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclDest,
    RECTL       *prclSrc,
    BLENDOBJ    *pBlendObj
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvAlphaBlend.

    DrvAlphaBlend provides bit-block transfer capabilities with alpha blending.

    Please refer to DDK documentation for more details.

Arguments:

    psoDest - Defines the surface on which to draw
    psoSrc - Defines the source
    pco - Limits the area to be modified on the Destination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    prclDest - Defines the area to be modified on the Destination surface
    prclSrc - Defines the area to be copied from the source surface
    BlendFunction - Specifies the blend function to be used

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMAlphaBlend entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDest->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMAlphaBlend: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMAlphaBlend: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvAlphaBlend)(psoDest,
                                        psoSrc,
                                        pco,
                                        pxlo,
                                        prclDest,
                                        prclSrc,
                                        pBlendObj);
}

BOOL APIENTRY
OEMGradientFill(
    SURFOBJ     *psoDest,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    TRIVERTEX   *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL       *prclExtents,
    POINTL      *pptlDitherOrg,
    ULONG       ulMode
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvGradientFill.

    DrvGradientFill shades the specified primitives.

    Please refer to DDK documentation for more details.

Arguments:

    psoDest - Defines the surface on which to draw
    pco - Limits the area to be modified on the Destination
    pxlo - Should be ignored by the driver
    pVertex - Pointer to an array of TRIVERTEX structures,
            with each entry containing position and color information
    nVertex - Specifies the number of TRIVERTEX structures in the
            array to which pVertex points
    pMesh - Pointer to an array of structures that define the connectivity
            of the TRIVERTEX elements to which pVertex points
    nMesh - Specifies the number of elements in the array to which
            pMesh points
    prclExtents - Pointer to a RECTL structure that defines the area in
                which the gradient drawing is to occur
    pptlDitherOrg - Pointer to a POINTL structure that defines the origin
                on the surface for dithering
    ulMode - Specifies the current drawing mode and how to interpret
            the array to which pMesh points

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMGradientFill entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDest->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMGradientFill: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMGradientFill: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvGradientFill)(psoDest,
                                    pco,
                                    pxlo,
                                    pVertex,
                                    nVertex,
                                    pMesh,
                                    nMesh,
                                    prclExtents,
                                    pptlDitherOrg,
                                    ulMode);
}

BOOL APIENTRY
OEMTransparentBlt(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclDst,
    RECTL       *prclSrc,
    ULONG       iTransColor,
    ULONG       ulReserved
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvTransparentBlt.

    DrvTransparentBlt provides bit-block transfer capabilities with
    transparency.

    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source
    pco - Limits the area to be modified on the Destination
    pxlo    - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    prclDst - Defines the area to be modified on the Destination surface
    prclSrc - Defines the area to be copied from the source surface
    iTransColor - Specifies the transparent color

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMTransparentBlt entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMTransparentBlt: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMTransparentBlt: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvTransparentBlt)(psoDst,
                                            psoSrc,
                                            pco,
                                            pxlo,
                                            prclDst,
                                            prclSrc,
                                            iTransColor,
                                            ulReserved);
}

BOOL APIENTRY
OEMTextOut(
    SURFOBJ     *pso,
    STROBJ      *pstro,
    FONTOBJ     *pfo,
    CLIPOBJ     *pco,
    RECTL       *prclExtra,
    RECTL       *prclOpaque,
    BRUSHOBJ    *pboFore,
    BRUSHOBJ    *pboOpaque,
    POINTL      *pptlOrg,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvTextOut.

    DrvTextOut is the entry point from GDI that calls for the
    driver to render a set of glyphs at specified positions.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to be written.
    pstro - Defines the glyphs to be rendered and their positions
    pfo - Specifies the font to be used
    pco - Defines the clipping path
    prclExtra - A NULL-terminated array of rectangles to be filled
    prclOpaque - Specifies an opaque rectangle
    pboFore - Defines the foreground brush
    pboOpaque - Defines the opaque brush
    pptlOrg - Pointer to POINT struct , defining th origin
    mix - Specifies the foreground and background ROPs for pboFore

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMTextOut entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    DBG_SURFOBJ(DBG_VERBOSE, L"pso", pso);
    DBG_STROBJ(DBG_VERBOSE, L"pstro", pstro);
    DBG_FONTOBJ(DBG_VERBOSE, L"pfo", pfo);
    DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);
    DBG_BRUSHOBJ(DBG_VERBOSE, L"pboFore", pboFore);
    DBG_BRUSHOBJ(DBG_VERBOSE, L"pboOpaque", pboOpaque);

    if (pOemPDEV->bPreAnalysis)
    {
        VERBOSE(TEXT("OEMTextOut: Begin pre-analysis...\r\n"));

        // Since the bPreAnalysis flag is set, this indicates that the current pass is the pre-analysis pass.
        // Perform any pre-analysis tasks here. Note that the surface used during pre-analysis might
        // differ from the surface passed in during the rendering pass.

        // Dump the bounds of the clip window available for pre-analysis.
        DBG_CLIPOBJ(DBG_VERBOSE, L"pco", pco);

        // Although unidrv calls the hooked drawing functions, it will return before any drawing is done.
        // So we should not perform any drawing on the surface. But all other pre-analysis tasks should
        // be performed here. For example, certain printers need to handle black objects differently if
        // they intersect with any color objects versus being stand alone. Another task could be to halftone
        // stretchblt objects differently from bitblt objects. Basically, the plug-in can analyze the objects on the page.

        // During the pre-analysis pass, drawing functions should not call back into Unidrv.
        // So we return here.
        VERBOSE(TEXT("OEMTextOut: End pre-analysis...\r\n"));

        return TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvTextOut)(pso,
                                    pstro,
                                    pfo,
                                    pco,
                                    prclExtra,
                                    prclOpaque,
                                    pboFore,
                                    pboOpaque,
                                    pptlOrg,
                                    mix);
}

PIFIMETRICS APIENTRY
OEMQueryFont(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG_PTR   *pid
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvQueryFont.

    DrvQueryFont is used by GDI to get the IFIMETRICS structure
    for a given font.

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    iFile - Identifies the driver font file
    iFace - One-based index of the driver font
    pid - Points to a LONG variable for returning an identifier
        which GDI will pass to DrvFree

Return Value:

    Pointer to an IFIMETRICS structure for the given font.
    NULL if there is an error

--*/

{
    VERBOSE(L"OEMQueryFont entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvQueryFont)(dhpdev,
                                        iFile,
                                        iFace,
                                        pid);
}

PVOID APIENTRY
OEMQueryFontTree(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR   *pid
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvQueryFontTree.

    DrvQueryFontTree provides GDI with a pointer to a structure that
    defines one of the following:
    - A mapping from Unicode to glyph handles, including glyph variants
    - A mapping of kerning pairs to kerning handles

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    iFile - Identifies the driver font file
    iFace - One-based index of the driver font
    iMode - Specifies the type of information to be provided
    pid - Points to a LONG variable for returning an identifier
        which GDI will pass to DrvFree

Return Value:

    Pointer to the structure requested based on iMode.
    NULL if there is an error

--*/

{
    VERBOSE(L"OEMQueryFontTree entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvQueryFontTree)(dhpdev,
                                            iFile,
                                            iFace,
                                            iMode,
                                            pid);
}

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

/*++

Routine Description:

    Implementation of DDI hook for DrvQueryFontData.

    DrvQueryFontData retrieves information about a realized font.
    GDI provides a pointer to an array of glyph or kerning handles,
    and the driver returns information about the glyphs or kerning
    pairs. The driver can assume that all handles in the array are valid.

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    pfo - Points to a FONTOBJ structure
    iMode - Type of information requested
    hg - A glyph handle
    pgd - Points to a GLYPHDATA structure
    pv - Points to output buffer
    cjSize - Size of output buffer

Return Value:

    Depends on iMode. FD_ERROR if there is an error

--*/

{
    VERBOSE(L"OEMQueryFontData entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvQueryFontData)(dhpdev,
                                            pfo,
                                            iMode,
                                            hg,
                                            pgd,
                                            pv,
                                            cjSize);
}

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

/*++

Routine Description:

    Implementation of DDI hook for DrvFontManagement.

    Is this function valid for Unidrv drivers?

    Please refer to DDK documentation for more details.

Arguments:

    pso - Points to a SURFOBJ structure
    pfo - Points to a FONTOBJ structure
    iMode - Escape number
    cjIn - Size of input buffer
    pvIn - Points to input buffer
    cjOut - Size of output buffer
    pvOut - Points to output buffer

Return Value:

    0x00000001 to 0x7fffffff for success
    0x80000000 to 0xffffffff for failure
    0 if the specified escape number if not supported

--*/

{
    VERBOSE(L"OEMFontManagement entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvFontManagement)(pso,
                                            pfo,
                                            iMode,
                                            cjIn,
                                            pvIn,
                                            cjOut,
                                            pvOut);
}

BOOL APIENTRY
OEMQueryAdvanceWidths(
    DHPDEV                                       dhpdev,
    FONTOBJ                                     *pfo,
    ULONG                                        iMode,
    _In_reads_(cGlyphs) HGLYPH                 *phg,
    _Out_writes_bytes_(cGlyphs*sizeof(USHORT)) PVOID   pvWidths,
    ULONG                                        cGlyphs
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvQueryAdvanceWidths.

    DrvQueryAdvanceWidths returns character advance widths
    for a specified set of glyphs.

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    pfo - Points to a FONTOBJ structure
    iMode - Type of information to be provided
    phg - Points to an array of HGLYPHs for which the driver will
        provide character advance widths
    pvWidths - Points to a buffer for returning width data
    cGlyphs - Number of glyphs in the phg array

Return Value:

    Depends on iMode

--*/

{
    VERBOSE(L"OEMQueryAdvanceWidths entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvQueryAdvanceWidths)(dhpdev,
                                                pfo,
                                                iMode,
                                                phg,
                                                pvWidths,
                                                cGlyphs);
}

ULONG APIENTRY
OEMGetGlyphMode(
    DHPDEV      dhpdev,
    FONTOBJ     *pfo
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvGetGlyphMode.

    DrvGetGlyphMode tells GDI how to cache glyph information.
    GDI calls a driver's DrvGetGlyphMode routine to determine the
    range of font information that should be cached for a particular
    font; that is, DrvGetGlyphMode determines what GDI stores in
    its font cache. A device that caches fonts on its own should return
    FO_HGLYPHS to minimize the storage requirements for the font.
    GDI calls DrvGetGlyphMode for each font realization. For example,
    a driver might want to download outlines for point sizes larger than
    12 point, but raster images for smaller fonts. However, GDI reserves
    the right to refuse this request.

    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    pfo - Points to a FONTOBJ structure

Return Value:

    The glyph mode or FO_GLYPHMODE, which is the default.

--*/

{
    VERBOSE(L"OEMGetGlyphMode entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvGetGlyphMode)(dhpdev,
                                            pfo);
}

BOOL APIENTRY
OEMStartDoc(
    SURFOBJ     *pso,
    _In_ PWSTR  pwszDocName,
    DWORD       dwJobId
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStartDoc.

    DrvStartDoc is called by GDI when it is ready to
    start sending a document to the driver for rendering.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    pDocName - Specifies a Unicode document name
    jobId - Identifies the print job

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMStartDoc entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStartDoc)(pso,
                                        pwszDocName,
                                        dwJobId);
}

BOOL APIENTRY
OEMEndDoc(
    SURFOBJ     *pso,
    FLONG       fl
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvEndDoc.

    DrvEndDoc is called by GDI when it has finished
    sending a document to the driver for rendering.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    flags - A set of flag bits

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMEndDoc entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvEndDoc)(pso,
                                        fl);
}

BOOL APIENTRY
OEMStartPage(
    SURFOBJ     *pso
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStartPage.

    DrvStartPage is called by GDI when it is ready to
    start sending the contents of a physical page to the
    driver for rendering.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMStartPage entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStartPage)(pso);
}

BOOL APIENTRY
OEMSendPage(
    SURFOBJ     *pso
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvSendPage.

    DrvSendPage is called by GDI when it has finished
    drawing a physical page, so the driver  can send
    the page to the printer.

    Please refer to DDK documentation for more details.

    NOTE: OEMSendPage will not be called in any driver that enables
    pre-analysis. But the reason we implement this is that the driver
    might be pushed to a down-level client and thereby have to run
    without pre-analysis support. In this case, it might have to support
    OEMSendPage.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMSendPage entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvSendPage)(pso);
}

BOOL APIENTRY
OEMStartBanding(
    SURFOBJ     *pso,
    POINTL      *pptl
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvStartBanding.

    DrvStartBanding is called by GDI when it is ready to start
    sending bands of a physical page to the driver for rendering.
    Note: DrvStartBanding is called to prepare the driver
    for banding, call only once per page (not at everyband!!)

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    pptl - Pointer to origin of next band (to return to GDI)

Return Value:

    Fill out pptl to contain the origin of the first band

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMStartBanding entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    DBG_POINTL(DBG_VERBOSE, L"pptl", pptl);

    if (pptl == NULL)
    {
        VERBOSE(TEXT("OEMStartBanding: Start of OEM Pre-analysis pass...\r\n"));
        VERBOSE(TEXT("\tOEM Pre-analysis pass...no drawing on surface in this pass.\r\n"));
        pOemPDEV->bPreAnalysis = TRUE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvStartBanding)(pso,
                                            pptl);
}

BOOL APIENTRY
OEMNextBand(
    SURFOBJ     *pso,
    POINTL      *pptl
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvNextBand.

    DrvNextBand is called by GDI when it has finished
    drawing a band for a physical page, so the driver
    can send the band to the printer.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    pptl - Pointer to origin of next band (to return to GDI)

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(L"OEMNextBand entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    DBG_POINTL(DBG_VERBOSE, L"pptl", pptl);

    if (pOemPDEV->bPreAnalysis)
    {
        //
        // First call into OEMNextBand indicates end of pre-analysis path.
        //
        VERBOSE(TEXT("OEMNextBand: End of OEM Pre-analysis pass...\r\n"));
        pOemPDEV->bPreAnalysis = FALSE;
    }

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvNextBand)(pso,
                                        pptl);
}

ULONG APIENTRY
OEMEscape(
    SURFOBJ                   *pso,
    ULONG                      iEsc,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEscape.

    DrvEscape is used for retrieving information from a device
    that is not available in a device-independent device driver
    interface; the particular query depends on the value of the
    iEsc parameter.

    Please refer to DDK documentation for more details.

Arguments:

    pso     - Describes the surface the call is directed to
    iEsc    - Specifies a query
    cjIn    - Specifies the size in bytes of the buffer pointed to by pvIn
    pvIn    - Points to input data buffer
    cjOut   - Specifies the size in bytes of the buffer pointed to by pvOut
    pvOut   -  Points to the output buffer

Return Value:

    Depends on the query specified by iEsc parameter.

    Zero if the function specified by the query is not supported.

--*/

{
    VERBOSE(L"OEMEscape entry.");

    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvEscape)(pso,
                                    iEsc,
                                    cjIn,
                                    pvIn,
                                    cjOut,
                                    pvOut);
}

