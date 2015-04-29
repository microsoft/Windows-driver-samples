/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfd.cpp

Abstract:

   Implementation of the XPS Fixed Document (FD) SAX handler. This class is
   responsible for retrieving and storing in the correct order the Fixed Pages
   that comprise the Fixed Document.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpsfd.h"

static PCSTR szPageContent = "PageContent";
static PCSTR szSource      = "Source";

/*++

Routine Name:

    CFixedDocument::CFixedDocument

Routine Description:

    CFixedDocument class constructor

Arguments:

    None

Return Value:

    None

--*/
CFixedDocument::CFixedDocument()
{
}

/*++

Routine Name:

    CFixedDocument::~CFixedDocument

Routine Description:

    CFixedDocument class destructor

Arguments:

    None

Return Value:

    None

--*/
CFixedDocument::~CFixedDocument()
{
}

/*++

Routine Name:

    CFixedDocument::startElement

Routine Description:

    This routine is the startElement method of the SAX handler. This is used
    to parse the fixed document mark-up retrieving and storing the list of
    fixed pages in the document

Arguments:

   pwchNamespaceUri - Unused: Local namespace URI
   cchNamespaceUri  - Unused: Length of the namespace URI
   pwchLocalName    - Unused: Local name string
   cchLocalName     - Unused: Length of the local name string
   pwchQName        - The qualified name with prefix
   cchQName         - The length of the qualified name with prefix
   pAttributes      - Pointer to the ISAXAttributes interface containing attributes attached to the element


Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CFixedDocument::startElement(
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
        if (cchQName <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    try
    {
        CStringXDA cstrElementName(pwchQName, cchQName);

        if (cstrElementName == szPageContent)
        {
            INT cAttributes = 0;

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->getLength(&cAttributes);
            }

            //
            // Run over all attributes identifying the document in the sequence
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
                if (SUCCEEDED(hr = pAttributes->getName(cIndex, &pszAttUri, &cchAttUri, &pszAttName, &cchAttName, &pszAttQName, &cchAttQName)) &&
                    SUCCEEDED(hr = pAttributes->getValue(cIndex, &pszAttValue, &cchAttValue)))
                {
                    CStringXDA cstrAttName(pszAttQName, cchAttQName);
                    CStringXDA cstrAttValue(pszAttValue, cchAttValue);

                    if (cstrAttName == szSource)
                    {
                        //
                        // Strip any leading "/"
                        //
                        if (cstrAttValue.GetAt(0) == '/')
                        {
                            cstrAttValue.Delete(0);
                        }

                        try
                        {
                            //
                            // Add the fixed page to the list
                            //
                            m_FixedPageList.push_back(cstrAttValue);
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

    CFixedDocument::GetFixedPageList

Routine Description:

    This routine retrieves the fixed page list constructed as the fixed document
    mark-up was parsed

Arguments:

    pFixedPageList - Pointer to the vector that recieves the fixed page list

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CFixedDocument::GetFixedPageList(
    _Out_ FileList* pFixedPageList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFixedPageList, E_POINTER)))
    {
        try
        {
            pFixedPageList->assign(m_FixedPageList.begin(), m_FixedPageList.end());
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

    return hr;
}

/*++

Routine Name:

    CFixedDocument::Clear

Routine Description:

    This routine clears the fixed page list

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CFixedDocument::Clear(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_FixedPageList.clear();

    ERR_ON_HR(hr);
    return hr;
}
