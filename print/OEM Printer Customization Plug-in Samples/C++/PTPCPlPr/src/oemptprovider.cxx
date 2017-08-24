//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   oemptprovider.cxx
//    
//
//  PURPOSE:    Implementation of IOEMPTProvider Class.
//              The Provider class implements the
//              IPrintOemPrintTicketProvider interface functions that
//              provide the plug-in access to Print Ticket and allow the
//              plug-in to understand the Unidrv private setting in
//              the Print Ticket and expose its private settings to
//              Unidrv by putting them publicly in the Print Ticket. The
//              various provider methods make use of the
//              IPrintCoreHelper Interface methods to
//              create instances of XML objects and to communicate
//              information with Unidrv.
//
//  Functions:
//              IPrintOemPrintTicketProvider Interface Functions.
//              Private Functions of class IOEMPTProvider.
//

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

#include "precomp.hxx"
#include "globals.hxx"
#include "debug.hxx"
#include "oem.hxx"
#include "xmlhandler.hxx"
#include "oemptprovider.hxx"
#include "printschema.hxx"

/*++

Routine Name:

    IOEMPTProvider constructor

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
IOEMPTProvider::IOEMPTProvider()
    : m_pCoreHelper(NULL),
      m_cRef(1)
{   
}

/*++

Routine Name:

    IOEMPTProvider destructor

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/

IOEMPTProvider::~IOEMPTProvider()
{

    //
    // Make sure that helper interface is released.
    //
    if (m_pCoreHelper)
    {
        m_pCoreHelper->Release();
        m_pCoreHelper = NULL;
    }

    //
    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    //
    ASSERT(0 == m_cRef);    

}

/*++

Routine Name:

    IOEMPTProvider QueryInterface

Routine Description:

    Returns a pointer to a specified interface on an object to which a client currently holds an 
    interface pointer. 
    This function must call IUnknown::AddRef on the pointer it returns. 

Arguments:

    riid - Identifier of requested Interface.
    ppObj - Output variable that receives the pointer to requested Interface on success

Return Value:

    HRESULT
    S_OK - If the Interface requested is supported 
    E_NOINTERFACE otherwise

--*/

STDMETHODIMP IOEMPTProvider::QueryInterface(
    REFIID riid, 
    void **ppObj
    )
{
    if ((IID_IUnknown == riid) || (IID_IPrintOemPrintTicketProvider == riid))
    {
        *ppObj = static_cast<IPrintOemPrintTicketProvider *>(this);       
    }
    else
    {
        *ppObj = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppObj)->AddRef();
    return S_OK ;
}

/*++

Routine Name:

    AddRef

Routine Description:

    The AddRef method increments the reference count for an an interface
    on an object. It should be called for every new copy of a pointer to
    an interface on a given object.

Arguments:

    None

Return Value:

    Returns the value of the new reference count.

--*/

STDMETHODIMP_(ULONG) IOEMPTProvider::AddRef(
    VOID
    )
{
    return InterlockedIncrement(&m_cRef);
}

/*++

Routine Name:

    Release

Routine Description:

    The Release method decrements the reference count for the calling
    interface on a object. If the reference count on the object falls
    to 0, the object is freed from memory.

Arguments:

    None

Return Value:

    Returns the resulting value of the reference count.

--*/
_At_(this, __drv_freesMem(object)) 
STDMETHODIMP_(ULONG) IOEMPTProvider::Release(
    THIS
    )
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

/*++

Routine Name:

    GetSupportedVersions

Routine Description:
    
    The routine returns major versions of Printschema schema supported by the plug-in Provider.


Arguments:
    hPrinter - Printer Handle
    ppVersions - OUT pointer to array of version numbers to be filled in by the plug-in
    cVersions - OUT pointer to count of Number of versions supported by plug-in

Return Value:
    HRESULT
    S_OK - On Success
    E_* - On failure

--*/
STDMETHODIMP IOEMPTProvider::GetSupportedVersions
(
    _In_                               HANDLE ,
    _Outptr_result_buffer_maybenull_(*cVersions) INT     *ppVersions[],
    _Out_                              INT     *cVersions 
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::GetSupportedVersions: entry.\r\n"));

    HRESULT hr = S_OK;

    //
    // check if input parameters are valid
    //
    if (!ppVersions || !cVersions)
    {
        return E_INVALIDARG;
    }
    
    //
    // The Plug-in Provider need to allocate memory for the input version array and
    // then fill it with version information
    //
    *ppVersions = (INT *)CoTaskMemAlloc(sizeof(INT)); 

    if (!*ppVersions)
    {
        *cVersions = 0;
        return E_OUTOFMEMORY;
    }
    else
    {
        //
        // version number 1 is the only version supported currently
        //

        *cVersions = 1;
        (*ppVersions)[0] = PRINTSCHEMA_VERSION_NUMBER;          
    }

    return hr;
} 

