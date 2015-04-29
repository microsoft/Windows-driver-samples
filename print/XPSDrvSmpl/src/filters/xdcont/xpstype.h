/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpstype.h

Abstract:

   Definition of the XPS content type part SAX handler. This class is responsible
   for retrieving all the content types information and storing them for access by the
   XPS processor.

--*/

#pragma once

#include "saxhndlr.h"
#include "xps.h"

class CContentTypes : public CSaxHandler
{
public:
    CContentTypes();

    virtual ~CContentTypes();

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

    HRESULT
    ValidateContentType(
        _In_ PCSTR              szPartName,
        _In_ CONST EContentType contentType
        );

private:
    HRESULT
    GetContentTypeFromString(
        _In_  CONST CStringXDA& cstrPartType,
        _Out_ EContentType*     pContentType
        );

private:
    ContentMap m_contentMap;
};

