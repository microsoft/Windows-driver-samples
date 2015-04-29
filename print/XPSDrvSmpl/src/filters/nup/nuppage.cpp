/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nuppage.cpp

Abstract:

   NUp page implementation. This class is responsible for maintaining
   the current NUp page. The public interface defines methods for adding
   fixed page content and closing the current page. When a page is added,
   the class uses a SAX handler to strip the FixedPage tags from the
   source page, apply a canvas with a transformation and add it to the
   current NUp page. When the page is full it is closed and sent and a new
   NUp page is created.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "nuppage.h"
#include "nupsax.h"
#include "widetoutf8.h"

static CONST PCWSTR pszOpenFPTag = L"<?xml version=\"1.0\" encoding=\"utf-8\"?><FixedPage Width=\"%.2f\" Height=\"%.2f\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/xps/2005/06\">";

/*++

Routine Name:

    CNUpPage::CNUpPage

Routine Description:

    Constructor for the CNUpPage class which initialises itself to sensible values
    and creates a new page transformation

Arguments:

    pNUpProps  - Pointer to class containing nup settings from the PrintTicket
    pResCopier - Pointer to page resource copy class used for copying markup between pages

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CNUpPage::CNUpPage(
    _In_ CNUpPTProperties* pNUpProps,
    _In_ CResourceCopier*  pResCopier
    ) :
    m_pNUpProps(NULL),
    m_pNUpTransform(NULL),
    m_pWriter(NULL),
    m_pFixedPage(NULL),
    m_cCurrPageIndex(0),
    m_cNUp(1),
    m_pResCopier(pResCopier)
{
    HRESULT hr = S_OK;
    ASSERTMSG(m_pResCopier != NULL, "Invalid resource copier\n");

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpProps, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pResCopier, E_POINTER)))
    {
        try
        {
            m_pNUpProps = new(std::nothrow) CNUpPTProperties(*pNUpProps);

            if (SUCCEEDED(hr = CHECK_POINTER(m_pNUpProps, E_OUTOFMEMORY)))
            {
                m_pNUpTransform = new(std::nothrow) CNUpTransform(m_pNUpProps);

                hr = CHECK_POINTER(m_pNUpTransform, E_OUTOFMEMORY);
            }

            if (SUCCEEDED(hr))
            {
                hr = m_pNUpProps->GetCount(&m_cNUp);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (FAILED(hr))
    {
        if (m_pNUpProps != NULL)
        {
            delete m_pNUpProps;
            m_pNUpProps = NULL;
        }

        if (m_pNUpTransform != NULL)
        {
            delete m_pNUpTransform;
            m_pNUpTransform = NULL;
        }

        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CNUpPage::~CNUpPage

Routine Description:

    Default destructor for the CNUpPage class

Arguments:

    None

Return Value:

    None

--*/
CNUpPage::~CNUpPage()
{
    DeleteProperties();
    DeleteTransform();

    ASSERTMSG(m_pWriter == NULL, "Destroying NUp page with open page writer\n");
    ASSERTMSG(m_pFixedPage == NULL, "Destroying NUp page with open fixed page\n");
}

