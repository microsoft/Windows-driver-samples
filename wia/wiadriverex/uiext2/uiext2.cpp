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

// {61364062-0593-4eda-84d2-f5531d8c3259}
static const GUID CLSID_WiaUIExt2 =
{ 0x61364062, 0x0593, 0x4eda, { 0x84, 0xd2, 0xf5, 0x53, 0x1d, 0x8c, 0x32, 0x59 } };

static LONG g_cLocks = 0;
static HINSTANCE g_hInst = 0;


void LockModule(void)   { InterlockedIncrement(&g_cLocks); }
void UnlockModule(void) { InterlockedDecrement(&g_cLocks); }

HRESULT TransferFromWiaItem( PDEVICEDIALOGDATA2 pDeviceDialogData, IWiaItem2 *pWiaFlatbed);

class CWiaUIExtension2 : public IWiaUIExtension2
{
public:

    STDMETHODIMP
    QueryInterface(const IID& iid_requested, _COM_Outptr_ void** ppInterfaceOut);

    STDMETHODIMP_(ULONG)
    AddRef(void);

    STDMETHODIMP_(ULONG)
    Release(void);

    STDMETHODIMP
    DeviceDialog(_In_ PDEVICEDIALOGDATA2 pDeviceDialogData );

    STDMETHODIMP
    GetDeviceIcon(_In_ BSTR bstrDeviceId, _Out_ HICON *phIcon, ULONG nSize );

    CWiaUIExtension2() : m_nRefCount(0) {}

private:

    LONG        m_nRefCount;
};

