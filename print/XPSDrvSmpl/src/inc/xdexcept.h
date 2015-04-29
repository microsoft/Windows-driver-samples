/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdexcept.h

Abstract:

   XPSDrv sample driver exception class

--*/

#pragma once

class CXDException
{
public:
    /*++

    Routine Name:

        CXDException

    Routine Description:

        Class constructor

    Arguments:

        None

    Return Value:

        None

    --*/
    CXDException() throw() :
        m_hr(E_FAIL)
    {
    }

    /*++

    Routine Name:

        CXDException

    Routine Description:

        Class constructor

    Arguments:

        hr - HRESULT value for the error for which the exception is thrown

    Return Value:

        None

    --*/
    CXDException(
        HRESULT hr
        ) throw() :
        m_hr(hr)
    {
    }

    /*++

    Routine Name:

        operator HRESULT()

    Routine Description:

        HRESULT cast operator

    Arguments:

        None

    Return Value:

        The HRESULT value associated with the exception

    --*/
    operator HRESULT() CONST throw()
    {
        return m_hr;
    }

public:
    HRESULT m_hr;
};

