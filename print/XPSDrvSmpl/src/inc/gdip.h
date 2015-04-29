/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   gdip.h

Abstract:

   Provides a wrapper class around GDIPlus that takes care of intialisation
   and shutdown. A class only need define this as a member variable to use
   GDIPlus - intialisation and shutdown are handled during construction and
   destruction.

Known issues:

    The class does not yet implement debug event handling.

--*/

#pragma once

class GDIPlus
{
public:
    /*++

    Routine Name:

        GDIPlus

    Routine Description:

        GDIPlus class constructor

    Arguments:

        None

    Return Value:

        None

    --*/
    GDIPlus() :
        m_pGDIPlusToken(NULL),
        m_GDIPlusStartStatus(GdiplusNotInitialized)
    {
        GdiplusStartupInput gdiPlusStartInput;

        m_GDIPlusStartStatus = GdiplusStartup(&m_pGDIPlusToken, &gdiPlusStartInput, NULL);
    }

    /*++

    Routine Name:

        ~GDIPlus

    Routine Description:

        GDIPlus class destructor

    Arguments:

        None

    Return Value:

        None

    --*/
    virtual ~GDIPlus()
    {
        GdiplusShutdown(m_pGDIPlusToken);
        m_pGDIPlusToken = NULL;
        m_GDIPlusStartStatus = GdiplusNotInitialized;
    }

    /*++

    Routine Name:

        GetGDIPlusStartStatus

    Routine Description:

        This routine returns the start status of GDI Plus

    Arguments:

        None

    Return Value:

        Gdiplus::Status
        Ok            - On success
        Gdiplus error - On Error

    --*/
    Status GetGDIPlusStartStatus(
        VOID
        )
    {
        return m_GDIPlusStartStatus;
    }

private:
    ULONG_PTR m_pGDIPlusToken;

    Status  m_GDIPlusStartStatus;
};

