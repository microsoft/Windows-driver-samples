#include "stdafx.h"
#include "WpdObjectManagement.tmh"

WpdObjectManagement::WpdObjectManagement()
{

}

WpdObjectManagement::~WpdObjectManagement()
{

}

HRESULT WpdObjectManagement::Initialize(_In_    FakeDevice *pFakeDevice)
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

HRESULT WpdObjectManagement::DispatchWpdMessage(
    _In_    REFPROPERTYKEY          Command,
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
        if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS.pid)
        {
            hr = OnDelete(pParams, pResults);
            CHECK_HR(hr, "Failed to delete object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY.pid)
        {
            hr = OnCreateObjectWithPropertiesOnly(pParams, pResults);
            CHECK_HR(hr, "Failed to create object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_AND_DATA.pid)
        {
            hr = OnCreateObjectWithPropertiesAndData(pParams, pResults);
            CHECK_HR(hr, "Failed to create object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_WRITE_OBJECT_DATA.pid)
        {
            hr = OnWriteObjectData(pParams, pResults);
            CHECK_HR(hr, "Failed to write object data");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_COMMIT_OBJECT.pid)
        {
            hr = OnCommit(pParams, pResults);
            CHECK_HR(hr, "Failed to commit object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_REVERT_OBJECT.pid)
        {
            hr = OnRevert(pParams, pResults);
            CHECK_HR(hr, "Failed to revert object");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_MOVE_OBJECTS.pid)
        {
            hr = OnMove(pParams, pResults);
            CHECK_HR(hr, "Failed to Move objects");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_COPY_OBJECTS.pid)
        {
            hr = OnCopy(pParams, pResults);
            CHECK_HR(hr, "Failed to Copy objects");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_MANAGEMENT_UPDATE_OBJECT_WITH_PROPERTIES_AND_DATA.pid)
        {
            hr = OnUpdateObjectWithPropertiesAndData(pParams, pResults);
            CHECK_HR(hr, "Failed to update object");
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
    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;
    CComPtr<IPortableDevicePropVariantCollection> pDeleteResults;
    CComPtr<IPortableDeviceValues> pEventParams;
    VARTYPE vt              = VT_EMPTY;

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
        DWORD   cObjects    = 0;
        // Loop through the object list and attempt to delete
        hr = pObjectIDs->GetCount(&cObjects);
        CHECK_HR(hr, "Failed to get number of objects to delete");

        if (hr == S_OK)
        {
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
                    pEventParams = NULL;
                    // Get the object properties used for sending the event.  Ignore errors (these are expected in some cases
                    // e.g. app is sending object ID that doesn't exist), since this relates to the event, not the delete operation,
                    // and we return results for the delete (an error posting the event is non-fatal).
                    hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pv.pwszVal, &pEventParams);
                    CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pv.pwszVal);

                    HRESULT     hrDelete = S_OK;
                    PROPVARIANT pvResult = {0};

                    PropVariantInit(&pvResult);

                    hrDelete = m_pFakeDevice->DeleteObject(dwOptions, pv.pwszVal);
                    CHECK_HR(hrDelete, "Failed to delete object [%ws]", pv.pwszVal);

                    if(FAILED(hrDelete))
                    {
                        bDeleteFailed = TRUE;
                    }

                    // Save this result
                    pvResult.vt       = VT_ERROR;
                    pvResult.scode    = hrDelete;
                    hrTemp = pDeleteResults->Add(&pvResult);
                    PropVariantClear(&pvResult);
                    CHECK_HR(hrTemp, "Failed to add result for [%ws] to list of results", pv.pwszVal);

                    if ((hrDelete == S_OK) && (hrTemp == S_OK))
                    {
                        HRESULT hrEvent = S_OK;
                        // Set the event-specific parameters
                        hrEvent = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_REMOVED);
                        CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

                        // Send the Event
                        if (hrEvent == S_OK)
                        {
                            PostWpdEvent(pParams, pEventParams);
                        }
                    }
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

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_MOVE_OBJECTS
 *  command.
 *
 * This command will move the specified objects to the destination folder.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS : the ObjectIDs, indicating which objects to move.  These may
 *      be folder objects.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID: Indicates the destination folder for the move operation.
 *
 *  The driver should:
 *  - Attempt to Move the object specified in WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS to the folder specified in
 *    WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID.
 * - Fill out the operation results in WPD_PROPERTY_OBJECT_MANAGEMENT_MOVE_RESULTS.  It contains an IportableDevicePropVariantCollection of
 *   VT_ERROR values indicating the success or failure of the operation for that element.
 *   Order is implicit, i.e. the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_MOVE_RESULTS corresponds to the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS and so on.
 *  - The driver should return:
 *      - S_OK if all objects were moved successfully.
 *      - S_FALSE if any object move failed.
 *      - An error return indicates that the driver did not move any objects, and
 *        WPD_PROPERTY_OBJECT_MANAGEMENT_MOVE_RESULTS is ignored.
 */
HRESULT WpdObjectManagement::OnMove(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                    = S_OK;
    LPWSTR  pszDestFolderObjectID = NULL;
    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;
    CComPtr<IPortableDevicePropVariantCollection> pMoveResults;
    BOOL    bMoveFailed           = FALSE;
    VARTYPE vt                    = VT_EMPTY;

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
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID, &pszDestFolderObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pMoveResults);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection");
    }


    if (hr == S_OK)
    {
        DWORD   cObjects    = 0;
        // Loop through the object list and attempt to move
        hr = pObjectIDs->GetCount(&cObjects);
        CHECK_HR(hr, "Failed to get number of objects to move");

        if (hr == S_OK)
        {
            for(DWORD dwIndex = 0; dwIndex < cObjects; dwIndex++)
            {
                HRESULT hrTemp = S_OK;
                PROPVARIANT pv = {0};

                PropVariantInit(&pv);
                // Get the next Object to move
                hrTemp = pObjectIDs->GetAt(dwIndex, &pv);
                CHECK_HR(hr, "Failed to get next object id at index %d", dwIndex);
                if (hrTemp == S_OK)
                {
                    // Move this object
                    hrTemp = m_pFakeDevice->MoveObject(pv.pwszVal, pszDestFolderObjectID);
                    CHECK_HR(hrTemp, "Failed to move object [%ws] to folder [%ws]", pv.pwszVal, pszDestFolderObjectID);

                    if(FAILED(hrTemp))
                    {
                        bMoveFailed = TRUE;
                    }

                    PROPVARIANT pvResult = {0};

                    PropVariantInit(&pvResult);

                    // Save this result
                    pvResult.vt       = VT_ERROR;
                    pvResult.scode    = hrTemp;
                    hrTemp = pMoveResults->Add(&pvResult);
                    CHECK_HR(hrTemp, "Failed to add result for [%ws] to list of results", pv.pwszVal);

                    PropVariantClear(&pvResult);
                }
                PropVariantClear(&pv);
            }
        }
    }

    // Set the results
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_MANAGEMENT_MOVE_RESULTS, pMoveResults);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_MOVE_RESULTS");
    }

    CoTaskMemFree(pszDestFolderObjectID);

    // If an object failed to move, make sure we return S_FALSE
    if ((hr == S_OK) && (bMoveFailed))
    {
        hr = S_FALSE;
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_COPY_OBJECTS
 *  command.
 *
 * This command will copy the specified objects to the destination folder.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS : the ObjectIDs, indicating which objects to copy.  These may
 *      be folder objects.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID: Indicates the destination folder for the copy operation.
 *
 *  The driver should:
 *  - Attempt to copy the object specified in WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS to the folder specified in
 *    WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID.
 * - Fill out the operation results in WPD_PROPERTY_OBJECT_MANAGEMENT_COPY_RESULTS.  It contains an IPortableDevicePropVariantCollection of
 *   VT_ERROR values indicating the success or failure of the operation for that element.
 *   Order is implicit, i.e. the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_COPY_RESULTS corresponds to the first element of WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS and so on.
 *  - The driver should return:
 *      - S_OK if all objects were copied successfully.
 *      - S_FALSE if any object copy failed.
 *      - An error return indicates that the driver did not copy any objects, and
 *        WPD_PROPERTY_OBJECT_MANAGEMENT_COPY_RESULTS is ignored.
 */
HRESULT WpdObjectManagement::OnCopy(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                    = S_OK;
    LPWSTR  pszDestFolderObjectID = NULL;
    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;
    CComPtr<IPortableDevicePropVariantCollection> pCopyResults;
    BOOL    bCopyFailed           = FALSE;
    VARTYPE vt                    = VT_EMPTY;

    hr = pParams->GetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS, &pObjectIDs);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_IDS");

    // Ensure that this is a collection of VT_LPWSTR
    if (SUCCEEDED(hr))
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

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID, &pszDestFolderObjectID);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_DESTINATION_FOLDER_OBJECT_ID");
    }

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pCopyResults);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection");
    }


    if (SUCCEEDED(hr))
    {
        DWORD   cObjects    = 0;
        // Loop through the object list and attempt to copy
        hr = pObjectIDs->GetCount(&cObjects);
        CHECK_HR(hr, "Failed to get number of objects to copy");

        if (SUCCEEDED(hr))
        {
            for(DWORD dwIndex = 0; dwIndex < cObjects; dwIndex++)
            {
                CComPtr<IPortableDeviceValues> pEventParams;
                LPWSTR  pszNewObjectID = NULL;
                HRESULT hrTemp = S_OK;
                PROPVARIANT pv = {0};

                PropVariantInit(&pv);
                // Get the next Object to copy
                hrTemp = pObjectIDs->GetAt(dwIndex, &pv);
                CHECK_HR(hr, "Failed to get next object id at index %d", dwIndex);
                if (SUCCEEDED(hrTemp))
                {
                    // Copy this object
                    hrTemp = m_pFakeDevice->CopyObject(pv.pwszVal, pszDestFolderObjectID, &pszNewObjectID);
                    CHECK_HR(hrTemp, "Failed to copy object [%ws] to folder [%ws]", pv.pwszVal, pszDestFolderObjectID);

                    if(FAILED(hrTemp))
                    {
                        bCopyFailed = TRUE;
                    }

                    PROPVARIANT pvResult = {0};

                    PropVariantInit(&pvResult);

                    // Save this result
                    pvResult.vt       = VT_ERROR;
                    pvResult.scode    = hrTemp;
                    hrTemp = pCopyResults->Add(&pvResult);
                    CHECK_HR(hrTemp, "Failed to add result for [%ws] to list of results", pv.pwszVal);

                    PropVariantClear(&pvResult);

                    if (SUCCEEDED(hrTemp))
                    {
                        // Send the event
                        // Get the object properties used for sending the event
                        hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pszNewObjectID, &pEventParams);
                        CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pszNewObjectID);
                        if (SUCCEEDED(hrTemp))
                        {
                            // Set the event-specific parameters
                            hrTemp = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_ADDED);
                            CHECK_HR(hrTemp, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

                            // Send the Event
                            if (SUCCEEDED(hrTemp))
                            {
                                PostWpdEvent(pParams, pEventParams);
                            }
                        }
                    }
                }

                if (pszNewObjectID != NULL)
                {
                    CoTaskMemFree(pszNewObjectID);
                    pszNewObjectID = NULL;
                }

                PropVariantClear(&pv);
            }
        }
    }

    // Set the results
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_MANAGEMENT_COPY_RESULTS, pCopyResults);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_COPY_RESULTS");
    }

    CoTaskMemFree(pszDestFolderObjectID);

    // If an object failed to copy, make sure we return S_FALSE
    if (SUCCEEDED(hr) && bCopyFailed)
    {
        hr = S_FALSE;
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
    HRESULT                         hr              = S_OK;
    LPWSTR                          pszContext      = NULL;
    CComPtr<IPortableDeviceValues>  pValues;
    CComPtr<IPortableDeviceValues>  pEventParams;

    // Get the Object Properties
    hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES, &pValues);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES");

    // Save the object to the device here.
    if (SUCCEEDED(hr))
    {
        hr = m_pFakeDevice->SaveNewObject(pValues, &pszContext);
        CHECK_HR(hr, "Failed to save new (properties only) object to device");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID, pszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");
    }

    if (SUCCEEDED(hr))
    {
        // Send the event
        HRESULT hrTemp = S_OK;
        // Get the object properties used for sending the event
        hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pszContext, &pEventParams);
        CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pszContext);
        if (SUCCEEDED(hrTemp))
        {
            // Set the event-specific parameters
            hrTemp = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_ADDED);
            CHECK_HR(hrTemp, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

            // Send the Event
            if (SUCCEEDED(hrTemp))
            {
                PostWpdEvent(pParams, pEventParams);
            }
        }
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_AND_DATA
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES: Contains an IPortableDeviceValues, describing
 *      properties of the new object.  At the very least, it will contain:
 *      -  WPD_OBJECT_NAME: The object name.
 *      -  WPD_PARENT_ID: Identifies the parent object.  The object should be inserted as a child of
 *         this parent (e.g. this would be the target directory in a file system based device).
 *      -  WPD_OBJECT_SIZE: The total size of the object data stream.
 *      -  WPD_OBJECT_FORMAT: The format the object data stream.
 *
 *  The driver should:
 *  - Create a context for this operation and return it in WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT.
 *  - Return the optimal transfer buffer size in WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE.
 */
