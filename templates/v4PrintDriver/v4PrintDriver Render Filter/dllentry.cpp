//
// File Name:
//
//    dllentry.cpp
//
// Abstract:
//
//    XPS Rendering Filter DLL entry points.
//

#include "precomp.h"
#include "WppTrace.h"
#include "CustomWppCommands.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "RenderFilter.h"

#include "dllentry.tmh"

namespace v4PrintDriver_Render_Filter
{

//
// Class Factory returned by DllGetClassObject()
//
class __declspec(uuid("ac9d373b-610e-4490-8129-aff0ff4e0e53")) RenderFilterFactory : public UnknownBase<IClassFactory>
{
public:
    RenderFilterFactory() :
        m_serverLocks(0)
    {
        ::InterlockedIncrement(&RenderFilter::ms_numObjects);
    }

    ~RenderFilterFactory()
    {
        ::InterlockedDecrement(&RenderFilter::ms_numObjects);
    }

    //
    //Routine Name:
    //
    //    RenderFilterFactory::CreateInstance
    //
    //Routine Description:
    //
    //    Returns an instance of RenderFilter.
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

        v4PrintDriver_Render_Filter::RenderFilter *pFilter = NULL;

        //
        // RenderFilter::RenderFilter() can throw, as can new
        //
        try
        {
            DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Instantiating filter");
            pFilter = new v4PrintDriver_Render_Filter::RenderFilter();
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
    //    RenderFilterFactory::LockServer
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
                ::InterlockedIncrement(&RenderFilter::ms_numObjects);
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
                ::InterlockedDecrement(&RenderFilter::ms_numObjects);
            }
        }

        return S_OK;
    }

private:
    volatile LONG m_serverLocks;
};

} // namespace v4PrintDriver_Render_Filter 

//
//Routine Name:
//
//    DllGetClassObject
//
//Routine Description:
//
//    Returns an instance of RenderFilterFactory.
//
//Arguments:
//
//    rclsid  - Requested class (RenderFilter)
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

    if (rclsid != __uuidof(v4PrintDriver_Render_Filter::RenderFilterFactory))
    {
        WPP_LOG_ON_FAILED_HRESULT(CLASS_E_CLASSNOTAVAILABLE);

        return CLASS_E_CLASSNOTAVAILABLE;
    }

    DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Instantiating class factory");

    v4PrintDriver_Render_Filter::RenderFilterFactory *pFactory = NULL;
    pFactory = new(std::nothrow) v4PrintDriver_Render_Filter::RenderFilterFactory();

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
//    whether there are any instances of RenderFilter.
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
    return (0 == v4PrintDriver_Render_Filter::RenderFilter::ms_numObjects) ? S_OK : S_FALSE;
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

        WPP_INIT_TRACING(L"v4PrintDriverRenderFilter");
        DoTraceMessage(RENDERFILTER_TRACE_INFO, L"DLL_PROCESS_ATTACH");
        break;

    case DLL_PROCESS_DETACH:

        DoTraceMessage(RENDERFILTER_TRACE_INFO, L"DLL_PROCESS_DETACH");
        WPP_CLEANUP();

        break;
    }

    return TRUE;
}
