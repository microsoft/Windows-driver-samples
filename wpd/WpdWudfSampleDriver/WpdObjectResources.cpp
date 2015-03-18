#include "stdafx.h"
#include "WpdObjectResources.tmh"

WpdObjectResources::WpdObjectResources()
{

}

WpdObjectResources::~WpdObjectResources()
{

}

HRESULT WpdObjectResources::Initialize(
    _In_    FakeDevice *pFakeDevice)
{
    HRESULT hr = S_OK;

    if(pFakeDevice == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    m_pFakeDevice = pFakeDevice;
    return hr;
}

HRESULT WpdObjectResources::DispatchWpdMessage(
    _In_    REFPROPERTYKEY          Command,
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES))
        {
            hr = OnGetAttributes(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to get resource attributes");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_OPEN))
        {
            hr = OnOpen(pParams, pResults);
            CHECK_HR(hr, "Failed to open resource");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_READ))
        {
            hr = OnRead(pParams, pResults);
            CHECK_HR(hr, "Failed to read resource data");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_WRITE))
        {
            hr = OnWrite(pParams, pResults);
            CHECK_HR(hr, "Failed to write resource data");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_CLOSE))
        {
            hr = OnClose(pParams, pResults);
            CHECK_HR(hr, "Failed to close resource");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_DELETE))
        {
            hr = OnDelete(pParams, pResults);
            CHECK_HR(hr, "Failed to delete resources");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_CREATE_RESOURCE))
        {
            hr = OnCreate(pParams, pResults);
            CHECK_HR(hr, "Failed to create resource");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_REVERT))
        {
            hr = OnRevert(pParams, pResults);
            CHECK_HR(hr, "Failed to revert resource operation");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_RESOURCES_SEEK))
        {
            hr = OnSeek(pParams, pResults);
            CHECK_HR(hr, "Failed resource seek operation");
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
    HRESULT hr              = S_OK;
    LPWSTR  pszObjectID     = NULL;

    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &pszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    // Get the collection of resource keys
    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetSupportedResources(pszObjectID, &pKeys);
        CHECK_HR(hr, "Failed to get resource keys collection on [%ws]", pszObjectID);
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: identifies the object whose resource attributes we want to return.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS: a collection of resource keys containing a single value,
 *      which is the key identifying the specific resource whose attributes we are requested to return.
 *
 *  The driver should:
 *  - Return the requested resource attributes.  If any resource attributes failed to be retrieved,
 *    the corresponding value should be set to type VT_ERROR with the 'scode' member holding the
 *    HRESULT reason for the failure.
 *  - S_OK should be returned if all resource attributes were read successfully.
 *  - S_FALSE should be returned if any resource attribute failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any resource attributes.
 */
HRESULT WpdObjectResources::OnGetAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT             hr              = S_OK;
    LPWSTR              pszObjectID     = NULL;
    PROPERTYKEY         Key             = {0};

    CComPtr<IPortableDeviceValues>       pAttributeStore;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &pszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetResourceAttributes(pszObjectID, Key, &pAttributeStore);
        CHECK_HR(hr, "Failed to get attributes on [%ws]", pszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;

        hrTemp = pResults->SetIUnknownValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES, pAttributeStore);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES"));

        if(FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_OPEN
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: identifies the object whose resource we are interested in.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE: specifies the access requested by the caller.  It will
 *                                               be either STGM_READ or STGM_WRITE.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS: a collection of resource keys containing a single value,
 *      which is the key identifying the specific resource the caller is interested in.
 *
 *  The driver should:
 *  - Create a context associated with this resource.  This context will be used by clients when reading/writing
 *    resource data.  Generally, most drivers will also lock the resource here if necessary.
 *    The context identifier should be a string value returned in WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT.
 * - Return the optimal transfer buffer size in WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE.
 */
HRESULT WpdObjectResources::OnOpen(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr                  = S_OK;
    LPWSTR      pszObjectID         = NULL;
    PROPERTYKEY Key                 = {0};
    DWORD       dwMode              = STGM_READ;
    LPWSTR      pszContext          = NULL;
    ContextMap* pContextMap         = NULL;
    BOOL        bSupportsResource   = FALSE;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID, &pszObjectID);
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
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Validate whether this object supports the requested resource
    if (SUCCEEDED(hr))
    {
        hr = m_pFakeDevice->SupportsResource(pszObjectID, Key, &bSupportsResource);
        CHECK_HR(hr, "Failed to check whether object supports resources");
    }

    if (SUCCEEDED(hr) && !bSupportsResource)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Object does not support this resource");
    }

    // Create the context
    if (SUCCEEDED(hr))
    {
        hr = CreateResourceContext(pContextMap, pszObjectID, Key, FALSE, &pszContext);
        CHECK_HR(hr, "Failed to create resource context for %ws.%d on %ws", CComBSTR(Key.fmtid), Key.pid, pszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, pszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Set the optimal buffer size
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE value");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_READ
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: identifies the previsouly opened resource we are going to read from.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ: specifies the next number of bytes to read.
 *
 *  The driver should:
 *  - Read up to the next WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ from the resource.
 *  - Return the number of bytes actually read in WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ.
 */
HRESULT WpdObjectResources::OnRead(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT             hr                  = S_OK;
    LPWSTR              pszContext          = NULL;
    DWORD               dwNumBytesToRead    = 0;
    DWORD               dwNumBytesRead      = 0;
    BYTE*               pBuffer             = NULL;
    ResourceContext*    pContext            = NULL;

    // Get the Context
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &pszContext);
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

    // Get the context for this transfer
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, pszContext, (IUnknown**) &pContext);
        CHECK_HR(hr, "Failed to get resource context");
    }

    // Read the next band of data for this transfer request
    if (SUCCEEDED(hr) && pBuffer != NULL)
    {
        hr = m_pFakeDevice->ReadData(pContext->ObjectID,
                                     pContext->Key,
                                     pContext->NumBytesTransfered,
                                     pBuffer,
                                     dwNumBytesToRead,
                                     &dwNumBytesRead);
        CHECK_HR(hr, "Failed to read %d bytes from [%ws] on resource {%ws}.%d", dwNumBytesToRead, pContext->ObjectID, CComBSTR(pContext->Key.fmtid), pContext->Key.pid);
        if (SUCCEEDED(hr))
        {
            pContext->NumBytesTransfered += dwNumBytesRead;
        }
    }

    if (SUCCEEDED(hr) && pBuffer != NULL)
    {
        hr = pResults->SetBufferValue(WPD_PROPERTY_OBJECT_RESOURCES_DATA, pBuffer, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_DATA");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pBuffer);

    SAFE_RELEASE(pContext);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_WRITE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: identifies the previsouly opened resource we are going to write to.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_WRITE: specifies the next number of bytes to write.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_DATA: specifies byte array where the data should be copied from.
 *
 *  The driver should:
 *  - Write the next WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_WRITE to the resource.
 *  - Return the number of bytes actually written in WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_WRITTEN.
 *    It is normally considered an error if this value does not match WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_WRITE.
 */
HRESULT WpdObjectResources::OnWrite(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT             hr                  = S_OK;
    LPWSTR              pszContext          = NULL;
    DWORD               dwNumBytesToWrite   = 0;
    DWORD               dwNumBytesWritten   = 0;
    BYTE*               pBuffer             = NULL;
    DWORD               cbBuffer            = 0;
    ResourceContext*    pContext            = NULL;

    // Get the Context string
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &pszContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the number of bytes to write
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_WRITE, &dwNumBytesToWrite);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_WRITE");
    }

    // Get the source buffer
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetBufferValue(WPD_PROPERTY_OBJECT_RESOURCES_DATA, &pBuffer, &cbBuffer);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_DATA");
    }

    // Get the resource context for this transfer
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, pszContext, (IUnknown**) &pContext);
        CHECK_HR(hr, "Faield to get resource context");
    }

    // Write the next band of data for this transfer request
    if (SUCCEEDED(hr) && pBuffer != NULL)
    {
        hr = m_pFakeDevice->WriteData(pContext->ObjectID,
                                      pContext->Key,
                                      pContext->NumBytesTransfered,
                                      pBuffer,
                                      dwNumBytesToWrite,
                                      &dwNumBytesWritten);
        CHECK_HR(hr, "Failed to write %d bytes to [%ws] on resource {%ws}.%d", dwNumBytesToWrite, pContext->ObjectID, CComBSTR(pContext->Key.fmtid), pContext->Key.pid);
        if (SUCCEEDED(hr))
        {
            pContext->NumBytesTransfered += dwNumBytesWritten;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_WRITTEN, dwNumBytesWritten);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_WRITTEN");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pBuffer);

    SAFE_RELEASE(pContext);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_CLOSE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: identifies the resource context associated with a
 *                                           previously opened resource.
 *
 *  The driver should:
 *  - Unlock the resource if necessary, and release any system and device resources associated with this WPD object resource.
 */
HRESULT WpdObjectResources::OnClose(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    // No results are expected to be returned
    UNREFERENCED_PARAMETER(pResults);

    HRESULT          hr              = S_OK;
    LPWSTR           pszContext      = NULL;
    ContextMap*      pContextMap     = NULL;
    ResourceContext* pContext        = NULL;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &pszContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the context map which the driver stored in pParams for convenience
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // If this was a creation request, enable the resource for the content
    if (SUCCEEDED(hr))
    {
        // Get the context for this transfer
        hr = GetClientContext(pParams, pszContext, (IUnknown**) &pContext);
        CHECK_HR(hr, "Failed to get resource context");
    }

    if (SUCCEEDED(hr))
    {
        if (pContext->CreateRequest == TRUE)
        {
            hr = m_pFakeDevice->EnableResource(pContext->ObjectID, pContext->Key);
            CHECK_HR(hr, "Failed to enable resource on object [%ws]", pContext->ObjectID);
        }
    }

    //Free the context
    SAFE_RELEASE(pContext);

    if (SUCCEEDED(hr))
    {
        hr = DestroyResourceContext(pContextMap, pszContext);
        CHECK_HR(hr, "Failed to remove resource context [%ws]", pszContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_DELETE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID: identifies the object whose resources should be deleted.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS: a collection of keys indicating which
 *      resources to delete.
 *
 *  The driver should:
 *  - Delete the specified resources from the object.
 *  - S_OK should be returned if all specified properties were successfully deleted.
 *  - E_ACCESSDENIED should be returned if the client attempts to delete a resource which is not deletable (i.e.
 *    WPD_RESOURCE_ATTRIBUTE_CAN_DELETE is FALSE for that resource.)
 */
HRESULT WpdObjectResources::OnDelete(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = E_ACCESSDENIED;

    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This driver has no resources which can be deleted.
    return hr;
}

HRESULT WpdObjectResources::OnCreate(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    HRESULT     hr               = S_OK;
    LPWSTR      pszObjectID      = NULL;
    LPWSTR      pszContext       = NULL;
    ContextMap* pContextMap      = NULL;
    PROPERTYKEY ResourceKey      = WPD_PROPERTY_NULL;
    GUID        guidObjectFormat = GUID_NULL;
    CComPtr<IPortableDeviceValues> pResourceAttributes;

    // Getthe Resource Attributes
    hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES, &pResourceAttributes);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_ATTRIBUTES");
    }

    // Get the Resource Key
    if (SUCCEEDED(hr))
    {
        hr = pResourceAttributes->GetKeyValue(WPD_RESOURCE_ATTRIBUTE_RESOURCE_KEY, &ResourceKey);
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing value for WPD_RESOURCE_ATTRIBUTE_RESOURCE_KEY");
        }
    }

    if (SUCCEEDED(hr) && !IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_CONTACT_PHOTO))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "CreateResource is only supported on WPD_RESOURCE_CONTACT_PHOTO");
    }

    if (SUCCEEDED(hr))
    {
        // Get the Object ID
        hr = pResourceAttributes->GetStringValue(WPD_OBJECT_ID, &pszObjectID);
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing value for WPD_OBJECT_ID");
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pFakeDevice->GetContentFormat(pszObjectID, guidObjectFormat);
        CHECK_HR(hr, "Failed to get content object format");

        if (SUCCEEDED(hr) && !IsEqualGUID(guidObjectFormat, WPD_OBJECT_FORMAT_VCARD2))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "CreateResource is only supported on contact objects");
        }
    }

    // Get the context map which the driver stored in pParams for convenience
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Create the context
    if (SUCCEEDED(hr))
    {
        hr = CreateResourceContext(pContextMap, pszObjectID, ResourceKey, TRUE, &pszContext);
        CHECK_HR(hr, "Failed to create resource context for %ws.%d on %ws", CComBSTR(ResourceKey.fmtid), ResourceKey.pid, pszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, pszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Set the optimal buffer size
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_OPTIMAL_TRANSFER_BUFFER_SIZE value");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_REVERT
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: identifies the resource context associated with a
 *                                           previously opened resource.
 *
 *  The driver should:
 *  - Unlock and remove the resource if necessary, and release any system and device resources associated with this WPD object resource.
 */
HRESULT WpdObjectResources::OnRevert(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    // No results are expected to be returned
    UNREFERENCED_PARAMETER(pResults);

    HRESULT     hr              = S_OK;
    LPWSTR      pszContext      = NULL;
    ContextMap* pContextMap     = NULL;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &pszContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the context map which the driver stored in pParams for convenience
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    //Free the context
    if (SUCCEEDED(hr))
    {
        hr = DestroyResourceContext(pContextMap, pszContext);
        CHECK_HR(hr, "Failed to remove resource context [%ws]", pszContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_RESOURCES_SEEK
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT: identifies the resource context associated with a
 *                                           previously opened resource.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_SEEK_OFFSET: Displacement to be added to the location indicated by the WPD_PROPERTY_OBJECT_RESOURCES_SEEK_ORIGIN_FLAG parameter.
 *  - WPD_PROPERTY_OBJECT_RESOURCES_SEEK_ORIGIN_FLAG: Specifies the origin of the displacement for the seek operation. Can be one of the following values:
 *                                                      STREAM_SEEK_SET - Offset is from the beginning of the stream
 *                                                      STREAM_SEEK_CUR - Offset is from the current position in the stream
 *                                                      STREAM_SEEK_END - Offset is from the end of the stream
 *
 *  The driver should:
 *  - Move the seek pointer for the resource to point to the specified position, so that
 *    subsequent read / write operations occur from the new position.
 *  - Return the new position as an offset relative to the start of the data stream in WPD_PROPERTY_OBJECT_RESOURCES_POSITION_FROM_START.
 */
HRESULT WpdObjectResources::OnSeek(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    // No results are expected to be returned
    UNREFERENCED_PARAMETER(pResults);

    HRESULT     hr              = S_OK;
    LPWSTR      pszContext      = NULL;
    DWORD       dwOrigin        = 0;
    LONGLONG    lOffset         = 0;

    ResourceContext*    pContext            = NULL;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &pszContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the offset
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetSignedLargeIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_SEEK_OFFSET, &lOffset);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_SEEK_OFFSET");
    }

    // Get the origin flags
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_SEEK_ORIGIN_FLAG, &dwOrigin);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_SEEK_ORIGIN_FLAG");
    }

    // Get the context for this transfer
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, pszContext, (IUnknown**) &pContext);
        CHECK_HR(hr, "Failed to get resource context");
    }

    // Update the current seek pointer.  For this driver, we update the NumBytesTransfered in the context so that subsequent reads/writes start
    // at the appropriate place.
    if (SUCCEEDED(hr))
    {
        ULONG ulSize = 0;
        ULONG ulOriginalNumBytesTransfered = pContext->NumBytesTransfered;
        CComPtr<IPortableDeviceValues> pResourceAttributes;

        // Get the total size of the resource
        hr = m_pFakeDevice->GetResourceAttributes(pContext->ObjectID, pContext->Key, &pResourceAttributes);
        CHECK_HR(hr, "Failed to get attributes on [%ws]", pContext->ObjectID);
        if (SUCCEEDED(hr))
        {
            hr = pResourceAttributes->GetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, &ulSize);
            CHECK_HR(hr, "Failed to get WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
        }

        if(dwOrigin == STREAM_SEEK_CUR)
        {
            pContext->NumBytesTransfered += (LONG) lOffset;
        }
        else if(dwOrigin == STREAM_SEEK_SET)
        {
            pContext->NumBytesTransfered = (LONG) lOffset;
        }
        else
        {
            pContext->NumBytesTransfered = ulSize + (LONG) lOffset;
        }

        // Validate that this is in the correct range
        if (SUCCEEDED(hr) && pContext->NumBytesTransfered > ulSize)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Attempting to seek beyond existing data");
            // Restore the seek pointer to its original value
            pContext->NumBytesTransfered = ulOriginalNumBytesTransfered;
        }
    }

    // Set the return value
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_POSITION_FROM_START, pContext->NumBytesTransfered);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_POSITION_FROM_START");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContext);

    return hr;
}

