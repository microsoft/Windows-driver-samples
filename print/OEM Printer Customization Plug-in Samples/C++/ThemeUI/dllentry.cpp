//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   dllentry.cpp
//
//
//  PURPOSE:  Source module for DLL entry function(s).
//

#include "precomp.h"
#include "oemui.h"
#include "fusutils.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

///////////////////////////////////////////////////////////
//
// DLL entry point
//
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            VERBOSE(DLLTEXT("Process attach.\r\n"));

            // Store the module handle in case we need it later.
            ghInstance = hInst;

            // NOTE: We don't create an Activation Context on module load,
            //       but on need of an Avtivation Context; the first time
            //       GetMyActivationContext() or CreateMyActivationContext() is called.
            break;

        case DLL_THREAD_ATTACH:
            VERBOSE(DLLTEXT("Thread attach.\r\n"));
            break;

        case DLL_PROCESS_DETACH:
            VERBOSE(DLLTEXT("Process detach.\r\n"));

            // Release the Activation Context if we created one somewhere
            // by calling GetMyActivationContext() or CreateMyActivationContext();
            if(INVALID_HANDLE_VALUE != ghActCtx)
            {
                ReleaseActCtx(ghActCtx);
                ghActCtx = INVALID_HANDLE_VALUE;
            }
            break;

        case DLL_THREAD_DETACH:
            VERBOSE(DLLTEXT("Thread detach.\r\n"));
            break;
    }

    return TRUE;
}

