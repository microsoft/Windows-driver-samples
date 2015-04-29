/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

File Name:

    pchndlr.cpp

Abstract:

    Base PrintCapabilities handler class implementation. This class provides common
    PrintCapabilities handling functionality for any feature that requires
    PrintCapabilities handling. A feature specific handler can derive from
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
#include "xdstring.h"
#include "pchndlr.h"

using XDPrintSchema::PRINTCAPABILITIES_NAME;
using XDPrintSchema::PARAM_DEF_ELEMENT_NAME;
using XDPrintSchema::NAME_ATTRIBUTE_NAME;
using XDPrintSchema::SCHEMA_CONDITIONAL;
using XDPrintSchema::SCHEMA_INTEGER;
using XDPrintSchema::DATATYPE_VALUE_NAME;
using XDPrintSchema::SCHEMA_QNAME;
using XDPrintSchema::DEFAULTVAL_VALUE_NAME;
using XDPrintSchema::SCHEMA_STRING;
using XDPrintSchema::MAX_VALUE_NAME;
using XDPrintSchema::MIN_VALUE_NAME;
using XDPrintSchema::MAX_LENGTH_NAME;
using XDPrintSchema::MIN_LENGTH_NAME;
using XDPrintSchema::MANDATORY_VALUE_NAME;
using XDPrintSchema::UNITTYPE_VALUE_NAME;
using XDPrintSchema::MULTIPLE_VALUE_NAME;
using XDPrintSchema::DISPLAYNAME_VALUE_NAME;
using XDPrintSchema::SCHEMA_DECIMAL;
using XDPrintSchema::FEATURE_ELEMENT_NAME;
using XDPrintSchema::PICKONE_VALUE_NAME;
using XDPrintSchema::SELECTIONTYPE_VALUE_NAME;
using XDPrintSchema::OPTION_ELEMENT_NAME;
using XDPrintSchema::PARAM_REF_ELEMENT_NAME;
using XDPrintSchema::FRAMEWORK_URI;
using XDPrintSchema::KEYWORDS_URI;

/*++

Routine Name:

    CPCHandler::CPCHandler

Routine Description:

    CPCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

    Note: Base Class (CPSHandler) - Throws CXDException(HRESULT) on an error

--*/
CPCHandler::CPCHandler(
    _In_ IXMLDOMDocument2 *pDOMDocument
    ) :
    CPSHandler(CComBSTR(PRINTCAPABILITIES_NAME), pDOMDocument)
{
}

