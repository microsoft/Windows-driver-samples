// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
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
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "OMConvertor.h"
#include "rasinterface.h"
#include "xpsrasfilter.h"
#include "PTHandler.h"

#include "PThandler.tmh"

namespace xpsrasfilter
{

//
// This is an arbitrary page margin that is used to simulate
// an imageable area for scaling calculations.
//
const FLOAT g_pageMargin = 0.25f;

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

//
//Routine Name:
//
//    PrintTicketHandler::GetMergedPrintTicketParams
//
//Routine Description:
//
//    Queries a set of parameters from the merged print ticket.
//
//    NOTE: Relies on successful calls to all three PrintTicket
//    processing methods.
//
//Return Value:
//
//    ParametersFromPrintTicket
//    The set of parameters queried from the merged
//    Print Ticket.
//
ParametersFromPrintTicket
PrintTicketHandler::GetMergedPrintTicketParams()
{
    DoTraceMessage(XPSRASFILTER_TRACE_VERBOSE, L"Getting Print Ticket parameters");

    if (!m_pPagePrintTicket)
    {
        DoTraceMessage(XPSRASFILTER_TRACE_ERROR, L"GetMergedPrintTicketParams called before ProcessPagePrintTicket");
        THROW_ON_FAILED_HRESULT(E_FAIL);
    }

    ParametersFromPrintTicket params;

    //
    // Seek the Page-level Print Ticket to 0
    //
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    THROW_ON_FAILED_HRESULT(
        m_pPagePrintTicket->Seek(zero, SEEK_SET, NULL)
        );

    //
    // Load the print ticket stream into a DOM document
    //
    // NOTE: We are only looking for features at the Page scope for this sample,
    // so we only extract features from the effective page-level PrintTicket.
    //
    Variant_t varStream(m_pPagePrintTicket);
    VARIANT_BOOL success;

    THROW_ON_FAILED_HRESULT(
        m_pDOMDoc->load(varStream, &success)
        );
    if (!success)
    {
        WPP_LOG_ON_FAILED_HRESULT(E_FAIL);
        THROW_ON_FAILED_HRESULT(E_FAIL);
    }

    //
    // Set the DOM Selection namespace to simplify queries
    //
    BSTR_t ns(L"xmlns:psf='http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework'");
    Variant_t nsProp(ns);
    THROW_ON_FAILED_HRESULT(
        m_pDOMDoc->setProperty(L"SelectionNamespaces", nsProp)
        );

    //
    // Query the print ticket for parameters of interest
    //
    params.destDPI =            QueryDPI();
    params.scaling =            QueryScaling();
    params.physicalPageSize =   QueryPhysicalPageSize();
    
    //
    // We simulate imageable area by assuming a constant margin of
    // around the entire page
    //
    params.imageableArea.x =        g_pageMargin * xpsDPI;
    params.imageableArea.y =        g_pageMargin * xpsDPI;
    params.imageableArea.height =   params.physicalPageSize.height 
                                        - 2 * g_pageMargin * xpsDPI;
    params.imageableArea.width =    params.physicalPageSize.width 
                                        - 2 * g_pageMargin * xpsDPI;

    return params;
}

//
//Routine Name:
//
//    PrintTicketHandler::QueryDPI
//
//Routine Description:
//
//    Queries the DPI from the Print Ticket using an XPath query.
//
//Return Value:
//
//    FLOAT
//    DPI from the Print Ticket.
//
FLOAT
PrintTicketHandler::QueryDPI()
{
    //
    // Default DPI: 96 DPI
    //
    FLOAT dpi = 96.0;

    //
    // Perform the page resolution query on the print ticket
    //
    IXMLDOMNodeList_t pNodes;

    //
    // The following XPath query is fairly straightforward, except for the
    // predicate attached to Feature. This is necessary to match both the
    // keyword AND the namespace of the "name" of the Feature as in:
    //
    //  <psf:Feature name="psk:PageResolution">
    //
    // In order to match the keyword, we match the substring after the colon:
    //
    //  [substring-after(@name,':')='PageResolution']
    //
    // We also need to ensure that the namespace refers to the correct
    // printschemakeywords namespace. Thus we get:
    //
    //  [name(namespace::*[.=PRINTSCHEMAKEYWORDNS])=substring-before(@name,':')]
    //
    // where PRINTSCHEMAKEYWORDNS is:
    //
    //  http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords
    //
    BSTR_t query(L"psf:PrintTicket/psf:Feature[substring-after(@name,':')='PageResolution']"
                 L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                 L"/*/*/psf:Value");

    THROW_ON_FAILED_HRESULT(
        m_pDOMDoc->selectNodes(query, &pNodes)
        );

    if (pNodes)
    {
        //
        // The Print Ticket may have both X and Y resolutions defined, but
        // the Xps Rasterization Service only accepts a single resolution.
        // We query for both X and Y resolutions and take the larger of the
        // two as the destination DPI. The resultant raster data could then
        // be scaled down in the other dimension to achieve the non-square
        // pixels.
        //

        LONG numResolutions;

        THROW_ON_FAILED_HRESULT(
            pNodes->get_length(&numResolutions)
            );

        if (numResolutions != 0 &&
            numResolutions != 1 &&
            numResolutions != 2)
        {
            //
            // We expect 0, 1, or 2 resolutions to be set in the Print Ticket.
            // Throw if this is not the case.
            //
            THROW_ON_FAILED_HRESULT(E_UNEXPECTED);
        }

        LONG maxResolution = 0;

        for (INT i = 0; i < numResolutions; i++)
        {
            IXMLDOMNode_t pCurrentNode;

            THROW_ON_FAILED_HRESULT(
                pNodes->get_item(i, &pCurrentNode)
                );

            BSTR_t strResolution;

            THROW_ON_FAILED_HRESULT(
                pCurrentNode->get_text(&strResolution)
                );

            LONG resolution;

            THROW_ON_FAILED_HRESULT(
                ::VarI4FromStr(
                    strResolution,
                    LOCALE_USER_DEFAULT, 
                    0, // no custom flags
                    &resolution
                    )
                );

            if (resolution > maxResolution)
            {
                maxResolution = resolution;
            }
        }

        dpi = static_cast<FLOAT>(maxResolution);
    }

    DoTraceMessage(XPSRASFILTER_TRACE_VERBOSE, L"Got DPI: %f", dpi);

    return dpi;
}

//
//Routine Name:
//
//    PrintTicketHandler::QueryPhysicalPageSize
//
//Routine Description:
//
//    Queries the physical page size from the 
//    Print Ticket using an XPath query.
//
//Return Value:
//
//    XPS_SIZE
//    Physical page size from the Print Ticket (in XPS units).
//
XPS_SIZE
PrintTicketHandler::QueryPhysicalPageSize()
{
    //
    // Default page size: 8.5" x 11" at Xps DPI
    //
    XPS_SIZE pageSize = {11.0f * xpsDPI,
                         8.5f  * xpsDPI};

    {
        //
        // Perform the page width query on the print ticket
        //
        IXMLDOMNode_t pNode;

        //
        // See the comment in PrintTicketHandler::QueryDPI() for details
        // about this XPath query.
        //
        BSTR_t query(L"psf:PrintTicket/psf:Feature[substring-after(@name,':')='PageMediaSize']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                     L"/*/psf:ScoredProperty[substring-after(@name,':')='MediaSizeWidth']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                     L"/psf:Value");

        THROW_ON_FAILED_HRESULT(
            m_pDOMDoc->selectSingleNode(query, &pNode)
            );

        if (pNode)
        {
            BSTR_t strWidth;
            THROW_ON_FAILED_HRESULT(
                pNode->get_text(&strWidth)
                );

            LONG width;
            THROW_ON_FAILED_HRESULT(
                ::VarI4FromStr(
                    strWidth,
                    LOCALE_USER_DEFAULT,
                    0, // no custom flags
                    &width
                    )
                );

            //
            // the page dimensions are in microns; convert to Xps units
            //
            pageSize.width = MicronsToXpsUnits(width);
        }
    }

    {
        //
        // Perform the page height query on the print ticket
        //
        IXMLDOMNode_t pNode;

        //
        // See the comment in PrintTicketHandler::QueryDPI() for details
        // about this XPath query.
        //
        BSTR_t query(L"psf:PrintTicket/psf:Feature[substring-after(@name,':')='PageMediaSize']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                     L"/*/psf:ScoredProperty[substring-after(@name,':')='MediaSizeHeight']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                     L"/psf:Value");

        THROW_ON_FAILED_HRESULT(
            m_pDOMDoc->selectSingleNode(query, &pNode)
            );

        if (pNode)
        {
            BSTR_t strHeight;
            THROW_ON_FAILED_HRESULT(
                pNode->get_text(&strHeight)
                );

            LONG height;
            THROW_ON_FAILED_HRESULT(
                ::VarI4FromStr(
                    strHeight, 
                    LOCALE_USER_DEFAULT, 
                    0, // no custom flags
                    &height
                    )
                );

            //
            // the page dimensions are in microns; convert to Xps unit
            //
            pageSize.height = MicronsToXpsUnits(height);
        }
    }

    {
        //
        // Perform the landscape query on the print ticket
        //
        IXMLDOMNode_t pNode;

        //
        // See the comment in PrintTicketHandler::QueryDPI() for details
        // about this XPath query.
        //
        BSTR_t query(L"psf:PrintTicket/psf:Feature[substring-after(@name,':')='PageOrientation']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]"
                     L"/psf:Option[substring-after(@name,':')='Landscape']"
                     L"[name(namespace::*[.='http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords'])=substring-before(@name,':')]");

        THROW_ON_FAILED_HRESULT(
            m_pDOMDoc->selectSingleNode(query, &pNode)
            );

        if (pNode)
        {
            //
            // landscape. swap height and width.
            //
            FLOAT tmp;

            tmp = pageSize.height;
            pageSize.height = pageSize.width;
            pageSize.width = tmp;
        }

        DoTraceMessage(XPSRASFILTER_TRACE_VERBOSE, L"Physical Page Size: %f x %f", pageSize.width, pageSize.height);
    }

    return pageSize;
}

//
//Routine Name:
//
//    PrintTicketHandler::QueryScaling
//
//Routine Description:
//
//    Queries the desired type of scaling from the 
//    Print Ticket using an XPath query.
//
//Return Value:
//
//    PrintTicketScaling
//    Scaling type from the Print Ticket.
//
PrintTicketScaling
PrintTicketHandler::QueryScaling()
{
    //
    // Default Scaling: FitApplicationBleedSizeToPageImageableSize
    //
    PrintTicketScaling scaling = SCALE_BLEEDTOIMAGEABLE;

    //
    // We do not query for media scaling. Rather, we always return
    // the equivalent of FitApplicationBleedSizeToPageImageableSize
    //
    return scaling;
}

} // namespace xpsrasfilter
