//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE: devmode.cpp
//    
//  PURPOSE: Implementation of Devmode functions shared with OEM UI and OEM rendering modules.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

BOOL 
ConvertOEMDevmode(
    _In_ PCOEMDEV pOEMDevIn, 
    _Inout_ POEMDEV pOEMDevOut
    );

BOOL 
MakeOEMDevmodeValid(
    _Out_ POEMDEV pOEMDevmode
    );

//+---------------------------------------------------------------------------
//
//  Member:
//      ::hrOEMDevMode
//
//  Synopsis:
//      Performs operation on UI Plugins Private DevMode Members.
//      Called via IOemUI::DevMode
//
//  Returns:
//      S_OK or E_FAIL.  For more detailed failure info, 
//      check GetLastError
//
//  Last error:
//      ERROR_INVALID_PARAMETER
//
//
//----------------------------------------------------------------------------
HRESULT 
hrOEMDevMode(
    DWORD dwMode,
        // Indicates which operation should be performed
    _Inout_ POEMDMPARAM pOemDMParam
        // contains various data structures needed by this 
        // routine.  The usage of this struct is determined by
        // the dwMode flag.
        )
{
    VERBOSE(DLLTEXT("hrOEMDevMode entry."));

    HRESULT hr = S_OK;
        // Return value.

    //
    // Verify parameters.
    // 
    if ((NULL == pOemDMParam) ||
        ((OEMDM_SIZE != dwMode) &&
         (OEMDM_DEFAULT != dwMode) &&
         (OEMDM_CONVERT != dwMode) &&
         (OEMDM_MERGE != dwMode)))
    {
        ERR(ERRORTEXT("DevMode() ERROR_INVALID_PARAMETER.\r\n"));
        ERR(DLLTEXT("\tdwMode = %d, pOemDMParam = %#lx.\r\n"), dwMode, pOemDMParam);

        SetLastError(ERROR_INVALID_PARAMETER);
        hr = E_FAIL;
    }
    else
    {
        POEMDEV pOEMDevIn = (POEMDEV) pOemDMParam->pOEMDMIn;;
            // Pointer to the supplied OEM devmode data, if applicable
            // based on dwMode.

        POEMDEV pOEMDevOut = (POEMDEV) pOemDMParam->pOEMDMOut;;
            // Pointer to the DEVMODE data that this routine is 
            // expected to configure, if applicable based on dwMode.

        switch (dwMode)
        {
            //
            // The Method should return the size of the memory allocation 
            // needed to store the UI plugin Private DEVMODE.
            //
            case OEMDM_SIZE:
                pOemDMParam->cbBufSize = sizeof(OEMDEV);
                break;

            //
            //Should fill the Private DEVMODE with the default values.
            //
            case OEMDM_DEFAULT:
                //
                //OEM_DMEXTRAHEADER Members
                //
                pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
                pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
                pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
                break;
                
            //
            // The method should convert private DEVMODE members to 
            // the current version, if necessary.
            //
            case OEMDM_CONVERT:
                ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
                break;
            
            //
            // The method should validate the information contained in private 
            // DEVMODE members and merge validated values into a private 
            // DEVMODE structure containing default values
            //
            case OEMDM_MERGE:
                ConvertOEMDevmode(pOEMDevIn, pOEMDevOut);
                MakeOEMDevmodeValid(pOEMDevOut);
                break;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::ConvertOEMDevmode
//
//  Synopsis:
//      confirm that the private DEVMODE being passed in is data that
//      this driver recognizes, read the settings from the input
//      OEM DEVMODE data, and write them to pOEMDevOut.
//
//  Returns:
//      True unless the routine was called with null pointers
//
//
//----------------------------------------------------------------------------
BOOL 
ConvertOEMDevmode(
    _In_ PCOEMDEV pOEMDevIn, 
        // Caller supplied DEVMODE that we are attempting to read
    _Inout_ POEMDEV pOEMDevOut
        // OEM private DEVMODE configured by this routine based on the settings
        // in pOEMDevIn
    )
{
    VERBOSE(DLLTEXT("ConvertOEMDevmode entry."));

    if ((NULL == pOEMDevIn) ||
        (NULL == pOEMDevOut))
    {
        ERR(ERRORTEXT("ConvertOEMDevmode() invalid parameters.\r\n"));
        return FALSE;
    }

    //
    // Check OEM Signature, if it doesn't match ours,
    // then just assume DMIn is bad and use defaults.
    // 
    if(pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature)
    {
        VERBOSE(DLLTEXT("Converting private OEM Devmode.\r\n"));

        //
        // Copy the old structure in to the new using which ever size is the 
        // smaller.  Devmode maybe from newer Devmode (not likely since there 
        // is only one), or Devmode maybe a newer Devmode, in which case it 
        // maybe larger, but the first part of the structure should be the same.
        //
        // DESIGN ASSUMPTION: the private DEVMODE structure only gets added to;
        // the fields that are in the DEVMODE never change only new fields get 
        // added to the end.
        // 
        memcpy(pOEMDevOut, 
               pOEMDevIn, 
               __min(pOEMDevOut->dmOEMExtra.dwSize, pOEMDevIn->dmOEMExtra.dwSize));

        //
        // Re-fill in the size and version fields to indicated 
        // that the DEVMODE is the current private DEVMODE version.
        // 
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;

        //
        // If the plug-in is capable of recognizing multiple versions of it's 
        // private DEVMODE data add custom handling here to support it.
        //
    }
    else
    {
        WARNING(DLLTEXT("Unknown DEVMODE signature, pOEMDMIn ignored.\r\n"));

        //
        // The private DEVMODE data is not something that this plug-in understands,
        // so use defaults.
        // 
        pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
        pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::MakeOEMDevmodeValid
//
//  Synopsis:
//      Force the OEM devmode settings to valid values.
//
//  Returns:
//      True if the OEMDEV was configured, else FALSE.  Only return false if
//      the parameter is NULL.
//
//
//----------------------------------------------------------------------------
BOOL 
MakeOEMDevmodeValid(
    _Out_ POEMDEV pOEMDevmode
        // OEM DEVMODE data to modify
    )
{
    if(NULL == pOEMDevmode)
    {
        return FALSE;
    }

    //
    // Assumption: pOEMDevmode is large enough to contain OEMDEV structure.
    // 
    pOEMDevmode->dmOEMExtra.dwSize       = sizeof(OEMDEV);
    pOEMDevmode->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
    pOEMDevmode->dmOEMExtra.dwVersion    = OEM_VERSION;

    return TRUE;
}


