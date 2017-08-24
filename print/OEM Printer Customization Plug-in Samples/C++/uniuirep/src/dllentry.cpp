//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	dllentry.cpp
//    
//  PURPOSE:  Source module for DLL entry function(s).
//
//--------------------------------------------------------------------------

#include "precomp.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);



//+---------------------------------------------------------------------------
//
//  Member:
//      ::DllMain
//
//  Synopsis:
//      Dll's main routine, called when new 
//      threads or processes reference the module
//
//  Returns:
//      TRUE.  Currently has no failure paths.  Return false if the module
//      cannot be properly initialized.
//
//  Notes:
//      The caller of this routine holds a process-wide lock preventing more 
//      than one module from being initialized at a time.  In general, 
//      plug-ins should do as little as possible in this routine to avoid 
//      stalling other threads that may need to load modules.  Also, calling
//      any function that would, in-turn, cause another module to be loaded 
//      could result in a deadlock.
//
//
//----------------------------------------------------------------------------
BOOL 
WINAPI DllMain(
    HINSTANCE hInst, 
    WORD wReason, 
    LPVOID lpReserved
    )
{
    VERBOSE(DLLTEXT("DllMain entry."));

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            VERBOSE(DLLTEXT("Process attach.\r\n"));
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            VERBOSE(DLLTEXT("Thread attach.\r\n"));
            break;

        case DLL_PROCESS_DETACH:
            VERBOSE(DLLTEXT("Process detach.\r\n"));
            break;

        case DLL_THREAD_DETACH:
            VERBOSE(DLLTEXT("Thread detach.\r\n"));
            break;
    }

    return TRUE;
}



