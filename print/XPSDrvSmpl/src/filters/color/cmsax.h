/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmsax.h

Abstract:

   Color management sax handler definition. The color management SAX handler
   is responsible for parsing the FixedPage mark-up for and vector objects
   or bitmaps which contain color data and altering the mark-up where appropriate.

--*/

#pragma once

#include "saxhndlr.h"
#include "colconv.h"

class CCMSaxHandler : public CSaxHandler
{
public:
    CCMSaxHandler(
        _In_     IPrintWriteStream*            pWriter,
        _In_     CBitmapColorConverter*        pBmpConverter,
        _In_     CColorRefConverter*           pRefConverter,
        _In_opt_ CResourceDictionaryConverter* pDictConverter
        );

    virtual ~CCMSaxHandler();

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

    CColorConverter*           m_pBmpConv;

    CColorConverter*           m_pRefConv;

    CColorConverter*           m_pDictConv;
};

