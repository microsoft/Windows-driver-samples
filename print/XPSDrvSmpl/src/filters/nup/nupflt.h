/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupflt.h

Abstract:

   NUp filter definition. This class derives from the Xps filter
   class and implements the necessary part handlers to support booklet
   printing. The nup filter is responsible for applyinh page transofmations
   appropriate to the NUp option selected.

Known Issues:

   The filter uses PageMediaSize and not PageImageableSize to calculate the
   canvas bounds

   The filter does not yet handle booklet offsets (gutter etc.)

--*/

#pragma once

#include "xdrchflt.h"
#include "nuppage.h"
#include "gdip.h"

class CNUpFilter : public CXDXpsFilter
{
public:
    CNUpFilter();

    virtual ~CNUpFilter();

private:

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

    HRESULT
    Finalize(
        VOID
        );

    VOID
    DeleteNUpPage(
        VOID
        );

    HRESULT
    CreateNUpPage(
        _In_ IXMLDOMDocument2* pPT
        );

protected:
    GDIPlus                     m_gdiPlus;

    CNUpPage*                   m_pNUpPage;

    BOOL                        m_bSendAllDocs;

    CNUpPTProperties::ENUpScope m_nupScope;

    CResourceCopier             m_resCopier;
};

