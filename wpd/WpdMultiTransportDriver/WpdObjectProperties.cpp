#include "stdafx.h"
#include "WpdObjectProperties.tmh"

const PROPERTYKEY g_SupportedCommonProperties[] =
{
    WPD_OBJECT_ID,
    WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    WPD_OBJECT_PARENT_ID,
    WPD_OBJECT_NAME,
    WPD_OBJECT_FORMAT,
    WPD_OBJECT_CONTENT_TYPE,
    WPD_OBJECT_CAN_DELETE,
};

const PROPERTYKEY g_SupportedDeviceProperties[] =
{
    WPD_DEVICE_FIRMWARE_VERSION,
    WPD_DEVICE_POWER_LEVEL,
    WPD_DEVICE_POWER_SOURCE,
    WPD_DEVICE_PROTOCOL,
    WPD_DEVICE_MODEL,
    WPD_DEVICE_SERIAL_NUMBER,
    WPD_DEVICE_SUPPORTS_NON_CONSUMABLE,
    WPD_DEVICE_MANUFACTURER,
    WPD_DEVICE_FRIENDLY_NAME,
    WPD_DEVICE_TYPE,
    WPD_FUNCTIONAL_OBJECT_CATEGORY,
};

const PROPERTYKEY g_SupportedStorageProperties[] =
{
    WPD_STORAGE_TYPE,
    WPD_STORAGE_FILE_SYSTEM_TYPE,
    WPD_STORAGE_CAPACITY,
    WPD_STORAGE_FREE_SPACE_IN_BYTES,
    WPD_STORAGE_SERIAL_NUMBER,
    WPD_STORAGE_DESCRIPTION,
    WPD_FUNCTIONAL_OBJECT_CATEGORY,
};

const PROPERTYKEY g_SupportedCommonFileProperties[] =
{
    WPD_OBJECT_ORIGINAL_FILE_NAME,
    WPD_OBJECT_SIZE,
    WPD_OBJECT_DATE_MODIFIED,
    WPD_OBJECT_DATE_CREATED,
};

const PROPERTYKEY g_SupportedCommonFolderProperties[] =
{
    WPD_OBJECT_ORIGINAL_FILE_NAME,
    WPD_OBJECT_DATE_MODIFIED,
    WPD_OBJECT_DATE_CREATED,
};

WpdObjectProperties::WpdObjectProperties()
{

}

WpdObjectProperties::~WpdObjectProperties()
{

}

