/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scalesax.h

Abstract:

   Page Scaling SAX handler definition. The class derives from the default SAX handler
   and implements only the necessary SAX APIs to process the mark-up. The
   handler is responsible for copying page mark-up to a writer, removing
   the fixed page opening and closing tags.

--*/

#pragma once

#include "saxhndlr.h"
#include "pagescale.h"

class CScaleSaxHandler : public CSaxHandler
{
public:
    CScaleSaxHandler(
        _In_ ISequentialStream* pWriter,
        _In_ CPageScaling*      pPageScaling
       );

    virtual ~CScaleSaxHandler();

    virtual HRESULT STDMETHODCALLTYPE
    startDocument(
        void
        );

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

private:
    CComPtr<ISequentialStream>  m_pWriter;

    CComBSTR                    m_bstrOpenElement;

    BOOL                        m_bOpenTag;

    CPageScaling*               m_pPageScaling;
};

