/*****************************************************************************
 *
 *  errhandler.cpp
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION:
 *
 *  CErrHandler is a simple error handler, which works together with
 *  the wiadriver.
 *
 *******************************************************************************/
#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_driver_)

#include "stdafx.h"

#define MYDESCSTRING TEXT("Special driver device status error (only for testing purposes). Press 'Ok' to continue. Hitting 'Cancel' will abort the transfer.")

#define HANDLED_PRIVATE_STATUS_ERROR_1  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1001)

// {CFC1A4D4-5F27-4881-81E4-1BE314EB22F7}
static const GUID CLSID_WiaErrorHandler =
{ 0xcfc1a4d4, 0x5f27, 0x4881, { 0x81, 0xe4, 0x1b, 0xe3, 0x14, 0xeb, 0x22, 0xf7 } };

static LONG g_cLocks = 0;

void LockModule(void)   { InterlockedIncrement(&g_cLocks); }
void UnlockModule(void) { InterlockedDecrement(&g_cLocks); }

class CErrHandler : public IWiaErrorHandler
{
public:

    STDMETHODIMP
    QueryInterface(const IID& iid_requested, _COM_Outptr_ void** ppInterfaceOut);

    STDMETHODIMP_(ULONG)
    AddRef(void);

    STDMETHODIMP_(ULONG)
    Release(void);

    STDMETHODIMP
    ReportStatus(
              LONG        lFlags,
        _In_  HWND        hwndParent,
        _In_  IWiaItem2   *pWiaItem2,
              HRESULT     hrStatus,
              LONG        lPercentComplete);

    STDMETHODIMP
    GetStatusDescription(
              LONG        lFlags,
        _In_  IWiaItem2   *pWiaItem2,
              HRESULT     hrStatus,
        _Out_ BSTR        *pbstrDescription);

    CErrHandler() : m_nRefCount(0) {}
private:

    LONG        m_nRefCount;
};

