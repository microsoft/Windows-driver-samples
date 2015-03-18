// Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include "WpdWudfSampleDriver.h"

#include "WpdWudfSampleDriver.tmh"

HINSTANCE g_hInstance = NULL;

class CWpdWudfSampleDriverModule : public CAtlDllModuleT< CWpdWudfSampleDriverModule >
{
public :
    DECLARE_LIBID(LIBID_WpdWudfSampleDriverLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_WpdWudfSampleDriver, "{9FF28171-F2BA-4720-AAE1-92DA54E8BB0E}")
};

CWpdWudfSampleDriverModule _AtlModule;


// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        // Initialize tracing.
        WPP_INIT_TRACING(MYDRIVER_TRACING_ID);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // Cleanup tracing.
        WPP_CLEANUP();
    }

    return _AtlModule.DllMain(dwReason, lpReserved);
}


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
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
