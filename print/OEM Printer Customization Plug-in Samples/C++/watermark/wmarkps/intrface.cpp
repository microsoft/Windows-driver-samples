//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Intrface.cpp
//
//
//  PURPOSE:  Interface for User Mode COM Customization DLL.
//
//
#pragma once

#include "precomp.h"

#include "wmarkps.h"
#include "debug.h"
#include "command.h"
#include "intrface.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0;     // Count of active components
static long g_cServerLocks = 0;    // Count of locks


////////////////////////////////////////////////////////////////////////////////
//
// IWaterMarkPS body
//
IWaterMarkPS::~IWaterMarkPS()
{
    // Make sure that helper interface is released.
    if(NULL != m_pOEMHelp)
    {
        m_pOEMHelp->Release();
        m_pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    assert(0 == m_cRef);
}


HRESULT __stdcall IWaterMarkPS::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        VERBOSE(DLLTEXT("IWaterMarkPS::QueryInterface IUnknown.\r\n"));
    }
    else if (iid == IID_IPrintOemPS)
    {
        *ppv = static_cast<IPrintOemPS*>(this);
        VERBOSE(DLLTEXT("IWaterMarkPS::QueryInterface IPrintOemPs.\r\n"));
    }
    else
    {
#if DBG && defined(USERMODE_DRIVER)
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, _countof(szOutput)); // can not fail!
        VERBOSE(DLLTEXT("IWaterMarkPS::QueryInterface interface not supported.\r\n"));
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IWaterMarkPS::AddRef()
{
    VERBOSE(DLLTEXT("IWaterMarkPS::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall IWaterMarkPS::Release()
{
    VERBOSE(DLLTEXT("IWaterMarkPS::Release() entry.\r\n"));
   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;

   }
   return cRef;
}


HRESULT __stdcall IWaterMarkPS::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::GetInfo entry.\r\n"));

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
        VERBOSE(DLLTEXT("IWaterMarkPS::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size.
    if(OEMGI_GETPUBLISHERINFO != dwMode)
    {
        *pcbNeeded = sizeof(DWORD);
    }
    else
    {
        *pcbNeeded = sizeof(PUBLISHERINFO);
        return E_FAIL;
    }

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING(DLLTEXT("IWaterMarkPS::GetInfo() exit insufficient buffer!\r\n"));
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
        case OEMGI_GETPUBLISHERINFO:
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IWaterMarkPS::GetInfo() exit mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::PublishDriverInterface() entry.\r\n"));

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->m_pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverPS, (void** ) &(this->m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT __stdcall IWaterMarkPS::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::EnableDriver() entry.\r\n"));

    UNREFERENCED_PARAMETER(dwDriverVersion);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pded);

    // Need to return S_OK so that DisableDriver() will be called, which Releases
    // the reference to the Printer Driver's interface.
    // If error occurs, return E_FAIL.
    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::DisableDriver(VOID)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::DisaleDriver() entry.\r\n"));

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::DisablePDEV() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);

    return E_NOTIMPL;
};

HRESULT __stdcall IWaterMarkPS::EnablePDEV(
    PDEVOBJ         pdevobj,
    _In_ PWSTR      pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *pDevOem)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::EnablePDEV() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(cPatterns);
    UNREFERENCED_PARAMETER(phsurfPatterns);
    UNREFERENCED_PARAMETER(cjGdiInfo);
    UNREFERENCED_PARAMETER(pGdiInfo);
    UNREFERENCED_PARAMETER(cjDevInfo);
    UNREFERENCED_PARAMETER(pDevInfo);
    UNREFERENCED_PARAMETER(pded);
    UNREFERENCED_PARAMETER(pDevOem);

    return E_NOTIMPL;
}


HRESULT __stdcall IWaterMarkPS::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ         pdevobjNew)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::ResetPDEV() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobjOld);
    UNREFERENCED_PARAMETER(pdevobjNew);

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkPS::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{
    VERBOSE(DLLTEXT("IWaterMarkPS:DevMode entry.\n"));
    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IWaterMarkPS::Command(
    PDEVOBJ     pdevobj,
    DWORD       dwIndex,
    PVOID       pData,
    DWORD       cbSize,
    OUT DWORD   *pdwResult)
{
    HRESULT hResult = E_NOTIMPL;


    VERBOSE(DLLTEXT("IWaterMarkPS::Command() entry.\r\n"));
    hResult = PSCommand(pdevobj, dwIndex, pData, cbSize, m_pOEMHelp, pdwResult);

    return hResult;
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
    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this);
    }
    else
    {
        VERBOSE(DLLTEXT("IOemCF::QueryInterface interface not supported.\r\n"));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
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
    //VERBOSE(DLLTEXT("Class factory:\t\tCreate component."));

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    IWaterMarkPS* pOemCP = new IWaterMarkPS;
    if (pOemCP == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pOemCP->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCP->Release();
    return hr;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
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


//
// Registration functions
//

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

    VERBOSE(DLLTEXT("DllCanUnloadNow entered.\r\n"));

    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(
    _In_ REFCLSID clsid, 
    _In_ REFIID iid, 
    _Outptr_ LPVOID* ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:\tCreate class factory.\r\n"));

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hr;
}
