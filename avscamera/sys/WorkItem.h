/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        workitem.h

    Abstract:

        This module contains a class wrapper definition for an IO_WORKITEM.

        A KWorkItem inherits from CRef and is reference counted.  The object 
        cannot be destroyed until all references are released.  This includes
        the reference held when the workitem is in flight.  This allows you 
        to store the workitem in an object that has a lifecycle shorter than
        the parent device object.

    History:

        created 5/30/2014

**************************************************************************/

#pragma once
class KWorkItem :
    public CRef     // Reference counted...
{
private:
    PIO_WORKITEM    m_WorkItem;
    PIO_WORKITEM_ROUTINE m_Callback;
    PVOID           m_Context;
    WORK_QUEUE_TYPE m_QueueType;

public:
    KWorkItem(
        _In_    PDEVICE_OBJECT          DeviceObj,
        _In_    PIO_WORKITEM_ROUTINE    Callback,
        _In_    PVOID                   Context=nullptr,
        _In_    WORK_QUEUE_TYPE         QueueType=NormalWorkQueue
    );

    virtual
    ~KWorkItem();

    BOOLEAN
    IsValid()
    {
        return m_WorkItem != nullptr;
    }

    //  Returns FALSE if failed to start.
    BOOLEAN
    Start();

private:
    static
    IO_WORKITEM_ROUTINE
    Handler;
};

