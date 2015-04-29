/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmpthndlr.cpp

Abstract:

   PageColorManagement PrintTicket handling implementation. The
   PageColorManagement PT handler is used to extract booklet settings
   from a PrintTicket and populate the PageColorManagement data
   structure with the retrieved settings.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "cmpthndlr.h"

using XDPrintSchema::PageColorManagement::ColorManagementData;
using XDPrintSchema::PageColorManagement::EPCMOption;
using XDPrintSchema::PageColorManagement::EPCMOptionMin;
using XDPrintSchema::PageColorManagement::EPCMOptionMax;
using XDPrintSchema::PageColorManagement::PCM_FEATURE;
using XDPrintSchema::PageColorManagement::PCM_OPTIONS;

/*++

Routine Name:

    CColorManagePTHandler::CColorManagePTHandler

Routine Description:

    CColorManagePTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CColorManagePTHandler::CColorManagePTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CColorManagePTHandler::~CColorManagePTHandler

Routine Description:

    CColorManagePTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CColorManagePTHandler::~CColorManagePTHandler()
{
}

/*++

Routine Name:

    CColorManagePTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with color management data
    retrieved from the PrintTicket passed to the class constructor.

Arguments:

    pCmData - Pointer to the color management data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CColorManagePTHandler::GetData(
    _Inout_ ColorManagementData* pCmData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pCmData, E_POINTER)))
    {
        CComBSTR bstrCMOption;

        //
        // Check whether to use driver or device color matching
        //
        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(PCM_FEATURE), &bstrCMOption)))
        {
            for (EPCMOption cmOpt = EPCMOptionMin;
                 cmOpt < EPCMOptionMax;
                 cmOpt = static_cast<EPCMOption>(cmOpt + 1))
            {
                if (bstrCMOption == PCM_OPTIONS[cmOpt])
                {
                    pCmData->cmOption = cmOpt;
                    break;
                }
            }
        }

        //
        // Validate the data
        //
        if (SUCCEEDED(hr))
        {
            if (pCmData->cmOption <  EPCMOptionMin ||
                pCmData->cmOption >= EPCMOptionMax)
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