HRESULT WpdObjectProperties::DispatchWpdMessage(
    _In_ REFPROPERTYKEY         Command,
    _In_ IPortableDeviceValues* pParams,
    _In_ IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if (Command.fmtid != WPD_CATEGORY_OBJECT_PROPERTIES)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED))
        {
            hr = OnGetSupportedProperties(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported properties");
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET))
        {
            hr = OnGetPropertyValues(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to get properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL))
        {
            hr = OnGetAllPropertyValues(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to get all properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_SET))
        {
            hr = OnSetPropertyValues(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to set properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES))
        {
            hr = OnGetPropertyAttributes(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to get property attributes");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_DELETE))
        {
            hr = OnDeleteProperties(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to delete properties");
            }
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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose supported properties have
 *      been requested.
 *
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_FILTER: the filter to use when returning supported properties.
 *      Since this parameter is optional, it may not exist.
 *      ! This driver currently ignores the filter parameter. !
 *
 *  The driver should:
 *  - Return supported property keys for the specified object in WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS
 */
HRESULT WpdObjectProperties::OnGetSupportedProperties(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  wszObjectID = NULL;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier whose supported properties have been requested
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing string value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // CoCreate a collection to store the supported property keys.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    // Add supported property keys for the specified object to the collection
    if (hr == S_OK)
    {
        hr = AddSupportedPropertyKeys(wszObjectID, pKeys);
        CHECK_HR(hr, "Failed to add supported property keys for object '%ws'", wszObjectID);
    }

    // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property values have been requested.
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS: a collection of property keys, identifying which
 *      specific property values we are requested to return.
 *
 *  The driver should:
 *  - Return all requested property values in WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES.  If any property read failed, the corresponding value should be
 *      set to type VT_ERROR with the 'scode' member holding the HRESULT reason for the failure.
 *  - S_OK should be returned if all properties were read successfully.
 *  - S_FALSE should be returned if any property read failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any property values.
 */
HRESULT WpdObjectProperties::OnGetPropertyValues(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  wszObjectID = NULL;
    CComPtr<IPortableDeviceValues>        pValues;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier whose property values have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // Get the list of property keys for the property values the caller wants to retrieve from the specified object
    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, &pKeys);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    // CoCreate a collection to store the property values.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pValues);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // Read the specified properties on the specified object and add the property values to the collection.
    if (hr == S_OK)
    {
        hr = GetPropertyValuesForObject(wszObjectID, pKeys, pValues);
        CHECK_HR(hr, "Failed to get property values for object '%ws'", wszObjectID);
    }

    // S_OK or S_FALSE can be returned from GetPropertyValuesForObject( ).
    // S_FALSE means that 1 or more property values could not be retrieved successfully.
    // The value for the specified property should be set to an error HRESULT of
    // the reason why the property could not be read.
    // (e.g. If the property being requested is not supported on the object then an error of
    // HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) should be set as the value.
    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES value in the results.
        HRESULT hrTemp = S_OK;
        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, pValues);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));

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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property values have been requested.
 *
 *  The driver should:
 *  - Return all property values in WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES.  If any property read failed, the corresponding value should be
 *      set to type VT_ERROR with the 'scode' member holding the HRESULT reason for the failure.
 *  - S_OK should be returned if all properties were read successfully.
 *  - S_FALSE should be returned if any property read failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any property values.
 */
HRESULT WpdObjectProperties::OnGetAllPropertyValues(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  wszObjectID = NULL;
    CComPtr<IPortableDeviceValues>        pValues;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier whose property values have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // CoCreate a collection to store the property values.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pValues);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // CoCreate a collection to store the property keys we are going to use
    // to request the property values of.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    // First we make a request for ALL supported property keys for the specified object.
    // Next, we delegate to our helper function GetPropertyValuesForObject( ) passing
    // the entire property key collection.  This will reuse existing implementation
    // in our driver to perform the GetAllPropertyValues operation.
    if (hr == S_OK)
    {
        hr = AddSupportedPropertyKeys(wszObjectID, pKeys);
        CHECK_HR(hr, "Failed to get ALL supported properties for object '%ws'", wszObjectID);
        if (hr == S_OK)
        {
            hr = GetPropertyValuesForObject(wszObjectID, pKeys, pValues);
            CHECK_HR(hr, "Failed to get property values for object '%ws'", wszObjectID);
        }
    }

    // S_OK or S_FALSE can be returned from GetPropertyValuesForObject( ).
    // S_FALSE means that 1 or more property values could not be retrieved successfully.
    // The value for the specified property key should be set to the error HRESULT of
    // the reason why the property could not be read.
    // (i.e. an error of HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) if a property value was
    //  requested and is not supported by the specified object.)
    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES value in the results
        HRESULT hrTemp = S_OK;
        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, pValues);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));

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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_SET
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property values we want to return.
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES: an IPortableDeviceValues of values, identifying which
 *      specific property values we are requested to write.
 *
 *  The driver should:
 *  - Write all requested property values.  For each property, a write result should be returned in the
 *      write result property store.
 *  - If any property write failed, the corresponding write result value should be
 *      set to type VT_ERROR with the 'scode' member holding the HRESULT reason for the failure.
 *  - S_OK should be returned if all properties were written successfully.
 *  - S_FALSE should be returned if any property write failed.
 *  - Any error return indicates that the driver did not write any results, and the caller will
 *      not attempt to unpack any property write results.
 */
