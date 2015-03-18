#include "stdafx.h"
#include "Device.h"
#include "WpdServiceSampleDriver_i.c"

#include "Device.tmh"

STDMETHODIMP_(HRESULT)
CDevice::OnD0Entry(_In_ IWDFDevice* /*pDevice*/, WDF_POWER_DEVICE_STATE /*previousState*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnD0Exit(_In_ IWDFDevice* /*pDevice*/, WDF_POWER_DEVICE_STATE /*newState*/)
{
    return S_OK;
}

STDMETHODIMP_(VOID)
CDevice::OnSurpriseRemoval(_In_ IWDFDevice* /*pDevice*/)
{
    return;
}

STDMETHODIMP_(HRESULT)
CDevice::OnQueryRemove(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnQueryStop(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(VOID)
CDevice::OnSelfManagedIoCleanup(_In_ IWDFDevice* /*pDevice*/)
{
    return;
}

STDMETHODIMP_(VOID)
CDevice::OnSelfManagedIoFlush(_In_ IWDFDevice* /*pDevice*/)
{
    return;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoInit(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoSuspend(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoRestart(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoStop(_In_ IWDFDevice* /*pDevice*/)
{
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnPrepareHardware(_In_ IWDFDevice* pDevice)
{
    HRESULT hr = S_OK;

    if (m_pWpdBaseDriver != NULL)
    {
        hr = m_pWpdBaseDriver->Initialize();
        CHECK_HR(hr, "Failed to Initialize the driver class");
    }

    // Initialize the WPD Class Extension. This will enable the appropriate WPD interface GUID,
    // as well as do any additional initialization (e.g. enabling Legacy Compatibility layers for those drivers
    // which requested support in their INF).
    if (hr == S_OK && m_pPortableDeviceClassExtension == NULL)
    {
        CComPtr<IPortableDeviceValues>                pOptions;
        CComPtr<IPortableDevicePropVariantCollection> pContentTypes;

        hr = CoCreateInstance(CLSID_PortableDeviceClassExtension,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceClassExtension,
                              (VOID**)&m_pPortableDeviceClassExtension);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceClassExtension");

        if (hr == S_OK)
        {
            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**)&pOptions);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

            if (hr == S_OK)
            {
                // Get supported content types
                if (hr == S_OK)
                {
                    hr = GetSupportedContentTypes(&pContentTypes);
                    CHECK_HR(hr, "Failed to get supported content types");
                }

                // Add the supported types to the options
                if (hr == S_OK)
                {
                    hr = pOptions->SetIPortableDevicePropVariantCollectionValue(WPD_CLASS_EXTENSION_OPTIONS_SUPPORTED_CONTENT_TYPES, pContentTypes);
                    CHECK_HR(hr, "Failed to set WPD_CLASS_EXTENSION_OPTIONS_SUPPORTED_CONTENT_TYPES");
                }

                // Initialize the PortableDeviceClassExtension with a list of supported content types for the
                // connected device.  This will ensure that the correct application compatibility settings will
                // be applied for your device.
                if (hr == S_OK)
                {
                    hr = m_pPortableDeviceClassExtension->Initialize(pDevice, pOptions);
                    CHECK_HR(hr, "Failed to Initialize portable device class extension object");
                }

                // Register the services as Plug and Play interfaces
                if (hr == S_OK)
                {
                    hr = RegisterServices(m_pPortableDeviceClassExtension, false /*bUnregister*/);
                    CHECK_HR(hr, "Failed to register services");
                }

                // Since users commonly have the abiltity to customize their device even when it is not
                // connected to the PC, we need to make sure the PC is current when the driver loads.
                //
                // Send the latest device friendly name to the PortableDeviceClassExtension component
                // so the system is always updated with the current device name.
                //
                // This call should also be made after a successful property set operation of
                // WPD_DEVICE_FRIENDLY_NAME.

                LPWSTR wszDeviceFriendlyName = NULL;

                if (hr == S_OK)
                {
                    hr = GetDeviceFriendlyName(&wszDeviceFriendlyName);
                    CHECK_HR(hr, "Failed to get device's friendly name");
                }

                if (hr == S_OK)
                {
                    hr = UpdateDeviceFriendlyName(m_pPortableDeviceClassExtension, wszDeviceFriendlyName);
                    CHECK_HR(hr, "Failed to update device's friendly name");
                }

                // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                CoTaskMemFree(wszDeviceFriendlyName);
            }
        }
    }

    return hr;
}

STDMETHODIMP_(HRESULT)
CDevice::OnReleaseHardware(_In_ IWDFDevice* /*pDevice*/)
{
    // Unregister the services as Plug and Play interfaces (errors are ignored).
    HRESULT hr = RegisterServices(m_pPortableDeviceClassExtension, true /*bUnregister*/);
    CHECK_HR(hr, "Failed to unregister services");

    if (m_pWpdBaseDriver != NULL)
    {
        m_pWpdBaseDriver->Uninitialize();
    }

    if (m_pPortableDeviceClassExtension != NULL)
    {
        m_pPortableDeviceClassExtension = NULL;
    }
    
    return S_OK;
}

HRESULT CDevice::GetSupportedContentTypes(
    _Outptr_ IPortableDevicePropVariantCollection** ppContentTypes)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pParams;
    CComPtr<IPortableDeviceValues> pResults;

    if (ppContentTypes == NULL)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    *ppContentTypes = NULL;

    // CoCreate a collection to store the WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES command parameters.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // CoCreate a collection to store the WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES command results.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // Set the params
    if (SUCCEEDED(hr))
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }
    if (SUCCEEDED(hr))
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }
    if (SUCCEEDED(hr))
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY, WPD_FUNCTIONAL_CATEGORY_ALL);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY"));
    }

    // Make the call
    if (SUCCEEDED(hr))
    {
        hr = m_pWpdBaseDriver->DispatchWpdMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to dispatch message to get supported content types"));
    }

    // Get the results
    if (SUCCEEDED(hr))
    {
        hr = pResults->GetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES, ppContentTypes);
        CHECK_HR(hr, ("Failed to get WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES"));
    }

    return hr;
}

