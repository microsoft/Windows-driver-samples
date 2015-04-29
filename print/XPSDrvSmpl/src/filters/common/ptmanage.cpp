/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ptmanage.cpp

Abstract:

   PrintTicket management class implementation. This class encapsulate
   PrintTicket handling algorithm defined in the XPS Document specification.
   It provides a simple set and get interace for to filters and handles
   merging of tickets and the use of the Win32 PrintTicket provider API.
   The algorithm for determining the PrintTicket applies as follows:

   1. Validate and merge the PrintTicket from the FDS with the default
      printicket converted from the property bag.
      The resultant ticket will be the Job level ticket.

   2. Validate and merge the PrintTicket from the current FD with the Job
      level ticket from step 1. The resultant ticket will be the document
      level ticket.

   3. Validate and merge the PrintTicket from the current FP with the Doc
      level ticket from step 2. The resultant ticket will be the page
      level ticket.


--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "ptmanage.h"
#include "streamcnv.h"

/*++

Routine Name:

    CPTManager::CPTManager

Routine Description:

    CPTManager class constructor

Arguments:

    None

Return Value:

    None

--*/
CPTManager::CPTManager() :
    m_pDefaultPT(NULL),
    m_pJobPT(NULL),
    m_pDocPT(NULL),
    m_pPagePT(NULL),
    m_hProvider(NULL),
    m_hToken(INVALID_HANDLE_VALUE)
{
}

/*++

Routine Name:

    CPTManager::~CPTManager

Routine Description:

    CPTManager class destructor

Arguments:

    None

Return Value:

    None

--*/
CPTManager::~CPTManager()
{
    //
    // Close the PT provider
    //
    CloseProvider();
}

