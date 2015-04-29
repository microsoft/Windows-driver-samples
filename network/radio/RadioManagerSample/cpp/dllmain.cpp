#include "precomp.h"
#pragma hdrstop

OBJECT_ENTRY_AUTO(__uuidof(SampleRadioManager), CSampleRadioManager);

class CSampleRadioModule : public CAtlDllModuleT<CSampleRadioModule>
{
public:
};

static CSampleRadioModule s_AtlModule;

//+---------------------------------------------------------------------------
// DLL Entry Point
//

EXTERN_C BOOL WINAPI DllMain
(
    HINSTANCE /*hInstance*/,
    DWORD dwReason,
    LPVOID pvReserved
)
{
    return s_AtlModule.DllMain(dwReason, pvReserved); 
}

STDAPI DllCanUnloadNow(void)
{
    return s_AtlModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(
    _In_ REFCLSID rclsid, 
    _In_ REFIID riid, 
    _Outptr_ LPVOID* ppv
    )
{
    return s_AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


//+---------------------------------------------------------------------------
// Adds entries to the system registry. Needed because the binary need copy to test machine.
// If binary is deployed by manifest, these functions are not needed.
STDAPI
DllRegisterServer ()
{
    s_AtlModule.DllRegisterServer(FALSE);
    return S_OK;
}

STDAPI
DllUnregisterServer ()
{
    HRESULT hr = s_AtlModule.DllUnregisterServer();
    return hr;
}
