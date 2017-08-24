//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2006  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:       gdlsmpl.cpp
//
//  PURPOSE:    Implementation of interface for Unidrv5 UI plug-ins.
//
//--------------------------------------------------------------------------

#include "precomp.h"

#include "debug.h"
#include "devmode.h"
#include "intrface.h"
#include "gdlsmpl.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

//--------------------------------------------------------------------------
//      Globals
//--------------------------------------------------------------------------

HINSTANCE   ghInstance = NULL;
    // Module's Instance handle from DLLEntry of process.


//--------------------------------------------------------------------------
//      Internal Globals
//--------------------------------------------------------------------------

static long g_cComponents = 0;
    // Count of active components

static long g_cServerLocks = 0;
    // Count of locks


//--------------------------------------------------------------------------
//      Local helper functions
//--------------------------------------------------------------------------
BOOL __stdcall
DebugPrint(
    LPCWSTR     pszMessage,
        // Format string for the error message
    ...
        // args specified in the format string.
    )
{
    va_list arglist;
        // varargs list for processing the '...' parameter.

    WCHAR szMsgBuf[MAX_PATH] = {0};
        // Use a stack error buffer so that we don't need to
        // allocate in the failure path.

    HRESULT     hResult;
        // Result from formatting the string.

    if (NULL == pszMessage)
    {
        return FALSE;
    }

    // Pass the variable parameters to wvsprintf to be formated.
    va_start(arglist, pszMessage);
    hResult = StringCbVPrintfW(szMsgBuf, MAX_PATH*sizeof(szMsgBuf[0]), pszMessage, arglist);
    va_end(arglist);

    // Dump string to debug output.
    OutputDebugStringW(szMsgBuf);

    return SUCCEEDED(hResult);
}


//--------------------------------------------------------------------------
//      Class factory for the Plug-In object
//--------------------------------------------------------------------------
class COemCF : public IClassFactory
{
public:

    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_
        REFIID riid,
        LPVOID FAR* ppvObj
        );

    STDMETHOD_(ULONG,AddRef) (THIS);

    _At_(this, __drv_freesMem(object))
        STDMETHOD_(ULONG,Release) (THIS);

    // IClassFactory methods
    STDMETHOD(CreateInstance) (THIS_
        _In_opt_    LPUNKNOWN pUnkOuter,
        _In_        REFIID riid,
        _Outptr_ LPVOID FAR* ppvObject
        );

    STDMETHOD(LockServer) (THIS_
        BOOL bLock
        );

    // Class-specific methods.
    COemCF(): m_cRef(1) { };

    ~COemCF() { };

protected:
    LONG m_cRef;

};


//+---------------------------------------------------------------------------
//
//  Member:
//      ::DllMain
//
//  Synopsis:
//      Dll's main routine, called when new
//      threads or processes reference the module
//
//  Returns:
//      TRUE.  Currently has no failure paths.  Return false if the module
//      cannot be properly initialized.
//
//  Notes:
//      the caller of this routine holds a process-wide lock preventing more
//      than one module from being initialized at a time.  In general,
//      plug-ins should do as little as possible in this routine to avoid
//      stalling other threads that may need to load modules.  Also, calling
//      any function that would, in-turn, cause another module to be loaded
//      could result in a deadlock.
//
//
//----------------------------------------------------------------------------
extern "C" BOOL
WINAPI DllMain(
    HINSTANCE hInst,
    WORD wReason,
    LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);

    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      ::DllCanUnloadNow
