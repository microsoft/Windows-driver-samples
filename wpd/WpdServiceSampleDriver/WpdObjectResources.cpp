#include "stdafx.h"

#include "WpdObjectResources.tmh"

WpdObjectResources::WpdObjectResources() : m_pDevice(NULL)
{

}

WpdObjectResources::~WpdObjectResources()
{

}

HRESULT WpdObjectResources::Initialize(_In_ FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pDevice = pDevice;
    return S_OK;
}

HRESULT WpdObjectResources::DispatchWpdMessage(
    _In_     REFPROPERTYKEY         Command,
    _In_     IPortableDeviceValues* pParams,
    _In_     IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_OBJECT_RESOURCES)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_GET_SUPPORTED))
        {
            hr = OnGetSupportedResources(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported resources");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_OPEN))
        {
            hr = OnOpenResource(pParams, pResults);
            CHECK_HR(hr, "Failed to open resource");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_READ))
        {
            hr = OnReadResource(pParams, pResults);
            CHECK_HR(hr, "Failed to read resource");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_CLOSE))
        {
            hr = OnCloseResource(pParams, pResults);
            CHECK_HR(hr, "Failed to close resource");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES))
        {
            hr = OnGetResourceAttributes(pParams, pResults);
            CHECK_HR(hr, "Failed to get resource attributes");
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "This object does not support this command id %d", Command.pid);
        }
    }
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_GET_SUPPORTED
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: identifies the object whose resources we want to return.
 *
 *  The driver should:
 *  - Return all resources for this object in WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS.
 */
