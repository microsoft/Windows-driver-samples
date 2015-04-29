/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupthndlr.h

Abstract:

   NUp properties class implementation. The NUp properties class
   is responsible for holding and controling NUp properties.

--*/

#pragma once


#include "pthndlr.h"
#include "nupdata.h"

class CNUpPTHandler : public CPTHandler
{
public:
    CNUpPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CNUpPTHandler();

    HRESULT
    GetData(
        _Out_ XDPrintSchema::NUp::NUpData* pNUpData
        );

    HRESULT
    SetData(
        _In_ CONST XDPrintSchema::NUp::NUpData* pNUpData
        );

    HRESULT
    Delete(
        VOID
        );
};

