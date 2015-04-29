/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

    pthndlr.cpp

Abstract:

    Base PrintTicket handler class implementation. This class provides common
    PrintTicket handling functionality for any filter that requires print
    ticket handling. A feature specific handler can derive from
    this class to get feature unspecific XML handling functionality.

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
#include "pthndlr.h"
#include "ptquerybld.h"

using XDPrintSchema::PRINTTICKET_NAME;
using XDPrintSchema::PARAM_INIT_ELEMENT_NAME;
using XDPrintSchema::PARAM_REF_ELEMENT_NAME;
using XDPrintSchema::NAME_ATTRIBUTE_NAME;
using XDPrintSchema::VALUE_ELEMENT_NAME;
using XDPrintSchema::SCHEMA_TYPE;
using XDPrintSchema::SCHEMA_INTEGER;
using XDPrintSchema::FEATURE_ELEMENT_NAME;
using XDPrintSchema::OPTION_ELEMENT_NAME;
using XDPrintSchema::FRAMEWORK_URI;
using XDPrintSchema::SCHEMA_INST_URI;

/*++

Routine Name:

    CPTHandler::CPTHandler

Routine Description:

    CPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

    Note: Base Class (CPSHandler) - Throws CXDException(HRESULT) on an error

--*/
CPTHandler::CPTHandler(
    _In_ IXMLDOMDocument2 *pDOMDocument
    ) :
    CPSHandler(CComBSTR(PRINTTICKET_NAME), pDOMDocument)
{
}

