/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfds.cpp

Abstract:

   Implementation of the XPS Fixed Document Sequence (FDS) SAX handler.
   This class is responsible for retrieving and storing in the correct
   order the Fixed Documentss that comprise the Fixed Document
   Sequence.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpsfds.h"

static PCSTR szDocumentReference = "DocumentReference";
static PCSTR szSource            = "Source";

/*++

Routine Name:

    CFixedDocumentSequence::CFixedDocumentSequence

Routine Description:

    CFixedDocumentSequence class constructor

Arguments:

    None

Return Value:

    None

--*/
CFixedDocumentSequence::CFixedDocumentSequence()
{
}

/*++

Routine Name:

    CFixedDocumentSequence::~CFixedDocumentSequence

Routine Description:

    CFixedDocumentSequence class destructor

Arguments:

    None

Return Value:

    None

--*/
CFixedDocumentSequence::~CFixedDocumentSequence()
{
}

/*++

Routine Name:

    CFixedDocumentSequence::startElement

Routine Description:

    This routine is the startElement method of the SAX handler. This is used
    to parse the fixed document sequence mark-up retrieving and storing the list of
    fixed documents in the sequence

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
CFixedDocumentSequence::startElement(
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

        if (cstrElementName == szDocumentReference)
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
                            // Add the fixed document to the list
                            //
                            m_FixedDocumentList.push_back(cstrAttValue);
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

    CFixedDocumentSequence::GetFixedDocumentList

Routine Description:

    This routine retrieves the fixed document list constructed as the fixed document
    sequence mark-up was parsed

Arguments:

    pFixedDocumentList - Pointer to the vector that recieves the fixed document list

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CFixedDocumentSequence::GetFixedDocumentList(
    _Out_ FileList* pFixedDocumentList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFixedDocumentList, E_POINTER)))
    {
        try
        {
            pFixedDocumentList->assign(m_FixedDocumentList.begin(), m_FixedDocumentList.end());
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

    CFixedDocumentSequence::Clear

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
CFixedDocumentSequence::Clear(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_FixedDocumentList.clear();

    ERR_ON_HR(hr);
    return hr;
}
