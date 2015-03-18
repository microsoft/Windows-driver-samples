/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    dllsup.cpp

Abstract:

    This module contains the implementation of the UMDF Socktecho Sample 
    Driver's entry point and its exported functions for providing COM support.

    This module can be copied without modification to a new UMDF driver.  It
    depends on some of the code in comsup.cpp & comsup.h to handle DLL 
    registration and creating the first class factory.

    This module is dependent on the following defines:

        MYDRIVER_TRACING_ID - A wide string passed to WPP when initializing 
                              tracing.

        MYDRIVER_CLASS_ID -   A GUID encoded in struct format used to 
                              initialize the driver's ClassID.     

    These are defined in internal.h for the sample.  If you choose
    to use a different primary include file, you should ensure they are 
    defined there as well.

Environment:

    WDF User-Mode Driver Framework (WDF:UMDF)

--*/

#include "internal.h"
#include "dllsup.tmh"

const GUID CLSID_MyDriverCoClass = MYDRIVER_CLASS_ID;

class CNetNfpProviderModule : public CAtlDllModuleT< CNetNfpProviderModule >
{
};


OBJECT_ENTRY_AUTO(CLSID_MyDriverCoClass, CMyDriver)


CNetNfpProviderModule _AtlModule;

BOOL
WINAPI
DllMain(
    HINSTANCE ModuleHandle,
    DWORD Reason,
    PVOID Reserved
   )
/*++

  Routine Description:

    This is the entry point and exit point for the I/O trace driver.  This 
    does very little as the I/O trace driver has minimal global data.

    This method initializes tracing.

  Arguments:

    ModuleHandle - the DLL handle for this module.

    Reason - the reason this entry point was called.

    Reserved - unused

  Return Value:

    TRUE

--*/
{

    UNREFERENCED_PARAMETER( ModuleHandle );

    if (DLL_PROCESS_ATTACH == Reason)
    {
        //
        // Initialize tracing.
        //

        WPP_INIT_TRACING(MYDRIVER_TRACING_ID);
        TracingTlsInitialize();

    }
    else if (DLL_PROCESS_DETACH == Reason)
    {
        //
        // Cleanup tracing.
        //

        TracingTlsFree();
        WPP_CLEANUP();
    }

    return _AtlModule.DllMain(Reason, Reserved); 
;
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}
