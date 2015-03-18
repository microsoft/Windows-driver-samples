// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    dllentry.cpp
//
// Abstract:
//
//    Xps Rasterization Service filter DLL entry points.
//

#include "precomp.h"
#include "WppTrace.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "xpsrasfilter.h"

#include "dllentry.tmh"

namespace xpsrasfilter
{

//
// Class Factory returned by DllGetClassObject()
//
class __declspec( uuid("CFF7BE69-E62D-403b-BE4F-48EC73F5DA1A") ) XPSRasFilterFactory : public UnknownBase<IClassFactory>
{
public:
    XPSRasFilterFactory() :
        m_serverLocks(0)
    {
        ::InterlockedIncrement(&XPSRasFilter::ms_numObjects);
    }

    ~XPSRasFilterFactory()
    {
        ::InterlockedDecrement(&XPSRasFilter::ms_numObjects);
    }

    //
    //Routine Name:
    //
    //    XPSRasFilterFactory::CreateInstance
    //
    //Routine Description:
    //
    //    Returns an instance of XPSRasFilter.
    //
    //Arguments:
    //
    //    pUnkOuter - Outer class (must be NULL)
    //    riid      - Requested interface (IPrintPipelineFilter)
    //    ppvObject - Pointer to the requested interface
    //
    //Return Value:
    //
    //    HRESULT
    //    S_OK      - On success
    //    Otherwise - Failure
    //
    HRESULT
    STDMETHODCALLTYPE
    CreateInstance(
        IUnknown    *pUnkOuter,
        REFIID      riid,
        void        **ppvObject
        )
    {
        HRESULT hr = S_OK;

        if (pUnkOuter != NULL)
        {
            WPP_LOG_ON_FAILED_HRESULT(CLASS_E_NOAGGREGATION);

            return CLASS_E_NOAGGREGATION;
        }

        if (ppvObject == NULL)
        {
            WPP_LOG_ON_FAILED_HRESULT(E_POINTER);

            return E_POINTER;
        }

        *ppvObject = NULL;

        xpsrasfilter::XPSRasFilter *pFilter = NULL;

        //
        // XpsRasFilter::XpsRasFilter() can throw, as can new
        //
        try
        {
            DoTraceMessage(XPSRASFILTER_TRACE_INFO, L"Instantiating filter");
            pFilter = new xpsrasfilter::XPSRasFilter();
        }
        CATCH_VARIOUS(hr)

        if (SUCCEEDED(hr))
        {
            WPP_LOG_ON_FAILED_HRESULT(
                hr = pFilter->QueryInterface(riid, ppvObject)
                );

            pFilter->Release();
        }

        return hr;
    }

    //
    //Routine Name:
    //
    //    XPSRasFilterFactory::LockServer
    //
    //Routine Description:
    //
    //    Allows clients to lock the filter factory in
    //    memory.
    //
    //Arguments:
    //
    //    fLock - TRUE - lock; FALSE - unlock
    //
    //Return Value:
    //
    //    HRESULT
    //    S_OK      - On success
    //
    HRESULT
    STDMETHODCALLTYPE
    LockServer(
        BOOL    fLock
        )
    {
        LONG result;

        if (fLock)  // lock
        {
            result = ::InterlockedIncrement(&m_serverLocks);

            if (result == 1)
            {
                //
                // This was the first 'lock' call; increment the
                // global numObjects
                //
                ::InterlockedIncrement(&XPSRasFilter::ms_numObjects);
            }
        }
        else // unlock
        {
            result = ::InterlockedDecrement(&m_serverLocks);

            if (result == 0)
            {
                //
                // All locks have been unlocked; decrement the
                // global numObjects
                //
                ::InterlockedDecrement(&XPSRasFilter::ms_numObjects);
            }
        }

        return S_OK;
    }

private:
    volatile LONG m_serverLocks;
};

} // namespace xpsrasfilter 

//
//Routine Name:
//
//    DllGetClassObject
//
//Routine Description:
//
//    Returns an instance of XPSRasFilterFactory.
//
//Arguments:
//
//    rclsid  - Requested class (XPSRasFilter)
//    riid    - Requested interface (IClassFactory)
//    ppv     - Pointer to the requested interface
//
//Return Value:
//
//    HRESULT
//    S_OK      - On success
//    Otherwise - Failure
//
STDAPI
DllGetClassObject(
    _In_ REFCLSID        rclsid,
    _In_ REFIID          riid,
    _Outptr_ LPVOID   *ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL)
    {
        WPP_LOG_ON_FAILED_HRESULT(E_POINTER);

        return E_POINTER;
    }

    *ppv = NULL;

    if (rclsid != __uuidof(xpsrasfilter::XPSRasFilterFactory))
    {
        WPP_LOG_ON_FAILED_HRESULT(CLASS_E_CLASSNOTAVAILABLE);

        return CLASS_E_CLASSNOTAVAILABLE;
    }

    DoTraceMessage(XPSRASFILTER_TRACE_INFO, L"Instantiating class factory");

    xpsrasfilter::XPSRasFilterFactory *pFactory = NULL;
    pFactory = new(std::nothrow) xpsrasfilter::XPSRasFilterFactory();

    if (pFactory == NULL)
    {
        WPP_LOG_ON_FAILED_HRESULT(E_OUTOFMEMORY);

        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        WPP_LOG_ON_FAILED_HRESULT(
            hr = pFactory->QueryInterface(riid, ppv)
            );

        pFactory->Release();
    }

    return hr;
}

//
//Routine Name:
//
//    DllCanUnloadNow
//
//Routine Description:
//
//    Checks whether the DLL can be unloaded. That is,
//    whether there are any instances of XPSRasFilter.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    HRESULT
//    S_OK      - Can unload.
//    Otherwise - Cannot unload.
//
STDAPI
DllCanUnloadNow()
{
    return (0 == xpsrasfilter::XPSRasFilter::ms_numObjects) ? S_OK : S_FALSE;
}

//
//Routine Name:
//
//    DllMain
//
//Routine Description:
//
//    Initializes WPP tracing when the DLL is loaded
//    and cleans up WPP tracing when the DLL is unloaded.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    HRESULT
//    S_OK      - On success.
//    Otherwise - Otherwise.
//
extern "C"
BOOL
WINAPI
DllMain(
    _In_     HINSTANCE   hinstDLL,
    _In_     DWORD       fdwReason,
    _In_opt_ LPVOID      /*lpvReserved*/
    )
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        ::DisableThreadLibraryCalls(hinstDLL);

        WPP_INIT_TRACING(L"XpsRasFilter");
        DoTraceMessage(XPSRASFILTER_TRACE_INFO, L"DLL_PROCESS_ATTACH");

        break;

    case DLL_PROCESS_DETACH:

        DoTraceMessage(XPSRASFILTER_TRACE_INFO, L"DLL_PROCESS_DETACH");
        WPP_CLEANUP();

        break;
    }

    return TRUE;
}

