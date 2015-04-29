/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   saxhndlr.h

Abstract:

   Default sax handler definition. Provides default implementations
   for the ISAXContentHandler. This allows derived classes to only need
   to implement the methods that are required.

--*/

#pragma once

#include "CUnknown.h"

class CSaxHandler : public CUnknown<ISAXContentHandler>
{
public:
    CSaxHandler();

    virtual ~CSaxHandler();

    virtual HRESULT STDMETHODCALLTYPE
    putDocumentLocator(
        ISAXLocator *
        );

    virtual HRESULT STDMETHODCALLTYPE
    startDocument(
        void
        );

    virtual HRESULT STDMETHODCALLTYPE
    endDocument(
        void
        );

    virtual HRESULT STDMETHODCALLTYPE
    startPrefixMapping(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    endPrefixMapping(
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    startElement(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        _In_ ISAXAttributes*
        );

    virtual HRESULT STDMETHODCALLTYPE
    endElement(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    characters(
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    ignorableWhitespace(
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    processingInstruction(
        CONST wchar_t*,
        INT,
        CONST wchar_t*,
        INT
        );

    virtual HRESULT STDMETHODCALLTYPE
    skippedEntity(
        CONST wchar_t*,
        INT
        );

protected:
    HRESULT
    WriteToPrintStream(
        _In_ CStringXDW*          pcstrOut,
        _In_ IPrintWriteStream* pWriter
        );

    HRESULT
    WriteToPrintStream(
        _In_ CStringXDW*          pcstrOut,
        _In_ ISequentialStream* pWriter
        );

    HRESULT
    EscapeEntity(
        _Inout_ BSTR* pStr
        );
};