HRESULT WpdObjectProperties::OnSetPropertyValues(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  wszObjectID = NULL;
    DWORD   cValues     = 0;
    CComPtr<IPortableDeviceValues> pValues;
    CComPtr<IPortableDeviceValues> pWriteResults;
    CComPtr<IPortableDeviceValues> pEventParams;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier whose property values are being set
    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // Get the caller-supplied property values requested to be set on the object
    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, &pValues);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES");
    }

    // CoCreate a collection to store the property set operation results.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pWriteResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // Set the property values on the specified object
    if (hr == S_OK)
    {
        // Since this driver does not support setting any properties, all property set operation
        // results will be set to E_ACCESSDENIED.
        if (hr == S_OK)
        {
            hr = pValues->GetCount(&cValues);
            CHECK_HR(hr, "Failed to get total number of values");
        }

        if (hr == S_OK)
        {
            for (DWORD dwIndex = 0; dwIndex < cValues; dwIndex++)
            {
                PROPERTYKEY Key = WPD_PROPERTY_NULL;
                hr = pValues->GetAt(dwIndex, &Key, NULL);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d", dwIndex);

                if (hr == S_OK)
                {
                    hr = pWriteResults->SetErrorValue(Key, E_ACCESSDENIED);
                    CHECK_HR(hr, "Failed to set error result value at index %d", dwIndex);
                }
            }
        }

        // Since we have set failures for the property set operations we must let the application
        // know by returning S_FALSE. This will instruct the application to look at the
        // property set operation results for failure values.
        if (hr == S_OK)
        {
            hr = S_FALSE;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS value in the results
        HRESULT hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS, pWriteResults);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS"));

        if (FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property attributes we want to return.
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS: a collection of property keys containing a single value,
 *      which is the key identifying the specific property attributes we are requested to return.
 *
 *  The driver should:
 *  - Return the requested property attributes.  If any property attributes failed to be retrieved,
 *    the corresponding value should be set to type VT_ERROR with the 'scode' member holding the
 *    HRESULT reason for the failure.
 *  - S_OK should be returned if all property attributes were read successfully.
 *  - S_FALSE should be returned if any property attribute failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any property values.
 */
HRESULT WpdObjectProperties::OnGetPropertyAttributes(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT     hr          = S_OK;
    LPWSTR      wszObjectID = NULL;
    PROPERTYKEY Key         = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues>  pAttributes;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier whose property attributes have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // Get the list of property keys whose attributes are being requested
    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    // CoCreate a collection to store the property attributes.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // Get the attributes for the specified properties on the specified object and add them
    // to the collection.
    if (hr == S_OK)
    {
        hr = GetPropertyAttributesForObject(wszObjectID, Key, pAttributes);
        CHECK_HR(hr, "Failed to get property attributes");
    }

    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_ATTRIBUTES value in the results
        HRESULT hrTemp = S_OK;
        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_ATTRIBUTES, pAttributes);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_ATTRIBUTES"));

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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_DELETE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose properties should be deleted.
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS: a collection of property keys indicating which
 *      properties to delete.
 *
 *  The driver should:
 *  - Delete the specified properties from the object.
 *  - S_OK should be returned if all specified properties were successfully deleted.
 *  - E_ACCESSDENIED should be returned if the client attempts to delete a property which is not deletable (i.e.
 *    WPD_PROPERTY_ATTRIBUTE_CAN_DELETE is FALSE for that property.)
 */
HRESULT WpdObjectProperties::OnDeleteProperties(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr = E_ACCESSDENIED;

    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This driver has no properties which can be deleted.

    return hr;
}

/**
 *  This method is called to populate supported PROPERTYKEYs found on objects.
 *
 *  The parameters sent to us are:
 *  wszObjectID - the object whose supported property keys are being requested
 *  pKeys - An IPortableDeviceKeyCollection to be populated with supported PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the specified object.
 */
HRESULT AddSupportedPropertyKeys(
    _In_ LPCWSTR                        wszObjectID,
    _In_ IPortableDeviceKeyCollection*  pKeys)
{
    HRESULT     hr          = S_OK;
    CAtlStringW strObjectID = wszObjectID;

    // Add Common PROPERTYKEYs for ALL WPD objects
    AddCommonPropertyKeys(pKeys);

    if (strObjectID.CompareNoCase(WPD_DEVICE_OBJECT_ID) == 0)
    {
        // Add the PROPERTYKEYs for the 'DEVICE' object
        AddDevicePropertyKeys(pKeys);
    }

    if (strObjectID.CompareNoCase(STORAGE_OBJECT_ID) == 0)
    {
        // Add the PROPERTYKEYs for the storage object
        AddStoragePropertyKeys(pKeys);
    }

    if (strObjectID.CompareNoCase(README_FILE_OBJECT_ID) == 0)
    {
        // Add the PROPERTYKEYs for the file object
        AddFilePropertyKeys(pKeys);
    }

    if (strObjectID.CompareNoCase(DOCUMENTS_FOLDER_OBJECT_ID) == 0)
    {
        // Add the PROPERTYKEYs for the folder object
        AddFolderPropertyKeys(pKeys);
    }

    // Add other PROPERTYKEYs for other supported objects...

    return hr;
}

/**
 *  This method is called to populate common PROPERTYKEYs found on ALL objects.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the ALL objects.
 */
VOID AddCommonPropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommonProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedCommonProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add common property");
        }
    }
}

