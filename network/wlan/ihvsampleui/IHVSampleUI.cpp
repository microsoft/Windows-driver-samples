//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//


#include "precomp.h"

CIHVClassFactory *g_pIHVClassFactory = NULL;

// instance handle to dll
HINSTANCE g_hInst;

// object ref count
long g_objRefCount = 0;
long g_serverLock = 0; //lock count on server

//
// DllRegisterServer - Adds entries to the system registry
//
STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    hr = CRegHelper::RegisterServer();

    return hr;
}

//
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    hr = CRegHelper::UnregisterServer();

    return hr;
}



//
// Used to determine whether the DLL can be unloaded by COM
//
STDAPI DllCanUnloadNow(void)
{
	if ((g_objRefCount == 0) && (g_serverLock == 0) /*&& (_Module.GetLockCount() == 0)*/)
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}



STDAPI 
DllGetClassObject(
   _In_ REFCLSID rclsid, 
   _In_ REFIID riid, 
   _Outptr_ LPVOID *ppv)
{
   HRESULT hr = E_NOINTERFACE;

   if (NULL == g_pIHVClassFactory)
   {
       g_pIHVClassFactory = new(std::nothrow) CIHVClassFactory();
       if (NULL == g_pIHVClassFactory)
       {
           hr = E_OUTOFMEMORY;
       }
   }

   if(NULL != g_pIHVClassFactory && 
        (rclsid == GUID_SAMPLE_IHVUI_CLSID || rclsid == IID_IWizardExtension))
   {
        hr = g_pIHVClassFactory->QueryInterface(riid, ppv);
   }
   return hr;
}


//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Synopsis:  Main entry point into the DLL.
//
// Arguments: HINSTANCE  hinstDLL - Our HINSTANCE
//            DWORD  fdwReason - The reason we are being called.
//            LPVOID  lpvReserved - Reserved
//
// Returns:   BOOL WINAPI - TRUE - always
//
//+----------------------------------------------------------------------------
extern "C" 
BOOL WINAPI 
DllMain(
    HINSTANCE  hInstDLL, 
    DWORD  fdwReason, 
    LPVOID  lpvReserved
    ) 
{
    UNREFERENCED_PARAMETER(lpvReserved);

    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        // Set our global instance handle
        g_hInst = hInstDLL;
        (VOID)DisableThreadLibraryCalls(hInstDLL);
    }
    else if (DLL_PROCESS_DETACH == fdwReason)
    {
    }

    return TRUE;
}

