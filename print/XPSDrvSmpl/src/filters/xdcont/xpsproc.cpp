/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsproc.cpp

Abstract:

   Implementation of the XPS processor. This class is responsible for handling
   the logical document and page structure of the XPS container. The class starts
   with the content types part and .rels part to find the Fixed Document Sequence.
   It then parses the FDS to find the Fixed Documents and finally the FDs to find
   each Fixed Page. Each fixed page is then reported to a client FixedPageProcessor
   to interpret the  mark-up and write out modifications as required. The class is
   additionally responsible for ensuring all XPS parts and accompanying resources are
   sent on to the PK archive handler to be written out.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpsproc.h"
#include "xpsfilew.h"

static PCSTR szContentTypes = "[Content_Types].xml";
static PCSTR szRelsPre      = "_rels/";
static PCSTR szRelsPost     = ".rels";

/*++

Routine Name:

    CXPSProcessor::CXPSProcessor

Routine Description:

    CXPSProcessor class constructor

Arguments:

    pReadStream    - Pointer to the print read stream
    pWriteStream   - Pointer to the print write stream
    pPageProcessor - Pointer to the page processor interface
    pPropertyBag   - Pointer to the filter pipeline property bag

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CXPSProcessor::CXPSProcessor(
    _In_ IPrintReadStream*          pReadStream,
    _In_ IPrintWriteStream*         pWriteStream,
    _In_ IFixedPageProcessor*       pPageProcessor,
    _In_ IPrintPipelinePropertyBag* pPropertyBag,
    _In_ CPTManager*                pPtManager
    ) :
    m_xpsArchive(pReadStream, pWriteStream),
    m_pSaxRdr(NULL),
    m_pPageProcessor(pPageProcessor),
    m_pPrintPropertyBag(pPropertyBag),
    m_pPtManager(pPtManager)
{
    HRESULT hr = S_OK;

    CComPtr<ISequentialStream> pFileReader(NULL);

    //
    // Setup the SAX reader and extract the content types
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pReadStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pWriteStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pPageProcessor, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pPtManager, E_POINTER)) &&
        SUCCEEDED(hr = m_pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)) &&
        SUCCEEDED(hr = m_xpsArchive.GetFileStream(szContentTypes, &pFileReader)) &&
        SUCCEEDED(hr = m_pSaxRdr->putContentHandler(&m_contentTypes)) &&
        SUCCEEDED(hr = m_pSaxRdr->parse(CComVariant(pFileReader))))
    {
        //
        // We've got all the data we need - send it on
        //
        hr = m_xpsArchive.SendCurrentFile();
    }

    //
    // Close the curent file ready for the next
    //
    m_xpsArchive.CloseCurrent();

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CXPSProcessor::~CXPSProcessor

Routine Description:

    CXPSProcessor class destructor

Arguments:

    None

Return Value:

    None

--*/
CXPSProcessor::~CXPSProcessor()
{
}