HRESULT WpdObjectManagement::OnCreateObjectWithPropertiesAndData(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                         hr              = S_OK;
    LPWSTR                          pszContext      = NULL;
    LPWSTR                          pszObjectName   = NULL;
    IUnknown*                       pContextMap     = NULL;
    CComPtr<IPortableDeviceValues>  pValues;

    // Get the Object Properties
    hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES, &pValues);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES");

    // Get Object Name from pValues
    if (SUCCEEDED(hr))
    {
        hr = pValues->GetStringValue(WPD_OBJECT_NAME, &pszObjectName);
        CHECK_HR(hr, "Failed to get WPD_OBJECT_NAME");
    }

    // Get the context map
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, &pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        // Object data will be provided by the caller.  So Create a context identifying this
        // creation request, since the actual data will come later, and the object can only be saved
        // when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_COMMIT_OBJECT call.
        hr = CreateObjectManagementContext((ContextMap*) pContextMap, pszObjectName, pValues, false /*bUpdateRequest*/, &pszContext);
        CHECK_HR(hr, "Failed to create object management context");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT, pszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT");
    }

    // Set the optimal buffer size
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE value");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectName);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_REVERT_OBJECT
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT: identifies the object creation request (driver returned this in
 *          WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT).
 *
 *  The driver should:
 *  - Free any resources associated with this context/operation.  The object should not be saved to the
 *    device.  If a placeholder was created on the device for this operation, it should be removed.
 */
