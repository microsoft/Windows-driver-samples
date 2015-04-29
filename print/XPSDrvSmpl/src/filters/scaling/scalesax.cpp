/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scalesax.xpp

Abstract:

   Page Scaling SAX handler implementation. The class derives from the default SAX handler
   and implements only the necessary SAX APIs to process the mark-up. The
   handler is responsible for copying page mark-up to a writer, removing
   the fixed page opening and closing tags.

Known Issues:

    Need to check if all resources are required resources are being added to the
    resource copying class.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "scalesax.h"

/*++

Routine Name:

    CScaleSaxHandler::CScaleSaxHandler

Routine Description:

    CScaleSaxHandler class constructor

Arguments:

    pWriter - Pointer to the output stream interface for the XPS page markup.
    pPageScaling - Pointer to the Page Scaling Interface.

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CScaleSaxHandler::CScaleSaxHandler(
    _In_ ISequentialStream* pWriter,
    _In_ CPageScaling*      pPageScaling
    ) :
    m_pWriter(pWriter),
    m_pPageScaling(pPageScaling),
    m_bOpenTag(FALSE)
{
    ASSERTMSG(m_pWriter != NULL, "NULL writer passed to page scaling SAX handler.\n");
    ASSERTMSG(m_pPageScaling != NULL, "NULL page scaling class passed to page scaling SAX handler.\n");

    HRESULT hr = S_OK;
    if (FAILED(hr = CHECK_POINTER(m_pWriter, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pPageScaling, E_POINTER)))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CScaleSaxHandler::~CScaleSaxHandler

Routine Description:

    CScaleSaxHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CScaleSaxHandler::~CScaleSaxHandler()
{
    m_bstrOpenElement.Empty();
}

/*++

Routine Name:

    CScaleSaxHandler::startElement

Routine Description:

    Receives notification of the beginning of an XML element in the XPS page.
    The page scaling filter parses each element, applies any changes
    and writes out the resultant XPS markup.

Arguments:

    pwchQName   - The XML 1.0 qualified name (QName), with prefix, or an empty string
                  (if QNames are not available).
    cchQName    - The length of the QName.
    pAttributes - The attributes attached to the element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CScaleSaxHandler::startElement(
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
        // Check if we need to close an opened tag
        //
        if (m_bOpenTag)
        {
            cstrOut.Append(L">\n");
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

        REAL widthPage(0.0f);
        REAL heightPage(0.0f);
        RectF bleedBox(0.0f, 0.0f, 0.0f, 0.0f);
        RectF contentBox(0.0f, 0.0f, 0.0f, 0.0f);
        
        BOOL bBleedBoxSet = FALSE;
        BOOL bContentBoxSet = FALSE;

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

                            //
                            // If this is the fixed page we want to  retrieve the
                            // dimensions of the fixed page - take the opportunity
                            // to do so now while we parse the attributes
                            //
                            if (bIsFixedPage)
                            {
                                if (bstrAttName == L"Width")
                                {
                                    widthPage = static_cast<REAL>(_wtof(bstrAttValue));
                                }
                                else if (bstrAttName == L"Height")
                                {
                                    heightPage = static_cast<REAL>(_wtof(bstrAttValue));
                                }
                                else if (bstrAttName == L"ContentBox")
                                {
                                    CStringXDW str( bstrAttValue );
                                    CStringXDW resToken;
                                    INT curPos= 0;
                                    bContentBoxSet = TRUE;

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != L"")
                                    {
                                        contentBox.X = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != L"")
                                    {
                                        contentBox.Y = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != L"")
                                    {
                                        contentBox.Width = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != L"")
                                    {
                                        contentBox.Height = static_cast<REAL>(_wtof(resToken));
                                    }
                                }
                                else if (bstrAttName == L"BleedBox")
                                {
                                    CStringXDW str( bstrAttValue );
                                    CStringXDW resToken;
                                    INT curPos= 0;
                                    bBleedBoxSet = TRUE;

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != "")
                                    {
                                        bleedBox.X = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != "")
                                    {
                                        bleedBox.Y = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != "")
                                    {
                                        bleedBox.Width = static_cast<REAL>(_wtof(resToken));
                                    }

                                    resToken = str.Tokenize(L",",curPos);

                                    if (resToken != "")
                                    {
                                        bleedBox.Height = static_cast<REAL>(_wtof(resToken));
                                    }
                                }
                            }

                            //
                            // As we are applying scaling we need to remove the bleedBox and contentBox
                            //
                            if (bstrAttName != L"BleedBox" &&
                                bstrAttName != L"ContentBox")
                            {
                                //
                                // Replace the Width and Height aValues with the target papersize
                                //
                                if (bstrAttName == L"Width")
                                {
                                    CComBSTR bstrWidthValue;
                                    if (SUCCEEDED(hr = m_pPageScaling->GetFixedPageWidth(&bstrWidthValue)))
                                    {
                                        if (bstrWidthValue.Length() > 0)
                                        {
                                            bstrAttValue = bstrWidthValue;
                                        }
                                    }
                                }
                                else if (bstrAttName == L"Height")
                                {
                                    CComBSTR bstrHeightValue;
                                    if (SUCCEEDED(hr = m_pPageScaling->GetFixedPageHeight(&bstrHeightValue)))
                                    {
                                        if (bstrHeightValue.Length() > 0)
                                        {
                                            bstrAttValue = bstrHeightValue;
                                        }
                                    }
                                }

                                if (SUCCEEDED(hr))
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
                        }
                        catch (CXDException& e)
                        {
                            hr = e;
                        }
                    }
                }
            }
        }

        //
        // If bleedbox hasn't been set, set it to the page dimensions
        //
        if (bBleedBoxSet == FALSE)
        {
            bleedBox.Width = widthPage;
            bleedBox.Height = heightPage;
        }

        //
        // If contentbox hasn't been set, set it to the page dimensions
        //
        if (bContentBoxSet == FALSE)
        {
            contentBox.Width = widthPage;
            contentBox.Height = heightPage;
        }

        if (SUCCEEDED(hr = m_pPageScaling->SetPageDimensions(widthPage, heightPage)) &&
            SUCCEEDED(hr = m_pPageScaling->SetBleedBox(&bleedBox)) &&
            SUCCEEDED(hr = m_pPageScaling->SetContentBox(&contentBox)))
        {
            if (bIsFixedPage)
            {
                try
                {
                    //
                    // Close the fixed page tag
                    //
                    cstrOut.Append(L">");

                    //
                    // Create and retrieve the markup for the Canvas
                    //
                    CComBSTR bstrCanvasText;
                    if (SUCCEEDED(hr = m_pPageScaling->GetOpenTagXML(&bstrCanvasText)))
                    {
                        //
                        // Insert the mark-up by writing after the opening fixed page
                        // remembering to close the tag first
                        //
                        cstrOut.Append(bstrCanvasText);
                    }
                }
                catch (CXDException& e)
                {
                    hr = e;
                }
            }
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

    CScaleSaxHandler::endElement

Routine Description:

    Receives notification of the end of an XML element in the XPS page.
    The page scaling filter parses each element, applies any changes
    and writes out the resultant XPS markup.

    A corresponding startElement method is invoked for every endElement method, even when the element is empty.

Arguments:

    pwchQName - The XML 1.0 qualified name (QName), with prefix, or an empty string
                (if QNames are not available).
    cchQName - The length of the QName.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CScaleSaxHandler::endElement(
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
    BOOL bIsFixedPage = FALSE;

    try
    {
        CComBSTR bstrElement(cchQName, pwchQName);

        bIsFixedPage = (bstrElement == L"FixedPage");

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
            if (m_bOpenTag)
            {
                cstrClose.Append(L"/>\n");
            }
        }
        else
        {
            //
            // Close Canvas if previously opened.
            //
            if (bIsFixedPage)
            {
                //
                // Insert the closing canvas mark-up before closing the fixed page
                //
                CComBSTR bstrCloseCanvasText;
                if (SUCCEEDED(hr = m_pPageScaling->GetCloseTagXML(&bstrCloseCanvasText)))
                {
                    cstrClose.Append(bstrCloseCanvasText);
                }
            }

            //
            // Add a full closing tag
            //
            cstrClose.Append(L"</");
            cstrClose.Append(bstrElement);
            cstrClose.Append(L">\n");
        }

        m_bOpenTag = FALSE;
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    if (SUCCEEDED(hr))
    {
        hr = WriteToPrintStream(&cstrClose, m_pWriter);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CScaleSaxHandler::startDocument

Routine Description:

    This method writes out the header markup for the XPS page.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CScaleSaxHandler::startDocument()
{
    HRESULT hr = S_OK;

    try
    {
        CStringXDW cstrXMLVersion(L"<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        hr = WriteToPrintStream(&cstrXMLVersion, m_pWriter);
    }
    catch (CXDException& e)
    {
        hr = e;
    }

    ERR_ON_HR(hr);
    return hr;
}

