/*++

Copyright (c) 2008 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofpthndlr.h

Abstract:

   PageSourceColorProfile PrintCapabilities handling definition.

--*/

#pragma once

#include "pchndlr.h"
#include "cmprofiledata.h"

class CColorManageProfilePCHandler : public CPCHandler
{
public:

    CColorManageProfilePCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CColorManageProfilePCHandler();

    HRESULT
    SetCapabilities(
        VOID
        );
};

