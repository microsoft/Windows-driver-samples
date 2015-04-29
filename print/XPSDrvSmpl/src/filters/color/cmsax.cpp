/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmsax.cpp

Abstract:

   Color managed image SAX handler implementation. The CCMSaxHandler class is responsible
   for handling the XML markup in the container.

   Note regarding opacity masks: Ideally we would like to avoid color matching whilst
   within an opacity mask element as only the alpha channel is of interest. This is
   complicated however by the fact that the opacity mask resources my be re-used as
   rendering resources. Ideally we would color match opacity mask elements only when
   the resources are shared with other rendering operations. This is difficult to
   achieve with SAX however as we do not know a priori if the resource is shared or
   the resource is going to be shared.
   Currently we are performing unnecessary color matching when an opacity mask is
   encountered. We could alternatively ensure opacity mask resources are preserved and
   other render sources are modified and written as new resources. The trade off is
   reduced speed performance against added resource handling complexity and XPS
   content bloat. This implementation takes the first option which trades processing time
   against the amount of data written to the container and resource handling complexity.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "cmsax.h"

/*++

Routine Name:

    CCMSaxHandler::CCMSaxHandler

Routine Description:

    Contructor for the color management filters SAX handler.
    The constructor registers a writer for sending new markup out to and a color
    converter object which handles any color conversion work for those markup
    elements containing color data.

Arguments:

    pWriter    - Pointer to a write stream which receives markup
    pConverter - Pointer to a color converter object which handles any
                 color conversion work for elements containing color data

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CCMSaxHandler::CCMSaxHandler(
    _In_     IPrintWriteStream*            pWriter,
    _In_     CBitmapColorConverter*        pBmpConverter,
    _In_     CColorRefConverter*           pRefConverter,
    _In_opt_ CResourceDictionaryConverter* pDictConverter
    ) :
    m_pWriter(pWriter),
    m_bOpenTag(FALSE),
    m_pBmpConv(pBmpConverter),
    m_pRefConv(pRefConverter),
    m_pDictConv(pDictConverter)
{
    ASSERTMSG(m_pWriter != NULL, "NULL writer passed to color SAX handler.\n");
    ASSERTMSG(m_pBmpConv != NULL, "NULL bitmap color converter passed to color SAX handler.\n");
    ASSERTMSG(m_pRefConv != NULL, "NULL color ref converter passed to color SAX handler.\n");

    HRESULT hr = S_OK;
    if (FAILED(hr = CHECK_POINTER(m_pWriter, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pBmpConv, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pRefConv, E_POINTER)))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CCMSaxHandler::~CCMSaxHandler

Routine Description:

    Default destructor for the color management filters SAX handler

Arguments:

    None

Return Value:

    None

--*/
CCMSaxHandler::~CCMSaxHandler()
{
    m_bstrOpenElement.Empty();
}

/*++

Routine Name:

    CCMSaxHandler::startElement

Routine Description:

    SAX handler method which handles each start element for the XML markup.

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
CCMSaxHandler::startElement(
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

    if (SUCCEEDED(hr = CHECK_POINTER(pwchQName, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pAttributes, E_POINTER)))
    {
        if (cchQName < 1)
        {
            hr = E_INVALIDARG;
        }
    }

    CStringXDW cstrOut;
    if (SUCCEEDED(hr))
    {
        try
        {
            CComBSTR bstrElement(cchQName, pwchQName);

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

            //
            // Create output element
            //
            cstrOut.Append(L"<");
            cstrOut.Append(bstrElement);

            //
            // We opened a tag
            //
            m_bOpenTag = TRUE;

            //
            // Find the number of attributes and enumerate over all of them
            //
            INT cAttributes = 0;
            if (SUCCEEDED(hr = pAttributes->getLength(&cAttributes)))
            {
                for (INT cIndex = 0; cIndex < cAttributes; cIndex++)
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
                            CComBSTR bstrAttName(cchAttQName, pszAttQName);
                            CComBSTR bstrAttValue(cchAttValue, pszAttValue);

                            if (bstrElement == L"ResourceDictionary" &&
                                bstrAttName == L"Source")
                            {
                                //
                                // Convert remote resource dictionary
                                //
                                if (SUCCEEDED(hr = CHECK_POINTER(m_pDictConv, E_FAIL)))
                                {
                                    hr = m_pDictConv->Convert(&bstrAttValue);
                                }
                            }
                            else
                            {
                                //
                                // Find all color refs and image sources.
                                //
                                if (bstrAttName == L"Color" ||
                                    bstrAttName == L"Fill" ||
                                    bstrAttName == L"Stroke" )
                                {
                                    //
                                    // Process the value passing the colorref
                                    //
                                    hr = m_pRefConv->Convert(&bstrAttValue);
                                }
                                else if (bstrAttName == L"ImageSource")
                                {
                                    //
                                    // Process the value passing the URI
                                    //
                                    hr = m_pBmpConv->Convert(&bstrAttValue);
                                }
                            }

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
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = WriteToPrintStream(&cstrOut, m_pWriter);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CCMSaxHandler::endElement

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
CCMSaxHandler::endElement(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    _In_reads_(cchQName) CONST wchar_t* pwchQName,
    _In_                  INT            cchQName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pwchQName, E_POINTER)))
    {
        if (cchQName < 1)
        {
            hr = E_INVALIDARG;
        }
    }

    CStringXDW cstrClose;
    if (SUCCEEDED(hr))
    {
        try
        {
            CComBSTR bstrElement(cchQName, pwchQName);

            //
            // If this is a root element with child nodes, the open
            // element will not match the last startElement. In this case
            // we need to add an appropriate closing tag
            //
            if (bstrElement == m_bstrOpenElement)
            {
                //
                // Names match so just add a closing bracket
                //
                // We might have closed the tag when writing a new element
                //
                if (m_bOpenTag)
                {
                    cstrClose.Append(L"/>");
                }
            }
            else
            {
                //
                // Add a clossing tag
                //
                cstrClose.Append(L"</");
                cstrClose.Append(bstrElement);
                cstrClose.Append(L">");
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = WriteToPrintStream(&cstrClose, m_pWriter);
    }

    m_bOpenTag = FALSE;

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CCMSaxHandler::startDocument

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
CCMSaxHandler::startDocument(
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

