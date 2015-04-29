/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmintpthndlr.h

Abstract:

   PageICMRenderingIntent PrintTicket handler definition. Derived from
   CPTHandler, this provides PageICMRenderingIntent specific Get and Set methods
   acting on the PrintTicket (as a DOM document) passed.

--*/

#pragma once

#include "pthndlr.h"
#include "cmintentsdata.h"

class CColorManageIntentsPTHandler : public CPTHandler
{
public:
    CColorManageIntentsPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CColorManageIntentsPTHandler();

    HRESULT
    GetData(
        _Inout_ XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData* pCmData
        );
};

