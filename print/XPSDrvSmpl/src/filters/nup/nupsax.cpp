/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupsax.xpp

Abstract:

   NUp SAX handler implementation. The class derives from the default SAX handler
   and implements only the necessary SAX APIs to process the mark-up. The
   handler is responsible for copying page mark-up to a writer, removing
   the fixed page opening and closing tags. It is also responsible for
   identifying resources that need to be copied from the source page to
   the NUp page.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "nupsax.h"

//
// We will need to preserve namespaces defined by the source fixed page into each canvas,
// e.g.  xmlns:x="http://schemas.microsoft.com/xps/2005/06/resourcedictionary-key". To do
// this we will create and populate a vector of namespaces to add to the canvas wrappning
// the source fixed page.
//
typedef pair<CStringXDW, CStringXDW> AttribValuePair;
typedef vector<AttribValuePair> NamespaceVector;

/*++

Routine Name:

    CNUpSaxHandler::CNUpSaxHandler

Routine Description:

    Contructor for the nup filters SAX handler which registers
    internally the writer for sending new markup out to and a
    resource copier which copies markup between pages

Arguments:

    pWriter    - Pointer to a write stream which receives markup
    pResCopier - Pointer to a resource markup copier object which
                 copies markup between pages

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CNUpSaxHandler::CNUpSaxHandler(
    _In_ IPrintWriteStream* pWriter,
    _In_ CResourceCopier*   pResCopier,
    _In_ CNUpTransform*     pNUpTransform
    ) :
    m_pWriter(pWriter),
    m_bOpenTag(FALSE),
    m_pResCopier(pResCopier),
    m_pNUpTransform(pNUpTransform)
{
    ASSERTMSG(m_pWriter != NULL, "NULL writer passed to NUp SAX handler.\n");
    ASSERTMSG(m_pResCopier != NULL, "NULL resource copier passed to NUp SAX handler.\n");
    ASSERTMSG(m_pNUpTransform != NULL, "NULL NUp Transform object passed to NUp SAX handler.\n");

    HRESULT hr = S_OK;
    if (FAILED(hr = CHECK_POINTER(m_pWriter, E_PENDING)) ||
        FAILED(hr = CHECK_POINTER(m_pResCopier, E_PENDING)) ||
        FAILED(hr = CHECK_POINTER(m_pNUpTransform, E_PENDING)))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CNUpSaxHandler::~CNUpSaxHandler

Routine Description:

    Default destructor for the nup filters SAX handler

Arguments:

    None

Return Value:

    None

--*/
CNUpSaxHandler::~CNUpSaxHandler()
{
    m_bstrOpenElement.Empty();
}

