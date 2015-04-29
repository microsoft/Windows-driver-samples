/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscdmptcnv.h

Abstract:

   PageScaling devmode <-> PrintTicket conversion class definition.
   The class defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#pragma once

#include "ftrdmptcnv.h"
#include "pgscpthndlr.h"
#include "pgscpchndlr.h"
#include "uiproperties.h"

class CPageScalingDMPTConv : public CFeatureDMPTConvert<XDPrintSchema::PageScaling::PageScalingData>
{
public:
    CPageScalingDMPTConv();

    ~CPageScalingDMPTConv();

private:
    HRESULT
    GetPTDataSettingsFromDM(
        _In_  PDEVMODE pDevmode,
        _In_  ULONG    cbDevmode,
        _In_  PVOID    pPrivateDevmode,
        _In_  ULONG    cbDrvPrivateSize,
        _Out_ XDPrintSchema::PageScaling::PageScalingData* pDataSettings
        );

    HRESULT
    MergePTDataSettingsWithPT(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _Inout_ XDPrintSchema::PageScaling::PageScalingData*  pDrvSettings
        );

    HRESULT
    SetPTDataInDM(
        _In_    CONST XDPrintSchema::PageScaling::PageScalingData& drvSettings,
        _Inout_ PDEVMODE pDevmode,
        _In_    ULONG    cbDevmode,
        _Inout_ PVOID    pPrivateDevmode,
        _In_    ULONG    cbDrvPrivateSize
        );

    HRESULT
    SetPTDataInPT(
        _In_    CONST XDPrintSchema::PageScaling::PageScalingData&  drvSettings,
        _Inout_ IXMLDOMDocument2* pPrintTicket
        );

    HRESULT STDMETHODCALLTYPE
    CompletePrintCapabilities(
        _In_opt_ IXMLDOMDocument2*,
        _Inout_  IXMLDOMDocument2* pPrintCapabilities
        );
};

