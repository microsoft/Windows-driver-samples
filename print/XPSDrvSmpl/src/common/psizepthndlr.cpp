/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   psizepthndlr.cpp

Abstract:

   PageMediaSize PrintTicket handler implementation. Derived from CPTHandler,
   this provides PageMediaSize specific Get method acting on the passed in
   PrintTicket (as a DOM document).

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "psizepthndlr.h"

using XDPrintSchema::PageMediaSize::PageMediaSizeData;
using XDPrintSchema::PageMediaSize::MediaSizeHeight;
using XDPrintSchema::PageMediaSize::MediaSizeWidth;
using XDPrintSchema::PageMediaSize::PAGESIZE_FEATURE;
using XDPrintSchema::PageMediaSize::PAGESIZE_PROPS;

/*++

Routine Name:

    CPageSizePTHandler::CPageSizePTHandler

Routine Description:

    CPageSizePTHandler class constructor

Arguments:

    pPrintTicket - Pointer to the DOM document representation of the PrintTicket

Return Value:

    None

--*/
CPageSizePTHandler::CPageSizePTHandler(
    _In_ IXMLDOMDocument2* pPrintTicket
    ) :
    CPTHandler(pPrintTicket)
{
}

/*++

Routine Name:

    CPageSizePTHandler::~CPageSizePTHandler

Routine Description:

    CPageSizePTHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageSizePTHandler::~CPageSizePTHandler()
{
}

/*++

Routine Name:

    CPageSizePTHandler::GetData

Routine Description:

    The routine fills the data structure passed in with page size data retrieved from
    the PrintTicket passed to the class constructor.

Arguments:

    pPageMediaSizeData - Pointer to the page size data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintTicket
    E_*                 - On error

--*/
HRESULT
CPageSizePTHandler::GetData(
    _Out_ PageMediaSizeData* pPageMediaSizeData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageMediaSizeData, E_POINTER)))
    {
        CComBSTR bstrPageSizeOption;

        //
        // We don't actually need the pages size option name, just the dimensions
        //
        if (SUCCEEDED(hr = GetFeatureOption(CComBSTR(PAGESIZE_FEATURE), &bstrPageSizeOption)) &&
            SUCCEEDED(hr = GetScoredPropertyValue(CComBSTR(PAGESIZE_FEATURE),
                                                  CComBSTR(PAGESIZE_PROPS[MediaSizeWidth]),
                                                  &pPageMediaSizeData->pageWidth)))
        {
            hr = GetScoredPropertyValue(CComBSTR(PAGESIZE_FEATURE),
                                        CComBSTR(PAGESIZE_PROPS[MediaSizeHeight]),
                                        &pPageMediaSizeData->pageHeight);
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