/*++

Routine Name:

    CPCHandler::~CPCHandler

Routine Description:

    CPCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPCHandler::~CPCHandler()
{
}


/*++

Routine Name:

    CPCHandler::CreateStringParameterDef

Routine Description:

    Creates a String type ParameterDef Element.

Arguments:

    bstrParamName - Keyword value name for the parameter
    bstrDisplayName - Optional string containing a description of the parameter definition.
    defaultValue - Default value of the parameter.
    minLength - Minimium valid value.
    maxLength - Maximium valid value.
    multiple - value can be increased or decreased in multiples of.
    bstrUnitType - String containing a description of the unit type of the parameter value.
    ppParameterDef - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateStringParameterDef(
    _In_z_      CONST BSTR       bstrParamName,
    _In_        CONST BOOL       bIsPublicKeyword,
    _In_opt_z_  CONST BSTR       bstrDisplayName,
    _In_        CONST BSTR       defaultValue,
    _In_        CONST INT        minLength,
    _In_        CONST INT        maxLength,
    _In_z_      CONST BSTR       bstrUnitType,
    _Outptr_ IXMLDOMElement** ppParameterDef
    )
{
    HRESULT hr = S_OK;

    //
    // Create the parameterDef
    //
    CComBSTR bstrParameter(m_bstrFrameworkPrefix);
    bstrParameter += PARAM_DEF_ELEMENT_NAME;

    //
    // PageWatermarkSizeWidth and PageWatermarkSizeHeight are non-standard
    // parameter names so need to be accessed using a user defined namespace
    //
    CComBSTR bstrParameterAttrib;
    if (bIsPublicKeyword)
    {
        hr = bstrParameterAttrib.Append(m_bstrKeywordsPrefix);
    }
    else
    {
        hr = bstrParameterAttrib.Append(m_bstrUserKeywordsPrefix);
    }

    if (SUCCEEDED(hr))
    {
        bstrParameterAttrib += bstrParamName;
    }

    if(SUCCEEDED(hr))
    {
        hr = CreateXMLElement(bstrParameter, FRAMEWORK_URI, ppParameterDef);
    }

    if(SUCCEEDED(hr))
    {
        if(SUCCEEDED(hr = CreateXMLAttribute(*ppParameterDef, NAME_ATTRIBUTE_NAME, NULL, bstrParameterAttrib )))
        {
            CComPtr<IXMLDOMElement> pDataTypeProp(NULL);
            CComPtr<IXMLDOMElement> pDefValProp(NULL);
            CComPtr<IXMLDOMElement> pMaxLengthProp(NULL);
            CComPtr<IXMLDOMElement> pMinLengthProp(NULL);
            CComPtr<IXMLDOMElement> pMandatoryProp(NULL);
            CComPtr<IXMLDOMElement> pUnitTypeProp(NULL);

            CComBSTR bstrMandatory(m_bstrKeywordsPrefix);
            bstrMandatory += SCHEMA_CONDITIONAL;

            CStringXDW cstrMaxLength;
            cstrMaxLength.Format(L"%i", maxLength);

            CStringXDW cstrMinLength;
            cstrMinLength.Format(L"%i", minLength);

            CComBSTR bstrStringType(m_bstrSchemaPrefix);
            bstrStringType += SCHEMA_STRING;

            if (SUCCEEDED(hr = CreateProperty(CComBSTR(DATATYPE_VALUE_NAME), CComBSTR(SCHEMA_QNAME), bstrStringType, &pDataTypeProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(DEFAULTVAL_VALUE_NAME), CComBSTR(SCHEMA_STRING), defaultValue, &pDefValProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MAX_LENGTH_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrMaxLength), &pMaxLengthProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MIN_LENGTH_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrMinLength), &pMinLengthProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MANDATORY_VALUE_NAME), CComBSTR(SCHEMA_QNAME), bstrMandatory, &pMandatoryProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(UNITTYPE_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrUnitType, &pUnitTypeProp)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pDataTypeProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pDefValProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMaxLengthProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMinLengthProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMandatoryProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pUnitTypeProp, NULL)))
            {
                if (SUCCEEDED(hr) &&
                    SysStringLen(bstrDisplayName) > 0)
                {
                    CComPtr<IXMLDOMElement> pDisplayProp(NULL);

                    if (SUCCEEDED(hr = CreateProperty(CComBSTR(DISPLAYNAME_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrDisplayName, &pDisplayProp)))
                    {
                        hr = (*ppParameterDef)->appendChild(pDisplayProp, NULL);
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

    CPCHandler::CreateIntParameterDef

Routine Description:

    Creates a Integer type ParameterDef Element.

Arguments:

    bstrParamName - Keyword value name for the parameter
    bstrDisplayName - Optional string containing a description of the parameter definition.
    defaultValue - Default value of the parameter.
    minValue - Minimium valid value.
    maxLength - Maximium valid value.
    multiple - value can be increased or decreased in multiples of.
    bstrUnitType - String containing a description of the unit type of the parameter value.
    ppParameterDef - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateIntParameterDef(
    _In_z_      CONST BSTR       bstrParamName,
    _In_        CONST BOOL       bIsPublicKeyword,
    _In_opt_z_  CONST BSTR       bstrDisplayName,
    _In_        CONST INT        defaultValue,
    _In_        CONST INT        minValue,
    _In_        CONST INT        maxValue,
    _In_        CONST INT        multiple,
    _In_z_      CONST BSTR       bstrUnitType,
    _Outptr_ IXMLDOMElement** ppParameterDef
    )
{
    HRESULT hr = S_OK;

    //
    // Create the parameterDef
    //
    CComBSTR bstrParameter(m_bstrFrameworkPrefix);
    bstrParameter += PARAM_DEF_ELEMENT_NAME;

    CComBSTR bstrParameterAttrib;
    if (bIsPublicKeyword)
    {
        hr = bstrParameterAttrib.Append(m_bstrKeywordsPrefix);
    }
    else
    {
        hr = bstrParameterAttrib.Append(m_bstrUserKeywordsPrefix);
    }

    if (SUCCEEDED(hr))
    {
        bstrParameterAttrib += bstrParamName;
    }

    if(SUCCEEDED(hr))
    {
        hr = CreateXMLElement(bstrParameter, FRAMEWORK_URI, ppParameterDef);
    }

    if(SUCCEEDED(hr))
    {
        if(SUCCEEDED(hr = CreateXMLAttribute(*ppParameterDef, NAME_ATTRIBUTE_NAME, NULL, bstrParameterAttrib )))
        {
            CComPtr<IXMLDOMElement> pDataTypeProp(NULL);
            CComPtr<IXMLDOMElement> pDefValProp(NULL);
            CComPtr<IXMLDOMElement> pMaxValueProp(NULL);
            CComPtr<IXMLDOMElement> pMinValueProp(NULL);
            CComPtr<IXMLDOMElement> pMandatoryProp(NULL);
            CComPtr<IXMLDOMElement> pUnitTypeProp(NULL);
            CComPtr<IXMLDOMElement> pMultipleProp(NULL);

            CStringXDW cstrMultiple;
            cstrMultiple.Format(L"%i", multiple);

            CComBSTR bstrMandatory(m_bstrKeywordsPrefix);
            bstrMandatory += SCHEMA_CONDITIONAL;

            CStringXDW cstrDefaultValue;
            cstrDefaultValue.Format(L"%i", defaultValue);

            CStringXDW cstrMaxValue;
            cstrMaxValue.Format(L"%i", maxValue);

            CStringXDW cstrMinValue;
            cstrMinValue.Format(L"%i", minValue);

            CComBSTR bstrIntegerType(m_bstrSchemaPrefix);
            bstrIntegerType += SCHEMA_INTEGER;

            if (SUCCEEDED(hr = CreateProperty(CComBSTR(DATATYPE_VALUE_NAME), CComBSTR(SCHEMA_QNAME), bstrIntegerType, &pDataTypeProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(DEFAULTVAL_VALUE_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrDefaultValue), &pDefValProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MAX_VALUE_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrMaxValue), &pMaxValueProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MIN_VALUE_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrMinValue), &pMinValueProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MANDATORY_VALUE_NAME), CComBSTR(SCHEMA_QNAME), bstrMandatory, &pMandatoryProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(UNITTYPE_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrUnitType, &pUnitTypeProp)) &&
                SUCCEEDED(hr = CreateProperty(CComBSTR(MULTIPLE_VALUE_NAME), CComBSTR(SCHEMA_INTEGER), CComBSTR(cstrMultiple), &pMultipleProp)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pDataTypeProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pDefValProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMaxValueProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMinValueProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMandatoryProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pUnitTypeProp, NULL)) &&
                SUCCEEDED(hr = (*ppParameterDef)->appendChild(pMultipleProp, NULL)))
            {
                if (SysStringLen(bstrDisplayName) > 0)
                {
                    CComPtr<IXMLDOMElement> pDisplayProp(NULL);

                    if (SUCCEEDED(hr = CreateProperty(CComBSTR(DISPLAYNAME_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrDisplayName, &pDisplayProp)))
                    {
                        hr = (*ppParameterDef)->appendChild(pDisplayProp, NULL);
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

    CPCHandler::CreateFeature

Routine Description:

    Creates a Feature Element.

Arguments:

    bstrFeatureName - Keyword value name for the feature.
    bstrDisplayName - Optional string containing a description of the feature.
    ppFeatureElement - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateFeature(
    _In_z_      CONST BSTR bstrFeatureName,
    _In_opt_z_  CONST BSTR bstrDisplayName,
    _Outptr_ IXMLDOMElement** ppFeatureElement
    )
{
    HRESULT hr = S_OK;

    //
    // Create the base feature node
    //
    CComBSTR bstrFeature(m_bstrFrameworkPrefix);
    bstrFeature += FEATURE_ELEMENT_NAME;

    CComBSTR bstrFeatureAttrib(m_bstrKeywordsPrefix);
    bstrFeatureAttrib += bstrFeatureName;

    if(SUCCEEDED(hr))
    {
        hr = CreateXMLElement(bstrFeature, FRAMEWORK_URI, ppFeatureElement);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = CreateXMLAttribute(*ppFeatureElement, NAME_ATTRIBUTE_NAME, NULL, bstrFeatureAttrib );
    }

    //
    // Append the Display Name property type
    //
    if (SUCCEEDED(hr) &&
        SysStringLen(bstrDisplayName) > 0)
    {
        CComPtr<IXMLDOMElement> pDisplayProp(NULL);

        if (SUCCEEDED(hr = CreateProperty(CComBSTR(DISPLAYNAME_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrDisplayName, &pDisplayProp)))
        {
            hr = (*ppFeatureElement)->appendChild(pDisplayProp, NULL);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPCHandler::CreateFeatureSelection

Routine Description:

    Creates a Selection Feature Element.

Arguments:

    bstrFeatureName - Keyword value name for the feature.
    bstrDisplayName - Optional string containing a description of the feature.
    ppFeatureElement - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateFeatureSelection(
    _In_z_      CONST BSTR bstrFeatureName,
    _In_opt_z_  CONST BSTR bstrDisplayName,
    _Outptr_ IXMLDOMElement** ppFeatureElement
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CreateFeature(bstrFeatureName, bstrDisplayName, ppFeatureElement)))
    {
        //
        // Append the property type
        //
        CComPtr<IXMLDOMElement> pPropertyTypeElement(NULL);

        //
        // Create the keyname
        //
        CComBSTR bstrPickOne(m_bstrKeywordsPrefix);
        bstrPickOne += PICKONE_VALUE_NAME;

        if (SUCCEEDED(hr = CreateFWProperty(CComBSTR(SELECTIONTYPE_VALUE_NAME), CComBSTR(SCHEMA_QNAME), bstrPickOne, &pPropertyTypeElement)))
        {
            hr = (*ppFeatureElement)->appendChild(pPropertyTypeElement, NULL);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPCHandler::CreateOption

Routine Description:

    Creates an Option Type Element.

Arguments:

    bstrOptionName - Keyword value name for the option.
    bstrDisplayName - Optional string containing a description of the feature.
    ppOptionElement - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateOption(
    _In_z_      CONST BSTR bstrOptionName,
    _In_opt_z_  CONST BSTR bstrDisplayName,
    _Outptr_ IXMLDOMElement** ppOptionElement
    )
{
    HRESULT hr = S_OK;

    //
    // Create the base feature node
    //
    CComBSTR bstrOption(m_bstrFrameworkPrefix);
    bstrOption += OPTION_ELEMENT_NAME;

    CComBSTR bstrOptionAttrib(m_bstrKeywordsPrefix);
    bstrOptionAttrib += bstrOptionName;

    if(SUCCEEDED(hr))
    {
        hr = CreateXMLElement(bstrOption, FRAMEWORK_URI, ppOptionElement);
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateXMLAttribute(*ppOptionElement, NAME_ATTRIBUTE_NAME, NULL, bstrOptionAttrib );
    }

    //
    // Append the Display Name property type
    //
    if (SUCCEEDED(hr) &&
        SysStringLen(bstrDisplayName) > 0)
    {
        CComPtr<IXMLDOMElement> pDisplayProp(NULL);

        if (SUCCEEDED(hr = CreateProperty(CComBSTR(DISPLAYNAME_VALUE_NAME), CComBSTR(SCHEMA_STRING), bstrDisplayName, &pDisplayProp)))
        {
            hr = (*ppOptionElement)->appendChild(pDisplayProp, NULL);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPCHandler::CreateParameterRef

Routine Description:

    Creates a Reference to a ParameterDef Element.

Arguments:

    bstrParamRefName - Keyword value name for the parameter reference.
    ppParamRefElement - Pointer to an IXMLDOMElement that recieves the new element.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPCHandler::CreateParameterRef(
    _In_z_      CONST BSTR bstrParamRefName,
    _Outptr_ IXMLDOMElement** ppParamRefElement
    )
{
    HRESULT hr = S_OK;

    //
    // Create the base feature node
    //
    CComBSTR bstrParameterRef(m_bstrFrameworkPrefix);
    bstrParameterRef += PARAM_REF_ELEMENT_NAME;


    //
    // PageWatermarkSizeWidth and PageWatermarkSizeHeight are non-standard
    // parameter names so need to be accessed using a user defined namespace
    //
    CComBSTR bstrParamRefAttrib;
    if (wcscmp(bstrParamRefName, L"PageWatermarkSizeWidth") == 0 ||
        wcscmp(bstrParamRefName, L"PageWatermarkSizeHeight") == 0)
    {
        hr = bstrParamRefAttrib.Append(m_bstrUserKeywordsPrefix);
    }
    else
    {
        hr = bstrParamRefAttrib.Append(m_bstrKeywordsPrefix);
    }

    if (SUCCEEDED(hr))
    {
        bstrParamRefAttrib += bstrParamRefName;
    }

    //
    //Create the IXMLDOMNode for ParameterRef
    //
    CComPtr<IXMLDOMNode> pParameterRefNode(NULL);

    if(SUCCEEDED(hr))
    {
        hr = CreateXMLElement(bstrParameterRef, FRAMEWORK_URI, ppParamRefElement);
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateXMLAttribute(*ppParamRefElement, NAME_ATTRIBUTE_NAME, NULL, bstrParamRefAttrib );
    }

    ERR_ON_HR(hr);
    return hr;
}

