/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscpthndlr.h

Abstract:

   PageScaling PrintTicket handling definition. The PageScaling PT handler
   is used to extract PageScaling settings from a PrintTicket and populate
   the PageScaling data structure with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "pgscdata.h"

class CPageScalingPTHandler : public CPTHandler
{
public:
    CPageScalingPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CPageScalingPTHandler();

    HRESULT
    GetData(
        _Out_ XDPrintSchema::PageScaling::PageScalingData* pPageScaleData
        );

    HRESULT
    SetData(
        _In_ CONST XDPrintSchema::PageScaling::PageScalingData* pPageScaleData
        );

    HRESULT
    Delete(
        void
        );
};

