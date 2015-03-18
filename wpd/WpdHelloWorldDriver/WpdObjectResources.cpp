#include "stdafx.h"
#include "wpdobjectresources.tmh"

WpdObjectResources::WpdObjectResources()
{

}

WpdObjectResources::~WpdObjectResources()
{

}

HRESULT WpdObjectResources::DispatchWpdMessage(
    _In_ REFPROPERTYKEY         Command,
    _In_ IPortableDeviceValues* pParams,
    _In_ IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if (Command.fmtid != WPD_CATEGORY_OBJECT_RESOURCES)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
        hr = GetSupportedResourcesForObject(wszObjectID, pKeys);
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
        hr = GetResourceAttributesForObject(wszObjectID, Key, pAttributes);
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_OBJECT_ID");
    }

    // Get the resource key
    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_RESOURCE_KEYS");
    }

    // Get the access mode
    if (hr == S_OK)
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE, &dwMode);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_ACCESS_MODE");
    }

    // Validate whether the params given to us are correct.  In this case, we need to check that the object
    // supports the resource requested, and can be opened in the requested access mode.
    if (hr == S_OK)
    {
        // In this sample, we only have one object (README_FILE_OBJECT_ID) which supports a
        // resource (WPD_RESOURCE_DEFAULT) for reading only.
        // So if any other Object ID or any other resource is specified, it must be invalid.
        strStrObjectID = wszObjectID;
        if(strStrObjectID.CompareNoCase(README_FILE_OBJECT_ID) != 0)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Object [%ws] does not support resources", wszObjectID);
        }
        if (hr == S_OK)
        {
            if (!IsEqualPropertyKey(Key, WPD_RESOURCE_DEFAULT))
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Only WPD_RESOURCE_DEFAULT is supported in this sample driver");
            }
        }
        if (hr == S_OK)
        {
            if ((dwMode & STGM_WRITE) != 0)
            {
                hr = E_ACCESSDENIED;
                CHECK_HR(hr, "This resource is not available for write access");
            }
        }
    }

    // Get the context map which the driver stored in pParams for convenience
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Create a new resource operation context, initialize it, and add it to the client context map.
    if (hr == S_OK)
    {
        WpdObjectResourceContext* pResourceContext = new WpdObjectResourceContext();
        if (pResourceContext != NULL)
        {
            // Initialize the resource context with ...
            pResourceContext->m_strObjectID      = wszObjectID;
            pResourceContext->m_Resource         = Key;
            pResourceContext->m_BytesTransferred = 0;
            pResourceContext->m_BytesTotal       = GetObjectSize(wszObjectID);

            // Add the resource context to the context map
            pContextMap->Add(pResourceContext, strResourceContext);

            // Release the resource context because it has been AddRef'ed during Add()
            SAFE_RELEASE(pResourceContext);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate resource context");
        }
    }

    if (hr == S_OK)
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, strResourceContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Set the optimal buffer size
    if (hr == S_OK)
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT                   hr                 = S_OK;
    LPWSTR                    wszResourceContext = NULL;
    DWORD                     dwNumBytesToRead   = 0;
    DWORD                     dwNumBytesRead     = 0;
    BYTE*                     pBuffer            = NULL;
    WpdObjectResourceContext* pResourceContext   = NULL;
    ContextMap*               pContextMap        = NULL;

    // Get the enumeration context identifier for this enumeration operation.  We will
    // need this to lookup the specific enumeration context in the client context map.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT, &wszResourceContext);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the number of bytes to read
    if (hr == S_OK)
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ, &dwNumBytesToRead);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_TO_READ");
    }

    // Allocate the destination buffer
    if (hr == S_OK)
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
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (hr == S_OK)
    {
        pResourceContext = (WpdObjectResourceContext*)pContextMap->GetContext(wszResourceContext);
        if (pResourceContext == NULL)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing resource context");
        }
    }

    // Read the next chunk of data for this request
    if (hr == S_OK && pBuffer != NULL)
    {
        hr = ReadDataFromResource(pResourceContext, pBuffer, dwNumBytesToRead, &dwNumBytesRead);
        CHECK_HR(hr, "Failed to read %d bytes from resource", dwNumBytesToRead);
    }

    if (hr == S_OK && pBuffer != NULL)
    {
        hr = pResults->SetBufferValue(WPD_PROPERTY_OBJECT_RESOURCES_DATA, pBuffer, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_DATA");
    }

    if (hr == S_OK)
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ, dwNumBytesRead);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_RESOURCES_NUM_BYTES_READ");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszResourceContext);

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pBuffer);

    SAFE_RELEASE(pContextMap);

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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT");
    }

    // Get the client context map so we can retrieve the resource context for this resource
    // operation using the WPD_PROPERTY_OBJECT_RESOURCES_CONTEXT property value obtained above.
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Destroy any data allocated/associated with the resource context and then remove it from the context map.
    // We no longer need to keep this context around because the resource operation has been ended.
    if (hr == S_OK)
    {
        pContextMap->Remove(wszResourceContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszResourceContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called to populate PROPERTYKEYs found on objects.
 *
 *  The parameters sent to us are:
 *  wszObjectID - the object whose supported resource keys are being requested
 *  pKeys - An IPortableDeviceKeyCollection to be populated with supported PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the specified object.
 */
HRESULT WpdObjectResources::GetSupportedResourcesForObject(
    _In_ LPCWSTR                       wszObjectID,
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    HRESULT     hr = S_OK;
    CAtlStringW strObjectID;

    if ((wszObjectID == NULL) ||
        (pKeys       == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    strObjectID = wszObjectID;

    if (strObjectID.CompareNoCase(README_FILE_OBJECT_ID) == 0)
    {
        hr = pKeys->Add(WPD_RESOURCE_DEFAULT);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_DEFAULT");
    }

    return hr;
}

/**
 *  This method is called to populate resource attributes found on a particular object
 *  resource.
 *
 *  The parameters sent to us are:
 *  wszObjectID - the object whose resource attributes are being requested
 *  Key - the resource on the specified object whose attributes are being returned
 *  pAttributes - An IPortableDeviceValues to be populated with resource attributes.
 *
 *  The driver should:
 *  Add attributes pertaining to the resource on the specified object.
 */
HRESULT WpdObjectResources::GetResourceAttributesForObject(
    _In_ LPCWSTR                wszObjectID,
    _In_ REFPROPERTYKEY         Key,
    _In_ IPortableDeviceValues* pAttributes)
{
    HRESULT     hr = S_OK;
    CAtlStringW strObjectID;

    if ((wszObjectID == NULL) ||
        (pAttributes == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    strObjectID = wszObjectID;

    if ((strObjectID.CompareNoCase(README_FILE_OBJECT_ID) == 0) && (IsEqualPropertyKey(Key, WPD_RESOURCE_DEFAULT)))
    {
        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_DELETE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_DELETE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedLargeIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetObjectSize(strObjectID));
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_READ, TRUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_READ");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_WRITE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_WRITE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_DELETE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_DELETE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, GetObjectFormat(strObjectID));
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE, FILE_OPTIMAL_READ_BUFFER_SIZE_VALUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE, FILE_OPTIMAL_WRITE_BUFFER_SIZE_VALUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE");
        }
    }

    return hr;
}

/**
 *  This method is called to read data from a particular object
 *  resource.
 *
 *  The parameters sent to us are:
 *  pResourceContext - the resource operation context
 *  pBuffer - the buffer to read the resource data into
 *  dwNumBytesToRead - number of bytes to read into the resource.  This is also
 *                     the total size of the passed in pBuffer.
 *  pdwNumBytesRead - On return, should contain the actual number of bytes read into pBuffer
 *
 *  The driver should:
 *   - Read data from the specified resource
 *   - Update the resource operation context with transfer state information
 *   - Return the actual number of bytes written in pdwNumBytesRead.
 */
HRESULT WpdObjectResources::ReadDataFromResource(
    _In_  WpdObjectResourceContext* pResourceContext,
    _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)  BYTE* pBuffer,
          DWORD                     dwNumBytesToRead,
    _Out_ DWORD*                    pdwNumBytesRead)
{
    HRESULT hr = S_OK;

    if ((pResourceContext == NULL) ||
        (pBuffer          == NULL) ||
        (pdwNumBytesRead  == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    *pdwNumBytesRead = 0;
    ZeroMemory(pBuffer, dwNumBytesToRead * sizeof(BYTE));

    // If we have data left to transfer, then transfer up to dwNumBytesToRead
    // if possible.
    if (pResourceContext->m_BytesTotal >= pResourceContext->m_BytesTransferred)
    {
        dwNumBytesToRead = (DWORD)min((ULONGLONG)dwNumBytesToRead,(pResourceContext->m_BytesTotal - pResourceContext->m_BytesTransferred));
    }

    // Read the data from the resource
    if (dwNumBytesToRead > 0)
    {
        // If we are reading from our single file resource, make sure you read
        // from the proper source data contents.
        if (pResourceContext->m_strObjectID.CompareNoCase(README_FILE_OBJECT_ID) == 0)
        {
            hr = StringCbCopyA((LPSTR)pBuffer, dwNumBytesToRead, README_FILE_OBJECT_CONTENTS);
            CHECK_HR(hr, "StringCbCopyA failed, dwNumBytesToRead = %ld", dwNumBytesToRead);
        }
    }

    if (SUCCEEDED(hr))
    {
        // update the number of bytes transferred in the resource context
        pResourceContext->m_BytesTransferred += dwNumBytesToRead;

        // set the number of bytes actually read into to pBuffer
        *pdwNumBytesRead = dwNumBytesToRead;
    }

    return hr;
}
