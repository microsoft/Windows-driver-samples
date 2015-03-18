#include "stdafx.h"

#include "FakeDevice.tmh"

const PROPERTYKEY* g_SupportedCommands[] =
{
    // WPD_CATEGORY_OBJECT_ENUMERATION
    &WPD_COMMAND_OBJECT_ENUMERATION_START_FIND,
    &WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT,
    &WPD_COMMAND_OBJECT_ENUMERATION_END_FIND,

    // WPD_CATEGORY_OBJECT_PROPERTIES
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL,
    &WPD_COMMAND_OBJECT_PROPERTIES_SET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES,
    &WPD_COMMAND_OBJECT_PROPERTIES_DELETE,

    // WPD_CATEGORY_OBJECT_RESOURCES
    &WPD_COMMAND_OBJECT_RESOURCES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_RESOURCES_OPEN,
    &WPD_COMMAND_OBJECT_RESOURCES_READ,
    &WPD_COMMAND_OBJECT_RESOURCES_CLOSE,
    &WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES,

    // WPD_CATEGORY_CAPABILITIES
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_COMMANDS,
    &WPD_COMMAND_CAPABILITIES_GET_COMMAND_OPTIONS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FUNCTIONAL_CATEGORIES,
    &WPD_COMMAND_CAPABILITIES_GET_FUNCTIONAL_OBJECTS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMATS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES,
    &WPD_COMMAND_CAPABILITIES_GET_FIXED_PROPERTY_ATTRIBUTES,

    // WPD_CATEGORY_OBJECT_MANAGEMENT
    &WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY,
    &WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS,

    // WPD_CATEGORY_OBJECT_PROPERTIES_BULK
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_END,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_END,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_END,

    // WPD_CATEGORY_COMMON
    &WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS,
};


const GUID* g_SupportedFunctionalCategories[] =
{
    &WPD_FUNCTIONAL_CATEGORY_DEVICE,
    &WPD_FUNCTIONAL_CATEGORY_STORAGE,
    &SERVICE_Contacts,
};

const PROPERTYKEY* g_SupportedCommonProperties[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,
};


HRESULT FakeDevice::InitializeContent()
{
    HRESULT hr = m_DeviceContent.InitializeContent(&m_dwLastObjectID);
    CHECK_HR(hr, "Failed to initialize device content");
    return hr;
}

FakeContactsService* FakeDevice::GetContactsService()
{
    return &m_ContactsService;
}

ACCESS_SCOPE FakeDevice::GetAccessScope(
    _In_    IPortableDeviceValues* pParams)
{
    ACCESS_SCOPE Scope      = FULL_DEVICE_ACCESS;
    LPWSTR       pszFileName = NULL;

    // For simplicity, our request filename is the same as the the service object ID
    if (pParams && (pParams->GetStringValue(PRIVATE_SAMPLE_DRIVER_REQUEST_FILENAME, &pszFileName) == S_OK))
    {
        CAtlStringW strRequestFilename = pszFileName;
        // For simplicity, our request filename is the same as the the service object ID
        // Case-insensitive comparison is required
        if (strRequestFilename.CompareNoCase(m_ContactsService.GetRequestFilename()) == 0)
        {
            Scope = CONTACTS_SERVICE_ACCESS;
        }
    }

    CoTaskMemFree(pszFileName);
    return Scope;
}

