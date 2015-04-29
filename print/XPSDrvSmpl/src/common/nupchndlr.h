/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupchndlr.h

Abstract:

   NUp PrintCapabilities handling definition. The NUp PC handler
   is used to set NUp settings in a PrintCapabilities.

--*/

#pragma once

#include "pchndlr.h"
#include "nupdata.h"

class CNUpPCHandler : public CPCHandler
{
public:
    CNUpPCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CNUpPCHandler();

    HRESULT
    SetCapabilities(
        VOID
        );
};