//
//  Synopsis:
//      Can the DLL be unloaded from memory?  Answers no if any objects are
//      still allocated, or if the server has been locked.
//
//      To avoid leaving OEM DLL still in memory when Unidrv or Pscript
//      drivers are unloaded, Unidrv and Pscript driver ignore the return
//      value of DllCanUnloadNow of the OEM DLL, and always call FreeLibrary
//      on the OEMDLL.
//
//      If the plug-in spins off a working thread that also uses this DLL,
//      the thread needs to call LoadLibrary and FreeLibraryAndExitThread,
//      otherwise it may crash after Unidrv or Pscript calls FreeLibrary.
//
//  Returns:
//      S_OK if the DLL can be unloaded safely, otherwise S_FALSE
//
//
//----------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::DllGetClassObject
//
//  Synopsis:
//      Retrieve the class factory object for the indicated class.
//
//  Returns:
//      CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, or S_OK.
//
//  Notes:
//
//
//----------------------------------------------------------------------------
STDAPI
DllGetClassObject(
    _In_ CONST CLSID& clsid,
        // GUID of the class object that the caller wants a class factory for
    _In_ CONST IID& iid,
        // Interface ID to provide in ppv
    _Outptr_ VOID** ppv
        // out pointer to the interface requested.
    )
{
    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    //
    // Can we create this component?
    //
    if (clsid != CLSID_OEMUI)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    //
    // Create class factory.
    // Reference count set to 1 in Constructor
    //
    COemCF* pGDLSampleClassFactory = new COemCF;
    if (pGDLSampleClassFactory == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Get requested interface.
    //
    HRESULT hr = pGDLSampleClassFactory->QueryInterface(iid, ppv);
    pGDLSampleClassFactory->Release();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemCF::QueryInterface
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemCF::QueryInterface(
    CONST IID& iid,
    VOID** ppv
    )
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<COemCF*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemCF::AddRef
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
ULONG __stdcall
COemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemCF::Release
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
_At_(this, __drv_freesMem(object))
ULONG __stdcall
COemCF::Release()
{
    assert(0 != m_cRef);

   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;
   }
   return cRef;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemCF::CreateInstance
//
//  Synopsis:
//      Attempt to create an object that implements the specified interface.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemCF::CreateInstance(
    _In_opt_    IUnknown* pUnknownOuter,
    _In_        CONST IID& iid,
    _Outptr_ VOID** ppv
    )
{
    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    //
    // Cannot aggregate.
    //
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    HRESULT hr = S_OK;

    if (iid == IID_IUnknown ||
        iid == IID_IPrintOemUI2)
    {
        //
        // Create component.
        //
        COemUI2* pOemCB = new COemUI2;
        if (pOemCB == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            // Get the requested interface.
            //
            hr = pOemCB->QueryInterface(iid, ppv);

            //
            // Release the IUnknown pointer.
            // (If QueryInterface failed, component will delete itself.)
            //
            pOemCB->Release();
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemCF::LockServer
//
//  Synopsis:
//      Locking the server holds the objects server in memory, so that new
//      instances of objects can be created more quickly.
//
//  Returns:
//      S_OK
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemCF::LockServer(
    BOOL bLock
        // If true, increment the lock count, otherwise decrement the lock
        // count.
    )
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2 (destructor)
//
//  Synopsis:
//      Release the helper interface, as well as any other resources that
//      the plug-in is holding onto.
//
//  Returns:
//
//  Notes:
//
//
//----------------------------------------------------------------------------
COemUI2::~COemUI2()
{
    //
    // Make sure that helper interface is released.
    //
    if(NULL != m_pCoreHelper)
    {
        m_pCoreHelper->Release();
        m_pCoreHelper = NULL;
    }

    //
    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    //
    assert(0 == m_cRef);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::PublishDriverInterface
//
//  Synopsis:
//      This routine is called by the core driver to supply objects to the
//      plug-in which the plug-in can use to implement various features.
//
//      This implementation requires an object that IPrintCoreHelper interface
//      from the core driver to implement full UI replacement.  If the object
//      passed in is the one desired, this routine will return S_OK, otherwise
//      it will return E_FAIL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::PublishDriverInterface(
    IUnknown *pIUnknown
    )
{
    HRESULT hr = S_OK;

    //
    // Save a pointer to the IPrintCoreHelperUni interface.  This will allow
    // the plug-in to access it's GDL file (which can be it's GPD file as well)
    //
    if (m_pCoreHelper == NULL)
    {
        hr = pIUnknown->QueryInterface(IID_IPrintCoreHelperUni, (VOID**) &(m_pCoreHelper));

        if (!SUCCEEDED(hr))
        {
            m_pCoreHelper = NULL;
            hr = E_FAIL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::GetInfo
//
//  Synopsis:
//      Most of this routine is a standard implementation common to most
//      plug-ins.  Of particular note in this sample is that to get a handle
//      the IPrintCoreHelper plug-in in the PublishDriverInterface callback,
//      this routine must return OEMPUBLISH_IPRINTCOREHELPER in the out
//      parameter when the mode is OEMGI_GETREQUESTEDHELPERINTERFACES.
//
//  Returns:
//      S_OK for supported modes, E_FAIL for everything else.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::GetInfo(
    DWORD  dwMode,
        // The mode indicates what information is being requested by the
        // core driver.
    _Out_writes_bytes_(cbSize) PVOID  pBuffer,
        // The output buffer is the location into which the requested info
        // should be written
    DWORD  cbSize,
        // The size of the output buffer.
    PDWORD pcbNeeded
        // If the size of the buffer is insufficient, use this parameter
        // to indicate to the core driver how much space is required.
    )
{
    HRESULT hrResult = S_OK;

    //
    // Validate parameters.  OEMGI_GETPUBLISHERINFO is excluded from this
    // list because that GetInfo mode applies only to PScript rendering plug-ins.
    //
    if ((NULL == pcbNeeded) ||
        ((OEMGI_GETSIGNATURE != dwMode) &&
         (OEMGI_GETVERSION != dwMode) &&
         (OEMGI_GETREQUESTEDHELPERINTERFACES != dwMode)))
    {
        WARNING("COemUI2::GetInfo() exit pcbNeeded is NULL or mode is not supported\r\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    //
    // Set expected buffer size and number of bytes written.
    //
    *pcbNeeded = sizeof(DWORD);

    //
    // Check buffer size is sufficient.
    //
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING("COemUI2::GetInfo() exit insufficient buffer!\r\n");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        hrResult = E_FAIL;
    }
    else
    {
        switch(dwMode)
        {
            //
            // OEM DLL Signature
            //
            case OEMGI_GETSIGNATURE:
                *(PDWORD)pBuffer = OEM_SIGNATURE;
                break;

            //
            // OEM DLL version
            //
            case OEMGI_GETVERSION:
                *(PDWORD)pBuffer = OEM_VERSION;
                break;

            //
            // The UI is asking whether the plug-in uses the IPrintCoreHelper based
            // helper object.  Indicate that the plug-in does need this object.
            //
            case OEMGI_GETREQUESTEDHELPERINTERFACES:
                *(PDWORD)pBuffer = 0;
                *(PDWORD)pBuffer |= OEMPUBLISH_IPRINTCOREHELPER;
                break;

            //
            // dwMode not supported.
            //
            default:
                //
                // Set written bytes to zero since nothing was written.
                //
                WARNING("COemUI2::GetInfo() exit mode not supported.\r\n");
                *pcbNeeded = 0;
                SetLastError(ERROR_NOT_SUPPORTED);
                hrResult = E_FAIL;
        }
    }

    return hrResult;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DevMode
//
//  Synopsis:
//      This routine operates in various modes to manage the private DEVMODE.
//      This sample does not provide much by way of illustration of how to
//      handle OEM settings in the private DEVMODE.  See the sample OEMUI for
//      a more in-depth look at this routine.  The function hrOEMDevMode in
//      devmode.cpp also provides more information on how to handle the various
//      modes.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DevMode(
    DWORD  dwMode,
        // The operation that should be performed
    POEMDMPARAM pOemDMParam
        // The input & output DEVMODEs, including pointers to the public &
        // private DEVMODE data.
    )
{
    return hrOEMDevMode(dwMode, pOemDMParam);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DocumentPropertySheets
//
//  Synopsis:
//      Show the GPD version in the debugger whenever the
//      printing preferences UI is shown.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    UNREFERENCED_PARAMETER(pPSUIInfo);
    UNREFERENCED_PARAMETER(lParam);

    this->DbgShowGPDVersion();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DbgShowGPDVersion
//
//  Synopsis:
//      Display the GPD version in the debugger using a GDL snapshot & MSXML 6
//      To make this work on versions of Windows prior to Windows Vista, change
//      references to MSXML 6 with MSXML 3.
//
//
//----------------------------------------------------------------------------
HRESULT
COemUI2::DbgShowGPDVersion()
{
    HRESULT hr;
    IXMLDOMDocument2 *pDOMSnapshot = NULL;
    IXMLDOMNode *pVersionNode = NULL;

    //
    // Get a DOM representation of the snapshot using MSXML 3
    //
    hr = LoadGDLSnapshot(&pDOMSnapshot);

    if (hr == REGDB_E_CLASSNOTREG)
    {
        DebugPrint(L"COemUI2::PublishHelperInterface: MSXML 6.0 not registered, GDL version lookup disabled\r\n");
    }

    //
    // Find the GDL node that contains the GPDFileVersion.  This should return a single XML DOM Element.
    //
    if (SUCCEEDED(hr))
    {
        hr = pDOMSnapshot->selectSingleNode((BSTR)TEXT("//gdl:SnapshotRoot/gdl:GDL_ATTRIBUTE[@Name=\"*GPDFileVersion\"]"), &pVersionNode);
    }

    //
    // If the node was found, write the version information to the debugger.
    // It's content is represented as CDATA in the XML, but MSXML will
    // normalize that to a regular string when it parses the XML.
    //
    if (SUCCEEDED(hr) && pVersionNode)
    {
        VARIANT varVersionString;

        //
        // Get DefaultOption name
        //
        hr = pVersionNode->get_nodeTypedValue(&varVersionString);

        if (SUCCEEDED(hr))
        {
            DebugPrint(L"COemUI2::PublishHelperInterface: GDL Version: %s\r\n", varVersionString.bstrVal);
        }

        VariantClear(&varVersionString);
    }

    if (pDOMSnapshot)
    {
        pDOMSnapshot->Release();
        pDOMSnapshot = NULL;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::LoadGDLSnapshot
//
//  Synopsis:
//      Load the GDL snapshot from IPrintCoreHelperUni into a DOM object,
//      and set appropriate properties on the DOM to prepare it for use
//      with xpath queries.
//
//
//----------------------------------------------------------------------------
HRESULT
COemUI2::LoadGDLSnapshot(
    _Out_ IXMLDOMDocument2 **ppDOMSnapshot
    )
{
    HRESULT hr = S_OK;
    IStream *pSnapshotStream = NULL;
    VARIANT_BOOL loadResult = 0;

    if (!ppDOMSnapshot)
    {
        return E_INVALIDARG;
    }

    (*ppDOMSnapshot) = NULL;

    hr = m_pCoreHelper->CreateDefaultGDLSnapshot(0, &pSnapshotStream);

    if (SUCCEEDED(hr))
    {
        //
        // Create DOM document
        //
        hr = m_pCoreHelper->CreateInstanceOfMSXMLObject(CLSID_DOMDocument60,
                                                        NULL,
                                                        CLSCTX_INPROC_SERVER,
                                                        IID_IXMLDOMDocument2,
                                                        (LPVOID *)ppDOMSnapshot);
    }

    if (SUCCEEDED(hr))
    {
        VARIANT streamVar;

        VariantInit(&streamVar);
        V_VT(&streamVar) = VT_UNKNOWN;
        V_UNKNOWN(&streamVar) = pSnapshotStream;
        pSnapshotStream->AddRef();

        //
        // Load the snapshot into the DOM document
        //
        (*ppDOMSnapshot)->put_async(VARIANT_FALSE);
        (*ppDOMSnapshot)->put_validateOnParse(VARIANT_FALSE);
        (*ppDOMSnapshot)->put_resolveExternals(VARIANT_FALSE);

        hr = (*ppDOMSnapshot)->load(streamVar, &loadResult);

        VariantClear(&streamVar);
    }

    //
    // If the DOM fails to load, the HRESULT will be S_FALSE, so explicitly
    // look for S_OK.
    //
    if (hr == S_OK)
    {
        //
        // Set the namespaces that should be available for xpath queries later.
        //
        BSTR bstrDefNameSpace = SysAllocString(TEXT("xmlns:gdl=\"http://schemas.microsoft.com/2002/print/gdl/1.0\""));

        if (!bstrDefNameSpace)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            VARIANT varDefNameSpace;

            VariantInit(&varDefNameSpace);
            varDefNameSpace.vt = VT_BSTR;
            varDefNameSpace.bstrVal = bstrDefNameSpace;

            hr = (*ppDOMSnapshot)->setProperty((BSTR)TEXT("SelectionNamespaces"), varDefNameSpace);
            SysFreeString(bstrDefNameSpace);
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if (FAILED(hr) && *ppDOMSnapshot)
    {
        (*ppDOMSnapshot)->Release();
        (*ppDOMSnapshot) = NULL;
    }

    if (pSnapshotStream)
    {
        pSnapshotStream->Release();
        pSnapshotStream = NULL;
    }

    return hr;
}

