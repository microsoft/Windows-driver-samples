#include "stdafx.h"
#include "WpdObjectProperties.tmh"

WpdObjectProperties::WpdObjectProperties()
{

}

WpdObjectProperties::~WpdObjectProperties()
{

}

HRESULT WpdObjectProperties::Initialize(_In_ FakeDevice *pFakeDevice)
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


HRESULT WpdObjectProperties::DispatchWpdMessage(_In_    REFPROPERTYKEY          Command,
                                                _In_    IPortableDeviceValues*  pParams,
                                                _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_OBJECT_PROPERTIES)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
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
            hr = OnGetValues(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to read properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL))
        {
            hr = OnGetAllValues(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to read all properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_SET))
        {
            hr = OnWriteProperties(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to write properties");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES))
        {
            hr = OnGetAttributes(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to get property attributes");
            }
        }
        else if(IsEqualPropertyKey(Command, WPD_COMMAND_OBJECT_PROPERTIES_DELETE))
        {
            hr = OnDelete(pParams, pResults);
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
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose properties we want to return.
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_FILTER: the filter to use when returning supported properties.
 *      Since this parameter is optional, it may not exist.
 *      ! This driver currently ignores the filter parameter. !
 *
 *  The driver should:
 *  - Return all properties that match the filter.
 */
HRESULT WpdObjectProperties::OnGetSupportedProperties(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                  = S_OK;
    LPWSTR  pszObjectID         = NULL;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &pszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing string value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    // Get the properties to read
    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetSupportedProperties(pszObjectID, &pKeys);
        CHECK_HR(hr, "Failed to get property keys collection");
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property values we want to return.
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
HRESULT WpdObjectProperties::OnGetValues(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT             hr          = S_OK;
    LPWSTR              wszObjectID = NULL;

    CComPtr<IPortableDeviceValues>        pValueStore;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, &pKeys);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetValues(wszObjectID, pKeys, &pValueStore);
        CHECK_HR(hr, "Failed to read properties on [%ws]", wszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;

        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, pValueStore);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));

        if(FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

    CoTaskMemFree(wszObjectID);
    wszObjectID = NULL;

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID: identifies the object whose property values we want to return.
 *
 *  The driver should:
 *  - Return all property values in WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES.  If any property read failed, the corresponding value should be
 *      set to type VT_ERROR with the 'scode' member holding the HRESULT reason for the failure.
 *  - S_OK should be returned if all properties were read successfully.
 *  - S_FALSE should be returned if any property read failed.
 *  - Any error return indicates that the driver did not fill in any results, and the caller will
 *      not attempt to unpack any property values.
 */
HRESULT WpdObjectProperties::OnGetAllValues(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT             hr          = S_OK;
    LPWSTR              wszObjectID = NULL;

    CComPtr<IPortableDeviceValues>        pValueStore;

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &wszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetAllValues(wszObjectID, &pValueStore);
        CHECK_HR(hr, "Failed to read all properties on [%ws]", wszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;

        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, pValueStore);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES"));

        if(FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

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
HRESULT WpdObjectProperties::OnWriteProperties(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  pszObjectID = NULL;
    CComPtr<IPortableDeviceValues>  pValues;
    CComPtr<IPortableDeviceValues>  pWriteResults;
    CComPtr<IPortableDeviceValues>  pEventParams;

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &pszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES, &pValues);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES");
    }

    if (hr == S_OK)
    {
        hr = m_pFakeDevice->WritePropertiesOnObject(pszObjectID, pValues, &pWriteResults);
        if(FAILED(hr))
        {
            CHECK_HR(hr, "Failed to write properties on [%ws]", pszObjectID);
        }
    }

    // Be sure to preserve possible S_FALSE returns
    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;
        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS, pWriteResults);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS"));

        if(FAILED(hrTemp))
        {
            hr = hrTemp;
        }

        pEventParams = NULL;
        hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pszObjectID, &pEventParams);
        CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pszObjectID);

        if (hrTemp == S_OK)
        {
            HRESULT hrEvent = S_OK;
            // Set the event-specific parameters
            hrEvent = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_UPDATED);
            CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

            // Send the Event
            if (hrEvent == S_OK)
            {
                PostWpdEvent(pParams, pEventParams);
            }
        }
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);

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
HRESULT WpdObjectProperties::OnGetAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  pszObjectID = NULL;

    PROPERTYKEY         Key = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues>  pAttributeStore;

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &pszObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS, &Key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS");
    }

    if (hr == S_OK)
    {
        hr = m_pFakeDevice->GetAttributes(pszObjectID, Key, &pAttributeStore);
        CHECK_HR(hr, "Failed to get attributes on [%ws]", pszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = S_OK;

        hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_ATTRIBUTES, pAttributeStore);
        CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_ATTRIBUTES"));

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
HRESULT WpdObjectProperties::OnDelete(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = E_ACCESSDENIED;

    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This driver has no properties which can be deleted.
    return hr;
}

