//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This source code is intended only as a supplement to Microsoft
//  Development Tools and/or on-line documentation.  See these other
//  materials for detailed information regarding Microsoft code samples.
//
//  THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
//  Abstract:
//     WDK print filter sample.
//     This file contains a smart pointer implementation.
//
//----------------------------------------------------------------------------

#ifndef __SAMPLE_SMART_PTR_HXX__
#define __SAMPLE_SMART_PTR_HXX__

namespace Tools
{

template<class T>
inline
void
SmartAddRef(
    T       *pInterface
    ) throw()
{
    if (pInterface)
    {
        pInterface->AddRef();
    }
}

template<class T>
inline
void
SmartRelease(
    T       **ppInterface
    ) throw()
{
    if (*ppInterface)
    {
        (*ppInterface)->Release();
        *ppInterface = NULL;
    }
}

template<class T>
class SmartPtr
{
public:

    inline
    SmartPtr(
        void
        ) throw() : m_Pointer(NULL)
    {
    }

    inline
    SmartPtr(
        _In_opt_  T   *pointer
        ) throw()
        : m_Pointer(pointer)
    {
        SmartAddRef(m_Pointer);
    }

    inline
    SmartPtr(
        _In_      const SmartPtr  &copy
        ) throw()
    {
        m_Pointer = copy.m_Pointer;
        SmartAddRef(m_Pointer);
    }

    inline
    ~SmartPtr(
        void
        ) throw()
    {
        SmartRelease(&m_Pointer);
    }

    inline
    SmartPtr &
    operator=(
        _In_opt_   T     *pointer
        ) throw()
    {
        T   *temp = m_Pointer;

        m_Pointer = pointer;
        SmartAddRef(m_Pointer);
        SmartRelease(&temp);

        return *this;
    }

    inline
    SmartPtr &
    operator=(
        _In_      const SmartPtr  &copy
        ) throw()
    {
        T   *temp = m_Pointer;

        m_Pointer = copy.m_Pointer;
        SmartAddRef(m_Pointer);
        SmartRelease(&temp);

        return *this;
    }

    inline
    operator T *() throw()
    {
        return m_Pointer;
    }

    inline
    operator const T *() const throw()
    {
        return m_Pointer;
    }

    inline
    T **
    operator&() throw()
    {
        return &m_Pointer;
    }

    inline
    const T **
    operator&() const throw()
    {
        return &m_Pointer;
    }

    inline
    T *
    operator->() throw()
    {
        return m_Pointer;
    }

    inline
    const T *
    operator->(
        ) const throw()
    {
        return m_Pointer;
    }

    inline
    void
    Attach(
        _In_opt_      T           *pointer
        ) throw()
    {
        T       *temp = m_Pointer;
        m_Pointer = pointer;

        SmartRelease(&temp);
    }

    inline
    T *
    Detach(
        void
        ) throw()
    {
        T   *pTemp = m_Pointer;
        m_Pointer = NULL;

        return pTemp;
    }

    inline
    void
    Clear(
        void
        ) throw()
    {
        SmartRelease(&m_Pointer);
    }

private:

    T   *m_Pointer;
};

///////////////////////////////////////////////////////////////////////////////

class SmartBSTR
{
public:

    SmartBSTR() throw()
    : m_bstr (NULL)
    {
    }

    ~SmartBSTR() throw()
    {
        ::SysFreeString(m_bstr);
    }

    operator BSTR() const throw()
    {
        return m_bstr;
    }

    BSTR*
    operator&() throw()
    {
        return &m_bstr;
    }

private:

    BSTR   m_bstr;
};

}; // namespace tools

#endif
