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

   PrintTicket management class definition. This class encapsulate
   PrintTicket handling algorithm defined in the XPS Document specification.
   It provides a simple set and get interace for to filters and handles
   merging of tickets and the use of the Win32 PrintTicket provider API.
   The algorithm for determining the PrintTicket applies as follows:

   1. Validate and merge the PrintTicket from the FDS with the default
      printicket converted from the default devmode in the property bag.
      The resultant ticket will be the Job level ticket.

   2. Validate and merge the PrintTicket from the current FD with the Job
      level ticket from step 1. The resultant ticket will be the document
      level ticket.

   3. Validate and merge the PrintTicket from the current FP with the Doc
      level ticket from step 2. The resultant ticket will be the page
      level ticket.

--*/

#pragma once

class CPTManager
{
public:
    CPTManager();

    virtual ~CPTManager();

    HRESULT
    Initialise(
        _In_   IPrintReadStream* pDefaultTicketStream,
        _In_z_ BSTR              bstrPrinterName,
        _In_   HANDLE            userToken
        );

    HRESULT
    SetTicket(
        _In_ CONST IFixedDocumentSequence* pFDS
        );

    HRESULT
    SetTicket(
        _In_ CONST IFixedDocument* pFD
        );

    HRESULT
    SetTicket(
        _In_ CONST IFixedPage* pFP
        );

    HRESULT
    SetTicket(
        _In_     CONST EPrintTicketScope ptScope,
        _In_opt_ CONST IXMLDOMDocument2* pPT
        );

    HRESULT
    GetTicket(
        _In_        CONST EPrintTicketScope ptScope,
        _Outptr_ IXMLDOMDocument2**      ppTicket
        );

    HRESULT
    GetCapabilities(
        _In_        IXMLDOMDocument2*  pTicket,
        _Outptr_ IXMLDOMDocument2** ppCapabilities
        );

private:
    HRESULT
    InitialisePrintTickets(
        _In_ IStream* pDefaultPTStream
        );

    HRESULT
    SetPTFromDOMDoc(
        _In_        IXMLDOMDocument2*  pPTDOMDoc,
        _Outptr_ IXMLDOMDocument2** ppDomDoc
        );

    HRESULT
    SetPTFromStream(
        _In_        IStream*           pPTStream,
        _Outptr_ IXMLDOMDocument2** ppDomDoc
        );

    HRESULT
    MergeTicket(
        _In_ CONST EPrintTicketScope ptScope,
        _In_ CONST IPartPrintTicket* pPTRef
        );

    HRESULT
    MergeTicket(
        _In_ CONST EPrintTicketScope ptScope,
        _In_ CONST IXMLDOMDocument2* pPT
        );

    HRESULT
    GetMergedTicket(
        _In_        CONST EPrintTicketScope ptScope,
        _In_        CONST IXMLDOMDocument2* pDelta,
        _In_        IXMLDOMDocument2*       pBase,
        _Outptr_ IXMLDOMDocument2**      ppResult
        );

    HRESULT
    CloseProvider(
        VOID
        );

    HRESULT
    UpdateDefaultPTs(
        _In_ CONST EPrintTicketScope ptScope
        );

    VOID
    FreePrintTickets(
        _In_ CONST EPrintTicketScope ptScope
        );

private:
    CComPtr<IXMLDOMDocument2> m_pDefaultPT;

    CComPtr<IXMLDOMDocument2> m_pJobPT;

    CComPtr<IXMLDOMDocument2> m_pDocPT;

    CComPtr<IXMLDOMDocument2> m_pPagePT;

    HPTPROVIDER               m_hProvider;

    HANDLE                    m_hToken;
};