/*++

Routine Name:

    CNUpPage::AddPageContent

Routine Description:

    Method to write out fixed page with a transformation matrix

Arguments:

    pWriter - Pointer to a writer which the transformed fixed page will be sent to
    pFP     - Pointer to the fixed page to be transformed and written out

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPage::AddPageContent(
    _In_ IXpsDocumentConsumer* pWriter,
    _In_ IFixedPage*           pFP
    )
{
    ASSERTMSG(m_pNUpTransform != NULL, "NULL transform object\n");
    ASSERTMSG(m_pNUpProps != NULL, "NULL NUp properties object\n");

    HRESULT hr = S_OK;

    SizeF sizePage;
    if (SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pNUpProps, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pNUpTransform, E_PENDING)) &&
        SUCCEEDED(hr = m_pNUpProps->GetPageSize(&sizePage)) &&
        m_pFixedPage == NULL)
    {
        hr = CreateNewPage(pWriter, sizePage);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create a SAX reader to parse the mark-up write out the page
        // content
        //
        CComPtr<ISAXXMLReader> pSaxRdr(NULL);
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)))
        {
            try
            {
                m_pNUpTransform->SetCurrentPage(m_cCurrPageIndex);

                //
                // Create our NUp sax handler
                //
                CNUpSaxHandler nupSaxHndlr(m_pWriter, m_pResCopier, m_pNUpTransform);

                //
                // Set-up the SAX reader and begin parsing the mark-up
                //
                CComPtr<IPrintReadStream> pReader(NULL);
                if (SUCCEEDED(hr = pSaxRdr->putContentHandler(&nupSaxHndlr)) &&
                    SUCCEEDED(hr = pFP->GetStream(&pReader)))
                {
                    CComPtr<ISequentialStream>  pReadStreamToSeq(NULL);

                    pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pReader));

                    if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)))
                    {
                        hr = pSaxRdr->parse(CComVariant(static_cast<ISequentialStream*>(pReadStreamToSeq)));
                    }
                }
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }

        //
        // Copy resources
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CHECK_POINTER(m_pResCopier, E_PENDING)))
        {
            hr = m_pResCopier->CopyPageResources(pFP, m_pFixedPage);
        }

        m_cCurrPageIndex++;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check if we need to close the page
        //
        if (m_cCurrPageIndex == m_cNUp)
        {
            hr = ClosePage(pWriter);
            m_cCurrPageIndex = 0;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPage::ClosePage

Routine Description:

    Method to close an open fixed page including the addition of any required markup

Arguments:

    pWriter - Pointer to a writer which receives the closing markup

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPage::ClosePage(
    _In_ IXpsDocumentConsumer* pWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        //
        // Add closing page mark-up, close the current page writer
        //
        if (m_pWriter != NULL)
        {
            try
            {
                CStringXDW cstrCloseFP(L"</FixedPage>");
                hr = WriteToPrintStream(&cstrCloseFP, m_pWriter);
            }
            catch (CXDException& e)
            {
                hr = e;
            }

            ASSERTMSG(SUCCEEDED(hr), "Failed to write fixed page closing tag\n");

            m_pWriter->Close();
        }

        //
        // Send the current page
        //
        if (m_pFixedPage != NULL)
        {
            hr = pWriter->SendFixedPage(m_pFixedPage);
        }

        ASSERTMSG(SUCCEEDED(hr), "Failed to send page\n");
    }

    //
    // Release the writer and the fixed page
    //
    m_pWriter = NULL;
    m_pFixedPage = NULL;

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPage::SetProperties

Routine Description:

    Method to set the nup properties at a start of a new run of pages

Arguments:

    pNUpProps - Pointer to an object containing nup settings from the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPage::SetProperties(
    _In_ CNUpPTProperties* pNUpProps
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpProps, E_POINTER)))
    {
        DeleteProperties();
        DeleteTransform();

        m_pNUpProps = new(std::nothrow) CNUpPTProperties(*pNUpProps);

        if (SUCCEEDED(hr = CHECK_POINTER(m_pNUpProps, E_OUTOFMEMORY)))
        {
            m_pNUpTransform = new(std::nothrow) CNUpTransform(m_pNUpProps);

            if (SUCCEEDED(hr = CHECK_POINTER(m_pNUpTransform, E_OUTOFMEMORY)))
            {
                hr = m_pNUpProps->GetCount(&m_cNUp);
                m_cCurrPageIndex = 0;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPage::CreateNewPage

Routine Description:

    Method to create a new fixed page to contain the original pages as canvas'

Arguments:

    pWriter  - Pointer to a writer which the new fixed page will be sent to
    sizePage - Size of the new page

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPage::CreateNewPage(
    _In_ IXpsDocumentConsumer* pWriter,
    _In_ SizeF                 sizePage
    )
{
    HRESULT hr = S_OK;

    CComBSTR bstrPageURI;

    if (SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        //
        // Create a unique page URI
        //
        try
        {
            //
            // Create a unique name for the NUp page for this print session
            //
            CStringXDW cstrPageURI;
            cstrPageURI.Format(L"/NUpPage_%u.xml", GetUniqueNumber());
            bstrPageURI.Empty();
            bstrPageURI.Attach(cstrPageURI.AllocSysString());
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    //
    // Close any open pages, create the new page and retrieve the page writer
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = ClosePage(pWriter)) &&
        SUCCEEDED(hr = pWriter->GetNewEmptyPart(bstrPageURI,
                                                __uuidof(IFixedPage),
                                                reinterpret_cast<VOID**>(&m_pFixedPage),
                                                &m_pWriter)))
    {
        //
        // Construct the opening FixedPage tag
        //
        try
        {
            CStringXDW cstrOpenFP;
            cstrOpenFP.Format(pszOpenFPTag, sizePage.Width, sizePage.Height);
            hr = WriteToPrintStream(&cstrOpenFP, m_pWriter);
        }
        catch (CXDException& e)
        {
            hr = e;
        }

        ASSERTMSG(SUCCEEDED(hr), "Failed to write fixed page opening tag\n");
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPage::DeleteProperties

Routine Description:

    Method to delete the current nup properties object

Arguments:

    None

Return Value:

    None

--*/
VOID
CNUpPage::DeleteProperties(
    VOID
    )
{
    if (m_pNUpProps != NULL)
    {
        delete m_pNUpProps;
        m_pNUpProps = NULL;
    }
}

/*++

Routine Name:

    CNUpPage::DeleteTransform

Routine Description:

    Method to delete the current nup transform object

Arguments:

    None

Return Value:

    None

--*/
VOID
CNUpPage::DeleteTransform(
    VOID
    )
{
    delete m_pNUpTransform;
    m_pNUpTransform = NULL;
}


/*++

Routine Name:

    CNUpPage::WriteToPrintStream

Routine Description:

    This routine converts a CStringXDW buffer to UTF-8 string to be
    written to the write stream provided

Arguments:

    pcstrOut - Pointer to the Atl CStringXDW containing the mark-up to be written
    pWriter  - Pointer to the print write stream to write to

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT
CNUpPage::WriteToPrintStream(
    _In_ CStringXDW*          pcstrOut,
    _In_ IPrintWriteStream* pWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcstrOut, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        ULONG cbWritten = 0;
        PVOID pData = NULL;
        ULONG cbData = 0;

        try
        {
            CWideToUTF8 wideToUTF8(pcstrOut);

            if (SUCCEEDED(hr = wideToUTF8.GetBuffer(&pData, &cbData)))
            {
                hr = pWriter->WriteBytes(pData, cbData, &cbWritten);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

