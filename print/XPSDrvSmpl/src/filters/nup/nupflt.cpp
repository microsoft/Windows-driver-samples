/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupflt.cpp

Abstract:

   NUp filter implementation. This class derives from the Xps filter class
   and implements the necessary part handlers to support NUp printing. The
   NUp filter is responsible for applying page transformations appropriate
   to the NUp option selected.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "nupflt.h"
#include "nupsax.h"
#include "nupthndlr.h"
#include "bkpthndlr.h"
#include "psizepthndlr.h"
#include "porientpthndlr.h"


using XDPrintSchema::NUp::NUpData;

using XDPrintSchema::Binding::BindingData;

using XDPrintSchema::PageMediaSize::PageMediaSizeData;

using XDPrintSchema::PageOrientation::PageOrientationData;

/*++

Routine Name:

    CNUpFilter::CNUpFilter

Routine Description:

    Default constructor for the nup filter which ensures GDI plus is correctly running
    and initialises the CNUpFilter variables to sensible defaults

Arguments:

    None

Return Value:

    None

--*/
CNUpFilter::CNUpFilter() :
    m_bSendAllDocs(TRUE),
    m_pNUpPage(NULL),
    m_nupScope(CNUpPTProperties::None)
{
    ASSERTMSG(m_gdiPlus.GetGDIPlusStartStatus() == Ok, "GDI plus is not correctly initialized.\n");
}

