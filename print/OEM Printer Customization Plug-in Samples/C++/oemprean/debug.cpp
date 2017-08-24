//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1996 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Debug.cpp
//
//  PURPOSE:  Implementation of the debug functions.
//
//
//  History:
//          06/28/03    xxx created.
//
//

#include "precomp.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      INTERNAL DEFINES
////////////////////////////////////////////////////////

#define DEBUG_BUFFER_SIZE       1024
#define PATH_SEPARATOR          '\\'

////////////////////////////////////////////////////////
//      EXTERNAL GLOBALS
////////////////////////////////////////////////////////

#ifdef DBG

INT giDebugLevel = DEBUG_LEVEL;


PCWSTR
EnsureLabel(
    _In_opt_ PCWSTR      pszInLabel,
    _In_ PCWSTR      pszDefLabel
    )

/*++

Routine Description:

    This function checks if pszInLabel is valid. If not, it returns
    pszDefLabel else pszInLabel is returned.

Arguments:

    pszInLabel - custom label string passed in with the call to the dump function
    pszDefLabel - default label string in case custom label string is not valid

Return Value:

    pszInLabel if it is valid, else pszDefLabel

--*/

{
    // By design, pszDefLabel is assumed to be a valid, non-empty
    // string (since it is supplied internally).
    //
    if (!pszInLabel || !*pszInLabel)
    {
        // The caller supplied a NULL string or empty string;
        // supply the internal default.
        //
        return pszDefLabel;
    }

    return pszInLabel;
}

BOOL OEMDebugMessage(
    LPCWSTR lpszMessage,
    ...
    )

/*++

Routine Description:

    Outputs variable argument debug string.

Arguments:

    dwSize - size of temp buffer to hold formatted string
    lpszMessage - format string
    arglist - Variable argument list...

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    va_list VAList;
    HRESULT hResult;

    // Parameter checking.
    if( NULL == lpszMessage)
    {
        return FALSE;
    }

    LPWSTR lpszMsgBuf = new WCHAR[DEBUG_BUFFER_SIZE];
    if(NULL == lpszMsgBuf)
        return FALSE;

    va_start(VAList, lpszMessage);
    // Pass the variable parameters to wvsprintf to be formatted.
    hResult = StringCchVPrintfW(lpszMsgBuf, DEBUG_BUFFER_SIZE, lpszMessage, VAList);
    va_end(VAList);

    // Dump string to debug output.
    OutputDebugStringW(lpszMsgBuf);

    // Clean up.
    delete[] lpszMsgBuf;

    return SUCCEEDED(hResult);
}

PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    PCSTR   pstr;

    pstr = strrchr(pstrFilename, PATH_SEPARATOR);
    if (pstr)
        return pstr + 1;

    return pstrFilename;
}

void
vDumpFlags(
    DWORD           dwFlags,
    PDBG_FLAGS      pDebugFlags
    )

/*++

Routine Description:

    Dumps the combination of flags in members such as
    pso->fjBitmap, pstro->flAccel, pfo->flFontType etc.

Arguments:

    dwFlags - combined value of the relevant member
        Example values are pso->fjBitmap, pstro->flAccel
    pDebugFlags - structure containing the different possible values
        that can be combined in dwFlags

Return Value:

    NONE

--*/

{
    DWORD dwFound = 0;
    BOOL bFirstFlag = FALSE;

    OEMDebugMessage(L"%#x [", dwFlags);

    // Traverse through the list of flags to see if any match
    //
    for ( ; pDebugFlags->dwFlag; ++pDebugFlags)
    {
        if(dwFlags & pDebugFlags->dwFlag)
        {
            if (!bFirstFlag)
            {
                OEMDebugMessage(L"%s", pDebugFlags->pszFlag);
                bFirstFlag = TRUE;
            }
            else
            {
                OEMDebugMessage(L" | %s", pDebugFlags->pszFlag);
            }
            dwFound |= pDebugFlags->dwFlag;
        }
    }

    OEMDebugMessage(L"]");

    //
    // Check if there are extra bits set that we don't understand.
    //
    if(dwFound != dwFlags)
    {
        OEMDebugMessage(L"  <ExtraBits: %x>", dwFlags & ~dwFound);
    }

    OEMDebugMessage(L"\r\n");
}

void
vDumpOemDMParam(
    INT             iDebugLevel,
    _In_ PWSTR      pszInLabel,
    POEMDMPARAM     pOemDMParam
    )

