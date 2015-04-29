/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkflt.h

Abstract:

   Booklet filter class definition. This class derives from the Xps filter
   class and implements the necessary part handlers to support booklet
   printing. The booklet filter is responsible for re-ordering pages and re-uses
   the NUp filter to provide 2-up and offset support.

--*/

#pragma once

#include "xdrchflt.h"
#include "bkprps.h"

class CBookletFilter : public CXDXpsFilter
{
public:
    CBookletFilter();

    virtual ~CBookletFilter();

private:
    HRESULT
    ProcessPart(
        _Inout_ IFixedDocumentSequence* pFDS
        );

    HRESULT
    ProcessPart(
        _Inout_ IFixedDocument* pFD
        );

    HRESULT
    ProcessPart(
        _Inout_ IFixedPage* pFP
        );

    HRESULT
    Finalize(
        VOID
        );

    HRESULT
    FlushCache(
        VOID
        );

    HRESULT
    CreatePadPage(
        _Outptr_ IFixedPage** ppNewPage
        );

    HRESULT
    SetBindingScope(
        _In_ IXMLDOMDocument2* pPT
        );

private:
    vector<CComPtr<IFixedPage> >   m_cacheFP;

    BOOL                           m_bSendAllDocs;

    CBkPTProperties::EBookletScope m_bookScope;
};

