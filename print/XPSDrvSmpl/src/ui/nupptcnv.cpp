/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupptcnv.cpp

Abstract:

   JobNUpAllDocumentsContiguously and DocumentNUp devmode <-> PrintTicket conversion class implementation.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "nupptcnv.h"
#include "nupchndlr.h"

using XDPrintSchema::NUp::NUpData;
using XDPrintSchema::NUp::ENUpFeature;
using XDPrintSchema::NUp::ENUpFeatureMin;
using XDPrintSchema::NUp::JobNUpAllDocumentsContiguously;
using XDPrintSchema::NUp::DocumentNUp;
using XDPrintSchema::NUp::ENUpFeatureMax;

using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption;
using XDPrintSchema::NUp::PresentationDirection::LeftTop;
using XDPrintSchema::NUp::PresentationDirection::LeftBottom;
using XDPrintSchema::NUp::PresentationDirection::RightTop;
using XDPrintSchema::NUp::PresentationDirection::RightBottom;
using XDPrintSchema::NUp::PresentationDirection::BottomRight;
using XDPrintSchema::NUp::PresentationDirection::BottomLeft;
using XDPrintSchema::NUp::PresentationDirection::TopRight;
using XDPrintSchema::NUp::PresentationDirection::TopLeft;

PCSTR g_pszNUpFeature[ENUpFeatureMax] = {
    "JobNUpAllDocumentsContiguously",
    "DocumentNUp"
};
static GPDStringToOption<INT> g_pagesPerSheetOption[] = {
    {"1",  1},
    {"2",  2},
    {"4",  4},
    {"6",  6},
    {"8",  8},
    {"9",  9},
    {"16", 16},
};
UINT g_cPagesPerSheetOption = sizeof(g_pagesPerSheetOption)/sizeof(GPDStringToOption<INT>);

PCSTR g_pszPresDirFeature[ENUpFeatureMax] = {
    "JobNUpContiguouslyPresentationOrder",
    "DocumentNUpPresentationOrder",
};
static GPDStringToOption<ENUpDirectionOption> g_presDirTypeOption[] = {
    {"RightBottom", RightBottom},
    {"BottomRight", BottomRight},
    {"LeftBottom",  LeftBottom},
    {"BottomLeft",  BottomLeft},
    {"RightTop",    RightTop},
    {"TopRight",    TopRight},
    {"LeftTop",     LeftTop},
    {"TopLeft",     TopLeft},
};
UINT g_cPresDirOption = sizeof(g_presDirTypeOption)/sizeof(GPDStringToOption<ENUpDirectionOption>);


