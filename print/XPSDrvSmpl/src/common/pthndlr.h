/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

   pthndlr.h

Abstract:

   Base PrintTicket handler class definition. This class provides common
   PrintTicket handling functionality for any filter that requires print
   ticket handling. A filter can from this class to get feature unspecific
   XML handling functionality.

--*/

#pragma once

#include "schema.h"
#include "pshndlr.h"

class CPTHandler : public CPSHandler
{
public:
    //
    // Constructors and destructors
    //
    CPTHandler(
        _In_ IXMLDOMDocument2 *pPrintTicket
        );

    virtual ~CPTHandler();

public:
    HRESULT
    DeleteFeature(
        _In_z_ BSTR bstrFeature
        );

    HRESULT
    DeleteProperty(
        _In_z_ BSTR bstrProperty
        );

protected:
    HRESULT
    QueryNodeOption(
        _In_z_            BSTR  bstrQuery,
        _Outptr_result_maybenull_z_ BSTR* pbstrOption
        );

    HRESULT
    FeaturePresent(
        _In_z_          BSTR          bstrFeature,
        _Outptr_opt_ IXMLDOMNode** ppFeatureNode = NULL
        );

    HRESULT
    GetFeatureOption(
        _In_z_            BSTR  bstrFeature,
        _Outptr_result_maybenull_z_ BSTR* pbstrOption
        );

    HRESULT
    GetSubFeatureOption(
        _In_z_            BSTR  bstrParentFeature,
        _In_z_            BSTR  bstrFeature,
        _Outptr_result_maybenull_z_ BSTR* pbstrOption
        );

    HRESULT
    GetScoredPropertyValue(
        _In_z_          BSTR  bstrParentFeature,
        _In_z_          BSTR  bstrProperty,
        _Outptr_result_maybenull_ BSTR* pbstrValue
        );

    HRESULT
    GetScoredPropertyValue(
        _In_z_ BSTR bstrParentFeature,
        _In_z_ BSTR bstrProperty,
        _Out_  INT* pValue
        );

    HRESULT
    GetScoredPropertyValue(
        _In_z_ BSTR  bstrParentFeature,
        _In_z_ BSTR  bstrProperty,
        _Out_  REAL* pValue
        );

    HRESULT
    SetFeatureOption(
        _In_z_ BSTR bstrFeature,
        _In_z_ BSTR bstrOption
        );

    HRESULT
    SetSubFeatureOption(
        _In_z_ BSTR bstrParentFeature,
        _In_z_ BSTR bstrFeature,
        _In_z_ BSTR bstrOption
        );

    HRESULT
    SetPropertyAsValue(
        _In_z_ BSTR bstrFeature,
        _In_z_ BSTR bstrProperty,
        _In_z_ BSTR bstrValue
        );

    HRESULT
    SetPropertyAsParameterRef(
        _In_z_ BSTR bstrFeature,
        _In_z_ BSTR bstrProperty,
        _In_z_ BSTR bstrParameterRef,
        _In_z_ BSTR bstrValue
        );

    HRESULT
    CreateParamRefInitPair(
        _In_        CONST BSTR       bstrParam,
        _In_        CONST BSTR       bstrType,
        _In_        CONST BSTR       bstrValue,
        _Outptr_ IXMLDOMElement** ppParamRef,
        _Outptr_ IXMLDOMElement** ppParamInit
        );

    HRESULT
    CreateParamRefInitPair(
        _In_        CONST BSTR       bstrParam,
        _In_        CONST INT        intValue,
        _Outptr_ IXMLDOMElement** ppParamRef,
        _Outptr_ IXMLDOMElement** ppParamInit
        );

    HRESULT
    CreateFeatureOptionPair(
        _In_        CONST BSTR       bstrFeatureName,
        _Outptr_ IXMLDOMElement** ppFeatureElement,
        _Outptr_ IXMLDOMElement** ppOptionElement
        );

    HRESULT
    CreateFeatureOptionPair(
        _In_        CONST BSTR       bstrFeatureName,
        _In_        CONST BSTR       bstrOptionName,
        _Outptr_ IXMLDOMElement** ppFeatureElement,
        _Outptr_ IXMLDOMElement** ppOptionElement
        );

    HRESULT
    AppendToElement(
        _In_  CONST BSTR   bstrElementName,
        _In_  IXMLDOMNode* pAppendNode
        );

private:
    HRESULT
    StripKeywordNamespace(
        _Inout_ BSTR* pbstrValue
        );

    HRESULT
    DeleteNodeList(
       _Inout_ IXMLDOMNodeList* pNodeList
       );

    HRESULT
    DeleteParamInitOrphans(
       _In_    IXMLDOMNodeList* pRefList,
       _Inout_ IXMLDOMNodeList* pInitList
       );
};

