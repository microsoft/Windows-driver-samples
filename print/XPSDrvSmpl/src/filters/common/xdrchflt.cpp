/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdrchflt.cpp

Abstract:

   Base Xps filter implementation. The CXDXpsFilter provides common filter
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

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdrchflt.h"

/*++

Routine Name:

    CXDXpsFilter::CXDXpsFilter

Routine Description:

    CXDXpsFilter class constructor

Arguments:

    None

Return Value:

    None

--*/
CXDXpsFilter::CXDXpsFilter() :
    m_pXDReader(NULL),
    m_pXDWriter(NULL)
{
    VERBOSE("Constructing Xps filter\n");
}

/*++

Routine Name:

    CXDXpsFilter::~CXDXpsFilter

Routine Description:

    CXDXpsFilter class destructor

Arguments:

    None

Return Value:

    None

--*/
CXDXpsFilter::~CXDXpsFilter()
{
    VERBOSE("Destroying Xps filter\n");
}

/*++

Routine Name:

    CXDXpsFilter::StartOperation

Routine Description:

    This is the XPS Doxument interface implementation of IPrintPipelineFilter::StartOperation
    shared by all XPS Document filters

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::StartOperation(
    VOID
    )
{
    VERBOSE("Starting filter operation.\n");

    HRESULT hr = S_OK;
    BOOL bDoCoUninitialize = FALSE;

    if (SUCCEEDED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        bDoCoUninitialize = TRUE;
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = InitialiseXDIO()) &&
        SUCCEEDED(hr = InitializePrintTicketManager()))
    {
        CComPtr<IUnknown> pUnk(NULL);

        while (SUCCEEDED(hr) &&
               SUCCEEDED(hr = m_pXDReader->GetXpsPart(&pUnk)) &&
               pUnk != NULL &&
               !m_bFilterFinished)
        {
            CComPtr<IXpsDocument> pXD(NULL);
            CComPtr<IFixedDocumentSequence> pFDS(NULL);
            CComPtr<IFixedDocument> pFD(NULL);
            CComPtr<IFixedPage> pFP(NULL);

            //
            // Query interface to find the part type and pass to the
            // appropriate part handler
            //
            if (SUCCEEDED(pUnk.QueryInterface(&pXD)))
            {
                hr = ProcessPart(pXD);
            }
            else if (SUCCEEDED(pUnk.QueryInterface(&pFDS)))
            {
                hr = ProcessPart(pFDS);
            }
            else if (SUCCEEDED(pUnk.QueryInterface(&pFD)))
            {
                hr = ProcessPart(pFD);
            }
            else if (SUCCEEDED(pUnk.QueryInterface(&pFP)))
            {
                hr = ProcessPart(pFP);
            }
            else
            {
                //
                // Unrecognised part - send as unknown.
                //
                hr = m_pXDWriter->SendXpsUnknown(pUnk);
            }

            //
            // Must call release since pUnk is declared outside of the while loop
            //
            pUnk.Release();
        }

        if (SUCCEEDED(hr))
        {
            //
            // Call finalize letting derived classes know we have
            // processed all parts
            //
            hr = Finalize();
        }

        //
        // Close the xps package consumer
        //
        m_pXDWriter->CloseSender();
    }

    //
    // If the filter failed make sure we shutdown the pipeline
    //
    if (FAILED(hr))
    {
        if (m_bFilterFinished)
        {
            //
            // Filter is already closing down so report S_OK
            //
            hr = S_OK;
        }
        else
        {
            //
            // Request the pipeline manager shutdown the filter
            //
            ERR("Requesting filter shutdown\n");
            RequestShutdown(hr);
        }
    }

    //
    // Let the filter pipe manager know the filter is finished
    //
    if (bDoCoUninitialize)
    {
        CoUninitialize();
    }

    FilterFinished();

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDXpsFilter::ProcessPart

Routine Description:

    This routine is the default XPS document part handler

Arguments:

    pXD - Pointer to the IXPSDocument interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::ProcessPart(
    _Inout_ IXpsDocument* pXD
    )
{
    VERBOSE("Processing XPS Document part with default handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pXD, E_POINTER)))
    {
        hr = m_pXDWriter->SendXpsDocument(pXD);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDXpsFilter::ProcessPart

Routine Description:

    This routine is the default fixed document sequence part handler

Arguments:

    pFDS - Pointer to the IFixedDocumentSequence interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::ProcessPart(
    _Inout_ IFixedDocumentSequence* pFDS
    )
{
    VERBOSE("Processing Fixed Document Sequence part with default handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFDS, E_POINTER)))
    {
        //
        // Default handler: Set the PT and send the doc sequence
        //
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFDS)))
        {
            hr = m_pXDWriter->SendFixedDocumentSequence(pFDS);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDXpsFilter::ProcessPart

Routine Description:

    This routine is the default fixed document part handler

Arguments:

    pFD - Pointer to the IFixedDocument interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::ProcessPart(
    _Inout_ IFixedDocument* pFD
    )
{
    VERBOSE("Processing Fixed Document part with default handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFD, E_POINTER)))
    {
        //
        // Default handler: Set the PT and send the doc
        //
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFD)))
        {
            hr = m_pXDWriter->SendFixedDocument(pFD);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDXpsFilter::ProcessPart

Routine Description:

    This routine is the default fixed page handler

Arguments:

    pFP - Pointer to the IFixedPage interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::ProcessPart(
    _Inout_ IFixedPage* pFP
    )
{
    VERBOSE("Processing Fixed Page part with default handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)))
    {
        //
        // Default handler: No ticket required, just send the page
        //
        hr = m_pXDWriter->SendFixedPage(pFP);
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CXDXpsFilter::Finalize

Routine Description:

    This method is the default finalize method called when all parts in the XPS document
    have been processed

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::Finalize(
    VOID
    )
{
    return S_OK;
}

/*++

Routine Name:

    CXDXpsFilter::InitialiseXDIO

Routine Description:

    This routine initialises the XPS producer and consumer interfaces used by
    all XPS document filters

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDXpsFilter::InitialiseXDIO(
    VOID
    )
{
    VERBOSE("Retrieving Xps producer and consumer.\n");

    HRESULT hr = S_OK;

    //
    // Ensure the produver and consumer are released
    //
    m_pXDReader = NULL;
    m_pXDWriter = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pInterFltrComm, E_PENDING)))
    {
        //
        // Get the producer and consumer from the filter communicator
        //
        if (SUCCEEDED(hr = m_pInterFltrComm->RequestReader(reinterpret_cast<VOID**>(&m_pXDReader))))
        {
            hr = m_pInterFltrComm->RequestWriter(reinterpret_cast<VOID**>(&m_pXDWriter));
        }

        //
        // If anything went wrong, ensure the produver and consumer are released
        //
        if (FAILED(hr))
        {
            m_pXDReader = NULL;
            m_pXDWriter = NULL;
        }
    }

    //
    // Check interface is as expected. If not then it is likely that
    // the wrong GUID has been defined in the filter configuration file
    //
    if (SUCCEEDED(hr))
    {
        CComPtr<IXpsDocumentProvider> pReaderCheck(NULL);
        CComPtr<IXpsDocumentConsumer> pWriterCheck(NULL);

        if (FAILED(m_pXDReader.QueryInterface(&pReaderCheck)) ||
            FAILED(m_pXDWriter.QueryInterface(&pWriterCheck)))
        {
            RIP("Invalid reader and writer defined - check GUIDs in the filter configuration file\n");

            //
            // Request the pipeline manager shutsdown the filter
            //
            hr = E_FAIL;
            RequestShutdown(hr);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

