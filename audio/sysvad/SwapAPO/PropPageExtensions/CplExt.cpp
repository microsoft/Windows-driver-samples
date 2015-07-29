//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    CplExt.cpp
//
// Abstract:    Implementation of DLL Exports
//
// ----------------------------------------------------------------------------


#include "stdafx.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)

HINSTANCE   g_hInstance = NULL;


class CCplExtModule : public CAtlDllModuleT< CCplExtModule >
{
public :
    DECLARE_LIBID(LIBID_CplExtLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_CPL_EXT, "{A7D2EC8B-B70F-434C-A0CE-0DF324805F7D}")
};

CCplExtModule _AtlModule;


// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    g_hInstance = hInstance;
    return _AtlModule.DllMain(dwReason, lpReserved);
}


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(_In_ REFCLSID rclsid,_In_  REFIID riid, _Outptr_ LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
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