/*++

Routine Description:

    Dumps the members of a OEMDMPARAM structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pOemDMParam - pointer to the OEMDMPARAM strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pOemDMParam");

    // Return if strct to be dumped is invalid
    //
    if (!pOemDMParam)
    {
        OEMDebugMessage(L"\npOemDMParam [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\npOemDMParam [%s]: %#x\r\n", pszLabel, pOemDMParam);
    OEMDebugMessage(L"\tcbSize = %d\r\n", pOemDMParam->cbSize);
    OEMDebugMessage(L"\tpdriverobj = %#x\r\n", pOemDMParam->pdriverobj);
    OEMDebugMessage(L"\thPrinter = %#x\r\n", pOemDMParam->hPrinter);
    OEMDebugMessage(L"\thModule = %#x\r\n", pOemDMParam->hModule);
    OEMDebugMessage(L"\tpPublicDMIn = %#x\r\n", pOemDMParam->pPublicDMIn);
    OEMDebugMessage(L"\tpPublicDMOut = %#x\r\n", pOemDMParam->pPublicDMOut);
    OEMDebugMessage(L"\tpOEMDMIn = %#x\r\n", pOemDMParam->pOEMDMIn);
    OEMDebugMessage(L"\tpOEMDMOut = %#x\r\n", pOemDMParam->pOEMDMOut);
    OEMDebugMessage(L"\tcbBufSize = %d\r\n", pOemDMParam->cbBufSize);

    OEMDebugMessage(L"\n");

}

#if DBG
    DBG_FLAGS gafdSURFOBJ_fjBitmap[] = {
        { L"BMF_TOPDOWN",       BMF_TOPDOWN},
        { L"BMF_NOZEROINIT",        BMF_NOZEROINIT},
        { L"BMF_DONTCACHE",     BMF_DONTCACHE},
        { L"BMF_USERMEM",       BMF_USERMEM},
        { L"BMF_KMSECTION",     BMF_KMSECTION},
        { L"BMF_NOTSYSMEM",     BMF_NOTSYSMEM},
        { L"BMF_WINDOW_BLT",    BMF_WINDOW_BLT},
        {NULL, 0}               // The NULL entry is important
//      { L"BMF_UMPDMEM",       BMF_UMPDMEM},
//      { L"BMF_RESERVED",      BMF_RESERVED},
    };
#else
    DBG_FLAGS gafdSURFOBJ_fjBitmap[] = {
        {NULL, 0}
    };
#endif

void
vDumpSURFOBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    SURFOBJ     *pso
    )

/*++

Routine Description:

    Dumps the members of a SURFOBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pso - pointer to the SURFOBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pso");

    // Return if strct to be dumped is invalid
    //
    if (!pso)
    {
        OEMDebugMessage(L"\nSURFOBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nSURFOBJ [%s]: %#x\r\n", pszLabel, pso);
    OEMDebugMessage(L"\tdhSurf: %#x\r\n", pso->dhsurf);
    OEMDebugMessage(L"\thSurf: %#x\r\n", pso->hsurf);
    OEMDebugMessage(L"\tdhpdev: %#x\r\n", pso->dhpdev);
    OEMDebugMessage(L"\thdev: %#x\r\n", pso->hdev);
    OEMDebugMessage(L"\tsizlBitmap: [%ld x %ld]\r\n", pso->sizlBitmap.cx, pso->sizlBitmap.cy);
    OEMDebugMessage(L"\tcjBits: %ld\r\n", pso->cjBits);
    OEMDebugMessage(L"\tpvBits: %#x\r\n", pso->pvBits);
    OEMDebugMessage(L"\tpvScan0: %#x\r\n", pso->pvScan0);
    OEMDebugMessage(L"\tlDelta: %ld\r\n", pso->lDelta);
    OEMDebugMessage(L"\tiUniq: %ld\r\n", pso->iUniq);

    PWSTR psziBitmapFormat = L"0";
    switch (pso->iBitmapFormat)
    {
        case BMF_1BPP : psziBitmapFormat = L"BMF_1BPP" ; break;
        case BMF_4BPP : psziBitmapFormat = L"BMF_4BPP" ; break;
        case BMF_8BPP : psziBitmapFormat = L"BMF_8BPP" ; break;
        case BMF_16BPP: psziBitmapFormat = L"BMF_16BPP"; break;
        case BMF_24BPP: psziBitmapFormat = L"BMF_24BPP"; break;
        case BMF_32BPP: psziBitmapFormat = L"BMF_32BPP"; break;
        case BMF_4RLE : psziBitmapFormat = L"BMF_4RLE" ; break;
        case BMF_8RLE : psziBitmapFormat = L"BMF_8RLE" ; break;
        case BMF_JPEG : psziBitmapFormat = L"BMF_JPEG" ; break;
        case BMF_PNG  : psziBitmapFormat = L"BMF_PNG " ; break;
    }
    OEMDebugMessage(L"\tiBitmapFormat: %s\r\n", psziBitmapFormat);

    PWSTR psziType = L"0";
    switch (pso->iType)
    {
        case STYPE_BITMAP   : psziType = L"STYPE_BITMAP"   ; break;
        case STYPE_DEVBITMAP: psziType = L"STYPE_DEVBITMAP"; break;
        case STYPE_DEVICE   : psziType = L"STYPE_DEVICE"   ; break;
    }
    OEMDebugMessage(L"\tiType: %s\r\n", psziType);

    OEMDebugMessage(L"\tfjBitmap: ");
    if (STYPE_BITMAP == pso->iType)
        vDumpFlags(pso->fjBitmap, gafdSURFOBJ_fjBitmap);
    else
        OEMDebugMessage(L"IGNORE\r\n");

    OEMDebugMessage(L"\n");

}

#if DBG
    DBG_FLAGS gafdSTROBJ_flAccel[] = {
        { L"SO_FLAG_DEFAULT_PLACEMENT",             SO_FLAG_DEFAULT_PLACEMENT},
        { L"SO_HORIZONTAL",                         SO_HORIZONTAL},
        { L"SO_VERTICAL",                           SO_VERTICAL},
        { L"SO_REVERSED",                           SO_REVERSED},
        { L"SO_ZERO_BEARINGS",                      SO_ZERO_BEARINGS},
        { L"SO_CHAR_INC_EQUAL_BM_BASE",         SO_CHAR_INC_EQUAL_BM_BASE},
        { L"SO_MAXEXT_EQUAL_BM_SIDE",               SO_MAXEXT_EQUAL_BM_SIDE},
        { L"SO_DO_NOT_SUBSTITUTE_DEVICE_FONT",      SO_DO_NOT_SUBSTITUTE_DEVICE_FONT},
        { L"SO_GLYPHINDEX_TEXTOUT",                 SO_GLYPHINDEX_TEXTOUT},
        { L"SO_ESC_NOT_ORIENT",                     SO_ESC_NOT_ORIENT},
        { L"SO_DXDY",                               SO_DXDY},
        { L"SO_CHARACTER_EXTRA",                    SO_CHARACTER_EXTRA},
        { L"SO_BREAK_EXTRA",                            SO_BREAK_EXTRA},
        {NULL, 0}                                   // The NULL entry is important
    };
#else
    DBG_FLAGS gafdSTROBJ_flAccel[] = {
        {NULL, 0}
    };
#endif

void
vDumpSTROBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    STROBJ      *pstro
    )

/*++

Routine Description:

    Dumps the members of a STROBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pstro - pointer to the STROBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pstro");

    // Return if strct to be dumped is invalid
    //
    if (!pstro)
    {
        OEMDebugMessage(L"\nSTROBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nSTROBJ [%s]: %#x\r\n", pszLabel, pstro);
    OEMDebugMessage(L"\tcGlyphs: %ld\r\n", pstro->cGlyphs);
    OEMDebugMessage(L"\tflAccel: ");
    vDumpFlags(pstro->flAccel, gafdSTROBJ_flAccel);
    OEMDebugMessage(L"\tulCharInc: %ld\r\n", pstro->ulCharInc);
    OEMDebugMessage(L"\trclBkGround: (%ld, %ld) (%ld, %ld)\r\n", pstro->rclBkGround.left, pstro->rclBkGround.top, pstro->rclBkGround.right, pstro->rclBkGround.bottom);
    if (!pstro->pgp)
        OEMDebugMessage(L"\tpgp: NULL\r\n");
    else
        OEMDebugMessage(L"\tpgp: %#x\r\n", pstro->pgp);
    OEMDebugMessage(L"\tpwszOrg: \"%s\"\r\n", pstro->pwszOrg);

    OEMDebugMessage(L"\n");

}

#if DBG
    DBG_FLAGS gafdFONTOBJ_flFontType[] = {
        { L"FO_TYPE_RASTER",        FO_TYPE_RASTER},
        { L"FO_TYPE_DEVICE",        FO_TYPE_DEVICE},
        { L"FO_TYPE_TRUETYPE",  FO_TYPE_TRUETYPE},
        { L"FO_TYPE_OPENTYPE",  0x8},
        { L"FO_SIM_BOLD",       FO_SIM_BOLD},
        { L"FO_SIM_ITALIC",     FO_SIM_ITALIC},
        { L"FO_EM_HEIGHT",      FO_EM_HEIGHT},
        { L"FO_GRAY16",         FO_GRAY16},
        { L"FO_NOGRAY16",       FO_NOGRAY16},
        { L"FO_CFF",                FO_CFF},
        { L"FO_POSTSCRIPT",     FO_POSTSCRIPT},
        { L"FO_MULTIPLEMASTER", FO_MULTIPLEMASTER},
        { L"FO_VERT_FACE",      FO_VERT_FACE},
        { L"FO_DBCS_FONT",      FO_DBCS_FONT},
        {NULL, 0}               // The NULL entry is important
    };
#else
    DBG_FLAGS gafdFONTOBJ_flFontType[] = {
        {NULL, 0}
    };
#endif

void
vDumpFONTOBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    FONTOBJ     *pfo
    )

/*++

Routine Description:

    Dumps the members of a FONTOBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pfo - pointer to the FONTOBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pfo");

    // Return if strct to be dumped is invalid
    //
    if (!pfo)
    {
        OEMDebugMessage(L"\nFONTOBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nFONTOBJ [%s]: %#x\r\n", pszLabel, pfo);
    OEMDebugMessage(L"\tiUniq: %ld\r\n", pfo->iUniq);
    OEMDebugMessage(L"\tiFace: %ld\r\n", pfo->iFace);
    OEMDebugMessage(L"\tcxMax: %ld\r\n", pfo->cxMax);
    OEMDebugMessage(L"\tflFontType: ");
    vDumpFlags(pfo->flFontType, gafdFONTOBJ_flFontType);
    OEMDebugMessage(L"\tiTTUniq: %#x\r\n", pfo->iTTUniq);
    OEMDebugMessage(L"\tiFile: %#x\r\n", pfo->iFile);
    OEMDebugMessage(L"\tsizLogResPpi: [%ld x %ld]\r\n", pfo->sizLogResPpi.cx, pfo->sizLogResPpi.cy);
    OEMDebugMessage(L"\tulStyleSize: %ld\r\n", pfo->ulStyleSize);
    if (!pfo->pvConsumer)
        OEMDebugMessage(L"\tpvConsumer: NULL\r\n");
    else
        OEMDebugMessage(L"\tpvConsumer: %#x\r\n", pfo->pvConsumer);
    if (!pfo->pvProducer)
        OEMDebugMessage(L"\tpvProducer: NULL\r\n");
    else
        OEMDebugMessage(L"\tpvProducer: %#x\r\n", pfo->pvProducer);

    OEMDebugMessage(L"\n");

}

void
vDumpCLIPOBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    CLIPOBJ     *pco
    )

/*++

Routine Description:

    Dumps the members of a CLIPOBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pco - pointer to the CLIPOBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pco");

    // Return if strct to be dumped is invalid
    //
    if (!pco)
    {
        OEMDebugMessage(L"\nCLIPOBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nCLIPOBJ [%s]: %#x\r\n", pszLabel, pco);
    OEMDebugMessage(L"\tiUniq: %ld\r\n", pco->iUniq);
    OEMDebugMessage(L"\trclBounds: (%ld, %ld) (%ld, %ld)\r\n", pco->rclBounds.left, pco->rclBounds.top, pco->rclBounds.right, pco->rclBounds.bottom);

    PWSTR psziDComplexity = L"Unknown iDComplexity.";
    switch (pco->iDComplexity )
    {
        case DC_COMPLEX: psziDComplexity = L"DC_COMPLEX"; break;
        case DC_RECT: psziDComplexity = L"DC_RECT"; break;
        case DC_TRIVIAL: psziDComplexity = L"DC_TRIVIAL"; break;
    }
    OEMDebugMessage(L"\tiDComplexity: %s\r\n", psziDComplexity);

    PWSTR psziFComplexity = L"0";
    switch (pco->iFComplexity)
    {
        case FC_COMPLEX: psziFComplexity = L"FC_COMPLEX"; break;
        case FC_RECT: psziFComplexity = L"FC_RECT"; break;
        case FC_RECT4: psziFComplexity = L"FC_RECT4"; break;
    }
    OEMDebugMessage(L"\tiFComplexity: %s\r\n", psziFComplexity);

    PWSTR psziMode = L"0";
    switch (pco->iMode)
    {
        case TC_PATHOBJ: psziMode = L"TC_PATHOBJ"; break;
        case TC_RECTANGLES: psziMode = L"TC_RECTANGLES"; break;
    }
    OEMDebugMessage(L"\tiMode: %s\r\n", psziMode);

    PWSTR pszfjOptions = L"0";
    switch (pco->fjOptions)
    {
        case OC_BANK_CLIP: pszfjOptions = L"TC_PATHOBJ"; break;
    }
    OEMDebugMessage(L"\tfjOptions: %s\r\n", pszfjOptions);

    OEMDebugMessage(L"\n");
}

#if DBG
    DBG_FLAGS gafdBRUSHOBJ_flColorType[] = {
        { L"BR_CMYKCOLOR",      BR_CMYKCOLOR},
        { L"BR_DEVICE_ICM",     BR_DEVICE_ICM},
        { L"BR_HOST_ICM",       BR_HOST_ICM},
        {NULL, 0}               // The NULL entry is important
    };
#else
    DBG_FLAGS gafdBRUSHOBJ_flColorType[] = {
        {NULL, 0}
    };
#endif

void
vDumpBRUSHOBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    BRUSHOBJ    *pbo
    )

/*++

Routine Description:

    Dumps the members of a BRUSHOBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pbo - pointer to the BRUSHOBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pbo");

    // Return if strct to be dumped is invalid
    //
    if (!pbo)
    {
        OEMDebugMessage(L"\nBRUSHOBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nBRUSHOBJ [%s]: %#x\r\n", pszLabel, pbo);
    OEMDebugMessage(L"\tiSolidColor: %#x\r\n", pbo->iSolidColor);
    OEMDebugMessage(L"\tpvRbrush: %#x\r\n", pbo->pvRbrush);
    OEMDebugMessage(L"\tflColorType: ");
    vDumpFlags(pbo->flColorType, gafdBRUSHOBJ_flColorType);

    OEMDebugMessage(L"\n");
}

#if DBG
    DBG_FLAGS gafdGDIINFO_flHTFlags[] = {
        { L"HT_FLAG_8BPP_CMY332_MASK",          HT_FLAG_8BPP_CMY332_MASK},
        { L"HT_FLAG_ADDITIVE_PRIMS",                HT_FLAG_ADDITIVE_PRIMS},
        { L"HT_FLAG_DO_DEVCLR_XFORM",           HT_FLAG_DO_DEVCLR_XFORM},
        { L"HT_FLAG_HAS_BLACK_DYE",             HT_FLAG_HAS_BLACK_DYE},
        { L"HT_FLAG_HIGH_INK_ABSORPTION",       HT_FLAG_HIGH_INK_ABSORPTION},
        { L"HT_FLAG_HIGHER_INK_ABSORPTION",     HT_FLAG_HIGHER_INK_ABSORPTION},
        { L"HT_FLAG_HIGHEST_INK_ABSORPTION",    HT_FLAG_HIGHEST_INK_ABSORPTION},
        { L"HT_FLAG_INK_ABSORPTION_IDX0",       HT_FLAG_INK_ABSORPTION_IDX0},
        { L"HT_FLAG_INK_ABSORPTION_IDX1",       HT_FLAG_INK_ABSORPTION_IDX1},
        { L"HT_FLAG_INK_ABSORPTION_IDX2",       HT_FLAG_INK_ABSORPTION_IDX2},
        { L"HT_FLAG_INK_ABSORPTION_IDX3",       HT_FLAG_INK_ABSORPTION_IDX3},
        { L"HT_FLAG_INK_HIGH_ABSORPTION",       HT_FLAG_INK_HIGH_ABSORPTION},
        { L"HT_FLAG_LOW_INK_ABSORPTION",        HT_FLAG_LOW_INK_ABSORPTION},
        { L"HT_FLAG_LOWER_INK_ABSORPTION",      HT_FLAG_LOWER_INK_ABSORPTION},
        { L"HT_FLAG_LOWEST_INK_ABSORPTION",     HT_FLAG_LOWEST_INK_ABSORPTION},
        { L"HT_FLAG_OUTPUT_CMY",                    HT_FLAG_OUTPUT_CMY},
        { L"HT_FLAG_PRINT_DRAFT_MODE",          HT_FLAG_PRINT_DRAFT_MODE},
        { L"HT_FLAG_SQUARE_DEVICE_PEL",         HT_FLAG_SQUARE_DEVICE_PEL},
        { L"HT_FLAG_USE_8BPP_BITMASK",          HT_FLAG_USE_8BPP_BITMASK},
        { L"HT_FLAG_NORMAL_INK_ABSORPTION",     HT_FLAG_NORMAL_INK_ABSORPTION},
//      { L"HT_FLAG_INVERT_8BPP_BITMASK_IDX",   HT_FLAG_INVERT_8BPP_BITMASK_IDX},
        {NULL, 0}                               // The NULL entry is important
    };
#else
    DBG_FLAGS gafdGDIINFO_flHTFlags[] = {
        {NULL, 0}
    };
#endif

void
vDumpGDIINFO(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    GDIINFO     *pGdiInfo
    )

/*++

Routine Description:

    Dumps the members of a GDIINFO structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pGdiInfo - pointer to the GDIINFO strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pGdiInfo");

    // Return if strct to be dumped is invalid
    //
    if (!pGdiInfo)
    {
        OEMDebugMessage(L"\nGDIINFO [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nGDIINFO [%s]: %#x\r\n", pszLabel, pGdiInfo);
    OEMDebugMessage(L"\tulVersion: %#x\r\n", pGdiInfo->ulVersion);

    PWSTR pszulTechnology = L"???";
    switch (pGdiInfo->ulTechnology)
    {
        case DT_PLOTTER: pszulTechnology = L"DT_PLOTTER"; break;
        case DT_RASDISPLAY: pszulTechnology = L"DT_RASDISPLAY"; break;
        case DT_RASPRINTER: pszulTechnology = L"DT_RASPRINTER"; break;
        case DT_RASCAMERA: pszulTechnology = L"DT_RASCAMERA"; break;
        case DT_CHARSTREAM: pszulTechnology = L"DT_CHARSTREAM"; break;
    }
    OEMDebugMessage(L"\tulTechnology: %ld [%s]\r\n", pGdiInfo->ulTechnology, pszulTechnology);

    OEMDebugMessage(L"\tulHorzSize: %ld\r\n", pGdiInfo->ulHorzSize);
    OEMDebugMessage(L"\tulVertSize: %ld\r\n", pGdiInfo->ulVertSize);
    OEMDebugMessage(L"\tulHorzRes: %ld\r\n", pGdiInfo->ulHorzRes);
    OEMDebugMessage(L"\tulVertRes: %ld\r\n", pGdiInfo->ulVertRes);
    OEMDebugMessage(L"\tcBitsPixel: %ld\r\n", pGdiInfo->cBitsPixel);
    OEMDebugMessage(L"\tcPlanes: %ld\r\n", pGdiInfo->cPlanes);
    OEMDebugMessage(L"\tulNumColors: %ld\r\n", pGdiInfo->ulNumColors);
    OEMDebugMessage(L"\tflRaster: %#x\r\n", pGdiInfo->flRaster);
    OEMDebugMessage(L"\tulLogPixelsX: %ld\r\n", pGdiInfo->ulLogPixelsX);
    OEMDebugMessage(L"\tulLogPixelsY: %ld\r\n", pGdiInfo->ulLogPixelsY);

    PWSTR pszTextCaps = L"Text scrolling through DrvBitBlt/DrvCopyBits";
    if (TC_SCROLLBLT == (pGdiInfo->flTextCaps & TC_SCROLLBLT))
    {
        pszTextCaps = L"Text scrolling through DrvTextOut";
    }
    OEMDebugMessage(L"\tflTextCaps: %s\n", pszTextCaps);

    OEMDebugMessage(L"\tulDACRed: %#x\r\n", pGdiInfo->ulDACRed);
    OEMDebugMessage(L"\tulDACGreen: %#x\r\n", pGdiInfo->ulDACGreen);
    OEMDebugMessage(L"\tulDACBlue: %#x\r\n", pGdiInfo->ulDACBlue);
    OEMDebugMessage(L"\tulAspectX: %ld\r\n", pGdiInfo->ulAspectX);
    OEMDebugMessage(L"\tulAspectY: %ld\r\n", pGdiInfo->ulAspectY);
    OEMDebugMessage(L"\tulAspectXY: %ld\r\n", pGdiInfo->ulAspectXY);
    OEMDebugMessage(L"\txStyleStep: %ld\r\n", pGdiInfo->xStyleStep);
    OEMDebugMessage(L"\tyStyleStep: %ld\r\n", pGdiInfo->yStyleStep);
    OEMDebugMessage(L"\tdenStyleStep: %ld\r\n", pGdiInfo->denStyleStep);
    OEMDebugMessage(L"\tptlPhysOffset: (%ld, %ld)\r\n", pGdiInfo->ptlPhysOffset.x, pGdiInfo->ptlPhysOffset.y);
    OEMDebugMessage(L"\tszlPhysSize: (%ld, %ld)\r\n", pGdiInfo->szlPhysSize.cx, pGdiInfo->szlPhysSize.cy);
    OEMDebugMessage(L"\tulNumPalReg: %ld\r\n", pGdiInfo->ulNumPalReg);
    OEMDebugMessage(L"\tulDevicePelsDPI: %ld\r\n", pGdiInfo->ulDevicePelsDPI);

    PWSTR pszPrimaryOrder = L"???";
    switch(pGdiInfo->ulPrimaryOrder)
    {
        case PRIMARY_ORDER_ABC: pszPrimaryOrder = L"PRIMARY_ORDER_ABC [RGB/CMY]"; break;
        case PRIMARY_ORDER_ACB: pszPrimaryOrder = L"PRIMARY_ORDER_ACB [RBG/CYM]"; break;
        case PRIMARY_ORDER_BAC: pszPrimaryOrder = L"PRIMARY_ORDER_BAC [GRB/MCY]"; break;
        case PRIMARY_ORDER_BCA: pszPrimaryOrder = L"PRIMARY_ORDER_BCA [GBR/MYC]"; break;
        case PRIMARY_ORDER_CBA: pszPrimaryOrder = L"PRIMARY_ORDER_CBA [BGR/YMC]"; break;
        case PRIMARY_ORDER_CAB: pszPrimaryOrder = L"PRIMARY_ORDER_CAB [BRG/YCM]"; break;
    }
    OEMDebugMessage(L"\tulPrimaryOrder: %s\n", pszPrimaryOrder);

    PWSTR pszHTPat = L"???";
    switch(pGdiInfo->ulHTPatternSize)
    {
        case HT_PATSIZE_2x2         : pszHTPat = L"HT_PATSIZE_2x2 ";            break;
        case HT_PATSIZE_2x2_M       : pszHTPat = L"HT_PATSIZE_2x2_M";           break;
        case HT_PATSIZE_4x4         : pszHTPat = L"HT_PATSIZE_4x4";         break;
        case HT_PATSIZE_4x4_M       : pszHTPat = L"HT_PATSIZE_4x4_M";           break;
        case HT_PATSIZE_6x6         : pszHTPat = L"HT_PATSIZE_6x6";         break;
        case HT_PATSIZE_6x6_M       : pszHTPat = L"HT_PATSIZE_6x6_M";           break;
        case HT_PATSIZE_8x8         : pszHTPat = L"HT_PATSIZE_8x8";         break;
        case HT_PATSIZE_8x8_M       : pszHTPat = L"HT_PATSIZE_8x8_M";           break;
        case HT_PATSIZE_10x10       : pszHTPat = L"HT_PATSIZE_10x10";           break;
        case HT_PATSIZE_10x10_M     : pszHTPat = L"HT_PATSIZE_10x10_M";     break;
        case HT_PATSIZE_12x12       : pszHTPat = L"HT_PATSIZE_12x12";           break;
        case HT_PATSIZE_12x12_M     : pszHTPat = L"HT_PATSIZE_12x12_M";     break;
        case HT_PATSIZE_14x14       : pszHTPat = L"HT_PATSIZE_14x14";           break;
        case HT_PATSIZE_14x14_M     : pszHTPat = L"HT_PATSIZE_14x14_M";     break;
        case HT_PATSIZE_16x16       : pszHTPat = L"HT_PATSIZE_16x16";           break;
        case HT_PATSIZE_16x16_M     : pszHTPat = L"HT_PATSIZE_16x16_M";     break;
        case HT_PATSIZE_SUPERCELL   : pszHTPat = L"HT_PATSIZE_SUPERCELL";       break;
        case HT_PATSIZE_SUPERCELL_M : pszHTPat = L"HT_PATSIZE_SUPERCELL_M"; break;
        case HT_PATSIZE_USER        : pszHTPat = L"HT_PATSIZE_USER";            break;
//      case HT_PATSIZE_MAX_INDEX   : pszHTPat = L"HT_PATSIZE_MAX_INDEX";       break;
//      case HT_PATSIZE_DEFAULT     : pszHTPat = L"HT_PATSIZE_DEFAULT";     break;
    }
    OEMDebugMessage(L"\tulHTPatternSize: %s\n", pszHTPat);

    PWSTR pszHTOutputFormat = L"???";
    switch(pGdiInfo->ulHTOutputFormat)
    {
        case HT_FORMAT_1BPP         : pszHTOutputFormat = L"HT_FORMAT_1BPP";        break;
        case HT_FORMAT_4BPP         : pszHTOutputFormat = L"HT_FORMAT_4BPP";        break;
        case HT_FORMAT_4BPP_IRGB    : pszHTOutputFormat = L"HT_FORMAT_4BPP_IRGB";   break;
        case HT_FORMAT_8BPP         : pszHTOutputFormat = L"HT_FORMAT_8BPP";        break;
        case HT_FORMAT_16BPP        : pszHTOutputFormat = L"HT_FORMAT_16BPP";       break;
        case HT_FORMAT_24BPP        : pszHTOutputFormat = L"HT_FORMAT_24BPP";       break;
        case HT_FORMAT_32BPP        : pszHTOutputFormat = L"HT_FORMAT_32BPP";       break;
    }
    OEMDebugMessage(L"\tulHTOutputFormat: %s\n", pszHTOutputFormat);

    OEMDebugMessage(L"\tflHTFlags: ");
    vDumpFlags(pGdiInfo->flHTFlags, gafdGDIINFO_flHTFlags);
    OEMDebugMessage(L"\tulVRefresh: %ld\r\n", pGdiInfo->ulVRefresh);
    OEMDebugMessage(L"\tulBltAlignment: %ld\r\n", pGdiInfo->ulBltAlignment);
    OEMDebugMessage(L"\tulPanningHorzRes: %ld\r\n", pGdiInfo->ulPanningHorzRes);
    OEMDebugMessage(L"\tulPanningVertRes: %ld\r\n", pGdiInfo->ulPanningVertRes);
    OEMDebugMessage(L"\txPanningAlignment: %ld\r\n", pGdiInfo->xPanningAlignment);
    OEMDebugMessage(L"\tyPanningAlignment: %ld\r\n", pGdiInfo->yPanningAlignment);
    OEMDebugMessage(L"\tcxHTPat: %ld\r\n", pGdiInfo->cxHTPat);
    OEMDebugMessage(L"\tcyHTPat: %ld\r\n", pGdiInfo->cyHTPat);
    OEMDebugMessage(L"\tpHTPatA: %#x\r\n", pGdiInfo->pHTPatA);
    OEMDebugMessage(L"\tpHTPatB: %#x\r\n", pGdiInfo->pHTPatB);
    OEMDebugMessage(L"\tpHTPatC: %#x\r\n", pGdiInfo->pHTPatC);
    OEMDebugMessage(L"\tflShadeBlend: %#x\r\n", pGdiInfo->flShadeBlend);

    PWSTR pszPhysPixChars = L"???";
    switch(pGdiInfo->ulPhysicalPixelCharacteristics)
    {
        case PPC_DEFAULT: pszPhysPixChars = L"PPC_DEFAULT"; break;
        case PPC_BGR_ORDER_HORIZONTAL_STRIPES: pszPhysPixChars = L"PPC_BGR_ORDER_HORIZONTAL_STRIPES"; break;
        case PPC_BGR_ORDER_VERTICAL_STRIPES: pszPhysPixChars = L"PPC_BGR_ORDER_VERTICAL_STRIPES"; break;
        case PPC_RGB_ORDER_HORIZONTAL_STRIPES: pszPhysPixChars = L"PPC_RGB_ORDER_HORIZONTAL_STRIPES"; break;
        case PPC_RGB_ORDER_VERTICAL_STRIPES: pszPhysPixChars = L"PPC_RGB_ORDER_VERTICAL_STRIPES"; break;
        case PPC_UNDEFINED: pszPhysPixChars = L"PPC_UNDEFINED"; break;
    }
    OEMDebugMessage(L"\tulPhysicalPixelCharacteristics: %s\n", pszPhysPixChars);

    PWSTR pszPhysPixGamma = L"???";
    switch(pGdiInfo->ulPhysicalPixelGamma)
    {
        case PPG_DEFAULT: pszPhysPixGamma = L"PPG_DEFAULT"; break;
        case PPG_SRGB   : pszPhysPixGamma = L"PPG_SRGB"; break;
    }
    OEMDebugMessage(L"\tulPhysicalPixelGamma: %s\n", pszPhysPixGamma);

    OEMDebugMessage(L"\n");
}

#if DBG
    DBG_FLAGS gafdDEVINFO_flGraphicsCaps[] = {
        { L"GCAPS_BEZIERS",             GCAPS_BEZIERS},
        { L"GCAPS_GEOMETRICWIDE",       GCAPS_GEOMETRICWIDE},
        { L"GCAPS_ALTERNATEFILL",       GCAPS_ALTERNATEFILL},
        { L"GCAPS_WINDINGFILL",         GCAPS_WINDINGFILL},
        { L"GCAPS_HALFTONE",                GCAPS_HALFTONE},
        { L"GCAPS_COLOR_DITHER",        GCAPS_COLOR_DITHER},
        { L"GCAPS_HORIZSTRIKE",         GCAPS_HORIZSTRIKE},
        { L"GCAPS_VERTSTRIKE",          GCAPS_VERTSTRIKE},
        { L"GCAPS_OPAQUERECT",          GCAPS_OPAQUERECT},
        { L"GCAPS_VECTORFONT",          GCAPS_VECTORFONT},
        { L"GCAPS_MONO_DITHER",         GCAPS_MONO_DITHER},
        { L"GCAPS_ASYNCCHANGE",     GCAPS_ASYNCCHANGE},
        { L"GCAPS_ASYNCMOVE",           GCAPS_ASYNCMOVE},
        { L"GCAPS_DONTJOURNAL",         GCAPS_DONTJOURNAL},
        { L"GCAPS_DIRECTDRAW",          GCAPS_DIRECTDRAW},
        { L"GCAPS_ARBRUSHOPAQUE",       GCAPS_ARBRUSHOPAQUE},
        { L"GCAPS_PANNING",             GCAPS_PANNING},
        { L"GCAPS_PALMANAGED",          GCAPS_PALMANAGED},
        { L"GCAPS_DITHERONREALIZE",     GCAPS_DITHERONREALIZE},
        { L"GCAPS_NO64BITMEMACCESS",    GCAPS_NO64BITMEMACCESS},
        { L"GCAPS_FORCEDITHER",         GCAPS_FORCEDITHER},
        { L"GCAPS_GRAY16",              GCAPS_GRAY16},
        { L"GCAPS_ICM",                 GCAPS_ICM},
        { L"GCAPS_CMYKCOLOR",           GCAPS_CMYKCOLOR},
        { L"GCAPS_LAYERED",             GCAPS_LAYERED},
        { L"GCAPS_ARBRUSHTEXT",         GCAPS_ARBRUSHTEXT},
        { L"GCAPS_SCREENPRECISION",     GCAPS_SCREENPRECISION},
        { L"GCAPS_FONT_RASTERIZER",     GCAPS_FONT_RASTERIZER},
        { L"GCAPS_NUP",                 GCAPS_NUP},
        {NULL, 0}                       // The NULL entry is important
    };

    DBG_FLAGS gafdDEVINFO_flGraphicsCaps2[] = {
        { L"GCAPS2_JPEGSRC",            GCAPS2_JPEGSRC},
        { L"GCAPS2_xxxx",               GCAPS2_xxxx},
        { L"GCAPS2_PNGSRC",             GCAPS2_PNGSRC},
        { L"GCAPS2_CHANGEGAMMARAMP",    GCAPS2_CHANGEGAMMARAMP},
        { L"GCAPS2_ALPHACURSOR",        GCAPS2_ALPHACURSOR},
        { L"GCAPS2_SYNCFLUSH",          GCAPS2_SYNCFLUSH},
        { L"GCAPS2_SYNCTIMER",          GCAPS2_SYNCTIMER},
        { L"GCAPS2_ICD_MULTIMON",       GCAPS2_ICD_MULTIMON},
        { L"GCAPS2_MOUSETRAILS",        GCAPS2_MOUSETRAILS},
        { L"GCAPS2_REMOTEDRIVER",       GCAPS2_REMOTEDRIVER},
        {NULL, 0}                       // The NULL entry is important
    };
#else
    DBG_FLAGS gafdDEVINFO_flGraphicsCaps[] = {
        {NULL, 0}
    };

    DBG_FLAGS gafdDEVINFO_flGraphicsCaps2[] = {
        {NULL, 0}
    };
#endif

void
vDumpDEVINFO(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    DEVINFO     *pDevInfo
    )

/*++

Routine Description:

    Dumps the members of a DEVINFO structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pDevInfo - pointer to the DEVINFO strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pDevInfo");

    // Return if strct to be dumped is invalid
    //
    if (!pDevInfo)
    {
        OEMDebugMessage(L"\nDEVINFO [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nDEVINFO [%s]: %#x\r\n", pszLabel, pDevInfo);
    OEMDebugMessage(L"\tflGraphicsCaps: ");
    vDumpFlags(pDevInfo->flGraphicsCaps, gafdDEVINFO_flGraphicsCaps);
    OEMDebugMessage(L"\tcFonts: %ld\r\n", pDevInfo->cFonts);

    PWSTR psziDitherFormat = L"0";
    switch (pDevInfo->iDitherFormat)
    {
        case BMF_1BPP : psziDitherFormat = L"BMF_1BPP" ; break;
        case BMF_4BPP : psziDitherFormat = L"BMF_4BPP" ; break;
        case BMF_8BPP : psziDitherFormat = L"BMF_8BPP" ; break;
        case BMF_16BPP: psziDitherFormat = L"BMF_16BPP"; break;
        case BMF_24BPP: psziDitherFormat = L"BMF_24BPP"; break;
        case BMF_32BPP: psziDitherFormat = L"BMF_32BPP"; break;
        case BMF_4RLE : psziDitherFormat = L"BMF_4RLE" ; break;
        case BMF_8RLE : psziDitherFormat = L"BMF_8RLE" ; break;
        case BMF_JPEG : psziDitherFormat = L"BMF_JPEG" ; break;
        case BMF_PNG  : psziDitherFormat = L"BMF_PNG " ; break;
    }
    OEMDebugMessage(L"\tiDitherFormat: %s\r\n", psziDitherFormat);

    OEMDebugMessage(L"\tcxDither: %ld\r\n", pDevInfo->cxDither);
    OEMDebugMessage(L"\tcyDither: %ld\r\n", pDevInfo->cyDither);
    OEMDebugMessage(L"\thpalDefault: %#x\r\n", pDevInfo->hpalDefault);
    OEMDebugMessage(L"\tflGraphicsCaps2: ");
    vDumpFlags(pDevInfo->flGraphicsCaps2, gafdDEVINFO_flGraphicsCaps2);

    OEMDebugMessage(L"\n");
}

void
vDumpBitmapInfoHeader(
    INT                 iDebugLevel,
    _In_ PWSTR          pszInLabel,
    BITMAPINFOHEADER    *pBitmapInfoHeader
    )

/*++

Routine Description:

    Dumps the members of a BITMAPINFOHEADER structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pBitmapInfoHeader - pointer to the BITMAPINFOHEADER strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pBitmapInfoHeader");

    // Return if strct to be dumped is invalid
    //
    if (!pBitmapInfoHeader)
    {
        OEMDebugMessage(L"\nBITMAPINFOHEADER [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nBITMAPINFOHEADER [%s]: %#x\r\n", pszLabel, pBitmapInfoHeader);
    OEMDebugMessage(L"\tbiSize: %ld\r\n", pBitmapInfoHeader->biSize);
    OEMDebugMessage(L"\tbiWidth: %ld\r\n", pBitmapInfoHeader->biWidth);
    OEMDebugMessage(L"\tbiHeight: %ld\r\n", pBitmapInfoHeader->biHeight);
    OEMDebugMessage(L"\tbiPlanes: %ld\r\n", pBitmapInfoHeader->biPlanes);
    OEMDebugMessage(L"\tbiBitCount: %ld\r\n", pBitmapInfoHeader->biBitCount);

    PWSTR pszbiCompression = L"0";
    switch (pBitmapInfoHeader->biCompression)
    {
        case BI_RGB : pszbiCompression = L"BI_RGB" ; break;
        case BI_RLE8 : pszbiCompression = L"BI_RLE8" ; break;
        case BI_RLE4 : pszbiCompression = L"BI_RLE4" ; break;
        case BI_BITFIELDS: pszbiCompression = L"BI_BITFIELDS"; break;
        case BI_JPEG : pszbiCompression = L"BI_JPEG" ; break;
        case BI_PNG  : pszbiCompression = L"BI_PNG " ; break;
    }
    OEMDebugMessage(L"\tbiCompression: %s\r\n", pszbiCompression);

    OEMDebugMessage(L"\tbiSizeImage: %ld\r\n", pBitmapInfoHeader->biSizeImage);
    OEMDebugMessage(L"\tbiXPelsPerMeter: %ld\r\n", pBitmapInfoHeader->biXPelsPerMeter);
    OEMDebugMessage(L"\tbiYPelsPerMeter: %ld\r\n", pBitmapInfoHeader->biYPelsPerMeter);
    OEMDebugMessage(L"\tbiClrUsed: %ld\r\n", pBitmapInfoHeader->biClrUsed);
    OEMDebugMessage(L"\tbiClrImportant: %ld\r\n", pBitmapInfoHeader->biClrImportant);

    OEMDebugMessage(L"\n");
}

void
vDumpPOINTL(
    INT          iDebugLevel,
    _In_ PWSTR   pszInLabel,
    POINTL       *pptl
    )

/*++

Routine Description:

    Dumps the members of a POINTL structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pptl - pointer to the POINTL strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pptl");

    // Return if strct to be dumped is invalid
    //
    if (!pptl)
    {
        OEMDebugMessage(L"\nPOINTL [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nPOINTL [%s]: %#x (%ld, %ld)\r\n", pszLabel, pptl, pptl->x, pptl->y);

    OEMDebugMessage(L"\n");
}

void
vDumpRECTL(
    INT          iDebugLevel,
    _In_ PWSTR   pszInLabel,
    RECTL        *prcl
    )

/*++

Routine Description:

    Dumps the members of a RECTL structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    prcl - pointer to the RECTL strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"prcl");

    // Return if strct to be dumped is invalid
    //
    if (!prcl)
    {
        OEMDebugMessage(L"\nRECTL [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nRECTL [%s]: %#x (%ld, %ld) (%ld, %ld)\r\n", pszLabel, prcl, prcl->left, prcl->top, prcl->right, prcl->bottom);

    OEMDebugMessage(L"\n");
}

#if DBG
    DBG_FLAGS gafdXLATEOBJ_flXlate[] = {
        { L"XO_DEVICE_ICM",     XO_DEVICE_ICM},
        { L"XO_FROM_CMYK",      XO_FROM_CMYK},
        { L"XO_HOST_ICM",       XO_HOST_ICM},
        { L"XO_TABLE",          XO_TABLE},
        { L"XO_TO_MONO",        XO_TO_MONO},
        { L"XO_TRIVIAL",            XO_TRIVIAL},
        {NULL, 0}               // The NULL entry is important
    };
#else
    DBG_FLAGS gafdXLATEOBJ_flXlate[] = {
        {NULL, 0}
    };
#endif

void
vDumpXLATEOBJ(
    INT         iDebugLevel,
    _In_ PWSTR  pszInLabel,
    XLATEOBJ    *pxlo
    )

/*++

Routine Description:

    Dumps the members of a XLATEOBJ structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pxlo - pointer to the XLATEOBJ strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pxlo");

    // Return if strct to be dumped is invalid
    //
    if (!pxlo)
    {
        OEMDebugMessage(L"\nXLATEOBJ [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nXLATEOBJ [%s]: %#x\r\n", pszLabel, pxlo);
    OEMDebugMessage(L"\tiUniq: %ld\r\n", pxlo->iUniq);
    OEMDebugMessage(L"\tflXlate: ");
    vDumpFlags(pxlo->flXlate, gafdXLATEOBJ_flXlate);
    OEMDebugMessage(L"\tiSrcType: %ld [obsolete]\r\n", pxlo->iSrcType);
    OEMDebugMessage(L"\tiDstType: %ld [obsolete]\r\n", pxlo->iDstType);
    OEMDebugMessage(L"\tcEntries: %ld\r\n", pxlo->cEntries);
    if (pxlo->pulXlate)
        OEMDebugMessage(L"\tpulXlate: %#x [%ld]\r\n", pxlo->pulXlate, *pxlo->pulXlate);
    else
        OEMDebugMessage(L"\tpulXlate: %#x\r\n", pxlo->pulXlate);

    OEMDebugMessage(L"\n");
}

#if DBG
    DBG_FLAGS gafdCOLORADJUSTMENT_caFlags[] = {
        { L"CA_NEGATIVE",       CA_NEGATIVE},
        { L"CA_LOG_FILTER",     CA_LOG_FILTER},
        {NULL, 0}               // The NULL entry is important
    };
#else
    DBG_FLAGS gafdCOLORADJUSTMENT_caFlags[] = {
        {NULL, 0}
    };
#endif

void
vDumpCOLORADJUSTMENT(
    INT             iDebugLevel,
    _In_ PWSTR      pszInLabel,
    COLORADJUSTMENT *pca
    )

/*++

Routine Description:

    Dumps the members of a COLORADJUSTMENT structure.

Arguments:

    iDebugLevel - desired output debug level
    pszInLabel - output label string
    pca - pointer to the COLORADJUSTMENT strct to be dumped

Return Value:

    NONE

--*/

