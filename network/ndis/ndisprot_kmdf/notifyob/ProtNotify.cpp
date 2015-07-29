/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ProtNotify.cpp

Abstract:

    This module contains the component that calls into the Wdf Installer to
    to load/unload the WDF loader during the device installation/removal.

--*/

#include "Common.hpp"

#ifdef _ATL_STATIC_REGISTRY
//
// Disable warnings generated in the standard headers
//
// Disable warning C4100: unreferenced formal parameter
// Disable warning C4189: local variable is initialized but not referenced
//
#pragma warning (disable:4100 4189)

#include <statreg.h>

#pragma warning (default:4100 4189)
#endif


//
// The CProtNotify class implements the interfaces necessary to monitor when
// the NdisProt driver is being installed or uninstalled so that the WDF library
// can be loaded or unloaded, respectively.
//
class CProtNotify : public CComObjectRoot,
                    public CComCoClass<CProtNotify, &CLSID_CProtNotify>,
                    public INetCfgComponentControl,
                    public INetCfgComponentSetup
{
private:

    INetCfg* _NetCfgObj;
    INetCfgComponent* _NetCfgComponentObj;

    //
    // The name of the Wdf Section in the inf file.
    //
    LPWSTR _WdfSectionName;

    //
    // The name of the original inf file.
    //
    LPWSTR _SourceInfFileName;

public:

    CProtNotify();
    ~CProtNotify();

    BEGIN_COM_MAP(CProtNotify)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(NDIS_NOTIFY_RESOURCE_ID2)

    //
    // INetCfgComponentControl methods.
    //

    STDMETHOD (Initialize)(
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling
        );

    STDMETHOD (CancelChanges)(
        );

    STDMETHOD (ApplyRegistryChanges)(
        );

    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback
        );

    //
    // INetCfgComponentSetup methods.
    //

    STDMETHOD (Install)(
        IN DWORD dwSetupFlags
        );

    STDMETHOD (Upgrade)(
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo
        );

    STDMETHOD (ReadAnswerFile)(
        IN PCWSTR szAnswerFile,
        IN PCWSTR szAnswerSections
        );

    STDMETHOD (Removing)(
        );

private:

    //
    // A routine to query the registry for the Inf file name and the Wdf Section
    // name.
    //
    STDMETHOD (QueryInfNameAndWdfSection)(
        );

};


//
// Define the Object Map array using the CProtNotify class.
//
BEGIN_OBJECT_MAP(ObjectMapArray)
    OBJECT_ENTRY(CLSID_CProtNotify, CProtNotify)
END_OBJECT_MAP()

ATL::_ATL_OBJMAP_ENTRY* ObjectMap = ObjectMapArray;


CProtNotify::CProtNotify()
{
    _NetCfgObj          = NULL;
    _NetCfgComponentObj = NULL;
    _SourceInfFileName  = NULL;
    _WdfSectionName     = NULL;
}


CProtNotify::~CProtNotify()
{
    if (_NetCfgObj != NULL) {
        _NetCfgObj->Release();
    }
    if (_NetCfgComponentObj != NULL) {
        _NetCfgComponentObj->Release();
    }
    if (_SourceInfFileName != NULL) {
        CoTaskMemFree(_SourceInfFileName);
    }
    if (_WdfSectionName != NULL) {
        CoTaskMemFree(_WdfSectionName);
    }
}


STDMETHODIMP CProtNotify::Initialize(
    IN INetCfgComponent* pIComp,
    IN INetCfg* pINetCfg,
    IN BOOL fInstalling
    )
{
    if (fInstalling) {

        _NetCfgObj = pINetCfg;
        _NetCfgComponentObj = pIComp;

        if (_NetCfgObj != NULL) {
            _NetCfgObj->AddRef();
        }
        if (_NetCfgComponentObj != NULL) {
            _NetCfgComponentObj->AddRef();
        }
    }

    return S_OK;
}


STDMETHODIMP CProtNotify::CancelChanges(
    )
{
    return S_OK;
}


STDMETHODIMP CProtNotify::ApplyRegistryChanges(
    )
{
    return S_OK;
}


STDMETHODIMP CProtNotify::ApplyPnpChanges(
    IN INetCfgPnpReconfigCallback* pICallback
    )
{
    UNREFERENCED_PARAMETER(pICallback);

    return S_OK;
}


