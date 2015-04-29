/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

    pshndlr.cpp

Abstract:

    Base PrintSchema Document handler class implementation.
    This class provides common PrintTicket / PrintCapabilities handling functionality.
    A class can derive from this base class to get PrintTicket/PrintCapabilities
    unspecific XML handling functionality.

    Note: The PrintSchema handler code is only intended to work with the sdtandard
    public PrintSchema keywords.

--*/


//
// Note on handling missing DOM nodes:
//
// Convert MSXML's S_FALSE to E_ELEMENT_NOT_FOUND. This allows clients to
// use the SUCCEEDED macro more effectively.
//
// E_ELEMENT_NOT_FOUND should not be propogated as an error to the
// filter pipeline or config module - treat as though the requested feature
// has not been enabled.
//

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "pshndlr.h"

using XDPrintSchema::FRAMEWORK_URI;
using XDPrintSchema::KEYWORDS_URI;
using XDPrintSchema::SCHEMA_INST_URI;
using XDPrintSchema::SCHEMA_DEF_URI;
using XDPrintSchema::PROPERTY_ELEMENT_NAME;
using XDPrintSchema::NAME_ATTRIBUTE_NAME;
using XDPrintSchema::VALUE_ELEMENT_NAME;
using XDPrintSchema::SCHEMA_TYPE;
using XDPrintSchema::SCORED_PROP_ELEMENT_NAME;
using XDPrintSchema::SCHEMA_STRING;
using XDPrintSchema::SCHEMA_INTEGER;
using XDPrintSchema::SCHEMA_DECIMAL;
using XDPrintSchema::PARAM_REF_ELEMENT_NAME;
using XDPrintSchema::PARAM_INIT_ELEMENT_NAME;
using XDPrintSchema::SCHEMA_INST_URI;

static LPCWSTR szSelectNS               = L"SelectionNamespaces";
static LPCWSTR szTmpNS                  = L"psf";
static LPCWSTR szNSSelection            = L"xmlns:%s='%s'";
static LPCWSTR szSelectLang             = L"SelectionLanguage";
static LPCWSTR szLangSection            = L"XPath";

static LPCWSTR szPTRootQuery            = L"//%s:%s";