HRESULT FakeDevice::GetSupportedCommands(
    _In_    IPortableDeviceKeyCollection* pCommands)
{
    HRESULT hr = S_OK;

    if(pCommands == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommands); dwIndex++)
    {
        PROPERTYKEY key = *(g_SupportedCommands[dwIndex]);
        hr = pCommands->Add(key);
        CHECK_HR(hr, "Failed to add supported command at index %d", dwIndex);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

HRESULT FakeDevice::GetCommandOptions(
    _In_    REFPROPERTYKEY         Command,
    _In_    IPortableDeviceValues* pOptions)
{
    HRESULT hr = S_OK;

    if(pOptions == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    //  Check for command options
    if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS, Command))
    {
        // This driver does not support recursive deletion
        hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED, TRUE);
        CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED");
    }
    if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_RESOURCES_READ, Command))
    {
        // For better read performance, tell the API not to provide the input buffer parameter
        // when issuing a WPD_COMMAND_OBJECT_RESOURCES_READ command.
        hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_RESOURCES_NO_INPUT_BUFFER_ON_READ, TRUE);
        CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_RESOURCES_NO_INPUT_BUFFER_ON_READ");
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedFunctionalCategories(
    _In_    IPortableDevicePropVariantCollection* pFunctionalCategories)
{
    HRESULT hr = S_OK;

    if(pFunctionalCategories == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Device-wide command
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedFunctionalCategories); dwIndex++)
    {
        PROPVARIANT pv = {0};
        PropVariantInit(&pv);
        // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs

        pv.vt    = VT_CLSID;
        pv.puuid = (CLSID*)g_SupportedFunctionalCategories[dwIndex];

        hr = pFunctionalCategories->Add(&pv);
        CHECK_HR(hr, "Failed to add supported functional category at index %d", dwIndex);
        if (FAILED(hr))
        {
            break;
        }
    }

    return hr;
}

