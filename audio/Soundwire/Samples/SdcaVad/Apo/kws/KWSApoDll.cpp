//
// KWSApoDll.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Author:
//
// Description:
//
// KWSApoDll.cpp : Implementation of DLL Exports.

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlsync.h>
#include <mmreg.h>

#include "resource.h"
#include "KWSApoDll.h"
#include <KWSApo.h>

#include <KWSApoDll_i.c>


//-------------------------------------------------------------------------
// Array of APO_REG_PROPERTIES structures implemented in this module.
// Each new APO implementation will be added to this array.
//
APO_REG_PROPERTIES const *gCoreAPOs[] =
{
    &CKWSApoEFX::sm_RegProperties.m_Properties
};

// {secret}
class CKWSApoDllModule : public CAtlDllModuleT< CKWSApoDllModule >
{
public :
    DECLARE_LIBID(LIBID_KWSApoDlllib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_KWSAPODLL, "{0A21D954-674A-4C09-806E-DB4FBE8F199C}")

};

// {secret}
CKWSApoDllModule _AtlModule;


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
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}
