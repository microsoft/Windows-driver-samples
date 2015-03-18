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

const PROPERTYKEY g_SupportedSensorProperties[] = 
{
    SENSOR_READING,
    SENSOR_UPDATE_INTERVAL,
    WPD_FUNCTIONAL_OBJECT_CATEGORY,
};


WpdObjectProperties::WpdObjectProperties() : 
    m_dwUpdateInterval(0), 
    m_llSensorReading(0)
{

}

WpdObjectProperties::~WpdObjectProperties()
{
    m_pBaseDriver = NULL;
}

HRESULT WpdObjectProperties::Initialize(_In_ WpdBaseDriver* pBaseDriver)
{
    m_pBaseDriver = pBaseDriver;
    return S_OK;
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

    // Free the memory.
    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

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

    // Free the memory.
    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

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

    // Free the memory.
    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

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
    HRESULT      hr          = S_OK;
    HRESULT      hrResult    = S_OK;
    LPWSTR       wszObjectID = NULL;
    DWORD        cValues     = 0;
    CAtlStringW  strObjectID;   

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
        strObjectID = wszObjectID;

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
                PROPVARIANT Value = {0};

                PropVariantInit(&Value);

                hr = pValues->GetAt(dwIndex, &Key, &Value);
                CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d", dwIndex);

                if (hr == S_OK)
                {
                    // TODO: Add other ...OBJECT_ID strings where applicable
                    if (
                           (strObjectID.CompareNoCase(SENSOR_OBJECT_ID) == 0)           ||
                           (strObjectID.CompareNoCase(TEMP_SENSOR_OBJECT_ID) == 0)      ||
                           (strObjectID.CompareNoCase(FLEX_SENSOR_OBJECT_ID) == 0)      ||
                           (strObjectID.CompareNoCase(PIR_SENSOR_OBJECT_ID) == 0)       ||
                           (strObjectID.CompareNoCase(PING_SENSOR_OBJECT_ID) == 0)      ||
                           (strObjectID.CompareNoCase(QTI_SENSOR_OBJECT_ID) == 0)       ||
                           (strObjectID.CompareNoCase(MEMSIC_SENSOR_OBJECT_ID) == 0)    ||
                           (strObjectID.CompareNoCase(HITACHI_SENSOR_OBJECT_ID) == 0)   ||
                           (strObjectID.CompareNoCase(PIEZO_SENSOR_OBJECT_ID) == 0)     ||
                           (strObjectID.CompareNoCase(COMPASS_SENSOR_OBJECT_ID) == 0)
                       )
                    {
                        if (IsEqualPropertyKey(Key, SENSOR_UPDATE_INTERVAL))
                        {
                            if (Value.vt == VT_UI4)
                            {
                                hr = SendUpdateIntervalToDevice(Value.ulVal);
                                CHECK_HR(hr, "Failed to send the new SENSOR_UPDATE_INTERVAL %d on the device", Value.ulVal);
                            }
                            else
                            {
                                // property failed to be set as it is an invalid vartype
                                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                                CHECK_HR(hr, "Failed to update the SENSOR_UPDATE_INTERVAL because the vartype is invalid.  Expected VT_UI4, got %d", Value.vt);
                            }

                            // An error has occurred, set the overall result to S_FALSE
                            if (hr != S_OK)
                            {
                                hrResult = S_FALSE;
                            }

                            hr = pWriteResults->SetErrorValue(Key, hr);
                            CHECK_HR(hr, "Failed to set error result value of SENSOR_UPDATE_INTERVAL for '%ws'", wszObjectID);
                        }
                        else
                        {
                            // Other properties for the sensor object are read only
                            hr = pWriteResults->SetErrorValue(Key, E_ACCESSDENIED);
                            CHECK_HR(hr, "Failed to set error result value at index %d for '%ws'", dwIndex, wszObjectID);
                            hrResult = S_FALSE;
                        }
                    } 
                    else
                    {
                        // Properties for all other objects are read only
                        hr = pWriteResults->SetErrorValue(Key, E_ACCESSDENIED);
                        CHECK_HR(hr, "Failed to set error result value at index %d for '%ws'", dwIndex, wszObjectID);                           
                        hrResult = S_FALSE;
                    }
                }

                PropVariantClear(&Value);

            } // end for
        } // end if
    }

    // At least one property failed to be set
    if (hrResult != S_OK)
    {
        hr = hrResult;
    }

    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS value in the results
        HRESULT hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS, pWriteResults);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS"));

        // Don't override the S_FALSE hresult that indicates at least one property had failed
        if (hr == S_OK)
        {
            hr = hrTemp;
        }
    }

    // Free the memory.
    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

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

    // Free the memory.
    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

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

    // Add other PROPERTYKEYs for other supported objects...
    // TODO: Add comparison to other ..._OBJECT_IDs where applicable
    if (
           (strObjectID.CompareNoCase(SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(TEMP_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(FLEX_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(PIR_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(PING_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(QTI_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(MEMSIC_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(HITACHI_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(PIEZO_SENSOR_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(COMPASS_SENSOR_OBJECT_ID) == 0)
       )
    {
        // Add the PROPERTYKEYs for the Sensor object
        AddSensorPropertyKeys(pKeys);
    }

    

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
 *  This method is called to populate PROPERTYKEYs found on the SENSOR object.
 *
 *  The parameters sent to us are:
 *  pKeys - An IPortableDeviceKeyCollection to be populated with PROPERTYKEYs
 *
 *  The driver should:
 *  Add PROPERTYKEYs pertaining to the DEVICE object.
 */
VOID AddSensorPropertyKeys(
    _In_ IPortableDeviceKeyCollection* pKeys)
{
    if (pKeys != NULL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedSensorProperties); dwIndex++)
        {
            HRESULT hr = S_OK;
            hr = pKeys->Add(g_SupportedSensorProperties[dwIndex] );
            CHECK_HR(hr, "Failed to add sensor property");
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

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_LEVEL))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_LEVEL, DEVICE_POWER_LEVEL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_LEVEL");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_SOURCE))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_SOURCE, WPD_POWER_SOURCE_EXTERNAL);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_SOURCE");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_PROTOCOL))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_PROTOCOL, DEVICE_PROTOCOL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_PROTOCOL");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_MODEL))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_MODEL, DEVICE_MODEL_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_MODEL");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_SERIAL_NUMBER))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_SERIAL_NUMBER, DEVICE_SERIAL_NUMBER_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_SERIAL_NUMBER");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_SUPPORTS_NON_CONSUMABLE))
                    {
                        hr = pValues->SetBoolValue(WPD_DEVICE_SUPPORTS_NON_CONSUMABLE, DEVICE_SUPPORTS_NONCONSUMABLE_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_SUPPORTS_NON_CONSUMABLE");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_MANUFACTURER))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_MANUFACTURER, DEVICE_MANUFACTURER_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_MANUFACTURER");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_FRIENDLY_NAME))
                    {
                        hr = pValues->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, DEVICE_FRIENDLY_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_FRIENDLY_NAME");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_DEVICE_TYPE))
                    {
                        hr = pValues->SetUnsignedIntegerValue(WPD_DEVICE_TYPE, WPD_DEVICE_TYPE_GENERIC);
                        CHECK_HR(hr, "Failed to set WPD_DEVICE_TYPE");
                    }

                    // Set general properties for DEVICE
                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        // Retrieves the "DEVICE" string that identifies the root device
                        hr = pValues->SetStringValue(WPD_OBJECT_NAME, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, L"");
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
                    {
                        hr = pValues->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_DEVICE);
                        CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
                    }
                }
            }
        }
        // Retrieve the temperature sensor properties
        else if (
                 (strObjectID.CompareNoCase(SENSOR_OBJECT_ID) == 0)         ||
                 (strObjectID.CompareNoCase(TEMP_SENSOR_OBJECT_ID) == 0)    ||
                 (strObjectID.CompareNoCase(FLEX_SENSOR_OBJECT_ID) == 0)    ||
                 (strObjectID.CompareNoCase(PIR_SENSOR_OBJECT_ID) == 0)     ||
                 (strObjectID.CompareNoCase(PING_SENSOR_OBJECT_ID) == 0)    ||
                 (strObjectID.CompareNoCase(QTI_SENSOR_OBJECT_ID) == 0)     ||
                 (strObjectID.CompareNoCase(MEMSIC_SENSOR_OBJECT_ID) == 0)  ||
                 (strObjectID.CompareNoCase(HITACHI_SENSOR_OBJECT_ID) == 0) ||
                 (strObjectID.CompareNoCase(PIEZO_SENSOR_OBJECT_ID) == 0)   ||
                 (strObjectID.CompareNoCase(COMPASS_SENSOR_OBJECT_ID) == 0)
                )
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
                        hr = pValues->SetStringValue(WPD_OBJECT_ID, strObjectID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
                    {

                        // Retrieve the ID of the sensor using the m_SensorType member of the
                        // basedriver that is set during the data-read operation.
                        hr = pValues->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, strObjectID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
                    {
                        hr = pValues->SetStringValue(WPD_OBJECT_PARENT_ID, WPD_DEVICE_OBJECT_ID);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
                    {
                        // Retrieve the name of the sensor using the m_SensorType member of the
                        // basedriver that is set during the data-read operation.
                        if (m_pBaseDriver->m_SensorType == 0)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 2)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, TEMP_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 3)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, FLEX_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 4)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, PING_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 5)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, PIR_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 6)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, MEMSIC_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 7)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, QTI_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 8)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, PIEZO_SENSOR_OBJECT_NAME_VALUE);
                        else if (m_pBaseDriver->m_SensorType == 9)
                            hr = pValues->SetStringValue(WPD_OBJECT_NAME, HITACHI_SENSOR_OBJECT_NAME_VALUE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
                    {
                        hr = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
                    }

                    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
                    {
                        hr = pValues->SetBoolValue(WPD_OBJECT_CAN_DELETE, FALSE);
                        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
                    }

                    else if (IsEqualPropertyKey(Key, SENSOR_READING))
                    {
                        hr = pValues->SetUnsignedLargeIntegerValue(SENSOR_READING, GetSensorReading());
                        CHECK_HR(hr, "Failed to set SENSOR_READING");
                    }

                    else if (IsEqualPropertyKey(Key, SENSOR_UPDATE_INTERVAL))
                    {
                        hr = pValues->SetUnsignedLargeIntegerValue(SENSOR_UPDATE_INTERVAL, GetUpdateInterval());
                        CHECK_HR(hr, "Failed to set SENSOR_UPDATE_INTERVAL");
                    }
                    else if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
                    {
                        hr = pValues->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, FUNCTIONAL_CATEGORY_SENSOR_SAMPLE);
                        CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
                    }
                }
             } // end for
        } // end else if
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

    //
    // Since ALL of our properties have the same attributes, we are ignoring the
    // passed in wszObjectID parameter.  This parameter allows you to
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
        // Allow writes for the update interval property
        if (IsEqualPropertyKey(Key, SENSOR_UPDATE_INTERVAL))
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE for SENSOR_UPDATE_INTERVAL");
        }
        else
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
        }
    }

    if (hr == S_OK)
    {
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY, TRUE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY");
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Key, SENSOR_UPDATE_INTERVAL))
        {
            // Form range attributes for the update interval property
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_RANGE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MIN for SENSOR_UPDATE_INTERVAL");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MIN, SENSOR_UPDATE_INTERVAL_MIN);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MIN for SENSOR_UPDATE_INTERVAL");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MAX, SENSOR_UPDATE_INTERVAL_MAX);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MAX for SENSOR_UPDATE_INTERVAL");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_STEP, SENSOR_UPDATE_INTERVAL_STEP);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_STEP for SENSOR_UPDATE_INTERVAL");
        }
        else if (IsEqualPropertyKey(Key, SENSOR_READING))
        {
            // Form range attributes for the reading property
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_RANGE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MIN for SENSOR_READING");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MIN, SENSOR_READING_MIN);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MIN for SENSOR_READING");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MAX, SENSOR_READING_MAX);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MAX for SENSOR_READING");

            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_STEP, SENSOR_READING_STEP);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_STEP for SENSOR_READING");   
        }
        else
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_UNSPECIFIED);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM");
        }
    }

    return hr;
}


