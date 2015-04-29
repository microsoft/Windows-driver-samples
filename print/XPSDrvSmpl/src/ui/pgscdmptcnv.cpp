/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscdmptcnv.cpp

Abstract:

   PageScaling devmode <-> PrintTicket conversion class implementation.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "pgscdmptcnv.h"

using XDPrintSchema::PageScaling::PageScalingData;
using XDPrintSchema::PageScaling::EScaleOption;
using XDPrintSchema::PageScaling::None;
using XDPrintSchema::PageScaling::Custom;
using XDPrintSchema::PageScaling::CustomSquare;
using XDPrintSchema::PageScaling::FitBleedToImageable;
using XDPrintSchema::PageScaling::FitContentToImageable;
using XDPrintSchema::PageScaling::FitMediaToImageable;
using XDPrintSchema::PageScaling::FitMediaToMedia;

using XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOption;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomLeft;
using XDPrintSchema::PageScaling::OffsetAlignment::BottomRight;
using XDPrintSchema::PageScaling::OffsetAlignment::Center;
using XDPrintSchema::PageScaling::OffsetAlignment::LeftCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::RightCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::TopCenter;
using XDPrintSchema::PageScaling::OffsetAlignment::TopLeft;
using XDPrintSchema::PageScaling::OffsetAlignment::TopRight;

//
// Lookup table between GPD page scaling option string and schema enumerated type
//
PCSTR g_pszPageScalingTypeFeature = "PageScaling";
static GPDStringToOption<EScaleOption> g_pageScalingTypeOption[] = {
    {"None",                                         None},
    {"Custom",                                       Custom},
    {"CustomSquare",                                 CustomSquare},
    {"FitApplicationBleedSizeToPageImageableSize",   FitBleedToImageable},
    {"FitApplicationContentSizeToPageImageableSize", FitContentToImageable},
    {"FitApplicationMediaSizeToPageImageableSize",   FitMediaToImageable},
    {"FitApplicationMediaSizeToPageMediaSize",       FitMediaToMedia},
};
UINT g_cPageScalingTypeOption = sizeof(g_pageScalingTypeOption)/sizeof(GPDStringToOption<EScaleOption>);

//
// Lookup table between GPD scaling offset alignment option string and schema enumerated type
//
PCSTR g_pszScaleOffsetTypeFeature = "ScaleOffsetAlignment";
static GPDStringToOption<EScaleOffsetOption> g_scaleOffsetTypeOption[] = {
    {"BottomCenter", BottomCenter},
    {"BottomLeft",   BottomLeft},
    {"BottomRight",  BottomRight},
    {"Center",       Center},
    {"LeftCenter",   LeftCenter},
    {"RightCenter",  RightCenter},
    {"TopCenter",    TopCenter},
    {"TopLeft",      TopLeft},
    {"TopRight",     TopRight},
};
UINT g_cScaleOffsetTypeOption = sizeof(g_scaleOffsetTypeOption)/sizeof(GPDStringToOption<EScaleOffsetOption>);