HRESULT WpdObjectResources::OnGetSupportedResources(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr           = S_OK;
    LPWSTR  wszObjectID  = NULL;

    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &wszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    // Create the collection to hold the resource keys
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    if (hr == S_OK)
    {
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->GetSupportedResources(Scope, wszObjectID, pKeys);
        CHECK_HR(hr, "Failed to get supported resources for object '%ws'", wszObjectID);
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: identifies the object whose resource attributes we want to return.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS: a collection of property keys containing a single value,
 *      which is the key identifying the specific resource we are requested to return attributes for.
 *
 *  The driver should:
 *  - Return the requested property attributes in WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES.
 *    If any resource attributes failed to be retrieved,
 *    the corresponding value should be set to type VT_ERROR with the 'scode' member holding the
 *    HRESULT reason for the failure.
 *  - S_OK should be returned if all resource attributes were read successfully.
 *  - S_FALSE should be returned if any resource attribute failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any property values.
 *
 */
HRESULT WpdObjectResources::OnGetResourceAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr          = S_OK;
    LPWSTR      wszObjectID = NULL;
    PROPERTYKEY Key         = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues>  pAttributes;

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->GetResourceAttributes(Scope, wszObjectID, Key, pAttributes);
        CHECK_HR(hr, "Failed to get resource attributes");
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;

        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES, pAttributes);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES"));

        if(FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_OPEN
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: the object identifier of the
 *      object which contains the specified resource
 *
 *  - WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS: the specified resource
 *      to open
 *
 *  - WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE: the access mode to which to
 *      open the specified resource
 *
 *  The driver should:
 *  - Create a new context for this resource operation.
 *  - Return an identifier for the context in WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT.
 *  - Set the optimal transfer size in WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE
 *
 */
HRESULT WpdObjectResources::OnOpenResource(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr              = S_OK;
    LPWSTR      wszObjectID     = NULL;
    PROPERTYKEY Key             = WPD_PROPERTY_NULL;
    DWORD       dwMode          = STGM_READ;
    CAtlStringW strStrObjectID;
    CAtlStringW strResourceContext;
    ContextMap* pContextMap     = NULL;

    // Get the Object identifier of the object which contains the specified resource
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &wszObjectID);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    // Get the resource key
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    // Get the access mode
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE, &dwMode);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE");
    }

    // Get the context map which the driver stored in pParams for convenience
    if (SUCCEEDED(hr))
    {
        hr = GetClientContextMap(pParams, &pContextMap);
        CHECK_HR(hr, "Failed to get the client context map");
    }

    // Create a new resource operation context, initialize it, and add it to the client context map.
    if (SUCCEEDED(hr))
    {
        WpdObjectResourceContext* pResourceContext = new WpdObjectResourceContext();
        if (pResourceContext != NULL)
        {
            ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
            hr = m_pDevice->OpenResource(Scope, wszObjectID, Key, dwMode, pResourceContext);
            CHECK_HR(hr, "Failed to open resource");

            if (SUCCEEDED(hr))
            {
                // Add the resource context to the context map, this calls AddRef() on pResourceContext
                hr = pContextMap->Add(pResourceContext, strResourceContext);
                CHECK_HR(hr, "Failed to add the resource context");
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate resource context");
        }
        SAFE_RELEASE(pResourceContext);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, strResourceContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Set the optimal buffer size
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE, FILE_OPTIMAL_READ_BUFFER_SIZE_VALUE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE value");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszObjectID);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_READ
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: the context the driver returned to
 *      the client in OnOpenResource.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ: the number of bytes to
 *      read from the resource.
 *
 *  The driver should:
 *  - Read data associated with the resource and return it back to the caller in
 *      WPD_PROPERTY_OBJECT_RESOURCES_DATA.
 *  - Report the number of bytes actually read from the resource in
 *      WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ.  This number may be smaller
 *      than WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ when reading the last
 *      chunk of data from the resource.
 */
HRESULT WpdObjectResources::OnReadResource(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                   hr                 = S_OK;
    LPWSTR                    wszResourceContext = NULL;
    DWORD                     dwNumBytesToRead   = 0;
    DWORD                     dwNumBytesRead     = 0;
    BYTE*                     pBuffer            = NULL;
    WpdObjectResourceContext* pResourceContext   = NULL;

    // Get the enumeration context identifier for this enumeration operation.  We will
    // need this to lookup the specific enumeration context in the client context map.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &wszResourceContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the number of bytes to read
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ, &dwNumBytesToRead);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ");
    }

    // Allocate the destination buffer
    if (SUCCEEDED(hr))
    {
        pBuffer = reinterpret_cast<BYTE *>(CoTaskMemAlloc(dwNumBytesToRead));
        if (pBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate the destination buffer");
        }
    }

    // Get the client context map so we can retrieve the resource context for this resource
    // operation using the WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT property value obtained above.
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, wszResourceContext, (IUnknown**)&pResourceContext);
        CHECK_HR(hr, "Failed to get the resource context");
    }

    // Read the next chunk of data for this request
    if (SUCCEEDED(hr))
    {
        hr = m_pDevice->ReadResourceData(pResourceContext, pBuffer, dwNumBytesToRead, &dwNumBytesRead);
        CHECK_HR(hr, "Failed to read %d bytes from resource", dwNumBytesToRead);
    }

    if (SUCCEEDED(hr))
    {
        pResourceContext->m_BytesTransferred += dwNumBytesRead;
        hr = pResults->SetBufferValue(WPD_PROPERTY_OBJECT_RESOURCES_DATA, pBuffer, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_DATA");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszResourceContext);

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pBuffer);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_CLOSE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: the context the driver returned to
 *      the client in OnOpenResource.
 *
 *  The driver should:
 *  - Destroy any data associated with this context.
 */
HRESULT WpdObjectResources::OnCloseResource(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr                 = S_OK;
    LPWSTR      wszResourceContext = NULL;
    ContextMap* pContextMap        = NULL;

    UNREFERENCED_PARAMETER(pResults);

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the resource context identifier for this resource operation.  We will
    // need this to lookup the specific resource context in the client context map.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &wszResourceContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the client context map so we can retrieve the resource context for this resource
    // operation using the WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT property value obtained above.
    if (SUCCEEDED(hr))
    {
        hr = GetClientContextMap(pParams, &pContextMap);
        CHECK_HR(hr, "Failed to get the client context map");
    }

    // Destroy any data allocated/associated with the resource context and then remove it from the context map.
    // We no longer need to keep this context around because the resource operation has been ended.
    if (SUCCEEDED(hr))
    {
        pContextMap->Remove(wszResourceContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszResourceContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}
