/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   dllentry.cpp

Abstract:

   Implementation of the color management filter dllentry points. Dllmain only
   stores the instance handle. DllGetClassObject calls on to a generic
   get class factory template function.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "clasfact.h"
#include "cmflt.h"
#include "xdexcept.h"
#include "xdstring.h"

/*++

Routine Name:

    DllMain

Routine Description:

    Entry point to the color management filter which is called when a new process is started

Arguments:

    hInst   - Handle to the DLL
    wReason - Specifies a flag indicating why the DLL entry-point function is being called

Return Value:

    TRUE

--*/
BOOL WINAPI
DllMain(
    _In_     HINSTANCE hInst,
    _In_     WORD      wReason,
    _In_opt_ LPVOID
    )
{
    switch (wReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hInstance = hInst;
        }
        break;
    }

    return TRUE;
}

/*++

Routine Name:

    DllCanUnloadNow

Routine Description:

    Method which reports whether the DLL is in use to allow the caller to unload
    the DLL safely

Arguments:

    None

Return Value:

    HRESULT
    S_OK    - Dll can unload
    S_FALSE - Dll can't unload

--*/
STDAPI
DllCanUnloadNow()
{
    if (g_cServerLocks == 0)
    {
        return S_OK ;
    }
    else
    {
        return S_FALSE;
    }
}

/*++

Routine Name:

    DllGetClassObject

Routine Description:

    Method to return the current class object

Arguments:

    rclsid - CLSID that will associate the correct data and code
    riid   - Reference to the identifier of the interface that the caller is to use to communicate with the class object
    ppv    - Address of pointer variable that receives the interface pointer requested in riid

Return Value:

    HRESULT
    S_OK                      - On success
    E_*                       - On error
    CLASS_E_CLASSNOTAVAILABLE - On unsupported class

--*/
STDAPI
DllGetClassObject(
    _In_        REFCLSID    rclsid,
    _In_        REFIID      riid,
    _Outptr_ LPVOID FAR* ppv
    )
{
    //
    // 21934A6D-EA30-480a-8DAD-C8045807F737
    //
    CLSID colorCLSID = {0x21934A6D, 0xEA30, 0x480a, {0x8D, 0xAD, 0xC8, 0x04, 0x58, 0x07, 0xF7, 0x37}};

    return GetFilterClassFactory<CColorManageFilter>(rclsid, riid, colorCLSID, ppv);
}