HRESULT CDevice::GetDeviceFriendlyName(
    _Outptr_ LPWSTR*   pwszDeviceFriendlyName)
{
    HRESULT hr = S_OK;

    CComPtr<IPortableDeviceValues>        pParams;
    CComPtr<IPortableDeviceValues>        pResults;
    CComPtr<IPortableDeviceKeyCollection> pKeys;
    CComPtr<IPortableDeviceValues>        pValues;

    if (pwszDeviceFriendlyName == NULL)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    *pwszDeviceFriendlyName = NULL;

    // CoCreate a collection to store the WPD_COMMAND_OBJECT_PROPERTIES_GET command parameters.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // CoCreate a collection to store the WPD_COMMAND_OBJECT_PROPERTIES_GET command results.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // CoCreate a collection to store the requested property keys.  In our case, we are requesting just the device friendly name
    // (WPD_DEVICE_FRIENDLY_NAME)
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**)&pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection for results");
    }

    // Set the params
    if (SUCCEEDED(hr))
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, WPD_COMMAND_OBJECT_PROPERTIES_GET.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, WPD_COMMAND_OBJECT_PROPERTIES_GET.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->SetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, WPD_DEVICE_OBJECT_ID);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID"));
    }

    if (SUCCEEDED(hr))
    {
        hr = pKeys->Add(WPD_DEVICE_FRIENDLY_NAME);
        CHECK_HR(hr, ("Failed to add WPD_DEVICE_FRIENDLY_NAME to key collection"));
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS"));
    }

    // Make the call
    if (SUCCEEDED(hr))
    {
        hr = m_pWpdBaseDriver->DispatchWpdMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to dispatch message to get supported content types"));
    }

    // Get the results
    if (SUCCEEDED(hr))
    {
        hr = pResults->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, &pValues);
        CHECK_HR(hr, ("Failed to get WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));
    }

    if (SUCCEEDED(hr))
    {
        hr = pValues->GetStringValue(WPD_DEVICE_FRIENDLY_NAME, pwszDeviceFriendlyName);
        CHECK_HR(hr, ("Failed to get WPD_DEVICE_FRIENDLY_NAME"));
    }

    return hr;
}