/**
 *  This method is called to update the sensor reading
 *
 *  The parameters sent to us are:
 *  dwNewReading - the temperature reading to set
 *
 *  The driver should:
 *  Update the sensor reading.
 */
VOID WpdObjectProperties::SetSensorReading(LONGLONG llNewReading)
{    
    // Ensure that this value isn't currently being accessed by another thread
    CComCritSecLock<CComAutoCriticalSection> Lock(m_SensorReadingCriticalSection);

    m_llSensorReading = llNewReading;
}


/**
 *  This method is called to retrieve the sensor reading
 *
 *  The parameters sent to us are:
 *
 *  The driver should:
 *  Return the saved sensor reading.
 */
LONGLONG WpdObjectProperties::GetSensorReading()
{    
    // Ensure that this value isn't currently being accessed by another thread
    CComCritSecLock<CComAutoCriticalSection> Lock(m_SensorReadingCriticalSection);

    return m_llSensorReading;
}


/**
 *  This method is called to set the cached sensor interval  
 *
 *  The parameters sent to us are:
 *  dwNewInterval - the sensor update interval to set
 *
 *  The driver should:
 *  Update the cached sensor interval property.
 */
VOID WpdObjectProperties::SetUpdateInterval(DWORD dwNewInterval)
{    
    m_dwUpdateInterval = dwNewInterval;
}


