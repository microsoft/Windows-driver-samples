//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   oemptprovider.hxx
//    
//
//  PURPOSE:  Definition of IOEMPTProvider Class.
//
//  

#pragma once

#include "xmlhandler.hxx"
#include "printschema.hxx"

using printschema::photoprintingintent::EIntentValue;

//
// The Provider class to Implement IPrintOemPrintTicketProvider Interface
//

class IOEMPTProvider : public IPrintOemPrintTicketProvider
{   
public:
    
    //  
    // Public Methods go here
    //
    IOEMPTProvider();
    virtual ~IOEMPTProvider();
    
    //
    // Basic COM Methods
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, void **ppvObj);

    STDMETHODIMP_(ULONG) AddRef(THIS);

    // the _At_ tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    _At_(this, __drv_freesMem(object)) 
    STDMETHODIMP_(ULONG) Release(THIS);


    //
    // IPrintOemPrintTicketProvider methods
    //  

    STDMETHOD(GetSupportedVersions)(THIS_
        _In_                               HANDLE  hPrinter,
        _Outptr_result_buffer_maybenull_(*cVersions) INT     *ppVersions[],
        _Out_                              INT     *cVersions 
        );
            
    STDMETHOD(BindPrinter)(THIS_
        _In_                                 HANDLE       hPrinter,
                                             INT          version,
        _Out_                                POEMPTOPTS   pOptions,
        _Out_                                INT          *cNamespaces,
        _Outptr_result_buffer_maybenull_(*cNamespaces) BSTR         **ppNamespaces
        );

    STDMETHOD(PublishPrintTicketHelperInterface)(THIS_
        _In_ IUnknown *pHelper
        );

    STDMETHOD(QueryDeviceDefaultNamespace)(THIS_
        _Out_ BSTR *pbstrNamespaceUri
        );

    STDMETHOD(ConvertPrintTicketToDevMode)(THIS_
        _In_    IXMLDOMDocument2 *pPrintTicket,
                ULONG             cbDevmode,
        _Inout_updates_bytes_(cbDevmode) 
                PDEVMODE          pDevmode,
                ULONG             cbDrvPrivateSize,
        _Inout_ PVOID             pPrivateDevmode
        );
            
    STDMETHOD(ConvertDevModeToPrintTicket)(THIS_
                ULONG             cbDevmode,
        _Inout_updates_bytes_(cbDevmode) 
                PDEVMODE          pDevmode,
                ULONG             cbDrvPrivateSize,
        _Inout_ PVOID             pPrivateDevmode,
        _Inout_ IXMLDOMDocument2 *pPrintTicket
        );
            
    STDMETHOD(CompletePrintCapabilities)(THIS_
        _In_opt_ IXMLDOMDocument2 *pPrintTicket,
        _Inout_  IXMLDOMDocument2 *pCapabilities
        );
           
    STDMETHOD(ExpandIntentOptions)(THIS_
        _Inout_ IXMLDOMDocument2 *pPrintTicket
        );

    STDMETHOD(ValidatePrintTicket)(THIS_
        _Inout_ IXMLDOMDocument2 *pPrintTicket
        );

private:

    //
    // Data members go here
    //
    LONG                 m_cRef; // COM reference count 
    HANDLE               m_hPrinterCached; // cached printer handle
    IPrintCoreHelperUni *m_pCoreHelper; // cached pointer to plug-in helper interface
    
    struct IntentConversionEntry
    {
        LPCWSTR pageMediaType;
        LPCWSTR pageOutputQuality;
    };
    
    static IntentConversionEntry intentConversionTable[printschema::photoprintingintent::EIntentValueMax];

    //
    // Private Methods
    //

    HRESULT ReadAndDeletePhotoPrintingIntent(
        OEMPTXMLHandler &opxh,
        EIntentValue &eIntentValue
        );

    HRESULT AddOrOverridePageMediaType(
        OEMPTXMLHandler &opxh,
        LPCWSTR lpcwstrPageMediaType
        );

    HRESULT AddOrOverridePageOutputQuality(
        OEMPTXMLHandler &opxh,
        LPCWSTR lpcwstrPageOutputQuality
        );

}; 

