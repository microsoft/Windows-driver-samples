/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkpthndlr.h

Abstract:

   Booklet PrintTicket handling definition. The booklet PT handler
   is used to extract booklet settings from a PrintTicket and populate
   the booklet data structure with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "bkdata.h"

class CBookPTHandler : public CPTHandler
{
public:
    CBookPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CBookPTHandler();

    HRESULT
    GetData(
        _Out_ XDPrintSchema::Binding::BindingData* pBindData
        );

    HRESULT
    SetData(
        _In_ CONST XDPrintSchema::Binding::BindingData* pBindData
        );

    HRESULT
    Delete(
        VOID
        );
};