/*++

Routine Name:

    CPageScalingDMPTConv::CPageScalingDMPTConv

Routine Description:

    CPageScalingDMPTConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingDMPTConv::CPageScalingDMPTConv()
{
}

/*++

Routine Name:

    CPageScalingDMPTConv::~CPageScalingDMPTConv

Routine Description:

    CPageScalingDMPTConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingDMPTConv::~CPageScalingDMPTConv()
{
}

/*++

Routine Name:

    CPageScalingDMPTConv::GetPTDataSettingsFromDM

Routine Description:

    Populates the page scaling data structure from the Devmode passed in.

Arguments:

    pDevmode - pointer to input devmode buffer.
    cbDevmode - size in bytes of full input devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.
    pDataSettings - Pointer to page scaling data structure to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingDMPTConv::GetPTDataSettingsFromDM(
    _In_  PDEVMODE         pDevmode,
    _In_  ULONG            cbDevmode,
    _In_  PVOID            pPrivateDevmode,
    _In_  ULONG            cbDrvPrivateSize,
    _Out_ PageScalingData* pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Retrieve the GPD and devmode controlled settings
    //
    CUIProperties uiProperties(static_cast<POEMDEV>(pPrivateDevmode));
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetOptionFromGPDString<EScaleOption>(pDevmode,
                                                            cbDevmode,
                                                            g_pszPageScalingTypeFeature,
                                                            g_pageScalingTypeOption,
                                                            g_cPageScalingTypeOption,
                                                            pDataSettings->pgscOption)) &&
        SUCCEEDED(hr = GetOptionFromGPDString<EScaleOffsetOption>(pDevmode,
                                                                  cbDevmode,
                                                                  g_pszScaleOffsetTypeFeature,
                                                                  g_scaleOffsetTypeOption,
                                                                  g_cScaleOffsetTypeOption,
                                                                  pDataSettings->offsetOption)) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszPSScaleWidth, &pDataSettings->scaleWidth, sizeof(pDataSettings->scaleWidth))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszPSScaleHeight, &pDataSettings->scaleHeight, sizeof(pDataSettings->scaleHeight))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszOffsetWidth, &pDataSettings->offWidth, sizeof(pDataSettings->offWidth))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszOffsetHeight, &pDataSettings->offHeight, sizeof(pDataSettings->offHeight))))
    {
        //
        // Convert length measurements from 100ths of an inch to microns
        //
        pDataSettings->scaleWidth  = pDataSettings->scaleWidth;
        pDataSettings->scaleHeight = pDataSettings->scaleHeight;
        pDataSettings->offWidth    = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->offWidth);
        pDataSettings->offHeight   = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->offHeight);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScalingDMPTConv::MergePTDataSettingsWithPT

Routine Description:

    This method updates the page scaling data structure from a PrintTicket description.

Arguments:

    pPrintTicket  - Pointer to the input PrintTicket.
    pDataSettings - Pointer to the page scaling data structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingDMPTConv::MergePTDataSettingsWithPT(
    _In_    IXMLDOMDocument2* pPrintTicket,
    _Inout_ PageScalingData*  pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        try
        {
            //
            // Get the page scaling settings from the PrintTicket and set the options in
            // the input Watermark data structure
            //
            PageScalingData       pgScData;
            CPageScalingPTHandler pgScPTHndlr(pPrintTicket);

            if (SUCCEEDED(hr = pgScPTHndlr.GetData(&pgScData)))
            {
                *pDataSettings = pgScData;
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // PageScaling setting not in the PT - this is not an error. Just
                // leave set the type as None and reset the HRESULT to S_OK
                //
                pDataSettings->pgscOption = None;
                hr = S_OK;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScalingDMPTConv::SetPTDataInDM

Routine Description:

    This method updates the page scaling options in the devmode from the UI Settings.

Arguments:

    dataSettings - Reference to page scaling data settings to be updated.
    pDevmode - pointer to devmode to be updated.
    cbDevmode - size in bytes of full devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingDMPTConv::SetPTDataInDM(
    _In_    CONST PageScalingData& dataSettings,
    _Inout_ PDEVMODE               pDevmode,
    _In_    ULONG                  cbDevmode,
    _Inout_ PVOID                  pPrivateDevmode,
    _In_    ULONG                  cbDrvPrivateSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // Convert lengths from microns to 100ths of an inch before writing to the DevMode
    //
    if (SUCCEEDED(hr))
    {
        INT scaleWidth  = dataSettings.scaleWidth;
        INT scaleHeight = dataSettings.scaleHeight;
        INT offWidth    = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.offWidth);
        INT offHeight   = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.offHeight);

        //
        // Set the GPD and devmode controlled settings
        //
        CUIProperties uiProperties(static_cast<POEMDEV>(pPrivateDevmode));

        if (SUCCEEDED(hr = SetGPDStringFromOption<EScaleOption>(pDevmode,
                                                                cbDevmode,
                                                                g_pszPageScalingTypeFeature,
                                                                g_pageScalingTypeOption,
                                                                g_cPageScalingTypeOption,
                                                                dataSettings.pgscOption)) &&
            SUCCEEDED(hr = SetGPDStringFromOption<EScaleOffsetOption>(pDevmode,
                                                                      cbDevmode,
                                                                      g_pszScaleOffsetTypeFeature,
                                                                      g_scaleOffsetTypeOption,
                                                                      g_cScaleOffsetTypeOption,
                                                                      dataSettings.offsetOption)) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszPSScaleWidth, &scaleWidth, sizeof(scaleWidth))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszPSScaleHeight, &scaleHeight, sizeof(scaleHeight))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszOffsetWidth, &offWidth, sizeof(offWidth))))
        {
            hr = uiProperties.SetItem(g_pszOffsetHeight, &offHeight, sizeof(offHeight));
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScalingDMPTConv::SetPTDataInPT

Routine Description:

    This method updates the watemark PrintTicket description from page scaling data structure.

Arguments:

    drvSettings  - Reference to page scaling data structure to update from.
    pPrintTicket - Pointer to the PrintTicket to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingDMPTConv::SetPTDataInPT(
    _In_    CONST PageScalingData& dataSettings,
    _Inout_ IXMLDOMDocument2*      pPrintTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        try
        {
            CPageScalingPTHandler pgScPTHndlr(pPrintTicket);
            hr = pgScPTHndlr.SetData(&dataSettings);
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPageScalingDMPTConv::CompletePrintCapabilities

Routine Description:

    Unidrv calls this routine with an input Device Capabilities Document
    that is partially populated with Device capabilities information
    filled in by Unidrv for features that it understands. The plug-in
    needs to read any private features in the input PrintTicket, delete
    them and add them back under Printschema namespace so that higher
    level applications can understand them and make use of them.

Arguments:

    pPrintTicket - pointer to input PrintTicket
    pCapabilities - pointer to Device Capabilities Document.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CPageScalingDMPTConv::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2*,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            CPageScalingPCHandler pagescalingpcHandler(pPrintCapabilities);
            pagescalingpcHandler.SetCapabilities();
        }
        catch(CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
