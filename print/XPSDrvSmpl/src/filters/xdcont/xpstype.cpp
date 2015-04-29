/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpstype.cpp

Abstract:

   Implementation of the XPS content type part SAX handler. This class is responsible
   for retrieving all the content types information and storing them for access by the
   XPS processor.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpstype.h"

static PCSTR szCTOverride    = "Override";
static PCSTR szCTPartName    = "PartName";
static PCSTR szCTContentType = "ContentType";
static PCSTR szCTDefault     = "Default";
static PCSTR szCTExtension   = "Extension";

static PCSTR szContentType[EContentTypeMax] = {
    "application/vnd.ms-package.xps-fixeddocumentsequence+xml",
    "application/vnd.ms-package.xps-fixeddocument+xml",
    "application/vnd.ms-package.xps-fixedpage+xml",
    "application/vnd.ms-package.xps-discard-control+xml",
    "application/vnd.ms-package.xps-documentstructure+xml",
    "application/vnd.ms-opentype",
    "application/vnd.ms-color.iccprofile",
    "application/vnd.ms-package.obfuscated-opentype",
    "application/vnd.ms-printing.printticket+xml",
    "application/vnd.ms-package.xps-resourcedictionary+xml",
    "application/vnd.ms-package.xps-storyfragments+xml",
    "image/jpeg",
    "image/png",
    "image/tiff",
    "image/vnd.ms-photo",
    "application/vnd.openxmlformats-package.core-properties+xml",
    "application/vnd.openxmlformats-package.digital-signature-certificate",
    "application/vnd.openxmlformats-package.digital-signature-origin",
    "application/vnd.openxmlformats-package.digital-signature-xmlsignature+xml",
    "application/vnd.openxmlformats-package.relationships+xml",
};

/*++

Routine Name:

    CContentTypes::CContentTypes

Routine Description:

    CContentTypes class constructor

Arguments:

    None

Return Value:

    None

--*/
CContentTypes::CContentTypes()
{
}

/*++

Routine Name:

    CContentTypes::~CContentTypes

Routine Description:

    CContentTypes class destructor

Arguments:

    None

Return Value:

    None

--*/
CContentTypes::~CContentTypes()
{
}

/*++

Routine Name:

    CContentTypes::startElement

Routine Description:

    This routine is the startElement method of the SAX handler. This is used
    to parse the content types part mark-up retrieving and storing the list of
    content types

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
CContentTypes::startElement(
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

        if (cstrElementName == szCTOverride ||
            cstrElementName == szCTDefault)
        {
            CStringXDA cstrPartName;
            CStringXDA cstrPartType;

            INT cAttributes = 0;

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->getLength(&cAttributes);
            }

            //
            // Run over all attributes identifying part names and part types
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

                    if (cstrAttName == szCTPartName ||
                        cstrAttName == szCTExtension)
                    {
                        cstrPartName = cstrAttValue;
                    }
                    else if (cstrAttName == szCTContentType)
                    {
                        cstrPartType = cstrAttValue;
                    }
                }
            }

            if (cstrPartName.GetLength() > 0)
            {
                if (cstrPartType.GetLength() > 0)
                {
                    EContentType contentType;
                    if (SUCCEEDED(hr = GetContentTypeFromString(cstrPartType, &contentType)))
                    {
                        //
                        // Strip any leading "/"
                        //
                        if (cstrPartName.GetAt(0) == '/')
                        {
                            cstrPartName.Delete(0);
                        }

                        try
                        {
                            m_contentMap[cstrPartName] = contentType;
                        }
                        catch (exception& DBG_ONLY(e))
                        {
                            ERR(e.what());
                            hr = E_FAIL;
                        }
                    }
                }
                else
                {
                    RIP("Part name identified with no corresponding type\n");

                    hr = E_FAIL;
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

    CContentTypes::ValidateContentType

Routine Description:

    This routine validates a part against a content type

Arguments:

    szPartName  - The name of the XPS part
    contentType - The content type to validate against

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - When the part is not present
    E_*                 - On error

--*/
HRESULT
CContentTypes::ValidateContentType(
    _In_ PCSTR              szPartName,
    _In_ CONST EContentType contentType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szPartName, E_POINTER)))
    {
        try
        {
            CStringXDA cstrPartName(szPartName);

            if (cstrPartName.GetLength() > 0)
            {
                //
                // Check if the content type is defined for the full part name
                //
                ContentMap::const_iterator iterContents = m_contentMap.find(cstrPartName);
                if (iterContents == m_contentMap.end())
                {
                    //
                    // Check if the type is defined by the extension
                    //
                    CStringXDA partExt(PathFindExtensionA(cstrPartName));

                    //
                    // Strip any leading "/"
                    //
                    if (partExt.GetAt(0) == '/')
                    {
                        partExt.Delete(0);
                    }

                    iterContents = m_contentMap.find(partExt);
                }

                if (iterContents != m_contentMap.end())
                {
                    if (contentType != iterContents->second)
                    {
                        hr = E_FAIL;
                    }
                }
                else
                {
                    hr = E_ELEMENT_NOT_FOUND;
                }
            }
            else
            {
                hr = E_INVALIDARG;
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

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CContentTypes::GetContentTypeFromString

Routine Description:

    This routine retrieves the content type enumeration defining the type from the
    content type string defining the type

Arguments:

    cstrPartType - The string defining the content type
    pContentType - Pointer to the content type that recieves the type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CContentTypes::GetContentTypeFromString(
    _In_  CONST CStringXDA& cstrPartType,
    _Out_ EContentType*     pContentType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pContentType, E_POINTER)))
    {
        *pContentType = ContentUnknown;

        try
        {
            if (cstrPartType.GetLength() > 0)
            {
                for (EContentType contentType = EContentTypeMin;
                     contentType < EContentTypeMax;
                     contentType = static_cast<EContentType>(contentType + 1))
                {
                    if (cstrPartType.CompareNoCase(szContentType[contentType]) == 0)
                    {
                        *pContentType = contentType;
                        break;
                    }
                }

                if (*pContentType == ContentUnknown)
                {
                    ERR("Unrecognized content type string.\n");

                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

