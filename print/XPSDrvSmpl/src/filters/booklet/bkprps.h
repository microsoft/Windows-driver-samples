/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkprps.h

Abstract:

   Booklet properties class definition. The booklet properties class
   is responsible for holding and controling booklet properties.

--*/

#pragma once

#include "bkdata.h"

class CBkPTProperties
{
public:
    enum EBookletScope
    {
        None = 0,
        Job,
        Document
    };

public:
    CBkPTProperties(
        _In_ CONST XDPrintSchema::Binding::BindingData& bindingData
        );

    virtual ~CBkPTProperties();

    HRESULT
    GetScope(
        _Out_ EBookletScope* pBkScope
        );

private:
    XDPrintSchema::Binding::BindingData   m_bindData;
};