/*++

Routine Name:

    CNUpFilter::~CNUpFilter

Routine Description:

    Default destructor for the color management filter

Arguments:

    None

Return Value:

    None

--*/
CNUpFilter::~CNUpFilter()
{
    DeleteNUpPage();
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
CNUpFilter::ProcessPart(
    _Inout_ IFixedDocumentSequence* pFDS
    )
{
    VERBOSE("Processing Fixed Document Sequence part with NUpFilter handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFDS, E_POINTER)))
    {
        //
        // Get the PT manager to return the correct ticket.
        //
        IXMLDOMDocument2* pPT = NULL;
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFDS)) &&
            SUCCEEDED(hr = m_ptManager.GetTicket(kPTJobScope, &pPT)))
        {
            hr = CreateNUpPage(pPT);
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

    CNUpFilter::ProcessPart

Routine Description:

    Method for processing each fixed document part in a container

Arguments:

    pFD - Pointer to the fixed document to process

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpFilter::ProcessPart(
    _Inout_ IFixedDocument* pFD
    )
{
    VERBOSE("Processing Fixed Document part with NUpFilter handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFD, E_POINTER)) &&
        SUCCEEDED(hr = m_ptManager.SetTicket(pFD)))
    {
        //
        // If we are in a JobNUpAllDocumentsContiguously session we want to maintain the current
        // JobNUpAllDocumentsContiguously settings so keep the current NUp page object
        //
        if (m_nupScope != CNUpPTProperties::Job)
        {
            //
            // Get the PT manager to return the correct ticket.
            //
            IXMLDOMDocument2* pPT = NULL;
            if (SUCCEEDED(hr = m_ptManager.GetTicket(kPTDocumentScope, &pPT)))
            {
                hr = CreateNUpPage(pPT);
            }
        }
    }

    if (SUCCEEDED(hr) &&
        m_bSendAllDocs)
    {
        hr = m_pXDWriter->SendFixedDocument(pFD);

        //
        // If we are JobNUpAllDocumentsContiguously we only ever send one doc - now we have
        // sent the first document we can test to see if we need to
        // send all of them
        //
        if (m_nupScope == CNUpPTProperties::Job)
        {
            m_bSendAllDocs = FALSE;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpFilter::ProcessPart

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
CNUpFilter::ProcessPart(
    _Inout_ IFixedPage* pFP
    )
{
    VERBOSE("Processing Fixed Page part with NUpFilter handler\n");

    ASSERTMSG(m_pXDWriter != NULL, "NULL consumer pointer\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pXDWriter, E_PENDING)))
    {
        if (m_pNUpPage != NULL)
        {
            //
            // Add this pages contents to our NUp page
            //
            hr = m_pNUpPage->AddPageContent(m_pXDWriter, pFP);
        }
        else
        {
            //
            // Just write out the fixed page
            //
            hr = m_pXDWriter->SendFixedPage(pFP);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CNUpFilter::Finalize

Routine Description:

    Method to close any pages which are still open as the last action of the filter

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpFilter::Finalize(
    VOID
    )
{
    ASSERTMSG(m_pXDWriter != NULL, "NULL consumer pointer\n");

    HRESULT hr = S_OK;

    if (m_pNUpPage != NULL)
    {
        //
        // Close any open pages - we are done
        //
        hr = m_pNUpPage->ClosePage(m_pXDWriter);
        DeleteNUpPage();
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpFilter::DeleteNUpPage

Routine Description:

    Method to delete the current CNUpPage object

Arguments:

    None

Return Value:

    None

--*/
VOID
CNUpFilter::DeleteNUpPage(
    VOID
    )
{
    if (m_pNUpPage != NULL)
    {
        delete m_pNUpPage;
        m_pNUpPage = NULL;
    }
}

/*++

Routine Name:

    CNUpFilter::CreateNUpPage

Routine Description:

    Method to create a new nup page based on the settings specified in the PrintTicket

Arguments:

    pPT - Pointer to the PrintTicket containing the nup driver settings

Return Value:

    HRESULT
    S_OK    - On success
    S_FALSE - When not enabled in the PT
    E_*     - On error

--*/
HRESULT
CNUpFilter::CreateNUpPage(
    _In_ IXMLDOMDocument2* pPT
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPT, E_POINTER)))
    {

        try
        {
            //
            // Retrieve the NUp, Booklet, PageMediaSize and PageOrientation data from the PT
            //
            NUpData             nUpData;
            BindingData         bindingData;
            PageMediaSizeData   pageMediaSizeData;
            PageOrientationData pageOrientData;

            CNUpPTHandler             nUpPTHandler(pPT);
            CBookPTHandler            bkPTHandler(pPT);
            CPageSizePTHandler        pageSizePTHandler(pPT);
            CPageOrientationPTHandler pageOrientPTHandler(pPT);

            //
            // Try the booklet data first
            //
            if (FAILED(hr = bkPTHandler.GetData(&bindingData)))
            {
                //
                // If booklet is not present try NUp settings
                //
                if (hr == E_ELEMENT_NOT_FOUND)
                {
                    hr = nUpPTHandler.GetData(&nUpData);
                }
            }

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = pageSizePTHandler.GetData(&pageMediaSizeData)) &&
                SUCCEEDED(hr = pageOrientPTHandler.GetData(&pageOrientData)))
            {
                CNUpPTProperties nUpProps(nUpData, bindingData, pageMediaSizeData, pageOrientData);

                UINT cNUp = 1;
                if (SUCCEEDED(hr = nUpProps.GetScope(&m_nupScope)) &&
                    SUCCEEDED(hr = nUpProps.GetCount(&cNUp)) &&
                    m_nupScope != CNUpPTProperties::None &&
                    cNUp > 1)
                {
                    if (m_pNUpPage == NULL)
                    {
                        //
                        // Create a new NUp page
                        //
                        m_pNUpPage = new(std::nothrow) CNUpPage(&nUpProps, &m_resCopier);

                        hr = CHECK_POINTER(m_pNUpPage, E_OUTOFMEMORY);
                    }
                    else if (m_nupScope == CNUpPTProperties::Document)
                    {
                        //
                        // Second or subsequent Document NUp session - close the page and set
                        // the new properties
                        //
                        if (SUCCEEDED(hr = m_pNUpPage->ClosePage(m_pXDWriter)))
                        {
                            hr = m_pNUpPage->SetProperties(&nUpProps);
                        }
                    }
                }
                else
                {
                    //
                    // NUp is not set - Fail the request so the page is closed and deleted
                    //
                    hr = E_ELEMENT_NOT_FOUND;
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (hr == E_ELEMENT_NOT_FOUND)
    {
        //
        // The relevant features are not present in the print ticket or contain unsupported values.
        // Close and delete the NUp page if it is currently open and do not propogate the fail status.
        //
        hr = S_FALSE;

        if (m_pNUpPage != NULL)
        {
            hr = m_pNUpPage->ClosePage(m_pXDWriter);
        }

        DeleteNUpPage();
    }

    ERR_ON_HR(hr);
    return hr;
}

