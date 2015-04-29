/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsrels.cpp

Abstract:

   Implementation of the XPS rels part SAX handler. This class is responsible
   for retrieving all the rels parts for a particular rels file and storing
   them for access by the XPS processor.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpsrels.h"

static PCSTR szRelationship = "Relationship";
static PCSTR szTarget       = "Target";
static PCSTR szType         = "Type";

static PCSTR szRels[ERelsTypeMax] = {
    "http://schemas.microsoft.com/xps/2005/06/annotations",
    "http://schemas.microsoft.com/xps/2005/06/signature-definitions",
    "http://schemas.microsoft.com/xps/2005/06/discard-control",
    "http://schemas.microsoft.com/xps/2005/06/documentstructure",
    "http://schemas.microsoft.com/xps/2005/06/printticket",
    "http://schemas.microsoft.com/xps/2005/06/required-resource",
    "http://schemas.microsoft.com/xps/2005/06/restricted-font",
    "http://schemas.microsoft.com/xps/2005/06/fixedrepresentation",
    "http://schemas.microsoft.com/xps/2005/06/storyfragments",
    "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties",
    "http://schemas.openxmlformats.org/package/2006/relationships/digital-signature/signature",
    "http://schemas.openxmlformats.org/package/2006/relationships/digital-signature/certificate",
    "http://schemas.openxmlformats.org/package/2006/relationships/digital-signature/origin",
    "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail",
};

/*++

Routine Name:

    CRels::CRels

Routine Description:

    CRels class constructor

Arguments:

    None

Return Value:

    None

--*/
CRels::CRels()
{
}

/*++

Routine Name:

    CRels::~CRels

Routine Description:

    CRels class destructor

Arguments:

    None

Return Value:

    None

--*/
CRels::~CRels()
{
}

/*++

Routine Name:

    CRels::startElement

Routine Description:

    This routine is the startElement method of the SAX handler. This is used
    to parse a relationships part mark-up retrieving and storing the list of
    relationships

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
CRels::startElement(
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

        if (SUCCEEDED(hr) &&
            cstrElementName == szRelationship)
        {
            CStringXDA cstrTargetName;
            CStringXDA cstrRelsType;

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

                    if (cstrAttName == szTarget)
                    {
                        cstrTargetName = cstrAttValue;
                    }
                    else if (cstrAttName == szType)
                    {
                        cstrRelsType = cstrAttValue;
                    }
                }
            }

            if (cstrTargetName.GetLength() > 0)
            {
                if (cstrRelsType.GetLength() > 0)
                {
                    ERelsType relsType;
                    if (SUCCEEDED(hr = GetRelsTypeFromString(cstrRelsType, &relsType)))
                    {
                        //
                        // Strip any leading "/"
                        //
                        if (cstrTargetName.GetAt(0) == '/')
                        {
                            cstrTargetName.Delete(0);
                        }

                        try
                        {
                            m_relsMap[m_cstrCurrentFileName].push_back(RelsNameType(cstrTargetName, relsType));
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
                    RIP("Target name identified with no corresponding type\n");

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

    CRels::SetCurrentFileName

Routine Description:

    This routine sets the filename of the part for which the relationships
    are being identified

Arguments:

    szFileName - The name of the XPS part

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRels::SetCurrentFileName(
    _In_ PCSTR szFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)))
    {
        try
        {
            m_cstrCurrentFileName = szFileName;
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

    CRels::GetRelsTypeList

Routine Description:

    This routine retrieves the list of the relationships for the specified
    part name

Arguments:

    szFileName - The name of the XPS part to retrieve the relationships list for
    ppFileList - Pointer to a RelsTypeList pointer that recieves the relationships list

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRels::GetRelsTypeList(
    _In_        PCSTR                szFileName,
    _Outptr_    CONST RelsTypeList** ppFileList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppFileList, E_POINTER)))
    {
        try
        {
            RelsMap::const_iterator iterRels = m_relsMap.find(szFileName);
            if (iterRels != m_relsMap.end())
            {
                *ppFileList = &(iterRels->second);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
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

    CRels::GetRelsTypeFromString

Routine Description:

    This routine retrieves the relationship enumeration defining the type from the
    relationships string defining the type

Arguments:

    cstrRelsType - The string defining the rels type
    pRelsType    - Pointer to the rels type that recieves the type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRels::GetRelsTypeFromString(
    _In_  CONST CStringXDA& cstrRelsType,
    _Out_ ERelsType*        pRelsType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pRelsType, E_POINTER)))
    {
        *pRelsType = RelsUnknown;

        try
        {
            if (cstrRelsType.GetLength() > 0)
            {
                for (ERelsType relsType = ERelsTypeMin;
                     relsType < ERelsTypeMax;
                     relsType = static_cast<ERelsType>(relsType + 1))
                {
                    if (cstrRelsType.CompareNoCase(szRels[relsType]) == 0)
                    {
                        *pRelsType = relsType;
                        break;
                    }
                }

                if (*pRelsType == RelsUnknown)
                {
                    ERR("Unrecognized relationships type string.\n");

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
