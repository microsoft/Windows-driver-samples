/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pimagepthndlr.h

Abstract:

   This class is responsible for retrieving the PageImageableSize properties from a
   PrintCapabilities document.

--*/

#pragma once

#include "pchndlr.h"
#include "pimagedata.h"

class CPageImageablePCHandler : public CPCHandler
{
public:
    CPageImageablePCHandler(
        _In_ IXMLDOMDocument2* pPrintCapabilities
        );

    virtual ~CPageImageablePCHandler();

    HRESULT
    GetData(
        _Inout_ XDPrintSchema::PageImageableSize::PageImageableData* pPageImageableData
        );
};

