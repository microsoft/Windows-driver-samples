/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkflt.cpp

Abstract:

   Booklet filter implementation. This class derives from the Xps filter
   class and implements the necessary part handlers to support booklet
   printing. The booklet filter is responsible for re-ordering pages and re-uses
   the NUp filter to provide 2-up and offset support.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "pthndlr.h"
#include "bkflt.h"
#include "bksax.h"
#include "bkpthndlr.h"

using XDPrintSchema::Binding::BindingData;

/*++

Routine Name:

    CBookletFilter::CBookletFilter

Routine Description:

    Default constructor for the booklet filter which initialises the
    filter to sensible default values

Arguments:

    None

Return Value:

    None

--*/
CBookletFilter::CBookletFilter() :
    m_bSendAllDocs(TRUE),
    m_bookScope(CBkPTProperties::None)
{
}

/*++

Routine Name:

    CBookletFilter::~CBookletFilter

Routine Description:

    Default destructor for the booklet filter

Arguments:

    None

Return Value:

    None

--*/
CBookletFilter::~CBookletFilter()
{
}

/*++

Routine Name:

    CNUpFilter::ProcessPart

Routine Description:

    Method for processing each fixed document sequence part in a container

Arguments:

    pFDS - Pointer to the fixed document sequence to process

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletFilter::ProcessPart(
    _Inout_ IFixedDocumentSequence* pFDS
    )
{
    VERBOSE("Processing Fixed Document Sequence part with booklet filter handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFDS, E_POINTER)))
    {
        //
        // Get the PT manager to return the FixedDocumentSequence ticket.
        //
        IXMLDOMDocument2* pPT = NULL;
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFDS)) &&
            SUCCEEDED(hr = m_ptManager.GetTicket(kPTJobScope, &pPT)))
        {
            //
            // Set the binding scope from the PrintTicket
            //
            hr = SetBindingScope(pPT);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pXDWriter->SendFixedDocumentSequence(pFDS);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletFilter::ProcessPart

Routine Description:

    Method for processing each fixed document part in a container

Arguments:

    pFD - Pointer to the fixed document to process

Return Value:

    HRESULT
    S_OK    - On success
    S_FALSE - When not enabled in the PT
    E_*     - On error

--*/
HRESULT
CBookletFilter::ProcessPart(
    _Inout_ IFixedDocument* pFD
    )
{
    VERBOSE("Processing Fixed Document part with booklet filter handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFD, E_POINTER)) &&
        SUCCEEDED(hr = m_ptManager.SetTicket(pFD)))
    {
        //
        // If we are in a JobBook session we want to maintain the current
        // JobBook settings
        //
        if (m_bookScope != CBkPTProperties::Job)
        {
            //
            // Flush any outstanding pages in case we have just completed a
            // DocNUp sequence
            //
            hr = FlushCache();

            //
            // Get the PT manager to return the FixedDocument ticket.
            //
            IXMLDOMDocument2* pPT = NULL;
            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = m_ptManager.GetTicket(kPTDocumentScope, &pPT)))
            {
                //
                // Set the binding scope from the PrintTicket
                //
                hr = SetBindingScope(pPT);
            }
        }
    }

    if (SUCCEEDED(hr) &&
        m_bSendAllDocs)
    {
        hr = m_pXDWriter->SendFixedDocument(pFD);

        //
        // If we are JobBindAllDocuments we only ever send one doc - now we have
        // sent the first document we can test to see if we need to send all of them
        //
        if (m_bookScope == CBkPTProperties::Job)
        {
            m_bSendAllDocs = FALSE;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletFilter::ProcessPart

Routine Description:

    Method for processing each fixed page part in a container

Arguments:

    pFP - Pointer to the fixed page to process

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletFilter::ProcessPart(
    _Inout_ IFixedPage* pFP
    )
{
    ASSERTMSG(m_pXDWriter != NULL, "XD writer is not initialised.\n");

    VERBOSE("Processing Fixed Page with booklet filter handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pXDWriter, E_PENDING)))
    {
        //
        // Check if we are processing a booklet job
        //
        if (m_bookScope != CBkPTProperties::None)
        {
            //
            // Cache pages for reordering
            //
            try
            {
                m_cacheFP.push_back(pFP);
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
        else
        {
            hr = m_pXDWriter->SendFixedPage(pFP);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletFilter::Finalize

Routine Description:

    Method to flush the cache of pages as the last action of the filter

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletFilter::Finalize(
    VOID
    )
{
    //
    // Just flush the cache of pages
    //
    return FlushCache();
}

/*++

Routine Name:

    CBookletFilter::FlushCache

Routine Description:

    Method to send the cached collection of pages in the correct order back

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletFilter::FlushCache(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (m_pXDWriter == NULL)
    {
        hr = E_PENDING;
    }

    size_t cPages = m_cacheFP.size();

    if (SUCCEEDED(hr) &&
        cPages > 0 &&
        m_bookScope != CBkPTProperties::None)
    {
        //
        // We may need to add a pad page if the page count is odd
        //
        CComPtr<IFixedPage> pNewFP(NULL);
        if (cPages%2 == 1 &&
            SUCCEEDED(hr = CreatePadPage(&pNewFP)))
        {
            //
            // We successfully created our pad page; add it to the cache
            //
            try
            {
                m_cacheFP.push_back(pNewFP);
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }

            cPages++;
        }

        if (SUCCEEDED(hr))
        {
            try
            {
                //
                // Re-order pages in the cache
                //
                map<size_t, IFixedPage*>  reorderedPages;
                size_t newIndex = 0;
                size_t pageIndex = 0;
                for (pageIndex = 0; pageIndex < cPages/2; pageIndex++)
                {
                    reorderedPages[newIndex] = m_cacheFP[pageIndex];
                    newIndex += 2;
                }

                newIndex = cPages - 1;
                for (pageIndex = cPages/2; pageIndex < cPages; pageIndex++)
                {
                    reorderedPages[newIndex] = m_cacheFP[pageIndex];
                    newIndex -= 2;
                }

                //
                // Write out reordered pages
                //
                for (pageIndex = 0; pageIndex < cPages && SUCCEEDED(hr); pageIndex++)
                {
                    hr = m_pXDWriter->SendFixedPage(reorderedPages[pageIndex]);
                }

                //
                // Clean out the cache
                //
                m_cacheFP.clear();
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletFilter::CreatePadPage

Routine Description:

    Method to create a pad page which is required for odd page counts to
    ensure pages are correctly ordered for presentation as a booklet

Arguments:

    ppNewPage - Pointer to a pointer to the newly created fixed page

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletFilter::CreatePadPage(
    _Outptr_ IFixedPage** ppNewPage
    )
{
    HRESULT hr = S_OK;

    //
    // Validate parameters and members before proceeding
    //
    if (SUCCEEDED(hr = CHECK_POINTER(ppNewPage, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pXDWriter, E_PENDING)))
    {
        *ppNewPage = NULL;
        PCWSTR pszPageName = NULL;

        try
        {
            //
            // Create a unique name for the pad page for this print session
            //
            CStringXDW szNewPageName;
            szNewPageName.Format(L"/Pad_page_%u.xaml", GetUniqueNumber());
            pszPageName = szNewPageName.GetBuffer();

            //
            // Create a new empty page and retrieve a writer. Also get a
            // reader from the first page so we can copy the FixedPage root
            // element. This ensures the page sizes match.
            //
            CComPtr<IPrintWriteStream>  pWriter(NULL);
            CComPtr<ISAXXMLReader>      pSaxRdr(NULL);

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = m_pXDWriter->GetNewEmptyPart(pszPageName,
                                                            IID_IFixedPage,
                                                            reinterpret_cast<PVOID*>(ppNewPage),
                                                            &pWriter)) &&
                SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)))
            {
                //
                // We use a simple SAX handler which copies only the root
                // element and discards all other content.
                //
                CBkSaxHandler bkSaxHndlr(pWriter);
                CComPtr<IPrintReadStream> pReader(NULL);

                IFixedPage* pFP = NULL;

                pFP = m_cacheFP[0];

                if (SUCCEEDED(hr) &&
                    SUCCEEDED(hr = pSaxRdr->putContentHandler(&bkSaxHndlr)) &&
                    SUCCEEDED(hr = pFP->GetStream(&pReader)))
                {
                    CComPtr<ISequentialStream> pReadStreamToSeq(NULL);

                    pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pReader));

                    if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)))
                    {
                        hr = pSaxRdr->parse(CComVariant(static_cast<ISequentialStream*>(pReadStreamToSeq)));
                    }
                }

                pWriter->Close();
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

/*++

Routine Name:

    CBookletFilter::SetBindingScope

Routine Description:

    Method to retrieve the binding scope from a PrintTicket

Arguments:

    pPT - Pointer to the PrintTicket to retrieve the scope from

Return Value:

    HRESULT
    S_OK    - On success
    S_FALSE - Booklet settings not present in the PT
    E_*     - On error

--*/
HRESULT
CBookletFilter::SetBindingScope(
    _In_ IXMLDOMDocument2* pPT
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPT, E_POINTER)))
    {
        try
        {
            BindingData    bindingData;
            CBookPTHandler bkPTHandler(pPT);

            //
            // Retrieve the booklet properties from the ticket via the handler
            //
            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = bkPTHandler.GetData(&bindingData)))
            {
                CBkPTProperties bookletPTProps(bindingData);

                //
                // Retrieve the booklet scope
                //
                hr = bookletPTProps.GetScope(&m_bookScope);
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // Booklet PT settings are not present - reset hr to S_FALSE and proceed
                //
                hr = S_FALSE;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    return hr;
}