/*++

Routine Name:
    
    BindPrinter

Routine Description:
    
    Bind Printer is the part of the Unidrv's activity to bind to a device. It allows the plug-in to cache
    certain information that can be used later on.

Arguments:
    
    hPrinter - Printer Handle supplied by Unidrv
    version - version of Printschema
    pOptions - Flags passed out to set configurable options supported by caller
    cNamespaces - Count of private namespaces of plug-in 
    ppNamespaces - OUT pointer to the array of Namespace URIs filled in by plug-in

Return Value:
    
    HRESULT
    S_OK - On Success
    E_VERSION_NOT_SUPPORTED - if printer version specified is not supported by plug-in
    E_* - On any other failure

--*/

STDMETHODIMP IOEMPTProvider::BindPrinter
(
    _In_                                 HANDLE       hPrinter,
                                         INT          version,
    _Out_                                POEMPTOPTS   pOptions,
    _Out_                                INT          *cNamespaces,
    _Outptr_result_buffer_maybenull_(*cNamespaces) BSTR         **ppNamespaces
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::BindPrinter: entry.\r\n"));

    HRESULT hr = S_OK;
    
    //
    // check if input parameters are NULL
    // Printer Handle should be provided by Unidrv in this call, which is cached by plug-in provider and
    // is later on used while making calls to Plug-in Helper Interface methods.
    //
    if (!pOptions  || !cNamespaces || !ppNamespaces || !hPrinter)
    {
        return E_INVALIDARG; 
    }


    *ppNamespaces = NULL;
    
    *cNamespaces = 0;

    //
    // Agree on the Printschema version with Unidrv
    // Version 1 is the only version currently supported
    //
    if (PRINTSCHEMA_VERSION_NUMBER != version)
    {
        return E_VERSION_NOT_SUPPORTED;        
    }

    //
    // Bind to the Physical Printer device
    // cache the printer handle for further use 
    //
    m_hPrinterCached = hPrinter;

    //
    // Flags to set configurable options, 
    // OEMPT_DEFAULT defined in prcomoem.h
    //
    *pOptions = OEMPT_DEFAULT;

    return hr;
    
}

/*++

Routine Name:
    
    PublishPrintTicketHelperInterface

Routine Description:

    For a number of operations, the plug-in needs to use the Helper Interface utilities provided by 
    Unidrv. Unidrv uses this method to publish the Print Ticket Helper Interface, IPrintCoreHelper.
    Plug-in should return SUCCESS after successfully incrementing the reference count of the interface.

Arguments:

    pHelper - IPrintCoreHelper interface pointer

Return Value:

    HRESULT
    S_OK - On Success
    E_* - On failure
    
--*/
STDMETHODIMP IOEMPTProvider::PublishPrintTicketHelperInterface
(
    _In_ IUnknown *pHelper
    )
{   
    VERBOSE(DLLTEXT("IOEMPTProvider::PublishPrintTicketHelperInterface: entry.\r\n"));

    if (!pHelper)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // Need to store pointer to Driver Helper functions, if we already haven't.
    //
    if (NULL == m_pCoreHelper)
    {
        hr = pHelper->QueryInterface(IID_IPrintCoreHelperUni, (VOID**)&m_pCoreHelper);
    }

    //
    // It's possible that this routine will publish other interfaces in the
    // future.  If the object published did not support the desired interface,
    // this routine should still succeed.
    //
    // If the helper interface is needed, but for some reason was not
    // published (this would be an error on the OS's part), you should
    // detect this and fail during the call-back where you intended to
    // use the helper interface.
    //

    if (E_NOINTERFACE == hr)
    {
        hr = S_OK;
    }

    return hr;
}