/*++

Routine Name:

    CXPSProcessor::Start

Routine Description:

    This routine kicks off the processing of the XPS archive struture. The
    routine parses the Fixed Document Sequence mark-up to find the Fixed
    Document parts, then parses the Fixed Document mark-up to find the Fixed
    Page parts before passing these on to the Fixed Page processor. Additionally
    the routine handles indentifying, processing and sending resources and part
    relationships. Note: This limits the XPS document processing to only modifying
    the fixed page data; the client is not presented the opportunity to modify any
    other parts.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::Start(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        //
        // Get the start part list and iterate over all parts
        //
        FileList fixedDocumentSequenceParts;
        if (SUCCEEDED(hr = GetFixedDocumentSequenceParts(&fixedDocumentSequenceParts)))
        {
            FileList::const_iterator iterFDS = fixedDocumentSequenceParts.begin();

            for (;
                 iterFDS != fixedDocumentSequenceParts.end() &&
                 SUCCEEDED(hr) &&
                 SUCCEEDED(hr = ProcessRelsParts(*iterFDS, ContentFixedDocumentSequence));
                 iterFDS++)
            {
                //
                // Parse the start part retrieving the FD list - iterate over all FDs
                //
                FileList fixedDocumentParts;
                if (SUCCEEDED(hr = GetFixedDocumentParts(*iterFDS, &fixedDocumentParts)))
                {
                    FileList::const_iterator iterFD = fixedDocumentParts.begin();

                    for (;
                         iterFD != fixedDocumentParts.end() &&
                         SUCCEEDED(hr) &&
                         SUCCEEDED(hr = ProcessRelsParts(*iterFD, ContentFixedDocument));
                         iterFD++)
                    {
                        //
                        // Parse the FD retrieving the FP list - iterate over all FPs
                        //
                        FileList fixedPageParts;
                        if (SUCCEEDED(hr = GetFixedPageParts(*iterFD, &fixedPageParts)))
                        {
                            FileList::const_iterator iterFP = fixedPageParts.begin();

                            for (;
                                 iterFP != fixedPageParts.end() &&
                                 SUCCEEDED(hr) &&
                                 SUCCEEDED(hr = ProcessRelsParts(*iterFP, ContentFixedPage));
                                 iterFP++)
                            {
                                //
                                // Process the fixed page - this calls on to the IFixedPageProcessor
                                // interface for the client to do the work
                                //
                                hr = ProcessFixedPage(*iterFP);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::GetFixedDocumentSequenceParts

Routine Description:

    This routine retrieves the Fixed Document Sequence from the root
    relationships part.

Arguments:

    pFixedDocumentSequencePartsList - List of fixed document sequences found to be populated

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::GetFixedDocumentSequenceParts(
    _Inout_ FileList* pFixedDocumentSequencePartsList
    )
{
    HRESULT hr = S_OK;

    //
    // Extract the .rels part to find the start part
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pFixedDocumentSequencePartsList, E_POINTER)) &&
        SUCCEEDED(hr = GetRelsForPart("")))
    {
        CONST RelsTypeList* pRelsTypeList = NULL;

        hr = m_rels.GetRelsTypeList("", &pRelsTypeList);

        RelsTypeList::const_iterator iterRels = pRelsTypeList->begin();

        for (;iterRels != pRelsTypeList->end() && SUCCEEDED(hr); iterRels++)
        {
            if (iterRels->second == RelsStartPart)
            {
                //
                // We have a start part add it to the part list
                //
                pFixedDocumentSequencePartsList->push_back(iterRels->first);
            }
            else
            {
                //
                // Send any other part on
                //
                if (SUCCEEDED(hr = m_xpsArchive.InitialiseFile(iterRels->first)) &&
                    SUCCEEDED(hr = m_xpsArchive.SendCurrentFile()))
                {
                   m_xpsArchive.CloseCurrent();
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::GetFixedDocumentParts

Routine Description:

    This routine retrieves the list of fixed documents from the fixed document
    sequence

Arguments:

    szFixedDocSeq - The name of the FDS part to parse
    pFixedDocumentParts - Pointer to the document list to be populated

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::GetFixedDocumentParts(
    _In_  PCSTR     szFixedDocSeq,
    _Out_ FileList* pFixedDocumentParts
    )
{
    HRESULT hr = S_OK;
    CComPtr<ISequentialStream> pFileReader(NULL);

    //
    // Set up the fixed document sequence as the current file in the
    // XPS archive and parse the file stream using the FDS SAX content
    // handler to retrieve all fixed documents in the sequence
    //
    if (SUCCEEDED(hr = CHECK_POINTER(szFixedDocSeq, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFixedDocumentParts, E_POINTER)) &&
        SUCCEEDED(hr = m_contentTypes.ValidateContentType(szFixedDocSeq, ContentFixedDocumentSequence)) &&
        SUCCEEDED(hr = m_xpsArchive.GetFileStream(szFixedDocSeq, &pFileReader)) &&
        SUCCEEDED(hr = m_fixedDocSeq.Clear()) &&
        SUCCEEDED(hr = m_pSaxRdr->putContentHandler(&m_fixedDocSeq)) &&
        SUCCEEDED(hr = m_pSaxRdr->parse(CComVariant(pFileReader))) &&
        SUCCEEDED(hr = m_fixedDocSeq.GetFixedDocumentList(pFixedDocumentParts)))
    {
        //
        // We are done with the FDS - send it on
        //
        hr = m_xpsArchive.SendCurrentFile();
    }

    //
    // Close the curent file ready for the next
    //
    m_xpsArchive.CloseCurrent();

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::GetFixedPageParts

Routine Description:

    This routine retrieves the list of fixed pages from the Fixed Document

Arguments:

    szFixedDoc      - The name of the FD part to parse
    pFixedPageParts - Pointer to the page list to be populated

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::GetFixedPageParts(
    _In_  PCSTR     szFixedDoc,
    _Out_ FileList* pFixedPageParts
    )
{
    HRESULT hr = S_OK;
    CComPtr<ISequentialStream> pFileReader(NULL);

    //
    // Set up the fixed document as the current file in the XPS
    // archive and parse the file stream using the FD SAX content
    // handler to retrieve all fixed pages in the document
    //
    if (SUCCEEDED(hr = CHECK_POINTER(szFixedDoc, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFixedPageParts, E_POINTER)) &&
        SUCCEEDED(hr = m_contentTypes.ValidateContentType(szFixedDoc, ContentFixedDocument)) &&
        SUCCEEDED(hr = m_xpsArchive.GetFileStream(szFixedDoc, &pFileReader)) &&
        SUCCEEDED(hr = m_fixedDoc.Clear()) &&
        SUCCEEDED(hr = m_pSaxRdr->putContentHandler(&m_fixedDoc)) &&
        SUCCEEDED(hr = m_pSaxRdr->parse(CComVariant(pFileReader))) &&
        SUCCEEDED(hr = m_fixedDoc.GetFixedPageList(pFixedPageParts)))
    {
        //
        // We are done with the FD - send it on
        //
        hr = m_xpsArchive.SendCurrentFile();
    }

    //
    // Close the curent file ready for the next
    //
    m_xpsArchive.CloseCurrent();

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::GetRelsForPart

Routine Description:

    This routine retrieves and populates the rels object with the relationships
    for the specified part

Arguments:

    szPartName - The name of the part to retrieve the relationships for

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::GetRelsForPart(
    _In_ PCSTR szPartName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szPartName, E_POINTER)))
    {
        try
        {
            CStringXDA cstrRelsPartName;
            CComPtr<ISequentialStream> pFileReader(NULL);

            //
            // Construct the rels part from the parent part name and parse for
            // relationships
            //
            if (SUCCEEDED(hr = MakeRelsPartName(szPartName, &cstrRelsPartName)) &&
                SUCCEEDED(hr = m_contentTypes.ValidateContentType(cstrRelsPartName, ContentRelationships)) &&
                SUCCEEDED(hr = m_xpsArchive.GetFileStream(cstrRelsPartName, &pFileReader)) &&
                SUCCEEDED(hr = m_pSaxRdr->putContentHandler(&m_rels)) &&
                SUCCEEDED(hr = m_rels.SetCurrentFileName(szPartName)) &&
                SUCCEEDED(hr = m_pSaxRdr->parse(CComVariant(pFileReader))))
            {
                //
                // We are done with the .rels part - send it on
                //
                hr = m_xpsArchive.SendCurrentFile();
            }

            //
            // Close the curent file ready for the next
            //
            m_xpsArchive.CloseCurrent();
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::MakeRelsPartName

Routine Description:

    This routine converts a part name into the appropriate .rels name.
    The rules for naming are:
        rels parts are stored in a directory named _rels relative to the current part
        rels parts always end in .rels
    e.g. \Documents\FixedDoc.fdseq has rels part \Documents\_rels\FixedDoc.fdseq.rels

Arguments:

    szPartName        - The name of the part to be converted
    pcstrRelsPartName - Pointer to a CStringXDA that recieves the new name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::MakeRelsPartName(
    _In_  PCSTR     szPartName,
    _Out_ CStringXDA* pcstrRelsPartName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szPartName, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcstrRelsPartName, E_POINTER)))
    {
        try
        {
            *pcstrRelsPartName = szPartName;

            //
            // Find the slash that splits the directory and part name
            //
            INT cLastSlash = -1;
            for (;;)
            {
                INT cSlash = pcstrRelsPartName->Find("/", cLastSlash + 1);

                if (cSlash != -1)
                {
                    cLastSlash = cSlash;
                }
                else
                {
                    break;
                }
            }
            cLastSlash++;

            //
            // Insert "_rels/" after the slash and append ".rels"
            //
            pcstrRelsPartName->Insert(cLastSlash, szRelsPre);
            pcstrRelsPartName->Append(szRelsPost);
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

    CXPSProcessor::ProcessRelsParts

Routine Description:

    This routine processes the related parts for a given part

Arguments:

    szPartName   - The name of the part
    eContentType - The type of the part to be processed. This must be either an FDS, FD or FP.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::ProcessRelsParts(
    _In_ PCSTR              szPartName,
    _In_ CONST EContentType eContentType
    )
{
    HRESULT hr = S_OK;

    BOOL bPrintTicketSent = FALSE;

    if (SUCCEEDED(hr = CHECK_POINTER(szPartName, E_POINTER)))
    {
        if (eContentType != ContentFixedDocumentSequence &&
            eContentType != ContentFixedDocument &&
            eContentType != ContentFixedPage)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            //
            // Validate the part content type
            // Get the rels and handle any print tickets
            //
            CONST RelsTypeList* pRelsTypeList;
            if (SUCCEEDED(hr = m_contentTypes.ValidateContentType(szPartName, eContentType)) &&
                SUCCEEDED(hr = GetRelsForPart(szPartName)) &&
                SUCCEEDED(hr = m_rels.GetRelsTypeList(szPartName, &pRelsTypeList)))
            {

                RelsTypeList::const_iterator iterRels = pRelsTypeList->begin();

                for (;iterRels != pRelsTypeList->end() && SUCCEEDED(hr); iterRels++)
                {
                    //
                    // Strip any leading "/"
                    //
                    CStringXDA cstrName(iterRels->first);
                    if (cstrName.GetAt(0) == '/')
                    {
                        cstrName.Delete(0);
                    }

                    //
                    // The second of the pair in the RelsTypeList iterator is the rels type
                    //
                    switch (iterRels->second)
                    {
                        case RelsPrintTicket:
                        {
                            hr = AddPrintTicket(cstrName, eContentType);
                            bPrintTicketSent = TRUE;
                        }
                        break;

                        case RelsAnnotations:
                        case RelsDigitalSignatureDefinitions:
                        case RelsDiscardControl:
                        case RelsDocumentStructure:
                        case RelsRequiredResource:
                        case RelsRestrictedFont:
                        case RelsStoryFragments:
                        case RelsCoreProperties:
                        case RelsDigitalSignature:
                        case RelsDigitalSignatureCertificate:
                        case RelsDigitalSignatureOrigin:
                        case RelsThumbnail:
                        {
                            //
                            // We are not interested in the content so intialise the current
                            // file in the XPS archive and send it on
                            //
                            if (SUCCEEDED(hr = m_xpsArchive.InitialiseFile(cstrName)))
                            {
                                hr = m_xpsArchive.SendCurrentFile();
                            }
                        }
                        break;

                        default:
                        {
                            ERR("Unrecognised rels part\n");

                            hr = E_FAIL;
                        }
                        break;
                    }

                    //
                    // Close the current file ready for the next
                    //
                    m_xpsArchive.CloseCurrent();
                }
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // There is no PrintTicket
                //
                hr = S_OK;
            }

            //
            // No print ticket has been sent so just inform the print ticket
            // manager that a suitable ticket needs to be set for this level
            //
            if (bPrintTicketSent == FALSE)
            {
                switch (eContentType)
                {
                    case ContentFixedPage:
                    {
                        hr = m_pPtManager->SetTicket(kPTPageScope, NULL);
                    }
                    break;
                    case ContentFixedDocument:
                    {
                        hr = m_pPtManager->SetTicket(kPTDocumentScope, NULL);
                    }
                    break;
                    case ContentFixedDocumentSequence:
                    {
                        hr = m_pPtManager->SetTicket(kPTJobScope, NULL);
                    }
                    break;
                    default:
                    {
                        hr = ERROR_NOT_SUPPORTED;
                    }
                    break;
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::AddPrintTicket

Routine Description:

    This routine retrieves the PrintTicket information and updates the PrintTicket
    manager at the appropriate scope

Arguments:

    szPTPartName - The part name for the PrintTicket
    eContentType - The type of the containing part. This must be either an FDS, FD or FP.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::AddPrintTicket(
    _In_ PCSTR              szPTPartName,
    _In_ CONST EContentType eContentType
    )
{
    HRESULT hr = S_OK;

    //
    // Validate the parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(szPTPartName, E_POINTER)))
    {
        if (eContentType != ContentFixedDocumentSequence &&
            eContentType != ContentFixedDocument &&
            eContentType != ContentFixedPage)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Extract the PrintTicket from the current file in the XPS archive
    // and pass to the PrinTicket manager at the appropriate scope
    //
    CComPtr<ISequentialStream> pFileReader(NULL);
    CComPtr<IXMLDOMDocument2>  pPT(NULL);

    VARIANT_BOOL fLoaded = VARIANT_FALSE;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_xpsArchive.GetFileStream(szPTPartName, &pFileReader)) &&
        SUCCEEDED(hr = pPT.CoCreateInstance(CLSID_DOMDocument60)) &&
        SUCCEEDED(hr = pPT->load(CComVariant(pFileReader), &fLoaded)))
    {
        if (fLoaded == VARIANT_TRUE)
        {
            //
            // Determine the scope from the parent parts content type
            //
            EPrintTicketScope ePTScope = kPTPageScope;

            if (eContentType == ContentFixedDocumentSequence)
            {
                ePTScope = kPTJobScope;
            }
            else if (eContentType == ContentFixedDocument)
            {
                ePTScope = kPTDocumentScope;
            }

            if (SUCCEEDED(hr = m_pPtManager->SetTicket(ePTScope, pPT)))
            {
                //
                // Make sure the PrintTicket is passed on
                //
                hr = m_xpsArchive.SendCurrentFile();
            }
        }
        else
        {
            ERR("Failed to load PT\n");

            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSProcessor::AddPrintTicket

Routine Description:

    This routine intialises the appropriate read and write streams for a fixed
    page processor and calls the processor to do the work

Arguments:

    szFPPartName - The name of the FixedPage part

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSProcessor::ProcessFixedPage(
    _In_ PCSTR szFPPartName
    )
{
    HRESULT hr = S_OK;
    //
    // Retrieve the file stream and pass to the page scaling handler
    //
    CComPtr<ISequentialStream> pFileReader(NULL);
    CXPSWriteFile* pXpsWriteFile = new(std::nothrow) CXPSWriteFile(szFPPartName);

    IXMLDOMDocument2* pPT = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pXpsWriteFile, E_OUTOFMEMORY)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pPageProcessor, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(szFPPartName, E_PENDING)) &&
        SUCCEEDED(hr = m_xpsArchive.GetFileStream(szFPPartName, &pFileReader)) &&
        SUCCEEDED(hr = m_pPtManager->GetTicket(kPTPageScope, &pPT)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPT, E_FAIL)) &&
        SUCCEEDED(hr = m_pPageProcessor->ProcessFixedPage(pPT, pFileReader, pXpsWriteFile)))
    {
        //
        // Send the new fixed page
        //
        ULONG cb = 0;
        PVOID pv = NULL;

        if (SUCCEEDED(hr = pXpsWriteFile->GetBuffer(&pv, &cb)))
        {
            hr = m_xpsArchive.SendFile(szFPPartName, pv, cb, CompDeflated);
        }
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
    {
        //
        // The page processor does not want to do any work - just send the page
        //
        hr = m_xpsArchive.SendCurrentFile();
    }

    if (pXpsWriteFile != NULL)
    {
        delete pXpsWriteFile;
        pXpsWriteFile = NULL;
    }

    //
    // Close the current file ready for the next
    //
    m_xpsArchive.CloseCurrent();

    ERR_ON_HR(hr);
    return hr;
}