/**
 *  This method is called to populate common PROPERTYKEYs found on the DEVICE object.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the DEVICE object.
 */
VOID AddDevicePropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedDeviceProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedDeviceProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add device property");
        }
    }
}

/**
 *  This method is called to populate common PROPERTYKEYs found on storage objects.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the storage objects.
 */
VOID AddStoragePropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedStorageProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedStorageProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add storage property");
        }
    }
}

/**
 *  This method is called to populate common PROPERTYKEYs found on file objects.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the file objects.
 */
VOID AddFilePropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommonFileProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedCommonFileProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add common file property");
        }
    }
}

/**
 *  This method is called to populate common PROPERTYKEYs found on folder objects.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the file objects.
 */
VOID AddFolderPropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommonFolderProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedCommonFolderProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add common folder property");
        }
    }
}

/**
 *  This method is called to populate property values for the object specified.
 *
 *  The parameters sent to us are:
 *  wszObjectID - the object whose properties are being requested.
 *  pKeys - the list of property keys of the properties to request from the object
 *  pValues - an IPortableDeviceValues which will contain the property values retreived from the object
 *
 *  The driver should:
 *  Read the specified properties for the specified object and populate pValues with the
 *  results.
 */
HRESULT WpdObjectProperties::GetPropertyValuesForObject(
    _In_ LPCWSTR                        wszObjectID,
    _In_ IPortableDeviceKeyCollection*  pKeys,
    _In_ IPortableDeviceValues*         pValues)
{
    HRESULT     hr          = S_OK;
    CAtlStringW strObjectID = wszObjectID;
    DWORD       cKeys       = 0;

    if ((wszObjectID == NULL) ||
        (pKeys       == NULL) ||
        (pValues     == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    hr = pKeys->GetCount(&cKeys);
    CHECK_HR(hr, "Failed to number of PROPERTYKEYs in collection");

    if (hr == S_OK)
    {
        // Get values for the DEVICE object
        if (strObjectID.CompareNoCase(WPD_DEVICE_OBJECT_ID) == 0)
        {
            for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
            {
                PROPERTYKEY Key = WPD_PROPERTY_NULL;
                hr = pKeys->GetAt(dwIndex, &Key);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d in collection", dwIndex);

                if (hr == S_OK)
                {
                    // Preset the property value to 'error not supported'.  The actual value
                    // will replace this value, if read from the device.
                    pValues->SetErrorValue(Key, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

                    // Set DEVICE object properties
                    if (IsEqualPropertyKey(Key, WPD_DEVICE_FIRMWARE_VERSION))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_FIRMWARE_VERSION, DEVICE_FIRMWARE_VERSION_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_FIRMWARE_VERSION");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_LEVEL))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_LEVEL, DEVICE_POWER_LEVEL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_LEVEL");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_SOURCE))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_SOURCE, WPD_POWER_SOURCE_EXTERNAL);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_SOURCE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_PROTOCOL))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_PROTOCOL, DEVICE_PROTOCOL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_PROTOCOL");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_MODEL))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_MODEL, DEVICE_MODEL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_MODEL");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_SERIAL_NUMBER))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_SERIAL_NUMBER, DEVICE_SERIAL_NUMBER_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_SERIAL_NUMBER");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_SUPPORTS_NON_CONSUMABLE))
                    {
                        hr = pValues->SetBoolValue(WPD_DEVICE_SUPPORTS_NON_CONSUMABLE, DEVICE_SUPPORTS_NONCONSUMABLE_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_SUPPORTS_NON_CONSUMABLE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_MANUFACTURER))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_MANUFACTURER, DEVICE_MANUFACTURER_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_MANUFACTURER");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_FRIENDLY_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, DEVICE_FRIENDLY_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_FRIENDLY_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_DEVICE_TYPE))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_TYPE, WPD_DEVICE_TYPE_GENERIC);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_TYPE");
                    }

                    // Set general properties for DEVICE
                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_NAME, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, L"");
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
                    {
                        hr = pValues->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_DEVICE);
                        CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
                    }
                }
            }
        }
        else if (strObjectID.CompareNoCase(STORAGE_OBJECT_ID) == 0)
        {
            for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
            {
                PROPERTYKEY Key = WPD_PROPERTY_NULL;
                hr = pKeys->GetAt(dwIndex, &Key);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d in collection", dwIndex);

                if (hr == S_OK)
                {
                    // Preset the property value to 'error not supported'.  The actual value
                    // will replace this value, if read from the device.
                    pValues->SetErrorValue(Key, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

                    // Set storage object properties
                    if (IsEqualPropertyKey(Key, WPD_STORAGE_SERIAL_NUMBER))
                    {
                        hr = pValues->SetStringValue(WPD_STORAGE_SERIAL_NUMBER, STORAGE_SERIAL_NUMBER_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_SERIAL_NUMBER");
                    }

                    if (IsEqualPropertyKey(Key, WPD_STORAGE_FREE_SPACE_IN_BYTES))
                    {
                        hr = pValues->SetUnsignedLargeIntegerValue(WPD_STORAGE_FREE_SPACE_IN_BYTES, (STORAGE_FREE_SPACE_IN_BYTES_VALUE - GetObjectSize(README_FILE_OBJECT_ID)));
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_FREE_SPACE_IN_BYTES");
                    }

                    if (IsEqualPropertyKey(Key, WPD_STORAGE_CAPACITY))
                    {
                        hr = pValues->SetUnsignedLargeIntegerValue(WPD_STORAGE_CAPACITY, STORAGE_CAPACITY_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_CAPACITY");
                    }

                    if (IsEqualPropertyKey(Key, WPD_STORAGE_TYPE))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_STORAGE_TYPE, WPD_STORAGE_TYPE_FIXED_ROM);
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_TYPE");
                    }

                    // Set general properties for storage
                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, STORAGE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_NAME, STORAGE_OBJECT_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, STORAGE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
                    {
                        hr = pValues->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_STORAGE);
                        CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
                    }

                    if (IsEqualPropertyKey(Key, WPD_STORAGE_FILE_SYSTEM_TYPE))
                    {
                        hr = pValues->SetStringValue(WPD_STORAGE_FILE_SYSTEM_TYPE, STORAGE_FILE_SYSTEM_TYPE_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_FILE_SYSTEM_TYPE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_STORAGE_DESCRIPTION))
                    {
                        hr = pValues->SetStringValue(WPD_STORAGE_DESCRIPTION, STORAGE_DESCRIPTION_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_STORAGE_DESCRIPTION");
                    }
                }
            }
        }
        else if (strObjectID.CompareNoCase(DOCUMENTS_FOLDER_OBJECT_ID) == 0)
        {
            for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
            {
                PROPERTYKEY Key = WPD_PROPERTY_NULL;
                hr = pKeys->GetAt(dwIndex, &Key);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d in collection", dwIndex);

                if (hr == S_OK)
                {
                    // Preset the property value to 'error not supported'.  The actual value
                    // will replace this value, if read from the device.
                    pValues->SetErrorValue(Key, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

                    // Set general properties for the folder object
                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, DOCUMENTS_FOLDER_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, DOCUMENTS_FOLDER_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, STORAGE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_NAME, DOCUMENTS_FOLDER_OBJECT_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ORIGINAL_FILE_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, DOCUMENTS_FOLDER_OBJECT_ORIGINAL_FILE_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FOLDER);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_DATE_MODIFIED))
                    {
                        PROPVARIANT pvDateModified = {0};
                        SYSTEMTIME  systemtime     = {0};

                        systemtime.wMonth = 6;
                        systemtime.wDay   = 26;
                        systemtime.wYear  = 2006;
                        systemtime.wHour  = 5;

                        // Initialize the Date Modified PROPVARIANT value
                        PropVariantInit(&pvDateModified);

                        pvDateModified.vt = VT_DATE;
                        if (SystemTimeToVariantTime(&systemtime, &pvDateModified.date) == TRUE)
                        {
                            hr = pValues->SetValue(WPD_OBJECT_DATE_MODIFIED, &pvDateModified);
                            CHECK_HR(hr, "Failed to set WPD_OBJECT_DATE_MODIFIED");
                        }
                        else
                        {
                            LONG lError = GetLastError();
                            hr = HRESULT_FROM_WIN32(lError);
                        }

                        PropVariantClear(&pvDateModified);
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_DATE_CREATED))
                    {
                        PROPVARIANT pvDateCreated = {0};
                        SYSTEMTIME  systemtime    = {0};

                        systemtime.wMonth = 1;
                        systemtime.wDay   = 24;
                        systemtime.wYear  = 2006;
                        systemtime.wHour  = 12;

                        // Initialize the Date Created PROPVARIANT value
                        PropVariantInit(&pvDateCreated);

                        pvDateCreated.vt = VT_DATE;
                        if (SystemTimeToVariantTime(&systemtime, &pvDateCreated.date) == TRUE)
                        {
                            hr = pValues->SetValue(WPD_OBJECT_DATE_CREATED, &pvDateCreated);
                            CHECK_HR(hr, "Failed to set WPD_OBJECT_DATE_CREATED");
                        }
                        else
                        {
                            LONG lError = GetLastError();
                            hr = HRESULT_FROM_WIN32(lError);
                        }

                        PropVariantClear(&pvDateCreated);
                    }
                }
            }
        }
        else if (strObjectID.CompareNoCase(README_FILE_OBJECT_ID) == 0)
        {
            for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
            {
                PROPERTYKEY Key = WPD_PROPERTY_NULL;
                hr = pKeys->GetAt(dwIndex, &Key);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d in collection", dwIndex);

                if (hr == S_OK)
                {
                    // Preset the property value to 'error not supported'.  The actual value
                    // will replace this value, if read from the device.
                    pValues->SetErrorValue(Key, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, README_FILE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, README_FILE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, DOCUMENTS_FOLDER_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_NAME, README_FILE_OBJECT_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_ORIGINAL_FILE_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, README_FILE_OBJECT_ORIGINAL_FILE_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, GetObjectFormat(strObjectID));
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, GetObjectContentType(strObjectID));
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_SIZE))
                    {
                        hr = pValues->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, GetObjectSize(strObjectID));
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_SIZE");
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_DATE_MODIFIED))
                    {
                        PROPVARIANT pvDateModified = {0};
                        SYSTEMTIME  systemtime     = {0};

                        systemtime.wMonth = 6;
                        systemtime.wDay   = 26;
                        systemtime.wYear  = 2006;
                        systemtime.wHour  = 5;

                        // Initialize the Date Modified PROPVARIANT value
                        PropVariantInit(&pvDateModified);

                        pvDateModified.vt = VT_DATE;
                        if (SystemTimeToVariantTime(&systemtime, &pvDateModified.date) == TRUE)
                        {
                            hr = pValues->SetValue(WPD_OBJECT_DATE_MODIFIED, &pvDateModified);
                            CHECK_HR(hr, "Failed to set WPD_OBJECT_DATE_MODIFIED");
                        }
                        else
                        {
                            LONG lError = GetLastError();
                            hr = HRESULT_FROM_WIN32(lError);
                        }

                        PropVariantClear(&pvDateModified);
                    }

                    if (IsEqualPropertyKey(Key, WPD_OBJECT_DATE_CREATED))
                    {
                        PROPVARIANT pvDateCreated = {0};
                        SYSTEMTIME  systemtime    = {0};

                        systemtime.wMonth = 1;
                        systemtime.wDay   = 24;
                        systemtime.wYear  = 2006;
                        systemtime.wHour  = 12;

                        // Initialize the Date Created PROPVARIANT value
                        PropVariantInit(&pvDateCreated);

                        pvDateCreated.vt = VT_DATE;
                        if (SystemTimeToVariantTime(&systemtime, &pvDateCreated.date) == TRUE)
                        {
                            hr = pValues->SetValue(WPD_OBJECT_DATE_CREATED, &pvDateCreated);
                            CHECK_HR(hr, "Failed to set WPD_OBJECT_DATE_CREATED");
                        }
                        else
                        {
                            LONG lError = GetLastError();
                            hr = HRESULT_FROM_WIN32(lError);
                        }

                        PropVariantClear(&pvDateCreated);
                    }
                }
            }
        }
    }

    return hr;
}

