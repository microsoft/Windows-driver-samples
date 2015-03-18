#include "stdafx.h"

#include "WpdObjectProperties.tmh"

WpdObjectProperties::WpdObjectProperties() : m_pDevice(NULL)
{
}

WpdObjectProperties::~WpdObjectProperties()
{

}

HRESULT WpdObjectProperties::Initialize(_In_    FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pDevice = pDevice;
    return S_OK;
}

HRESULT WpdObjectProperties::DispatchWpdMessage(
    _In_     REFPROPERTYKEY         Command,
    _In_     IPortableDeviceValues* pParams,
    _In_     IPortableDeviceValues* pResults)
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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->GetSupportedProperties(Scope, wszObjectID, pKeys);
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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->GetPropertyValues(Scope, wszObjectID, pKeys, pValues);
        CHECK_HR(hr, "Failed to get property values for object '%ws'", wszObjectID);
    }

    // S_OK or S_FALSE can be returned from GetPropertyValues( ).
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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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

    // First we make a request for ALL supported property keys for the specified object.
    // Next, we delegate to our helper function GetPropertyValuesForObject( ) passing
    // the entire property key collection.  This will reuse existing implementation
    // in our driver to perform the GetAllPropertyValues operation.
    if (hr == S_OK)
    {
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->GetAllPropertyValues(Scope, wszObjectID, pValues);
        CHECK_HR(hr, "Failed to get all property values for object '%ws'", wszObjectID);
    }

    // S_OK or S_FALSE can be returned from GetAllPropertyValues( ).
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
        CHECK_HR(hrTemp, "Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_VALUES");

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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr             = S_OK;
    LPWSTR  wszObjectID    = NULL;
    bool    bObjectChanged = false;
    
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

    // CoCreate a collection to store the property set event parameters.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pEventParams);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Set the property values on the specified object
    if (hr == S_OK)
    {
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->SetPropertyValues(Scope, wszObjectID, pValues, pWriteResults, pEventParams, &bObjectChanged);
        CHECK_HR(hr, "Failed to set property values on object '%ws'", wszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        // Set the WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS value in the results
        HRESULT hrTemp = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS, pWriteResults);
        CHECK_HR(hrTemp, "Failed to set WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_WRITE_RESULTS");

        if (FAILED(hrTemp))
        {
            hr = hrTemp;
        }

        if (SUCCEEDED(hr) && bObjectChanged)
        {
            // Post the event indicating the object has changed
            hrTemp = PostWpdEvent(pParams, pEventParams);
            CHECK_HR(hrTemp, "Failed post event for updated object [%ws] (errors ignored)", wszObjectID);
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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);        
        hr = m_pDevice->GetPropertyAtributes(Scope, wszObjectID, Key, pAttributes);
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
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = E_ACCESSDENIED;

    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This driver has no properties which can be deleted.

    return hr;
}
