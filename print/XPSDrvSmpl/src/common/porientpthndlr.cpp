/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   porientpthndlr.cpp

Abstract:

   PageOrientation PrintTicket handler implementation. Derived from CPTHandler,
   this provides a PageOrientation specific Get methods acting on the passed in
   PrintTicket (as a DOM document).

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "porientpthndlr.h"

using XDPrintSchema::PageOrientation::PageOrientationData;
using XDPrintSchema::PageOrientation::EOrientationOption;
using XDPrintSchema::PageOrientation::EOrientationOptionMin;
using XDPrintSchema::PageOrientation::EOrientationOptionMax;
using XDPrintSchema::PageOrientation::ORIENTATION_FEATURE;
using XDPrintSchema::PageOrientation::ORIENTATION_OPTIONS;

/*++

Routine Name:

    CPageOrientationPTHandler::CPageOrientationPTHandler

Routine Description:

    CPageOrientationPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CPageOrientationPTHandler::CPageOrientationPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CPageOrientationPTHandler::~CPageOrientationPTHandler

Routine Description:

    CPageOrientationPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageOrientationPTHandler::~CPageOrientationPTHandler()
{
}

/*++

Routine Name:

    CPageOrientationPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with page orientation data retrieved from
    the PrintTicket passed to the class constructor.

Arguments:

    pPageOrientationData - Pointer to the page orientation data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CPageOrientationPTHandler::GetData(
    _Inout_ PageOrientationData* pPageOrientationData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageOrientationData, E_POINTER)))
    {
        CComBSTR bstrPOOption;

        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(ORIENTATION_FEATURE), &bstrPOOption)))
        {
            for (EOrientationOption poOpt = EOrientationOptionMin;
                 poOpt < EOrientationOptionMax;
                 poOpt = static_cast<EOrientationOption>(poOpt + 1))
            {
                if (bstrPOOption == ORIENTATION_OPTIONS[poOpt])
                {
                    pPageOrientationData->orientation = poOpt;
                    break;
                }
            }
        }
    }

    //
    // Validate the data
    //
    if (SUCCEEDED(hr))
    {
        if (pPageOrientationData->orientation <  EOrientationOptionMin ||
            pPageOrientationData->orientation >= EOrientationOptionMax)
        {
            hr = E_FAIL;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

