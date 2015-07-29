//
// SwapAPODll.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Author:
//
// Description:
//
// SwapAPODll.cpp : Implementation of DLL Exports.

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlsync.h>
#include <mmreg.h>

#include "resource.h"
#include "SwapAPODll.h"
#include <SwapAPO.h>

#include <SwapAPODll_i.c>

//-------------------------------------------------------------------------
// Array of APO_REG_PROPERTIES structures implemented in this module.
// Each new APO implementation will be added to this array.
//
APO_REG_PROPERTIES const *gCoreAPOs[] =
{
    &CSwapAPOMFX::sm_RegProperties.m_Properties,
    &CSwapAPOSFX::sm_RegProperties.m_Properties
};

// {secret}
class CSwapAPODllModule : public CAtlDllModuleT< CSwapAPODllModule >
{
public :
    DECLARE_LIBID(LIBID_SwapAPODlllib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SWAPAPODLL, "{0A21D954-674A-4C09-806E-DB4FBE8F199C}")
public:
    HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE) throw();
    HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE) throw();
};

// {secret}
CSwapAPODllModule _AtlModule;


HRESULT CSwapAPODllModule::DllRegisterServer(BOOL bRegTypeLib) throw()
{
    HRESULT hResult;
    UINT32 u32APORegIndex = 0;
    UINT32 u32APOUnregIndex = 0;

    // Register all APOs implemented in this module.
    for (u32APORegIndex = 0; u32APORegIndex < SIZEOF_ARRAY(gCoreAPOs); u32APORegIndex++)
    {
        hResult = RegisterAPO(gCoreAPOs[u32APORegIndex]);
        if (FAILED(hResult))
        {
           goto ExitFailed;
        }
    }

    // Register the module.
    hResult = CAtlDllModuleT<CSwapAPODllModule>::DllRegisterServer(bRegTypeLib);
    if (FAILED(hResult))
    {
        goto ExitFailed;
    }

    return hResult;

ExitFailed:
    // Unregister all registered APOs if something failed.
    for (u32APOUnregIndex = 0; u32APOUnregIndex < u32APORegIndex; u32APOUnregIndex++)
    {
        ATLVERIFY(SUCCEEDED(UnregisterAPO(gCoreAPOs[u32APOUnregIndex]->clsid)));
    }

    return hResult;
} // DllRegisterServer

HRESULT CSwapAPODllModule::DllUnregisterServer(BOOL bUnRegTypeLib) throw()
{
    HRESULT hResult;
    UINT32 u32APOUnregIndex = 0;

    // Unregister the module.
    hResult = CAtlDllModuleT<CSwapAPODllModule>::UnregisterServer(bUnRegTypeLib);
    if (FAILED(hResult))
    {
        goto Exit;
    }

    // Unregister all the APOs that are implemented in this module.
    for (u32APOUnregIndex = 0; u32APOUnregIndex < SIZEOF_ARRAY(gCoreAPOs); u32APOUnregIndex++)
    {
        hResult = UnregisterAPO(gCoreAPOs[u32APOUnregIndex]->clsid);
        ATLASSERT(SUCCEEDED(hResult));
    }

Exit:
    return hResult;
} // DllUnregisterServer

// {secret}
extern "C" BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD dwReason, LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
    }
    // do necessary cleanup only if the DLL is being unloaded dynamically
    else if ((DLL_PROCESS_DETACH == dwReason) && (NULL == lpReserved))
    {
    }

    return _AtlModule.DllMain(dwReason, lpReserved);
}


// {secret}
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// {secret}
STDAPI DllGetClassObject(_In_ REFCLSID rclsid,_In_  REFIID riid, _Outptr_ LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// {secret}
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
    return hr;
}


// {secret}
STDAPI DllUnregisterServer(void)
{
    HRESULT hr = _AtlModule.DllUnregisterServer();
    return hr;
}
