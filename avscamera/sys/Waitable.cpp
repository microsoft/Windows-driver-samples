/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        waitable.cpp

    Abstract:

        Base interface for any object that is waitable.
        In otherwords, it contains a kernel dispatch object.

    History:

        created 5/30/2014

**************************************************************************/

#include "Common.h"

KWaitable::
KWaitable()
{}


KWaitable::
~KWaitable()
{}

NTSTATUS
KWaitable::
Wait(
    _In_opt_    PLARGE_INTEGER Timeout
)
/*++

Routine Description:

    Default Wait implementation.

Arguments:

    Timeout - 
        An optional timeout value in 100ns units.  Negative is relative time.  
        If NULL, wait forever.

Return Value:

    Success / Failure.

--*/
{
    PVOID   pObj = GetDispatchObject() ;

    if(pObj != NULL)
    {
        return
            KeWaitForSingleObject(
                pObj,
                Executive,
                KernelMode,
                FALSE,
                Timeout);
    }
    return STATUS_SUCCESS;
}