STDMETHODIMP CProtNotify::Install(
    IN DWORD dwSetupFlags
    )
{
    HRESULT hr = S_OK;
    ULONG winError;

    UNREFERENCED_PARAMETER(dwSetupFlags);

    //
    // Query the Source Inf File name and the Wdf Section name if it hasnt
    // been already queried.
    //
    hr = QueryInfNameAndWdfSection();
    if (SUCCEEDED(hr)) {

        winError = (pfnWdfPreDeviceInstall)(_SourceInfFileName, _WdfSectionName);
        if (winError == ERROR_SUCCESS) {
            winError = (pfnWdfPostDeviceInstall)(_SourceInfFileName, _WdfSectionName);
        }

        hr = HRESULT_FROM_WIN32(winError);
    }

    return hr;
}


STDMETHODIMP CProtNotify::Upgrade(
    IN DWORD dwSetupFlags,
    IN DWORD dwUpgradeFromBuildNo
    )
{
    UNREFERENCED_PARAMETER(dwSetupFlags);
    UNREFERENCED_PARAMETER(dwUpgradeFromBuildNo);

    return S_OK;
}


STDMETHODIMP CProtNotify::ReadAnswerFile(
    IN PCWSTR szAnswerFile,
    IN PCWSTR szAnswerSections
    )
{
    UNREFERENCED_PARAMETER(szAnswerFile);
    UNREFERENCED_PARAMETER(szAnswerSections);

    return S_OK;
}


STDMETHODIMP CProtNotify::Removing(
    )
{
    HRESULT hr;
    ULONG winError;

    //
    // Call the Wdf Pre device removal routine.
    //
    winError = (pfnWdfPreDeviceRemove)(_SourceInfFileName, _WdfSectionName);

    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Call the Wdf Post device removal routine.
    //
    winError = (pfnWdfPostDeviceRemove)(_SourceInfFileName, _WdfSectionName);

    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}


STDMETHODIMP CProtNotify::QueryInfNameAndWdfSection(
    )
{
    HRESULT hr = S_OK;
    ULONG winError = ERROR_SUCCESS;
    HKEY paramKey = NULL;
    DWORD length = 0;

    //
    // Query the source inf file name and wdf section name from the registry
    // only if it has not been successfully queried before.
    //
    if ((_SourceInfFileName != NULL) || (_WdfSectionName != NULL)) {
        goto exit;
    }

    //
    // Get the handle to the parameters key for the component.
    //
    hr = _NetCfgComponentObj->OpenParamKey(&paramKey);
    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Flush the key before querying values off of it.
    //
    hr = RegFlushKey(paramKey);
    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Query the number of bytes required to copy the SourceInfFile data.
    //
    winError = RegQueryValueEx(paramKey,
                               L"SourceInfFile",
                               NULL,
                               NULL,
                               NULL,
                               &length);
    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Allocate memory to hold the SourceInfFile data.
    //
    _SourceInfFileName = (LPWSTR)CoTaskMemAlloc(length);
    if (_SourceInfFileName == NULL) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    //
    // Query the SourceInfFile data.
    //
    winError = RegQueryValueEx(paramKey,
                               L"SourceInfFile",
                               NULL,
                               NULL,
                               (LPBYTE)_SourceInfFileName,
                               &length);
    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Query the number of bytes required to copy the WdfSection data.
    //
    length = 0;
    winError = RegQueryValueEx(paramKey,
                               L"WdfSection",
                               NULL,
                               NULL,
                               NULL,
                               &length);
    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {

        //
        // Its ok if the WdfSection value is not defined. This means that the
        // WDF Coinstaller will by default look for "WdfSection" in the inf
        // file.
        //
        hr = S_OK;
        goto exit;
    }

    //
    // Allocate memory to hold the WdfSection data.
    //
    _WdfSectionName = (LPWSTR)CoTaskMemAlloc(length);
    if (_WdfSectionName == NULL) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    //
    // Query the SourceInfFile data.
    //
    winError = RegQueryValueEx(paramKey,
                               L"WdfSection",
                               NULL,
                               NULL,
                               (LPBYTE)_WdfSectionName,
                               &length);
    hr = HRESULT_FROM_WIN32(winError);
    if (FAILED(hr)) {
        goto exit;
    }

exit:

    if (paramKey != NULL) {
        RegCloseKey(paramKey);
    }

    if (FAILED(hr)) {
        if (_SourceInfFileName != NULL) {
            CoTaskMemFree(_SourceInfFileName);
            _SourceInfFileName = NULL;
        }
        if (_WdfSectionName != NULL) {
            CoTaskMemFree(_WdfSectionName);
            _WdfSectionName = NULL;
        }
    }

    return hr;
}