/*++

Routine Name:

    CPTManager::Initialise

Routine Description:

    This routine Initialises the PrintTicket manager with the default PrintTicket
    and the device name.

Arguments:

    pDefaultTicketStream - Pointer to the default user PrintTicket as an IStream
    bstrPrinterName      - The name of the printer
    userToken            - The user's security token

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::Initialise(
    _In_   IPrintReadStream* pDefaultTicketStream,
    _In_z_ BSTR              bstrPrinterName,
    _In_   HANDLE            userToken
    )
{
    HRESULT hr = S_OK;

    m_hToken = userToken;

    //
    // If the provider is already open close it
    //
    CloseProvider();

    if (SUCCEEDED(hr = CHECK_POINTER(pDefaultTicketStream, E_POINTER)))
    {
        if (SysStringLen(bstrPrinterName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // We need to impersonate the user who submitted the job
    // in order to always have sufficient rights to call
    // PTOpenProvider.
    //
    if (SUCCEEDED(hr))
    {
        if (SetThreadToken(NULL, m_hToken))
        {
            //
            // Open the PT interface and initialise PrintTickets
            //
            if (SUCCEEDED(hr = PTOpenProvider(bstrPrinterName, 1, &m_hProvider)))
            {
                CComPtr<IStream> pPTStream(NULL);
                pPTStream.Attach(new(std::nothrow) CPrintReadStreamToIStream(pDefaultTicketStream));

                if (SUCCEEDED(hr = CHECK_POINTER(pPTStream, E_OUTOFMEMORY)))
                {
                    hr = InitialisePrintTickets(pPTStream);
                }
            }

            //
            // Always revert back to the default security context
            //
            if (!SetThreadToken(NULL, NULL))
            {
                //
                // We couldn't revert the security context. The filter pipeline
                // manager will clean up the thread when operation is complete,
                // when it is determined that the security context was not 
                // reverted. Since there are no security implications with 
                // running this filter in an elevated context, we can 
                // continue to run.
                //
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::SetTicket

Routine Description:

    This routine sets the Job level PrintTicket from the PrintTicket held in the
    FixedDocumentSequence. If there is no PrintTicket associated with the FDS, the
    default PrintTicket is used to set the Job level PrintTicket.

Arguments:

    pFDS - Pointer to the FixedDocumentSequence interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetTicket(
    _In_ CONST IFixedDocumentSequence* pFDS
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFDS, E_POINTER)))
    {
        CComPtr<IPartPrintTicket> pPTRef(NULL);

        //
        // If a PrintTicket is available merge it at the job level
        //
        if (SUCCEEDED(hr = const_cast<IFixedDocumentSequence*>(pFDS)->GetPrintTicket(&pPTRef)))
        {
            hr = MergeTicket(kPTJobScope, pPTRef);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            //
            // This indicates there is no print-ticket associated with
            // the job. Free the tickets from this scope upward and
            // reintialise the defaults
            //
            FreePrintTickets(kPTJobScope);
            hr = UpdateDefaultPTs(kPTJobScope);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::SetTicket

Routine Description:

    This routine sets the Socument level PrintTicket from the PrintTicket held in the
    FixedDocument. If there is no PrintTicket associated with the FD, the
    FixedDocumentSequence PrintTicket is used to set the Docuemnt level PrintTicket.

Arguments:

    pFD - Pointer to the FixedDocument interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetTicket(
    _In_ CONST IFixedDocument* pFD
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFD, E_POINTER)))
    {
        CComPtr<IPartPrintTicket> pPTRef(NULL);

        //
        // If a PrintTicket is available merge it at the doc level
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = const_cast<IFixedDocument*>(pFD)->GetPrintTicket(&pPTRef)))
        {
            hr = MergeTicket(kPTDocumentScope, pPTRef);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            //
            // This indicates there is no print-ticket associated with
            // the job. Free the tickets from this scope upward and
            // reintialise the defaults
            //
            FreePrintTickets(kPTDocumentScope);
            hr = UpdateDefaultPTs(kPTDocumentScope);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::SetTicket

Routine Description:

    This routine sets the Page level PrintTicket from the PrintTicket held in the
    FixedPage. If there is no PrintTicket associated with the FP, the
    FixedDocument PrintTicket is used to set the Page level PrintTicket.

Arguments:

    pFP - Pointer to the FixedPage interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetTicket(
    _In_ CONST IFixedPage* pFP
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)))
    {
        CComPtr<IPartPrintTicket> pPTRef(NULL);

        //
        // If a PrintTicket is available merge it at the page level
        //
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = const_cast<IFixedPage*>(pFP)->GetPrintTicket(&pPTRef)))
        {
            hr = MergeTicket(kPTPageScope, pPTRef);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            //
            // This indicates there is no print-ticket associated with
            // the job. Free the tickets from this scope upward and
            // reintialise the defaults
            //
            FreePrintTickets(kPTPageScope);
            hr = UpdateDefaultPTs(kPTPageScope);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CPTManager::SetTicket

Routine Description:

    Sets the PrintTicket at the specified scope given a PrintTicket defined
    as a DOM document

Arguments:

    ptScope - The scope of the PrintTicket to be set
    pPT     - Pointer to an IXMLDOMDocument2 interface that contains the PrintTicket to be set

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetTicket(
    _In_     CONST EPrintTicketScope ptScope,
    _In_opt_ CONST IXMLDOMDocument2* pPT
    )
{
    HRESULT hr = S_OK;

    //
    // Free the tickets from this scope upward and reintialise
    // from the appropriate default. This ensures we are not
    // using an old PrintTicket from the same scope
    //
    FreePrintTickets(ptScope);

    if (SUCCEEDED(hr = UpdateDefaultPTs(ptScope)))
    {
        if (pPT)
        {
            hr = MergeTicket(ptScope, pPT);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::GetTicket

Routine Description:

    This routine retrieves the PrintTicket at the requested scope

Arguments:

    ptScope  - The scope of the PrintTicket to be retrieved
    ppTicket - Pointer to an IXMLDOMDocument2 interface pointer that recieves the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::GetTicket(
    _In_        CONST EPrintTicketScope ptScope,
    _Outptr_ IXMLDOMDocument2**      ppTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppTicket, E_POINTER)) &&
        SUCCEEDED(hr = UpdateDefaultPTs(ptScope)))
    {
        *ppTicket = NULL;

        switch (ptScope)
        {
            case kPTPageScope:
            {
                //
                // Pointer should either be default or FP ticket but never NULL
                //
                ASSERTMSG(m_pPagePT != NULL, "NULL Page PrintTicket.\n");

                if (SUCCEEDED(hr = CHECK_POINTER(m_pPagePT, E_PENDING)))
                {
                    *ppTicket = m_pPagePT;
                }
            }
            break;

            case kPTDocumentScope:
            {
                //
                // Pointer should either be default or FD ticket but never NULL
                //
                ASSERTMSG(m_pDocPT != NULL, "NULL Document PrintTicket.\n");

                if (SUCCEEDED(hr = CHECK_POINTER(m_pDocPT, E_PENDING)))
                {
                    *ppTicket = m_pDocPT;
                }
            }
            break;

            case kPTJobScope:
            {
                //
                // Pointer should either be default or FDS ticket but never NULL
                //
                ASSERTMSG(m_pJobPT != NULL, "NULL Job PrintTicket.\n");

                if (SUCCEEDED(hr = CHECK_POINTER(m_pJobPT, E_PENDING)))
                {
                    *ppTicket = m_pJobPT;
                }
            }
            break;

            default:
            {
                RIP("Unrecognised PT scope\n");

                hr = E_INVALIDARG;
            }
            break;
        }

        if (*ppTicket == NULL)
        {
            RIP("Failed to retrieve a valid PrintTicket\n");

            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CPTManager::GetCapabilities

Routine Description:

    This routine retrieves a PrintCapabilities document given a PrintTicket

Arguments:

    pTicket - Pointer to the PrintTicket as a DOM document
    pTicket - Pointer to a DOM document pointer that recieves the PrintCapabilities

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::GetCapabilities(
    _In_        IXMLDOMDocument2*  pTicket,
    _Outptr_ IXMLDOMDocument2** ppCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pTicket, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppCapabilities, E_POINTER)))
    {
        *ppCapabilities = NULL;

        CComBSTR bstrError;
        CComPtr<IStream> pPTIn(NULL);
        CComPtr<IStream> pPCOut(NULL);
        CComPtr<IXMLDOMDocument2> pCapabilitiesDoc(NULL);

        //
        // Create the PrintCapabilities DOM document, retrieve the IStreams from the
        // DOM documents and call the PT api to retrieve the capabilities document
        //
        if (SUCCEEDED(hr = pCapabilitiesDoc.CoCreateInstance(CLSID_DOMDocument60)) &&
            SUCCEEDED(hr = pTicket->QueryInterface(IID_IStream, reinterpret_cast<VOID**>(&pPTIn))) &&
            SUCCEEDED(hr = pCapabilitiesDoc->QueryInterface(IID_IStream, reinterpret_cast<VOID**>(&pPCOut))))
        {
            if (SetThreadToken(NULL, m_hToken))
            {
                if (SUCCEEDED(hr = PTGetPrintCapabilities(m_hProvider, pPTIn, pPCOut, &bstrError)))
                {
                    LARGE_INTEGER cbMove = {0};
                    if (SUCCEEDED(hr = pPCOut->Seek(cbMove, STREAM_SEEK_SET, NULL)))
                    {
                        *ppCapabilities = pCapabilitiesDoc.Detach();
                    }
                }
                else
                {
                    try
                    {
                        CStringXDA cstrError(bstrError);
                        ERR(cstrError.GetBuffer());
                    }
                    catch (CXDException&)
                    {
                    }
                }
            
                //
                // Always revert back to the default security context
                //
                if (!SetThreadToken(NULL, NULL))
                {
                    //
                    // We couldn't revert the security context. The filter pipeline
                    // manager will clean up the thread when operation is complete,
                    // when it is determined that the security context was not 
                    // reverted. Since there are no security implications with 
                    // running this filter in an elevated context, we can 
                    // continue to run.
                    //
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());

                //
                // If SetThreadToken fails, GetLastError will return an error
                //
                _Analysis_assume_(FAILED(hr));
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::InitialisePrintTickets

Routine Description:

    This routine intialises the PrintTickets at the default, job, document and page scope
    using an IStream containing the default PrintTicket mark-up

Arguments:

    pDefaultPTStream - Pointer to an IStream interface that contains the default PrintTicket data

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::InitialisePrintTickets(
    _In_ IStream* pDefaultPTStream
    )
{
    ASSERTMSG(m_hProvider != NULL, "NULL  PrintTicket provider interface detected.\n");

    HRESULT hr = S_OK;

    //
    // Make sure all print tickets are released
    //
    m_pDefaultPT = NULL;
    m_pJobPT = NULL;
    m_pDocPT = NULL;
    m_pPagePT = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pDefaultPTStream, E_POINTER)))
    {
        //
        // Create the DOM documents for default PT
        //
        hr = SetPTFromStream(pDefaultPTStream, &m_pDefaultPT);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::SetPTFromDOMDoc

Routine Description:

    This routine copies one DOM document representation of the PrintTicket to another

Arguments:

    pPTDOMDoc - Pointer to the source PrintTicket as a DOM document
    ppDomDoc  - Pointer to an IXMLDOMDOcument pointer that recieves the copied PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetPTFromDOMDoc(
    _In_        IXMLDOMDocument2*  pPTDOMDoc,
    _Outptr_ IXMLDOMDocument2** ppDomDoc
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPTDOMDoc, E_POINTER)))
    {
        //
        // Get the IStream from the DOM doc and pass to SetPTFromStream
        //
        CComPtr<IStream> pStream(NULL);
        if (SUCCEEDED(hr = pPTDOMDoc->QueryInterface(IID_IStream, reinterpret_cast<VOID**>(&pStream))))
        {
            hr = SetPTFromStream(pStream, ppDomDoc);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::SetPTFromStream

Routine Description:

    This routine copies an IStream representation of the PrintTicket to a DOM document

Arguments:

    pPTStream - Pointer to the source PrintTicket as an IStream
    ppDomDoc  - Pointer to an IXMLDOMDOcument pointer that recieves the copied PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::SetPTFromStream(
    _In_        IStream*           pPTStream,
    _Outptr_ IXMLDOMDocument2** ppDomDoc
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPTStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppDomDoc, E_POINTER)))
    {
        *ppDomDoc = NULL;

        LARGE_INTEGER cbMoveFromStart = {0};
        CComPtr<IXMLDOMDocument2> pDOMDoc(NULL);
        VARIANT_BOOL fLoaded = VARIANT_FALSE;

        //
        // Create the DOM document instance, seek to the start of the
        // PT stream and load into the dom document
        //
        if (SUCCEEDED(hr = pDOMDoc.CoCreateInstance(CLSID_DOMDocument60)) &&
            SUCCEEDED(hr = pPTStream->Seek(cbMoveFromStart, STREAM_SEEK_SET, NULL)) &&
            SUCCEEDED(hr = pDOMDoc->load(CComVariant(pPTStream), &fLoaded)))
        {
            if (fLoaded == VARIANT_TRUE)
            {
                //
                // Assign the outgoing element pointer - detach from CComPtr to release ownership
                //
                *ppDomDoc = pDOMDoc.Detach();
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::MergeTicket

Routine Description:

    This routine merges a PrintTicket (supplied as a IPartPrintTicket pointer) into
    the DOM representation of the PrintTicket at the requested scope

Arguments:

    ptScope - The scope at which the PrintTicket is to be merged
    pPTRef  - Pointer to an IPartPrintTicket interface containing the PrintTicket to be merged

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::MergeTicket(
    _In_ CONST EPrintTicketScope ptScope,
    _In_ CONST IPartPrintTicket* pPTRef
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPTRef, E_POINTER)))
    {
        //
        // Create a DOM document from the PT ref stream
        //
        CComPtr<IXMLDOMDocument2> pNewPT(NULL);
        CComPtr<IPrintReadStream> pRead(NULL);

        if (SUCCEEDED(hr = pNewPT.CoCreateInstance(CLSID_DOMDocument60)) &&
            SUCCEEDED(hr = const_cast<IPartPrintTicket*>(pPTRef)->GetStream(&pRead)))
        {
            CComPtr<ISequentialStream> pReadStreamToSeq(NULL);
            VARIANT_BOOL fLoaded = VARIANT_FALSE;

            pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pRead));

            if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)) &&
                SUCCEEDED(hr = pNewPT->load(CComVariant(pReadStreamToSeq), &fLoaded)))
            {
                if (fLoaded == VARIANT_TRUE)
                {
                    hr = MergeTicket(ptScope, pNewPT);
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::MergeTicket

Routine Description:

    This routine merges a PrintTicket (supplied as a IXMLDOMDocument2 pointer) into
    the DOM representation of the PrintTicket at the requested scope

Arguments:

    ptScope - The scope at which the PrintTicket is to be merged
    pPT     - Pointer to an IXMLDOMDocument2 interface containing the PrintTicket to be merged

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::MergeTicket(
    _In_ CONST EPrintTicketScope ptScope,
    _In_ CONST IXMLDOMDocument2* pPT
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPT, E_POINTER)))
    {
        CComPtr<IXMLDOMDocument2> pPTLevelUp(NULL);
        CComPtr<IXMLDOMDocument2> pResult(NULL);

        switch (ptScope)
        {
            case kPTJobScope:
            {
                pPTLevelUp = m_pDefaultPT;
            }
            break;

            case kPTDocumentScope:
            {
                pPTLevelUp = m_pJobPT;
            }
            break;

            case kPTPageScope:
            {
                pPTLevelUp = m_pDocPT;
            }
            break;

            default:
            {
                RIP("Unrecognised PT scope\n");

                hr = E_INVALIDARG;
            }
            break;
        }

        ASSERTMSG(pPTLevelUp != NULL, "PrintTicket at the level up has not been set (check part handler is attempting to)\n");

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CHECK_POINTER(pPTLevelUp, E_PENDING)) &&
            SUCCEEDED(hr = GetMergedTicket(ptScope, pPT, pPTLevelUp, &pResult)))
        {
            switch (ptScope)
            {
                case kPTJobScope:
                {
                    m_pJobPT = pResult;
                }
                break;

                case kPTDocumentScope:
                {
                    m_pDocPT = pResult;
                }
                break;

                case kPTPageScope:
                {
                    m_pPagePT = pResult;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::GetMergedTicket

Routine Description:

    This routine retrieves a merged PrintTicket at the requested scope given the base
    and delta PrintTickets as IXMLDOMDocument2 interface pointers

Arguments:

    ptScope  - The scope at which the merge should take place
    pDelta   - The delta PrintTicket as an IXMLDOMDocument2 pointer
    pBase    - The base PrintTicket as an IXMLDOMDocument2 pointer
    ppResult - The resulting PrintTicket after the merge has completed

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::GetMergedTicket(
    _In_        CONST EPrintTicketScope ptScope,
    _In_        CONST IXMLDOMDocument2* pDelta,
    _In_        IXMLDOMDocument2*       pBase,
    _Outptr_ IXMLDOMDocument2**      ppResult
    )
{
    ASSERTMSG(pDelta != NULL, "NULL PT reference part passed.\n");
    ASSERTMSG(pBase != NULL, "NULL base PT passed.\n");
    ASSERTMSG(ppResult != NULL, "NULL out PT passed.\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_HANDLE(m_hProvider, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppResult, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDelta, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pBase, E_POINTER)))
    {
        *ppResult = NULL;
    }

    CComPtr<IStream> pResultStream(NULL);
    CComPtr<IStream> pBaseStream(NULL);
    CComPtr<IStream> pDeltaStream(NULL);

    CComPtr<IXMLDOMDocument2> pNewPT(NULL);

    //
    // Create a new DOM doc based off the result ticket and
    // assign to the resultant DOM doc
    //
    try
    {
        CComBSTR bstrErrorMessage;

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pResultStream)) &&
            SUCCEEDED(hr = pBase->QueryInterface(IID_IStream, reinterpret_cast<VOID**>(&pBaseStream)))  &&
            SUCCEEDED(hr = const_cast<IXMLDOMDocument2*>(pDelta)->QueryInterface(IID_IStream, reinterpret_cast<VOID**>(&pDeltaStream))))
        {
            if (SetThreadToken(NULL, m_hToken))
            {
                if (SUCCEEDED(hr = PTMergeAndValidatePrintTicket(m_hProvider,
                                                                 pBaseStream,
                                                                 pDeltaStream,
                                                                 ptScope,
                                                                 pResultStream,
                                                                 &bstrErrorMessage)))
                {
                    if (SUCCEEDED(hr = SetPTFromStream(pResultStream, &pNewPT)))
                    {
                        //
                        // Assign the outgoing element pointer - detach from CComPtr to release ownership
                        //
                        *ppResult = pNewPT.Detach();
                    }
                }
                else
                {
                    CStringXDA cstrMessage;
                    CStringXDA cstrError(bstrErrorMessage);
                    cstrMessage.Format("PTMergeAndValidatePrintTicket failed with message: %s\n", cstrError);

                    ERR(cstrMessage.GetBuffer());
                }

                //
                // Always revert back to the default security context
                //
                if (!SetThreadToken(NULL, NULL))
                {
                    //
                    // We couldn't revert the security context. The filter pipeline
                    // manager will clean up the thread when operation is complete,
                    // when it is determined that the security context was not 
                    // reverted. Since there are no security implications with 
                    // running this filter in an elevated context, we can 
                    // continue to run.
                    //
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());

                //
                // If SetThreadToken fails, GetLastError will return an error
                //
                _Analysis_assume_(FAILED(hr));
            }
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::CloseProvider

Routine Description:

    This routine closes the PrintTicket provider

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::CloseProvider(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (m_hProvider != NULL)
    {
        hr = PTCloseProvider(m_hProvider);
        m_hProvider = NULL;
    }

    return hr;
}

/*++

Routine Name:

    CPTManager::UpdateDefaultPTs

Routine Description:

    This routine updates the default PrintTicket at a given scope. This routine updates
    the Job PrintTicket with the default, the Document with the Job and the Page with the
    Document if they are not set and are within the requested scope.

Arguments:

    ptScope - The scope of the PrintTicket to update

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTManager::UpdateDefaultPTs(
    _In_ CONST EPrintTicketScope ptScope
    )
{
    HRESULT hr = S_OK;

    //
    // We need at least the default ticket in place
    //
    ASSERTMSG(m_pDefaultPT != NULL, "The default PrintTicket is not correctly set\n");

    if (SUCCEEDED(hr = CHECK_POINTER(m_pDefaultPT, E_PENDING)))
    {
        if (ptScope <= kPTJobScope &&
            m_pJobPT == NULL)
        {
            hr = SetPTFromDOMDoc(m_pDefaultPT, &m_pJobPT);
        }

        if (SUCCEEDED(hr) &&
            ptScope <= kPTDocumentScope &&
            m_pDocPT == NULL)
        {
            hr = SetPTFromDOMDoc(m_pJobPT, &m_pDocPT);
        }

        if (SUCCEEDED(hr) &&
            ptScope == kPTPageScope &&
            m_pPagePT == NULL)
        {
            hr = SetPTFromDOMDoc(m_pDocPT, &m_pPagePT);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTManager::FreePrintTickets

Routine Description:

    This routine releases the PrintTicket at a given scope and all PrintTickets
    that depend on it.

Arguments:

    ptScope - The scope from which the PrintTickets should be released

Return Value:

    None

--*/
VOID
CPTManager::FreePrintTickets(
    _In_ CONST EPrintTicketScope ptScope
    )
{
    if (ptScope >= kPTPageScope)
    {
        m_pPagePT = NULL;
    }

    if (ptScope >= kPTDocumentScope)
    {
        m_pDocPT = NULL;
    }

    if (ptScope >= kPTJobScope)
    {
        m_pJobPT = NULL;
    }
}

