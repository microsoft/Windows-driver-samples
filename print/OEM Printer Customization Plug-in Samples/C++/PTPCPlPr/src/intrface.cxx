//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    intrface.cxx
//    
//
//  PURPOSE:  Implementation of interface for PScript4, PScript5, Unidrv4, 
//            Unidrv5 UI plug-ins.
//
//


#include "precomp.hxx"
#include "debug.hxx"
#include "intrface.hxx"
#include "oem.hxx"
#include "oemptprovider.hxx"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      Private Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

////////////////////////////////////////////////////////////////////////////////
//
// IOemUI body
//

IOemUI::~IOemUI()
{
    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    assert(0 == m_cRef);
}

HRESULT __stdcall IOemUI::QueryInterface(const IID& iid, void** ppv)
{    
    VERBOSE(DLLTEXT("IOemUI:QueryInterface entry.\r\n\r\n")); 
    if (IID_IUnknown == iid)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IOemUI:Return pointer to IUnknown.\r\n\r\n")); 
    }
    else if (IID_IPrintOemUI == iid)
    {
        *ppv = static_cast<IPrintOemUI*>(this) ;
        VERBOSE(DLLTEXT("IOemUI:Return pointer to IPrintOemUI.\r\n")); 
    }
    else
    {
        VERBOSE(DLLTEXT("IOemUI::QueryInterface not supported.\r\n")); 
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemUI::AddRef()
{
    VERBOSE(DLLTEXT("IOemUI:AddRef entry.\r\n")); 
    return InterlockedIncrement(&m_cRef) ;
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall IOemUI::Release() 
{
   VERBOSE(DLLTEXT("IOemUI:Release entry.\r\n")); 
   ASSERT (0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;
   }
   return cRef;
}

HRESULT __stdcall IOemUI::PublishDriverInterface(
    IUnknown *)
{
    VERBOSE(DLLTEXT("IOemUI:PublishDriverInterface entry.\r\n")); 

    return S_OK;
}

HRESULT __stdcall IOemUI::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{
    VERBOSE(DLLTEXT("IOemUI::GetInfo entry.\r\r\n"));

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
          &&
          (OEMGI_GETVERSION != dwMode)
          &&
          (OEMGI_GETPUBLISHERINFO != dwMode)
        )
      )
    {
        WARNING(DLLTEXT("IOemUI::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER\r\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size and number of bytes written.
    *pcbNeeded = sizeof(DWORD);

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING(DLLTEXT("IOemUI::GetInfo() exit insufficient buffer!\r\r\n"));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = OEM_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = OEM_VERSION;
            break;

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IOemUI::GetInfo() exit mode not supported.\r\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IOemUI::GetInfo() exit S_OK.\r\r\n"));
    return S_OK;
}

HRESULT __stdcall IOemUI::DevMode(
    DWORD  ,
    POEMDMPARAM )
{   
    VERBOSE(DLLTEXT("IOemUI:DevMode entry.\r\n")); 

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::CommonUIProp(
    DWORD ,
    POEMCUIPPARAM)
{
    VERBOSE(DLLTEXT("IOemUI:CommonUIProp entry.\r\n")); 

    return E_NOTIMPL;
}


HRESULT __stdcall IOemUI::DocumentPropertySheets(
    PPROPSHEETUI_INFO ,
    LPARAM            )
{
    VERBOSE(DLLTEXT("IOemUI:DocumentPropertySheets entry.\r\n")); 

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DevicePropertySheets(
    PPROPSHEETUI_INFO ,
    LPARAM            )
{
    VERBOSE(DLLTEXT("IOemUI:DevicePropertySheets entry.\r\n")); 

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DeviceCapabilities(
            POEMUIOBJ ,
            HANDLE    ,
_In_        PWSTR     ,
            WORD      ,
            PVOID     ,
            PDEVMODE  ,
            PVOID     ,
            DWORD     ,
            DWORD *   )
{
    VERBOSE(DLLTEXT("IOemUI:DeviceCapabilities entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DevQueryPrintEx(
    POEMUIOBJ           ,
    PDEVQUERYPRINT_INFO ,
    PDEVMODE            ,
    PVOID               )
{
    VERBOSE(DLLTEXT("IOemUI:DevQueryPrintEx entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::UpgradePrinter(
    DWORD ,
    PBYTE )
{
    VERBOSE(DLLTEXT("IOemUI:UpgradePrinter entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::PrinterEvent(
_In_  PWSTR  ,
      INT    ,
      DWORD  ,
      LPARAM )
{
    VERBOSE(DLLTEXT("IOemUI:PrinterEvent entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DriverEvent(
    DWORD  ,
    DWORD  ,
    LPBYTE ,
    LPARAM )
{
    VERBOSE(DLLTEXT("IOemUI:DriverEvent entry.\r\n"));

    return E_NOTIMPL;
};


#define PROFILE_NAME    L"OEMPROFILE.icm\0"

HRESULT __stdcall IOemUI::QueryColorProfile(
            HANDLE      ,
            POEMUIOBJ   ,
            PDEVMODE    ,
            PVOID       ,
            ULONG       ,
            VOID       *,
            ULONG      *,
            FLONG      *)
{

    VERBOSE(DLLTEXT("IOemUI:QueryColorProfile entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IOemUI::FontInstallerDlgProc(
        HWND    ,
        UINT    ,
        WPARAM  ,
        LPARAM  ) 
{
    VERBOSE(DLLTEXT("IOemUI:FontInstallerDlgProc entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IOemUI::UpdateExternalFonts(
        HANDLE  ,
        HANDLE  ,
_In_    PWSTR   )
{
    VERBOSE(DLLTEXT("IOemUI:UpdateExternalFonts entry.\r\n"));

    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class IOemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG,AddRef)  (THIS);

    // the _At_ tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    _At_(this, __drv_freesMem(object)) 
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_
                               LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)     (THIS_ BOOL bLock);


    // Constructor
    IOemCF(): m_cRef(1) {};
    ~IOemCF() {};

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((IID_IUnknown == iid) || (IID_IClassFactory == iid))
    {
        *ppv = static_cast<IOemCF*>(this) ;
    }
    else
    {
        VERBOSE(DLLTEXT("IOemCF::QueryInterface not supported.\r\n")); 
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef) ;
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall IOemCF::Release()
{
   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;
   }
   return cRef;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    HRESULT hr = S_OK;

    //DbgPrint(DLLTEXT("Class factory:\t\tCreate component.")) ;

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Cannot aggregate.
    if (NULL != pUnknownOuter)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create component.
    IOemUI* pOemCB = new IOemUI ;
    if (NULL == pOemCB)
    {
        return E_OUTOFMEMORY ;
    }
    // Get the requested interface.
    hr = pOemCB->QueryInterface(iid, ppv) ;

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCB->Release() ;

    if (FAILED(hr))
    {
        IOEMPTProvider * pOemPT = new IOEMPTProvider();
        
        if(NULL == pOemPT)
        {
            return E_OUTOFMEMORY;
        }
        
        hr = pOemPT->QueryInterface(iid, ppv) ;
        
        pOemPT->Release();
    }

    return hr ;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks) ;
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks) ;
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Exported functions
//


// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    //
    // To avoid leaving OEM DLL still in memory when Unidrv or Pscript drivers 
    // are unloaded, Unidrv and Pscript driver ignore the return value of 
    // DllCanUnloadNow of the OEM DLL, and always call FreeLibrary on the OEMDLL.
    //
    // If OEM DLL spins off a working thread that also uses the OEM DLL, the 
    // thread needs to call LoadLibrary and FreeLibraryAndExitThread, otherwise 
    // it may crash after Unidrv or Pscript calls FreeLibrary.
    //

    if ((0 == g_cComponents) && (0 == g_cServerLocks))
    {
        return S_OK ;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI  DllGetClassObject(
    _In_ REFCLSID clsid, 
    _In_ REFIID iid, 
    _Outptr_ LPVOID* ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:Create class factory.\r\n"));

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Can we create this component?
    if (CLSID_OEMUI != clsid && CLSID_OEMPTPROVIDER != clsid)
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF ;  // Reference count set to 1
                                         // in constructor
    if (NULL == pFontCF)
    {
        return E_OUTOFMEMORY ;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv) ;
    pFontCF->Release() ;

    return hr ;
}
