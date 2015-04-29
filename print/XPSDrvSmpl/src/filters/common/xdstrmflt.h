/*++

Copyright (C) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdstrmflt.h

Abstract:

   Base stream filter definition. This provides stream interface specific
   functionality general to all filters that use the stream interface to process
   fixed pages. The class is responsible copying data from reader to writer in the
   absence of the PK archive handling module, for a default fixed page processing
   function (filters should implement their own to manipulate fixed page markup)
   and for initialising the stream reader and writer.

--*/

#pragma once

#include "xdsmplflt.h"
#include "ptmanage.h"
#include "xpsproc.h"

class CXDStreamFilter : public CXDSmplFilter, public IFixedPageProcessor
{
public:
    CXDStreamFilter();

    virtual ~CXDStreamFilter();

protected:
    virtual HRESULT STDMETHODCALLTYPE
    StartOperation(
        VOID
        );

    virtual HRESULT
    ProcessFixedPage(
        _In_  IXMLDOMDocument2*  pFPPT,
        _In_  ISequentialStream* pPageReadStream,
        _In_  ISequentialStream* pPageWriteStream
        );

    HRESULT
    InitialiseStreamIO(
        VOID
        );

protected:
    CComPtr<IPrintReadStream>  m_pStreamReader;

    CComPtr<IPrintWriteStream> m_pStreamWriter;
};

