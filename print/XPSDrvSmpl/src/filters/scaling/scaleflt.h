/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scaleflt.h

Abstract:

   Page Scaling filter defnition. This class derives from the Xps filter
   class and implements the necessary part handlers to support Page Scaling.
   The Page Scaling filter is responsible for adding mark-up to
   the XPS document to allow scaling of pages.

--*/

#pragma once

#include "xdstrmflt.h"
#include "pagescale.h"
#include "gdip.h"

class CPageScalingFilter : public CXDStreamFilter
{
public:
    CPageScalingFilter();

    virtual ~CPageScalingFilter();

    virtual HRESULT
    ProcessFixedPage(
        _In_  IXMLDOMDocument2*  pFPPT,
        _In_  ISequentialStream* pPageReadStream,
        _In_  ISequentialStream* pPageWriteStream
        );

private:
    HRESULT
    GetPageScaling(
        _In_            IXMLDOMDocument2*  pPrintTicket,
        _In_            IXMLDOMDocument2*  pPrintCapabilities,
        _Outptr_ CPageScaling** ppPageScaling
        );

private:
    GDIPlus                   m_gdiPlus;
};

