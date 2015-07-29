/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        timer.cpp

    Abstract:

        Contains the implementation of a KTimer and KPassiveTimer.

        KTimer wraps a kernel timer object.  KPassiveTimer uses a passive
        callback through a KWorkItem.

    History:

        created 5/30/2014

**************************************************************************/

#include "Common.h"

KTimer::
KTimer( TIMER_TYPE  type )
    : m_Period(0)
    , m_Context(NULL)
    , m_Callback(NULL)
{
    KeInitializeTimerEx( &m_Timer,type ) ;
    KeInitializeDpc( &m_Dpc, KTimer::DpcThunk, this ) ;
}

KTimer::
~KTimer()
{
    Cancel();
}

//
//  Cancels the timer and ensures no DPC is executing.
//
BOOLEAN
KTimer::
Cancel()
{
    //  Clear the timer and dequeue the DPC.
    if( !Reset() )
    {
        //  If the item wasn't found, flush all the DPC queues.
        KeFlushQueuedDpcs() ;

        //  Return false because we lost the race.
        return FALSE;
    }
    return TRUE;
}

//
//  Support for waitable class
//
PVOID
KTimer::
GetDispatchObject()
{
    return &m_Timer;
}

//
//  Dpc Thunking function for KeSetTimerEx
//
_Function_class_(KDEFERRED_ROUTINE)
VOID
KTimer::
DpcThunk(
    _In_        PKDPC   /*Dpc*/,
    _Inout_opt_ PVOID   DeferredContext,
    _Inout_opt_ PVOID   /*Arg1*/,
    _Inout_opt_ PVOID   /*Arg2*/
)
{
    KTimer *pKTimer = (KTimer *) DeferredContext ;
    pKTimer->Func() ;
}

//
//  Function invoked when a timer expires
//
//  IRQL <= DISPATCH_LEVEL
//
void
KTimer::
Func()
{
    if( m_Callback )
    {
        m_Callback( m_Context ) ;
    }
}

//
//  Dtor
//
//  Calls Cancel() here to clean up.
//
//  Note: KTimer will also call Cancel(), but it will invoke KTimer::Cancel().
//        Since KPassiveTimer::Cancel() first calls KTimer::Cancel() this second
//        call is redudant; but preventing it would probably require an explicit
//        call to Cancel() before destruction.  This way you can just delete
//        timers when they are no longer needed, but it make take a bit longer.
//
KPassiveTimer::
~KPassiveTimer()
{
    Cancel();
}

//
//  Function invoked when a timer expires
//
//  IRQL == DISPATCH_LEVEL
//
void
KPassiveTimer::
Func()
{
    if( m_Callback )
    {
        //  Start the work item.  If it fails, there's not much we can do since
        //  it can only fail if the owner of this timer is trying to cancel it.
        m_WorkItem.Start();
    }
}

VOID
KPassiveTimer::
WorkThunk(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if( Context )
    {
        ((KPassiveTimer *) Context)->KTimer::Func();
    }
}

//
//  Cancels the timer
//
BOOLEAN
KPassiveTimer::
Cancel()
{
    //  Kill the timer.
    if( !KTimer::Cancel() )
    {
        //  Wait until the work item is idle.
        m_WorkItem.Wait();

        //  Reset the WorkItem so we can use it again.
        m_WorkItem.Reset();

        return FALSE;
    }
    return TRUE;
}

