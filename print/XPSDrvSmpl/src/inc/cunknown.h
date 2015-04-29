/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cunknown.h

Abstract:

   Provides a simple template implementation for classes that need to
   implement IUnknown.

--*/

#pragma once

template <class _T>
class CUnknown : public _T
{
public:
    /*++

    Routine Name:

        CUnknown

    Routine Description:

        CUnknown class constructor

    Arguments:

        IIDTarget - The target interface guid

    Return Value:

        None

    --*/
    CUnknown(
        REFIID IIDTarget
        ) :
        m_IIDTarget(IIDTarget),
        m_cRef(1)
    {
    }

    /*++

    Routine Name:

        ~CUnknown

    Routine Description:

        CUnknown class destructor

    Arguments:

        None

    Return Value:

        None

    --*/
    virtual ~CUnknown()
    {
    }

    //
    // IUnknown methods
    //
    /*++

    Routine Name:

        QueryInterface

    Routine Description:

        This routine implements the IUnknown::QueryInterface method. Returns the raw interface
        pointer for the requested IID if it matches the target IID

    Arguments:

        riid - The IID of the requested interface
        ppv  - The raw interface pointer

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT STDMETHODCALLTYPE
    QueryInterface(
              REFIID riid,
        _Out_ PVOID* ppv
        )
    {
        HRESULT hr = S_OK;

        if (ppv != NULL)
        {
            if (riid == IID_IUnknown ||
                riid == m_IIDTarget)
            {
                *ppv = static_cast<_T*>(this);
            }
            else
            {
                *ppv = NULL;

                hr = E_NOINTERFACE;
            }
        }
        else
        {
            hr = E_POINTER;
        }

        if (SUCCEEDED(hr))
        {
            AddRef();
        }

        return hr;
    }

    /*++

    Routine Name:

        AddRef

    Routine Description:

        This routine increments the reference count on the current interface

    Arguments:

        None

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual ULONG STDMETHODCALLTYPE
    AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    /*++

    Routine Name:

        Release

    Routine Description:

        This routine decrements the reference count on the current interface

    Arguments:

        None

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    Notes:

        The drv_at annotation tells Prefast to consider this object's memory
        freed after Release has been called.

    --*/
    virtual
    ULONG STDMETHODCALLTYPE
    Release()
    {
        ULONG cRef = InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

private:
    LONG m_cRef;

    IID  m_IIDTarget;
};

