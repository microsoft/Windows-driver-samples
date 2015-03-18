#include "stdafx.h"

#include "helpers.tmh"

HRESULT UpdateDeviceFriendlyName(
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_    LPCWSTR                         wszDeviceFriendlyName)
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

HRESULT RegisterServices(
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
            const bool                      bUnregister)
{
    // If we were passed NULL parameters we have nothing to do, return S_OK.
    if (pPortableDeviceClassExtension == NULL)
    {
        return S_OK;
    }

    CComPtr<IPortableDeviceValues>                 pParams;
    CComPtr<IPortableDeviceValues>                 pResults;
    CComPtr<IPortableDevicePropVariantCollection>  pInterfaces;

    PROPERTYKEY commandToUse = bUnregister?
                                 WPD_COMMAND_CLASS_EXTENSION_UNREGISTER_SERVICE_INTERFACES:
                                 WPD_COMMAND_CLASS_EXTENSION_REGISTER_SERVICE_INTERFACES;

    // Prepare to make a call to register the services
    HRESULT hr = CoCreateInstance(CLSID_PortableDeviceValues,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDeviceValues,
                          (VOID**)&pParams);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

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
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**)&pInterfaces);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection for interfaces");
    }

    // Get the interfaces values to register and set them in WPD_PROPERTY_CLASS_EXTENSION_SERVICE_INTERFACES
    if (hr == S_OK)
    {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        pv.vt = VT_CLSID;

        pv.puuid = (CLSID*)&SERVICE_FullEnumSync;
        hr = pInterfaces->Add(&pv);
        CHECK_HR(hr, "Failed to add EnumerationSyncService to the list of requested interfaces");

        pv.puuid = (CLSID*)&SERVICE_Contacts;
        hr = pInterfaces->Add(&pv);
        CHECK_HR(hr, "Failed to add ContactsSyncService to the list of requested interfaces");

        // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs
    }

    // Set the params
    if (hr == S_OK)
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, commandToUse.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }
    if (hr == S_OK)
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, commandToUse.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }

    if (hr == S_OK)
    {
        hr = pParams->SetStringValue(WPD_PROPERTY_CLASS_EXTENSION_SERVICE_OBJECT_ID, CONTACTS_SERVICE_OBJECT_ID);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_CLASS_EXTENSION_SERVICE_OBJECT_ID"));
    }

    if (hr == S_OK)
    {
        hr = pParams->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_CLASS_EXTENSION_SERVICE_INTERFACES, pInterfaces);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_CLASS_EXTENSION_SERVICE_INTERFACES"));
    }

    // Make the call
    if (hr == S_OK)
    {
        hr = pPortableDeviceClassExtension->ProcessLibraryMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to process update device information message"));
    }

    return hr;
}

DWORD GetResourceSize(
    const UINT uiResource)
{
    HRESULT hr             = S_OK;
    LONG    lError         = ERROR_SUCCESS;
    DWORD   dwResourceSize = 0;

    HRSRC hResource = FindResource(g_hInstance, MAKEINTRESOURCE(uiResource), TEXT("DATA_FILE"));
    if (hResource)
    {
        HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
        if (hGlobal)
        {
            dwResourceSize = SizeofResource(g_hInstance, hResource);
        }
        else
        {
            lError = GetLastError();
            hr = HRESULT_FROM_WIN32(lError);
        }
    }
    else
    {
        lError = GetLastError();
        hr = HRESULT_FROM_WIN32(lError);
    }

    if (FAILED(hr))
    {
        CHECK_HR(hr, "Failed to get resource size for '%d'", uiResource);
    }

    return dwResourceSize;
}

PBYTE GetResourceData(
    const UINT uiResource)
{
    HRESULT hr     = S_OK;
    LONG    lError = ERROR_SUCCESS;
    PBYTE   pData  = NULL;

    HRSRC hResource = FindResource(g_hInstance, MAKEINTRESOURCE(uiResource), TEXT("DATA_FILE"));
    if (hResource)
    {
        HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
        if (hGlobal)
        {
            pData = static_cast<BYTE*>(LockResource(hGlobal));
        }
        else
        {
            lError = GetLastError();
            hr = HRESULT_FROM_WIN32(lError);
        }
    }
    else
    {
        lError = GetLastError();
        hr = HRESULT_FROM_WIN32(lError);
    }

    if (FAILED(hr))
    {
        CHECK_HR(hr, "Failed to get resource data pointer for '%d'", uiResource);
    }

    return pData;
}


