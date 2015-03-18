#include "stdafx.h"

#include "FakeContactsServiceContent.tmh"

// Change unit is a subset of the custom properties supported by a contact object.
// This typically contains at least one read-only property that indicates that the
// object has changed
const PROPERTYKEY* g_ContactsServiceChangeUnit[1] =
{
    &MyContactVersionIdentifier,
};

const PropertyAttributeInfo g_SupportedServiceProperties[] =
{
    {&WPD_OBJECT_ID,                                VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_PERSISTENT_UNIQUE_ID,              VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_PARENT_ID,                         VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_NAME,                              VT_LPWSTR,          UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,    NULL},
    {&WPD_OBJECT_FORMAT,                            VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CONTENT_TYPE,                      VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CAN_DELETE,                        VT_BOOL,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,    VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_FUNCTIONAL_OBJECT_CATEGORY,               VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_SERVICE_VERSION,                          VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&PKEY_Services_ServiceDisplayName,             VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NAME_Services_ServiceDisplayName},
    {&PKEY_Services_ServiceIcon,                    VT_VECTOR | VT_UI1, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NAME_Services_ServiceIcon},
    {&PKEY_FullEnumSyncSvc_SyncFormat,              VT_VECTOR | VT_UI1, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NAME_FullEnumSyncSvc_SyncFormat},
    {&PKEY_FullEnumSyncSvc_VersionProps,            VT_VECTOR | VT_UI1, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NAME_FullEnumSyncSvc_VersionProps},
    {&PKEY_FullEnumSyncSvc_LocalOnlyDelete,         VT_UI1,             UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,    NAME_FullEnumSyncSvc_LocalOnlyDelete},
    {&PKEY_FullEnumSyncSvc_FilterType,              VT_UI1,             UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,    NAME_FullEnumSyncSvc_FilterType},
    {&PKEY_FullEnumSyncSvc_ReplicaID,               VT_VECTOR | VT_UI1, UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,    NAME_FullEnumSyncSvc_ReplicaID},
};

