/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Name:   Server.cpp
*
*  Description: Contains various COM server infrastructure implemented by
*               the Production Scanner Driver Sample: INonDelegating and
*               IUnknown implementations, IClassFactory interface declaration
*               and implementation, DLL entry points including DllMain.
*
***************************************************************************/

#include "stdafx.h"

extern HINSTANCE g_hInst;

//
// Production Scanner Driver Sample GUID
//
// {EB135F56-B088-4bc7-9733-422F324B3A09}
//
DEFINE_GUID(CLSID_ProdScan, 0xeb135f56, 0xb088, 0x4bc7, 0x97, 0x33, 0x42, 0x2f, 0x32, 0x4b, 0x3a, 0x9);

//
// CWiaDriver::INonDelegating interface implementation
//

HRESULT CWiaDriver::NonDelegatingQueryInterface(
    REFIID  riid,
    LPVOID* ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *ppvObj = NULL;

        //
        // The Production Scanner Driver Sample object exports the standard WIA mini-driver COM interfaces:
        //
        // - IUnknown
        // - IStiUSD
        // - IWiaMiniDrv
        //
        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = static_cast<INonDelegatingUnknown*>(this);
        }
        else if (IsEqualIID(riid, IID_IStiUSD))
        {
            *ppvObj = static_cast<IStiUSD*>(this);
        }
        else if (IsEqualIID(riid, IID_IWiaMiniDrv))
        {
            *ppvObj = static_cast<IWiaMiniDrv*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
            WIAEX_ERROR((g_hInst,
                "Unsupported interface (0x%08X, 0x%04X, 0x%04X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X), hr = 0x%08X",
                riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
                riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        reinterpret_cast<IUnknown*>(*ppvObj)->AddRef();
    }

    return hr;
}

ULONG CWiaDriver::NonDelegatingAddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CWiaDriver::NonDelegatingRelease()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (!ulRef)
    {
        WIAS_TRACE((g_hInst, "INonDelegating::NonDelegatingRelease, deleting main driver object (%p, process: %u)..",
            this, GetCurrentProcessId()));
        delete this;
    }

    return ulRef;
}

//
// CWiaDriver::IClassFactory interface declaration and implementation
//

class CWiaDriverClassFactory : public IClassFactory
{
public:
    CWiaDriverClassFactory()
        : m_cRef(1)
    {
        return;
    }

    ~CWiaDriverClassFactory()
    {
        return;
    }

    HRESULT __stdcall
    QueryInterface(
        REFIID  riid,
        _COM_Outptr_ LPVOID* ppv)
    {
        HRESULT hr = S_OK;

        if (!ppv)
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr))
        {
            *ppv = NULL;
            if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
            {
                *ppv = static_cast<IClassFactory*>(this);
                reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            }
            else
            {
                hr = E_NOINTERFACE;
                WIAEX_ERROR((g_hInst,
                    "Unsupported interface (0x%08X, 0x%04X, 0x%04X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X), hr = 0x%08X",
                    riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
                    riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], hr));

            }
        }

        return hr;
    }

    ULONG __stdcall
    AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG __stdcall
    Release()
    {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (!ulRef)
        {
            WIAS_TRACE((g_hInst, "IClassFactory::Release, deleting driver class factory object.."));
            delete this;
        }
        return ulRef;
    }

    HRESULT __stdcall
