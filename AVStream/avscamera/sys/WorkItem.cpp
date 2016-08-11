/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        workitem.cpp

    Abstract:

        This module contains an implementation for KWorkItem.

        A KWorkItem inherits from CRef and is reference counted.  The object 
        cannot be destroyed until all references are released.  This includes
        the reference held when the workitem is in flight.  This allows you 
        to store the workitem in an object that has a lifecycle shorter than
        the parent device object.

    History:

        created 5/30/2014

**************************************************************************/

#include "Common.h"

KWorkItem::
KWorkItem(
    _In_    PDEVICE_OBJECT          DeviceObj,
    _In_    PIO_WORKITEM_ROUTINE    Callback,
    _In_    PVOID                   Context,
    _In_    WORK_QUEUE_TYPE         QueueType
)
    : m_Callback( Callback )
    , m_Context ( Context  )
    , m_QueueType( QueueType )
/*++

Routine Description:

    Constructor.  
    
    Sets up callback handler for the workitem and performs initialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    m_WorkItem = IoAllocateWorkItem( DeviceObj );
}


KWorkItem::
~KWorkItem()
/*++

Routine Description:

    Destructor.  
    
    Blocks until all references are released, then frees the workitem.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //  Make sure the work item has been run down.
    Wait();

    //  Now delete it.
    if( m_WorkItem )
    {
        IoFreeWorkItem( m_WorkItem );
    }
}

BOOLEAN
KWorkItem::
Start()
/*++

Routine Description:

    Enqueue this workitem and acquires a reference on the object.

Arguments:

    None.

Return Value:

    TRUE - a workitem was scheduled.

--*/
{
    //  Make sure construction succeeded.
    if( m_WorkItem )
    {
        //  Make sure the work item isn't already in flight and acquire the lock.
        //  This is to make sure this KWorkItem doesn't get destroyed until
        //  the callback is complete.
        if( Acquire() )
        {
            //  Schedule the work item.
            IoQueueWorkItem( m_WorkItem, &KWorkItem::Handler, m_QueueType, this );

            //  Note: Release() is called in Handler.
            return TRUE;
        }
    }
    return FALSE;
}

VOID
KWorkItem::
Handler(
    _In_        PDEVICE_OBJECT  IoObject,
    _In_opt_    PVOID           Context
)
/*++

Routine Description:

    Callback thunking function.

    Calls the callback handler registered in the constructor, if one exists.
    Releases the reference on this object.

Arguments:

    IoObject - 
        The parent device object.
    Context -
        An optional context object.

Return Value:

    void

--*/
{
    //  Get our "this" pointer (Me).
    KWorkItem *Me = (KWorkItem *) Context;

    //  Invoke the callback if one exists.
    if( Me->m_Callback )
    {
        Me->m_Callback( IoObject, Me->m_Context );
    }

    //  Unlock the work item; allow for destruction or re-enqueing.
    Me->Release();
}
