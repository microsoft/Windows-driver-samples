/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   porientpthndlr.h

Abstract:

   PageOrientation PrintTicket handling definition. The PageOrientation PT handler
   is used to extract PageOrientation settings from a PrintTicket and populate
   the PageOrientation data structure with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "porientdata.h"

class CPageOrientationPTHandler : public CPTHandler
{
public:
    CPageOrientationPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CPageOrientationPTHandler();

    HRESULT
    GetData(
        _Inout_ XDPrintSchema::PageOrientation::PageOrientationData* pPageOrientationData
        );
};