#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_2, "Set ppvObject to NULL if failed.")
    CreateInstance(
        _In_opt_     IUnknown* pUnkOuter,
        _In_         REFIID    riid,
        _COM_Outptr_ void**    ppvObject)
    {
        HRESULT hr = S_OK;
        CWiaDriver *pNewDriverObject = NULL;

        WIAEX_TRACE_BEGIN;

        if (!ppvObject)
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr))
        {
            *ppvObject = NULL;

            if ((pUnkOuter) && (!IsEqualIID(riid, IID_IUnknown)))
            {
                hr = CLASS_E_NOAGGREGATION;
                WIAEX_ERROR((g_hInst,
                    "NULL outer IUnknown* and not requesting IID_IUnknown, hr = 0x%08X (CLASS_E_NOAGGREGATION)", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            #pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "The main driver object instance is freed when the driver is unloaded")
            pNewDriverObject = new CWiaDriver(pUnkOuter);
            if (pNewDriverObject)
            {
                hr = pNewDriverObject->NonDelegatingQueryInterface(riid, ppvObject);
                pNewDriverObject->NonDelegatingRelease();

                if (SUCCEEDED(hr))
                {
                    WIAS_TRACE((g_hInst, "IClassFactory::CreateInstance, created main driver object (%p, process: %u)",
                        pNewDriverObject, GetCurrentProcessId()));
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAEX_ERROR((g_hInst, "Failed to allocate WIA driver class object, hr = 0x%08X", hr));
            }
        }

        WIAEX_TRACE_FUNC_HR;

        return hr;
    }

    HRESULT __stdcall
    LockServer(
        BOOL fLock)
    {
        UNREFERENCED_PARAMETER(fLock);
        return S_OK;
    }

private:
    LONG m_cRef;
};

//
// IUnknown implementation for CWiaDriver:
//

HRESULT CWiaDriver::QueryInterface(
    REFIID riid,
    _COM_Outptr_ LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *ppvObj = NULL;

        if (!m_punkOuter)
        {
            hr = E_NOINTERFACE;
            WIAEX_ERROR((g_hInst, "NULL outer IUnknown*, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_punkOuter->QueryInterface(riid, ppvObj);
    }

    return hr;
}

ULONG CWiaDriver::AddRef()
{
    ULONG ulRef = 0;

    if (!m_punkOuter)
    {
        WIAEX_ERROR((g_hInst, "NULL outer IUnknown*, returning 0"));
    }
    else
    {
        ulRef = m_punkOuter->AddRef();
    }

    return ulRef;
}

ULONG CWiaDriver::Release()
{
    ULONG ulRef = 0;

    if (!m_punkOuter)
    {
        WIAEX_ERROR((g_hInst, "NULL outer IUnknown*, returning 0"));
    }
    else
    {
        ulRef = m_punkOuter->Release();
    }

    return ulRef;
}

/**************************************************************************\
* DllMain
*
* Main DLL entry point
*
* Parameters:
*
*    hInst      - handle to the DLL module
*    dwReason   - indicates why the DLL entry-point function is being called
*    lpReserved - if fdwReason is DLL_PROCESS_ATTACH lpvReserved is NULL for
*                 dynamic loads and non-NULL for static loads; if fdwReason
*                 is DLL_PROCESS_DETACH, lpvReserved is NULL if DllMain has
*                 been called by using FreeLibrary and non-NULL if DllMain
*                 has been called during process termination.
*
* Return Value:
*
*    When the system calls the DllMain function with the DLL_PROCESS_ATTACH
*    value in this particular case the function returns TRUE every time
*    indicating it succeeds.
*
\**************************************************************************/

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(
    HINSTANCE         hInst,
    DWORD             dwReason,
    _Reserved_ LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    g_hInst = hInst;

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(g_hInst);
            WIAS_TRACE((g_hInst, "DLL_PROCESS_ATTACH (process: %u, thread: %u)",
                GetCurrentProcessId(), GetCurrentThreadId()));
            break;

        case DLL_PROCESS_DETACH:
            WIAS_TRACE((g_hInst, "DLL_PROCESS_DETACH (process: %u, thread: %u)",
                GetCurrentProcessId(), GetCurrentThreadId()));
            break;
    }

    return TRUE;
}

/**************************************************************************\
* DllCanUnloadNow
*
* Parameters: none
*
* Determines whether the DLL that implements this function is in use.
* If not, the caller can unload the DLL from memory.
*
* Return Value: S_OK (indicating the DLL can be unloaded at any time)
*
\**************************************************************************/

extern "C" HRESULT __stdcall DllCanUnloadNow(void)
{
    return S_OK;
}

/**************************************************************************\
* DllGetClassObject
*
* Retrieves the class object from a DLL object handler or object application.
* DllGetClassObject is called from within the CoGetClassObject function
* when the class context is a DLL.
*
* Parameters:
*
*    rclsid - CLSID that will associate the correct data and code
*    riid   - reference to the identifier of the interface that the caller
*             is to use to communicate with the class object. Usually, this
*             is IID_IClassFactory (the interface identifier for IClassFactory)
*    ppv    - [out] address of pointer variable that receives the interface
*             pointer requested in riid; upon successful return, *ppv contains
*             the requested interface pointer; if an error occurs, this is NULL.
*
* Return Values:
*
*   S_OK
*   E_INVALIDARG
*   CLASS_E_CLASSNOTAVAILABLE
*   E_OUTOFMEMORY
*
\**************************************************************************/

extern "C" HRESULT __stdcall DllGetClassObject(
    _In_        REFCLSID rclsid,
    _In_        REFIID   riid,
    _Outptr_ LPVOID* ppv)
{
    HRESULT hr = S_OK;
    CWiaDriverClassFactory *pNewClassFactory = NULL;

    WIAEX_TRACE_BEGIN;

    if (!ppv)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameters, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *ppv = NULL;

        if (IsEqualCLSID(rclsid, CLSID_ProdScan))
        {
            #pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "The final CWiaDriverClassFactory::Release frees the memory")
            pNewClassFactory = new CWiaDriverClassFactory;
            if (pNewClassFactory)
            {
                hr = pNewClassFactory->QueryInterface(riid, ppv);
                pNewClassFactory->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAEX_ERROR((g_hInst, "Failed to allocate WIA driver class factory object, hr = 0x%08X", hr));
            }
        }
        else
        {
            hr = CLASS_E_CLASSNOTAVAILABLE;
            WIAEX_ERROR((g_hInst, "Class not available, hr = 0x%08X (CLASS_E_CLASSNOTAVAILABLE)", hr));
        }
    }

    return hr;
}

/**************************************************************************\
* DllRegisterServer
*
* Instructs an in-process server to create its registry entries for all
* classes supported in this server module.
*
* Parameters: none
*
* Return Value: S_OK (registry entries - none - were created successfully)
*
\**************************************************************************/

extern "C" HRESULT __stdcall DllRegisterServer()
{
    return S_OK;
}

/**************************************************************************\
* DllUnregisterServer
*
* Instructs an in-process server to remove only those entries created
* through DllRegisterServer.
*
* Parameters: none
*
* Return Value: S_OK (registry entries - none - were removed successfully)
*
\**************************************************************************/

extern "C" HRESULT __stdcall DllUnregisterServer()
{
    return S_OK;
}