HRESULT FakeDevice::GetFunctionalObjects(
    _In_    REFGUID                               guidFunctionalCategory,
    _In_    IPortableDevicePropVariantCollection* pFunctionalObjects)
{
    HRESULT     hr = S_OK;
    PROPVARIANT pv = {0};

    if(pFunctionalObjects == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    PropVariantInit(&pv);
    // Don't call PropVariantClear, since we did not allocate the memory for these object identifiers

    // Add WPD_DEVICE_OBJECT_ID to the functional object identifiers collection
    if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_DEVICE) ||
        (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_ALL))
    {
        pv.vt       = VT_LPWSTR;
        pv.pwszVal  = WPD_DEVICE_OBJECT_ID;
        hr = pFunctionalObjects->Add(&pv);
        CHECK_HR(hr, "Failed to add device object ID");
    }

    // Add CONTACTS_SERVICE_OBJECT_ID to the functional object identifiers collection
    if (hr == S_OK)
    {
        if ((guidFunctionalCategory  == SERVICE_Contacts) ||
            (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_ALL))
        {
            pv.vt       = VT_LPWSTR;
            pv.pwszVal  = CONTACTS_SERVICE_OBJECT_ID;
            hr = pFunctionalObjects->Add(&pv);
            CHECK_HR(hr, "Failed to add contacts service object ID");
        }
    }

    // Add STORAGE_OBJECT_ID to the functional object identifiers collection
    // if request is not scoped by the contacts service
    if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_STORAGE) ||
        (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_ALL))
    {
        pv.vt       = VT_LPWSTR;
        pv.pwszVal  = STORAGE_OBJECT_ID;
        hr = pFunctionalObjects->Add(&pv);
        CHECK_HR(hr, "Failed to add storage object ID");
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedContentTypes(
    _In_    REFGUID                               guidFunctionalCategory,
    _In_    IPortableDevicePropVariantCollection* pContentTypes)
{
    HRESULT hr = S_OK;

    if(pContentTypes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    PROPVARIANT pv = {0};
    PropVariantInit(&pv);
    // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs

    // Add supported content types for known functional categories
    if (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_STORAGE)
    {
        // Add WPD_CONTENT_TYPE_DOCUMENT to the supported content type collection
        pv.vt    = VT_CLSID;
        pv.puuid = (CLSID*)&WPD_CONTENT_TYPE_DOCUMENT;
        hr = pContentTypes->Add(&pv);
        CHECK_HR(hr, "Failed to add WPD_CONTENT_TYPE_DOCUMENT");

        if (hr == S_OK)
        {
            // Add WPD_CONTENT_TYPE_FOLDER to the supported content type collection
            pv.vt    = VT_CLSID;
            pv.puuid = (CLSID*)&WPD_CONTENT_TYPE_FOLDER;
            hr = pContentTypes->Add(&pv);
            CHECK_HR(hr, "Failed to add WPD_CONTENT_TYPE_FOLDER");
        }
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedFormats(
    _In_    REFGUID                               guidContentType,
    _In_    IPortableDevicePropVariantCollection* pFormats)
{
    HRESULT hr = S_OK;

    if(pFormats == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    PROPVARIANT pv = {0};
    PropVariantInit(&pv);
    pv.vt    = VT_CLSID;

    // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs

    if ((guidContentType   == WPD_CONTENT_TYPE_CONTACT) ||
        (guidContentType  == WPD_CONTENT_TYPE_ALL))
    {
        pv.puuid = (CLSID*)&FORMAT_AbstractContact;
        hr = pFormats->Add(&pv);
        CHECK_HR(hr, "Failed to add FORMAT_AbstractContact");

        pv.puuid = (CLSID*)&FORMAT_VCard2Contact;
        hr = pFormats->Add(&pv);
        CHECK_HR(hr, "Failed to add FORMAT_VCard2Contact");
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedFormatProperties(
    _In_    REFGUID                       guidObjectFormat,
    _In_    IPortableDeviceKeyCollection* pKeys)
{
    HRESULT hr = S_OK;

    if(pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (guidObjectFormat == WPD_OBJECT_FORMAT_ALL)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommonProperties); dwIndex++)
        {
            PROPERTYKEY key = *g_SupportedCommonProperties[dwIndex];
            hr = pKeys->Add(key);
            CHECK_HR(hr, "Failed to add common property");
        }
    }

    return hr;
}

HRESULT FakeDevice::GetFixedPropertyAttributes(
    _In_    REFGUID                guidObjectFormat,
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pAttributes)
{
    UNREFERENCED_PARAMETER(guidObjectFormat);
    UNREFERENCED_PARAMETER(Key);

    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    //
    // Since ALL of our properties have the same attributes, we are ignoring the
    // passed in guidObjectFormat and Key parameters.  These parameters allow you to
    // customize fixed property attributes for properties for specific formats.
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

HRESULT FakeDevice::GetSupportedEvents(
    _In_    IPortableDevicePropVariantCollection* pEvents)
{
    UNREFERENCED_PARAMETER(pEvents);

    HRESULT hr = S_OK;

    if(pEvents == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    return hr;
}

HRESULT FakeDevice::GetEventOptions(
    _In_    IPortableDeviceValues* pOptions)
{
    UNREFERENCED_PARAMETER(pOptions);

    HRESULT hr = S_OK;

    if(pOptions == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    return hr;
}

void FakeDevice::InitializeEnumerationContext(
            ACCESS_SCOPE                Scope,
    _In_    LPCWSTR                     wszParentID,
    _In_    WpdObjectEnumeratorContext* pEnumContext)
{
    if (pEnumContext == NULL)
    {
        return;
    }

    pEnumContext->m_Scope = Scope;
    pEnumContext->m_strParentObjectID = wszParentID;

    if (pEnumContext->m_strParentObjectID.GetLength() == 0)
    {
        // Clients passing an 'empty' string for the parent are asking for the
        // 'DEVICE' object.  We should return 1 child in this case.
        pEnumContext->m_TotalChildren = 1;
    }
    else
    {
        FakeContent* pContent = NULL;
        HRESULT hr = GetContent(Scope, wszParentID, &pContent);
        if (hr == S_OK)
        {
            hr = pContent->InitializeEnumerationContext(Scope, pEnumContext);
            CHECK_HR(hr, "Failed to initialize enuemration context for '%ws'", wszParentID);
        }

        if (hr != S_OK)
        {
            // Invalid, or non-existing objects contain no children.
            pEnumContext->m_TotalChildren = 0;
        }
    }
}

HRESULT FakeDevice::FindNext(
                const DWORD                           dwNumObjectsRequested,
    _In_        WpdObjectEnumeratorContext*           pEnumContext,
    _In_        IPortableDevicePropVariantCollection* pObjectIDCollection,
    _Out_opt_   DWORD*                                pdwNumObjectsEnumerated)
{
    HRESULT hr                   = S_OK;
    DWORD   NumObjectsEnumerated = 0;

    if ((pEnumContext        == NULL) ||
        (pObjectIDCollection == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (pdwNumObjectsEnumerated)
    {
        *pdwNumObjectsEnumerated = 0;
    }

    // If the enumeration context reports that their are more objects to return, then continue, if not,
    // return an empty results set.
    if (pEnumContext->HasMoreChildrenToEnumerate())
    {
        if (pEnumContext->m_strParentObjectID.CompareNoCase(L"") == 0)
        {
            // We are being asked for the device
            hr = AddStringValueToPropVariantCollection(pObjectIDCollection, m_DeviceContent.ObjectID);
            CHECK_HR(hr, "Failed to add 'DEVICE' object ID to enumeration collection");

            // Update the the number of children we are returning for this enumeration call
            NumObjectsEnumerated++;
        }
        else
        {
            FakeContent* pContent = NULL;
            HRESULT hrGet = GetContent(pEnumContext->m_Scope, pEnumContext->m_strParentObjectID, &pContent);
            CHECK_HR(hrGet, "Failed to get content '%ws'", pEnumContext->m_strParentObjectID);

            if (hrGet == S_OK)
            {
                DWORD dwStartIndex = pEnumContext->m_ChildrenEnumerated;
                for (DWORD i=0; i<dwNumObjectsRequested; i++)
                {
                    FakeContent* pChild = NULL;
                    if (pContent->FindNext(pEnumContext->m_Scope, dwStartIndex, &pChild))
                    {
                        hr = AddStringValueToPropVariantCollection(pObjectIDCollection, pChild->ObjectID);
                        CHECK_HR(hr, "Failed to add object [%ws]", pChild->ObjectID);

                        if (hr == S_OK)
                        {
                            // Update the the number of children we are returning for this enumeration call
                            dwStartIndex++;
                            NumObjectsEnumerated++;
                        }
                    }
                    else
                    {
                        // no more children
                        break;
                    }
                }
            }
        }
    }

    if (hr == S_OK && pdwNumObjectsEnumerated)
    {
        *pdwNumObjectsEnumerated = NumObjectsEnumerated;
    }

    return hr;
}

HRESULT FakeDevice::GetObjectIDsByFormat(
            ACCESS_SCOPE                          Scope,
    _In_    REFGUID                               guidObjectFormat,
    _In_    LPCWSTR                               wszParentObjectID,
            const DWORD                           dwDepth,
    _In_    IPortableDevicePropVariantCollection* pObjectIDs)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if(pObjectIDs == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    hr = GetContent(Scope, wszParentObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszParentObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetObjectIDsByFormat(Scope, guidObjectFormat, dwDepth, pObjectIDs);
        CHECK_HR(hr, "Failed to get object IDs by format");
    }

    return hr;
}

HRESULT FakeDevice::GetObjectIDsFromPersistentUniqueIDs(
            ACCESS_SCOPE                          Scope,
    _In_    IPortableDevicePropVariantCollection* pPersistentIDs,
    _In_    IPortableDevicePropVariantCollection* pObjectIDs)
{
    HRESULT hr      = S_OK;
    DWORD   dwCount = 0;

    if ((pPersistentIDs == NULL) ||
        (pObjectIDs     == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    // Iterate through the persistent ID list and add the equivalent object ID for each element.
    hr = pPersistentIDs->GetCount(&dwCount);
    CHECK_HR(hr, "Failed to get count from persistent ID collection");

    if (hr == S_OK)
    {
        PROPVARIANT pvPersistentID = {0};

        for(DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            PropVariantInit(&pvPersistentID);

            hr = pPersistentIDs->GetAt(dwIndex, &pvPersistentID);
            CHECK_HR(hr, "Failed to get persistent ID at index %d", dwIndex);

            if (hr == S_OK)
            {
                hr = m_DeviceContent.GetObjectIDByPersistentID(Scope, pvPersistentID.pwszVal, pObjectIDs);
                CHECK_HR(hr, "Failed to get object ID from persistent unique ID '%ws'", pvPersistentID.pwszVal);
            }

            if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            {
                PROPVARIANT pvEmptyObjectID = {0};
                pvEmptyObjectID.vt = VT_LPWSTR;
                pvEmptyObjectID.pwszVal = L"";

                // Insert empty string when object cannot be found
                hr = pObjectIDs->Add(&pvEmptyObjectID);
                CHECK_HR(hr, "Failed to set empty string for persistent unique ID '%ws' when object cannot be found", pvPersistentID.pwszVal);
            }

            PropVariantClear(&pvPersistentID);

            if(FAILED(hr))
            {
                break;
            }
        }
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedProperties(
            ACCESS_SCOPE                  Scope,
    _In_    LPCWSTR                       wszObjectID,
    _In_    IPortableDeviceKeyCollection* pKeys)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pKeys       == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetSupportedProperties(pKeys);
        CHECK_HR(hr, "Failed to get supported properties for '%ws'", wszObjectID);
    }

    return hr;
}

HRESULT FakeDevice::GetAllPropertyValues(
            ACCESS_SCOPE                   Scope,
    _In_    LPCWSTR                        wszObjectID,
    _In_    IPortableDeviceValues*         pValues)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pValues     == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetAllValues(pValues);
        CHECK_HR(hr, "Failed to get all property values for '%ws'", wszObjectID);
    }
    return hr;
}


HRESULT FakeDevice::GetPropertyValues(
            ACCESS_SCOPE                   Scope,
    _In_    LPCWSTR                        wszObjectID,
    _In_    IPortableDeviceKeyCollection*  pKeys,
    _In_    IPortableDeviceValues*         pValues)
{
    HRESULT      hrReturn = S_OK;
    HRESULT      hr       = S_OK;
    DWORD        cKeys    = 0;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pKeys       == NULL) ||
        (pValues     == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pKeys->GetCount(&cKeys);
        CHECK_HR(hr, "Failed to number of PROPERTYKEYs in collection");
    }

    if (hr == S_OK)
    {
        for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
        {
            PROPERTYKEY Key = WPD_PROPERTY_NULL;
            hr = pKeys->GetAt(dwIndex, &Key);
            CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d in collection", dwIndex);

            if (hr == S_OK)
            {
                hr = pContent->GetValue(Key, pValues);
                CHECK_HR(hr, "Failed to get property at index %d", dwIndex);
                if (FAILED(hr))
                {
                    // Mark the property as failed by setting the error value
                    // hrReturn is marked as S_FALSE indicating that at least one property has failed.
                    hr = pValues->SetErrorValue(Key, hr);
                    hrReturn = S_FALSE;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        // A general error has occurred (rather than failure to set one or more properties)
        hrReturn = hr;
    }

    return hrReturn;
}

HRESULT FakeDevice::SetPropertyValues(
            ACCESS_SCOPE           Scope,
    _In_    LPCWSTR                wszObjectID,
    _In_    IPortableDeviceValues* pValues,
    _In_    IPortableDeviceValues* pResults,
    _In_    IPortableDeviceValues* pEventParams,
    _Out_   bool*                  pbObjectChanged)
{
    HRESULT      hr             = S_OK;
    FakeContent* pContent       = NULL;

    if ((wszObjectID     == NULL) ||
        (pValues         == NULL) ||
        (pResults        == NULL) ||
        (pEventParams    == NULL) ||
        (pbObjectChanged == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *pbObjectChanged = false;

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->WriteValues(pValues, pResults, pbObjectChanged);
        CHECK_HR(hr, "Failed to write value for '%ws'", wszObjectID);

        if (SUCCEEDED(hr) && (*pbObjectChanged))  // hr can be S_OK or S_FALSE (if one or more property writes failed)
        {
            HRESULT hrEvent = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_UPDATED);
            CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

            if (hrEvent == S_OK)
            {
                hrEvent = pEventParams->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, pContent->PersistentUniqueID);
                CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_PERSISTENT_UNIQUE_ID");
            }

            if (hrEvent == S_OK)
            {
                hrEvent = pEventParams->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID, pContent->ParentPersistentUniqueID);
                CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID");
            }

            if (hrEvent == S_OK)
            {
                // Adding this event parameter will allow WPD to scope this event to the container functional object
                hrEvent = pEventParams->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, pContent->ContainerFunctionalObjectID);
                CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID");
            }
        }

    }

    return hr;
}

HRESULT FakeDevice::GetPropertyAtributes(
            ACCESS_SCOPE           Scope,
    _In_    LPCWSTR                wszObjectID,
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pAttributes == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetPropertyAttributes(Key, pAttributes);
        CHECK_HR(hr, "Failed to get property attributes for '%ws'", wszObjectID);
    }

    return hr;
}

HRESULT FakeDevice::CreatePropertiesOnlyObject(
                ACCESS_SCOPE           Scope,
    _In_        IPortableDeviceValues* pObjectProperties,
    _In_        IPortableDeviceValues* pEventParams,
    _Outptr_result_nullonfailure_      LPWSTR*  ppszNewObjectID)
{
    HRESULT      hr;
    LPWSTR       pszParentID = NULL;
    FakeContent* pParent     = NULL;
    FakeContent* pNewObject  = NULL;

    if ((pObjectProperties == NULL) ||
        (pEventParams      == NULL) ||
        (ppszNewObjectID   == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppszNewObjectID = NULL;

    // Get WPD_OBJECT_PARENT_ID
    hr = pObjectProperties->GetStringValue(WPD_OBJECT_PARENT_ID, &pszParentID);
    CHECK_HR(hr, "Failed to get WPD_OBJECT_PARENT_ID");

    // Check if it is within our current access scope
    if (SUCCEEDED(hr))
    {
        hr = GetContent(Scope, pszParentID, &pParent);
        CHECK_HR(hr, "Failed to get content '%ws'", pszParentID);
    }

    if (SUCCEEDED(hr))
    {
        hr = pParent->CreatePropertiesOnlyObject(pObjectProperties, &m_dwLastObjectID, &pNewObject);
        CHECK_HR(hr, "Failed to create properties only object with parent '%ws'", pszParentID);
    }

    if (SUCCEEDED(hr))
    {
        *ppszNewObjectID = AtlAllocTaskWideString(pNewObject->ObjectID);
        if (*ppszNewObjectID == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate memory for created object ID");
        }

        HRESULT hrEvent = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_ADDED);
        CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

        if (hrEvent == S_OK)
        {
            hrEvent = pEventParams->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, pNewObject->PersistentUniqueID);
            CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_PERSISTENT_UNIQUE_ID");
        }

        if (hrEvent == S_OK)
        {
            hrEvent = pEventParams->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID, pNewObject->ParentPersistentUniqueID);
            CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID");
        }

        if (hrEvent == S_OK)
        {
            // Adding this event parameter will allow WPD to scope this event to the container functional object
            hrEvent = pEventParams->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, pNewObject->ContainerFunctionalObjectID);
            CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID");
        }
    }

    CoTaskMemFree(pszParentID);
    return hr;
}

HRESULT FakeDevice::DeleteObject(
            ACCESS_SCOPE           Scope,
            const DWORD            dwDeleteOptions,
    _In_    LPCWSTR                wszObjectID,
    _In_    IPortableDeviceValues* pEventParams)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pEventParams == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->MarkForDelete(dwDeleteOptions);
        CHECK_HR(hr, "Failed to mark '%ws' for delete with option %d", wszObjectID, dwDeleteOptions);
    }

    if (hr == S_OK)
    {
        HRESULT hrEvent = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_OBJECT_REMOVED);
        CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

        if (hrEvent == S_OK)
        {
            hrEvent = pEventParams->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, pContent->PersistentUniqueID);
            CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_PERSISTENT_UNIQUE_ID");
        }

        if (hrEvent == S_OK)
        {
            hrEvent = pEventParams->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID, pContent->ParentPersistentUniqueID);
            CHECK_HR(hrEvent, "Failed to add WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID");
        }

        if (hrEvent == S_OK)
        {
            // Adding this event parameter will allow WPD to scope this event to the container functional object
            hrEvent = pEventParams->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, pContent->ContainerFunctionalObjectID);
            CHECK_HR(hrEvent, "Failed to add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID");
        }

        hr = m_DeviceContent.RemoveObjectsMarkedForDeletion(Scope);
        CHECK_HR(hr, "Failed to remove all objects marked for deletion");
    }

    return hr;
}

