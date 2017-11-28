//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Intrface.cpp
//
//  PURPOSE:  Implementation of interface for PScript5, Unidrv5 UI plug-ins.
//
//--------------------------------------------------------------------------

#include "precomp.h"

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
//      Class factory for the Plug-In object
//--------------------------------------------------------------------------
class COemCF : public IClassFactory
{
public:

    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_
        REFIID riid,
        _COM_Outptr_ LPVOID FAR* ppvObj
        );

    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // IClassFactory methods
    STDMETHOD(CreateInstance) (THIS_
        _In_opt_     LPUNKNOWN pUnkOuter,
        _In_         REFIID riid,
        _COM_Outptr_ LPVOID FAR* ppvObject
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
        // GUID of the class object that the caller wants a class factory
        // for
    _In_ CONST IID& iid,
        // Interface ID to provide in ppv
    _Outptr_ VOID** ppv
        // out pointer to the interface requested.
    )
{
    VERBOSE(DLLTEXT("DllGetClassObject:Create class factory."));

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
    COemCF* pUIReplacementCF = new COemCF;
    if (pUIReplacementCF == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Get requested interface.
    //
    HRESULT hr = pUIReplacementCF->QueryInterface(iid, ppv);
    pUIReplacementCF->Release();

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
    _COM_Outptr_ VOID** ppv
    )
{
    VERBOSE(DLLTEXT("COemCF::QueryInterface entry."));

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
    _COM_Outptr_ VOID** ppv
    )
{
    VERBOSE(DLLTEXT("COemCF::CreateInstance entry."));

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
        VERBOSE(DLLTEXT("COemCF::IID_IUnknown\r\n"));

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
//      COemUI2::QueryInterface
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::QueryInterface(
    CONST IID& iid,
    _COM_Outptr_ VOID** ppv
    )
{
    VERBOSE(DLLTEXT("COemUI2::QueryInterface entry."));

    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        VERBOSE(DLLTEXT("COemUI2::QueryInterface - Return pointer to IUnknown.\r\n"));
    }
    else if (iid == IID_IPrintOemUI2)
    {
        *ppv = static_cast<IPrintOemUI2*>(this);
        VERBOSE(DLLTEXT("COemUI2::QueryInterface - Return pointer to IPrintOemUI.\r\n"));
    }
    else
    {
#if DBG
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, _countof(szOutput)); // can not fail!
        VERBOSE(DLLTEXT("COemUI2::QueryInterface %s not supported.\r\n"), szOutput);
#endif

        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::AddRef
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
ULONG __stdcall
COemUI2::AddRef()
{
    VERBOSE(DLLTEXT("COemUI2::AddRef entry."));

    return InterlockedIncrement(&m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::Release
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
ULONG __stdcall
COemUI2::Release()
{
   VERBOSE(DLLTEXT("COemUI2::Release entry."));

   assert(0 != m_cRef);

   ULONG cRef = InterlockedDecrement(&m_cRef);

   if (0 == cRef)
   {
      delete this;
      return 0;

   }

   return cRef;
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
    VERBOSE(DLLTEXT("COemUI2::PublishDriverInterface entry."));

    HRESULT hrResult = S_OK;

    //
    // Determine whether the published object is the helper interface
    // introduced in Vista to support full UI replacement.
    //
    if (m_pCoreHelper == NULL)
    {
        hrResult = pIUnknown->QueryInterface(IID_IPrintCoreHelper, (VOID**) &(m_pCoreHelper));

        if (!SUCCEEDED(hrResult))
        {
            m_pCoreHelper = NULL;
            hrResult = E_FAIL;
        }
    }

    return hrResult;
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
    _In_ DWORD  dwMode,
        // The mode indicates what information is being requested by the
        // core driver.
    _Out_writes_bytes_to_(cbSize, *pcbNeeded) PVOID  pBuffer,
        // The output buffer is the location into which the requested info
        // should be written
    _In_ DWORD  cbSize,
        // The size of the output buffer.
    _Out_ PDWORD pcbNeeded
        // If the size of the buffer is insufficient, use this parameter
        // to indicate to the core driver how much space is required.
    )
{
    HRESULT hrResult = S_OK;

#if DBG
    PWSTR pszTag;

    switch(dwMode)
    {
        case OEMGI_GETSIGNATURE: pszTag = L"COemUI2::GetInfo entry. [OEMGI_GETSIGNATURE]"; break;
        case OEMGI_GETVERSION: pszTag = L"COemUI2::GetInfo entry. [OEMGI_GETVERSION]"; break;
        case OEMGI_GETREQUESTEDHELPERINTERFACES: pszTag = L"COemUI2::GetInfo entry. [OEMGI_GETREQUESTEDHELPERINTERFACES]"; break;
        default: pszTag = L"COemUI2::GetInfo entry."; break;
    }

    VERBOSE(pszTag);
#endif

    //
    // Validate parameters.  OEMGI_GETPUBLISHERINFO is excluded from this
    // list because that GetInfo mode applies only to PScript rendering plug-ins.
    //
    if ((NULL == pcbNeeded) ||
        ((OEMGI_GETSIGNATURE != dwMode) &&
         (OEMGI_GETVERSION != dwMode) &&
         (OEMGI_GETREQUESTEDHELPERINTERFACES != dwMode)))
    {
        WARNING(DLLTEXT("COemUI2::GetInfo() exit pcbNeeded is NULL or mode is not supported\r\n"));
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
        WARNING(DLLTEXT("COemUI2::GetInfo() exit insufficient buffer!\r\n"));
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
            // New core helper interface support
            //
            case OEMGI_GETREQUESTEDHELPERINTERFACES:
                *(PDWORD)pBuffer = 0;
                *(PDWORD)pBuffer |= OEMPUBLISH_IPRINTCOREHELPER;
                break;

            //
            // dwMode not supported.
            //
            default:
                // Set written bytes to zero since nothing was written.
                //
                WARNING(DLLTEXT("COemUI2::GetInfo() exit mode not supported.\r\n"));
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
    VERBOSE(DLLTEXT("COemUI2::Devmode entry."));

    return hrOEMDevMode(dwMode, pOemDMParam);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::HideStandardUI
//
//  Synopsis:
//      This routine is critical to supporting full UI replacement.  This
//      routine allows the plug-in to disable Unidrv's (or PScript's) standard
//      UI panels.  If this returns E_NOTIMPL, the panel isn't hidden.  If
//      this routine returns S_OK, Unidrv will not display any UI for the
//      specified mode.  If this returns E_NOTIMPL, Unidrv will display it's
//      standard UI.
//
//  Returns:
//      S_OK to hide the indicated UI if desired, otherwise E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::HideStandardUI(
    DWORD       dwMode
        // document property sheets, printer property sheets, or possibly
        // something not yet defined.
    )
{
    VERBOSE(DLLTEXT("COemUI2::HideStandardUI entry."));

    HRESULT hrResult = E_NOTIMPL;

    switch(dwMode)
    {
        case OEMCUIP_DOCPROP:
        case OEMCUIP_PRNPROP:
            hrResult = S_OK;
            break;
        default:
            // If we don't recognize the mode, assume that we don't want to hide
            // the UI provided by Unidrv.
            hrResult = E_NOTIMPL;
    }

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DocumentPropertySheets
//
//  Synopsis:
//      See function hrOEMDocumentPropertySheets in uniuirep.cpp.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::DocumentPropertySheets entry."));

    return hrCommonPropSheetMethod(pPSUIInfo, lParam, m_pCoreHelper, &m_Features, OEMCUIP_DOCPROP);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DevicePropertySheets
//
//  Synopsis:
//      See function hrOEMDevicePropertySheets in uniuirep.cpp
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::DevicePropertySheets entry."));

    return hrCommonPropSheetMethod(pPSUIInfo, lParam, m_pCoreHelper, &m_Features, OEMCUIP_PRNPROP);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::CommonUIProp
//
//  Synopsis:
//      This routine is used to interact with the Unidrv-supplied
//      property sheet pages.  Since this plug-in implements full-UI
//      replacement, those property sheet pages are disabled, and this
//      routine does not need to do anything.
//
//  Returns:
//      S_OK
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::CommonUIProp entry."));

    UNREFERENCED_PARAMETER(dwMode);
    UNREFERENCED_PARAMETER(pOemCUIPParam);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DeviceCapabilities
//
//  Synopsis:
//      This routine can be used to replace the Unidrvs device capabilities
//      handling.  In this implementation, no custom capabilities handling is
//      provided.
//
//  Returns:
//      E_FAIL
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DeviceCapabilities(
    _Inout_ POEMUIOBJ poemuiobj,
        // OEM settings & information
    _In_  HANDLE      hPrinter,
        // Handle to the printer that the capabilities are being requested for
    _In_  PWSTR       pDeviceName,
        // Name of the device / driver
    _In_  WORD        wCapability,
        // The DC_ ID indicating which capability was requested.
    _Out_writes_(_Inexpressible_("varies with wCapability")) PVOID       pOutput,
        // Buffer to write requested information to.
    _In_  PDEVMODE    pPublicDM,
        // Pointer to the public DEVMODE representing the settings to get
        // capabilities with respect to.
    _In_  PVOID       pOEMDM,
        // OEM private DEVMODE settings.
    _In_  DWORD       dwOld,
        // Result from previous plug-in call to this routine.
    _Out_ DWORD       *dwResult
        // Result of this call.  If there are multiple UI plug-ins, this
        // result is passed to the next one as dwOld
    )
{
    VERBOSE(DLLTEXT("COemUI2::DeviceCapabilities entry."));

    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(pDeviceName);
    UNREFERENCED_PARAMETER(wCapability);
    UNREFERENCED_PARAMETER(pOutput);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);
    UNREFERENCED_PARAMETER(dwOld);
    UNREFERENCED_PARAMETER(dwResult);

    //
    // Do nothing.  Let Unidrv handle the device capabilities processing.
    //

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DevQueryPrintEx
//
//  Synopsis:
//      This routine is used to determine print job compatibility with the
//      current driver.  The plug-in can supplement Unidrv's handling of this
//      routine.  If Unidrv determines that a job can be printed on the current
//      printer it will call this routine to ask the plug-in whether it can
//      also handle the current job.  If Unidrv determines that the job cannot
//      be printed, it will not call the plug-in.  Additionally, XPS drivers
//      built on the Unidrv and PScript UI modules may not recieve this call-
//      back.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM
    )
{
    VERBOSE(DLLTEXT("COemUI2::DevQueryPrintEx entry."));

    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(pDQPInfo);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::UpgradePrinter
//
//  Synopsis:
//      Use this callback to upgrade any settings from previous versions
//      that are stored in the registry by the plug-in Not applicabe to
//      this sample.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::UpgradePrinter(
    _In_ DWORD   dwLevel,
    _At_((PDRIVER_UPGRADE_INFO_1)pDriverUpgradeInfo, _In_) PBYTE   pDriverUpgradeInfo
    )
{
    VERBOSE(DLLTEXT("COemUI2::UpgradePrinter entry."));

    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(pDriverUpgradeInfo);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::PrinterEvent
//
//  Synopsis:
//      Perform any special processing needed when various events occur to the
//      print queue.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::PrinterEvent(
    _In_ PWSTR   pPrinterName,
    _In_ INT     iDriverEvent,
    _In_ DWORD   dwFlags,
    _In_ LPARAM  lParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::PrinterEvent entry."));

    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(iDriverEvent);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DriverEvent
//
//  Synopsis:
//      Notifies the plug-in of changes or events relevant to the driver,
//      such as installing and upgrading.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DriverEvent(
    _In_ DWORD   dwDriverEvent,
    _In_ DWORD   dwLevel,
    _In_reads_(_Inexpressible_("varies")) LPBYTE  pDriverInfo,
    _In_ LPARAM  lParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::DriverEvent entry."));

    UNREFERENCED_PARAMETER(dwDriverEvent);
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(pDriverInfo);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
};


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::QueryColorProfile
//
//  Synopsis:
//      This routine can be implemented to provide a color profile from the
//      driver.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulQueryMode,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData
            )
{
    VERBOSE(DLLTEXT("COemUI2::QueryColorProfile entry."));

    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);
    UNREFERENCED_PARAMETER(ulQueryMode);
    UNREFERENCED_PARAMETER(pvProfileData);
    UNREFERENCED_PARAMETER(pcbProfileData);
    UNREFERENCED_PARAMETER(pflProfileData);

    return E_NOTIMPL;
};

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::FontInstallerDlgProc
//
//  Synopsis:
//      Plug-ins can use this method to replace Unidrv's provided soft-font
//      installer dialog.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::FontInstallerDlgProc(
    HWND    hWnd,
    UINT    usMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    VERBOSE(DLLTEXT("COemUI2::FontInstallerDlgProc entry."));

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(usMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
};

//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::UpdateExternalFonts
//
//  Synopsis:
//      This routine is used to notify the UI of any changes to installed
//      font cartridges, primarily for supporting soft-font UI replacement.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::UpdateExternalFonts(
        _In_ HANDLE  hPrinter,
        _In_ HANDLE  hHeap,
        _In_ PWSTR   pwstrCartridges
        )
{
    VERBOSE(DLLTEXT("COemUI2::UpdateExternalFonts entry."));

    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(hHeap);
    UNREFERENCED_PARAMETER(pwstrCartridges);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::QueryJobAttributes(
    _In_             HANDLE      hPrinter,
    _In_             PDEVMODE    pDevmode,
    _In_range_(1, 4) DWORD       dwLevel,
    _In_reads_(_Inexpressible_("varies")) LPBYTE      lpAttributeInfo
    )
{
    VERBOSE(DLLTEXT("COemUI2::QueryJobAttributes entry."));

    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(pDevmode);
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(lpAttributeInfo);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      COemUI2::DocumentEvent
//
//  Synopsis:
//      Perform any special processing needed at various points in time while
//      a job is printing.
//
//      Implementation of this routine is optional.  To indicate that the
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall
COemUI2::DocumentEvent(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pbIn,
    ULONG       cbOut,
    PVOID       pbOut,
    PINT        piResult
    )
{
    VERBOSE(DLLTEXT("COemUI2::DocumentEvent entry."));

    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(iEsc);
    UNREFERENCED_PARAMETER(cbIn);
    UNREFERENCED_PARAMETER(pbIn);
    UNREFERENCED_PARAMETER(cbOut);
    UNREFERENCED_PARAMETER(pbOut);
    UNREFERENCED_PARAMETER(piResult);

    return E_NOTIMPL;
}

