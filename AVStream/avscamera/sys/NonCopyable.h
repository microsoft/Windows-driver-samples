/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        NonCopyable.h

    Abstract:

        Simple base class that hides the copy ctor and assignment operators.

        Derive from this class any time its not safe (or reasonable) to
        permit copies of an object.

    History:

        created 5/8/2013

**************************************************************************/
#pragma once

//
//  General base class for noncopyable objects
//
class CNonCopyable
{
protected:
    CNonCopyable() {}
    ~CNonCopyable() {}

private:
    CNonCopyable(
        _In_    const CNonCopyable &
    )
    {}

    const CNonCopyable &
    operator =(
        _In_    const CNonCopyable &
    )
    {
        return *this;
    }
};