HRESULT AddStringValueToPropVariantCollection(
    _In_    IPortableDevicePropVariantCollection* pCollection,
    _In_    LPCWSTR                               wszValue)
{
    HRESULT hr = S_OK;

    if ((pCollection == NULL) ||
        (wszValue    == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    PROPVARIANT pv = {0};
    PropVariantInit(&pv);

    pv.vt      = VT_LPWSTR;
    pv.pwszVal = (LPWSTR)wszValue;

    // The wszValue will be copied into the collection, keeping the ownership
    // of the string belonging to the caller.
    // Don't call PropVariantClear, since we did not allocate the memory for these string values

    hr = pCollection->Add(&pv);

    return hr;
}

HRESULT GetClientContextMap(
    _In_     IPortableDeviceValues*  pParams,
    _Outptr_ ContextMap**            ppContextMap)
{
    HRESULT hr = S_OK;

    if(ppContextMap == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**) ppContextMap);
    CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");

    return hr;
}

HRESULT GetClientContext(
    _In_         IPortableDeviceValues*  pParams,
    _In_         LPCWSTR                 pszContextKey,
    _COM_Outptr_ IUnknown**              ppContext)
{
    HRESULT      hr             = S_OK;
    ContextMap*  pContextMap    = NULL;

    if(ppContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    *ppContext = NULL;

    hr = GetClientContextMap(pParams, &pContextMap);
    CHECK_HR(hr, ("Failed to get the client context map"));

    if (SUCCEEDED(hr) && pContextMap == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Client context map is NULL"));
    }

    if (SUCCEEDED(hr))
    {
        *ppContext = pContextMap->GetContext(pszContextKey);
        if(*ppContext == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_HR(hr, "Failed to find context %ws for this client", pszContextKey);
        }
    }

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT GetClientEventCookie(
    _In_                      IPortableDeviceValues*  pParams,
    _Outptr_result_maybenull_ LPWSTR*                 ppszEventCookie)
{
    HRESULT        hr               = S_OK;
    LPWSTR         pszClientContext = NULL;
    ClientContext* pClientContext   = NULL;

    if ((pParams         == NULL) ||
        (ppszEventCookie == NULL))
    {
        return E_POINTER;
    }

    *ppszEventCookie = NULL;

    hr = pParams->GetStringValue(WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT, &pszClientContext);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT");

    if (SUCCEEDED(hr))
    {
        // Get the client context for this request.
        hr = GetClientContext(pParams, pszClientContext, (IUnknown**)&pClientContext);
        CHECK_HR(hr, "Failed to get the client context");
    }

    if (SUCCEEDED(hr) && (pClientContext->EventCookie.GetLength() > 0))
    {
        // Get the event cookie only if it has been set
        *ppszEventCookie = AtlAllocTaskWideString(pClientContext->EventCookie);
        if (*ppszEventCookie == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate the client event cookie");
        }
    }

    // We're done with the context
    SAFE_RELEASE(pClientContext);

    CoTaskMemFree(pszClientContext);
    pszClientContext = NULL;

    return hr;
}


HRESULT PostWpdEvent(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams)
{
    HRESULT hr             = S_OK;
    BYTE*   pBuffer        = NULL;
    DWORD   cbBuffer       = 0;
    LPWSTR  pszEventCookie = NULL;

    CComPtr<IWDFDevice>     pDevice;
    CComPtr<IWpdSerializer> pSerializer;

    // Get the WUDF Device Object
    hr = pCommandParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, (IUnknown**) &pDevice);
    CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT");

    // Get the WpdSerializer Object
    if (hr == S_OK)
    {
        hr = pCommandParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, (IUnknown**) &pSerializer);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT");
    }

    if (hr == S_OK)
    {
        // Set the client event cookie if available.  This is benign, as some clients may not provide a cookie.
        HRESULT hrEventCookie = GetClientEventCookie(pCommandParams, &pszEventCookie);
        if ((hrEventCookie == S_OK) && (pszEventCookie != NULL))
        {
            hrEventCookie = pEventParams->SetStringValue(WPD_CLIENT_EVENT_COOKIE, pszEventCookie);
            CHECK_HR(hrEventCookie, "Failed to set WPD_CLIENT_EVENT_COOKIE (error ignored)");
        }
    }

    if (hr == S_OK)
    {
        // Create a buffer with the serialized parameters
        hr = pSerializer->GetBufferFromIPortableDeviceValues(pEventParams, &pBuffer, &cbBuffer);
        CHECK_HR(hr, "Failed to get buffer from IPortableDeviceValues");
    }

    // Send the event
    if (hr == S_OK && pBuffer != NULL)
    {
        hr = pDevice->PostEvent(WPD_EVENT_NOTIFICATION, WdfEventBroadcast, pBuffer, cbBuffer);
        CHECK_HR(hr, "Failed to post WPD (broadcast) event");
    }

    // Free the memory
    CoTaskMemFree(pBuffer);
    pBuffer = NULL;

    CoTaskMemFree(pszEventCookie);
    pszEventCookie = NULL;

    return hr;
}

HRESULT AddPropertyAttributesByType(
            const FakeDevicePropertyAttributesType type,
    _In_    IPortableDeviceValues*           pAttributes)
{
    HRESULT hr = S_OK;
    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (type == UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast)
    {
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, TRUE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_DELETE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_DELETE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_READ, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_READ");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_UNSPECIFIED);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM");
        }
        else
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
        }
    }
    else
    {
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, FALSE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_DELETE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_DELETE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_READ, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_READ");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_UNSPECIFIED);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM");
        }

    }

    return hr;
}