STDMETHODIMP
CWiaUIExtension2::QueryInterface(const IID& iid_requested, _COM_Outptr_ void** ppInterfaceOut)
{
    HRESULT hr = S_OK;

    hr = ppInterfaceOut ? S_OK : E_POINTER;

    if (SUCCEEDED(hr))
    {
        *ppInterfaceOut = NULL;
    }

    //
    //  We support IID_IUnknown and IWiaUIExtension2
    //
    if (SUCCEEDED(hr))
    {
        if (IID_IUnknown == iid_requested)
        {
            *ppInterfaceOut = static_cast<IUnknown*>(this);
        }
        else if (IID_IWiaUIExtension2 == iid_requested)
        {
            *ppInterfaceOut = static_cast<IWiaUIExtension2*>(this);
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
CWiaUIExtension2::AddRef(void)
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
CWiaUIExtension2::Release(void)
{
    ULONG nRetval = InterlockedDecrement(&m_nRefCount);

    if (0 == nRetval)
    {
        delete this;
        UnlockModule();
    }

    return nRetval;
}

//
// IWiaUIExtension2
//
STDMETHODIMP
CWiaUIExtension2::DeviceDialog(_In_ PDEVICEDIALOGDATA2 pDeviceDialogData)
{
    HRESULT             hr = S_OK;
    IEnumWiaItem2*      pEnumItem = NULL;
    IWiaItem2*          pWiaFlatbed = NULL;
    GUID                guidCategory = WIA_CATEGORY_FLATBED;
    TCHAR               bufDialog[MAX_PATH] = {0};
    TCHAR               bufTitle[MAX_PATH]  = {0};

    if (LoadString(g_hInst, IDS_MESSAGEBOX_WIAUIEXTENSION_DIALOG, bufDialog, ARRAYSIZE(bufDialog)) &&
        LoadString(g_hInst, IDS_MESSAGEBOX_WIAUIEXTENSION_TITLE, bufTitle, ARRAYSIZE(bufTitle))
        )
    {
        MessageBox( NULL, bufDialog, bufTitle, 0 );
    }

    hr = pDeviceDialogData->pIWiaItemRoot->EnumChildItems(&guidCategory, &pEnumItem);

    if (SUCCEEDED(hr))
    {
        ULONG ulFetched = 0;
        hr = pEnumItem->Next(1, &pWiaFlatbed, &ulFetched);

        if (SUCCEEDED(hr) && (1 == ulFetched))
        {
            hr = TransferFromWiaItem(pDeviceDialogData, pWiaFlatbed);

            pWiaFlatbed->Release();
        }

        pEnumItem->Release();
    }

    return hr;
}

STDMETHODIMP
CWiaUIExtension2::GetDeviceIcon(_In_ BSTR bstrDeviceId, _Out_ HICON *phIcon, ULONG nSize )
{
    UNREFERENCED_PARAMETER(bstrDeviceId);

    //
    // Load an icon, and copy it, using CopyIcon, so it will still be valid if our interface is freed
    //
    HICON hIcon = reinterpret_cast<HICON>(LoadImage( g_hInst, MAKEINTRESOURCE(IDI_TESTDEVICE), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR ));
    if (hIcon)
    {
        *phIcon = CopyIcon(hIcon);
        DestroyIcon(hIcon);
        return S_OK;
    }
    return E_FAIL;
}


/*****************************************************************************
 *
 *  Class Object
 *
 *******************************************************************************/
class CUIExt2ClassObject : public IClassFactory
{
public:

    STDMETHODIMP
    QueryInterface(const IID& iid_requested, _COM_Outptr_ void** ppInterfaceOut)
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
        CWiaUIExtension2  *pExt = NULL;
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
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pExt is freed on release.")
            pExt = new CWiaUIExtension2();

            hr = pExt ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            pExt->AddRef();
            hr = pExt->QueryInterface(riid, ppv);
            pExt->Release();
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
                         _In_           REFIID     riid,
                         _Outptr_    void       **ppv)
{
    static  CUIExt2ClassObject s_FilterClass;

    HRESULT     hr;

    hr = ppv ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        if (rclsid == CLSID_WiaUIExt2)
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

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hinst;
        break;
    }
    return TRUE;
}

class CSimpleWIACallback : public IWiaTransferCallback
{
private:
    volatile LONG m_cRef;

public:
    STDMETHODIMP CSimpleWIACallback::QueryInterface(const IID &iid_requested, _COM_Outptr_ void** ppInterfaceOut)
    {
        HRESULT hr = S_OK;

        hr = ppInterfaceOut ? S_OK : E_POINTER;

        if (SUCCEEDED(hr))
        {
            *ppInterfaceOut = NULL;
        }

        //
        //  We support IID_IUnknown and IID_IWiaTransferCallback
        //
        if (SUCCEEDED(hr))
        {
            if (IID_IUnknown == iid_requested)
            {
                *ppInterfaceOut = static_cast<IWiaTransferCallback*>(this);
            }
            else if (IID_IWiaTransferCallback == iid_requested)
            {
                *ppInterfaceOut = static_cast<IWiaTransferCallback*>(this);
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
    CSimpleWIACallback::AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    CSimpleWIACallback::Release()
    {
        ULONG ulRefCount = InterlockedDecrement(&m_cRef);

        if (0 == ulRefCount)
        {
            delete this;
        }

        return ulRefCount;
    }

private:
    IStream * m_pStream;

public:
    CSimpleWIACallback(IStream * pStream): m_cRef(1)
    {
        m_pStream = pStream;
    }

    STDMETHODIMP GetNextStream(
                       LONG     lFlags,
                _In_z_ BSTR     bstrItemName,
                _In_z_ BSTR     bstrFullItemName,
                _Outptr_result_maybenull_ _At_(*ppDestination, _When_(return == S_OK, _Post_notnull_))
                       IStream  **ppDestination)
    {
        UNREFERENCED_PARAMETER(lFlags);
        UNREFERENCED_PARAMETER(bstrItemName);
        UNREFERENCED_PARAMETER(bstrFullItemName);

        HRESULT hr = S_OK;

        if (ppDestination)
        {
            *ppDestination = NULL;
        }

        if (m_pStream)
        {
            *ppDestination = m_pStream;
        }
        else
        {
            hr = E_FAIL;
        }
        return hr;
    }

    STDMETHODIMP TransferCallback(LONG lFlags, _In_ WiaTransferParams *pWiaTransferParams)
    {
        UNREFERENCED_PARAMETER(lFlags);
        UNREFERENCED_PARAMETER(pWiaTransferParams);

        return S_OK;
    }

};


/*****************************************************************************\

    TransferFromWiaItem

    Transfers the file from the given wia item to the file name specified in the
    pDeviceDialogData

*****************************************************************************/
HRESULT TransferFromWiaItem( PDEVICEDIALOGDATA2 pDeviceDialogData, IWiaItem2 *pWiaFlatbed)
{

    HRESULT hr = S_OK;
    IStream * pStream = NULL;
    IWiaTransfer * pWiaTransfer = NULL;
    IWiaPropertyStorage * pPropertyStorage = NULL;

    if (!pDeviceDialogData || !pWiaFlatbed)
    {
        hr = E_INVALIDARG;
    }

    WCHAR *pFileName = (WCHAR*) LocalAlloc(LPTR, sizeof(WCHAR)*MAX_PATH);
    WCHAR *pUniqueFileName = (WCHAR*) LocalAlloc(LPTR, sizeof(WCHAR)*MAX_PATH);

    if(!pFileName || !pUniqueFileName)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pWiaFlatbed->QueryInterface(IID_IWiaPropertyStorage, (LPVOID *)&pPropertyStorage);

        if (SUCCEEDED(hr))
        {
            PROPSPEC pSpec[1] = {0};
            PROPVARIANT pVar[1] = {0};
            GUID guidFormat = WiaImgFmt_BMP;

            pSpec[0].ulKind = PRSPEC_PROPID;
            pSpec[0].propid = WIA_IPA_FORMAT;

            pVar[0].vt = VT_CLSID;
            pVar[0].puuid = &guidFormat;

            hr = pPropertyStorage->WriteMultiple(1, pSpec, pVar, WIA_IPS_FIRST);

            pPropertyStorage->Release();
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pWiaFlatbed->QueryInterface(IID_IWiaTransfer, (LPVOID *)&pWiaTransfer);
    }

    if (SUCCEEDED(hr))
    {
        hr = StringCchCopyW(pFileName, MAX_PATH, pDeviceDialogData->bstrFolderName);
    }

    if (SUCCEEDED(hr))
    {
        size_t cchFileNameLength = 0;

        hr = StringCchLengthW(pFileName, MAX_PATH, &cchFileNameLength);

        if (SUCCEEDED(hr) && (pFileName[cchFileNameLength - 1] != L'\\'))
        {
            hr = StringCchCatW(pFileName, MAX_PATH, L"\\");
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = StringCchCatW(pFileName, MAX_PATH, pDeviceDialogData->bstrFilename);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Add the extension. This will help if the application forgot the extension.
        //
        hr = StringCchCatW(pFileName, MAX_PATH, L".BMP");

        if (SUCCEEDED(hr))
        {
            if( !PathYetAnotherMakeUniqueName(pUniqueFileName, pFileName, NULL, NULL) )
            {
                hr = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // We dont have to release the stream. WIA service will release the stream after transfer.
        //
        hr = SHCreateStreamOnFileEx(pUniqueFileName, STGM_READWRITE, FILE_ATTRIBUTE_NORMAL, TRUE, 0, &pStream);
    }

    if (SUCCEEDED(hr))
    {
        CSimpleWIACallback * pCallback = NULL;

#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pTransferCallback is freed on release.")
        pCallback = new CSimpleWIACallback(pStream);

        if (pCallback)
        {
            IWiaTransferCallback * pTransferCallback = NULL;

            hr = pCallback->QueryInterface(IID_IWiaTransferCallback, (LPVOID*)&pTransferCallback);

            if (SUCCEEDED(hr))
            {
                hr = pWiaTransfer->Download(0, pTransferCallback);

                pTransferCallback ->Release();
            }

            pCallback->Release();
        }
    }

    //
    // Fill the Out Parameters
    //
    if (SUCCEEDED(hr))
    {
        pDeviceDialogData->lNumFiles = 1;

        pWiaFlatbed->AddRef();
        pDeviceDialogData->pWiaItem = pWiaFlatbed;

        pDeviceDialogData->pbstrFilePaths = (BSTR *)CoTaskMemAlloc(sizeof(BSTR *));

        if (pDeviceDialogData->pbstrFilePaths)
        {
            *(pDeviceDialogData->pbstrFilePaths) = SysAllocString(pUniqueFileName);
        }
        else
        {
            hr = E_FAIL;
        }
    }


    if (SUCCEEDED(hr))
    {
        WCHAR bufDialog[MAX_PATH] = {0};
        WCHAR bufTitle[MAX_PATH]  = {0};

        if (LoadString(g_hInst, IDS_MESSAGEBOX_TRANSFERMESSAGE_DIALOG, bufDialog, ARRAYSIZE(bufDialog)) &&
            LoadString(g_hInst, IDS_MESSAGEBOX_WIAUIEXTENSION_TITLE, bufTitle, ARRAYSIZE(bufTitle)) &&
            SUCCEEDED(StringCchCatW(bufDialog, MAX_PATH, pUniqueFileName))
            )
        {
#pragma prefast(suppress:__WARNING_CONCATENATED_RESOURCE_STRING, "The concatenated string that does not come from localizable resources is a file name constructed at run time."
            MessageBoxW (NULL, bufDialog, bufTitle, 0 );
        }
    }

    if (pWiaTransfer)
    {
        pWiaTransfer->Release();
        pWiaTransfer = NULL;
    }

    if (pFileName)
    {
        LocalFree(pFileName);
        pFileName = NULL;
    }

    if (pUniqueFileName)
    {
        LocalFree(pUniqueFileName);
        pUniqueFileName = NULL;
    }

    return hr;
}

