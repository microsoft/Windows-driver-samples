/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        dllmain.cpp

    Abstract:

        DLL and COM initialization.

    History:

        created 5/15/2014

**************************************************************************/
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved


#include "stdafx.h"
#include "Mft0.h"
#include "Mft0clsid.h"
#include "Mft0Impl.h"
#include <wrl\module.h>

using namespace Microsoft::WRL;

// Module Ref count
volatile ULONG  g_cRefModule = 0;

// Handle to the DLL's module
HMODULE g_hModule = NULL;

ULONG DllAddRef()
{
    return InterlockedIncrement(&g_cRefModule);
}

ULONG DllRelease()
{
    return InterlockedDecrement(&g_cRefModule);
}

class CClassFactory : public IClassFactory
{
public:
    static HRESULT CreateInstance(
        REFCLSID clsid,
        REFIID   riid,
        _COM_Outptr_ void **ppv
    )
    {
        *ppv = NULL;

        HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

        if(IsEqualGUID(clsid, CLSID_AvsCamMft0))
        {
            IClassFactory *pClassFactory = new (std::nothrow) CClassFactory();
            if(!pClassFactory)
            {
                return E_OUTOFMEMORY;
            }
            hr = pClassFactory->QueryInterface(riid, ppv);
            pClassFactory->Release();
        }
        return hr;
    }

    //IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
    {
        HRESULT hr = S_OK;

        if(ppvObject == NULL)
        {
            return E_POINTER;
        }

        if(riid == IID_IUnknown)
        {
            *ppvObject = (IUnknown*)this;
        }
        else if(riid == IID_IClassFactory)
        {
            *ppvObject = (IClassFactory*)this;
        }
        else 
        {
            *ppvObject = NULL;
            return E_NOINTERFACE;
        }

        AddRef();

        return hr;
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&m_cRef);
        if(cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    //IClassFactory Methods
    IFACEMETHODIMP CreateInstance(_In_ IUnknown *punkOuter, _In_ REFIID riid, _Outptr_ void **ppv)
    {
        return punkOuter ? CLASS_E_NOAGGREGATION : MFT0CreateInstance(riid, ppv);
    }

    IFACEMETHODIMP LockServer(BOOL fLock)
    {
        if(fLock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

private:
    CClassFactory() :
        m_cRef(1)
    {
        DllAddRef();
    }

    ~CClassFactory()
    {
        DllRelease();
    }

    long m_cRef;
};




STDAPI_(BOOL) DllMain(_In_opt_ HINSTANCE hinst, DWORD reason, _In_opt_ void *)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hModule = (HMODULE)hinst;
        DisableThreadLibraryCalls(hinst);
    }
    return TRUE;
}

//
// DllCanUnloadNow
//
/////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow()
{
    return (g_cRefModule == 0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////
//
// DllGetClassObject
//
/////////////////////////////////////////////////////////////////////////
_Check_return_
STDAPI  DllGetClassObject(
    _In_        REFCLSID rclsid,
    _In_        REFIID riid,
    _Outptr_    LPVOID FAR *ppv
)
{
    return CClassFactory::CreateInstance(rclsid, riid, ppv);
}
