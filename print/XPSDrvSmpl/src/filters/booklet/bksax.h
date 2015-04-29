/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bksax.cpp

Abstract:

   Booklet filter SAX handler definition. The booklet SAX handler derives
   from the default SAX handler and implements the necesary SAX interfaces to
   process fixed page mark-up for booklet printing.
   For documents with odd page counts, we add a blank padding page so that the
   2-Up behaves correctly. All that is required of the handler is to retrieve
   the fixed page open tag and write this out.

--*/

#pragma once

#include "saxhndlr.h"

class CBkSaxHandler : public CSaxHandler
{
public:
    CBkSaxHandler(
        _In_ IPrintWriteStream* pWriter
        );

    virtual ~CBkSaxHandler();

    HRESULT STDMETHODCALLTYPE
    startElement(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        _In_reads_(cchQName) CONST wchar_t*  pwchQName,
        _In_                  INT             cchQName,
        _In_                  ISAXAttributes* pAttributes
        );

    HRESULT STDMETHODCALLTYPE
    startDocument(
        void
        );

private:
    CComPtr<IPrintWriteStream> m_pWriter;
};

