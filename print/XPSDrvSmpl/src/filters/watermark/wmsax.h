/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmsax.h

Abstract:

   Watermark sax handler definition. The watermark SAX handler is responsible for
   parsing the FixedPage mark-up for the page size and inserting the watermark mark-up
   in appropriate place.

--*/

#pragma once

#include "saxhndlr.h"
#include "wmtext.h"

class CWMSaxHandler : public CSaxHandler
{
public:
    CWMSaxHandler(
        _In_ IPrintWriteStream* pWriter,
        _In_ CWatermark*        pWatermark
        );

    virtual ~CWMSaxHandler();

    virtual HRESULT STDMETHODCALLTYPE
    startElement(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        _In_reads_(cchQName) CONST wchar_t*  pwchQName,
        _In_                  INT             cchQName,
        _In_                  ISAXAttributes* pAttributes
        );

    virtual HRESULT STDMETHODCALLTYPE
    endElement(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        _In_reads_(cchQName) CONST wchar_t* pwchQName,
        _In_                  INT            cchQName
        );

    HRESULT STDMETHODCALLTYPE
    startDocument(
        void
        );

private:
    CComPtr<IPrintWriteStream> m_pWriter;

    CComBSTR                   m_bstrOpenElement;

    BOOL                       m_bOpenTag;

    CWatermark*                m_watermark;
};

