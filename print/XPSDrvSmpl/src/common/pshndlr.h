/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

    pshndlr.h

Abstract:

    Base PrintSchema Document handler class definition.
    This class provides common PrintTicket / PrintCapabilities handling functionality.
    A class can derive from this base class to get PrintTicket/PrintCapabilities
    unspecific XML handling functionality.

    Note: The PrintSchema handler code is only intended to work with the sdtandard
    public PrintSchema keywords.

--*/

#pragma once

#include "schema.h"

typedef vector< CComPtr<IXMLDOMElement> > PTDOMElementVector;

class CPSHandler
{
public:
    //
    // Constructors and destructors
    //
    CPSHandler(
        _In_z_ BSTR bstrDocumentType,
        _In_ IXMLDOMDocument2 *pPrintDocument
        );

    virtual ~CPSHandler();

public:
    HRESULT
    DeleteNode(
        _In_ IXMLDOMNode* pNode
        );

    HRESULT
    DeletePrivateFeatures(
        _In_z_ BSTR bstrPrivateNS
        );

protected:
    HRESULT
    QueryNode(
        _In_z_ BSTR  bstrQuery
        );

    HRESULT
    QueryNodeValue(
        _In_z_      BSTR  bstrQuery,
        _Outptr_ BSTR* pbstrValue
        );

    HRESULT
    QueryNodeValue(
        _In_z_ BSTR  bstrQuery,
        _Out_  REAL* pValue
        );

    HRESULT
    QueryNodeValue(
        _In_z_ BSTR  bstrQuery,
        _Out_  INT* pValue
        );

    HRESULT
    GetNodeValue(
        _In_z_      BSTR  bstrNodeQuery,
        _Inout_ _At_(*pbstrValue, _Pre_maybenull_)
        _When_(return == S_FALSE, _At_(*pbstrValue, _Post_maybenull_))
        _When_(return != S_FALSE, _At_(*pbstrValue, _Post_valid_))
                    BSTR* pbstrValue
        );

    HRESULT
    GetAttributeValue(
        _In_            CONST IXMLDOMNode* pNode,
        _In_z_          BSTR               bstrAttribName,
        _Outptr_result_maybenull_ BSTR*              pbstrResult
        );

    HRESULT
    GetNodes(
        _In_        BSTR              bstrElementName,
        _Outptr_ IXMLDOMNodeList** ppNodeList
        );

    HRESULT
    CreateProperty(
        _In_        CONST BSTR       bstrPropName,
        _Outptr_ IXMLDOMElement** ppPropElement
        );

    HRESULT
    CreateProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST BSTR       bstrType,
        _In_        CONST BSTR       bstrValue,
        _Outptr_ IXMLDOMElement** ppPropElement
        );

    HRESULT
    CreateFWProperty(
        _In_        CONST BSTR       bstrPropName,
        _Outptr_ IXMLDOMElement** ppPropElement
        );

    HRESULT
    CreateFWProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST BSTR       bstrType,
        _In_        CONST BSTR       bstrValue,
        _Outptr_ IXMLDOMElement** ppPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST BSTR       bstrType,
        _In_        CONST BSTR       bstrValue,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST BSTR       bstrValue,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST INT        intValue,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        CONST REAL       realValue,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    CreateScoredProperty(
        _In_        CONST BSTR       bstrPropName,
        _In_        IXMLDOMNode*     pValueNode,
        _Outptr_ IXMLDOMElement** ppScoredPropElement
        );

    HRESULT
    GetNode(
    _In_z_      BSTR          bstrNodeQuery,
    _Outptr_ IXMLDOMNode** ppNode
    );

    HRESULT
    CreateXMLAttribute(
        _Inout_    IXMLDOMElement *pTarget,
        _In_       PCWSTR pszName,
        _In_opt_   PCWSTR pszTargetURI,
        _In_       PCWSTR pszValue
    );

    HRESULT
    CreateXMLElement(
        _In_       PCWSTR pszName,
        _In_       PCWSTR pszTargetURI,
        _Out_opt_  IXMLDOMElement **ppEl
    );

private:

    HRESULT
    GetPrefixFromURI(
        _In_z_                    BSTR  bstrNSURI,
        _Outptr_result_maybenull_ BSTR* bstrNSPrefix
        );

protected:
    //
    // Document and Type
    //
    CComPtr<IXMLDOMDocument2> m_pPrintDocument;
    CComBSTR    m_bstrDocumentType;

    //
    // W3 namespace prefixes
    //
    CComBSTR    m_bstrSchemaPrefix;
    CComBSTR    m_bstrSchemaInstPrefix;

    //
    // Namespace abbreviations
    //
    CComBSTR    m_bstrFrameworkPrefix;
    CComBSTR    m_bstrKeywordsPrefix;
    CComBSTR    m_bstrUserKeywordsPrefix;
};