#define WPD_PROPERTY_ATTRIBUTE_MAX_SIZE_VALUE 1024
HRESULT SetPropertyAttributes(
    _In_                        REFPROPERTYKEY                  Key,
    _In_reads_(cAttributeInfo)  const PropertyAttributeInfo*    AttributeInfo,
    _In_                        DWORD                           cAttributeInfo,
    _In_                        IPortableDeviceValues*          pAttributes)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex=0; dwIndex<cAttributeInfo; dwIndex++)
    {
        if (IsEqualPropertyKey(Key, *(AttributeInfo[dwIndex].pKey)))
        {
            // Set vartype
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_VARTYPE, AttributeInfo[dwIndex].Vartype);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_VARTYPE");

            // Set name
            if (hr == S_OK && AttributeInfo[dwIndex].wszName != NULL)
            {
                hr = pAttributes->SetStringValue(WPD_PROPERTY_ATTRIBUTE_NAME, AttributeInfo[dwIndex].wszName);
                CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_NAME");
            }

            // Set max size for string properties
            if (hr == S_OK && AttributeInfo[dwIndex].Vartype == VT_LPWSTR)
            {
                hr = pAttributes->SetUnsignedLargeIntegerValue(WPD_PROPERTY_ATTRIBUTE_MAX_SIZE, WPD_PROPERTY_ATTRIBUTE_MAX_SIZE_VALUE);
                CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_MAX_SIZE");
            }

            // Set access attributes
            if (hr == S_OK)
            {
                hr = AddPropertyAttributesByType(AttributeInfo[dwIndex].AttributesType, pAttributes);
                CHECK_HR(hr, "Failed to set common property attributes");
            }

            break;
        }
    }

    return hr;
}

HRESULT SetMethodParameterAttributes(
    _In_                        REFPROPERTYKEY                      Parameter,
    _In_reads_(cAttributeInfo)  const MethodParameterAttributeInfo* AttributeInfo,
    _In_                        DWORD                               cAttributeInfo,
    _In_                        IPortableDeviceValues*              pAttributes)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex=0; dwIndex<cAttributeInfo; dwIndex++)
    {
        if (IsEqualPropertyKey(Parameter, *AttributeInfo[dwIndex].pKey))
        {
            // Set vartype
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PARAMETER_ATTRIBUTE_VARTYPE, AttributeInfo[dwIndex].Vartype);
            CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_VARTYPE");

            // Set form
            if (hr == S_OK)
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_PARAMETER_ATTRIBUTE_FORM, (DWORD)AttributeInfo[dwIndex].Form);
                CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_FORM");
            }

            // Set order
            if (hr == S_OK)
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_PARAMETER_ATTRIBUTE_ORDER, AttributeInfo[dwIndex].Order);
                CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_ORDER");
            }

            // Set usage
            if (hr == S_OK)
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_PARAMETER_ATTRIBUTE_USAGE, AttributeInfo[dwIndex].UsageType);
                CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_USAGE");
            }

            // Set name
            if (hr == S_OK)
            {
                hr = pAttributes->SetStringValue(WPD_PARAMETER_ATTRIBUTE_NAME, AttributeInfo[dwIndex].wszName);
                CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_NAME");
            }

            break;
        }
    }

    return hr;
}

HRESULT SetEventParameterAttributes(
    _In_                        REFPROPERTYKEY                      Parameter,
    _In_reads_(cAttributeInfo)  const EventParameterAttributeInfo*  AttributeInfo,
    _In_                        DWORD                               cAttributeInfo,
    _In_                        IPortableDeviceValues*              pAttributes)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex=0; dwIndex<cAttributeInfo; dwIndex++)
    {
        if (IsEqualPropertyKey(Parameter, *AttributeInfo[dwIndex].pParameter))
        {
            // Set vartype
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PARAMETER_ATTRIBUTE_VARTYPE, AttributeInfo[dwIndex].Vartype);
            CHECK_HR(hr, "Failed to set WPD_PARAMETER_ATTRIBUTE_VARTYPE");
            break;
        }
    }

    return hr;

}

HRESULT SetEventParameters(
    _In_                        REFGUID                             Event,
    _In_reads_(cAttributeInfo)  const EventParameterAttributeInfo*  AttributeInfo,
    _In_                        DWORD                               cAttributeInfo,
    _In_                        IPortableDeviceKeyCollection*       pParameters)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    if (pParameters == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex=0; dwIndex<cAttributeInfo; dwIndex++)
    {
        GUID guidEvent = *AttributeInfo[dwIndex].pEventGuid;
        PROPERTYKEY param = *AttributeInfo[dwIndex].pParameter;

        if (guidEvent == Event)
        {
            hr = pParameters->Add(param);
            CHECK_HR(hr, "Failed to add event parameter to collection");
        }
    }

    return hr;
}
