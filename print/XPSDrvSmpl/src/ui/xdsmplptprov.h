/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplptprov.h

Abstract:

   Definition of the PrintTicket provider plugin. This is responsible for
   handling PrintTicket features that are too complex for the GPD->PrintTicket
   automapping facility.

--*/

#pragma once

#include "cunknown.h"
#include "schema.h"
#include "ftrdmptcnv.h"

typedef vector<IFeatureDMPTConvert*> DMPTConvCollection;

class CXDSmplPTProvider : public CUnknown<IPrintOemPrintTicketProvider>
{
public:
    CXDSmplPTProvider();

    virtual ~CXDSmplPTProvider();

    //
    // IPrintOemPrintTicketProvider methods
    //
    virtual HRESULT STDMETHODCALLTYPE
    GetSupportedVersions(
        _In_                            HANDLE  hPrinter,
        _Outptr_result_buffer_(*pcVersions) INT*    ppVersions[],
        _Out_                           INT*    pcVersions
        );

    virtual HRESULT STDMETHODCALLTYPE
    BindPrinter(
        _In_                                  HANDLE     hPrinter,
                                              INT        version,
        _Out_                                 POEMPTOPTS pOptions,
        _Out_                                 INT*       pcNamespaces,
        _Outptr_result_buffer_maybenull_(*pcNamespaces) BSTR**     ppNamespaces
        );

    virtual HRESULT STDMETHODCALLTYPE
    PublishPrintTicketHelperInterface(
        _In_ IUnknown* pHelper
        );

    virtual HRESULT STDMETHODCALLTYPE
    QueryDeviceDefaultNamespace(
        _Out_ BSTR* pbstrNamespaceUri
        );

    virtual HRESULT STDMETHODCALLTYPE
    ConvertPrintTicketToDevMode(
        _In_    IXMLDOMDocument2* pPrintTicket,
                ULONG             cbDevmode,
        _Inout_updates_bytes_(cbDevmode)
                PDEVMODE          pDevmode,
                ULONG             cbDrvPrivateSize,
        _Inout_ PVOID             pPrivateDevmode
        );

    virtual HRESULT STDMETHODCALLTYPE
    ConvertDevModeToPrintTicket(
                ULONG             cbDevmode,
        _Inout_updates_bytes_(cbDevmode) 
                PDEVMODE          pDevmode,
                ULONG             cbDrvPrivateSize,
        _Inout_ PVOID             pPrivateDevmode,
        _Inout_ IXMLDOMDocument2* pPrintTicket
        );

    virtual HRESULT STDMETHODCALLTYPE
    CompletePrintCapabilities(
        _In_opt_ IXMLDOMDocument2* pPrintTicket,
        _Inout_  IXMLDOMDocument2* pCapabilities
        );

    virtual HRESULT STDMETHODCALLTYPE
    ExpandIntentOptions(
        _Inout_ IXMLDOMDocument2* pPrintTicket
        );

    virtual HRESULT STDMETHODCALLTYPE
    ValidatePrintTicket(
        _Inout_ IXMLDOMDocument2* pPrintTicket
        );

private:
    HRESULT
    DeleteConverters(
        VOID
        );

    HRESULT
    AddConverter(
        _In_opt_ __drv_aliasesMem IFeatureDMPTConvert* pHandler
        );

private:
    HANDLE                       m_hPrinterCached;

    CComPtr<IPrintCoreHelperUni> m_pCoreHelper;

    DMPTConvCollection           m_vectFtrDMPTConverters;

    CComBSTR                     m_bstrPrivateNS;
};

