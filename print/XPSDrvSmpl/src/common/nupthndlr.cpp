/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupthndlr.cpp

Abstract:

   NUp PrintTicket handling implementation. The nup PT handler
   is used to extract nup settings from a PrintTicket and populate
   the nup properties class with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "nupthndlr.h"

using XDPrintSchema::NUp::NUpData;
using XDPrintSchema::NUp::ENUpFeature;
using XDPrintSchema::NUp::ENUpFeatureMin;
using XDPrintSchema::NUp::ENUpFeatureMax;
using XDPrintSchema::NUp::NUP_FEATURES;
using XDPrintSchema::NUp::NUP_PROP;

using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMin;
using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOptionMax;
using XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_FEATURE;
using XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_OPTIONS;

/*++

Routine Name:

    CNUpPTHandler::CNUpPTHandler

Routine Description:

    CNUpPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CNUpPTHandler::CNUpPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CNUpPTHandler::~CNUpPTHandler

Routine Description:

    CNUpPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CNUpPTHandler::~CNUpPTHandler()
{
}

/*++

Routine Name:

    CNUpPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with NUp data retrieved from
    the PrintTicket passed to the class constructor.

Arguments:

    pNUpData - Pointer to the NUp data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CNUpPTHandler::GetData(
    _Out_ NUpData* pNUpData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpData, E_POINTER)))
    {
        for (ENUpFeature nUpFeature = ENUpFeatureMin;
             nUpFeature < ENUpFeatureMax;
             nUpFeature = static_cast<ENUpFeature>(nUpFeature + 1))
        {
            CComBSTR bstrPresentOption;
            CComBSTR bstrFeature(NUP_FEATURES[nUpFeature]);

            if (SUCCEEDED(hr = FeaturePresent(bstrFeature, NULL)) &&
                SUCCEEDED(hr = GetSubFeatureOption(bstrFeature,
                                                   CComBSTR(NUP_DIRECTION_FEATURE),
                                                   &bstrPresentOption)) &&
                SUCCEEDED(hr = GetScoredPropertyValue(bstrFeature, CComBSTR(NUP_PROP), &pNUpData->cNUp)))
            {
                //
                // We may be set to 1-up. If so treat as though the feature is not present
                //
                if (pNUpData->cNUp > 1)
                {
                    pNUpData->nUpFeature = nUpFeature;

                    //
                    // Convert the presentation direction string to our enumeration
                    //
                    for (ENUpDirectionOption presDir = ENUpDirectionOptionMin;
                         presDir < ENUpDirectionOptionMax;
                         presDir = static_cast<ENUpDirectionOption>(presDir + 1))
                    {
                        if (bstrPresentOption == NUP_DIRECTION_OPTIONS[presDir])
                        {
                            pNUpData->nUpPresentDir = presDir;
                            break;
                        }
                    }

                    //
                    // We have found and initialised the feature - break out the feature loop
                    //
                    break;
                }
                else
                {
                    hr = E_ELEMENT_NOT_FOUND;
                }
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
        if (pNUpData->nUpFeature    <  ENUpFeatureMin ||
            pNUpData->nUpFeature    >= ENUpFeatureMax ||
            pNUpData->nUpPresentDir <  ENUpDirectionOptionMin ||
            pNUpData->nUpPresentDir >= ENUpDirectionOptionMax)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CNUpPTHandler::SetData

Routine Description:

    This routine sets the NUp data in the PrintTicket passed to the
    class constructor. Note: The GPD handles the NUp count option and
    scored properties so we only have to add the presentation direction
    sub-feature.

Arguments:

    pNUpData - Pointer to the NUp data to be set in the PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTHandler::SetData(
    _In_ CONST NUpData* pNUpData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpData, E_POINTER)))
    {
        if (pNUpData->nUpFeature    <  ENUpFeatureMin ||
            pNUpData->nUpFeature    >= ENUpFeatureMax ||
            pNUpData->nUpPresentDir <  ENUpDirectionOptionMin ||
            pNUpData->nUpPresentDir >= ENUpDirectionOptionMax)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check the appropriate NUp option is set in the PT
        //
        CComPtr<IXMLDOMNode> pNUpFeature(NULL);
        CComBSTR bstrFeature(NUP_FEATURES[pNUpData->nUpFeature]);

        if (SUCCEEDED(hr = FeaturePresent(bstrFeature, &pNUpFeature)))
        {
            //
            // The NUp feature is present. Create the presentation direction sub-feature
            // and ensure it is set in the PrintTicket.
            //
            CComPtr<IXMLDOMElement> pPresDirFeature(NULL);
            CComPtr<IXMLDOMElement> pPresDirOption(NULL);

            if (SUCCEEDED(hr = DeleteFeature(CComBSTR(NUP_DIRECTION_FEATURE))) &&
                SUCCEEDED(hr = CreateFeatureOptionPair(CComBSTR(NUP_DIRECTION_FEATURE),
                                                       CComBSTR(NUP_DIRECTION_OPTIONS[pNUpData->nUpPresentDir]),
                                                       &pPresDirFeature,
                                                       &pPresDirOption)))
            {
                hr = pNUpFeature->appendChild(pPresDirFeature, NULL);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTHandler::Delete

Routine Description:

    This routine deletes the NUp feature from the PrintTicket passed to the
    class constructor

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTHandler::Delete()
{
    HRESULT hr = S_OK;

    for (ENUpFeature nUpFeature = ENUpFeatureMin;
         nUpFeature < ENUpFeatureMax && SUCCEEDED(hr);
         nUpFeature = static_cast<ENUpFeature>(nUpFeature + 1))
    {
        hr = DeleteFeature(CComBSTR(NUP_FEATURES[nUpFeature]));
    }

    ERR_ON_HR(hr);
    return hr;
}