HRESULT WpdObjectManagement::OnRevert(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr          = S_OK;
    LPWSTR      pszContext  = NULL;
    ContextMap*   pContextMap = NULL;

    // There are no return parameters for this operation.
    UNREFERENCED_PARAMETER(pResults);

    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT, &pszContext);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        hr = DestroyObjectManagementContext(pContextMap, pszContext);
        CHECK_HR(hr, "Failed to revert object identified by context [%ws]", pszContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_COMMIT_OBJECT
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT: identifies the object creation request (driver returned this in
 *          WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT).
 *
 *  The driver should:
 *  - Commit the object to the device.
 *  - Return the new/updated ObjectID in WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID.
 */
HRESULT WpdObjectManagement::OnCommit(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                     hr          = S_OK;
    LPWSTR                      pszContext  = NULL;
    ObjectManagementContext*    pContext    = NULL;
    ContextMap*                 pContextMap = NULL;
    CComPtr<IPortableDeviceValues> pEventParams;

    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT, &pszContext);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");

    // Get the object management context for this request
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, pszContext, (IUnknown**)&pContext);
        CHECK_HR(hr, "Failed to get Object Management Context");
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        if (pContext->UpdateRequest)
        {
            // Update the object
            hr = UpdateObject(pContext, pParams, pResults);
        }
        else
        {
            // Save the new object
            hr = CommitNewObject(pContext, pszContext, pParams, pResults);
        }            
    }

    // We're done with the context
    SAFE_RELEASE(pContext);

    // Destroy the context associated with this request, since it is no longer needed
    if (SUCCEEDED(hr))
    {
        hr = DestroyObjectManagementContext(pContextMap, pszContext);
        CHECK_HR(hr, "Failed to destroy object identified by context [%ws]", pszContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_WRITE_OBJECT_DATA
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT: identifies the object creation request previsouly started with
 *          WPD_PROPERTY_OBJECT_MANAGEMENT_CREATE_OBJECT.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_TO_WRITE: specifies the next number of bytes to write.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_DATA: specifies byte array where the data should be copied from.
 *
 *  The driver should:
 *  - Write the next WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_TO_WRITE to the resource.
 *  - Return the number of bytes actually written in WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_WRITTEN.
 *    It is normally considered an error if this value does not match WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_TO_WRITE.
 */
HRESULT WpdObjectManagement::OnWriteObjectData(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                     hr                  = S_OK;
    LPWSTR                      pszContext          = NULL;
    DWORD                       dwNumBytesToWrite   = 0;
    DWORD                       dwNumBytesWritten   = 0;
    BYTE*                       pBuffer             = NULL;
    DWORD                       cbBuffer            = 0;
    ObjectManagementContext*    pContext            = NULL;

    // Get the Context string
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT, &pszContext);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT");

    // Get the number of bytes to write
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_TO_WRITE, &dwNumBytesToWrite);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_TO_WRITE");
    }

    // Get the source buffer
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetBufferValue(WPD_PROPERTY_OBJECT_MANAGEMENT_DATA, &pBuffer, &cbBuffer);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_DATA");
    }

    // Get the resource context for this object creation request
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, pszContext, (IUnknown**)&pContext);
        CHECK_HR(hr, "Failed to get Object Management Context");
    }

    // Write the next band of data for this new object request
    if (SUCCEEDED(hr))
    {
        // TBD:  Write bytes to object.  This sample driver ignores the data content.
        dwNumBytesWritten = dwNumBytesToWrite;
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_WRITTEN, dwNumBytesWritten);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_NUM_BYTES_WRITTEN");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszContext);

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pBuffer);

    SAFE_RELEASE(pContext);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_UPDATE_OBJECT_WITH_PROPERTIES_AND_DATA
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID: Identifies the object to update.
 *  - WPD_PROPERTY_OBJECT_MANAGEMENT_UPDATE_PROPERTIES: Contains an IPortableDeviceValues, describing
 *      the updated properties of the object.  At the very least, it will contain:
 *      -  WPD_OBJECT_SIZE: The total size of the object data stream.
 *
 *  The driver should:
 *  - Create a context for this operation and return it in WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT.
 *  - Return the optimal transfer buffer size in WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE.
 */