/*++

Routine Name:

    CPTHandler::~CPTHandler

Routine Description:

    CPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPTHandler::~CPTHandler()
{
}

/*++

Routine Name:

    CPTHandler::DeleteFeature

Routine Description:

    This routine finds and deletes the named feature

Arguments:

    bstrFeature - the feature name to be deleted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::DeleteFeature(
    _In_z_ BSTR bstrFeature
    )
{
    HRESULT hr = S_OK;

    if (SysStringLen(bstrFeature) > 0)
    {
        //
        // Find the feature node
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        CComPtr<IXMLDOMNode> pFeatureNode(NULL);

        if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrFeature)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)))
        {
            //
            // Keep querying and deleting till all instances of the feature are
            // removed - generally this should be a single instance
            //
            while (SUCCEEDED(hr) &&
                   SUCCEEDED(hr = GetNode(bstrQuery, &pFeatureNode)) &&
                   hr != S_FALSE)
            {
                //
                // Delete the feature node and all children, then locate and delete
                // all orphaned parameter init nodes
                //
                if (SUCCEEDED(hr = DeleteNode(pFeatureNode)))
                {
                    //
                    // We need to delete all orphaned paramater init elements. Construct a
                    // list of parameter ref nodes and parameter init nodes and delete parameter
                    // init nodes that do not have a corresponding reference.
                    //
                    CComPtr<IXMLDOMNodeList> pInitList(NULL);
                    CComPtr<IXMLDOMNodeList> pRefList(NULL);

                    if (SUCCEEDED(hr = GetNodes(CComBSTR(PARAM_INIT_ELEMENT_NAME), &pInitList)) &&
                        hr != S_FALSE &&
                        SUCCEEDED(hr = GetNodes(CComBSTR(PARAM_REF_ELEMENT_NAME), &pRefList)) &&
                        hr != S_FALSE)
                    {
                        hr = DeleteParamInitOrphans(pRefList, pInitList);
                    }
                }

                //
                // Release the feature node for the next pass
                //
                pFeatureNode = NULL;
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

    CPTHandler::DeleteParamInitOrphans

Routine Description:

    When a feature is deleted it may orphan a set of parameter init nodes.
    This routine takes a list of parameter init nodes and a list of parameter
    ref nodes and deletes the init nodes with no corresponding ref node.

Arguments:

    pRefList  - pointer to a node list containined the ref nodes
    pInitList - pointer to a node list containined the init nodes

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::DeleteParamInitOrphans(
   _In_    IXMLDOMNodeList* pRefList,
   _Inout_ IXMLDOMNodeList* pInitList
   )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pRefList, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pInitList, E_POINTER)))
    {
        LONG cRefNodes = 0;

        if (SUCCEEDED(hr = pRefList->get_length(&cRefNodes)) &&
            cRefNodes == 0)
        {
            //
            // There are no ref nodes - delete all init nodes
            //
            hr = DeleteNodeList(pInitList);
        }
        else
        {
            //
            // Iterate over init list and search for a corresponding ref node
            //
            LONG cInitNodes = 0;
            hr = pInitList->get_length(&cInitNodes);

            for (LONG cInitNode = 0; cInitNode < cInitNodes && SUCCEEDED(hr); cInitNode++)
            {
                CComPtr<IXMLDOMNode> pInitNode(NULL);
                CComBSTR bstrInitName;

                if (SUCCEEDED(hr = pInitList->get_item(cInitNode, &pInitNode)) &&
                    hr != S_FALSE &&
                    SUCCEEDED(hr = GetAttributeValue(pInitNode,
                                                     CComBSTR(NAME_ATTRIBUTE_NAME),
                                                     &bstrInitName)) &&
                    hr != S_FALSE &&
                    SUCCEEDED(hr = pRefList->reset()))
                {
                    //
                    // Iterate over the ref list tring to match
                    //
                    BOOL bMatched = FALSE;

                    for (LONG cRefNode = 0; cRefNode < cRefNodes && SUCCEEDED(hr) && !bMatched; cRefNode++)
                    {
                        CComPtr<IXMLDOMNode> pRefNode(NULL);
                        CComBSTR bstrRefName;
                        if (SUCCEEDED(hr = pRefList->get_item(cRefNode, &pRefNode)) &&
                            hr != S_FALSE &&
                            SUCCEEDED(hr = GetAttributeValue(pRefNode,
                                                             CComBSTR(NAME_ATTRIBUTE_NAME),
                                                             &bstrRefName)) &&
                            hr != S_FALSE)
                        {
                            if (bstrRefName == bstrInitName)
                            {
                                bMatched = TRUE;
                            }
                        }
                    }

                    if (!bMatched)
                    {
                        //
                        // There were no matches - delete the init node
                        //
                        hr = DeleteNode(pInitNode);
                    }
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CPTHandler::DeleteNodeList

Routine Description:

    This routine deletes all nodes in a node list

Arguments:

    pNodeList - the node list containing the nodes to be deleted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::DeleteNodeList(
   _Inout_ IXMLDOMNodeList* pNodeList
   )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNodeList, E_POINTER)))
    {
        LONG cNodes = 0;

        hr = pNodeList->get_length(&cNodes);

        for (LONG cNode = 0; cNode < cNodes && SUCCEEDED(hr); cNode++)
        {
            CComPtr<IXMLDOMNode> pNode(NULL);
            if (SUCCEEDED(hr = pNodeList->get_item(cNode, &pNode)) &&
                hr != S_FALSE)
            {
                hr = DeleteNode(pNode);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTHandler::DeleteProperty

Routine Description:

    This routine finds and deletes the named property

Arguments:

    bstrProperty - the property name to be deleted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::DeleteProperty(
    _In_z_ BSTR bstrProperty
    )
{
    HRESULT hr = S_OK;

    if (SysStringLen(bstrProperty) > 0)
    {
        //
        // Find the property node
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        CComPtr<IXMLDOMNode> pPropertyNode(NULL);

        if (SUCCEEDED(hr = propertyQuery.AddProperty(m_bstrFrameworkPrefix, bstrProperty)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)) &&
            SUCCEEDED(hr = GetNode(bstrQuery, &pPropertyNode)) &&
            hr != S_FALSE)
        {
            //
            // Delete the property node and all children
            //
            hr = DeleteNode(pPropertyNode);
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

    CPTHandler::QueryNodeOption

Routine Description:

    This routine locates the node specified by the XPath query passed in and
    returns the value of the "name" attribute as a string. The PrintSchema
    keyword prefix is stripped from the result.

Arguments:

    bstrQuery   - The XPath query locating the node to retrieve the value for
    pbstrOption - Pointer to a BSTR to recieve the option string

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPTHandler::QueryNodeOption(
    _In_z_            BSTR  bstrQuery,
    _Outptr_result_maybenull_z_ BSTR* pbstrOption
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrOption, E_POINTER)))
    {
        *pbstrOption = NULL;

        if (SysStringLen(bstrQuery) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Given a bare PrintSchema query, retrieve the selected option
    //
    if (SUCCEEDED(hr))
    {
        CComPtr<IXMLDOMNode> pQueryNode(NULL);

        if (SUCCEEDED(hr = GetNode(bstrQuery, &pQueryNode)) &&
            hr != S_FALSE &&
            SUCCEEDED(hr = GetAttributeValue(pQueryNode, CComBSTR(NAME_ATTRIBUTE_NAME), pbstrOption)) &&
            hr != S_FALSE)
        {
            hr = StripKeywordNamespace(pbstrOption);
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

    CPTHandler::FeaturePresent

Routine Description:

    This routine checks if the named feature is present in the PrintTicket
    and optionally returns that node

Arguments:

    bstrFeature   - The feature name to be located
    ppFeatureNode - optional paramter that recieves the node

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPTHandler::FeaturePresent(
    _In_z_          BSTR          bstrFeature,
    _Outptr_opt_ IXMLDOMNode** ppFeatureNode
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SysStringLen(bstrFeature) > 0)
    {
        //
        // Find the feature node
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        CComPtr<IXMLDOMNode> pFeatureNode(NULL);

        if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrFeature)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)))
        {
            if (ppFeatureNode == NULL)
            {
                hr = GetNode(bstrQuery, &pFeatureNode);
            }
            else
            {
                hr = GetNode(bstrQuery, ppFeatureNode);
            }
        }
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

    CPTHandler::GetFeatureOption

Routine Description:

    This routine retrieves the option set for the named feature passed  in.

Arguments:

    bstrFeature - The feature name to retrieve the option for
    pbstrOption - Pointer to a BSTR to recieve the option

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPTHandler::GetFeatureOption(
    _In_z_            BSTR  bstrFeature,
    _Outptr_result_maybenull_z_ BSTR* pbstrOption
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrOption, E_POINTER)))
    {
        if (SysStringLen(bstrFeature) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Given a bare PrintSchema feature name, retrieve the selected option
    //
    if (SUCCEEDED(hr))
    {
        //
        // Find the feature option node
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        CComPtr<IXMLDOMNode> pOptionNode(NULL);

        if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrFeature)) &&
            SUCCEEDED(hr = propertyQuery.AddOption(m_bstrKeywordsPrefix)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)) &&
            SUCCEEDED(hr = GetNode(bstrQuery, &pOptionNode)) &&
            hr != S_FALSE)
        {
            //
            // If this is a shorthand option then the option is stored in the "name" attribute
            //
            hr = GetAttributeValue(pOptionNode, CComBSTR(NAME_ATTRIBUTE_NAME), pbstrOption);

            if (hr == S_FALSE)
            {
                //
                // This might be a longhand option in which case the node we have retrieved
                // is pointing at a scored property the value of which is the option
                //
                CComVariant varValue;
                if (SUCCEEDED(hr = pOptionNode->get_nodeTypedValue(&varValue)) &&
                    hr != S_FALSE)
                {
                    *pbstrOption = ::SysAllocString(varValue.bstrVal);

                    if (*pbstrOption == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (SUCCEEDED(hr) &&
                hr != S_FALSE)
            {
                hr = StripKeywordNamespace(pbstrOption);
            }
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

    CPTHandler::GetSubFeatureOption

Routine Description:

    This routine retrieves the current option for a sub feature. The routine
    finds the named parent feature, then the sub-feature and retrieves the
    option that is set.

Arguments:

    bstrParentFeature - The parent feature name
    bstrFeature       - The sub-feature name
    pbstrOption       - Pointer to a BSTR to recieve the option

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - If node does not exist
    E_*                 - On error

--*/
HRESULT
CPTHandler::GetSubFeatureOption(
    _In_z_            BSTR  bstrParentFeature,
    _In_z_            BSTR  bstrFeature,
    _Outptr_result_maybenull_z_ BSTR* pbstrOption
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrOption, E_POINTER)))
    {
        *pbstrOption = NULL;

        if (SysStringLen(bstrFeature) <= 0 ||
            SysStringLen(bstrParentFeature) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Given a bare PrintSchema feature name, retrieve the selected option
    //
    if (SUCCEEDED(hr))
    {
        //
        // Find the sub feature option node
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        CComPtr<IXMLDOMNode> pFeatureNode(NULL);

        if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrParentFeature)) &&
            SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrFeature)) &&
            SUCCEEDED(hr = propertyQuery.AddOption(m_bstrKeywordsPrefix)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)) &&
            SUCCEEDED(hr = GetNode(bstrQuery, &pFeatureNode)) &&
            hr != S_FALSE &&
            SUCCEEDED(hr = GetAttributeValue(pFeatureNode, CComBSTR(NAME_ATTRIBUTE_NAME), pbstrOption)) &&
            hr != S_FALSE)
        {
            hr = StripKeywordNamespace(pbstrOption);
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

    CPTHandler::GetScoredPropertyValue

Routine Description:

    This routine retrieves the scored property value as a string for a given feature.

Arguments:

    bstrParentFeature - The parent feature name
    bstrProperty      - The scored property name
    pbstrValue        - Pointer to a BSTR to recieve the scored property value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::GetScoredPropertyValue(
    _In_z_          BSTR  bstrParentFeature,
    _In_z_          BSTR  bstrProperty,
    _Outptr_result_maybenull_ BSTR* pbstrValue
    )
{
    HRESULT hr = S_OK;

    //
    // Validate input parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        *pbstrValue = NULL;

        if (SysStringLen(bstrProperty) <= 0 ||
            SysStringLen(bstrParentFeature) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the scored property value
        //
        CPTQueryBuilder propertyQuery(m_bstrFrameworkPrefix);
        CComBSTR bstrQuery;

        if (SUCCEEDED(hr = propertyQuery.AddFeature(m_bstrKeywordsPrefix, bstrParentFeature)) &&
            SUCCEEDED(hr = propertyQuery.AddScoredProperty(m_bstrKeywordsPrefix, bstrProperty)) &&
            SUCCEEDED(hr = propertyQuery.GetQuery(&bstrQuery)))
        {
            hr = GetNodeValue(bstrQuery, pbstrValue);
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

    CPTHandler::GetScoredPropertyValue

Routine Description:

    This routine retrieves the integer value of a scored property from a named
    feature

Arguments:

    bstrParentFeature - The name of the parent feature
    bstrProperty      - The name of the scored property to retrieve the value of
    pValue            - Pointer to an INT that recieves the property value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::GetScoredPropertyValue(
    _In_z_ BSTR bstrParentFeature,
    _In_z_ BSTR bstrProperty,
    _Out_  INT* pValue
    )
{
    HRESULT hr = S_OK;

    CComBSTR bstrValue;
    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (SysStringLen(bstrParentFeature) <= 0 ||
            SysStringLen(bstrProperty) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetScoredPropertyValue(bstrParentFeature, bstrProperty, &bstrValue)))
    {
        *pValue = static_cast<INT>(_wtoi(bstrValue));
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPTHandler::GetScoredPropertyValue

Routine Description:

    This routine retrieves the REAL value of a scored property from a named
    feature

Arguments:

    bstrParentFeature - The name of the parent feature
    bstrProperty      - The name of the scored property to retrieve the value of
    pValue            - Pointer to a REAL that recieves the property value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::GetScoredPropertyValue(
    _In_z_ BSTR  bstrParentFeature,
    _In_z_ BSTR  bstrProperty,
    _Out_  REAL* pValue
    )
{
    HRESULT hr = S_OK;

    CComBSTR bstrValue;
    if (SUCCEEDED(hr = CHECK_POINTER(pValue, E_POINTER)))
    {
        if (SysStringLen(bstrParentFeature) <= 0 ||
            SysStringLen(bstrProperty) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetScoredPropertyValue(bstrParentFeature, bstrProperty, &bstrValue)))
    {
        *pValue = static_cast<REAL>(_wtof(bstrValue));
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CPTHandler::StripKeywordNamespace

Routine Description:

    This routine strips the PrintSchema keyword namespace prefix from a value

Arguments:

    pbstrValue - Pointer to the value string to have the prefix stripped

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::StripKeywordNamespace(
    _Inout_ BSTR* pbstrValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrValue, E_POINTER)))
    {
        if (SysStringLen(*pbstrValue) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrStrippedName(*pbstrValue);

            INT    cchStringStart = cstrStrippedName.Find(m_bstrKeywordsPrefix);
            size_t cchKWPrefix = 0;
            if (cchStringStart != -1 &&
                SUCCEEDED(hr = StringCchLength(m_bstrKeywordsPrefix, STRSAFE_MAX_CCH, &cchKWPrefix)))
            {
                cstrStrippedName.Delete(cchStringStart, static_cast<INT>(cchKWPrefix));
                SysFreeString(*pbstrValue);
                *pbstrValue = cstrStrippedName.AllocSysString();
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

/*++

Routine Name:

    CPTHandler::CreateParamRefInitPair

Routine Description:

    This routine create a pair of DOM elements corresponding to the ParameterRef and
    the ParameterInit that describe a ScoredProperty value.

Arguments:

    bstrParam   - The name of the property
    bstrType    - The type of the value (string, integer etc.)
    bstrValue   - The value of the property
    ppParamRef  - Pointer to a IXMLDOMElement pointer that recives the new ParameterRef
    ppParamInit - Pointer to a IXMLDOMElement pointer that recives the new ParameterInit

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::CreateParamRefInitPair(
    _In_        CONST BSTR       bstrParam,
    _In_        CONST BSTR       bstrType,
    _In_        CONST BSTR       bstrValue,
    _Outptr_ IXMLDOMElement** ppParamRef,
    _Outptr_ IXMLDOMElement** ppParamInit
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppParamRef, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppParamInit, E_POINTER)))
    {
        *ppParamRef = NULL;
        *ppParamInit = NULL;

        if (SysStringLen(bstrParam) <= 0 ||
            SysStringLen(bstrType) <= 0 ||
            SysStringLen(bstrValue) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    CComPtr<IXMLDOMElement> pParamInit(NULL);
    CComPtr<IXMLDOMElement> pParamRef(NULL);

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrPRefName;
        if (wcscmp(bstrParam, L"PageWatermarkSizeWidth") == 0 ||
            wcscmp(bstrParam, L"PageWatermarkSizeHeight") == 0)
        {
            bstrPRefName += m_bstrUserKeywordsPrefix;
        }
        else
        {
            bstrPRefName += m_bstrKeywordsPrefix;
        }

        bstrPRefName += bstrParam;

        //
        // Create the parameter ref element
        //
        CComBSTR bstrTagName(m_bstrFrameworkPrefix);
        bstrTagName += PARAM_REF_ELEMENT_NAME;

        if(SUCCEEDED(hr))
        {
            hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, &pParamRef);
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateXMLAttribute(pParamRef, NAME_ATTRIBUTE_NAME, NULL, bstrPRefName );
        }

        //
        // Create the parameter init element
        //
        bstrTagName.Empty();
        bstrTagName += m_bstrFrameworkPrefix;
        bstrTagName += PARAM_INIT_ELEMENT_NAME;

        if(SUCCEEDED(hr))
        {
            hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, &pParamInit);
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateXMLAttribute(pParamInit, NAME_ATTRIBUTE_NAME, NULL, bstrPRefName );
        }

        //
        // Create the parameter init value and add to the parameter init element
        //
        CComPtr<IXMLDOMElement> pValue(NULL);

        bstrTagName.Empty();
        bstrTagName += m_bstrFrameworkPrefix;
        bstrTagName += VALUE_ELEMENT_NAME;

        CComBSTR bstrAttrib(m_bstrSchemaInstPrefix);
        bstrAttrib += SCHEMA_TYPE;

        CComBSTR bstrAttribValue(m_bstrSchemaPrefix);
        bstrAttribValue += bstrType;

        if(SUCCEEDED(hr))
        {
            hr = CreateXMLElement(bstrTagName, FRAMEWORK_URI, &pValue);
        }

        if (SUCCEEDED(hr))
        {
            if( SUCCEEDED(hr = CreateXMLAttribute(pValue, bstrAttrib, SCHEMA_INST_URI, bstrAttribValue )) &&
                SUCCEEDED(hr = pValue->put_text(bstrValue)))
            {
                hr = pParamInit->appendChild(pValue, NULL);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Assign the outgoing element pointers - detach from CComPtr to release ownership
        //
        *ppParamRef  = pParamRef.Detach();
        *ppParamInit = pParamInit.Detach();
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTHandler::CreateParamRefInitPair

Routine Description:

    This routine create a pair of DOM elements corresponding to the ParameterRef and
    the ParameterInit that describe a ScoredProperty value. This overload is INT specific

Arguments:

    bstrParam   - The name of the property
    intValue    - The integer value of the scored property
    ppParamRef  - Pointer to a IXMLDOMElement pointer that recives the new ParameterRef
    ppParamInit - Pointer to a IXMLDOMElement pointer that recives the new ParameterInit

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::CreateParamRefInitPair(
    _In_        CONST BSTR       bstrParam,
    _In_        CONST INT        intValue,
    _Outptr_ IXMLDOMElement** ppParamRef,
    _Outptr_ IXMLDOMElement** ppParamInit
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppParamRef, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppParamInit, E_POINTER)))
    {
        if (SysStringLen(bstrParam) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrValue;
            cstrValue.Format(L"%i", intValue);

            hr = CreateParamRefInitPair(bstrParam,
                                        CComBSTR(SCHEMA_INTEGER),
                                        CComBSTR(cstrValue.AllocSysString()),
                                        ppParamRef,
                                        ppParamInit);
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

    CPTHandler::CreateFeatureOptionPair

Routine Description:

    This routine create a pair of DOM elements corresponding to Feature and Option.
    Note this method does not intialise the option element, it merely creates an Option
    node appended to the Feature element. It is up to the caller to set the option value.

Arguments:

    bstrFeatureName  - The name of the feature
    ppFeatureElement - Pointer to a IXMLDOMElement pointer that recives the new Feature
    ppOptionElement  - Pointer to a IXMLDOMElement pointer that recives the new Option

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::CreateFeatureOptionPair(
    _In_        CONST BSTR       bstrFeatureName,
    _Outptr_ IXMLDOMElement** ppFeatureElement,
    _Outptr_ IXMLDOMElement** ppOptionElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppFeatureElement, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppOptionElement, E_POINTER)))
    {
        *ppFeatureElement = NULL;
        *ppOptionElement  = NULL;

        if (SysStringLen(bstrFeatureName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrFeature(m_bstrFrameworkPrefix);
        bstrFeature += FEATURE_ELEMENT_NAME;

        CComBSTR bstrAttribName(m_bstrKeywordsPrefix);
        bstrAttribName += bstrFeatureName;

        CComBSTR bstrOption(m_bstrFrameworkPrefix);
        bstrOption += OPTION_ELEMENT_NAME;

        if(SUCCEEDED(hr))
        {
            hr = CreateXMLElement(bstrFeature, FRAMEWORK_URI, ppFeatureElement);
        }

        if(SUCCEEDED(hr))
        {
            hr = CreateXMLElement(bstrOption, FRAMEWORK_URI, ppOptionElement);
        }

        if(SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr = CreateXMLAttribute(*ppFeatureElement, NAME_ATTRIBUTE_NAME, NULL, bstrAttribName )))
            {
                hr = (*ppFeatureElement)->appendChild(*ppOptionElement, NULL);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTHandler::CreateFeatureOptionPair

Routine Description:

    This routine create a pair of DOM elements corresponding to Feature and Option.

Arguments:

    bstrFeatureName  - The name of the feature
    bstrOptionName   - The name of the option
    ppFeatureElement - Pointer to a IXMLDOMElement pointer that recives the new Feature
    ppOptionElement  - Pointer to a IXMLDOMElement pointer that recives the new Option

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::CreateFeatureOptionPair(
    _In_        CONST BSTR       bstrFeatureName,
    _In_        CONST BSTR       bstrOptionName,
    _Outptr_ IXMLDOMElement** ppFeatureElement,
    _Outptr_ IXMLDOMElement** ppOptionElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppFeatureElement, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppOptionElement, E_POINTER)))
    {
        if (SysStringLen(bstrFeatureName) <= 0 ||
            SysStringLen(bstrOptionName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = CreateFeatureOptionPair(bstrFeatureName, ppFeatureElement, ppOptionElement)))
        {
            //
            // Set the option name attribute
            //
            CComBSTR bstrAttribValue(m_bstrKeywordsPrefix);
            bstrAttribValue += bstrOptionName;

            hr = CreateXMLAttribute(*ppOptionElement, NAME_ATTRIBUTE_NAME, NULL, bstrAttribValue );

        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTHandler::AppendToElement

Routine Description:

    This routine uses the passed element name to locate the node to which
    it will append the DOM node passed in.

Arguments:

    bstrElementName - The name of the element to append to
    pAppendNode     - The node to append

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTHandler::AppendToElement(
    _In_ CONST BSTR   bstrElementName,
    _In_ IXMLDOMNode* pAppendNode
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pAppendNode, E_POINTER)))
    {
        if (SysStringLen(bstrElementName) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IXMLDOMNode> pNode(NULL);

        CComBSTR bstrQuery(m_bstrFrameworkPrefix);
        bstrQuery += bstrElementName;

        if (SUCCEEDED(hr = GetNode(bstrQuery, &pNode)))
        {
            hr = pNode->appendChild(pAppendNode, NULL);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

