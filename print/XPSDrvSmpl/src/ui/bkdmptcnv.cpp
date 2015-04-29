/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkdmptcnv.cpp

Abstract:

   Booklet/Binding devmode <-> PrintTicket conversion class implementation. The class
   defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "bkdmptcnv.h"
#include "bkpchndlr.h"

using XDPrintSchema::Binding::BindingData;
using XDPrintSchema::Binding::EBinding;
using XDPrintSchema::Binding::EBindingMin;
using XDPrintSchema::Binding::JobBindAllDocuments;
using XDPrintSchema::Binding::DocumentBinding;
using XDPrintSchema::Binding::EBindingMax;

using XDPrintSchema::Binding::EBindingOption;
using XDPrintSchema::Binding::None;
using XDPrintSchema::Binding::BindLeft;
using XDPrintSchema::Binding::BindRight;
using XDPrintSchema::Binding::BindTop;
using XDPrintSchema::Binding::BindBottom;

PCSTR g_pszBindFeature[EBindingMax] = {
    "JobBindAllDocuments",
    "DocumentBinding",
};
static GPDStringToOption<EBindingOption> g_bindingTypeOption[] = {
    {"None",       None},
    {"BindLeft",   BindLeft},
    {"BindRight",  BindRight},
    {"BindTop",    BindTop},
    {"BindBottom", BindBottom},
};
UINT g_cBindOption = sizeof(g_bindingTypeOption)/sizeof(GPDStringToOption<EBindingOption>);

/*++

Routine Name:

    CBookletDMPTConv::CBookletDMPTConv

Routine Description:

    CBookletDMPTConv class constructor

Arguments:

    None

Return Value:

    None

--*/
CBookletDMPTConv::CBookletDMPTConv()
{
}

/*++

Routine Name:

    CBookletDMPTConv::~CBookletDMPTConv

Routine Description:

    CBookletDMPTConv class destructor

Arguments:

    None

Return Value:

    None

--*/
CBookletDMPTConv::~CBookletDMPTConv()
{
}

/*++

Routine Name:

    CBookletDMPTConv::GetPTDataSettingsFromDM

Routine Description:

    Populates the booklet data structure from the Devmode passed in.

Arguments:

    pDevmode - pointer to input devmode buffer.
    cbDevmode - size in bytes of full input devmode.
    pPrivateDevmode - pointer to input private devmode buffer.
    cbDrvPrivateSize - size in bytes of private devmode.
    pDataSettings - Pointer to booklet data structure to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletDMPTConv::GetPTDataSettingsFromDM(
    _In_    PDEVMODE         pDevmode,
    _In_    ULONG            cbDevmode,
    _In_    PVOID            pPrivateDevmode,
    _In_    ULONG            cbDrvPrivateSize,
    _Out_   BookletSettings* pDataSettings
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
    // Retrieve the GPD and devmode controlled settings for both Job and Document binding
    //
    for (EBinding bindFeature = EBindingMin;
         bindFeature < EBindingMax && SUCCEEDED(hr);
         bindFeature = static_cast<EBinding>(bindFeature + 1))
    {
        pDataSettings->settings[bindFeature].bindFeature = bindFeature;
        hr = GetOptionFromGPDString<EBindingOption>(pDevmode,
                                                    cbDevmode,
                                                    g_pszBindFeature[bindFeature],
                                                    g_bindingTypeOption,
                                                    g_cBindOption,
                                                    pDataSettings->settings[bindFeature].bindOption);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletDMPTConv::MergePTDataSettingsWithPT

Routine Description:

    This method updates the booklet data structure from a PrintTicket description.

Arguments:

    pPrintTicket  - Pointer to the input PrintTicket.
    pDataSettings - Pointer to the booklet data structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletDMPTConv::MergePTDataSettingsWithPT(
    _In_    IXMLDOMDocument2* pPrintTicket,
    _Inout_ BookletSettings*  pDataSettings
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDataSettings, E_POINTER)))
    {
        try
        {
            //
            // Get the binding settings from the PrintTicket and set the options in
            // the appropriate Job or Document equivalent in the BookletSettings structure
            //
            BindingData    bindData;
            CBookPTHandler bkPTHndlr(pPrintTicket);

            if (SUCCEEDED(hr = bkPTHndlr.GetData(&bindData)))
            {
                //
                // Only update settings relevant to the feature
                //
                if (bindData.bindFeature <  EBindingMax &&
                    bindData.bindFeature >= EBindingMin)
                {
                    pDataSettings->settings[bindData.bindFeature] = bindData;
                }
            }
            else if (hr == E_ELEMENT_NOT_FOUND)
            {
                //
                // Binding setting not in the PT - make sure neither Job or
                // Document binding are set in the outgoing data structure
                //
                pDataSettings->settings[JobBindAllDocuments].bindOption = None;
                pDataSettings->settings[DocumentBinding].bindOption = None;

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

    CBookletDMPTConv::SetPTDataInDM

Routine Description:

    This method updates the booklet options in the devmode from the UI Settings.

Arguments:

    dataSettings - Reference to booklet data settings to be updated.
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
CBookletDMPTConv::SetPTDataInDM(
    _In_    CONST BookletSettings& dataSettings,
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

    for (EBinding bindFeature = EBindingMin;
         bindFeature < EBindingMax && SUCCEEDED(hr);
         bindFeature = static_cast<EBinding>(bindFeature + 1))
    {
        hr = SetGPDStringFromOption<EBindingOption>(pDevmode,
                                                    cbDevmode,
                                                    g_pszBindFeature[bindFeature],
                                                    g_bindingTypeOption,
                                                    g_cBindOption,
                                                    dataSettings.settings[bindFeature].bindOption);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CBookletDMPTConv::SetPTDataInPT

Routine Description:

    This method updates the watemark PrintTicket description from booklet data structure.

Arguments:

    dataSettings - Reference to booklet data structure to update from.
    pPrintTicket - Pointer to the PrintTicket to be updated.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBookletDMPTConv::SetPTDataInPT(
    _In_    CONST BookletSettings& dataSettings,
    _Inout_ IXMLDOMDocument2*      pPrintTicket
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        try
        {
            CBookPTHandler bkPTHndlr(pPrintTicket);
            BindingData    bkData;

            //
            // Preferentially set Job over Document
            //
            if (dataSettings.settings[JobBindAllDocuments].bindOption != None)
            {
                bkData.bindFeature = JobBindAllDocuments;
                bkData.bindOption  = dataSettings.settings[JobBindAllDocuments].bindOption;
            }
            else
            {
                bkData.bindFeature = DocumentBinding;
                bkData.bindOption  = dataSettings.settings[DocumentBinding].bindOption;
            }

            //
            // If the option is enabled set, otherwise delete it from the PT
            //
            if (bkData.bindOption != None)
            {
                hr = bkPTHndlr.SetData(&bkData);
            }
            else
            {
                hr = bkPTHndlr.Delete();
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

    CBookletDMPTConv::CompletePrintCapabilities

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
CBookletDMPTConv::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2*,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            CBookPCHandler bookpcHandler(pPrintCapabilities);
            bookpcHandler.SetCapabilities();
        }
        catch(CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
