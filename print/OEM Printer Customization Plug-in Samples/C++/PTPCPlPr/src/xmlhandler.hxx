//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   xmlhandler.hxx
//
//
//  PURPOSE: Helper Class that handles all functionality related to
//  processing of input PrintTicket/PrintCapabilities XML document
//
//

#pragma once

//
// Print ticket handler class that performs all XML handling for the
// sample code
//

class OEMPTXMLHandler
{

public:

    OEMPTXMLHandler();
    virtual ~OEMPTXMLHandler();

    HRESULT SetRoot(
        _In_ IXMLDOMDocument2 *pRoot,
        _In_ IPrintCoreHelper *pHelper
        );

    //
    // Node creation methods
    //

    HRESULT CreateFeatureNode(
        _In_opt_ IXMLDOMElement *pParent,
        _In_ PCWSTR pszNamespaceURI,
        _In_ PCWSTR pszFeatureName,
        _Out_opt_ IXMLDOMElement **ppFeatureNode
        );

    HRESULT CreateOptionNode(
        _In_ IXMLDOMElement *pParent,
        _In_ PCWSTR pszNamespaceURI,
        _In_ PCWSTR pszOptionName,
        _Out_opt_ IXMLDOMElement **ppOptionElement
        );

    //
    // Get methods
    //

    HRESULT GetXMLAttribute(
        _In_ IXMLDOMElement *pElement,
        _Outptr_result_maybenull_ BSTR *pszAttrNamespaceURI,
        _Outptr_result_maybenull_ BSTR *pszAttrName
        ) const;

    HRESULT GetFeatureNode(
        _In_opt_ IXMLDOMElement *pParent,
        _In_ PCWSTR pszAttrNamespaceURI,
        _In_ PCWSTR pszAttrName,
        _Out_ _When_(return == S_FALSE, _At_(*ppElement, _Post_maybenull_))
              _When_(return != S_FALSE, _At_(*ppElement, _Post_valid_))
             IXMLDOMElement **ppElement
        ) const;

    HRESULT GetOptionNode(
        _In_ IXMLDOMElement *pParent,
        _Outptr_result_maybenull_ IXMLDOMElement **ppElement
        ) const;

    HRESULT DeleteFeatureNode(
        _In_opt_ IXMLDOMNode *pParent,
        _In_ IXMLDOMNode *pElement
        );

private:

    IXMLDOMDocument2    *m_pRootDocument;
    IXMLDOMElement      *m_pRootElement;
    IMXNamespaceManager *m_pNSManager;
    DWORD                m_dwNextIndex;

    HRESULT GetXMLElement(
        _In_ IXMLDOMElement *pParent,
        _In_ PCWSTR pszElementNamespaceURI,
        _In_ PCWSTR pszElementName,
        _In_ PCWSTR pszAttrNamespaceURI,
        _In_ PCWSTR pszAttrName,
        _Outptr_result_maybenull_ IXMLDOMElement **ppElement
        ) const;

    HRESULT GetXMLElementWithoutAttribute(
        _In_ IXMLDOMElement *pContext,
        _In_ PCWSTR pszElementNamespaceURI,
        _In_ PCWSTR pszElementName,
        _Outptr_result_maybenull_ IXMLDOMElement **ppChildElement
        ) const;

    HRESULT getPrefix(
        _In_ IXMLDOMElement *pContext,
        _In_ PCWSTR pszUri,
        _Outptr_opt_result_maybenull_ BSTR *pbstrPrefix
        ) const;

    HRESULT declarePrefix(
        _In_ IXMLDOMElement *pContext,
        _In_ LPCWSTR pszUri,
        _In_opt_ LPCWSTR pszPreferredPrefix,
        _Out_ BSTR *newPrefix
        );

    HRESULT CreateQName(
        _In_ IXMLDOMElement *pElement,
        _In_ PCWSTR pszUri,
        _In_ PCWSTR pszLocalName,
        _Out_ BSTR *pQName
        );

    //
    // Adds attributes to existing XML element
    //

    HRESULT CreateXMLAttribute(
        _In_ IXMLDOMElement *pElement,
        _In_ PCWSTR pszAttributeName,
        _In_ PCWSTR pszNamespaceURI,
        _In_ PCWSTR pszAttributeValue
        );

    //
    // Creates new XML element node
    //
    HRESULT CreateXMLElement(
        _In_ IXMLDOMElement *pParent,
        _In_ PCWSTR pszElementName,
        _In_ PCWSTR pszNamespaceURI,
        _Out_ IXMLDOMElement **ppCreatedElement
        );

    //
    // Helpers
    //
    HRESULT getLocalName(
        _In_ PCWSTR pszQName,
        _Out_ BSTR *ppbstrLocalName
        ) const;

    HRESULT getUri(
        IXMLDOMElement *pContext,
        _In_ PCWSTR pszQName,
        _Out_ BSTR *pbstrURI
        ) const;

};