{
    // Check if the debug level is appropriate
    //
    if (iDebugLevel < giDebugLevel)
    {
        // Nothing to output here
        //
        return;
    }

    // Prepare the label string
    //
    PCWSTR pszLabel = EnsureLabel(pszInLabel, L"pca");

    // Return if strct to be dumped is invalid
    //
    if (!pca)
    {
        OEMDebugMessage(L"\nCOLORADJUSTMENT [%s]: NULL\r\n", pszLabel);

        // Nothing else to output
        //
        return;
    }

    // Format the data
    //
    OEMDebugMessage(L"\nCOLORADJUSTMENT [%s]: %#x\r\n", pszLabel, pca);
    OEMDebugMessage(L"\tcaSize: %#x\r\n", pca->caSize);
    OEMDebugMessage(L"\tcaFlags: ");
    if (pca->caFlags)
        vDumpFlags(pca->caFlags, gafdCOLORADJUSTMENT_caFlags);
    else
        OEMDebugMessage(L"NULL\r\n");

    PWSTR pszcaIlluminantIndex = L"0";
    switch (pca->caIlluminantIndex)
    {
        case ILLUMINANT_DEVICE_DEFAULT: pszcaIlluminantIndex = L"ILLUMINANT_DEVICE_DEFAULT" ; break;
        case ILLUMINANT_A: pszcaIlluminantIndex = L"ILLUMINANT_A [Tungsten lamp]" ; break;
        case ILLUMINANT_B: pszcaIlluminantIndex = L"ILLUMINANT_B [Noon sunlight]" ; break;
        case ILLUMINANT_C: pszcaIlluminantIndex = L"ILLUMINANT_C [NTSC daylight]" ; break;
        case ILLUMINANT_D50: pszcaIlluminantIndex = L"ILLUMINANT_D50 [Normal print]" ; break;
        case ILLUMINANT_D55: pszcaIlluminantIndex = L"ILLUMINANT_D55 [Bond paper print]" ; break;
        case ILLUMINANT_D65: pszcaIlluminantIndex = L"ILLUMINANT_D65 [Standard daylight]" ; break;
        case ILLUMINANT_D75: pszcaIlluminantIndex = L"ILLUMINANT_D75 [Northern daylight]" ; break;
        case ILLUMINANT_F2: pszcaIlluminantIndex = L"ILLUMINANT_F2 [Cool white lamp]" ; break;
    }
    OEMDebugMessage(L"\tcaIlluminantIndex: %s\r\n", pszcaIlluminantIndex);

    OEMDebugMessage(L"\tcaRedGamma: %d\r\n", (int)pca->caRedGamma);
    OEMDebugMessage(L"\tcaGreenGamma: %d\r\n", (int)pca->caGreenGamma);
    OEMDebugMessage(L"\tcaBlueGamma: %d\r\n", (int)pca->caBlueGamma);
    OEMDebugMessage(L"\tcaReferenceBlack: %d\r\n", (int)pca->caReferenceBlack);
    OEMDebugMessage(L"\tcaReferenceWhite: %d\r\n", (int)pca->caReferenceWhite);
    OEMDebugMessage(L"\tcaContrast: %d\r\n", (int)pca->caContrast);
    OEMDebugMessage(L"\tcaBrightness: %d\r\n", (int)pca->caBrightness);
    OEMDebugMessage(L"\tcaColorfulness: %d\r\n", (int)pca->caColorfulness);
    OEMDebugMessage(L"\tcaRedGreenTint: %d\r\n", (int)pca->caRedGreenTint);

    OEMDebugMessage(L"\n");
}

#endif