HRESULT FakeDevice::GetSupportedResources(
            ACCESS_SCOPE                  Scope,
    _In_    LPCWSTR                       wszObjectID,
    _In_    IPortableDeviceKeyCollection* pResources)
{
    HRESULT      hr = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pResources       == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetSupportedResources(pResources);
        CHECK_HR(hr, "Failed to get the supported resources for '%ws'", wszObjectID);
    }

    return hr;
}

HRESULT FakeDevice::GetResourceAttributes(
            ACCESS_SCOPE           Scope,
    _In_    LPCWSTR                wszObjectID,
    _In_    REFPROPERTYKEY         Resource,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID == NULL) ||
        (pAttributes == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        hr = pContent->GetResourceAttributes(Resource, pAttributes);
        CHECK_HR(hr, "Failed to get the supported resources for '%ws'", wszObjectID);
    }

    return hr;
}

HRESULT FakeDevice::OpenResource(
            ACCESS_SCOPE              Scope,
    _In_    LPCWSTR                   wszObjectID,
    _In_    REFPROPERTYKEY            Resource,
            const DWORD               dwMode,
    _In_    WpdObjectResourceContext* pResourceContext)
{
    HRESULT      hr       = S_OK;
    FakeContent* pContent = NULL;

    if ((wszObjectID      == NULL) ||
        (pResourceContext == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = GetContent(Scope, wszObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    if (hr == S_OK)
    {
        pResourceContext->m_Scope = Scope;
        hr = pContent->OpenResource(Resource, dwMode, pResourceContext);
        CHECK_HR(hr, "Failed to open resource for '%ws'", wszObjectID);
    }

    return hr;
}

HRESULT FakeDevice::ReadResourceData(
    _In_                            WpdObjectResourceContext*   pResourceContext,
    _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)   BYTE* pBuffer,
                                    const DWORD                 dwNumBytesToRead,
    _Out_                           DWORD*                      pdwNumBytesRead)
{
    HRESULT hr            = S_OK;
    FakeContent* pContent = NULL;

    if ((pResourceContext == NULL) ||
        (pBuffer          == NULL) ||
        (pdwNumBytesRead  == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    *pdwNumBytesRead = 0;

    hr = GetContent(pResourceContext->m_Scope, pResourceContext->m_strObjectID, &pContent);
    CHECK_HR(hr, "Failed to get content '%ws'", pResourceContext->m_strObjectID);

    if (hr == S_OK)
    {
        hr = pContent->ReadResourceData(pResourceContext, pBuffer, dwNumBytesToRead, pdwNumBytesRead);
        CHECK_HR(hr, "Failed to read resource data for '%ws'", pResourceContext->m_strObjectID);
    }

    return hr;
}

HRESULT FakeDevice::GetContent(
                                    ACCESS_SCOPE   Scope,
    _In_                            LPCWSTR        wszObjectID,
    _Outptr_result_nullonfailure_   FakeContent**  ppContent)
{
    HRESULT hr = S_OK;

    if (ppContent == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
    }

    *ppContent = NULL;

    hr = m_DeviceContent.GetContent(Scope, wszObjectID, ppContent);
    CHECK_HR(hr, "Failed to get content '%ws'", wszObjectID);

    return hr;
}
