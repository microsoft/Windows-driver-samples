/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkpchndlr.h

Abstract:

   Booklet PrintCapabilities handling definition. The booklet PC handler
   is used to set booklet settings in a PrintCapabilities.

--*/

#pragma once

#include "pchndlr.h"
#include "bkdata.h"

class CBookPCHandler : public CPCHandler
{
public:
    CBookPCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CBookPCHandler();

    HRESULT
    SetCapabilities(
        VOID
        );
};