/*++

Routine Name:
    
    QueryDeviceDefaultNamespace

Routine Description:
    
    Plug-in should specify the name of Private namespace URI that Unidrv
    should be using to handle any features defined in the GPD that Unidrv
    does not recognize plug-in may specify a set of namespaces as a result
    of the call to BindPrinter method, and Unidrv needs to know which of
    them is to be used as default namespace so that, for all the features
    that Unidrv does not recognize, it will put them under this namespace
    in Print Ticket.

    This method is optional.  A plug-in can return E_NOTIMPL from this
    method if there are multiple plug-ins and another plug-in is providing
    the default, or if the plug-in does not want to change the default
    namespace that unidrv provides.

    Note: It is Unidrv's responsibility to add the private namespace URI
    that plug-in has specified through this call in the root node of the
    DOM document, and also define a prefix for it so that plug-in should
    use the prefix defined by Unidrv when it wishes to add any new node to
    the Print Ticket under its private namespace. Plug-in should not define
    its own prefix for this default private namespace URI.

Arguments:

    pbstrNamespaceUri - OUT Pointer to namespace URI to be filled in and returned by plug-in

Return Value:

    HRESULT
    S_OK - On success
    E_* - On failure

--*/

STDMETHODIMP IOEMPTProvider::QueryDeviceDefaultNamespace
(
    _Out_ BSTR *
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::QueryDeviceDefaultNamespace: entry.\r\n"));

    return E_NOTIMPL;
}

/*++

Routine Name:
    
    ConvertPrintTicketToDevMode

Routine Description:

    Unidrv will call this routine before it performs its part of PT->DM
    conversion. The plug-in is passed with an input Print Ticket that is
    fully populated, and Devmode which has default settings in it.  For
    this conversion, the plug-in needs to undo whatever changes it made to
    the Print Ticket during Devmode->PT conversion.

Arguments:

    pPrintTicket - pointer to input Print Ticket
    cbDevmode - size in bytes of input full devmode
    pDevmode - pointer to input full devmode buffer
    cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
    pPrivateDevmode - pointer to plug-in private devmode buffer
        

Return Value:

    HRESULT
    S_OK - On Success
    E_* - On failure

--*/

