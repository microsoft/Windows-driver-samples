/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkdmptcnv.h

Abstract:

   Booklet/Binding devmode <-> PrintTicket conversion class definition. The class
   defines a common data representation between the DevMode (GPD) and PrintTicket
   representations and implements the conversion and validation methods required
   by CFeatureDMPTConvert.

--*/

#pragma once

#include "ftrdmptcnv.h"
#include "bkpthndlr.h"

//
// The PT handling code defines a single BindingData structure that covers both
// JobBindAllDocuments and DocumentBinding to avoid conflicts. The GPD however
// controls both so we re-use the BindingData structure to handle both in
// the DevMode
//
struct BookletSettings
{
    XDPrintSchema::Binding::BindingData settings[XDPrintSchema::Binding::EBindingMax];
};

class CBookletDMPTConv : public CFeatureDMPTConvert<BookletSettings>
{
public:
    CBookletDMPTConv();

    ~CBookletDMPTConv();

private:
    HRESULT
    GetPTDataSettingsFromDM(
        _In_    PDEVMODE         pDevmode,
        _In_    ULONG            cbDevmode,
        _In_    PVOID            pPrivateDevmode,
        _In_    ULONG            cbDrvPrivateSize,
        _Out_   BookletSettings* pDataSettings
        );

    HRESULT
    MergePTDataSettingsWithPT(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _Inout_ BookletSettings*  pDrvSettings
        );

    HRESULT
    SetPTDataInDM(
        _In_    CONST BookletSettings& drvSettings,
        _Inout_ PDEVMODE               pDevmode,
        _In_    ULONG                  cbDevmode,
        _Inout_ PVOID                  pPrivateDevmode,
        _In_    ULONG                  cbDrvPrivateSize
        );

    HRESULT
    SetPTDataInPT(
        _In_    CONST BookletSettings& drvSettings,
        _Inout_ IXMLDOMDocument2*      pPrintTicket
        );

    HRESULT STDMETHODCALLTYPE
    CompletePrintCapabilities(
        _In_opt_ IXMLDOMDocument2* pPrintTicket,
        _Inout_  IXMLDOMDocument2* pPrintCapabilities
        );
};
