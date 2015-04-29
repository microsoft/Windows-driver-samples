/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkpthndlr.cpp

Abstract:

   Booklet PrintTicket handling implementation. The booklet PT handler
   is used to extract booklet settings from a PrintTicket and populate
   the booklet data structure with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "bkpthndlr.h"

using XDPrintSchema::PRINTTICKET_NAME;

using XDPrintSchema::Binding::BindingData;
using XDPrintSchema::Binding::EBinding;
using XDPrintSchema::Binding::EBindingMin;
using XDPrintSchema::Binding::EBindingMax;
using XDPrintSchema::Binding::EBindingOption;
using XDPrintSchema::Binding::EBindingOptionMin;
using XDPrintSchema::Binding::EBindingOptionMax;
using XDPrintSchema::Binding::BIND_FEATURES;
using XDPrintSchema::Binding::BIND_OPTIONS;
using XDPrintSchema::Binding::BIND_PROP;
using XDPrintSchema::Binding::BIND_PROP_REF_SUFFIX;

/*++

Routine Name:

    CBookPTHandler::CBookPTHandler

Routine Description:

    CBookPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CBookPTHandler::CBookPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CBookPTHandler::~CBookPTHandler

Routine Description:

    CBookPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CBookPTHandler::~CBookPTHandler()
{
}

/*++

Routine Name:

    CBookPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with binding data retrieved from
    the PrintTicket passed to the class constructor.

Arguments:

    pBindData - Pointer to the binding data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CBookPTHandler::GetData(
    _Out_ BindingData* pBindData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBindData, E_POINTER)))
    {
        for (EBinding bindFeature = EBindingMin;
             bindFeature < EBindingMax;
             bindFeature = static_cast<EBinding>(bindFeature + 1))
        {
            CComBSTR bstrBindOption;
            CComBSTR bstrFeature(BIND_FEATURES[bindFeature]);

            if (SUCCEEDED(hr = GetFeatureOption(bstrFeature, &bstrBindOption)))
            {
                pBindData->bindFeature = bindFeature;

                //
                // Identify the option
                //
                for (EBindingOption bindOption = EBindingOptionMin;
                     bindOption < EBindingOptionMax;
                     bindOption = static_cast<EBindingOption>(bindOption + 1))
                {
                    if (bstrBindOption == BIND_OPTIONS[bindOption])
                    {
                        pBindData->bindOption = bindOption;
                        break;
                    }
                }

                //
                // Get the gutter value if one exists otherwise use a default
                //
                if (FAILED(GetScoredPropertyValue(bstrFeature, CComBSTR(BIND_PROP), &pBindData->bindGutter)))
                {
                    pBindData->bindGutter = 0;
                }

                break;
            }
            else if (hr != E_ELEMENT_NOT_FOUND)
            {
                //
                // We have an error other than the element is not present
                // so we break and let the result return
                //
                break;
            }
        }
    }

    //
    // Validate the data
    //
    if (SUCCEEDED(hr))
    {
        if (pBindData->bindFeature < EBindingMin ||
            pBindData->bindFeature >= EBindingMax ||
            pBindData->bindOption < EBindingOptionMin ||
            pBindData->bindOption >= EBindingOptionMax)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CBookPTHandler::SetData

Routine Description:

    This routine sets the binding data in the PrintTicket passed to the
    class constructor.

Arguments:

    pBindData - Pointer to the binding data to be set in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookPTHandler::SetData(
    _In_ CONST BindingData* pBindData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBindData, E_POINTER)))
    {
        if (pBindData->bindFeature <  EBindingMin ||
            pBindData->bindFeature >= EBindingMax ||
            pBindData->bindOption  <  EBindingOptionMin ||
            pBindData->bindOption  >= EBindingOptionMax)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Delete any exsting NUp settings as JobNUpAllDocumentsContiguously and DocumentNUp are mutually exclusive
    //
    if (SUCCEEDED(hr) &&
        pBindData->bindOption != XDPrintSchema::Binding::None)
    {
        //
        // Create the following elements
        //    Feature and Option
        //    Parameter Ref and Parameter Init
        //    ScoredProperty
        //
        // Then append the scored property to the option node and the feature
        // and parameter init nodes to the root PrintTicket node. Note: the
        // scored poperty is created passing the parameter ref so it is already
        // appended
        //
        CComPtr<IXMLDOMElement> pFeature(NULL);
        CComPtr<IXMLDOMElement> pOption(NULL);
        CComPtr<IXMLDOMElement> pParamRef(NULL);
        CComPtr<IXMLDOMElement> pParamInit(NULL);
        CComPtr<IXMLDOMElement> pScoredProp(NULL);

        CComBSTR bstrFeature(BIND_FEATURES[pBindData->bindFeature]);

        if (SUCCEEDED(DeleteFeature(bstrFeature)))
        {
            CComBSTR bstrPropName(BIND_PROP_REF_SUFFIX);
            CComBSTR bstrParamRefName(bstrFeature);
            bstrParamRefName += bstrPropName;

            if (SUCCEEDED(hr = CreateFeatureOptionPair(bstrFeature,
                                                       CComBSTR(BIND_OPTIONS[pBindData->bindOption]),
                                                       &pFeature,
                                                       &pOption)) &&
                SUCCEEDED(hr = CreateParamRefInitPair(bstrParamRefName, pBindData->bindGutter, &pParamRef, &pParamInit)) &&
                SUCCEEDED(hr = CreateScoredProperty(CComBSTR(BIND_PROP), pParamRef, &pScoredProp)) &&
                SUCCEEDED(hr = pOption->appendChild(pScoredProp, NULL)) &&
                SUCCEEDED(hr = AppendToElement(CComBSTR(PRINTTICKET_NAME), pFeature)))
            {
                hr = AppendToElement(CComBSTR(PRINTTICKET_NAME), pParamInit);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookPTHandler::Delete

Routine Description:

    This routine deletes the binding feature from the PrintTicket passed to the
    class constructor

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookPTHandler::Delete(
    VOID
    )
{
    HRESULT hr = S_OK;

    for (EBinding bindFeature = EBindingMin;
         bindFeature < EBindingMax && SUCCEEDED(hr);
         bindFeature = static_cast<EBinding>(bindFeature + 1))
    {
        hr = DeleteFeature(CComBSTR(BIND_FEATURES[bindFeature]));
    }

    ERR_ON_HR(hr);
    return hr;
}