///
///  QueryInterface
///
STDMETHODIMP
CErrHandler::QueryInterface(const IID& iid_requested, _COM_Outptr_ void** ppInterfaceOut)
{
    HRESULT hr = S_OK;

    hr = ppInterfaceOut ? S_OK : E_POINTER;

    if (SUCCEEDED(hr))
    {
        *ppInterfaceOut = NULL;
    }

    //
    //  We support IID_IUnknown and IID_IWiaErrorHandler
    //
    if (SUCCEEDED(hr))
    {
        if (IID_IUnknown == iid_requested)
        {
            *ppInterfaceOut = static_cast<IUnknown*>(this);
        }
        else if (IID_IWiaErrorHandler == iid_requested)
        {
            *ppInterfaceOut = static_cast<IWiaErrorHandler*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    if (SUCCEEDED(hr))
    {
        reinterpret_cast<IUnknown*>(*ppInterfaceOut)->AddRef();
    }

    return hr;
}

///
///  AddRef
///
STDMETHODIMP_(ULONG)
CErrHandler::AddRef(void)
{
    if (m_nRefCount == 0)
    {
        LockModule();
    }

    return InterlockedIncrement(&m_nRefCount);
}

///
///  Release
///
STDMETHODIMP_(ULONG)
CErrHandler::Release(void)
{
    ULONG nRetval = InterlockedDecrement(&m_nRefCount);

    if (0 == nRetval)
    {
        delete this;
        UnlockModule();
    }

    return nRetval;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CErrHandler::ReportStatus | ReportStatus implementation
 *
 *  @parm   LONG | lFlags |
 *          Flags - currently unused.
 *
 *  @parm   HWND | hwndParent |
 *          Window handle provided by the application
 *
 *  @parm   IWiaItem2 | pWiaItem2 |
 *          The item which is currently being transferred
 *
 *  @parm   HRESULT | hrStatus |
 *          Status code
 *
 *  @parm   LONG    | lPercentComplete
 *          Percent of operation completed (e.g. warming up device)
 *
 *
 *  @comm
 *  ReportStatus handles HANDLED_PRIVATE_STATUS_ERROR_1 for which it displays a modal
 *  dialog box which enables a user to cancel the transfer or to continue.
 *  For all other messages we return WIA_STATUS_NOT_HANDLED
 *
 *  @rvalue S_OK    |
 *              The function successfully handled the device status message.
 *
 *  @rvalue WIA_STATUS_NOT_HANDLED |
 *              The function does not handle this device status message
 *
 *  @rvalue E_XXX   |
 *              Error
 *
 *****************************************************************************/
STDMETHODIMP
CErrHandler::ReportStatus(
          LONG        lFlags,
    _In_  HWND        hwndParent,
    _In_  IWiaItem2   *pWiaItem2,
          HRESULT     hrStatus,
          LONG        lPercentComplete)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(lPercentComplete);

    HRESULT hr = pWiaItem2 ? WIA_STATUS_NOT_HANDLED : E_INVALIDARG;

    if ((WIA_STATUS_NOT_HANDLED == hr) && (HANDLED_PRIVATE_STATUS_ERROR_1 == hrStatus))
    {
        HINSTANCE hModule         = NULL;
        TCHAR bufDialog[MAX_PATH] = {0};
        TCHAR bufTitle[MAX_PATH]  = {0};

        hModule = GetModuleHandle(L"errhandler.dll");

        if (NULL != hModule &&
            LoadString(hModule, IDS_MESSAGEBOX_ERRORHANDLE_DIALOG, bufDialog, ARRAYSIZE(bufDialog)) &&
            LoadString(hModule, IDS_MESSAGEBOX_ERRORHANDLE_TITLE, bufTitle, ARRAYSIZE(bufTitle)) &&
            IDOK == MessageBox(hwndParent, bufDialog, bufTitle, MB_OKCANCEL|MB_TASKMODAL|MB_ICONERROR)
            )
        {
            hr = S_OK;
        }
        else
        {
            hr = HANDLED_PRIVATE_STATUS_ERROR_1;
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CErrHandler::GetStatusDescription | GetStatusDescription implementation
 *
 *  @parm   LONG | lFlags |
 *          Flags - currently unused.
 *
 *  @parm   IWiaItem2 | pWiaItem2 |
 *          The item which is currently being transferred
 *
 *  @parm   HRESULT | hrStatus |
 *          Status code
 *
 *  @parm   BSTR* | pbstrDescription |
 *          On S_OK this pbstrDescription will point to string with status description
 *
 *
 *  @comm
 *  GetStatusDescription handles HANDLED_PRIVATE_STATUS_ERROR_1 for which it returns
 *  a description string. It returns WIA_STATUS_NOT_HANDLED for all other messages
 *
 *  @rvalue S_OK    |
 *              The function successfully handled the device status message.
 *
 *  @rvalue WIA_STATUS_NOT_HANDLED |
 *              The function does not handle this device status message
 *
 *  @rvalue E_XXX   |
 *              Error
 *
 *****************************************************************************/
STDMETHODIMP
CErrHandler::GetStatusDescription(
          LONG        lFlags,
    _In_  IWiaItem2   *pWiaItem2,
          HRESULT     hrStatus,
    _Out_ BSTR        *pbstrDescription)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = (pbstrDescription && pWiaItem2) ? WIA_STATUS_NOT_HANDLED : E_INVALIDARG;

    if (WIA_STATUS_NOT_HANDLED == hr)
    {
        BSTR    bstrDescription = NULL;

        bstrDescription = SysAllocString((HANDLED_PRIVATE_STATUS_ERROR_1 == hrStatus) ? MYDESCSTRING : L"");

        if (bstrDescription)
        {
            *pbstrDescription = bstrDescription;
            hr = (HANDLED_PRIVATE_STATUS_ERROR_1 == hrStatus) ? S_OK : WIA_STATUS_NOT_HANDLED;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/*****************************************************************************
 *
 *  Class Object
 *
 *******************************************************************************/
class CErrClassObject : public IClassFactory
{
public:

    STDMETHODIMP
    QueryInterface(const IID& iid_requested, void** ppInterfaceOut)
    {
        HRESULT hr = S_OK;

        hr = ppInterfaceOut ? S_OK : E_POINTER;

        if (SUCCEEDED(hr))
        {
            *ppInterfaceOut = NULL;
        }

        //
        //  We only support IID_IUnknown and IID_IClassFactory
        //
        if (SUCCEEDED(hr))
        {
            if (IID_IUnknown == iid_requested)
            {
                *ppInterfaceOut = static_cast<IUnknown*>(this);
            }
            else if (IID_IClassFactory == iid_requested)
            {
                *ppInterfaceOut = static_cast<IClassFactory*>(this);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }

        if (SUCCEEDED(hr))
        {
            reinterpret_cast<IUnknown*>(*ppInterfaceOut)->AddRef();
        }

        return hr;
    }

    STDMETHODIMP_(ULONG)
    AddRef(void)
    {
        LockModule();
        return 2;
    }

    STDMETHODIMP_(ULONG)
    Release(void)
    {
        UnlockModule();
        return 1;
    }

    STDMETHODIMP
    CreateInstance(_In_opt_     IUnknown *pUnkOuter,
                   _In_         REFIID   riid,
                   _COM_Outptr_ void     **ppv)
    {
        CErrHandler  *pErrHandler = NULL;
        HRESULT      hr;

        hr = ppv ? S_OK : E_POINTER;

        if (SUCCEEDED(hr))
        {
            *ppv = 0;
        }

        if (SUCCEEDED(hr))
        {
            if (pUnkOuter)
            {
                hr = CLASS_E_NOAGGREGATION;
            }
        }

        if (SUCCEEDED(hr))
        {
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pErrHandler is freed on release.")
            pErrHandler = new CErrHandler();

            hr = pErrHandler ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            pErrHandler->AddRef();
            hr = pErrHandler->QueryInterface(riid, ppv);
            pErrHandler->Release();
        }

        return hr;
    }

    STDMETHODIMP
    LockServer(BOOL bLock)
    {
        if (bLock)
        {
            LockModule();
        }
        else
        {
            UnlockModule();
        }

        return S_OK;
    }
};

STDAPI DllCanUnloadNow(void)
{
    return (g_cLocks == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_           REFCLSID   rclsid,
                        _In_            REFIID     riid,
                        _Outptr_     void       **ppv)
{
    static  CErrClassObject s_FilterClass;

    HRESULT     hr;

    hr = ppv ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        if (rclsid == CLSID_WiaErrorHandler)
        {
            hr = s_FilterClass.QueryInterface(riid, ppv);
        }
        else
        {
            *ppv = 0;
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    return hr;
}

STDAPI DllUnregisterServer()
{
    return S_OK;
}

STDAPI DllRegisterServer()
{
    return S_OK;
}


