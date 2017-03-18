/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        waitable.h

    Abstract:

        Base interface for any object that is waitable.
        In otherwords, it contains a kernel dispatch object.

        Also implements a Cancelable interface class.

    History:

        created 5/30/2014

**************************************************************************/

#pragma once

//
//  Interface definitions for KWaitable and Cancelable
//
class KWaitable :
    public CNonCopyable
{
public:
    KWaitable();

    virtual
    ~KWaitable();

    virtual
    NTSTATUS
    Wait(
        _In_opt_    PLARGE_INTEGER Timeout = NULL
    );

    //  Must implement
    virtual
    PVOID
    GetDispatchObject() = 0;
};

class Cancelable
{
public:
    //  Must implement
    virtual
    BOOLEAN
    Cancel() = 0;
};

