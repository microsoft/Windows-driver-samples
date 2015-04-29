/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupptcnv.h

Abstract:

   JobNUpAllDocumentsContiguously and DocumentNUp devmode <-> PrintTicket conversion class definition.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#pragma once

#include "ftrdmptcnv.h"
#include "nupthndlr.h"
#include "nupdata.h"

//
// The PT handling code defines a single NUpData structure that covers
// both JobNUpAllDocumentsContiguously and DocumentNUp to avoid conflicts. The GPD however
// controls both so we re-use the NUpData structure to handle both in
// the DevMode by using a NUpData for each Job and Document feature
//
struct NUpSettings
{
    XDPrintSchema::NUp::NUpData settings[XDPrintSchema::NUp::ENUpFeatureMax];
};

class CNUpDMPTConv : public CFeatureDMPTConvert<NUpSettings>
{
public:
    CNUpDMPTConv();

    virtual ~CNUpDMPTConv();

private:
    HRESULT
    GetPTDataSettingsFromDM(
        _In_    PDEVMODE     pDevmode,
        _In_    ULONG        cbDevmode,
        _In_    PVOID        pPrivateDevmode,
        _In_    ULONG        cbDrvPrivateSize,
        _Out_   NUpSettings* pDataSettings
        );

    HRESULT
    MergePTDataSettingsWithPT(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _Inout_ NUpSettings*      pDrvSettings
        );

    HRESULT
    SetPTDataInDM(
        _In_    CONST NUpSettings& drvSettings,
        _Inout_ PDEVMODE           pDevmode,
        _In_    ULONG              cbDevmode,
        _Inout_ PVOID              pPrivateDevmode,
        _In_    ULONG              cbDrvPrivateSize
        );

    HRESULT
    SetPTDataInPT(
        _In_    CONST NUpSettings& drvSettings,
        _Inout_ IXMLDOMDocument2*  pPrintTicket
        );

    HRESULT STDMETHODCALLTYPE
    CompletePrintCapabilities(
        _In_opt_ IXMLDOMDocument2*,
        _Inout_  IXMLDOMDocument2* pPrintCapabilities
        );
};

