//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//

#include "stdafx.h"
#include "common.h"



volatile long CDMFTModuleLifeTimeManager::s_lObjectCount = 0;
HRESULT CComDMFTCreateInstance(_In_ REFIID riid, _Outptr_ VOID** ppUnknown);
HRESULT RegisterObject(HMODULE hModule, REFGUID guid, PCWSTR pszDescription, PCWSTR pszThreadingModel);
HRESULT UnregisterObject(const GUID& guid);

long g_cRefModule = 0;
HMODULE g_hModule = NULL;
HMODULE CLoader::g_lModule = NULL; // SensorGroup Loader


TRACELOGGING_DEFINE_PROVIDER(
    g_hSensorDeviceMFTProvider,
    "SensorGroupDeviceTransform",
    (0xf4001f31, 0xdcfc, 0x45cc, 0xb9, 0xe5, 0xd4, 0xae, 0x9e, 0x24, 0xfd, 0xc3));

void DllAddRef()
{
    InterlockedIncrement(&g_cRefModule);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRefModule);
}
//////////////////////////////////////////////////////////////////////////
// IClassFactory implementation
//////////////////////////////////////////////////////////////////////////

typedef HRESULT(*PFNCREATEINSTANCE)(REFIID riid, VOID **ppvObject);
struct CLASS_OBJECT_INIT
{
    const CLSID *pClsid;
    PFNCREATEINSTANCE pfnCreate;
};
// Classes supported by this module:
const CLASS_OBJECT_INIT c_rgClassObjectInit[] =
{
    { &CLSID_SensorTransformDeviceMFT, CComDMFTCreateInstance },
};

class CClassFactory : public IClassFactory
{
public:

    static HRESULT CreateInstance(
        REFCLSID clsid,                                 // The CLSID of the object to create (from DllGetClassObject)
        const CLASS_OBJECT_INIT *pClassObjectInits,     // Array of class factory data.
        size_t cClassObjectInits,                       // Number of elements in the array.
        REFIID riid,                                    // The IID of the interface to retrieve (from DllGetClassObject)
        void **ppv                                      // Receives a pointer to the interface.
    )
    {
        HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
        SDMFTCHECKNULL_GOTO(ppv, done, E_INVALIDARG);
        *ppv = NULL;
        for (size_t i = 0; i < cClassObjectInits; i++)
        {
            if (IsEqualCLSID(clsid,*pClassObjectInits[i].pClsid))
            {
                ComPtr<IClassFactory> spClassFactory = new (std::nothrow) CClassFactory(pClassObjectInits[i].pfnCreate);
                SDMFTCHECKNULL_GOTO(spClassFactory.Get(), done, E_OUTOFMEMORY);
                SDMFTCHECKHR_GOTO(spClassFactory.Get()->QueryInterface(riid, ppv),done);
                break; // match found
            }
        }
    done:
        return hr;
    }
    //jitesh 
    IFACEMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        HRESULT hr = S_OK;
        SDMFTCHECKNULL_GOTO(ppv, done, E_INVALIDARG);
        if (riid == __uuidof(IClassFactory))
        {
            *ppv = static_cast< IClassFactory* >(this);
            (VOID)AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    done:
        return hr;
    }
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }
    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }
    // IClassFactory methods
    IFACEMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
    {
        return punkOuter ? CLASS_E_NOAGGREGATION : m_pfnCreate(riid, ppv);
    }
    IFACEMETHODIMP LockServer(BOOL fLock)
    {
        if (fLock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

private:
    CClassFactory(PFNCREATEINSTANCE pfnCreate) : m_cRef(1), m_pfnCreate(pfnCreate)
    {
        DllAddRef();
    }

    ~CClassFactory()
    {
        DllRelease();
    }
    LONG m_cRef;
    PFNCREATEINSTANCE m_pfnCreate;
};
//
// Standard DLL functions
//

STDMETHODIMP_(BOOL) WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, void *)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = (HMODULE)hInstance;
        DisableThreadLibraryCalls(hInstance);
        TraceLoggingRegister(g_hSensorDeviceMFTProvider);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TraceLoggingUnregister(g_hSensorDeviceMFTProvider);
    }
    return TRUE;
}

