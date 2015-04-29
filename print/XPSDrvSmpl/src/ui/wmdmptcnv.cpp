/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmdmptcnv.cpp

Abstract:

   PageWatermark devmode <-> PrintTicket conversion class implementation.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "wmdmptcnv.h"
#include "wmpchndlr.h"

using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::NoWatermark;
using XDPrintSchema::PageWatermark::TextWatermark;
using XDPrintSchema::PageWatermark::BitmapWatermark;
using XDPrintSchema::PageWatermark::VectorWatermark;
using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::WatermarkData;

using XDPrintSchema::PageWatermark::Layering::ELayeringOption;
using XDPrintSchema::PageWatermark::Layering::Overlay;
using XDPrintSchema::PageWatermark::Layering::Underlay;

//
// Look-up data converting GPD PageWatermarkType feature options to
// watermark type enumeration
//
PCSTR g_pszWatermarkTypeFeature = "PageWatermarkType";
static GPDStringToOption<EWatermarkOption> g_watermarkTypeOption[] = {
    {"None",   NoWatermark},
    {"Text",   TextWatermark},
    {"Raster", BitmapWatermark},
    {"Vector", VectorWatermark},
};
UINT g_cWatermarkTypeOption = sizeof(g_watermarkTypeOption)/sizeof(GPDStringToOption<EWatermarkOption>);

//
// Look-up data converting GPD PageWatermarkLayering feature options to
// watermark layering enumeration
//
PCSTR g_pszLayeringFeature = "PageWatermarkLayering";
static GPDStringToOption<ELayeringOption> g_watermarkLayeringOption[] = {
    {"Overlay",  Overlay},
    {"Underlay", Underlay},
};
UINT g_cLayeringOption = sizeof(g_watermarkLayeringOption)/sizeof(GPDStringToOption<ELayeringOption>);