STDMETHODIMP IOEMPTProvider::ConvertPrintTicketToDevMode
(
    _In_    IXMLDOMDocument2 *pPrintTicket,
            ULONG             cbDevmode,
    _Inout_updates_bytes_(cbDevmode) 
            PDEVMODE          pDevmode,
            ULONG             , // cbDrvPrivateSize
    _Inout_ PVOID               // pPrivateDevmode
    )
{   
    VERBOSE(DLLTEXT("IOEMPTProvider::ConvertPrintTicketToDevMode: entry.\r\n"));

    //
    // cbDrvPrivateSize will be 0 because this plug-in doesn't 
    // support a private DEVMODE.
    //
    if (!cbDevmode || !pDevmode || !pPrintTicket)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

/*++

Routine Name:

    ConvertDevModeToPrintTicket

Routine Description: 

    Unidrv will call the routine with an Input Print
    Ticket that is populated with public and Unidrv private features. For
    those features in the GPD that Unidrv does not understand, it puts
    them in the PT under the private namespace (either specified by the
    plug-in through QueryDeviceDefaultNamespace or created by itself). It
    is the plug-in's responsibility to read the corresponding features
    from the input PT and put them in public printschema namespace, so
    that any higher level application making use of a print ticket can
    read and interpret these settings.


Arguments:

    cbDevmode - size in bytes of input full devmode
    pDevmode - pointer to input full devmode buffer
    cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
    pPrivateDevmode - pointer to plug-in private devmode buffer
    pPrintTicket - pointer to input Print Ticket

Return Value:

    HRESULT
    S_OK - On Success
    E_* - On failure

--*/
STDMETHODIMP IOEMPTProvider::ConvertDevModeToPrintTicket
(
            ULONG             cbDevmode,
    _Inout_updates_bytes_(cbDevmode)
            PDEVMODE          pDevmode,
            ULONG             , // cbDrvPrivateSize
    _Inout_ PVOID             , // pPrivateDevmode,
    _Inout_ IXMLDOMDocument2 *pPrintTicket
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::ConvertDevModeToPrintTicket: entry.\r\n"));

    //
    // cbDrvPrivateSize will be 0 because this plug-in doesn't 
    // support a private DEVMODE.
    //
    if (!cbDevmode || !pDevmode || !pPrintTicket)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    OEMPTXMLHandler opxh;
    EIntentValue eIntentValue = printschema::photoprintingintent::None;

    //
    // Initialize PrintTicketHandler 
    //  

    hr = opxh.SetRoot(pPrintTicket, m_pCoreHelper);  
    
    if (SUCCEEDED(hr))
    {
        hr = ReadAndDeletePhotoPrintingIntent(opxh, eIntentValue);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    return hr;
}


/*++

Routine Name:
    
    CompletePrintCapabilities

Routine Description:

    Unidrv calls this routine with an input Device Capabilities Document
    that is partially populated with Device capabilities information
    filled in by Unidrv for features that it understands. The plug-in
    needs to read any private features in the input Print Ticket, delete
    them and add them back under Printschema namespace so that higher
    level applications can understand them and make use of them.

Arguments:

    pPrintTicket - pointer to input Print Ticket
    pCapabilities - pointer to Device Capabilities Document.

Return Value:

    HRESULT
    S_OK - On Success
    E_* - On failure

--*/
STDMETHODIMP IOEMPTProvider::CompletePrintCapabilities
(
    _In_opt_ IXMLDOMDocument2 *,
    _Inout_  IXMLDOMDocument2 *pCapabilities
    )
{    
    VERBOSE(DLLTEXT("IOEMPTProvider::CompletePrintCapabilities: entry.\r\n"));

    if (!pCapabilities)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

/*++

Routine Name:

    ExpandIntentOptions
        
Routine Description: 

    As part of its Merge and Validate Procedure, the
    Unidrv/Postscript driver will call this routine to give the plug-in a
    chance to expand options which represent intent into their individual
    settings in other features in the print ticket.  This has two
    important effects: the client sees the results of the intent
    expansion, and unidrv resolves constraints against the individual
    features which are affected by the intent.

    If the plug-in does not support any intent features, then the plug-in
    should simply return S_OK.

Arguments:

    pPrintTicket - Pointer to input Print Ticket.

Return Value:

    HRESULT
    S_OK - On Success
    E_* - On failure

--*/

STDMETHODIMP IOEMPTProvider::ExpandIntentOptions(
    _Inout_ IXMLDOMDocument2 *pPrintTicket
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::ExpandIntentOptions: entry.\r\n"));

    if (!pPrintTicket)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    OEMPTXMLHandler opxh;

    //
    // Initialize PrintTicketHandler 
    //  

    hr = opxh.SetRoot(pPrintTicket, m_pCoreHelper);  
    
    //
    // Read in the photo printing intent value, if it exists. If it
    // doesn't, the default value of None is assumed instead.
    //

    EIntentValue eIntentValue = printschema::photoprintingintent::None;

    if (SUCCEEDED(hr))
    {
        hr = ReadAndDeletePhotoPrintingIntent(opxh, eIntentValue);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    if (printschema::photoprintingintent::None != eIntentValue)
    {
        IOEMPTProvider::IntentConversionEntry entry = IOEMPTProvider::intentConversionTable[eIntentValue];
        
        //
        // Set appropriate values for PageMediaType and
        // PageOutputQuality. Override any existing values of the
        // options (by deleting before adding).
        //
        
        if (SUCCEEDED(hr))
        {
            hr = AddOrOverridePageMediaType(opxh, entry.pageMediaType);
        }
        
        if (SUCCEEDED(hr))
        {
            hr = AddOrOverridePageOutputQuality(opxh, entry.pageOutputQuality);
        }
    }

    return hr;
}

/*++

Routine Name:

    ValidatePrintTicket
        
Routine Description:

    The plug-in might need to delete any feature under private namespace
    from input PT that are also in the public namespace because of Merge
    and Validate.

    The Validate method should also perform any conflict resolution if
    necessary looking at the settings made in public and unidrv private
    part of Print Ticket, to make sure that the resultant Print Ticket is a
    valid one, and has all constraints resolved.

Arguments:
    
    pBaseTicket - Pointer to input Print Ticket.

Return Value:

    HRESULT
    S_NO_CONFLICT/S_CONFLICT_RESOLVED - On Success
    E_* - On failure

--*/

STDMETHODIMP IOEMPTProvider::ValidatePrintTicket(
    _Inout_ IXMLDOMDocument2 *pPrintTicket    
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::ValidatePrintTicket: entry.\r\n"));

    if (!pPrintTicket)
    {
        return E_INVALIDARG;
    }

    //
    // If the plug-in performs validation successfully, it needs to
    // send either S_NO_CONFLICT or S_CONFLICT_RESOLVED as the result
    // code, sending hr = S_OK will not work since Unidrv expects one
    // of the above two success codes to interpret successful
    // completion of plug-in validate method.
    //

    return S_NO_CONFLICT;
}

//
// Private stuff
//

IOEMPTProvider::IntentConversionEntry IOEMPTProvider::intentConversionTable[] = 
{
    { NULL, NULL, },
    { printschema::pagemediatype::PLAIN, printschema::pageoutputquality::DRAFT, },
    { printschema::pagemediatype::PHOTOGRAPHIC, printschema::pageoutputquality::HIGH, },
    { printschema::pagemediatype::PHOTOGRAPHIC, printschema::pageoutputquality::PHOTOGRAPHIC, },
};

/*++

Routine Name:

    ReadAndDeletePhotoPrintingIntent
        
Routine Description:

    This function gets the value of the Photo Printing Intent from a Print
    Ticket and returns it in eIntentValue. It also deletes the entire
    feature from the Ticket.

Arguments:

    opxh - reference to input XMLHandler
    eIntentValue - reference to output intent value

Return Value:

    HRESULT
    S_OK - On success, if intent was found
    S_FALSE - On Success, if intent was not found
    E_* - On failure

--*/

HRESULT IOEMPTProvider::ReadAndDeletePhotoPrintingIntent(
    OEMPTXMLHandler &opxh,
    EIntentValue &eIntentValue
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::ReadAndDeletePhotoPrintingIntent: entry.\r\n"));

    HRESULT hr = S_OK;

    IXMLDOMElement *pPhotoPrintingIntentFeature = NULL;
    IXMLDOMElement *pPhotoPrintingIntentOption = NULL;

    hr = opxh.GetFeatureNode(NULL, // meaning root is parent
        printschema::KEYWORDS_URI,
        printschema::photoprintingintent::FEATURE,
        &pPhotoPrintingIntentFeature);

    //  
    // Check for S_OK == hr and not SUCCEEDED(hr) because
    // GetFeatureNode returns S_FALSE if feature is not found
    //

    if (S_OK == hr && pPhotoPrintingIntentFeature)
    {
        hr = opxh.GetOptionNode(pPhotoPrintingIntentFeature,
            &pPhotoPrintingIntentOption);
    }

    BSTR bstrIntentNamespace = NULL;
    BSTR bstrIntentValue = NULL;

    if (S_OK == hr && pPhotoPrintingIntentOption)
    {
        hr = opxh.GetXMLAttribute(pPhotoPrintingIntentOption, &bstrIntentNamespace, &bstrIntentValue);
    }

    if (S_OK == hr && NULL != bstrIntentNamespace)
    {
        if (0 != wcscmp(bstrIntentNamespace, printschema::KEYWORDS_URI))
        {
            //
            // We do not delete the node if it specifies a different
            // namespace
            //
            hr = S_FALSE;
        }
    }

    //
    // Search for intent, looping across all possible (valid) values, and store it in eIntentValue
    //

    if (S_OK == hr && NULL != bstrIntentValue)
    {
        EIntentValue e = EIntentValue(0);

        for (; e < printschema::photoprintingintent::EIntentValueMax; ++e)
        {
             if (0 == wcscmp(bstrIntentValue, printschema::photoprintingintent::OPTIONS[e]))
             {
                 eIntentValue = e;
                 break;
             }
        }
        
        if (printschema::photoprintingintent::EIntentValueMax == e)
        {
            //
            // We do not delete the node if the attribute name
            // in the Print Ticket is not one of the valid ones
            //
            hr = S_FALSE;
        }
    }

    //
    // If found, delete intent
    //

    if (S_OK == hr)
    {
        hr = opxh.DeleteFeatureNode(NULL, pPhotoPrintingIntentFeature);
    }

    //
    // Make sure all BSTRs are freed, ...
    //

    if (bstrIntentNamespace)
    {
        SysFreeString(bstrIntentNamespace);
        bstrIntentNamespace = NULL;
    }
    
    if (bstrIntentValue)
    {
        SysFreeString(bstrIntentValue);
        bstrIntentValue = NULL;
    }

    //
    // ... and interface pointers released
    //

    if (pPhotoPrintingIntentFeature)
    {
        pPhotoPrintingIntentFeature->Release();
    }

    if (pPhotoPrintingIntentOption)
    {
        pPhotoPrintingIntentOption->Release();
    }

    return hr;
}

/*++

Routine Name:

    AddOrOverridePageMediaType
        
Routine Description:

    This routine takes in the value of the PageMediaType to be set,
    depending on the intent that was present. It then deletes any existing
    PageMediaType value in the ticket, and then adds the new PageMediaType
    value.

Arguments:

    opxh - reference to input XMLHandler
    lpcwstrPageMediaType - input PageMediaType string

Return Value:

    HRESULT
    S_OK - On success
    E_* - On failure

--*/

HRESULT IOEMPTProvider::AddOrOverridePageMediaType(
    OEMPTXMLHandler &opxh,
    LPCWSTR lpcwstrPageMediaType
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::AddOrOverridePageMediaType: entry.\r\n"));

    HRESULT hr = S_OK;

    if (NULL == lpcwstrPageMediaType)
    {
        return S_OK;
    }

    IXMLDOMElement *pPageMediaTypeOld = NULL;
    IXMLDOMElement *pPageMediaTypeNew = NULL;

    //
    // Search for, and delete any old PageMediaType option, since we're
    // going to override it
    //

    hr = opxh.GetFeatureNode(NULL,
        printschema::KEYWORDS_URI,
        printschema::pagemediatype::FEATURE,
        &pPageMediaTypeOld);

    if (S_OK == hr)
    {
        hr = opxh.DeleteFeatureNode(NULL, pPageMediaTypeOld);
    }

    //
    // GetFeatureNode might have returned S_FALSE if it couldn't find
    // such a node, so from now on, we check for SUCCEEDED
    //

    //
    // Create PageMediaType feature node
    //

    if (SUCCEEDED(hr))
    {
        hr = opxh.CreateFeatureNode(NULL,
            printschema::KEYWORDS_URI,
            printschema::pagemediatype::FEATURE,
            &pPageMediaTypeNew);
    }

    //
    // Create option node under the feature node above
    //

    if (SUCCEEDED(hr))
    {
        hr = opxh.CreateOptionNode(pPageMediaTypeNew,
            printschema::KEYWORDS_URI,
            lpcwstrPageMediaType,
            NULL);
    }

    if (pPageMediaTypeOld)
    {
        pPageMediaTypeOld->Release();
    }

    if (pPageMediaTypeNew)
    {
        pPageMediaTypeNew->Release();
    }

    return hr;
}

/*++

Routine Name:

    AddOrOverridePageOutputQuality
        
Routine Description:

    This routine, similar to AddOrOverridePageMediaType, takes in the
    value of the PageOutputQuality to be set, depending on the intent that
    was present. It then deletes any existing PageOutputQuality value in
    the ticket, and then adds the new PageOutputQuality value.

Arguments:

    opxh - reference to input XMLHandler
    lpcwstrPageOutputQuality - input PageOutputQuality string

Return Value:

    HRESULT
    S_OK - On success
    E_* - On failure

--*/

HRESULT IOEMPTProvider::AddOrOverridePageOutputQuality(
    OEMPTXMLHandler &opxh,
    LPCWSTR lpcwstrPageOutputQuality
    )
{
    VERBOSE(DLLTEXT("IOEMPTProvider::AddOrOverridePageOutputQuality: entry.\r\n"));

    HRESULT hr = S_OK;

    if (NULL == lpcwstrPageOutputQuality)
    {
        return S_OK;
    }

    IXMLDOMElement *pPageOutputQualityOld = NULL;
    IXMLDOMElement *pPageOutputQualityNew = NULL;

    //
    // Search for, and delete any old PageOutputQuality option, since we're
    // going to override it
    //

    hr = opxh.GetFeatureNode(NULL,
        printschema::KEYWORDS_URI,
        printschema::pageoutputquality::FEATURE,
        &pPageOutputQualityOld);

    if (S_OK == hr)
    {
        hr = opxh.DeleteFeatureNode(NULL, pPageOutputQualityOld);
    }

    //
    // GetFeatureNode might have returned S_FALSE if it couldn't find
    // such a node, so from now on, we check for SUCCEEDED
    //

    //
    // Create PageOutputQuality feature node
    //

    if (SUCCEEDED(hr))
    {
        hr = opxh.CreateFeatureNode(NULL,
            printschema::KEYWORDS_URI,
            printschema::pageoutputquality::FEATURE,
            &pPageOutputQualityNew);
    }

    //
    // Create option node under the feature node above
    //

    if (SUCCEEDED(hr))
    {
        hr = opxh.CreateOptionNode(pPageOutputQualityNew,
            printschema::KEYWORDS_URI,
            lpcwstrPageOutputQuality,
            NULL);
    }

    if (pPageOutputQualityOld)
    {
        pPageOutputQualityOld->Release();
    }

    if (pPageOutputQualityNew)
    {
        pPageOutputQualityNew->Release();
    }

    return hr;
}