/**
 *  This method is called to populate property attributes for the object and property specified.
 *
 *  The parameters sent to us are:
 *  wszObjectID - the object whose property attributes are being requested.
 *  Key - the property whose attributes are being requested
 *  pAttributes - an IPortableDeviceValues which will contain the resulting property attributes
 *
 *  The driver should:
 *  Read the property attributes for the specified property on the specified object and
 *  populate pAttributes with the results.
 */
HRESULT WpdObjectProperties::GetPropertyAttributesForObject(
    _In_ LPCWSTR                wszObjectID,
    _In_ REFPROPERTYKEY         Key,
    _In_ IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if ((wszObjectID == NULL) ||
        (pAttributes == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    UNREFERENCED_PARAMETER(wszObjectID);
    UNREFERENCED_PARAMETER(Key);

    //
    // Since ALL of our properties have the same attributes, we are ignoring the
    // passed in wszObjectID and Key parameters.  These parameters allow you to
    // customize attributes for properties on specific objects. (i.e. WPD_OBJECT_ORIGINAL_FILE_NAME
    // may be READ/WRITE on some objects and READONLY on others. )
    //

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
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, FALSE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
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

    return hr;
}

/**
 *  This method is called to return the total size of the specified object
 *
 *  The parameters sent to us are:
 *  strObjectID - the object whose total size is being requested.
 *
 *  The driver should:
 *  Calculate or read the total size of the object and return it to the caller.
 */
ULONGLONG GetObjectSize(_In_ LPCWSTR wszObjectID)
{
    ULONGLONG FileObjectSize = 0;

    if (_wcsicmp(wszObjectID, README_FILE_OBJECT_ID) == 0)
    {
        size_t cbFileObjectContents = 0;
        if (SUCCEEDED(StringCbLengthA(README_FILE_OBJECT_CONTENTS, STRSAFE_MAX_CCH*sizeof(CHAR), &cbFileObjectContents)))
        {
            // StringCbLength() returns the size of the string excluding the null terminator, 
            // so we will account for it in our size calculation.
            FileObjectSize = cbFileObjectContents + sizeof(CHAR);
        }
    }

    return FileObjectSize;
}

/**
 *  This method is called to return the WPD format of the specified object
 *
 *  The parameters sent to us are:
 *  strObjectID - the object whose WPD format is being requested.
 *
 *  The driver should:
 *  Read the native format of the object and return a WPD format to the caller.
 */
GUID GetObjectFormat(_In_ LPCWSTR wszObjectID)
{
    GUID FileObjectFormat = WPD_OBJECT_FORMAT_UNSPECIFIED;

    if (_wcsicmp(wszObjectID, README_FILE_OBJECT_ID) == 0)
    {
        FileObjectFormat = WPD_OBJECT_FORMAT_TEXT;
    }

    return FileObjectFormat;
}

/**
 *  This method is called to return the WPD content type of the specified object
 *
 *  The parameters sent to us are:
 *  strObjectID - the object whose WPD content type is being requested.
 *
 *  The driver should:
 *  Read the native content type of the object and return a WPD content type to the caller.
 */
GUID GetObjectContentType(_In_ LPCWSTR wszObjectID)
{
    GUID FileObjectFormat = WPD_CONTENT_TYPE_UNSPECIFIED;

    if (_wcsicmp(wszObjectID, README_FILE_OBJECT_ID) == 0)
    {
        FileObjectFormat = WPD_CONTENT_TYPE_DOCUMENT;
    }

    return FileObjectFormat;
}