/**
 *  This method is called to retrieve the sensor update interval
 *
 *  The parameters sent to us are: 
 *
 *  The driver should:
 *  Return the interval property.
 */
DWORD WpdObjectProperties::GetUpdateInterval()
{    
    return m_dwUpdateInterval;
}


/**
 *  This method is called to update the sensor interval on the device
 *
 *  The parameters sent to us are:
 *  dwNewInterval - the sensor update interval to set
 *
 *  The driver should:
 *  Update the cached sensor property and send a write request to the device
 */
HRESULT WpdObjectProperties::SendUpdateIntervalToDevice(DWORD dwNewInterval)
{    
    HRESULT      hr                           = S_OK;
    RS232Target* pDeviceTarget                = NULL;

    CHAR  szInterval[INTERVAL_DATA_LENGTH+1]  = {0};

    // Check the input value
    if (IsValidUpdateInterval(dwNewInterval) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HR(hr, "Invalid update interval: %d", dwNewInterval);
    }

    // Format a write request with the input value
    if (hr == S_OK)
    {
        hr = StringCchPrintfA(szInterval, ARRAYSIZE(szInterval), "%u", dwNewInterval);
        CHECK_HR(hr, "Failed to convert the new interval to a CHAR string");
    }

    // Send the write request to the device
    if (hr == S_OK)
    {
        pDeviceTarget = m_pBaseDriver->GetRS232Target();

        if (pDeviceTarget->IsReady())
        {
            hr = pDeviceTarget->SendWriteRequest((BYTE *)szInterval, sizeof(szInterval));
            CHECK_HR(hr, "Failed to send the write request to set the new sensor update interval");

            if (hr == S_OK)
            {
                TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_FLAG_DRIVER, "%!FUNC! Sent new interval: %s", szInterval);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
            CHECK_HR(hr, "Device is not ready to receive write requests");
        }
    }
    
    if (hr == S_OK)
    {
        // Update the cached value on the driver
        SetUpdateInterval(dwNewInterval);
    }

    return hr;
}


/**
 *  This method is called to check that interval value falls within the accepted range
 *  of 2000 to 60000 milliseconds
 *
 *  The parameters sent to us are: 
 *  wszInterval - the sensor update interval to check
 *
 *  The driver should:
 *  Return TRUE if the interval is within range
 */
BOOL WpdObjectProperties::IsValidUpdateInterval(DWORD dwInterval)
{
    if ((dwInterval >= SENSOR_UPDATE_INTERVAL_MIN) && 
        (dwInterval <= SENSOR_UPDATE_INTERVAL_MAX))
    {
        return TRUE;
    }

    return FALSE;
}

