/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofpthndlr.h

Abstract:

   PageSourceColorProfile PrintTicket handling definition.
   The PageSourceColorProfile PT handler is used to extract
   PageSourceColorProfile settings from a PrintTicket and populate
   the PageSourceColorProfile data structure with the retrieved
   settings. The class also defines a method for setting the feature in
   the PrintTicket given the data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "cmprofiledata.h"

class CColorManageProfilePTHandler : public CPTHandler
{
public:
    CColorManageProfilePTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CColorManageProfilePTHandler();

    HRESULT
    GetData(
        _Inout_ XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData* pCmData
        );

    HRESULT
    SetData(
        _In_ CONST XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData* pCmData
        );
};

