/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        Mutex.cpp

    Abstract:

        This file provides the implementation of KMutex and KScopedMutex.
        
        KMutex provides a wrapper around a KMUTEX object.  KScopedMutex is
        provided as a helper class to ensure all exits release the lock.

    History:

        created 7/16/2013

**************************************************************************/

#include "Common.h"

//
//  Dummy dtor.  Required because the dtor is virtual.
//
KMutex::~KMutex()
{}

//
//  Preferred lock method.
//
_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL &&Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTSTATUS
KMutex::Lock(
    _In_opt_ PLARGE_INTEGER Timeout
)
{
    //DBG_ENTER(": Thread Owner = %p", m_Lock.OwnerThread);

    NTSTATUS status =
        KeWaitForSingleObject(
            &m_Lock,
            Executive,
            KernelMode,
            FALSE,
            Timeout );

    //DBG_LEAVE(": Status = %d Thread Owner = %p", status, m_Lock.OwnerThread);

    return status;
}

//
//  Preferred unlock method.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
void
KMutex::Unlock()
{
    //DBG_ENTER(": Thread Owner = %p", m_Lock.OwnerThread);

    KeReleaseMutex( &m_Lock, FALSE );

    //DBG_LEAVE(": Thread Owner = %p", m_Lock.OwnerThread);
}
