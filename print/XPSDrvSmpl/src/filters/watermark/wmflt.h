/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmflt.h

Abstract:

   Watermark filter defnition. This class derives from the Xps filter
   class and implements the necessary part handlers to support Watermark
   printing. The Watermark filter is responsible for adding resources to
   the XPS document and putting the appropriate mark-up onto pages with a
   watermark

--*/

#pragma once

#include "xdrchflt.h"
#include "wmbase.h"
#include "gdip.h"

class CWatermarkFilter : public CXDXpsFilter
{
public:
    CWatermarkFilter();

    virtual ~CWatermarkFilter();

private:
    virtual HRESULT
    ProcessPart(
        _Inout_ IFixedPage* pFP
        );

    HRESULT
    GetWatermark(
        _In_            IXMLDOMDocument2*  pPrintTicket,
        _Outptr_result_maybenull_ CWatermark**       ppWatermark
        );

private:
    GDIPlus                   m_gdiPlus;
};

