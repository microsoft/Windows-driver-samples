//
// File Name:
//
//    PThandler.cpp
//
// Abstract:
//
//    Print Ticket Handler class definition.
//

#include "precomp.h"
#include "WppTrace.h"
#include "CustomWppCommands.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "OMConvertor.h"
#include "RenderFilter.h"
#include "PTHandler.h"

#include "PThandler.tmh"

namespace v4PrintDriver_Render_Filter
{

//
//Routine Name:
//
//    PrintTicketHandler::CreatePrintTicketHandler
//
//Routine Description:
//
//    Static factory method that creates an instance of
//    PrintTicketHandler.
//
//Arguments:
//
//    pPropertyBag - Property Bag
//
//Return Value:
//
//    PrintTicketHandler_t (smart ptr)
//    The new PrintTicketHandler.
//
PrintTicketHandler_t
PrintTicketHandler::CreatePrintTicketHandler(
    const IPrintPipelinePropertyBag_t &pPropertyBag
    )
{
    //
    // Create MSXML DOM document
    //
    IXMLDOMDocument2_t pDOMDoc;

    THROW_ON_FAILED_HRESULT(
        ::CoCreateInstance(
            __uuidof(DOMDocument60),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IXMLDOMDocument2),
            reinterpret_cast<LPVOID*>(&pDOMDoc)
            )
        );

    //
    // Get the default user Print Ticket Stream Factory
    //
    Variant_t varUserPrintTicket;
    THROW_ON_FAILED_HRESULT(
        pPropertyBag->GetProperty(
            XPS_FP_USER_PRINT_TICKET, 
            &varUserPrintTicket
            )
        );
    IUnknown_t pUnk = varUserPrintTicket.punkVal;

    IPrintReadStreamFactory_t pStreamFactory;

    THROW_ON_FAILED_HRESULT(
        pUnk.QueryInterface(&pStreamFactory)
        );

    //
    // Get the default user Print Ticket stream
    // and wrap it in an IStream
    //

    IPrintReadStream_t pUserPrintTicketStream;

    THROW_ON_FAILED_HRESULT(
        pStreamFactory->GetStream(&pUserPrintTicketStream)
        );

    IStream_t pUserPrintTicket = 
        CreateIStreamFromIPrintReadStream(pUserPrintTicketStream);

    //
    // Get the Printer Name
    //
    Variant_t varPrinterName;
    THROW_ON_FAILED_HRESULT(
        pPropertyBag->GetProperty(
            XPS_FP_PRINTER_NAME, 
            &varPrinterName
            )
        );

    BSTR_t pPrinterName(varPrinterName.bstrVal);

    //
    // Get the User Security Token
    // Avoid CComVariant if getting the XPS_FP_USER_TOKEN property.
    // Please refer to http://go.microsoft.com/fwlink/?LinkID=255534 for detailed information.
    //
    SafeVariant varUserSecurityToken;
    THROW_ON_FAILED_HRESULT(
        pPropertyBag->GetProperty(
            XPS_FP_USER_TOKEN,
            &varUserSecurityToken
            )
        );
    
    //
    // Open the Print Ticket Provider
    //    
    SafeHPTProvider_t pHProvider(
                        new SafeHPTProvider(
                                pPrinterName,
                                varUserSecurityToken.byref
                                )
                        );

    PrintTicketHandler_t toReturn(
                            new PrintTicketHandler(
                                    pDOMDoc,
                                    pHProvider,
                                    pUserPrintTicket
                                    )
                            );

    return toReturn;
}

//
//Routine Name:
//
//    PrintTicketHandler::PrintTicketHandler
//
//Routine Description:
//
//    Constructor for the Print Ticket Handler.
//
//Arguments:
//
//    pDoc        - initialized MSXML DOM document
//    pHProvider   - handle to the Print Ticket Provider
//
PrintTicketHandler::PrintTicketHandler(
    const IXMLDOMDocument2_t    &pDoc,
    SafeHPTProvider_t           pHProvider,
    const IStream_t             &pUserPrintTicket
    ) : m_pDOMDoc(pDoc),
        m_pHProvider(pHProvider),
        m_pDefaultUserPrintTicket(pUserPrintTicket)
{
}

