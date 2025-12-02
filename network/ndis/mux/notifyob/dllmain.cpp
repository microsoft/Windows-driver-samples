//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       DLLMAIN . C P P
//
//  Contents:   Main entry points into the DLL
//
//  Notes:
//
//----------------------------------------------------------------------------


#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include <devguid.h>

#include "notify.h"
#include "notifyn_i.c"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CMuxNotify, CMuxNotify)
END_OBJECT_MAP()





/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain (HINSTANCE hInstance,
                     DWORD dwReason,
                     LPVOID /*lpReserved*/)
{
	TraceMsg( L"-->DllMain.\n");

	if (dwReason == DLL_PROCESS_ATTACH) {

		TraceMsg( L"   Reason: Attach.\n");

		_Module.Init(ObjectMap, hInstance);

		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {

		TraceMsg( L"   Reason: Detach.\n");

		   _Module.Term();
	}

	TraceMsg( L"<--DllMain.\n");

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
	HRESULT hr;

	TraceMsg( L"-->DllCanUnloadNow.\n");

	hr = (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;

	TraceMsg( L"-->DllCanUnloadNow(HRESULT = %x).\n",
	        hr );

	return hr;  
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
	TraceMsg( L"-->DllGetClassObject.\n");

	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// Registers object, typelib and all interfaces in typelib

	TraceMsg( L"-->DllRegisterServer.\n");

	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	TraceMsg( L"-->DllUnregisterServer.\n");

	_Module.UnregisterServer();

	return S_OK;
}
