/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplcf.cpp

Abstract:

   XPSDrv feature sample class factory definition.

--*/

#pragma once

#include "cunknown.h"

class CXDSmplUICF : public CUnknown<IClassFactory>
{
public:
    //
    // Constructor and Destruction
    //
    CXDSmplUICF() :
        CUnknown<IClassFactory>(IID_IClassFactory)
    {
    }

    virtual ~CXDSmplUICF()
    {
    }

    //
    // IClassFactory methods
    //
    virtual HRESULT STDMETHODCALLTYPE
    CreateInstance(
        _In_opt_    LPUNKNOWN pUnkOuter,
        _In_        REFIID    riid,
        _Outptr_ PVOID*    ppvObject
        );

    virtual HRESULT STDMETHODCALLTYPE
    LockServer(
        BOOL bLock
        );
};

