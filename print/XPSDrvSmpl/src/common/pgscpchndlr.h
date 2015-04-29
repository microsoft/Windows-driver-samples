/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscpchndlr.h

Abstract:

   Page Scaling PrintCapabilities handling definition. The page scaling PC handler
   is used to set page scaling settings in a PrintCapabilities.

--*/

#pragma once

#include "pchndlr.h"
#include "pgscdata.h"

class CPageScalingPCHandler : public CPCHandler
{
public:
    CPageScalingPCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CPageScalingPCHandler();

    HRESULT
    SetCapabilities(
        VOID
        );
};