/*++

Routine Name:

    CNUpSaxHandler::startElement

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
CNUpSaxHandler::startElement(
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

    //
    // If this is the fixed page we need the width and height retrieved
    //
    BOOL bIsFixedPage = FALSE;

    try
    {
        CComBSTR bstrElement(cchQName, pwchQName);

        bIsFixedPage = (bstrElement == L"FixedPage");

        //
        // The resource dictionary is now canvas wide
        //
        if (bstrElement == L"FixedPage.Resources")
        {
            bstrElement = L"Canvas.Resources";
        }

        //
        // Check if we need to close an opened tag
        //
        if (m_bOpenTag)
        {
            cstrOut.Append(L">");
        }

        //
        // Store the opened element name so we can handle nested elements
        //
        m_bstrOpenElement = bstrElement;
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    if (SUCCEEDED(hr))
    {
        //
        // If this is the fixed page we do not write the tag
        //
        if (!bIsFixedPage)
        {
            //
            // Write out element
            //
            try
            {
                cstrOut.Append(L"<");
                cstrOut.Append(m_bstrOpenElement);
            }
            catch (CXDException& e)
            {
                hr = e;
            }

            //
            // We opened a tag
            //
            m_bOpenTag = TRUE;
        }

        SizeF sizePage(0.0f, 0.0f);

        //
        // Record additional namespaces defined by the fixed page
        //
        NamespaceVector fpNamespaces;

        //
        // Find the number of attributes and enumerate over all of them
        //
        INT cAttributes = 0;
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = pAttributes->getLength(&cAttributes)))
        {
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
                                                        &cchAttQName)))
                {
                    if (SUCCEEDED(pAttributes->getValue(cIndex, &pszAttValue, &cchAttValue)))
                    {
                        try
                        {
                            CComBSTR bstrAttName(cchAttQName, pszAttQName);
                            CComBSTR bstrAttValue(cchAttValue, pszAttValue);

                            if (bIsFixedPage)
                            {
                                //
                                // Extract the page Width and Height values
                                //
                                CStringXDW cstrAttName(bstrAttName);
                                if (cstrAttName == L"Width")
                                {
                                    sizePage.Width = static_cast<REAL>(_wtof(bstrAttValue));

                                    if (sizePage.Width < 1.f)
                                    {
                                        //
                                        // According to the Xps Specification, 
                                        // FixedPage Width must be >= 1.0
                                        //
                                        hr = E_FAIL;
                                    }
                                }
                                else if (cstrAttName == L"Height")
                                {
                                    sizePage.Height = static_cast<REAL>(_wtof(bstrAttValue));

                                    if (sizePage.Height < 1.f)
                                    {
                                        //
                                        // According to the Xps Specification,
                                        // FixedPage Height must be >= 1.0
                                        //
                                        hr = E_FAIL;
                                    }
                                }
                                else if (cstrAttName.Find(L"xmlns:") == 0)
                                {
                                    //
                                    // We have an additional namespace - add to the vector to be applied to the canvas element
                                    //
                                    fpNamespaces.push_back(AttribValuePair(cstrAttName, CStringXDW(bstrAttValue)));
                                }
                            }
                            else
                            {
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
                }
            }
        }

        if (SUCCEEDED(hr) &&
            bIsFixedPage)
        {
            //
            // Write the canvas out
            //
            try
            {
                //
                // Get the page transform for the current page
                //
                CComBSTR bstrMatrix;
                Matrix matrixTransform;

                if (SUCCEEDED(hr = m_pNUpTransform->GetPageTransform(sizePage, &matrixTransform)) &&
                    SUCCEEDED(hr = m_pNUpTransform->MatrixToXML(&matrixTransform, &bstrMatrix)))
                {

                    //
                    // Create a clipping region which is the size of the logical page
                    //
                    CStringXDW strClip;

                    strClip.Format(L"M 0,0 L %.2f,0 L %.2f,%.2f L 0, %.2f Z",
                                   sizePage.Width,
                                   sizePage.Width,
                                   sizePage.Height,
                                   sizePage.Height);

                    cstrOut.Format(L"<Canvas RenderTransform=\"%s\" Clip=\"%s\"",
                                      bstrMatrix,
                                      strClip);

                    //
                    // Add additional namespaces from the FixedPage
                    //
                    for (NamespaceVector::const_iterator iterNamespaces = fpNamespaces.begin();
                         iterNamespaces != fpNamespaces.end();
                         iterNamespaces++)
                    {
                        cstrOut += L" ";
                        cstrOut += (*iterNamespaces).first;
                        cstrOut += L"=\"";
                        cstrOut += (*iterNamespaces).second;
                        cstrOut += L"\"";
                    }

                    cstrOut += L">";
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

            ASSERTMSG(SUCCEEDED(hr), "Failed to write canvas\n");
        }

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

    CNUpSaxHandler::endElement

Routine Description:

    SAX handler method which handles each end element for the XML markup

Arguments:

    pwchQName - Pointer to a string containing the element name
    cchQName  - Count of the number of characters in the element name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CNUpSaxHandler::endElement(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    _In_reads_(cchQName) CONST wchar_t* pwchQName,
    _In_                  INT            cchQName
    )
{
    HRESULT hr = S_OK;
    CStringXDW cstrClose;
    BOOL bCloseTag = FALSE;

    try
    {
        CComBSTR bstrElement(cchQName, pwchQName);

        //
        // The resource dictionary is now canvas wide
        //
        if (bstrElement == L"FixedPage.Resources")
        {
            bstrElement = L"Canvas.Resources";
        }

        //
        // Ignore close fixed page tag
        //
        if (bstrElement != L"FixedPage")
        {
            bCloseTag = TRUE;

            //
            // If this is a root element with child nodes, the open
            // element will not match the last startElement. In this case
            // we need to add an appropriate closing tag
            //
            if (bstrElement == m_bstrOpenElement && !!m_bOpenTag)
            {
                //
                // Names match so just add a closing bracket
                //
                // We might have closed the tag when writing the watermark XML
                //
                cstrClose.Append(L"/>");
            }
            else
            {
                //
                // Add a closing tag
                //
                cstrClose.Append(L"</");
                cstrClose.Append(bstrElement);
                cstrClose.Append(L">");
            }

            m_bOpenTag = FALSE;
        }
        else
        {
            //
            // Close the canvas
            //
            try
            {
                bCloseTag = TRUE;
                cstrClose.Append(L"</Canvas>");
            }
            catch (CXDException& e)
            {
                hr = e;
            }

            ASSERTMSG(SUCCEEDED(hr), "Failed to write canvas\n");
        }
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    if (SUCCEEDED(hr) &&
        bCloseTag)
    {
        hr = WriteToPrintStream(&cstrClose, m_pWriter);
    }

    ERR_ON_HR(hr);
    return hr;
}

