/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        timer.h

    Abstract:

        Contains the definition of a KTimer and KPassiveTimer.

        KTimer wraps a kernel timer object.  KPassiveTimer uses a passive
        callback through a KWorkItem.

    History:

        created 5/30/2014

**************************************************************************/

#pragma once

typedef void (*PKTimerCallback)( PVOID Context ) ;

class KTimer : public KWaitable, public Cancelable
{
public:
    KTimer( TIMER_TYPE  type=NotificationTimer );

    virtual
    ~KTimer();

public:
    //
    //  Override function that is invoked when a timer expires
    //
    virtual
    void
    Func() ;

    //
    //  Cancels the timer
    //
    virtual
    BOOLEAN
    Cancel();

    //
    //  Returns TRUE is if the timer is signalled
    //
    BOOLEAN
    State()
    {
        return KeReadStateTimer( &m_Timer ) ;
    }

    //
    //  Resets (Cancels) the timer and DPC.
    //
    //  Reset() returns TRUE if the KTIMER or the KDPC are dequeued.
    //  If Reset() returns FALSE the DPC may still be executing on another
    //  processor.
    //
    BOOLEAN
    Reset()
    {
        //  Try cancelling the timer.
        if( !KeCancelTimer( &m_Timer ) )
        {
            //  If the timer couldn't be cancelled, try removing the DPC.
            return KeRemoveQueueDpc( &m_Dpc ) ;
        }
        return TRUE;
    }

    //
    //  Sets the timer
    //
    BOOLEAN
    Set(
        _In_        LARGE_INTEGER   DueTime,
        _In_opt_    PKTimerCallback Callback=NULL,
        _In_opt_    PVOID           Context=NULL,
        _In_        LONG            Period=0
    )
    {
        m_Callback = Callback ;
        m_Context = Context ;
        m_Period = Period ;
        return KeSetTimerEx( &m_Timer, DueTime, Period, &m_Dpc ) ;
    }

    //
    //  Sets the timer
    //
    BOOLEAN
    Set(
        _In_        LONGLONG        DueTime,
        _In_opt_    PKTimerCallback Callback=NULL,
        _In_opt_    PVOID           Context=NULL,
        _In_        LONG            Period=0
    )
    {
        m_Callback = Callback ;
        m_Context = Context ;
        m_Period = Period ;
        LARGE_INTEGER   Due;
        Due.QuadPart = DueTime;
        return KeSetTimerEx( &m_Timer, Due, Period, &m_Dpc ) ;
    }

    //
    //  Support for waitable class
    //
    virtual
    PVOID
    GetDispatchObject();

private:
    //
    //  Dpc Thunking function for KeSetTimerEx
    //
    _Function_class_(KDEFERRED_ROUTINE)
    static
    VOID
    DpcThunk(
        _In_        PKDPC   Dpc,
        _Inout_opt_ PVOID   DeferredContext,
        _Inout_opt_ PVOID   Arg1,
        _Inout_opt_ PVOID   Arg2
    ) ;

    TIMER_TYPE  GetType()
    {
        return m_type;
    }

private:
    KTIMER          m_Timer ;
    TIMER_TYPE      m_type;

    KDPC            m_Dpc ;
    LONG            m_Period ;

protected:
    PKTimerCallback m_Callback ;
    PVOID           m_Context ;
};

class KPassiveTimer : public KTimer
{
public:
    KPassiveTimer(
        _In_    PDEVICE_OBJECT          DeviceObj,
        _In_    TIMER_TYPE              type=NotificationTimer
    )
        : KTimer( type )
        , m_WorkItem( DeviceObj, WorkThunk, this )
    {}

    virtual
    ~KPassiveTimer();

    //
    //  Override function that is invoked when a timer expires
    //
    virtual
    void
    Func() ;

    //
    //  Cancels the timer
    //
    virtual
    BOOLEAN
    Cancel();

    //
    //  Make sure it's in a valid state.
    //
    virtual
    BOOLEAN
    IsValid()
    {
        return m_WorkItem.IsValid();
    }
private:
    KWorkItem   m_WorkItem;

    //
    //  Dpc Thunking function for KeSetTimerEx
    //
    static
    IO_WORKITEM_ROUTINE
    WorkThunk;
};