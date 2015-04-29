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

   Implementation of the page scaling filter dllentry points. Dllmain only
   stores the instance handle. DllGetClassObject calls on to a generic
   get class factory template function.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "clasfact.h"
#include "scaleflt.h"
#include "xdexcept.h"

/*++

Routine Name:

    DllMain

Routine Description:

    Entry point to the page scaling filter which is called when a new process is started

Arguments:

    hInst - Handle to the DLL module.
    wReason - Indicates why the DLL entry-point function is being called.

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

    Determines whether the DLL is in use.
    If not, the caller can unload the DLL from memory.

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

    Retrieves the class object for the DLL.
    Called from within the CoGetClassObject function.

Arguments:

    rclsid - CLSID that will associate the correct data and code.
    riid - Reference to the identifier of the interface that the caller is to use to communicate with the class object.
    ppv - Address of pointer variable that receives the interface pointer requested in riid.

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
    // 976EDCE4-274E-482a-9773-12453BE3E7F1
    //
    CLSID scalingCLSID = {0x976EDCE4, 0x274E, 0x482a, {0x97, 0x73, 0x12, 0x45, 0x3B, 0xE3, 0xE7, 0xF1}};

    return GetFilterClassFactory<CPageScalingFilter>(rclsid, riid, scalingCLSID, ppv);
}

