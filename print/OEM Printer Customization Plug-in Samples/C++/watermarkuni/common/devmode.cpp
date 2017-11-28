//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Devmode.cpp
//
//  PURPOSE:  Implementation of Devmode functions shared with OEM UI and OEM rendering modules.
//

// This file is wrapped in fdebug.cpp in other parts of the watermark sample.
// The appropriate precompiled header should be included there.
// #include "precomp.h
#include "debug.h"
#include "devmode.h"

// indicate to prefast that this is a user-mode component.
_Analysis_mode_(_Analysis_code_type_user_driver_);

HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam)
{
    HRESULT hResult     = S_OK;
    POEMDEV pOEMDevIn   = NULL;
    POEMDEV pOEMDevOut  = NULL;


    // Verify parameters.
    if( (NULL == pOemDMParam)
        ||
        ( (OEMDM_SIZE != dwMode)
          &&
          (OEMDM_DEFAULT != dwMode)
          &&
          (OEMDM_CONVERT != dwMode)
          &&
          (OEMDM_MERGE != dwMode)
        )
      )
    {
        ERR(DLLTEXT("DevMode() ERROR_INVALID_PARAMETER.\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Cast generic (i.e. PVOID) to OEM private devomode pointer type.
    pOEMDevIn = (POEMDEV) pOemDMParam->pOEMDMIn;
    pOEMDevOut = (POEMDEV) pOemDMParam->pOEMDMOut;

    switch(dwMode)
    {
        case OEMDM_SIZE:
            pOemDMParam->cbBufSize = sizeof(OEMDEV);
            break;

        case OEMDM_DEFAULT:
            pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
            pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
            pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
            pOEMDevOut->bWmarkEnabled                = WATER_MARK_DEFAULT_ENABLED;
            pOEMDevOut->dwWmarkFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
            pOEMDevOut->crWmarkTextColor             = WATER_MARK_DEFAULT_COLOR;
            hResult = StringCbCopyW(pOEMDevOut->szWaterMark, sizeof(pOEMDevOut->szWaterMark), WATER_MARK_DEFAULT_TEXT);
            break;

        case OEMDM_CONVERT:
            ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            break;

        case OEMDM_MERGE:
            ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            MakeOEMDevmodeValid(pOEMDevOut);
            break;
    }

    return hResult;
}


BOOL ConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut)
{
    HRESULT hCopy = S_OK;


    if( (NULL == pOEMDevIn)
        ||
        (NULL == pOEMDevOut)
      )
    {
        ERR(DLLTEXT("ConvertOEMDevmode() invalid parameters.\r\n"));
        return FALSE;
    }

    // Check OEM Signature, if it doesn't match ours,
    // then just assume DMIn is bad and use defaults.
    if(pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature)
    {
        VERBOSE(DLLTEXT("Converting private OEM Devmode.\r\n"));

        // Set the devmode defaults so that anything the isn't copied over will
        // be set to the default value.
        pOEMDevOut->bWmarkEnabled                = WATER_MARK_DEFAULT_ENABLED;
        pOEMDevOut->dwWmarkFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
        pOEMDevOut->crWmarkTextColor             = WATER_MARK_DEFAULT_COLOR;
        hCopy = StringCbCopyW(pOEMDevOut->szWaterMark, sizeof(pOEMDevOut->szWaterMark), WATER_MARK_DEFAULT_TEXT);

        // Copy the old structure in to the new using which ever size is the smaller.
        // Devmode maybe from newer Devmode (not likely since there is only one), or
        // Devmode maybe a newer Devmode, in which case it maybe larger,
        // but the first part of the structure should be the same.

        // DESIGN ASSUMPTION: the private DEVMODE structure only gets added to;
        // the fields that are in the DEVMODE never change only new fields get added to the end.

        memcpy(pOEMDevOut, pOEMDevIn, __min(pOEMDevOut->dmOEMExtra.dwSize, pOEMDevIn->dmOEMExtra.dwSize));

        // Re-fill in the size and version fields to indicated 
        // that the DEVMODE is the current private DEVMODE version.
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
    }
    else
    {
        WARNING(DLLTEXT("Unknown DEVMODE signature, pOEMDMIn ignored.\r\n"));

        // Don't know what the input DEVMODE is, so just use defaults.
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
        pOEMDevOut->bWmarkEnabled                = WATER_MARK_DEFAULT_ENABLED;
        pOEMDevOut->dwWmarkFontSize              = WATER_MARK_DEFAULT_FONTSIZE;
        pOEMDevOut->crWmarkTextColor             = WATER_MARK_DEFAULT_COLOR;
        hCopy = StringCbCopyW(pOEMDevOut->szWaterMark, sizeof(pOEMDevOut->szWaterMark), WATER_MARK_DEFAULT_TEXT);
    }

    return SUCCEEDED(hCopy);
}


BOOL MakeOEMDevmodeValid(POEMDEV pOEMDevmode)
{
    if(NULL == pOEMDevmode)
    {
        return FALSE;
    }

    // ASSUMPTION: pOEMDevmode is large enough to contain OEMDEV structure.

    // Make sure that dmOEMExtra indicates the current OEMDEV structure.
    pOEMDevmode->dmOEMExtra.dwSize       = sizeof(OEMDEV);
    pOEMDevmode->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
    pOEMDevmode->dmOEMExtra.dwVersion    = OEM_VERSION;

    // bEnable should be either TRUE or FALSE.
    if( (TRUE != pOEMDevmode->bWmarkEnabled)
        &&
        (FALSE != pOEMDevmode->bWmarkEnabled)
      )
    {
        pOEMDevmode->bWmarkEnabled = WATER_MARK_DEFAULT_ENABLED;
    }


    // dwWmarkFontSize should be 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, or 72.
    if(!IsValidFontSize(pOEMDevmode->dwWmarkFontSize))
    {
        pOEMDevmode->dwWmarkFontSize = WATER_MARK_DEFAULT_FONTSIZE;
    }

    // The high byte of the hi word should be zero.
    if(0 != HIBYTE(HIWORD(pOEMDevmode->crWmarkTextColor)))
    {
        pOEMDevmode->crWmarkTextColor = WATER_MARK_DEFAULT_COLOR;
    }

    // Make sure that water mark string is terminated.
    pOEMDevmode->szWaterMark[WATER_MARK_TEXT_SIZE - 1] = L'\0';

    return TRUE;
}


BOOL IsValidFontSize(DWORD dwWmarkFontSize)
{
    BOOL    bValid = FALSE;


    switch(dwWmarkFontSize)
    {
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 14:
        case 16:
        case 18:
        case 20:
        case 22:
        case 24:
        case 26:
        case 28:
        case 36:
        case 48:
        case 72:
            bValid = TRUE;
            break;

        default:
            ERR(DLLTEXT("IsValidFontSize() found invalid font size\r\n"));
            break;
    }

    return bValid;
}
