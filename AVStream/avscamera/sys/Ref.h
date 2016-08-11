/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ref.h

    Abstract:

        Implement a reference counting base class.  Also implement something
        similar to ComPtr<>, called a CRefPtr<>.  The implementation may not 
        be as robust as ComPtr<>, but it should do the job here.

    History:

        created 12/8/2014

**************************************************************************/

#pragma once

class CRef : public CNonCopyable
{
private:
    EX_RUNDOWN_REF  m_Rundown;

public:
    //  Initialize.
    CRef()
    {
        Reset();
    }

    //  Prevent deletion until the last reference is gone.
    ~CRef()
    {
        Wait();
    }

    //  Increment the reference.
    BOOLEAN
    Acquire()
    {
        return ExAcquireRundownProtection( &m_Rundown );
    }

    //  Decrement the reference.
    void
    Release()
    {
        return ExReleaseRundownProtection( &m_Rundown );
    }

    //  Wait for the last reference to go away.
    //  Note: This must be called by the child object's destructor.
    void
    Wait()
    {
        ExWaitForRundownProtectionRelease( &m_Rundown );
    }

    //  Reinitialize to reuse the associated object.
    void
    Reset()
    {
        ExInitializeRundownProtection( &m_Rundown );
    }
};

template<class T>
class CRefPtr
{
private:
    T   *m_ptr;

public:
    CRefPtr()
        : m_ptr(nullptr)
    {}

    //  Copy constructor.
    CRefPtr( const CRefPtr<T> &ptr )
        : m_ptr(nullptr)
    {
        //  Make an initial assignment.
        if( ptr && ptr->Acquire() )
        {
            m_ptr = ptr;
        }
    }

    //  Initial construction with a T object.
    CRefPtr( T *ptr )
        : m_ptr(nullptr)
    {
        //  Make an initial assignment.
        if( ptr && ptr->Acquire() )
        {
            m_ptr = ptr;
        }
    }

    //  Decrement the reference on destruction.
    ~CRefPtr()
    {
        //  Free any ptr.
        if( m_ptr )
        {
            m_ptr->Release();
        }
    }

    //  Assignment
    CRefPtr<T> &
    operator = ( T *ptr )
    {
        if( m_ptr )
        {
            m_ptr->Release();
        }

        if( ptr )
        {
            ptr->Acquire();
        }

        m_ptr = ptr;
        return *this;
    }

    //  Assignment
    CRefPtr<T> &
    operator = ( CRefPtr<T> &ptr )
    {
        return
            (*this = ptr.m_ptr);
    }

    //  Dereference the pointer.
    T *
    operator -> ()
    {
        return m_ptr;
    }

    //  Test for nullptr.
    operator bool()
    {
        return m_ptr != nullptr;
    }

    //  Test for !nullptr.
    bool
    operator !()
    {
        return !m_ptr;
    }

    //  Test for equality.
    bool
    operator ==( T *ptr )
    {
        return m_ptr == ptr;
    }

    //  Unsafe accessor.  Use sparingly.
    T *
    get()
    {
        return m_ptr;
    }
};

//  Test for equality.
template <class T>
inline
bool
operator ==( T *ptr1, CRefPtr<T> &ptr2 )
{
    return ptr2 == ptr1;
}

