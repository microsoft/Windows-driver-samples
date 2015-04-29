/*++

Copyright (C) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdstrmflt.cpp

Abstract:

   Base stream filter implementation. This provides stream interface specific
   functionality general to all filters that use the stream interface to process
   fixed pages. The class is responsible copying data from reader to writer in the
   absence of the PK archive handling module, for a default fixed page processing
   function (filters should implement their own to manipulate fixed page markup)
   and for initialising the stream reader and writer.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstrmflt.h"

/*++

Routine Name:

    CXDStreamFilter::CXDStreamFilter

Routine Description:

    CXDStreamFilter class constructor

Arguments:

    None

Return Value:

    None

--*/
CXDStreamFilter::CXDStreamFilter() :
    m_pStreamReader(NULL),
    m_pStreamWriter(NULL)
{
    VERBOSE("Constructing stream filter\n");
}

/*++

Routine Name:

    CXDStreamFilter::~CXDStreamFilter

Routine Description:

    CXDStreamFilter clsas destructor

Arguments:

    None

Return Value:

    None

--*/
CXDStreamFilter::~CXDStreamFilter()
{
    VERBOSE("Destroying stream filter\n");
}

/*++

Routine Name:

    CXDStreamFilter::StartOperation

Routine Description:

    This is the stream interface implementation of IPrintPipelineFilter::StartOperation
    shared by all stream interface filters

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDStreamFilter::StartOperation(
    VOID
    )
{
    VERBOSE("Starting stream filter operation.\n");

    HRESULT hr = S_OK;
    BOOL bDoCoUninitialize = FALSE;

    if (SUCCEEDED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        bDoCoUninitialize = TRUE;
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = InitialiseStreamIO()) &&
        SUCCEEDED(hr = InitializePrintTicketManager()))
    {
        try
        {
            CXPSProcessor* pXpsProcessor = new(std::nothrow) CXPSProcessor(m_pStreamReader, m_pStreamWriter, this, m_pPrintPropertyBag, &m_ptManager);

            if (SUCCEEDED(hr = CHECK_POINTER(pXpsProcessor, E_OUTOFMEMORY)))
            {
                hr = pXpsProcessor->Start();
            }

            if (pXpsProcessor != NULL)
            {
                delete pXpsProcessor;
                pXpsProcessor = NULL;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (...)
        {
            hr = E_FAIL;
        }

        if (hr == E_NOINTERFACE)
        {
            //
            // The PK archive handler is missing - just pass the data on
            //

            if (SUCCEEDED(hr = CHECK_POINTER(m_pStreamReader, E_POINTER)) &&
                SUCCEEDED(hr = CHECK_POINTER(m_pStreamWriter, E_POINTER)))
            {
                ULONG cbRead = 0;
                BOOL bEOF = FALSE;

                PBYTE pBuff = new(std::nothrow) BYTE[CB_COPY_BUFFER];

                if (SUCCEEDED(hr = CHECK_POINTER(pBuff, E_OUTOFMEMORY)))
                {
                    do
                    {
                        if (SUCCEEDED(hr = m_pStreamReader->ReadBytes(pBuff, CB_COPY_BUFFER, &cbRead, &bEOF)))
                        {
                            ULONG cbWritten = 0;
                            hr = m_pStreamWriter->WriteBytes(pBuff, cbRead, &cbWritten);
                        }
                    }
                    while (SUCCEEDED(hr) &&
                           !bEOF &&
                           cbRead > 0);

                    delete[] pBuff;
                    pBuff = NULL;
                }
            }
        }
    }

    if (m_pStreamWriter)
    {
        m_pStreamWriter->Close();
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

    CXDStreamFilter::ProcessFixedPage

Routine Description:

    This routine is the default implemenation of the fixed page processor called by
    the XPS processor object.

Arguments:

    pFPPT            - Pointer to the fixed page PrintTicket
    pPageReadStream  - Pointer to the page read stream
    pPageWriteStream - Pointer to the page write stream

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDStreamFilter::ProcessFixedPage(
    _In_  IXMLDOMDocument2*  pFPPT,
    _In_  ISequentialStream* pPageReadStream,
    _In_  ISequentialStream* pPageWriteStream
    )
{
    VERBOSE("Processing stream fixed page with default handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFPPT, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPageReadStream, E_POINTER)))
    {
        hr = CHECK_POINTER(pPageWriteStream, E_POINTER);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDStreamFilter::InitialiseStreamIO

Routine Description:

    This routine initialises the print stream read and write interfaces used by
    all stream filters

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDStreamFilter::InitialiseStreamIO(
    VOID
    )
{
    VERBOSE("Retrieving stream reader and writer.\n");

    HRESULT hr = S_OK;

    //
    // Make sure the reader and writer are released
    //
    m_pStreamReader = NULL;
    m_pStreamWriter = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pInterFltrComm, E_PENDING)))
    {
        //
        // Get the reader and writer from the filter communicator
        //
        if (SUCCEEDED(hr = m_pInterFltrComm->RequestReader(reinterpret_cast<VOID**>(&m_pStreamReader))))
        {
            hr = m_pInterFltrComm->RequestWriter(reinterpret_cast<VOID**>(&m_pStreamWriter));
        }

        //
        // If anything went wrong, make sure the reader and writer
        // have been released
        //
        if (FAILED(hr))
        {
            m_pStreamReader = NULL;
            m_pStreamWriter = NULL;
        }
    }

    //
    // Check interface is as expected. If not then it is likely that
    // the wrong GUID has been defined in the filter configuration file
    //
    if (SUCCEEDED(hr))
    {
        CComPtr<IPrintReadStream>  pReaderCheck(NULL);
        CComPtr<IPrintWriteStream> pWriterCheck(NULL);

        if (FAILED(m_pStreamReader.QueryInterface(&pReaderCheck)) ||
            FAILED(m_pStreamWriter.QueryInterface(&pWriterCheck)))
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