HRESULT WpdObjectResources::CreateResourceContext(
    _In_     ContextMap*     pContextMap,
    _In_     LPCWSTR         pszObjectID,
    _In_     REFPROPERTYKEY  ResourceKey,
    _In_     BOOL            bCreateRequest,
    _Outptr_ LPWSTR*         ppszResourceContext)
{
    HRESULT         hr              = S_OK;
    GUID            guidContext     = GUID_NULL;
    ResourceContext* pContext       = NULL;

    if((pContextMap         == NULL) ||
       (pszObjectID         == NULL) ||
       (ppszResourceContext == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppszResourceContext = NULL;

    hr = CoCreateGuid(&guidContext);
    CHECK_HR(hr, "Failed to CoCreateGuid used for identifying the resource context");

    if (SUCCEEDED(hr))
    {
        pContext = new ResourceContext();
        if(pContext == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate new resource context");
        }
    }

    if (SUCCEEDED(hr))
    {
        pContext->ObjectID      = pszObjectID;
        pContext->Key           = ResourceKey;
        pContext->CreateRequest = bCreateRequest;

        CAtlStringW strKey = CComBSTR(guidContext);
        hr = pContextMap->Add(strKey, pContext);
        CHECK_HR(hr, "Failed to insert bulk property operation context into our context Map");
    }

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(guidContext, ppszResourceContext);
        CHECK_HR(hr, "Failed to allocate string from GUID for resource context");
    }

    SAFE_RELEASE(pContext);

    return hr;
}

HRESULT WpdObjectResources::DestroyResourceContext(
    _In_    ContextMap* pContextMap,
    _In_    LPCWSTR     pszResourceContext)
{
    HRESULT hr = S_OK;

    if(pszResourceContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    CAtlStringW strKey = pszResourceContext;
    pContextMap->Remove(strKey);

    return hr;
}