HRESULT FakeContactsServiceContent::InitializeContent(
    _Inout_ DWORD *pdwLastObjectID)
{
    HRESULT hr = S_OK;

    if (pdwLastObjectID == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Add contact objects to the contact service
    for(DWORD dwContactIndex = 1; dwContactIndex <= NUM_CONTACT_OBJECTS; dwContactIndex++)
    {
        (*pdwLastObjectID)++;

        CAutoPtr<FakeContactContent> pContact(new FakeContactContent());
        if (pContact)
        {
            pContact->ParentID                    = ObjectID;
            pContact->ContainerFunctionalObjectID = ObjectID;
            pContact->ParentPersistentUniqueID    = PersistentUniqueID;
            pContact->RequiredScope               = CONTACTS_SERVICE_ACCESS;
            pContact->Name.Format(L"Contact%d", *pdwLastObjectID);
            pContact->ObjectID.Format(L"%d", *pdwLastObjectID);
            pContact->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContact->ObjectID.GetString());
            pContact->GivenName.Format(L"GivenName%d", dwContactIndex);
            pContact->FamilyName.Format(L"FamilyName%d", dwContactIndex);

            _ATLTRY
            {
                m_Children.Add(pContact);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeContactContent");
            }

            if (SUCCEEDED(hr))
            {
                pContact.Detach();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate contact content at index %d", dwContactIndex);
            return hr;
        }
    }

    return hr;
}

HRESULT FakeContactsServiceContent::CreatePropertiesOnlyObject(
    _In_                            IPortableDeviceValues* pObjectProperties,
    _Out_                           DWORD*                 pdwLastObjectID,
    _Outptr_result_nullonfailure_   FakeContent**          ppNewObject)
{
    HRESULT hr              = S_OK;
    HRESULT hrTemp          = S_OK;
    GUID    guidContentType = WPD_CONTENT_TYPE_UNSPECIFIED;
    GUID    guidFormat      = WPD_OBJECT_FORMAT_UNSPECIFIED;

    if (pObjectProperties == NULL || pdwLastObjectID == NULL || ppNewObject == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    *pdwLastObjectID = NULL;
    *ppNewObject = NULL;

    // Get WPD_OBJECT_FORMAT
    if (SUCCEEDED(hr))
    {
        hr = pObjectProperties->GetGuidValue(WPD_OBJECT_FORMAT, &guidFormat);
        CHECK_HR(hr, "Failed to get WPD_OBJECT_FORMAT");
    }

    if (SUCCEEDED(hr) && (guidFormat != WPD_OBJECT_FORMAT_VCARD2) && (guidFormat != WPD_OBJECT_FORMAT_ABSTRACT_CONTACT))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Invalid Format [%ws]", CComBSTR(guidFormat));
    }

    if (SUCCEEDED(hr))
    {
        // Create the object
        CAutoPtr<FakeContactContent> pContent(new FakeContactContent());
        if (pContent)
        {
            (*pdwLastObjectID)++;
            pContent->ParentID                    = ObjectID;
            pContent->ParentPersistentUniqueID    = PersistentUniqueID;
            pContent->Name.Format(L"Contact%d", *pdwLastObjectID);
            pContent->ObjectID.Format(L"%d", (*pdwLastObjectID));
            pContent->ContentType                 = guidContentType;
            pContent->Format                      = guidFormat;
            pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());
            pContent->ContainerFunctionalObjectID = ObjectID;
            pContent->RequiredScope               = CONTACTS_SERVICE_ACCESS;

            // Get the other optional contact properties.
            LPWSTR  pszTempString   = NULL;
            hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_NAME, &pszTempString);
            if(hrTemp == S_OK)
            {
                pContent->Name = pszTempString;
                CoTaskMemFree(pszTempString);
            }

            hrTemp = pObjectProperties->GetStringValue(PKEY_ContactObj_FamilyName, &pszTempString);
            if(hrTemp == S_OK)
            {
                pContent->FamilyName = pszTempString;
                CoTaskMemFree(pszTempString);
            }

            hrTemp = pObjectProperties->GetStringValue(PKEY_ContactObj_GivenName, &pszTempString);
            if(hrTemp == S_OK)
            {
                pContent->GivenName = pszTempString;
                CoTaskMemFree(pszTempString);
            }

            _ATLTRY
            {
                m_Children.Add(pContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeContactContent");
            }

            if (SUCCEEDED(hr))
            {
                *ppNewObject = pContent.Detach();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate new FakeContactContent object");
        }
    }
    return hr;
}

HRESULT FakeContactsServiceContent::GetSupportedProperties(
    _In_    IPortableDeviceKeyCollection* pKeys)
{
    HRESULT hr = S_OK;

    if (pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; (dwIndex < ARRAYSIZE(g_SupportedServiceProperties)) && (hr == S_OK); dwIndex++)
    {
        // Common WPD service properties
        hr = pKeys->Add(*g_SupportedServiceProperties[dwIndex].pKey);
        CHECK_HR(hr, "Failed to add common service property");
    }

    return hr;
}

HRESULT FakeContactsServiceContent::GetPropertyAttributes(
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = SetPropertyAttributes(Key, &g_SupportedServiceProperties[0], ARRAYSIZE(g_SupportedServiceProperties), pAttributes);

    return hr;
}

HRESULT FakeContactsServiceContent::GetValue(
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr = S_OK;

    PropVariantWrapper pvValue;

    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
    {
        // Add WPD_OBJECT_ID
        pvValue = ObjectID;
        hr = pStore->SetValue(WPD_OBJECT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        // Add WPD_OBJECT_NAME
        pvValue = Name;
        hr = pStore->SetValue(WPD_OBJECT_NAME, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_NAME"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
    {
        // Add WPD_OBJECT_PERSISTENT_UNIQUE_ID
        pvValue = PersistentUniqueID;
        hr = pStore->SetValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
    {
        // Add WPD_OBJECT_PARENT_ID
        pvValue = ParentID;
        hr = pStore->SetValue(WPD_OBJECT_PARENT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PARENT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
    {
        // Add WPD_OBJECT_FORMAT
        hr = pStore->SetGuidValue(WPD_OBJECT_FORMAT, Format);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_FORMAT"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
    {
        // Add WPD_OBJECT_CONTENT_TYPE
        hr = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTENT_TYPE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
    {
        // Add WPD_OBJECT_CAN_DELETE
        hr = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CAN_DELETE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
    {
        // Add WPD_FUNCTIONAL_OBJECT_CATEGORY
        hr = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, FunctionalCategory);
        CHECK_HR(hr, ("Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID))
    {
        // Add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID
        hr = pStore->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, ContainerFunctionalObjectID);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_SERVICE_VERSION))
    {
        // Add WPD_SERVICE_VERSION
        hr = pStore->SetStringValue(WPD_SERVICE_VERSION, Version);
        CHECK_HR(hr, ("Failed to set WPD_SERVICE_VERSION"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_Services_ServiceDisplayName))
    {
        // Add PKEY_Services_ServiceDisplayName
        pvValue = HumanReadableName;
        hr = pStore->SetValue(PKEY_Services_ServiceDisplayName, &pvValue);
        CHECK_HR(hr, ("Failed to set PKEY_Services_ServiceDisplayName"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_SyncFormat))
    {
        // Add PKEY_FullEnumSyncSvc_SyncFormat
        hr = pStore->SetBufferValue(PKEY_FullEnumSyncSvc_SyncFormat, reinterpret_cast<BYTE*>(&PreferredSyncFormat), sizeof(PreferredSyncFormat));
        CHECK_HR(hr, ("Failed to set PKEY_FullEnumSyncSvc_SyncFormat"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_Services_ServiceIcon))
    {
        // Add PKEY_Services_ServiceIcon
        hr = GetIconData(pStore);
        CHECK_HR(hr, "Failed to set PKEY_Services_ServiceIcon");
    }
    else if (IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_VersionProps))
    {
        // Add PKEY_FullEnumSyncSvc_VersionProps
        hr = GetVICData(pStore);
        CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_VersionProps");
    }
    else if (IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_LocalOnlyDelete))
    {
        // Add PKEY_FullEnumSyncSvc_LocalOnlyDelete
        PROPVARIANT pv = {0};
        pv.vt = VT_UI1;
        pv.bVal = LocalOnlyDelete;

        hr = pStore->SetValue(PKEY_FullEnumSyncSvc_LocalOnlyDelete, &pv);
        CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_LocalOnlyDelete");
    }
    else if (IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_FilterType))
    {
        // Add PKEY_FullEnumSyncSvc_FilterType
        PROPVARIANT pv = {0};
        pv.vt = VT_UI1;
        pv.bVal = FilterType;

        hr = pStore->SetValue(PKEY_FullEnumSyncSvc_FilterType, &pv);
        CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_FilterType");
    }
    else if (IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_ReplicaID))
    {
        // Add PKEY_FullEnumSyncSvc_ReplicaID
        hr = pStore->SetBufferValue(PKEY_FullEnumSyncSvc_ReplicaID, reinterpret_cast<BYTE*>(&ReplicaId), sizeof(ReplicaId));
        CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_ReplicaID");
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property {%ws}.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }

    return hr;
}

/**
 *  This method is called to get the contacts service icon data.
 *
 *  The parameters sent to us are:
 *  pValues - An IPortableDeviceValues to be populated with the icon data
 *
 *  The driver should:
 *  Retrieve the icon data and set it in pValues for PKEY_Services_ServiceIcon
 */
HRESULT FakeContactsServiceContent::GetIconData(
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr         = S_OK;
    PBYTE   pIconData  = NULL;
    DWORD   cbIconData = 0;

    if (pStore == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    pIconData = GetResourceData(IDR_WPD_SAMPLEDRIVER_SERVICE_ICON);
    cbIconData = GetResourceSize(IDR_WPD_SAMPLEDRIVER_SERVICE_ICON);

    if ((pIconData == NULL) || (cbIconData == 0))
    {
        hr = E_UNEXPECTED;
        CHECK_HR(hr, "Failed to get resource representing the service icon data");
    }

    if (hr == S_OK)
    {
        hr = pStore->SetBufferValue(PKEY_Services_ServiceIcon, pIconData, cbIconData);
        CHECK_HR(hr, "Failed to copy the icon data to IPortableDeviceValues");
    }

    return hr;
}

/**
 *  This method is called to get the contacts service's full enumeration version properties
 *
 *  The parameters sent to us are:
 *  pValues - An IPortableDeviceValues to be populated with the version property data
 *
 *  The driver should:
 *  Retrieve the version property data blob and set it in pValues for SVCPROP_FullEnumVersionProps
 *
 * Version property data blob must adhere to the following format:
 *
 * Count of Change Unit Groups
 *     Change Unit PROPERTYKEY (group 0)
 *     Count of Keys (group 0)
 *     Key0, Key1..Keyn(group 0)
 *     ...
 *     Change Unit PROPERTYKEY (group 1)
 *     Count of Keys (group 1)
 *     Key0, Key1..Keyn(group 1)
 */
HRESULT FakeContactsServiceContent::GetVICData(
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr                = E_OUTOFMEMORY;
    const DWORD   cGroup      = 1;   // currently support only 1 group
    DWORD         cVIC        = ARRAYSIZE(g_ContactsServiceChangeUnit);

    // Our VIC contains only a single change unit, identified by GUID_NULL
    DWORD   cbVIC =
                    sizeof(cGroup)      +                                           // count of groups of change units
                    // Currently we support only 1 group
                    sizeof(PROPERTYKEY)   +                                         // change unit identifier
                    sizeof(cVIC)        +                                           // change unit count
                    ARRAYSIZE(g_ContactsServiceChangeUnit) * sizeof(PROPERTYKEY);   // Properties in this change unit

    BYTE    *pVIC  = new BYTE[cbVIC];

    if (pVIC)
    {
        hr = E_FAIL;

        BYTE *pPos   = pVIC;
        const BYTE *pEnd = pVIC + cbVIC;

        if ((pPos + sizeof(cGroup)) <= pEnd)
        {
            // count of groups
            memcpy(pPos, &cGroup, sizeof(cGroup));
            pPos+=sizeof(cGroup);

            if ((pPos + sizeof(PROPERTYKEY)) <= pEnd)
            {
                // Change Unit Identifier
                memcpy(pPos, &WPD_PROPERTY_NULL, sizeof(PROPERTYKEY));
                pPos+=sizeof(PROPERTYKEY);

                if ((pPos + sizeof(cVIC)) <= pEnd)
                {
                    // Number of items in the change unit
                    memcpy(pPos, &cVIC, sizeof(cVIC));
                    pPos+=sizeof(cVIC);

                    // Change Unit Property List
                    DWORD i = 0;
                    while ((i < cVIC) && (pPos + sizeof(PROPERTYKEY) <= pEnd))
                    {
                        memcpy(pPos, g_ContactsServiceChangeUnit[i], sizeof(PROPERTYKEY));
                        pPos+=sizeof(PROPERTYKEY);
                        i++;
                    }

                    if ((pPos == pEnd) && (i == cVIC))
                    {
                        // All done
                        hr = pStore->SetBufferValue(PKEY_FullEnumSyncSvc_VersionProps, pVIC, cbVIC);
                    }
                }
            }
        }
        delete [] pVIC;
    }

    CHECK_HR(hr, "Failed to copy the PKEY_FullEnumSyncSvc_VersionProps data to IPortableDeviceValues");
    return hr;
}

HRESULT FakeContactsServiceContent::WriteValue(
    _In_    REFPROPERTYKEY  Key,
    _In_    REFPROPVARIANT  Value)
{
    HRESULT             hr      = S_OK;
    PropVariantWrapper  pvValue;

    if(IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        if(Value.vt == VT_LPWSTR)
        {
            if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
            {
                Name = Value.pwszVal;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME because value was an empty string");
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME because type (%d) was not VT_LPWSTR", Value.vt);
        }
    }
    else if(IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_LocalOnlyDelete))
    {
        if(Value.vt == VT_UI1)
        {
            LocalOnlyDelete = Value.bVal;
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_LocalOnlyDelete because type (%d) was not VT_UI1", Value.vt);
        }
    }
    else if(IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_FilterType))
    {
        if(Value.vt == VT_UI1)
        {
            FilterType = Value.bVal;
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_FilterType because type (%d) was not VT_UI1", Value.vt);
        }
    }
    else if(IsEqualPropertyKey(Key, PKEY_FullEnumSyncSvc_ReplicaID))
    {
        if(Value.vt == (VT_VECTOR | VT_UI1) && Value.caub.cElems == sizeof(ReplicaId))
        {
            CopyMemory(&ReplicaId, Value.caub.pElems, sizeof(ReplicaId));
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set PKEY_FullEnumSyncSvc_FilterType because type (%d) was not VT_VECTOR | VT_UI1", Value.vt);
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(Key.fmtid), Key.pid, ObjectID);
    }

    return hr;
}
