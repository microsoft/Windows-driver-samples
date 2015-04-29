/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   clasfact.h

Abstract:

   This file defines a template IClassFactory implementation
   to be returned by each filter's DllGetClassObject() function.

   Also, a routine to return one of these class factories so that
   this code does not need to be duplicated in every filter's
   DllGetClassObject().

--*/

#pragma once

#include "cunknown.h"
#include "globals.h"

template <class _T>
class CClassFactory : public CUnknown<IClassFactory>
{
public:
    //
    // Constructor and Destruction
    //
    CClassFactory() :
        CUnknown<IClassFactory>(IID_IClassFactory)
    { }

    virtual ~CClassFactory()
    { }

    //
    // IClassFactory methods
    //
    virtual HRESULT STDMETHODCALLTYPE
    CreateInstance(
        _In_opt_    LPUNKNOWN pUnkOuter,
        _In_        REFIID    riid,
        _Outptr_ PVOID*    ppvObject
        )
    {
        HRESULT hr = S_OK;

        if (ppvObject == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }
        *ppvObject = NULL;

        if (pUnkOuter == NULL)
        {
            //
            // Create Filter
            //
            _T* pFilter = NULL;

            try
            {
                pFilter = new(std::nothrow) _T;
                hr = CHECK_POINTER(pFilter, E_OUTOFMEMORY);
            }
            catch (CXDException& e)
            {
                if (pFilter != NULL)
                {
                    delete pFilter;
                    pFilter = NULL;
                }

                hr = e;
            }
            catch(...)
            {
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Get the requested interface
                //
                hr = pFilter->QueryInterface(riid, ppvObject) ;

                //
                // Release the IUnknown pointer. If QueryInterface failed
                // the Release() call will clean up
                //
                pFilter->Release();
            }
        }
        else
        {
            //
            // Cannot aggregate
            //
            hr = CLASS_E_NOAGGREGATION;
        }

    Exit:
        ERR_ON_HR(hr);
        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE
    LockServer(
        _In_ BOOL bLock
        )
    {
        if (bLock)
        {
            InterlockedIncrement(&g_cServerLocks);
        }
        else
        {
            InterlockedDecrement(&g_cServerLocks);
        }

        return S_OK;
    }
};

template <class _T>
HRESULT
GetFilterClassFactory(
    _In_  REFCLSID rclsid,
    _In_  REFIID   riid,
    _In_  REFCLSID expectedclsid,
    _Out_ VOID**   ppv
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppv, E_POINTER)))
    {
        *ppv = NULL;

        //
        // Make sure the appropriate class factory is being requested
        //
        if (rclsid == expectedclsid)
        {
            CClassFactory<_T>* pFactory = NULL;

            try
            {
                pFactory = new(std::nothrow) CClassFactory<_T>();
                hr = CHECK_POINTER(pFactory, E_OUTOFMEMORY);
            }
            catch (CXDException& e)
            {
                if (pFactory != NULL)
                {
                    delete pFactory;
                    pFactory = NULL;
                }

                hr = e;
            }
            catch (...)
            {
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Get the requested interface.
                //
                hr = pFactory->QueryInterface(riid, ppv);

                //
                // Release the IUnknown pointer.
                // (If QueryInterface failed, component will delete itself.)
                //
                pFactory->Release();
            }
        }
        else
        {
            hr =  CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

