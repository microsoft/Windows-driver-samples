/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmpthndlr.h

Abstract:

   PageColorManagement PrintTicket handling definition. The
   PageColorManagement PT handler is used to extract booklet settings
   from a PrintTicket and populate the PageColorManagement data
   structure with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "cmdata.h"

class CColorManagePTHandler : public CPTHandler
{
public:
    CColorManagePTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CColorManagePTHandler();

    HRESULT
    GetData(
        _Inout_ XDPrintSchema::PageColorManagement::ColorManagementData* pCmData
        );
};

