/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bksax.cpp

Abstract:

   Booklet filter SAX handler implementation. The booklet SAX handler derives
   from the default SAX handler and implements the necesary SAX interfaces to
   process fixed page mark-up for booklet printing.
   For documents with odd page counts, we add a blank padding page so that the
   2-Up behaves correctly. All that is required of the handler is to retrieve
   the fixed page open tag and write this out.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "bksax.h"

/*++

Routine Name:

    CBkSaxHandler::CBkSaxHandler

Routine Description:

    Contructor for the booklet filters SAX handler which registers
    internally the writer for sending new markup out to

Arguments:

    pWriter - Pointer to a write stream which receives markup

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CBkSaxHandler::CBkSaxHandler(
    _In_ IPrintWriteStream* pWriter
    ) :
    m_pWriter(pWriter)
{
    ASSERTMSG(m_pWriter != NULL, "NULL writer passed to booklet SAX handler.\n");

    HRESULT hr = S_OK;
    if (FAILED(hr = CHECK_POINTER(m_pWriter, E_POINTER)))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CBkSaxHandler::~CBkSaxHandler

Routine Description:

    Default destructor for the booklet filters SAX handler

Arguments:

    None

Return Value:

    None

--*/
CBkSaxHandler::~CBkSaxHandler()
{
}

/*++

Routine Name:

    CBkSaxHandler::startElement

Routine Description:

    SAX handler method which handles each start element for the XML markup

Arguments:

    pwchQName   - Pointer to a string containing the element name
    cchQName    - Count of the number of characters in the element name
    pAttributes - Pointer to the attribute list for the supplied element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CBkSaxHandler::startElement(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    _In_reads_(cchQName) CONST wchar_t*  pwchQName,
    _In_                  INT             cchQName,
    _In_                  ISAXAttributes* pAttributes
    )
{
    HRESULT hr = S_OK;
    CStringXDW cstrOut;
    CComBSTR bstrElement;

    hr = bstrElement.Append(pwchQName, cchQName);

    //
    // All we are doing is copying the fixed page root into the new
    // document to ensure the same page size
    //
    INT cAttributes = 0;
    if (SUCCEEDED(hr) &&
        bstrElement == L"FixedPage" &&
        SUCCEEDED(hr = pAttributes->getLength(&cAttributes)))
    {
        try
        {
            cstrOut.Append(L"<");
            cstrOut.Append(bstrElement);
        }
        catch (CXDException& e)
        {
            hr = e;
        }

        //
        // For all attributes
        //
        for (INT cIndex = 0; cIndex < cAttributes && SUCCEEDED(hr); cIndex++)
        {
            PCWSTR pszAttUri   = NULL;
            INT    cchAttUri   = 0;
            PCWSTR pszAttName  = NULL;
            INT    cchAttName  = 0;
            PCWSTR pszAttQName = NULL;
            INT    cchAttQName = 0;
            PCWSTR pszAttValue = NULL;
            INT    cchAttValue = 0;

            //
            // Get the attribute data ready to write out
            //
            if (SUCCEEDED(hr = pAttributes->getName(cIndex,
                                                    &pszAttUri,
                                                    &cchAttUri,
                                                    &pszAttName,
                                                    &cchAttName,
                                                    &pszAttQName,
                                                    &cchAttQName)) &&
                SUCCEEDED(hr = pAttributes->getValue(cIndex, &pszAttValue, &cchAttValue)))
            {
                try
                {
                    CComBSTR bstrAttName(cchAttQName, pszAttQName);
                    CComBSTR bstrAttValue(cchAttValue, pszAttValue);

                    //
                    // Delimit attributes with a space
                    //
                    cstrOut.Append(L" ");

                    //
                    // Reconstruct the attribute and write back to
                    // the fixed page
                    //
                    cstrOut.Append(bstrAttName);
                    cstrOut.Append(L"=\"");

                    //
                    // If this is a UnicodeString we may need to escape entities
                    //
                    if (bstrAttName == L"UnicodeString")
                    {
                        hr = EscapeEntity(&bstrAttValue);
                    }

                    cstrOut.Append(bstrAttValue);
                    cstrOut.Append(L"\"");
                }
                catch (CXDException& e)
                {
                    hr = e;
                }
            }
        }

        //
        // Close the fixed page tag
        //
        if (SUCCEEDED(hr))
        {
            try
            {
                cstrOut.Append(L"/>");
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }

        //
        // Write out the empty page
        //
        if (SUCCEEDED(hr))
        {
            hr = WriteToPrintStream(&cstrOut, m_pWriter);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CBkSaxHandler::startDocument

Routine Description:

    SAX handler method which handles the start document call to ensure
    the xml version is correctly set

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CBkSaxHandler::startDocument(
    void
    )
{
    HRESULT hr = S_OK;

    try
    {
        if (SUCCEEDED(hr = CHECK_POINTER(m_pWriter, E_FAIL)))
        {
            CStringXDW cstrOut(L"<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            hr = WriteToPrintStream(&cstrOut, m_pWriter);
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    ERR_ON_HR(hr);
    return hr;
}

