/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nuppage.h

Abstract:

   NUp page definition. This class is responsible for maintaining
   the current NUp page. The public interface defines methods for adding
   fixed page content and closing the current page. When a page is added,
   the class uses a SAX handler to strip the FixedPage tags from the
   source page, apply a canvas with a transformation and add it to the
   current NUp page. When the page is full it is closed and sent and a new
   NUp page is created.

--*/

#pragma once

#include "rescpy.h"
#include "xdstring.h"
#include "nupxform.h"

class CNUpPage
{
public:
    CNUpPage(
        _In_ CNUpPTProperties* pNUpProps,
        _In_ CResourceCopier*  pResCopier
        );

    virtual ~CNUpPage();

    HRESULT
    AddPageContent(
        _In_ IXpsDocumentConsumer* pWriter,
        _In_ IFixedPage*           pFP
        );

    HRESULT
    ClosePage(
        _In_ IXpsDocumentConsumer* pWriter
        );

    HRESULT
    SetProperties(
        _In_ CNUpPTProperties* pNUpProps
        );

private:
    HRESULT
    CreateNewPage(
        _In_ IXpsDocumentConsumer* pWriter,
        _In_ SizeF                 sizePage
        );

    HRESULT
    CopyResources(
        _In_    IFixedPage* pFPSrc,
        _Inout_ IFixedPage* pFPDst
        );

    VOID
    DeleteProperties(
        VOID
        );

    VOID
    DeleteTransform(
        VOID
        );

    HRESULT
    WriteToPrintStream(
        _In_ CStringXDW*          pcstrOut,
        _In_ IPrintWriteStream* pWriter
        );

private:
    CComPtr<IPrintWriteStream> m_pWriter;

    CComPtr<IFixedPage>        m_pFixedPage;

    //
    // Current page index (i.e. 0 - 5 for 6 Up)
    //
    UINT                      m_cCurrPageIndex;

    UINT                      m_cNUp;

    CNUpTransform*            m_pNUpTransform;

    CNUpPTProperties*         m_pNUpProps;

    CResourceCopier*          m_pResCopier;
};

