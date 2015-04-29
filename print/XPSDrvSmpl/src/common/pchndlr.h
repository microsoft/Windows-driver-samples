/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

    pchndlr.h

Abstract:

    Base PrintCapabilities handler class definition. This class provides common
    PrintCapabilities handling functionality for any feature that requires
    PrintCapabilities handling. A feature specific handler can inherit from
    this class to get feature unspecific XML handling functionality.

--*/

#pragma once

#include "schema.h"
#include "pshndlr.h"

class CPCHandler : public CPSHandler
{
public:
    //
    // Constructors and destructors
    //
    CPCHandler(
        _In_ IXMLDOMDocument2 *pPrintCapabilities
        );

    virtual ~CPCHandler();

protected:

    HRESULT
    CreateStringParameterDef(
        _In_z_      CONST BSTR       bstrParamName,
        _In_        CONST BOOL       bIsPublicKeyword,
        _In_opt_z_  CONST BSTR       bstrDisplayName,
        _In_        CONST BSTR       bstrDefaultValue,
        _In_        CONST INT        minLength,
        _In_        CONST INT        maxLength,
        _In_z_      CONST BSTR       bstrUnitType,
        _Outptr_ IXMLDOMElement** ppParameterDef
        );

    HRESULT
    CreateIntParameterDef(
        _In_z_      CONST BSTR       bstrParamName,
        _In_        CONST BOOL       bIsPublicKeyword,
        _In_opt_z_  CONST BSTR       bstrDisplayName,
        _In_        CONST INT        defaultValue,
        _In_        CONST INT        minValue,
        _In_        CONST INT        maxValue,
        _In_        CONST INT        multiple,
        _In_z_      CONST BSTR       bstrUnitType,
        _Outptr_ IXMLDOMElement** ppParameterDef
        );

    HRESULT
    CreateFeature(
        _In_z_      CONST BSTR bstrFeatureName,
        _In_opt_z_  CONST BSTR bstrDisplayName,
        _Outptr_ IXMLDOMElement** ppFeatureElement
        );

    HRESULT
    CreateOption(
        _In_z_      CONST BSTR bstrOptionName,
        _In_opt_z_  CONST BSTR bstrDisplayName,
        _Outptr_ IXMLDOMElement** ppOptionElement
        );

    HRESULT
    CreateFeatureSelection(
        _In_z_      CONST BSTR bstrFeatureName,
        _In_opt_z_  CONST BSTR bstrDisplayName,
        _Outptr_ IXMLDOMElement** ppFeatureElement
        );

    HRESULT
    CreateParameterRef(
        _In_z_      CONST BSTR bstrParamRefName,
        _Outptr_ IXMLDOMElement** ppParamRefElement
        );
};

