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

   Implementation of the UI plugin dllentry points.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdsmplcf.h"

/*++

Routine Name:

    DllMain

Routine Description:

    Entry point into the dynamic-link library (DLL).
    Called by the system when processes and threads are initialized and terminated,
    or upon calls to the LoadLibrary and FreeLibrary functions.

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

    Retrieves the class objects for the DLL.
    Supported class ids are CLSID_OEMUI and CLSID_OEMPTPROVIDER.
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
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppv, E_POINTER)))
    {
        *ppv = NULL;

        //
        // Make sure the appropriate CLSID is being requested
        //
        if (rclsid == CLSID_OEMUI ||
            rclsid == CLSID_OEMPTPROVIDER)
        {
            CXDSmplUICF* pXDSmplUICF = new(std::nothrow) CXDSmplUICF();
            hr = CHECK_POINTER(pXDSmplUICF, E_OUTOFMEMORY);

            if (SUCCEEDED(hr))
            {
                //
                // Get the requested interface.
                //
                hr = pXDSmplUICF->QueryInterface(riid, ppv);

                //
                // Release the IUnknown pointer.
                // (If QueryInterface failed, component will delete itself.)
                //
                pXDSmplUICF->Release();
            }
        }
        else
        {
            hr =  CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