HRESULT WpdObjectManagement::OnUpdateObjectWithPropertiesAndData(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                         hr              = S_OK;
    LPWSTR                          pszContext      = NULL;
    LPWSTR                          pszObjectId     = NULL;
    IUnknown*                       pContextMap     = NULL;
    BOOL                            bUpdatable      = NULL;
    CComPtr<IPortableDeviceValues>  pValues;

    // Get the Object Identifier.  
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID, &pszObjectId);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");

    // Get the Object Properties
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_OBJECT_MANAGEMENT_UPDATE_PROPERTIES, &pValues);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_MANAGEMENT_CREATION_PROPERTIES");
    }

    // Get the context map
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, &pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Validate whether this object supports the default resource
    if (SUCCEEDED(hr))
    {
        hr = m_pFakeDevice->SupportsResource(pszObjectId, WPD_RESOURCE_DEFAULT, &bUpdatable);
        CHECK_HR(hr, "Failed to check whether object supports WPD_RESOURCE_DEFAULT");

        if (SUCCEEDED(hr) && !bUpdatable)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Object does not support WPD_RESOURCE_DEFAULT");
        }
    }

    if (SUCCEEDED(hr))
    {
        // Object data will be provided by the caller.  So Create a context identifying this
        // creation request, since the actual data will come later, and the object can only be saved
        // when we receive a WPD_COMMAND_OBJECT_MANAGEMENT_COMMIT_OBJECT call.
        hr = CreateObjectManagementContext((ContextMap*) pContextMap, pszObjectId, pValues, true /*bUpdateRequest*/, &pszContext);
        CHECK_HR(hr, "Failed to create object management context");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT, pszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_CONTEXT");
    }

    // Set the optimal buffer size
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OPTIMAL_TRANSFER_BUFFER_SIZE value");
    }

    CoTaskMemFree(pszContext);
    CoTaskMemFree(pszObjectId);

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT WpdObjectManagement::CreateObjectManagementContext(
    _In_    ContextMap*             pContextMap,
    _In_    LPCWSTR                 pszObjectName,
    _In_    IPortableDeviceValues*  pObjectProperties,
    _In_    BOOL                    bUpdateRequest,
    _Outptr_result_nullonfailure_   LPWSTR* ppszObjectManagementContext)
{
    HRESULT                     hr              = S_OK;
    GUID                        guidContext     = GUID_NULL;
    ObjectManagementContext*    pContext        = NULL;
    CComBSTR    bstrContext;

    if((pContextMap                 == NULL) ||
       (pObjectProperties           == NULL) ||
       (pszObjectName               == NULL) ||
       (ppszObjectManagementContext == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppszObjectManagementContext = NULL;

    hr = CoCreateGuid(&guidContext);
    if (SUCCEEDED(hr))
    {
        bstrContext = guidContext;
        if(bstrContext.Length() == 0)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to create BSTR from GUID");
        }
    }

    if (SUCCEEDED(hr))
    {
        pContext = new ObjectManagementContext();
        if(pContext != NULL)
        {
            CAtlStringW strKey = bstrContext;
            pContext->Name              = pszObjectName;
            pContext->ObjectProperties  = pObjectProperties;
            pContext->UpdateRequest     = bUpdateRequest;

            hr = pContextMap->Add(strKey, pContext);
            CHECK_HR(hr, "Failed to add object creation context to client context map");

            pContext->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate object creation context");
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppszObjectManagementContext = AtlAllocTaskWideString(bstrContext);
    }

    return hr;
}

HRESULT WpdObjectManagement::DestroyObjectManagementContext(
    _In_    ContextMap* pContextMap,
    _In_    LPCWSTR     pszObjectManagementContext)
{
    HRESULT     hr  = S_OK;

    CAtlStringW strKey = pszObjectManagementContext;
    pContextMap->Remove(strKey);

    return hr;
}

HRESULT WpdObjectManagement::CommitNewObject(
    _In_    ObjectManagementContext*    pContext,
    _In_    LPCWSTR                     pszContext,
    _In_    IPortableDeviceValues*      pParams,
    _In_    IPortableDeviceValues*      pResults)
{
    LPWSTR pszObjectID = NULL;

    // Save the new object
    HRESULT hr = m_pFakeDevice->SaveNewObject(pContext->ObjectProperties, &pszObjectID);
    CHECK_HR(hr, "Failed to save new object [%ws] to device", pContext->Name);

    // Let the caller know the new object's ID.
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID, pszObjectID);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");
    }

    if (SUCCEEDED(hr))
    {
        // Send the event
        HRESULT hrTemp = S_OK;
        CComPtr<IPortableDeviceValues> pEventParams;

        // Get the object properties used for sending the event
        hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pszObjectID, &pEventParams);
        CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pszObjectID);

        if (SUCCEEDED(hrTemp))
        {
            // Set the event-specific parameters
            hrTemp = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_ADDED);
            CHECK_HR(hrTemp, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

            hrTemp = pEventParams->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_CREATION_COOKIE, pszContext);
            CHECK_HR(hrTemp, "Failed to add WPD_EVENT_PARAMETER_OBJECT_CREATION_COOKIE");
            
            // Send the Event
            if (SUCCEEDED(hr))
            {
                PostWpdEvent(pParams, pEventParams);
            }
        }
    }
    
    CoTaskMemFree(pszObjectID);

    return hr;
}

