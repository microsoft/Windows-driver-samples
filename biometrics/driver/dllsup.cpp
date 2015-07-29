/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Dllsup.cpp

Abstract:

    This module contains the implementation of the Driver DLL entry point.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "dllsup.tmh"

//
// TODO - define a new GUID here
// This GUID goes in the inf file in the DriverCLSID value for the service binary
// {F1CB3C15-A916-47bc-BEA1-D5D4163BC6AE}
//
const CLSID CLSID_BiometricUsbSample = 
{ 0xf1cb3c15, 0xa916, 0x47bc, { 0xbe, 0xa1, 0xd5, 0xd4, 0x16, 0x3b, 0xc6, 0xae } };



HINSTANCE g_hInstance = NULL;

class CBiometricDriverModule :
    public CAtlDllModuleT< CBiometricDriverModule >
{
};

CBiometricDriverModule _AtlModule;

//
// DLL Entry Point
// 

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        WPP_INIT_TRACING(MYDRIVER_TRACING_ID);
        
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);

    } else if (dwReason == DLL_PROCESS_DETACH) {
        WPP_CLEANUP();
    }

    return _AtlModule.DllMain(dwReason, lpReserved);
}


//
// Returns a class factory to create an object of the requested type
// 

STDAPI
DllGetClassObject(
    _In_ REFCLSID rclsid,
    _In_ REFIID riid,
    _Outptr_ LPVOID FAR* ppv
    )
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


