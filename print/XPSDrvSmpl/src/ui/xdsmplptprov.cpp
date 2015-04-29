/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplptprov.cpp

Abstract:

   Implementation of the PrintTicket provider plugin. This is responsible for
   handling PrintTicket features that are too complex for the GPD->PrintTicket
   automapping facility.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "xdsmplptprov.h"
#include "wmdmptcnv.h"
#include "pgscdmptcnv.h"
#include "bkdmptcnv.h"
#include "nupptcnv.h"
#include "coldmptcnv.h"
#include "pthndlr.h"
#include "pchndlr.h"

static LPCWSTR PRIVATE_URI = L"http://schemas.microsoft.com/windows/2003/08/printing/XPSDrv_Feature_Sample";


/*++

Routine Name:

    CXDSmplPTProvider::CXDSmplPTProvider

Routine Description:

    CXDSmplPTProvider class constructor

Arguments:

    None

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CXDSmplPTProvider::CXDSmplPTProvider() :
    CUnknown<IPrintOemPrintTicketProvider>(IID_IPrintOemPrintTicketProvider),
    m_hPrinterCached(NULL),
    m_pCoreHelper(NULL),
    m_bstrPrivateNS(PRIVATE_URI)
{
    HRESULT hr = S_OK;
    IFeatureDMPTConvert* pHandler = NULL;

//
// This Prefast warning indicates that memory could be leaked
// in the event of an exception. We suppress this false positive because we
// know that the local try/catch block will clean up a single
// IFeatureDMPTConvert, and the destructor will clean up any that are
// successfully added to the vector.
//
#pragma prefast(push)
#pragma prefast(disable:__WARNING_ALIASED_MEMORY_LEAK_EXCEPTION)
    try
    {
        //
        // Populate the feature DM<->PT conversion vector
        //
        pHandler = new(std::nothrow) CWatermarkDMPTConv();

        hr = AddConverter(pHandler);

        if (SUCCEEDED(hr))
        {
            pHandler = new(std::nothrow) CPageScalingDMPTConv();

            hr = AddConverter(pHandler);
        }

        if (SUCCEEDED(hr))
        {
            pHandler = new(std::nothrow) CBookletDMPTConv();

            hr = AddConverter(pHandler);
        }

        if (SUCCEEDED(hr))
        {
            pHandler = new(std::nothrow) CNUpDMPTConv();

            hr = AddConverter(pHandler);
        }

        if (SUCCEEDED(hr))
        {
            pHandler = new(std::nothrow) CColorProfileDMPTConv();

            hr = AddConverter(pHandler);
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());

        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        //
        // If we successfully created a handler but failed to push it onto
        // the handler vector we need to free the allocated handler
        //
        if (pHandler != NULL)
        {
            delete pHandler;
            pHandler = NULL;
        }

        //
        // Delete any DM<->PT converters that were successfully instantiated
        //
        DeleteConverters();

        throw CXDException(hr);
    }
#pragma prefast(pop)
}

/*++

Routine Name:

    CXDSmplPTProvider::~CXDSmplPTProvider

Routine Description:

    CXDSmplPTProvider class destructor

Arguments:

    None

Return Value:

    None

--*/
CXDSmplPTProvider::~CXDSmplPTProvider()
{
    //
    // Clean up all DM<->PT converters
    //
    DeleteConverters();
}

