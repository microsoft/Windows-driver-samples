/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmpchndlr.h

Abstract:

   Page Watermark PrintCapabilities handling definition. The booklet PC handler
   is used to set booklet settings in a PrintCapabilities.

--*/

#pragma once

#include "pchndlr.h"
#include "wmdata.h"

class CWMPCHandler : public CPCHandler
{
public:
    CWMPCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CWMPCHandler();

    HRESULT
    SetCapabilities(
        VOID
        );
};
