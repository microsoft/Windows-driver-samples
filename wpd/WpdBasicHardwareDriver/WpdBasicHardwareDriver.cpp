#include "stdafx.h"

#include "WpdBasicHardwareDriver.tmh"

HINSTANCE g_hInstance = NULL;

class CWpdBasicHardwareDriverModule : public CAtlDllModuleT< CWpdBasicHardwareDriverModule >
{
public :
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_WpdBasicHardwareDriver, "{021AD204-6411-4698-8CFB-C1A72B581733}")
    DECLARE_LIBID(LIBID_WpdBasicHardwareDriverLib)
};

CWpdBasicHardwareDriverModule _AtlModule;

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