/*++

Routine Name:

    CNUpDMPTConv::CNUpDMPTConv

Routine Description:

    CNUpDMPTConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CNUpDMPTConv::CNUpDMPTConv()
{
}

/*++

Routine Name:

    CNUpDMPTConv::~CNUpDMPTConv

Routine Description:

    CNUpDMPTConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CNUpDMPTConv::~CNUpDMPTConv()
{
}

/*++

Routine Name:

    CNUpDMPTConv::GetPTDataSettingsFromDM

Routine Description:

    Populates the NUp data structure from the Devmode passed in.

Arguments:

    pDevmode - pointer to input devmode buffer.
    cbDevmode - size in bytes of full input devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.
    pDataSettings - Pointer to NUp data structure to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpDMPTConv::GetPTDataSettingsFromDM(
    _In_    PDEVMODE     pDevmode,
    _In_    ULONG        cbDevmode,
    _In_    PVOID        pPrivateDevmode,
    _In_    ULONG        cbDrvPrivateSize,
    _Out_   NUpSettings* pDataSettings
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
    // Retrieve the GPD and devmode controlled settings for both Job and Document NUp
    //
    for (ENUpFeature nUpFeature = ENUpFeatureMin;
         nUpFeature < ENUpFeatureMax && SUCCEEDED(hr);
         nUpFeature = static_cast<ENUpFeature>(nUpFeature + 1))
    {
        pDataSettings->settings[nUpFeature].nUpFeature = nUpFeature;
        if (SUCCEEDED(hr = GetOptionFromGPDString<INT>(pDevmode,
                                                       cbDevmode,
                                                       g_pszNUpFeature[nUpFeature],
                                                       g_pagesPerSheetOption,
                                                       g_cPagesPerSheetOption,
                                                       pDataSettings->settings[nUpFeature].cNUp)))
        {
            hr = GetOptionFromGPDString<ENUpDirectionOption>(pDevmode,
                                                             cbDevmode,
                                                             g_pszPresDirFeature[nUpFeature],
                                                             g_presDirTypeOption,
                                                             g_cPresDirOption,
                                                             pDataSettings->settings[nUpFeature].nUpPresentDir);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpDMPTConv::MergePTDataSettingsWithPT

Routine Description:

    This method updates the NUp data structure from a PrintTicket description.

Arguments:

    pPrintTicket  - Pointer to the input PrintTicket.
    pDataSettings - Pointer to the NUp data structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpDMPTConv::MergePTDataSettingsWithPT(
    _In_    IXMLDOMDocument2* pPrintTicket,
    _Inout_ NUpSettings*      pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        try
        {
            //
            // Get the NUp settings from the PrintTicket and set the options in
            // the appropriate Job or Document equivalent in the NUpSettings structure
            //
            NUpData       nUpData;
            CNUpPTHandler nUpPTHndlr(pPrintTicket);

            if (SUCCEEDED(hr = nUpPTHndlr.GetData(&nUpData)))
            {
                //
                // Only update settings relevant to the feature
                //
                if (nUpData.nUpFeature <  ENUpFeatureMax &&
                    nUpData.nUpFeature >= ENUpFeatureMin)
                {
                    pDataSettings->settings[nUpData.nUpFeature] = nUpData;
                }
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // NUp setting not in the PT - make sure neither Job or
                // Document NUp are set in the outgoing data structure
                //
                pDataSettings->settings[JobNUpAllDocumentsContiguously].cNUp = 1;
                pDataSettings->settings[DocumentNUp].cNUp = 1;

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

    CNUpDMPTConv::SetPTDataInDM

Routine Description:

    This method updates the NUp options in the devmode from the UI Settings.

Arguments:

    dataSettings - Reference to NUp data settings to be updated.
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
CNUpDMPTConv::SetPTDataInDM(
    _In_    CONST NUpSettings& dataSettings,
    _Inout_ PDEVMODE           pDevmode,
    _In_    ULONG              cbDevmode,
    _Inout_ PVOID              pPrivateDevmode,
    _In_    ULONG              cbDrvPrivateSize
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

    for (ENUpFeature nUpFeature = ENUpFeatureMin;
         nUpFeature < ENUpFeatureMax && SUCCEEDED(hr);
         nUpFeature = static_cast<ENUpFeature>(nUpFeature + 1))
    {
        if (SUCCEEDED(hr = SetGPDStringFromOption<INT>(pDevmode,
                                                       cbDevmode,
                                                       g_pszNUpFeature[nUpFeature],
                                                       g_pagesPerSheetOption,
                                                       g_cPagesPerSheetOption,
                                                       dataSettings.settings[nUpFeature].cNUp)))
        {
            hr = SetGPDStringFromOption<ENUpDirectionOption>(pDevmode,
                                                             cbDevmode,
                                                             g_pszPresDirFeature[nUpFeature],
                                                             g_presDirTypeOption,
                                                             g_cPresDirOption,
                                                             dataSettings.settings[nUpFeature].nUpPresentDir);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpDMPTConv::SetPTDataInPT

Routine Description:

    This method updates the watemark PrintTicket description from NUp data structure.

Arguments:

    dataSettings - Reference to NUp data structure to update from.
    pPrintTicket - Pointer to the PrintTicket to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpDMPTConv::SetPTDataInPT(
    _In_    CONST NUpSettings& dataSettings,
    _Inout_ IXMLDOMDocument2*  pPrintTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        try
        {
            CNUpPTHandler nUpPTHndlr(pPrintTicket);
            NUpData       nUpData;

            //
            // Preferentially set Job over Document
            //
            if (dataSettings.settings[JobNUpAllDocumentsContiguously].cNUp > 1)
            {
                nUpData.nUpFeature    = JobNUpAllDocumentsContiguously;
                nUpData.cNUp          = dataSettings.settings[JobNUpAllDocumentsContiguously].cNUp;
                nUpData.nUpPresentDir = dataSettings.settings[JobNUpAllDocumentsContiguously].nUpPresentDir;
            }
            else
            {
                nUpData.nUpFeature    = DocumentNUp;
                nUpData.cNUp          = dataSettings.settings[DocumentNUp].cNUp;
                nUpData.nUpPresentDir = dataSettings.settings[DocumentNUp].nUpPresentDir;
            }

            //
            // If the option is enabled set, otherwise delete it from the PT
            //
            if (nUpData.cNUp > 1)
            {
                hr = nUpPTHndlr.SetData(&nUpData);
            }
            else
            {
                hr = nUpPTHndlr.Delete();
            }
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

    CNUpDMPTConv::CompletePrintCapabilities

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
CNUpDMPTConv::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2*,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            CNUpPCHandler nuppcHandler(pPrintCapabilities);
            nuppcHandler.SetCapabilities();
        }
        catch(CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
