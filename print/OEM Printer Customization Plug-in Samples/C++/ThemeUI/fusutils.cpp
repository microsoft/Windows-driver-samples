//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	fsutils.h
//    
//
//  PURPOSE:   Fusion utilities
//

#include "precomp.h"
#include "globals.h"
#include "fusutils.h"

#define MAX_LOOP    10

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


HANDLE GetMyActivationContext()
{
    // Make sure we've created our activation context.
    CreateMyActivationContext();

    // Return the global.
    return ghActCtx;
}


BOOL CreateMyActivationContext()
{
    if(INVALID_HANDLE_VALUE != ghActCtx)
    {
        return TRUE;
    }

    ghActCtx = CreateActivationContextFromResource(ghInstance, MAKEINTRESOURCE(MANIFEST_RESOURCE));

    return (INVALID_HANDLE_VALUE != ghActCtx);
}



HANDLE CreateActivationContextFromResource(HMODULE hModule, LPCTSTR pszResourceName)
{
    DWORD   dwSize          = MAX_PATH;
    DWORD   dwUsed          = 0;
    DWORD   dwLoop;
    PTSTR   pszModuleName   = NULL;
    ACTCTX  act;
    HANDLE  hActCtx         = INVALID_HANDLE_VALUE;

    
    // Get the name for the module that contains the manifest resource
    // to create the Activation Context from.
    dwLoop = 0;
    do 
    {
        // May need to allocate or re-alloc buffer for module name.
        if(NULL != pszModuleName)
        {
            // Need to re-alloc bigger buffer.

            // First, delete old buffer.
            delete[] pszModuleName;

            // Second, increase buffer alloc size.
            dwSize <<= 1;
        }
        pszModuleName = new TCHAR[dwSize];
        if(NULL == pszModuleName)
        {
            goto Exit;
        }

        // Try to get the module name.
        dwUsed = GetModuleFileName(hModule, pszModuleName, dwSize);

        // Check to see if it failed.
        if(0 == dwUsed)
        {
            goto Exit;
        }

        // If dwUsed is equla to dwSize or larger,
        // the buffer passed in wasn't big enough.
    } while ( (dwUsed >= dwSize) && (++dwLoop < MAX_LOOP) );

    // Now let's try to create an activation context
    // from manifest resource.
    ::ZeroMemory(&act, sizeof(act));
    act.cbSize          = sizeof(act);
    act.dwFlags         = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpResourceName  = pszResourceName;
    act.lpSource        = pszModuleName;

    hActCtx = CreateActCtx(&act);


Exit:

    //
    //  Clean up.
    //

    if(NULL != pszModuleName)
    {
        delete[] pszModuleName;
    }

    return hActCtx;
}


