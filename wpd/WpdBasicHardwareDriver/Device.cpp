#include "stdafx.h"
#include "Device.tmh"

#include "WpdBasicHardwareDriver_i.c"

STDMETHODIMP_(HRESULT)
CDevice::OnD0Entry(_In_ IWDFDevice* pDevice,
                        WDF_POWER_DEVICE_STATE previousState)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! Entry");

    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(previousState);

    HRESULT      hr      = S_OK;
    RS232Target* pTarget = NULL;

    if (m_pWpdBaseDriver != NULL)
    {
        pTarget = m_pWpdBaseDriver->GetRS232Target();
        if (pTarget != NULL)
        {
            hr = pTarget->Start();
        }
    }

    return hr;
}

STDMETHODIMP_(HRESULT)
CDevice::OnD0Exit(_In_ IWDFDevice* pDevice,
                       WDF_POWER_DEVICE_STATE newState)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! Entry");

    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(newState);

    HRESULT hr = S_OK;
    RS232Target* pTarget = NULL;

    if (m_pWpdBaseDriver != NULL)
    {
        pTarget = m_pWpdBaseDriver->GetRS232Target();
        if (pTarget != NULL)
        {
            hr = pTarget->Stop();
        }
    }
    return hr;
}

STDMETHODIMP_(VOID)
CDevice::OnSurpriseRemoval(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
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
CDevice::OnSelfManagedIoCleanup(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return;
}

STDMETHODIMP_(VOID)
CDevice::OnSelfManagedIoFlush(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoInit(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoSuspend(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoRestart(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnSelfManagedIoStop(_In_ IWDFDevice* pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
    return S_OK;
}

STDMETHODIMP_(HRESULT)
CDevice::OnPrepareHardware(_In_ IWDFDevice* pDevice)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! Entry");

    HRESULT hr = S_OK;

    if (m_pWpdBaseDriver != NULL)
    {
        hr = m_pWpdBaseDriver->Initialize(pDevice);
        CHECK_HR(hr, "Failed to Initialize the driver class");
    }

    // Initialize the WPD Class Extension. This will enable the appropriate WPD interface GUID,
    // as well as do any additional initialization (e.g. enabling Legacy Compatibility layers for those drivers
    // which requested support in their INF).
    if (hr == S_OK && m_pPortableDeviceClassExtension == NULL)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceClassExtension,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceClassExtension,
                              (VOID**)&m_pPortableDeviceClassExtension);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceClassExtension");

        if (hr == S_OK)
        {
            CComPtr<IPortableDeviceValues> pOptions;
            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**)&pOptions);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

            if (hr == S_OK)
            {
                CComPtr<IPortableDevicePropVariantCollection> pContentTypes;
                hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IPortableDevicePropVariantCollection,
                                (VOID**)&pContentTypes);
                CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");

                // Driver has no supported content types, add an empty list of supported content types to the options
                if (hr == S_OK)
                {
                    hr = pOptions->SetIPortableDevicePropVariantCollectionValue(WPD_CLASS_EXTENSION_OPTIONS_SUPPORTED_CONTENT_TYPES, pContentTypes);
                    CHECK_HR(hr, "Failed to set WPD_CLASS_EXTENSION_OPTIONS_SUPPORTED_CONTENT_TYPES");
                }

                if (hr == S_OK)
                {
                    // Initialize the PortableDeviceClassExtension 
                    hr = m_pPortableDeviceClassExtension->Initialize(pDevice, pOptions);
                    CHECK_HR(hr, "Failed to Initialize portable device class extension object");
                }
            }
        }

        if (hr == S_OK)
        {
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

            if (hr == S_OK && wszDeviceFriendlyName != NULL)
            {
                hr = UpdateDeviceFriendlyName(m_pPortableDeviceClassExtension, wszDeviceFriendlyName);
                CHECK_HR(hr, "Failed to update device's friendly name");
            }

            // Free the memory.
            CoTaskMemFree(wszDeviceFriendlyName);
            wszDeviceFriendlyName = NULL;
        }
    }
    return hr;
}

STDMETHODIMP_(HRESULT)
CDevice::OnReleaseHardware(_In_ IWDFDevice* pDevice)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! Entry");

    UNREFERENCED_PARAMETER(pDevice);
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

HRESULT CDevice::GetDeviceFriendlyName(
    _Outptr_result_maybenull_ LPWSTR*   pwszDeviceFriendlyName)
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
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // CoCreate a collection to store the WPD_COMMAND_OBJECT_PROPERTIES_GET command results.
    if (hr == S_OK)
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
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**)&pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection for results");
    }

    // Set the params
    if (hr == S_OK)
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, WPD_COMMAND_OBJECT_PROPERTIES_GET.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }

    if (hr == S_OK)
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, WPD_COMMAND_OBJECT_PROPERTIES_GET.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }

    if (hr == S_OK)
    {
        hr = pParams->SetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, WPD_DEVICE_OBJECT_ID);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID"));
    }

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_DEVICE_FRIENDLY_NAME);
        CHECK_HR(hr, ("Failed to add WPD_DEVICE_FRIENDLY_NAME to key collection"));
    }

    if (hr == S_OK)
    {
        hr = pParams->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS"));
    }

    // Make the call
    if (hr == S_OK)
    {
        hr = m_pWpdBaseDriver->DispatchWpdMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to dispatch message to get supported content types"));
    }

    // Get the results
    if (hr == S_OK)
    {
        hr = pResults->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, &pValues);
        CHECK_HR(hr, ("Failed to get WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));
    }

    if (hr == S_OK)
    {
        hr = pValues->GetStringValue(WPD_DEVICE_FRIENDLY_NAME, pwszDeviceFriendlyName);
        CHECK_HR(hr, ("Failed to get WPD_DEVICE_FRIENDLY_NAME"));
    }

    return hr;
}

HRESULT UpdateDeviceFriendlyName(
    _In_ IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_ LPCWSTR                         wszDeviceFriendlyName)
{
    HRESULT hr = S_OK;

    // If we were passed NULL parameters we have nothing to do, return S_OK.
    if ((pPortableDeviceClassExtension == NULL) ||
        (wszDeviceFriendlyName         == NULL))
    {
        return S_OK;
    }

    CComPtr<IPortableDeviceValues>  pParams;
    CComPtr<IPortableDeviceValues>  pResults;
    CComPtr<IPortableDeviceValues>  pValues;

    // Prepare to make a call to set the device information
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues for results");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pValues);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues for results");
    }

    // Get the information values to update and set them in WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES
    if (hr == S_OK)
    {
        hr = pValues->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, wszDeviceFriendlyName);
        CHECK_HR(hr, ("Failed to set WPD_DEVICE_FRIENDLY_NAME"));
    }

    // Set the params
    if (hr == S_OK)
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, WPD_COMMAND_CLASS_EXTENSION_WRITE_DEVICE_INFORMATION.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }
    if (hr == S_OK)
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, WPD_COMMAND_CLASS_EXTENSION_WRITE_DEVICE_INFORMATION.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }
    if (hr == S_OK)
    {
        hr = pParams->SetIPortableDeviceValuesValue(WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES, pValues);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES"));
    }

    // Make the call
    if (hr == S_OK)
    {
        hr = pPortableDeviceClassExtension->ProcessLibraryMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to process update device information message"));
    }

    // A Failed ProcessLibraryMessage operation for updating this value is not considered
    // fatal and should return S_OK.

    return S_OK;
}