/*++

Routine Name:

    CXDSmplPTProvider::GetSupportedVersions

Routine Description:

    The routine returns major versions of Printschema schema supported by the plug-in Provider.

Arguments:

    hPrinter - Printer Handle
    ppVersions - OUT pointer to array of version numbers to be filled in by the plug-in
    cVersions - OUT pointer to count of Number of versions supported by plug-in

Return Value:

    HRESULT
    S_OK - On success
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::GetSupportedVersions(
    _In_                            HANDLE,
    _Outptr_result_buffer_(*pcVersions) INT* ppVersions[],
    _Out_                           INT* pcVersions
    )
{
    HRESULT hr = S_OK;

    //
    // Check if input parameters are valid
    //
    if (SUCCEEDED(hr = CHECK_POINTER(ppVersions, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcVersions, E_POINTER)))
    {
        *pcVersions = 0;

        //
        // The Plug-in Provider need to allocate memory for the input version array and
        // then fill it with version information
        //
        *ppVersions = static_cast<INT*>(CoTaskMemAlloc(sizeof(INT)));

        if (*ppVersions != NULL)
        {
            //
            // version number 1 is the only version supported currently
            //
            *pcVersions = 1;
            (*ppVersions)[0] = PRINTSCHEMA_VERSION_NUMBER;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::BindPrinter

Routine Description:

    Bind Printer is the part of the Unidrv's activity to bind to a device. It allows the plug-in to cache
    certain information that can be used later on such as the private namespaces used by the plug-in.

Arguments:

    hPrinter     - Printer Handle supplied by Unidrv
    version      - version of Printschema
    pOptions     - Flags passed out to set configurable options supported by caller
    cNamespaces  - Count of private namespaces of plug-in
    ppNamespaces - OUT pointer to the array of Namespace URIs filled in by plug-in

Return Value:

    HRESULT
    S_OK - On success
    E_VERSION_NOT_SUPPORTED - if printer version specified is not supported by plug-in
    E_* - On any other failure

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::BindPrinter(
    _In_                                  HANDLE     hPrinter,
                                          INT        version,
    _Out_                                 POEMPTOPTS pOptions,
    _Out_                                 INT*       pcNamespaces,
    _Outptr_result_buffer_maybenull_(*pcNamespaces) BSTR**     ppNamespaces
    )
{
    HRESULT hr = S_OK;

    //
    // Printer Handle should be provided by Unidrv in this call, which is cached by plug-in provider and
    // is later on used while making calls to Plug-in Helper Interface methods.
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pOptions, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcNamespaces, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppNamespaces, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(hPrinter, E_HANDLE)))
    {
        *ppNamespaces = NULL;
        *pcNamespaces = 0;

        //
        // Agree on the Printschema version with Unidrv Version 1 is the only
        // version currently supported
        //
        if (PRINTSCHEMA_VERSION_NUMBER == version)
        {
            //
            // Cache the printer handle for further use
            //
            m_hPrinterCached = hPrinter;

            //
            // Flags to set configurable options, OEMPT_DEFAULT defined in prcomoem.h
            //
            *pOptions = OEMPT_NOSNAPSHOT;

            //
            // Publish the private namespace
            //
            *pcNamespaces = 1;
            *ppNamespaces = static_cast<BSTR*>(CoTaskMemAlloc(sizeof(BSTR)));

            if (SUCCEEDED(hr = CHECK_POINTER(*ppNamespaces, E_OUTOFMEMORY)))
            {
                hr = m_bstrPrivateNS.CopyTo(*ppNamespaces);
            }
        }
        else
        {
            hr =  E_VERSION_NOT_SUPPORTED;
        }
    }

    ERR_ON_HR_EXC(hr, E_VERSION_NOT_SUPPORTED);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::PublishPrintTicketHelperInterface

Routine Description:

    For a number of operations, the plug-in needs to use the Helper Interface utilities provided by
    Unidrv. Unidrv uses this method to publish the PrintTicket Helper Interface, IPrintCoreHelper.
    Plug-in should return SUCCESS after successfully incrementing the reference count of the interface.

Arguments:

    pHelper - IPrintCoreHelper interface pointer

Return Value:

    HRESULT
    S_OK - On success
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::PublishPrintTicketHelperInterface(
    _In_ IUnknown *pHelper
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pHelper, E_POINTER)))
    {
        //
        // Need to store pointer to Driver Helper functions, if we already haven't.
        //
        if (m_pCoreHelper == NULL)
        {
            hr = pHelper->QueryInterface(IID_IPrintCoreHelperUni, reinterpret_cast<VOID**>(&m_pCoreHelper));
        }

        //
        // It's possible that this routine will publish other interfaces in the future.
        // If the object published did not support the desired interface, this routine
        // should still succeed.
        //
        // If the helper interface is needed, but for some reason was not published
        // (this would be an error on the OS's part), you should detect this and fail
        // during the call-back where you intended to use the helper interface.
        //
        if (E_NOINTERFACE == hr)
        {
            hr = S_OK;
        }
        else
        {
            try
            {
               //
               // Iterate over all PrintTicket feature converters publishing the helper interface
               //
               DMPTConvCollection::iterator iterConverters =  m_vectFtrDMPTConverters.begin();

               for (; iterConverters != m_vectFtrDMPTConverters.end() && SUCCEEDED(hr); iterConverters++)
               {
                   hr = (*iterConverters)->PublishPrintTicketHelperInterface(m_pCoreHelper);
               }
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::QueryDeviceDefaultNamespace

Routine Description:

    This method provides the plug-in with the opportunity to specify the name of the
    Private namespace URI that Unidrv should be using to handle any features defined
    in the GPD that Unidrv does not recognize. The plug-in may specify a set of
    namespaces as a result of the call to BindPrinter method, and Unidrv needs to know
    which of them is to be used as default namespace so that, for all the features that
    Unidrv does not recognize, it will put them under this namespace in the PrintTicket.

    Note: It is Unidrv's responsibility to add the private namespace URI that plug-in
    has specified through this call in the root node of the DOM document, and also define
    a prefix for it so that plug-in should use the prefix defined by Unidrv when it wishes
    to add any new node to the PrintTicket under its private namespace. Plug-in should
    not define its own prefix for this default private namespace URI.

Arguments:

    pbstrNamespaceUri - OUT Pointer to namespace URI to be filled in and returned by plug-in

Return Value:

    HRESULT
    S_OK - On success
    E_NOTIMPL - The plugin does not require a private namespace
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::QueryDeviceDefaultNamespace(
    _Out_ BSTR* pbstrNamespaceUri
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrNamespaceUri, E_POINTER)))
    {
        hr = m_bstrPrivateNS.CopyTo(pbstrNamespaceUri);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::ConvertPrintTicketToDevMode

Routine Description:

    Unidrv will call this routine before it performs its part of PT->DM
    conversion. The plug-in is passed with an input PrintTicket that is
    fully populated, and Devmode which has default settings in it.

    This routine merely passes the call on to the individual feature handlers.

Arguments:

    pPrintTicket - pointer to input PrintTicket
    cbDevmode - size in bytes of input full devmode
    pDevmode - pointer to input full devmode buffer
    cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
    pPrivateDevmode - pointer to plug-in private devmode buffer


Return Value:

    HRESULT
    S_OK - On success
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::ConvertPrintTicketToDevMode(
    _In_    IXMLDOMDocument2* pPrintTicket,
            ULONG             cbDevmode,
    _Inout_updates_bytes_(cbDevmode) 
            PDEVMODE          pDevmode,
            ULONG             cbDrvPrivateSize,
    _Inout_ PVOID             pPrivateDevmode
    )
{
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    try
    {
        if (SUCCEEDED(hr))
        {
            //
            // Iterate over all PrintTicket feature converters letting them
            // provide the conversion
            //
            DMPTConvCollection::iterator iterConverters =  m_vectFtrDMPTConverters.begin();

            for (; iterConverters != m_vectFtrDMPTConverters.end() && SUCCEEDED(hr); iterConverters++)
            {
                hr = (*iterConverters)->ConvertPrintTicketToDevMode(pPrintTicket, cbDevmode, pDevmode, cbDrvPrivateSize, pPrivateDevmode);
            }
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::ConvertDevModeToPrintTicket

Routine Description:

    Unidrv will call the routine with an Input PrintTicket that is populated
    with public and Unidrv private features. For those features in the GPD that
    Unidrv does not understand, it puts them in the PT under the private namespace
    (either specified by the plug-in through QueryDeviceDefaultNamespace or created
    by itself). It is the plug-in's responsibility to read the corresponding features
    from the input PT and put them in public printschema namespace, so that any higher
    level application making use of a PrintTicket can read and interpret these settings.

    This routine merely passes the call on to the individual feature handlers.


Arguments:

    cbDevmode - size in bytes of input full devmode
    pDevmode - pointer to input full devmode buffer
    cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
    pPrivateDevmode - pointer to plug-in private devmode buffer
    pPrintTicket - pointer to input PrintTicket

Return Value:

    HRESULT
    S_OK - On success
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::ConvertDevModeToPrintTicket(
            ULONG             cbDevmode,
    _Inout_updates_bytes_(cbDevmode) 
            PDEVMODE          pDevmode,
            ULONG             cbDrvPrivateSize,
    _Inout_ PVOID             pPrivateDevmode,
    _Inout_ IXMLDOMDocument2* pPrintTicket
    )
{
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
    {
        if (cbDevmode < sizeof(DEVMODE) ||
            cbDrvPrivateSize == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    try
    {
        if (SUCCEEDED(hr))
        {
            //
            // Iterate over all PrintTicket feature converters letting them
            // provide the conversion
            //
            DMPTConvCollection::iterator iterConverters =  m_vectFtrDMPTConverters.begin();

            for (; iterConverters != m_vectFtrDMPTConverters.end() && SUCCEEDED(hr); iterConverters++)
            {
                hr = (*iterConverters)->ConvertDevModeToPrintTicket(cbDevmode, pDevmode, cbDrvPrivateSize, pPrivateDevmode, pPrintTicket);
            }

            //
            // Delete all the private features generated by the Unidrv parser from the GPD.
            // These are not required in the PrintTicket as they are only used to control
            // features in the public PrintSchema and have no meaning outside the config
            // module.
            //
            if (SUCCEEDED(hr))
            {
                CPTHandler ptHandler(pPrintTicket);
                hr = ptHandler.DeletePrivateFeatures(m_bstrPrivateNS);
            }
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::CompletePrintCapabilities

Routine Description:

    Unidrv calls this routine with an input Device Capabilities Document
    that is partially populated with Device capabilities information
    filled in by Unidrv for features that it understands. The plug-in
    needs to read any private features in the input PrintTicket, delete
    them and add them back under Printschema namespace so that higher
    level applications can understand them and make use of them.

    The XPSDrv sample driver does not define any private device capabilities
    so this method merely returns S_OK

Arguments:

    pPrintTicket - pointer to input PrintTicket
    pCapabilities - pointer to Device Capabilities Document.

Return Value:

    HRESULT
    S_OK - Always

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::CompletePrintCapabilities(
    _In_opt_ IXMLDOMDocument2* pPrintTicket,
    _Inout_  IXMLDOMDocument2* pPrintCapabilities
    )
{
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (SUCCEEDED(hr = CHECK_POINTER(pPrintCapabilities, E_POINTER)))
    {
        try
        {
            //
            // Iterate over all PrintTicket feature converters letting them
            // provide the conversion
            //
            DMPTConvCollection::iterator iterConverters =  m_vectFtrDMPTConverters.begin();

            for (; iterConverters != m_vectFtrDMPTConverters.end() && SUCCEEDED(hr); iterConverters++)
            {
                hr = (*iterConverters)->CompletePrintCapabilities(pPrintTicket, pPrintCapabilities);
            }

            //
            // Delete all the private features generated by the Unidrv parser from the GPD.
            // These are not required in the PrintCapabilities as they are only used to control
            // features in the public PrintSchema and have no meaning outside the config
            // module.
            //
            if (SUCCEEDED(hr))
            {
                CPCHandler pcHandler(pPrintCapabilities);
                hr = pcHandler.DeletePrivateFeatures(m_bstrPrivateNS);
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplPTProvider::ExpandIntentOptions

Routine Description:

    As part of its Merge and Validate Procedure, the Unidrv/Postscript driver
    will call this routine to give the plug-in a chance to expand options
    which represent intent into their individual settings in other features
    in the PrintTicket.  This has two important effects: the client sees the
    results of the intent expansion, and unidrv resolves constraints against
    the individual features which are affected by the intent.

    The XPSDrv sample driver plug-in does not support any intent features, therefore
    simply returns S_OK.

Arguments:

    pPrintTicket - Pointer to input PrintTicket.

Return Value:

    HRESULT
    S_OK - Always

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::ExpandIntentOptions(
    _Inout_ IXMLDOMDocument2 *
    )
{
    return S_OK;
}

/*++

Routine Name:

    CXDSmplPTProvider::ValidatePrintTicket

Routine Description:

    The plug-in might need to delete any feature under private namespace
    from input PT that are also in the public namespace because of Merge
    and Validate.

    The Validate method should also perform any conflict resolution if
    necessary looking at the settings made in public and unidrv private
    part of PrintTicket, to make sure that the resultant PrintTicket is a
    valid one, and has all constraints resolved.

    This routine merely passes the validate call on to the feature handlers

Arguments:

    pPrintTicket - Pointer to input PrintTicket.

Return Value:

    HRESULT
    S_NO_CONFLICT/S_CONFLICT_RESOLVED - On success
    E_* - On error

--*/
HRESULT STDMETHODCALLTYPE
CXDSmplPTProvider::ValidatePrintTicket(
    _Inout_ IXMLDOMDocument2* pPrintTicket
    )
{
    HRESULT hr = S_NO_CONFLICT;

    //
    // Validate parameters
    //
    if (pPrintTicket != NULL)
    {
        try
        {
            //
            // Iterate over all PrintTicket feature converters letting them
            // provide the validation
            //
            DMPTConvCollection::iterator iterConverters =  m_vectFtrDMPTConverters.begin();

            for (; iterConverters != m_vectFtrDMPTConverters.end() && SUCCEEDED(hr); iterConverters++)
            {
                HRESULT hrConverter = (*iterConverters)->ValidatePrintTicket(pPrintTicket);

                if (FAILED(hrConverter) ||
                    hrConverter == static_cast<HRESULT>(S_CONFLICT_RESOLVED))
                {
                    hr = hrConverter;
                }
            }
        }
        catch (...)
        {
            hr = E_FAIL;
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

    CXDSmplPTProvider::DeleteConverters

Routine Description:

    This routine cleans up the vector of DM <-> PT converters.

Arguments:

    NONE

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplPTProvider::DeleteConverters(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        while (!m_vectFtrDMPTConverters.empty())
        {
            if (m_vectFtrDMPTConverters.back() != NULL)
            {
                delete m_vectFtrDMPTConverters.back();
                m_vectFtrDMPTConverters.back() = NULL;
            }

            m_vectFtrDMPTConverters.pop_back();
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

//
// Use __drv_aliasesMem annotation to avoid PREfast warning 28197: Possibly leaking memory,
//
HRESULT
// Prefast warning 28194: The function was declared as aliasing the value in variable and exited without doing so.
// We suppress this false positive because STL vector does not have annotation, and we know the pointer has been saved
// to STL vector, which is released in DeleteConverters().
#pragma warning(suppress: 28194)
CXDSmplPTProvider::AddConverter(
    _In_opt_ __drv_aliasesMem IFeatureDMPTConvert *pHandler
    )
{
    HRESULT hr = CHECK_POINTER(pHandler, E_OUTOFMEMORY);

    if (SUCCEEDED(hr))
    {
        m_vectFtrDMPTConverters.push_back(pHandler);
    }

    return hr;
}
