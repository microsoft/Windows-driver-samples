/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmintpthndlr.cpp

Abstract:

   PageICMRenderingIntent PrintTicket handler implementation. Derived from
   CPTHandler, this provides PageICMRenderingIntent specific Get and Set methods
   acting on the PrintTicket (as a DOM document) passed.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "cmintpthndlr.h"

using XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData;
using XDPrintSchema::PageICMRenderingIntent::EICMIntentOption;
using XDPrintSchema::PageICMRenderingIntent::EICMIntentOptionMin;
using XDPrintSchema::PageICMRenderingIntent::BusinessGraphics;
using XDPrintSchema::PageICMRenderingIntent::EICMIntentOptionMax;
using XDPrintSchema::PageICMRenderingIntent::ICMINTENT_FEATURE;
using XDPrintSchema::PageICMRenderingIntent::ICMINTENT_OPTIONS;

/*++

Routine Name:

    CColorManageIntentsPTHandler::CColorManageIntentsPTHandler

Routine Description:

    CColorManageIntentsPTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CColorManageIntentsPTHandler::CColorManageIntentsPTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CColorManageIntentsPTHandler::~CColorManageIntentsPTHandler

Routine Description:

    CColorManageIntentsPTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorManageIntentsPTHandler::~CColorManageIntentsPTHandler()
{
}

/*++

Routine Name:

    CColorManageIntentsPTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with color intent data retrieved
    from the PrintTicket passed to the class constructor.

Arguments:

    pCmData - Pointer to the color intent data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CColorManageIntentsPTHandler::GetData(
    _Inout_ PageICMRenderingIntentData* pCmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pCmData, E_POINTER)))
    {
        CComBSTR bstrCMOption;

        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(ICMINTENT_FEATURE), &bstrCMOption)))
        {
            //
            // Get the profile type
            //
            for (EICMIntentOption cmOption = EICMIntentOptionMin;
                 cmOption < EICMIntentOptionMax;
                 cmOption = static_cast<EICMIntentOption>(cmOption + 1))
            {
                if (bstrCMOption == ICMINTENT_OPTIONS[cmOption])
                {
                    pCmData->cmOption = cmOption;
                    break;
                }
            }

            if (SUCCEEDED(hr))
            {
                if (pCmData->cmOption <  EICMIntentOptionMin ||
                    pCmData->cmOption >= EICMIntentOptionMax)
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            pCmData->cmOption = BusinessGraphics;
            hr = S_OK;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

