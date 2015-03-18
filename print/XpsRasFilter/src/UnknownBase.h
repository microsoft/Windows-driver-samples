// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    UnknownBase.h
//
// Abstract:
//
//    IUnknown implementation common to filter components derived from 
//    IUnknown.
//

#pragma once

namespace xpsrasfilter
{

template <class Interface>
class UnknownBase : public Interface
{
public:
    UnknownBase() : m_cRef(1) { }
    virtual ~UnknownBase() { };

    //
    //Routine Name:
    //
    //    UnknownBase::QueryInterface
    //
    //Routine Description:
    //
    //    Implements IUnknown QueryInterface.
    //
    //Arguments:
    //
    //    riid - id of the interface
    //    ppv - void pointer to the requested interface
    //
    //Return Value:
    //
    //    HRESULT
    //    S_OK          - On success
    //    E_NOINTERFACE - Invalid interface
    //
    _Must_inspect_result_
    HRESULT STDMETHODCALLTYPE
    QueryInterface(
        _In_ REFIID            riid,
        _Outptr_ PVOID      *ppv
        )
    {
        HRESULT hr = S_OK;

        if (ppv == NULL)
        {
            WPP_LOG_ON_FAILED_HRESULT(E_POINTER);

            return E_POINTER;
        }

        if (riid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown *>(this);
        }
        else if (riid == __uuidof(Interface))
        {
            *ppv = static_cast<Interface *>(this);
        }
        else
        {
            *ppv = NULL;
            WPP_LOG_ON_FAILED_HRESULT(
                hr = E_NOINTERFACE
                );
        }

        if (SUCCEEDED(hr))
        {
            AddRef();
        }

        return hr;
    }

    //
    //Routine Name:
    //
    //    UnknownBase::AddRef
    //
    //Routine Description:
    //
    //    Implements IUnknown reference count increment
    //    on the current interface.
    //
    //Arguments:
    //
    //    None
    //
    //Return Value:
    //
    //    ULONG
    //    New reference count
    //
    ULONG STDMETHODCALLTYPE
    AddRef()
    {
        return ::InterlockedIncrement(&m_cRef);
    }

    //
    //Routine Name:
    //
    //    UnknownBase::Release
    //
    //Routine Description:
    //
    //    Implements IUnknown reference count decrement
    //    on the current interface.
    //
    //Arguments:
    //
    //    None
    //
    //Return Value:
    //
    //    ULONG
    //    New reference count
    //
    //Note:
    //
    //    The drv_at annotation tells Prefast to consider this object's memory
    //    freed after Release has been called.
    //
    _At_(this, __drv_freesMem(object))
    ULONG STDMETHODCALLTYPE
    Release()
    {
        ULONG cRef = ::InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

private:
    volatile ULONG m_cRef; // interface reference count
};

} // namespace xpsrasfilter

