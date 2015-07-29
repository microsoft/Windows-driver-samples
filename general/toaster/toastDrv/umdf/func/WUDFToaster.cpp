/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    WUDFToaster.cpp

  Abstract:

    Implementation of DLL Exports.

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "stdafx.h"
#include "resource.h"

#include "WUDFToaster.h"

#include "internal.h"
#include "WUDFToaster.tmh"
 
const GUID CLSID_MyDriverCoClass = MYDRIVER_CLASS_ID;

class CToasterDriverModule : public CAtlDllModuleT< CToasterDriverModule >
{
public :
    DECLARE_LIBID(LIBID_WUDFToasterLib)	
};

CToasterDriverModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    hInstance;

    if(dwReason == DLL_PROCESS_ATTACH)
    {
        //
        // Initialize tracing.
        //

        WPP_INIT_TRACING(MYDRIVER_TRACING_ID);
    }
    else if(dwReason == DLL_PROCESS_DETACH)
    {
        //
        // Cleanup tracing.
        //

        WPP_CLEANUP();
    }
    
    return _AtlModule.DllMain(dwReason, lpReserved); 
}

// Returns a class factory to create an object of the requested type
_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
    return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    HRESULT hr = _AtlModule.DllUnregisterServer();
    return hr;
}