/*++

Routine Name:

    CWatermarkDMPTConv::CWatermarkDMPTConv

Routine Description:

    CWatermarkDMPTConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CWatermarkDMPTConv::CWatermarkDMPTConv()
{
}

/*++

Routine Name:

    CWatermarkDMPTConv::~CWatermarkDMPTConv

Routine Description:

    CWatermarkDMPTConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CWatermarkDMPTConv::~CWatermarkDMPTConv()
{
}

/*++

Routine Name:

    CWatermarkDMPTConv::GetPTDataSettingsFromDM

Routine Description:

    Populates the watermark data structure from the Devmode passed in.

Arguments:

    pDevmode - pointer to input devmode buffer.
    cbDevmode - size in bytes of full input devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.
    pDataSettings - Pointer to watermark data structure to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkDMPTConv::GetPTDataSettingsFromDM(
    _In_  PDEVMODE       pDevmode,
    _In_  ULONG          cbDevmode,
    _In_  PVOID          pPrivateDevmode,
    _In_  ULONG          cbDrvPrivateSize,
    _Out_ WatermarkData* pDataSettings
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

    DWORD dwTextColor = 0;
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetOptionFromGPDString<EWatermarkOption>(pDevmode,
                                                                cbDevmode,
                                                                g_pszWatermarkTypeFeature,
                                                                g_watermarkTypeOption,
                                                                g_cWatermarkTypeOption,
                                                                pDataSettings->type)) &&
        SUCCEEDED(hr = GetOptionFromGPDString<ELayeringOption>(pDevmode,
                                                               cbDevmode,
                                                               g_pszLayeringFeature,
                                                               g_watermarkLayeringOption,
                                                               g_cLayeringOption,
                                                               pDataSettings->layering)) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMOffsetWidth, &pDataSettings->widthOrigin, sizeof(pDataSettings->widthOrigin))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMOffsetHeight, &pDataSettings->heightOrigin, sizeof(pDataSettings->heightOrigin))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMSizeWidth, &pDataSettings->widthExtent, sizeof(pDataSettings->widthExtent))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMSizeHeight, &pDataSettings->heightExtent, sizeof(pDataSettings->heightExtent))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMTransparency, &pDataSettings->transparency, sizeof(pDataSettings->transparency))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMAngle, &pDataSettings->angle, sizeof(pDataSettings->angle))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMFontSize, &pDataSettings->txtData.fontSize, sizeof(pDataSettings->txtData.fontSize))) &&
        SUCCEEDED(hr = uiProperties.GetItem(g_pszWMFontColor, &dwTextColor, sizeof(dwTextColor))))
    {
        //
        // Convert the color to the txtData BSTR
        //
        try
        {
            CStringXDW cstrColor;
            cstrColor.Format(L"#%08X", dwTextColor);
            pDataSettings->txtData.bstrFontColor.Empty();
            pDataSettings->txtData.bstrFontColor.Attach(cstrColor.AllocSysString());
        }
        catch (CXDException& e)
        {
            hr = e;
        }

        TCHAR text[MAX_WATERMARK_TEXT + 1] = {0};

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = uiProperties.GetItem(g_pszWMText, text, MAX_WATERMARK_TEXT * sizeof(TCHAR))))
        {
            pDataSettings->txtData.bstrText = text;

            //
            // Convert measurements from 100ths of an inch to microns
            //
            pDataSettings->widthOrigin  = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->widthOrigin);
            pDataSettings->heightOrigin = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->heightOrigin);
            pDataSettings->widthExtent  = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->widthExtent);
            pDataSettings->heightExtent = HUNDREDTH_OFINCH_TO_MICRON(pDataSettings->heightExtent);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkDMPTConv::MergePTDataSettingsWithPT

Routine Description:

    This method updates the watermark data structure from a PrintTicket description.

Arguments:

    pPrintTicket  - Pointer to the input PrintTicket.
    pDataSettings - Pointer to the watermark data structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkDMPTConv::MergePTDataSettingsWithPT(
    _In_    IXMLDOMDocument2* pPrintTicket,
    _Inout_ WatermarkData*    pDataSettings
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
            // Get the watermark settings from the PrintTicket and set the options in
            // the input Watermark data structure
            //
            WatermarkData wmData;
            CWMPTHandler  wmPTHndlr(pPrintTicket);

            if (SUCCEEDED(hr = wmPTHndlr.GetData(&wmData)))
            {
                //
                // Only update settings relevant to the feature so that we do not unset
                // other watermark settings in the devmode
                //
                pDataSettings->type = NoWatermark;
                switch (wmData.type)
                {
                    case TextWatermark:
                    {
                        pDataSettings->type = TextWatermark;
                        pDataSettings->widthOrigin = wmData.widthOrigin;
                        pDataSettings->heightOrigin = wmData.heightOrigin;
                        pDataSettings->transparency = wmData.transparency;
                        pDataSettings->angle = wmData.angle;
                        pDataSettings->layering = wmData.layering;
                        pDataSettings->txtData.bstrFontColor = wmData.txtData.bstrFontColor;
                        pDataSettings->txtData.fontSize = wmData.txtData.fontSize;
                        pDataSettings->txtData.bstrText = wmData.txtData.bstrText;
                    }
                    break;

                    case BitmapWatermark:
                    {
                        pDataSettings->type = BitmapWatermark;
                        pDataSettings->widthOrigin = wmData.widthOrigin;
                        pDataSettings->heightOrigin = wmData.heightOrigin;
                        pDataSettings->widthExtent = wmData.widthExtent;
                        pDataSettings->heightExtent = wmData.heightExtent;
                        pDataSettings->transparency = wmData.transparency;
                        pDataSettings->angle = wmData.angle;
                        pDataSettings->layering = wmData.layering;
                    }
                    break;

                    case VectorWatermark:
                    {
                        pDataSettings->type = VectorWatermark;
                        pDataSettings->widthOrigin = wmData.widthOrigin;
                        pDataSettings->heightOrigin = wmData.heightOrigin;
                        pDataSettings->widthExtent = wmData.widthExtent;
                        pDataSettings->heightExtent = wmData.heightExtent;
                        pDataSettings->transparency = wmData.transparency;
                        pDataSettings->angle = wmData.angle;
                        pDataSettings->layering = wmData.layering;
                    }
                    break;

                    case NoWatermark:
                    break;

                    default:
                    {
                        WARNING("Unrecognized watermark feature - setting to default\n");
                    }
                    break;
                }
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // Watermark setting not in the PT - this is not an error. Just
                // leave the type as NoWatermark and reset the HRESULT to S_OK
                //
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

    CWatermarkDMPTConv::SetPTDataInDM

Routine Description:

    This method updates the watermark options in the devmode from the UI Settings.

Arguments:

    dataSettings - Reference to watermark data settings to be updated.
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
CWatermarkDMPTConv::SetPTDataInDM(
    _In_    CONST WatermarkData& dataSettings,
    _Inout_ PDEVMODE             pDevmode,
    _In_    ULONG                cbDevmode,
    _Inout_ PVOID                pPrivateDevmode,
    _In_    ULONG                cbDrvPrivateSize
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
    // Convert from microns to 100ths of an inch before writing to the DevMode
    //
    if (SUCCEEDED(hr))
    {
        INT widthOrigin  = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.widthOrigin);
        INT heightOrigin = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.heightOrigin);
        INT widthExtent  = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.widthExtent);
        INT heightExtent = MICRON_TO_HUNDREDTH_OFINCH(dataSettings.heightExtent);

        //
        // Set the GPD and devmode controlled settings
        //
        CUIProperties uiProperties(static_cast<POEMDEV>(pPrivateDevmode));
        if (SUCCEEDED(hr = SetGPDStringFromOption<EWatermarkOption>(pDevmode,
                                                                    cbDevmode,
                                                                    g_pszWatermarkTypeFeature,
                                                                    g_watermarkTypeOption,
                                                                    g_cWatermarkTypeOption,
                                                                    dataSettings.type)) &&
            SUCCEEDED(hr = SetGPDStringFromOption<ELayeringOption>(pDevmode,
                                                                   cbDevmode,
                                                                   g_pszLayeringFeature,
                                                                   g_watermarkLayeringOption,
                                                                   g_cLayeringOption,
                                                                   dataSettings.layering)) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMOffsetWidth, &widthOrigin, sizeof(widthOrigin))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMOffsetHeight, &heightOrigin, sizeof(heightOrigin))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMSizeWidth, &widthExtent, sizeof(widthExtent))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMSizeHeight, &heightExtent, sizeof(heightExtent))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMTransparency, &dataSettings.transparency, sizeof(dataSettings.transparency))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMAngle, &dataSettings.angle, sizeof(dataSettings.angle))) &&
            SUCCEEDED(hr = uiProperties.SetItem(g_pszWMFontSize, &dataSettings.txtData.fontSize, sizeof(dataSettings.txtData.fontSize))))
        {
            try
            {
                //
                // Convert the font color to a channel array
                //
                BYTE fontColor[4] = {0};
                CStringXDW cstrFontColor(dataSettings.txtData.bstrFontColor);
                cstrFontColor.Trim();
                if (cstrFontColor.Find(L"#") == 0)
                {
                    cstrFontColor.Delete(0);
                    cstrFontColor.Trim();
                }

                cstrFontColor.Truncate(8);
                INT cChannel = 3;
                while (cstrFontColor.GetLength() > 0 &&
                       cChannel < sizeof(DWORD) &&
                       cChannel >= 0)
                {
                    //
                    // Add the data a channel at a time so we dont overflow wcstol
                    //
#pragma prefast(suppress:__WARNING_MUST_USE, "All possible two-digit hex numbers are valid.")
                    fontColor[cChannel] = static_cast<BYTE>(wcstol(cstrFontColor.Left(2), NULL, 16));

                    //
                    // Delete the channel from the color ref string
                    //
                    cstrFontColor.Delete(0, 2);

                    //
                    // Traverse array in reverse to correct for endianness
                    //
                    cChannel--;
                }

                hr = uiProperties.SetItem(g_pszWMFontColor, fontColor, sizeof(DWORD));

                //
                // Set the text string
                //
                if (SUCCEEDED(hr))
                {
                    CStringXD text(dataSettings.txtData.bstrText);
                    text.Truncate(MAX_WATERMARK_TEXT);

                    hr = uiProperties.SetItem(g_pszWMText, text.GetBuffer(), (text.GetLength() + 1) * sizeof(TCHAR));
                }
            }
            catch (CXDException& e)
            {
                hr = e;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkDMPTConv::SetPTDataInPT

Routine Description:

    This method updates the watemark PrintTicket description from watermark data structure.

Arguments:

    drvSettings  - Reference to watermark data structure to update from.
    pPrintTicket - Pointer to the PrintTicket to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkDMPTConv::SetPTDataInPT(
    _In_    CONST WatermarkData& dataSettings,
    _Inout_ IXMLDOMDocument2*    pPrintTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        try
        {
            CWMPTHandler  wmPTHndlr(pPrintTicket);
            hr = wmPTHndlr.SetData(&dataSettings);
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

    CWatermarkDMPTConv::CompletePrintCapabilities

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
    S_OK - Always

--*/
HRESULT STDMETHODCALLTYPE
CWatermarkDMPTConv::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2*,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            CWMPCHandler watermarkpcHandler(pPrintCapabilities);
            watermarkpcHandler.SetCapabilities();
        }
        catch(CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