STDMETHODIMP DllCanUnloadNow()
{
    return  ((g_cRefModule == 0) && (CDMFTModuleLifeTimeManager::GetDMFTObjCount() == 0)) ? S_OK : S_FALSE;
}

_Check_return_
STDAPI  DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
    return CClassFactory::CreateInstance(clsid, c_rgClassObjectInit, ARRAYSIZE(c_rgClassObjectInit), riid, ppv);
}

STDMETHODIMP DllRegisterServer()
{
    return RegisterObject(g_hModule, CLSID_SensorTransformDeviceMFT, TEXT("Sensor Transform"), TEXT("Both"));
}

STDMETHODIMP DllUnregisterServer()
{
    return UnregisterObject(CLSID_SensorTransformDeviceMFT);
}


// Converts a CLSID into a string with the form "CLSID\{clsid}"
STDMETHODIMP CreateObjectKeyName(REFGUID guid, _Out_writes_(cchMax) PWSTR pszName, DWORD cchMax)
{
    HRESULT hr = S_OK;
    OLECHAR szCLSID[64];
    SDMFTCHECKHR_GOTO(StringFromGUID2(guid, szCLSID, 64), done);
    SDMFTCHECKHR_GOTO(StringCchPrintf(pszName, cchMax, TEXT("Software\\Classes\\CLSID\\%ls"), szCLSID),done);
done:
    return hr;
}

// Creates a registry key (if needed) and sets the default value of the key
STDMETHODIMP CreateRegKeyAndValue(HKEY hKey, PCWSTR pszSubKeyName, PCWSTR pszValueName,
    PCWSTR pszData, PHKEY phkResult)
{
    *phkResult = NULL;
    LONG lRet = RegCreateKeyExW(
        hKey, pszSubKeyName,
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, NULL, phkResult, NULL);

    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegSetValueExW(
            (*phkResult),
            pszValueName, 0, REG_SZ,
            (LPBYTE)pszData,
            ((DWORD)wcslen(pszData) + 1) * sizeof(WCHAR)
        );

        if (lRet != ERROR_SUCCESS)
        {
            RegCloseKey(*phkResult);
        }
    }

    return HRESULT_FROM_WIN32(lRet);
}

// Creates the registry entries for a COM object.
HRESULT RegisterObject(HMODULE hModule, REFGUID guid, PCWSTR pszDescription, PCWSTR pszThreadingModel)
{
    HKEY hKey = NULL;
    HKEY hSubkey = NULL;
    TCHAR achTemp[MAX_PATH];
    HRESULT hr = S_OK;

    // Create the name of the key from the object's CLSID
    SDMFTCHECKHR_GOTO(CreateObjectKeyName(guid, achTemp, MAX_PATH),done);
    // Create the new key.
    SDMFTCHECKHR_GOTO(CreateRegKeyAndValue(HKEY_LOCAL_MACHINE, achTemp, NULL, pszDescription, &hKey),done);
    (void)GetModuleFileName(hModule, achTemp, MAX_PATH);
    SDMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(GetLastError()),done);
    SDMFTCHECKHR_GOTO(CreateRegKeyAndValue(hKey, L"InProcServer32", NULL, achTemp, &hSubkey),done);
    SDMFTCHECKHR_GOTO(CreateRegKeyAndValue(hKey, L"InProcServer32", L"ThreadingModel", pszThreadingModel, &hSubkey),done);
done:
    SAFE_CLOSE(hKey);
    SAFE_CLOSE(hSubkey);
    return hr;
}


HRESULT UnregisterObject(const GUID& guid)
{
    HRESULT hr = S_OK;
    WCHAR achTemp[MAX_PATH];
    SDMFTCHECKHR_GOTO(CreateObjectKeyName(guid, achTemp, MAX_PATH),done);
    SDMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(RegDeleteTree(HKEY_LOCAL_MACHINE, achTemp)),done);
done:
    return hr;
}
