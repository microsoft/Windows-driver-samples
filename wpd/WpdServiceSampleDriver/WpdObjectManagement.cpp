#include "stdafx.h"

#include "WpdObjectManagement.tmh"

WpdObjectManagement::WpdObjectManagement() : m_pDevice(NULL)
{

}

WpdObjectManagement::~WpdObjectManagement()
{

}

HRESULT WpdObjectManagement::Initialize(_In_    FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pDevice = pDevice;
    return S_OK;
}

HRESULT WpdObjectManagement::DispatchWpdMessage(
    _In_     REFPROPERTYKEY         Command,
    _In_     IPortableDeviceValues* pParams,
    _In_     IPortableDeviceValues* pResults)
{

    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_OBJECT_MANAGEMENT)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
    }

    if (hr == S_OK)
    {
        if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY.pid)
        {
            hr = OnCreateObjectWithPropertiesOnly(pParams, pResults);
            CHECK_HR(hr, "Failed to create object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS.pid)
        {
            hr = OnDelete(pParams, pResults);
            CHECK_HR(hr, "Failed to delete object");
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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES: Contains an IPortableDeviceValues, describing
 *      properties of the new object.  At the very least, it will contain:
 *      -  WPD_OBJECT_NAME: The object name.
 *      -  WPD_PARENT_ID: Identifies the parent object.  The object should be inserted as a child of
 *         this parent (e.g. this would be the target directory in a file system based device).
 *
 *  The driver should:
 *  - Create the object, and return its ID in WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID.
 */
HRESULT WpdObjectManagement::OnCreateObjectWithPropertiesOnly(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  pszObjectID = NULL;

    CComPtr<IPortableDeviceValues>  pObjectProperties;
    CComPtr<IPortableDeviceValues>  pEventParams;

    hr = CoCreateInstance(CLSID_PortableDeviceValues,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDeviceValues,
                          (VOID**) &pEventParams);
    CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");

    // Get the Object Properties
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES, &pObjectProperties);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES");
    }

    // Save the object to the device here.
    if (SUCCEEDED(hr))
    {
        ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
        hr = m_pDevice->CreatePropertiesOnlyObject(Scope, pObjectProperties, pEventParams, &pszObjectID);
        CHECK_HR(hr, "Failed to save new (properties only) object to device");
    }

    if (SUCCEEDED(hr))
    {
        // Create is successful, so we post an event.  
        // This is best effort, so errors are ignored
        HRESULT hrEvent = PostWpdEvent(pParams, pEventParams);  
        CHECK_HR(hrEvent, "Failed post event for new object [%ws] (errors ignored)", pszObjectID);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID, pszObjectID);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS: the ObjectIDs, indicating which objects to delete.  These may
 *      contain children.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_OPTIONS: Flag parameter indicating delete options. Must be one
 *                  of the following:
 *                 - PORTABLE_DEVICE_DELETE_NO_RECURSION - Deletes the
 *                          object only.  This should fail if children exist.
 *                 - PORTABLE_DEVICE_DELETE_WITH_RECURSION - Deletes this
 *                          object and all children.
 *
 *  The driver should:
 *  - If the flag is PORTABLE_DEVICE_DELETE_NO_RECURSION the driver should delete the
 *    specified object only.  If the object still has children the driver should not delete
 *    the object and instead return HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION).
 *  - If the flag is PORTABLE_DEVICE_DELETE_WITH_RECURSION the driver should delete the
 *    specified object and all of its children.
 *  - Fill out the operation results in WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_RESULTS.  It contains an IPortableDevicePropVariantCollection of
 *    VT_ERROR values indicating the success or failure of the operation for that element.
 *    Order is implicit, i.e. the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_RESULTS corresponds to the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS and so on.
 *  - The driver should return:
 *      - S_OK if all objects were deleted successfully.
 *      - S_FALSE if any object delete failed.
 *      - An error return indicates that the driver did not delete any objects, and
 *        WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_RESULTS is ignored.
 */
HRESULT WpdObjectManagement::OnDelete(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{

    HRESULT hr              = S_OK;
    DWORD   dwOptions       = PORTABLE_DEVICE_DELETE_NO_RECURSION;
    BOOL    bDeleteFailed   = FALSE;
    VARTYPE vt              = VT_EMPTY;

    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;
    CComPtr<IPortableDevicePropVariantCollection> pDeleteResults;
    CComPtr<IPortableDeviceValues>                pEventParams;                

    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS, &pObjectIDs);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS");
    }

    // Ensure that this is a collection of VT_LPWSTR
    if (hr == S_OK)
    {
        hr = pObjectIDs->GetType(&vt);
        CHECK_HR(hr, "Failed to get the VARTYP of WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS");
        if (hr == S_OK)
        {
            if (vt != VT_LPWSTR)
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS is not a collection of VT_LPWSTR");
            }
        }
    }

    if (hr == S_OK)
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_OPTIONS, &dwOptions);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_OPTIONS");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pDeleteResults);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pEventParams);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        DWORD   cObjects    = 0;
        // Loop through the object list and attempt to delete
        hr = pObjectIDs->GetCount(&cObjects);
        CHECK_HR(hr, "Failed to get number of objects to delete");

        if (hr == S_OK)
        {
            ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);

            for(DWORD dwIndex = 0; dwIndex < cObjects; dwIndex++)
            {
                HRESULT hrTemp = S_OK;
                PROPVARIANT pv = {0};

                PropVariantInit(&pv);
                // Get the next Object to delete
                hr = pObjectIDs->GetAt(dwIndex, &pv);
                CHECK_HR(hr, "Failed to get next object id at index %d", dwIndex);
                if (hr == S_OK)
                {
                    HRESULT     hrDelete = S_OK;
                    PROPVARIANT pvResult = {0};

                    PropVariantInit(&pvResult);

                    hrDelete = m_pDevice->DeleteObject(Scope, dwOptions, pv.pwszVal, pEventParams);
                    CHECK_HR(hrDelete, "Failed to delete object [%ws]", pv.pwszVal);

                    if(FAILED(hrDelete))
                    {
                        bDeleteFailed = TRUE;
                    }
                    else
                    {
                        // Delete is successful, so we post an event.  
                        // This is best effort, so errors are ignored
                        HRESULT hrEvent = PostWpdEvent(pParams, pEventParams);  
                        CHECK_HR(hrEvent, "Failed post event for deleted object [%ws] (errors ignored)", pv.pwszVal);
                    }

                    // Clear event parameters for reuse
                    pEventParams->Clear();

                    // Save this result
                    pvResult.vt       = VT_ERROR;
                    pvResult.scode    = hrDelete;
                    hrTemp = pDeleteResults->Add(&pvResult);
                    PropVariantClear(&pvResult);
                    CHECK_HR(hrTemp, "Failed to add result for [%ws] to list of results", pv.pwszVal);

                    PropVariantClear(&pv);
                }
                else
                {
                    break;
                }
            }
        }
    }

    // Set the results
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_RESULTS, pDeleteResults);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_DELETE_RESULTS");
    }

    // If an object failed to delete, make sure we return S_FALSE
    if ((hr == S_OK) && (bDeleteFailed))
    {
        hr = S_FALSE;
    }

    return hr;
}