/*++

Routine Name:

    CPSHandler::CPSHandler

Routine Description:

    CPSHandler class constructor

Arguments:

    pDOMDocument - Pointer to the DOM document representation of the PrintTicket/PrintCapabilities

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CPSHandler::CPSHandler(
    _In_z_ BSTR bstrDocumentType,
    _In_ IXMLDOMDocument2 *pPrintDocument
    ) :
    m_bstrDocumentType(bstrDocumentType),
    m_pPrintDocument(pPrintDocument)
{
    ASSERTMSG(m_pPrintDocument != NULL, "NULL PrintDocument passed to PS manager.\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pPrintDocument, E_PENDING)))
    {
        try
        {
            //
            // Make sure XPath selection language is set
            //
            if (SUCCEEDED(hr = m_pPrintDocument->setProperty(CComBSTR(szSelectLang), CComVariant(szLangSection))))
            {
                //
                // Construct the alias namespace string
                //
                CStringXDW cstrNamespaces;
                cstrNamespaces.Format(szNSSelection, szTmpNS, FRAMEWORK_URI);

                //
                // Specify an alias PrintSchema namespace prefix to allow us to find the real ones
                //
                if (SUCCEEDED(hr = m_pPrintDocument->setProperty(CComBSTR(szSelectNS), CComVariant(cstrNamespaces))))
                {
                    if (SUCCEEDED(hr = GetPrefixFromURI(CComBSTR(FRAMEWORK_URI), &m_bstrFrameworkPrefix)) &&
                        SUCCEEDED(hr = GetPrefixFromURI(CComBSTR(KEYWORDS_URI), &m_bstrKeywordsPrefix)) &&
                        SUCCEEDED(hr = GetPrefixFromURI(CComBSTR(SCHEMA_INST_URI), &m_bstrSchemaInstPrefix)) &&
                        SUCCEEDED(hr = GetPrefixFromURI(CComBSTR(SCHEMA_DEF_URI), &m_bstrSchemaPrefix)))
                    {
                        CStringXDW cstrNSUserPrefix(L"ns0000:");
                        m_bstrUserKeywordsPrefix.Empty();
                        m_bstrUserKeywordsPrefix.Attach(cstrNSUserPrefix.AllocSysString());

                        //
                        // Restore the original namespace prefix for the PrintSchema framework
                        //
                        CComBSTR bstrOrgNamespaces;
                        CStringXDW cstrNSPrefix(m_bstrFrameworkPrefix);

                        INT iIndex = cstrNSPrefix.Find(L":");
                        if (iIndex != -1 &&
                            cstrNSPrefix.Delete(iIndex, 1) > 0)
                        {
                            CStringXDW cstrOrgNamespaces;
                            cstrOrgNamespaces.Format(szNSSelection, cstrNSPrefix, FRAMEWORK_URI);

                            hr = m_pPrintDocument->setProperty(CComBSTR(szSelectNS), CComVariant(cstrOrgNamespaces));
                        }
                        else
                        {
                             RIP("Could not create namespace prefix correctly\n");
                             hr = E_FAIL;
                        }
                    }
                }
                else
                {
                    ERR("Failed to set SelectionNamespaces.\n");
                }
            }
            else
            {
                RIP("Failed to set SelectionLanguage.\n");
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CPSHandler::~CPSHandler

Routine Description:

    CPSHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPSHandler::~CPSHandler()
{
}

/*++

Routine Name:

    CPSHandler::DeleteNode

Routine Description:

    This routine deletes the given node from the PrintDocument

Arguments:

    pNode - Pointer to the DOM node to be deleted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::DeleteNode(
    _In_ IXMLDOMNode* pNode
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNode, E_POINTER)))
    {
        CComPtr<IXMLDOMNode> pParentNode(NULL);
        CComPtr<IXMLDOMNode> pDeletedNode(NULL);

        if (SUCCEEDED(hr = pNode->get_parentNode(&pParentNode)) &&
            hr != S_FALSE &&
            SUCCEEDED(hr = pParentNode->removeChild(pNode, &pDeletedNode)) &&
            pDeletedNode == NULL)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::GetNode

Routine Description:

    This routine retrieves a node given an XPath query

Arguments:

    bstrNodeQuery - The XPath query for a node
    ppNode        - Pointer to an IXMLDOMNode pointer that recieves the node retrieved

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::GetNode(
    _In_z_      BSTR          bstrNodeQuery,
    _Outptr_ IXMLDOMNode** ppNode
    )
{
    //
    // We should have the PrintDocment in place
    //
    ASSERTMSG(m_pPrintDocument != NULL, "NULL PS detected whilst retrieving node.\n");

    //
    // Method which takes a query string and returns the first node that matches
    //
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppNode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pPrintDocument, E_PENDING)))
    {
        *ppNode = NULL;
        if (SysStringLen(bstrNodeQuery) == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the node from the PrintDocument
        //
        hr = m_pPrintDocument->selectSingleNode(bstrNodeQuery, ppNode);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::DeletePrivateFeatures

Routine Description:

    This routine finds and deletes features with values defined in the
    private namespace passed in

Arguments:

    bstrPrivateNS - the private namespace

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::DeletePrivateFeatures(
    _In_z_ BSTR bstrPrivateNS
    )
{
    HRESULT hr = S_OK;

    if (SysStringLen(bstrPrivateNS) > 0)
    {
        CComBSTR bstrNSPrefix;
        if (SUCCEEDED(hr = GetPrefixFromURI(bstrPrivateNS, &bstrNSPrefix)))
        {
            //
            // Find all root feature elements in the namespace
            //
            CComPtr<IXMLDOMNodeList> pNodeList(NULL);

            CComBSTR bstrFeatureQuery(L"//");
            bstrFeatureQuery += m_bstrFrameworkPrefix;
            bstrFeatureQuery += L"Feature";

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = m_pPrintDocument->selectNodes(bstrFeatureQuery, &pNodeList)) &&
                SUCCEEDED(hr = pNodeList->reset()))
            {
                //
                // Delete all the feature with name attributes using the private namespace
                //
                CComPtr<IXMLDOMNode> pDeleteNode(NULL);

                while (SUCCEEDED(hr) &&
                       SUCCEEDED(hr = pNodeList->nextNode(&pDeleteNode)) &&
                       hr != S_FALSE)
                {
                    CComPtr<IXMLDOMNamedNodeMap> pDeleteNodeAtts(NULL);
                    CComPtr<IXMLDOMNode>         pNameNode(NULL);
                    CComVariant                  varNameValue;

                    if (SUCCEEDED(hr = pDeleteNode->get_attributes(&pDeleteNodeAtts)) &&
                        hr != S_FALSE &&
                        SUCCEEDED(hr = pDeleteNodeAtts->getNamedItem(CComBSTR(L"name"), &pNameNode)) &&
                        hr != S_FALSE &&
                        SUCCEEDED(hr = pNameNode->get_nodeValue(&varNameValue)) &&
                        hr != S_FALSE)
                    {
                        try
                        {
                            CStringXDW cstrNameValue(varNameValue.bstrVal);

                            if (cstrNameValue.Find(bstrNSPrefix) == 0)
                            {
                                hr = DeleteNode(pDeleteNode);
                            }
                        }
                        catch (CXDException& e)
                        {
                            hr = e;
                        }
                    }

                    pDeleteNode = NULL;
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateProperty

Routine Description:

    This routine creates a property element of the given name. Note: this
    method only creates the property element; it is up to the caller to
    set the property value

Arguments:

    bstrPropName  - The name of the property
    ppPropElement - Pointer to a IXMLDOMElement pointer that recieves the newly created Property element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateProperty(
    _In_        CONST BSTR       bstrPropName,
    _Outptr_ IXMLDOMElement** ppPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPropElement, E_POINTER)))
    {
        *ppPropElement = NULL;

        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrTagName(m_bstrFrameworkPrefix);
        bstrTagName += PROPERTY_ELEMENT_NAME;

        CComBSTR bstrAttribName(szTmpNS);
        bstrAttribName += L":";
        bstrAttribName += bstrPropName;

        hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, ppPropElement);

        if (SUCCEEDED(hr))
        {
            if (*ppPropElement != NULL)
            {
                hr = CreateXMLAttribute(*ppPropElement, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateProperty

Routine Description:

    This routine creates a property element of the given name.

Arguments:

    bstrPropName  - The name of the property to be created
    bstrType      - The type of the property value (integer, string etc.)
    bstrValue     - The value of the property
    ppPropElement - Pointer to an IXMLDOMElement pointer that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST BSTR       bstrType,
    _In_        CONST BSTR       bstrValue,
    _Outptr_ IXMLDOMElement** ppPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0 ||
            SysStringLen(bstrType) <= 0 ||
            SysStringLen(bstrValue) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the scored property and value node
        //
        if (SUCCEEDED(hr = CreateProperty(bstrPropName, ppPropElement)))
        {
            CComPtr<IXMLDOMElement> pValue(NULL);

            CComBSTR bstrValueElement(m_bstrFrameworkPrefix);
            bstrValueElement += VALUE_ELEMENT_NAME;

            CComBSTR bstrTypeAttribName(m_bstrSchemaInstPrefix);
            bstrTypeAttribName += SCHEMA_TYPE;

            CComBSTR bstrTypeAttribValue(m_bstrSchemaPrefix);
            bstrTypeAttribValue += bstrType;

            hr = CreateXMLElement(bstrValueElement, FRAMEWORK_URI, &pValue);

            if(SUCCEEDED(hr))
            {
                if (SUCCEEDED(hr = CreateXMLAttribute(pValue, bstrTypeAttribName, SCHEMA_INST_URI, bstrTypeAttribValue )) &&
                    SUCCEEDED(hr = pValue->put_text(bstrValue)))
                {
                    hr = (*ppPropElement)->appendChild(pValue, NULL);
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateFWProperty

Routine Description:

    This routine creates a property element of the given name. Note: this
    method only creates the property element; it is up to the caller to
    set the property value

Arguments:

    bstrPropName  - The name of the property
    ppPropElement - Pointer to a IXMLDOMElement pointer that recieves the newly created Property element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateFWProperty(
    _In_        CONST BSTR       bstrPropName,
    _Outptr_ IXMLDOMElement** ppPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPropElement, E_POINTER)))
    {
        *ppPropElement = NULL;

        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrTagName(m_bstrFrameworkPrefix);
        bstrTagName += PROPERTY_ELEMENT_NAME;

        CComBSTR bstrAttribName(m_bstrFrameworkPrefix);
        bstrAttribName += bstrPropName;

        hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, ppPropElement);

        if (SUCCEEDED(hr))
        {
            if (*ppPropElement != NULL)
            {
                hr = CreateXMLAttribute(*ppPropElement, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateFWProperty

Routine Description:

    This routine creates a property element of the given name.

Arguments:

    bstrPropName  - The name of the property to be created
    bstrType      - The type of the property value (integer, string etc.)
    bstrValue     - The value of the property
    ppPropElement - Pointer to an IXMLDOMElement pointer that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateFWProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST BSTR       bstrType,
    _In_        CONST BSTR       bstrValue,
    _Outptr_ IXMLDOMElement** ppPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0 ||
            SysStringLen(bstrType) <= 0 ||
            SysStringLen(bstrValue) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the scored property and value node
        //
        if (SUCCEEDED(hr = CreateFWProperty(bstrPropName, ppPropElement)))
        {
            CComPtr<IXMLDOMElement> pValue(NULL);

            CComBSTR bstrValueElement(m_bstrFrameworkPrefix);
            bstrValueElement += VALUE_ELEMENT_NAME;

            CComBSTR bstrTypeAttribName(m_bstrSchemaInstPrefix);
            bstrTypeAttribName += SCHEMA_TYPE;

            CComBSTR bstrTypeAttribValue(m_bstrSchemaPrefix);
            bstrTypeAttribValue += bstrType;

            hr = CreateXMLElement(bstrValueElement, FRAMEWORK_URI, &pValue);

            if(SUCCEEDED(hr))
            {
                if (SUCCEEDED(hr = CreateXMLAttribute(pValue, bstrTypeAttribName, SCHEMA_INST_URI, bstrTypeAttribValue )) &&
                    SUCCEEDED(hr = pValue->put_text(bstrValue)))
                {
                    hr = (*ppPropElement)->appendChild(pValue, NULL);
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property. Note: this does not intialise
    the scored property value - this is the responsibility of the caller

Arguments:

    bstrPropName        - The name of the property element to be created
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        *ppScoredPropElement = NULL;

        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrTagName(m_bstrFrameworkPrefix);
        bstrTagName += SCORED_PROP_ELEMENT_NAME;

        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += bstrPropName;

        hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, ppScoredPropElement);

        if (SUCCEEDED(hr))
        {
            if (*ppScoredPropElement != NULL)
            {
                hr = CreateXMLAttribute(*ppScoredPropElement, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName );
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property.

Arguments:

    bstrPropName        - The name of the property element to be created
    bstrType            - The type of the value to be created (integer, string etc.)
    bstrValue           - The value to be set as a string
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST BSTR       bstrType,
    _In_        CONST BSTR       bstrValue,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0 ||
            SysStringLen(bstrType) <= 0 ||
            SysStringLen(bstrValue) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the scored property and value node
        //
        if (SUCCEEDED(hr = CreateScoredProperty(bstrPropName, ppScoredPropElement)))
        {
            CComPtr<IXMLDOMElement> pValue(NULL);

            CComBSTR bstrValueElement(m_bstrFrameworkPrefix);
            bstrValueElement += VALUE_ELEMENT_NAME;

            CComBSTR bstrTypeAttribName(m_bstrSchemaInstPrefix);
            bstrTypeAttribName += SCHEMA_TYPE;

            CComBSTR bstrTypeAttribValue(m_bstrSchemaPrefix);
            bstrTypeAttribValue += bstrType;

            hr = CreateXMLElement(bstrValueElement, FRAMEWORK_URI, &pValue);

            if(SUCCEEDED(hr))
            {
                if (SUCCEEDED(hr = CreateXMLAttribute(pValue, bstrTypeAttribName, SCHEMA_INST_URI, bstrTypeAttribValue )) &&
                    SUCCEEDED(hr = pValue->put_text(bstrValue)))
                {
                    hr = (*ppScoredPropElement)->appendChild(pValue, NULL);
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property of type string.

Arguments:

    bstrPropName        - The name of the property element to be created
    bstrValue           - The string value to be set
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST BSTR       bstrValue,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) > 0 &&
            SysStringLen(bstrValue) > 0)
        {
            //
            // Create the scored property and value node
            //
            hr = CreateScoredProperty(bstrPropName, CComBSTR(SCHEMA_STRING), bstrValue, ppScoredPropElement);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property of type INT.

Arguments:

    bstrPropName        - The name of the property element to be created
    intValue            - The INT value to be set
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST INT        intValue,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            //
            // Construct the value string
            //
            CStringXDW cstrValue;
            cstrValue.Format(L"%i", intValue);

            //
            // Create the scored property and value node
            //
            hr = CreateScoredProperty(bstrPropName,
                                      CComBSTR(SCHEMA_INTEGER),
                                      CComBSTR(cstrValue.AllocSysString()),
                                      ppScoredPropElement);
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

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property of type REAL.

Arguments:

    bstrPropName        - The name of the property element to be created
    realValue           - The REAL value to be set
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        CONST REAL       realValue,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            //
            // Construct the value string
            //
            CStringXDW cstrValue;
            cstrValue.Format(L"%.2f", realValue);

            //
            // Create the scored property and value node
            //
            hr = CreateScoredProperty(bstrPropName,
                                      CComBSTR(SCHEMA_DECIMAL),
                                      CComBSTR(cstrValue.AllocSysString()),
                                      ppScoredPropElement);
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

    CPSHandler::QueryNode

Routine Description:

    This routine checks for the existence of a node given the XPath
    query passed in

Arguments:

    bstrQuery - The XPath query defining the node or nodes to be located

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPSHandler::QueryNode(
    _In_z_ BSTR  bstrQuery
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SysStringLen(bstrQuery) > 0)
    {
        CComPtr<IXMLDOMNode> pNode(NULL);

        hr = GetNode(bstrQuery, &pNode);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (hr == S_FALSE)
    {
        hr = E_ELEMENT_NOT_FOUND;
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPSHandler::QueryNodeValue

Routine Description:

    This routine locates the node specified by the XPath query passed in and
    returns the value of the "value" attribute as a string.

Arguments:

    bstrQuery   - The XPath query locating the node to retrieve the value for
    pbstrOption - Pointer to a BSTR to recieve the value string

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPSHandler::QueryNodeValue(
    _In_z_      BSTR  bstrQuery,
    _Outptr_ BSTR* pbstrValue
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        *pbstrValue = NULL;

        if (SysStringLen(bstrQuery) > 0)
        {
            hr = GetNodeValue(bstrQuery, pbstrValue);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (hr == S_FALSE)
    {
        hr = E_ELEMENT_NOT_FOUND;
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPSHandler::QueryNodeValue

Routine Description:

    This routine locates the node specified by the XPath query passed in and
    returns the value of the "value" attribute as an INT.

Arguments:

    bstrQuery - The XPath query locating the node to retrieve the value for
    pValue    - Pointer to an INT to recieve the value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::QueryNodeValue(
    _In_z_ BSTR bstrQuery,
    _Out_  INT* pValue
    )
{
    HRESULT hr = S_OK;

    CComBSTR bstrValue;
    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (SysStringLen(bstrQuery) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = QueryNodeValue(bstrQuery, &bstrValue)))
    {
        *pValue = static_cast<INT>(_wtoi(bstrValue));
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPSHandler::QueryNodeValue

Routine Description:

    This routine locates the node specified by the XPath query passed in and
    returns the value of the "value" attribute as a REAL.

Arguments:

    bstrQuery - The XPath query locating the node to retrieve the value for
    pValue    - Pointer to a REAL to recieve the value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::QueryNodeValue(
    _In_z_ BSTR bstrQuery,
    _Out_  REAL* pValue
    )
{
    HRESULT hr = S_OK;

    CComBSTR bstrValue;
    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (SysStringLen(bstrQuery) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = QueryNodeValue(bstrQuery, &bstrValue)))
    {
        *pValue = static_cast<REAL>(_wtof(bstrValue));
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPSHandler::GetNodeValue

Routine Description:

    This routine retrieves the value of a node given the XPath query to the node

Arguments:

    bstrNodeQuery - The XPath query for the node from which the value should be retrieved
    pbstrValue    - Pointer to ta BSTR that recieves the value of the node

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

Note:

    The annotation for pbstrValue is intended to express that the BSTR may
    be NULL when S_FALSE is returned, but not when S_OK is returned.
    
    In any case, when a failure HRESULT is returned, all out parameter
    annotations are ignored.

--*/
HRESULT
CPSHandler::GetNodeValue(
    _In_z_      BSTR  bstrNodeQuery,
    _Inout_ _At_(*pbstrValue, _Pre_maybenull_)
    _When_(return == S_FALSE, _At_(*pbstrValue, _Post_maybenull_))
    _When_(return != S_FALSE, _At_(*pbstrValue, _Post_valid_))
                BSTR* pbstrValue
    )
{
    HRESULT hr = S_OK;

    CComPtr<IXMLDOMNode> pFeatureNode(NULL);

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        SysFreeString(*pbstrValue);
        *pbstrValue = NULL;
        if (SysStringLen(bstrNodeQuery) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetNode(bstrNodeQuery, &pFeatureNode)) &&
        hr != S_FALSE)
    {
        //
        // Scored properties can either be defined as "Value" or "ParameterRef"
        // Find out which of these applies to this property and handle accordingly
        //
        CComBSTR bstrValue(m_bstrFrameworkPrefix);
        bstrValue += VALUE_ELEMENT_NAME;

        CComBSTR bstrParamRef(m_bstrFrameworkPrefix);
        bstrParamRef += PARAM_REF_ELEMENT_NAME;

        CComVariant varValue;

        CComPtr<IXMLDOMNode> pPropertyNode(NULL);
        if (SUCCEEDED(hr = pFeatureNode->selectSingleNode(bstrValue, &pPropertyNode)) &&
            hr != S_FALSE)
        {
            //
            // Value node. Just retrieve the node value
            //
            hr = pPropertyNode->get_nodeTypedValue(&varValue);
        }
        else if (SUCCEEDED(hr = pFeatureNode->selectSingleNode(bstrParamRef, &pPropertyNode)) &&
                 hr != S_FALSE)
        {
            //
            // Property defined by parameter ref. Retrieve the name and look up the value
            // from the parameter init elements
            //
            CComPtr<IXMLDOMNodeList> pParamInitList(NULL);
            CComBSTR bstrParamRefValue;

            if (SUCCEEDED(hr = GetAttributeValue(pPropertyNode, CComBSTR(NAME_ATTRIBUTE_NAME), &bstrParamRefValue)) &&
                hr != S_FALSE &&
                SUCCEEDED(hr = GetNodes(CComBSTR(PARAM_INIT_ELEMENT_NAME), &pParamInitList)) &&
                hr != S_FALSE)
            {
                CComBSTR bstrParamInit;
                CComPtr<IXMLDOMNode> pInitNode(NULL);

                hr = pParamInitList->reset();

                while (SUCCEEDED(hr) &&
                       hr != S_FALSE)
                {
                    if (SUCCEEDED(hr = pParamInitList->nextNode(&pInitNode)) && hr != S_FALSE && pInitNode != NULL)
                    {
                        hr = GetAttributeValue(pInitNode, CComBSTR(NAME_ATTRIBUTE_NAME), &bstrParamInit);
                        if (SUCCEEDED(hr) && bstrParamInit == bstrParamRefValue)
                        {
                            hr = pInitNode->get_nodeTypedValue(&varValue);
                            break;
                        }
                    }


                    //
                    // Release the node and name before getting the next
                    //
                    pInitNode = NULL;
                    bstrParamInit.Empty();
                }
            }
        }

        if (SUCCEEDED(hr) &&
            hr != S_FALSE &&
            SysStringLen(varValue.bstrVal) > 0)
        {
            *pbstrValue = ::SysAllocString(varValue.bstrVal);

            if (*pbstrValue == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::GetAttributeValue

Routine Description:

    This routine retrieves the value of a named attribute as a string from a DOM node

Arguments:

    pNode          - Pointer to the DOM node to retrieve the attribute value from
    bstrAttribName - The name of the attribute
    pbstrResult    - Pointer to a BSTR that recieves the attribute value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::GetAttributeValue(
    _In_            CONST IXMLDOMNode* pNode,
    _In_z_          BSTR               bstrAttribName,
    _Outptr_result_maybenull_ BSTR*              pbstrResult
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrResult, E_POINTER)))
    {
        if (SysStringLen(bstrAttribName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pbstrResult = NULL;

        //
        // Map the associated attributes and get the string result
        //
        CComPtr<IXMLDOMNamedNodeMap> pIXMLDOMNamedNodeMap(NULL);
        CComPtr<IXMLDOMNode>         pSubNode(NULL);

        if (SUCCEEDED(hr = const_cast<IXMLDOMNode*>(pNode)->get_attributes(&pIXMLDOMNamedNodeMap)) &&
            hr != S_FALSE &&
            SUCCEEDED(hr = pIXMLDOMNamedNodeMap->getNamedItem(bstrAttribName, &pSubNode)) &&
            hr != S_FALSE)
        {
            hr = pSubNode->get_text(pbstrResult);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::GetNodes

Routine Description:

    This routine returns a list of all the nodes in the PrintTicket with the
    specified element name

Arguments:

    ppParamInit - Pointer to an IXMLDOMNodeList pointer that recieves the node list

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::GetNodes(
    _In_        BSTR              bstrElementName,
    _Outptr_ IXMLDOMNodeList** ppNodeList
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppNodeList, E_POINTER)))
    {
        if (SysStringLen(bstrElementName) > 0)
        {
            *ppNodeList = NULL;

            //
            // Retrieve all psf:ParameterInit nodes in the PrintTicket
            //
            CComBSTR bstrParamInitQuery(L"//");
            bstrParamInitQuery += m_bstrFrameworkPrefix;
            bstrParamInitQuery += bstrElementName;

            hr = m_pPrintDocument->selectNodes(bstrParamInitQuery, ppNodeList);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::CreateScoredProperty

Routine Description:

    This routine creates a scored property using the value node passed in.

Arguments:

    bstrPropName        - The name of the property element to be created
    pValueNode          - Pointer to the IXMLDOMNode that represents the scored property value
    ppScoredPropElement - Pointer to an IXMLDOMElement that recieves the new element

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::CreateScoredProperty(
    _In_        CONST BSTR       bstrPropName,
    _In_        IXMLDOMNode*     pValueNode,
    _Outptr_ IXMLDOMElement** ppScoredPropElement
    )
{
    HRESULT hr = S_OK;

    //
    // Validate parameters then create the scored property and value node
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pValueNode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppScoredPropElement, E_POINTER)))
    {
        if (SysStringLen(bstrPropName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CreateScoredProperty(bstrPropName, ppScoredPropElement)))
    {
        hr = (*ppScoredPropElement)->appendChild(pValueNode, NULL);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPSHandler::GetPrefixFromURI

Routine Description:

    Obtains the Prefix associated with a Namespace URI from an XML PrintSchema Document.

Arguments:

    bstrNamespaceURI - Namespace URI to be used in look-up.
    bstrNamespacePrefix - Namespace Prefix to be returned.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPSHandler::GetPrefixFromURI(
    _In_z_                    BSTR  bstrNamespaceURI,
    _Outptr_result_maybenull_ BSTR* bstrNamespacePrefix
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(bstrNamespacePrefix, E_POINTER)))
    {
        if (SysStringLen(bstrNamespaceURI) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        *bstrNamespacePrefix = NULL;

        try
        {
            //
            // Construct the Root Query string
            //
            CStringXDW cstrRootQuery;
            cstrRootQuery.Format(szPTRootQuery, szTmpNS, m_bstrDocumentType);

            //
            // Find the PrintSchema element so we can retrieve the private namespace prefix
            //
            CComPtr<IXMLDOMNode>         pPTNode(NULL);
            CComPtr<IXMLDOMNamedNodeMap> pPTAtts(NULL);

            if (SUCCEEDED(hr = GetNode(CComBSTR(cstrRootQuery), &pPTNode)) &&
                hr != S_FALSE &&
                SUCCEEDED(hr = pPTNode->get_attributes(&pPTAtts)) &&
                SUCCEEDED(hr = pPTAtts->reset()))
            {
                //
                // Iterate over all attributes and match the node value against
                // the namespace URI
                //
                BOOL bMatched = FALSE;
                CComPtr<IXMLDOMNode> pAttNode(NULL);

                while (SUCCEEDED(hr) &&
                       !bMatched &&
                       SUCCEEDED(hr = pPTAtts->nextNode(&pAttNode)) &&
                       hr != S_FALSE)
                {
                    CComVariant varValue;

                    if (SUCCEEDED(hr = pAttNode->get_nodeValue(&varValue)))
                    {
                        if (CComBSTR(varValue.bstrVal) == bstrNamespaceURI)
                        {
                            bMatched = TRUE;
                        }
                    }

                    if (!bMatched)
                    {
                        //
                        // No match - free the attribute node before retrieving the next
                        //
                        pAttNode = NULL;
                    }
                }

                if (SUCCEEDED(hr) &&
                    bMatched)
                {
                    //
                    // If we match then get the node name and strip xmlns: to derive
                    // the private namespace prefix
                    //
                    CStringXDW cstrNSPrefix;
                    CComBSTR bstrNodeName;

                    if (SUCCEEDED(hr = pAttNode->get_nodeName(&bstrNodeName)))
                    {
                         cstrNSPrefix = bstrNodeName;
                         CStringXDW cstrXMLNS(L"xmlns:");

                         if (cstrNSPrefix.Find(cstrXMLNS) != 0 ||
                             cstrNSPrefix.Delete(0, cstrXMLNS.GetLength()) <= 0)
                         {
                             ERR("Could not create private namespace prefix correctly\n");

                             hr = E_FAIL;
                         }
                         else
                         {
                             cstrNSPrefix += L":";

                             SysFreeString(*bstrNamespacePrefix);
                             *bstrNamespacePrefix = cstrNSPrefix.AllocSysString();
                         }
                    }
                }
            }
            else
            {
                ERR("Failed to find the PC root element.\n");
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

HRESULT
CPSHandler::CreateXMLAttribute(
    _Inout_    IXMLDOMElement *pTarget,
    _In_       PCWSTR pszName,
    _In_opt_   PCWSTR pszTargetURI,
    _In_       PCWSTR pszValue
    )
/*++

Routine Description:

    This routine adds a new attribute to the given XML element.  The URI
    parameter must be present, but may be an empty string.

    If this routine fails, the document being constructed should be
    considered invalid, and be thrown out in its entirity.

    The newly created attribute will be appended to the list of attributes on the element

Arguments:

    pTarget - the element to which the attribute is to be added
    pszName - the name of the attribtue
    pszTargetURI - the URI in which the attribute should reside
    pszValue - the value to put in the attribute

Returns:

    S_OK on success, else
    E_* on failure

--*/
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMAttribute> pCurrentAttr;
    CComPtr<IXMLDOMNode> pCurrentNode;

    if( !( pTarget && pszName && pszValue ) )
    {
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        VARIANT type;
        VariantInit( &type );
        V_VT(&type) = VT_I4;
        V_I4(&type) = NODE_ATTRIBUTE;

        hr = m_pPrintDocument->createNode(type, const_cast<BSTR>(pszName), const_cast<BSTR>(pszTargetURI), &pCurrentNode);

        VariantClear( &type );
    }

    if( SUCCEEDED(hr) )
    {
        hr = pCurrentNode->QueryInterface( IID_IXMLDOMAttribute, (void**)&pCurrentAttr );
    }

    if( SUCCEEDED(hr) )
    {
        VARIANT attrVal;
        BSTR bstrValue = SysAllocString(pszValue);

        if( bstrValue )
        {
            VariantInit(&attrVal);
            V_VT(&attrVal) = VT_BSTR;
            V_BSTR(&attrVal) = bstrValue;
            hr = pCurrentAttr->put_value(attrVal);

            if( SUCCEEDED(hr) )
            {
                hr = VariantClear(&attrVal);
            }
            else
            {
                VariantClear(&attrVal);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if( SUCCEEDED(hr) )
    {
        hr = pTarget->setAttributeNode( pCurrentAttr, NULL );
    }

    return hr;
}

HRESULT
CPSHandler::CreateXMLElement(
    _In_       PCWSTR pszName,
    _In_       PCWSTR pszTargetURI,
    _Out_opt_  IXMLDOMElement **ppEl
    )
/*++

Routine Description:

    Create a new DOM element using the given QName and URI.  Implemented using DOMDocument->CreateNode.

Arguments:

    pszName - The QName of the element to be created
    pszTargetUri - The namespace in which the created element lives.  The
        caller does not have control over the prefix... just the URI
    ppEl - If the caller needs a pointer to the newly created node, this should
        be a non-null

Return Value:

    S_OK on success,
    E_* on failure.  Common values that would be expected
        include E_OUTOFMEMORY, and E_INVALIDARG.

--*/
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMNode> pCurrentNode;

    if( !( pszName && pszTargetURI ) )
    {
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        VARIANT type;
        VariantInit( &type );
        V_VT(&type) = VT_I4;
        V_I4(&type) = NODE_ELEMENT;
        hr = m_pPrintDocument->createNode( type, const_cast<BSTR>(pszName), const_cast<BSTR>(pszTargetURI), &pCurrentNode );
        VariantClear( &type );
    }

    //
    // Only give the client back the new value element if we succeed.
    //
    if( SUCCEEDED(hr) && ppEl )
    {
        hr = pCurrentNode->QueryInterface( IID_IXMLDOMElement, (void**)ppEl );
    }

    return hr;
}
