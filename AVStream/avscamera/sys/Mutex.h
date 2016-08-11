/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        Mutex.h

    Abstract:

        This file provides the declaration of KMutex and KScopedMutex.
        
        KMutex provides a wrapper around a KMUTEX object.  KScopedMutex is
        provided as a helper class to ensure all exits release the lock.

    History:

        created 7/16/2013

**************************************************************************/
#pragma once

//
//  Specialized class for KMUTEX
//
class KMutex : CNonCopyable
{
    KMUTEX  m_Lock;

public:
    KMutex()
    {
        KeInitializeMutex( &m_Lock, 0 );
    }

    virtual ~KMutex();

    //
    //  Preferred lock method.
    //
    _IRQL_requires_min_(PASSIVE_LEVEL)
    _When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
    _When_((Timeout!=NULL &&Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
    NTSTATUS
    Lock(
        _In_opt_ PLARGE_INTEGER Timeout=nullptr
    );

    //
    //  Preferred unlock method.
    //
    _IRQL_requires_max_(DISPATCH_LEVEL)
    void
    Unlock();
};

//
//  Use this class to hold a spinlock through a particular scope
//
class KScopedMutex : CNonCopyable
{
    KMutex  &m_Lock;

public:
    _IRQL_requires_min_(PASSIVE_LEVEL)
    _IRQL_requires_max_(APC_LEVEL)
    KScopedMutex( KMutex &lock )
        : m_Lock(lock)
    {
        m_Lock.Lock();
    }

    _IRQL_requires_max_(DISPATCH_LEVEL)
    ~KScopedMutex(void)
    {
        m_Lock.Unlock();
    }
};