HRESULT WpdObjectManagement::UpdateObject(
    _In_    ObjectManagementContext*    pContext,
    _In_    IPortableDeviceValues*      pParams,
    _In_    IPortableDeviceValues*      pResults)
{
    // For convenience, the ObjectID was stored in Name for object updates
    LPCWSTR pszObjectId = pContext->Name;
    
    // Update the object
    HRESULT hr = m_pFakeDevice->UpdateContentObject(pszObjectId, pContext->ObjectProperties);
    CHECK_HR(hr, "Failed to update object [%ws] to device", pszObjectId);

    // Set the object ID.
    if (hr == S_OK)
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID, pszObjectId);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_MANAGEMENT_OBJECT_ID");
    }

    if (hr == S_OK)
    {
        // Send the event
        HRESULT hrTemp = S_OK;
        CComPtr<IPortableDeviceValues> pEventParams;

        // Get the object properties used for sending the event
        hrTemp = m_pFakeDevice->GetObjectPropertiesForEvent(pContext->Name, &pEventParams);
        CHECK_HR(hrTemp, "Failed to get properties for event on object %ws", pszObjectId);

        if (hrTemp == S_OK)
        {
            // Set the event-specific parameters
            hrTemp = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_UPDATED);
            CHECK_HR(hrTemp, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

            // Send the Event
            if (hrTemp == S_OK)
            {
                PostWpdEvent(pParams, pEventParams);
            }
        }
    }
    
    return hr;
}
