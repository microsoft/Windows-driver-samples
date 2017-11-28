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

#include "precomp.h"

#include "resource.h"
#include "devmode.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


HRESULT hrOEMDevMode(
    DWORD           dwMode, 
    POEMDMPARAM pOemDMParam
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DevMode. 
    hrOEMDevMode is called by IPrintOemUni::DevMode 
    which is defined in intrface.cpp. 

    The IPrintOemUni::DevMode method, provided by 
    rendering plug-ins for Unidrv, performs operations 
    on private DEVMODE members.
    
    Please refer to DDK documentation for more details.

Arguments:

    dwMode - caller-supplied constant. Refer to the docs 
        for more information. 
    pOemDMParam - pointer to an OEMDMPARAM structure.

Return Value:

    S_OK The operation succeeded. 
    E_FAIL The operation failed. 

--*/

{
    POEMDEV pOEMDevIn;
    POEMDEV pOEMDevOut;

    VERBOSE("hrOEMDevMode entry.");

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
        ))
    {
        ERR("DevMode() ERROR_INVALID_PARAMETER.\r\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Cast generic (i.e. PVOID) to OEM private devmode pointer type.
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
            pOEMDevOut->dwDriverData            = 0;
            break;

        case OEMDM_CONVERT:
            bConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            break;

        case OEMDM_MERGE:
            bConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
            bMakeOEMDevmodeValid(pOEMDevOut);
            break;
    }

    return S_OK;
}


BOOL bConvertOEMDevmode(
    PCOEMDEV    pOEMDevIn, 
    POEMDEV pOEMDevOut
    )

/*++

Routine Description:

    Converts private DEVMODE members to the 
    current version.

Arguments:

    pOEMDevIn - pointer to OEM private devmode
    pOEMDevOut - pointer to OEM private devmode

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE("bConvertOEMDevmode entry.");
    
    if( (NULL == pOEMDevIn)
        ||
        (NULL == pOEMDevOut)
        )
    {
        ERR("ConvertOEMDevmode() invalid parameters.\r\n");
        return FALSE;
    }

    // Check OEM Signature, if it doesn't match ours,
    // then just assume DMIn is bad and use defaults.
    if(pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature)
    {
        VERBOSE("Converting private OEM Devmode.\r\n");

        // Set the devmode defaults so that anything the isn't copied over will
        // be set to the default value.
        pOEMDevOut->dwDriverData = 0;

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
        VERBOSE("Unknown DEVMODE signature, pOEMDMIn ignored.\r\n");

        // Don't know what the input DEVMODE is, so just use defaults.
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
        pOEMDevOut->dwDriverData            = 0;
    }

    return TRUE;
}


BOOL bMakeOEMDevmodeValid(
    POEMDEV pOEMDevmode
    )

/*++

Routine Description:

    Ensures that private OEM devmode members are valid.

Arguments:

    pOEMDevmode - pointer to OEM private devmode
    
Return Value:

    TRUE if successful, FALSE if there is an error

--*/

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

    // Set driver data.
    pOEMDevmode->dwDriverData = 0;

    return TRUE;
}

