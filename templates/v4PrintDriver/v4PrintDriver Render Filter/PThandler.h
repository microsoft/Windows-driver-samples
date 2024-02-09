//
// File Name:
//
//    PThandler.h
//
// Abstract:
//
//    Print Ticket Handler class declaration.
//

#pragma once

namespace v4PrintDriver_Render_Filter
{

class PrintTicketHandler
{
public:
    static
    PrintTicketHandler_t
    CreatePrintTicketHandler(
        const IPrintPipelinePropertyBag_t &pPropertyBag
        );

    void
    ProcessPart(
        const IFixedDocumentSequence_t  &pFDS
        );

    void
    ProcessPart(
        const IFixedDocument_t          &pFD
        );

    void
    ProcessPart(
        const IFixedPage_t              &pFP
        );

private:
    //
    // Constructor is private; use CreatePrintTicketHandler
    // to create instances.
    //
    PrintTicketHandler(
        const IXMLDOMDocument2_t    &pDoc,
        SafeHPTProvider_t           pHProvider,
        const IStream_t             &pUserPrintTicket
        );

    IStream_t
    ProcessPrintTicket(
        const IStream_t             &pBasePrintTicket,
        const IPartPrintTicket_t    &pDeltaPrintTicketPart,
        EPrintTicketScope           scope
        );

    //
    // Inline function to convert from microns to Xps Units
    //
    inline
    FLOAT
    MicronsToXpsUnits(
        long dimensionInMicrons
        )
    {
        const FLOAT xpsDPI = 96.0f;
        const FLOAT micronsPerInch = 25400.0f;      // 2.54 cm/in --> 25400 um/in

        return ((static_cast<FLOAT>(dimensionInMicrons) / micronsPerInch) * xpsDPI);
    }

    //
    // MSXML DOM document
    //
    IXMLDOMDocument2_t m_pDOMDoc;

    //
    // Handle to the Print Ticket Provider
    //
    SafeHPTProvider_t m_pHProvider;

    //
    // Cached Print Tickets
    //
    IStream_t m_pDefaultUserPrintTicket;
    IStream_t m_pJobPrintTicket;
    IStream_t m_pDocumentPrintTicket;
    IStream_t m_pPagePrintTicket;
};

} // namespace v4PrintDriver_Render_Filter