//
//Routine Name:
//
//    PrintTicketHandler::ProcessPrintTicket
//
//Routine Description:
//
//    Gets the print ticket from the part, merges with
//    the base ticket, and returns the result.
//
//Arguments:
//
//    pBasePrintTicket        - Base Print Ticket
//    pDeltaPrintTicketPart   - Delta Print Pipeline Print Ticket Part
//    scope                   - Scope of the merged Print Ticket
//
//Return Value:
//
//    IStream_t (smart pointer)
//    Merged stream
//
IStream_t
PrintTicketHandler::ProcessPrintTicket(
    const IStream_t             &pBasePrintTicket,
    const IPartPrintTicket_t    &pDeltaPrintTicketPart,
    EPrintTicketScope           scope
    )
{
    IStream_t pMergedPrintTicket;
    IStream_t pDeltaPrintTicket;

    pDeltaPrintTicket = GetStreamFromPart(
        static_cast<IPartBase_t>(pDeltaPrintTicketPart)
        );

    //
    // Before calling PTMergeAndValidatePrintTicket, both input
    // Print Ticket streams MUST be at position 0. The temp Print 
    // Ticket stream is already at position 0, but the Base Print 
    // Ticket may not be. Seek it to 0 to be sure.
    //
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    THROW_ON_FAILED_HRESULT(
        pBasePrintTicket->Seek(zero, SEEK_SET, NULL)
        );

    //
    // Merge the delta Print Ticket with the
    // base Print Ticket
    //
    THROW_ON_FAILED_HRESULT(
        ::CreateStreamOnHGlobal(
            NULL,
            TRUE, // delete on release
            &pMergedPrintTicket
            )
        );

    m_pHProvider->PTMergeAndValidatePrintTicket(
                        pBasePrintTicket,
                        pDeltaPrintTicket,
                        scope,
                        pMergedPrintTicket
                        );

    return pMergedPrintTicket;
}

//
//Routine Name:
//
//    PrintTicketHandler::ProcessPart
//
//Routine Description:
//
//    Merges the Fixed Document Sequence Print Ticket with
//    the default user Print Ticket and caches the result.
//
//Arguments:
//
//    pFDS - Fixed Document Sequence part
//
void
PrintTicketHandler::ProcessPart(
    const IFixedDocumentSequence_t    &pFDS
    )
{
    IPartPrintTicket_t pPrintTicket;

    HRESULT hr = pFDS->GetPrintTicket(&pPrintTicket);

    //
    // E_ELEMENT_NOT_FOUND means that this Fixed Document Sequence
    // does not have a Print Ticket. Propagate the Default User
    // Print Ticket.
    // All other failed HRESULTs should be thrown.
    //
    if (hr == E_ELEMENT_NOT_FOUND)
    {
        m_pJobPrintTicket = m_pDefaultUserPrintTicket;
        return;
    }

    THROW_ON_FAILED_HRESULT(hr);

    m_pDocumentPrintTicket = NULL;
    m_pPagePrintTicket = NULL;

    m_pJobPrintTicket = ProcessPrintTicket(
                            m_pDefaultUserPrintTicket,
                            pPrintTicket,
                            kPTJobScope
                            );
}

//
//Routine Name:
//
//    PrintTicketHandler::ProcessPart
//
//Routine Description:
//
//    Merges the Fixed Document Print Ticket with the
//    Fixed Document Sequence Print Ticket and caches
//    the result. The tickets are merged at the Document
//    scope, so the result contains no job-level features.
//
//Arguments:
//
//    pFD - Fixed Document part
//
void
PrintTicketHandler::ProcessPart(
    const IFixedDocument_t    &pFD
    )
{
    IPartPrintTicket_t pPrintTicket;

    HRESULT hr = pFD->GetPrintTicket(&pPrintTicket);

    //
    // E_ELEMENT_NOT_FOUND means that this Fixed Document
    // does not have a Print Ticket. Propagate the Job
    // Print Ticket.
    // All other failed HRESULTs should be thrown.
    //
    if (hr == E_ELEMENT_NOT_FOUND)
    {
        m_pDocumentPrintTicket = m_pJobPrintTicket;
        return;
    }

    THROW_ON_FAILED_HRESULT(hr);

    m_pPagePrintTicket = NULL;

    m_pDocumentPrintTicket = ProcessPrintTicket(
                                m_pJobPrintTicket,
                                pPrintTicket,
                                kPTDocumentScope
                                );
}

//
//Routine Name:
//
//    PrintTicketHandler::ProcessPart
//
//Routine Description:
//
//    Merges the Fixed Page Print Ticket with the
//    Fixed Document Print Ticket and caches the result.
//    The tickets are merged at the Page scope, so the
//    result contains no job-level or document-level features.
//
//Arguments:
//
//    pFP - Fixed Page part
//
void
PrintTicketHandler::ProcessPart(
    const IFixedPage_t    &pFP
    )
{
    IPartPrintTicket_t pPrintTicket;

    HRESULT hr = pFP->GetPrintTicket(&pPrintTicket);

    //
    // E_ELEMENT_NOT_FOUND means that this Fixed Page
    // does not have a Print Ticket. Propagate the Document
    // Print Ticket.
    // All other failed HRESULTs should be thrown.
    //
    if (hr == E_ELEMENT_NOT_FOUND)
    {
        m_pPagePrintTicket = m_pDocumentPrintTicket;
        return;
    }

    THROW_ON_FAILED_HRESULT(hr);

    m_pPagePrintTicket = ProcessPrintTicket(
                            m_pDocumentPrintTicket,
                            pPrintTicket,
                            kPTPageScope
                            );
}


} // namespace v4PrintDriver_Render_Filter
