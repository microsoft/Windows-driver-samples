/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdrchflt.h

Abstract:

   Base Xps filter definition. The CXDXpsFilter provides common filter
   functionality for Xps filters. It provides default handlers for part
   handlers that set print tickets appropriately through the PrintTicket
   manager class. This allows derived classes to implement only the part
   handlers that they require (for example, the watermark filter is only
   interested in the fixed page, and leaves all other parts to be handled
   by this class). The class implements IPrintPipelineFilter::StartOperation
   which is responsible for retrieving parts from the Xps provider and
   dispatching them to the relevant part handler. It is also responsible for
   intialising the Xps provider and consumer.

--*/

#pragma once

#include "xdsmplflt.h"
#include "rescache.h"

class CXDXpsFilter : public CXDSmplFilter
{
public:
    CXDXpsFilter();

    virtual ~CXDXpsFilter();

protected:
    virtual HRESULT STDMETHODCALLTYPE
    StartOperation(
        VOID
        );

    virtual HRESULT
    ProcessPart(
        _Inout_ IXpsDocument* pXD
        );

    virtual HRESULT
    ProcessPart(
        _Inout_ IFixedDocumentSequence* pFDS
        );

    virtual HRESULT
    ProcessPart(
        _Inout_ IFixedDocument* pFD
        );

    virtual HRESULT
    ProcessPart(
        _Inout_ IFixedPage* pFP
        );

    virtual HRESULT
    Finalize(
        VOID
        );

    virtual HRESULT
    InitialiseXDIO(
        VOID
        );

protected:
    CComPtr<IXpsDocumentProvider>  m_pXDReader;

    CComPtr<IXpsDocumentConsumer>  m_pXDWriter;

    CFileResourceCache             m_resCache;
};

