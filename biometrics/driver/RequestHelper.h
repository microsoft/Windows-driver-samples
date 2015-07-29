/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    RequestHelper.h

Abstract:

    This module contains the class definition and implementation
    of an RAII Request object helper class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// This class handles RAII for IWdfIoRequest pointers.
// A function can declare this class at the beginning, and
// set the HRESULT for the request completion.
//
// The destructor is always called on function exit.
// It will complete the request only if the HRESULT
// is something besides HRESULT_FROM_WIN32(ERROR_IO_PENDING)
//
// If the function does not want to complete the request,
// it should not call SetCompletionHr.  Then the request
// will remain pending.
//

class CRequestHelper
{

//
// Public methods
//
public:

    CRequestHelper(
        IWDFIoRequest *FxRequest
        )
    {
        m_Request = FxRequest;
        m_Hr = HRESULT_FROM_WIN32(ERROR_IO_PENDING);
    }

    ~CRequestHelper()
    {
        if (m_Hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
        {
            m_Request->Complete(m_Hr);
        }    
    }

    void
    SetCompletionHr(
        HRESULT Hr
        )
    {
        m_Hr = Hr;
    }

    void
    SetInformation(
        SIZE_T Information
        )
    {
        m_Request->SetInformation(Information);
    }

//
// Private members
//
private:

    IWDFIoRequest * m_Request;
    HRESULT         m_Hr;

};
