#include "stdafx.h"

#include "FakeContactContent.tmh"

// Properties supported by a contact
const PropertyAttributeInfo g_SupportedContactProperties[] =
{
    // Standard WPD properties.
    {&WPD_OBJECT_ID,                             VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NAME_GenericObj_ObjectID},
    {&WPD_OBJECT_PERSISTENT_UNIQUE_ID,           VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NAME_GenericObj_PersistentUID},
    {&WPD_OBJECT_PARENT_ID,                      VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NAME_GenericObj_ParentID},
    {&WPD_OBJECT_NAME,                           VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NAME_GenericObj_Name},
    {&WPD_OBJECT_FORMAT,                         VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NAME_GenericObj_ObjectFormat},
    {&WPD_OBJECT_CONTENT_TYPE,                   VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  L"ObjectContentType"},
    {&WPD_OBJECT_CAN_DELETE,                     VT_BOOL,   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  L"ObjectCanDelete"},
    {&WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  L"StorageID"},

    // Contact Service extension properties
    {&PKEY_ContactObj_GivenName,                 VT_LPWSTR, UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,     NAME_ContactObj_GivenName},
    {&PKEY_ContactObj_FamilyName,                VT_LPWSTR, UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,     NAME_ContactObj_FamilyName},

    // Custom property used to store the version of this object
    {&MyContactVersionIdentifier,                VT_UI4,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  L"ContactVersionIdentifier"},
};

HRESULT GetSupportedContactProperties(
    _In_    IPortableDeviceKeyCollection *pKeys)
{
    HRESULT  hr = S_OK;

    if(pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL collection parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedContactProperties); dwIndex++)
    {
        hr = pKeys->Add(*g_SupportedContactProperties[dwIndex].pKey);
        CHECK_HR(hr, "Failed to add custom contacts property");
    }

    return hr;
}

HRESULT GetContactPropertyAttributes(
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

    hr = SetPropertyAttributes(Key, &g_SupportedContactProperties[0], ARRAYSIZE(g_SupportedContactProperties), pAttributes);

    return hr;
}


// For this object, the supported properties are the same as the supported 
// format properties.
// This is where customization for supported properties per object can happen
HRESULT FakeContactContent::GetSupportedProperties(
    _In_    IPortableDeviceKeyCollection *pKeys)
{
    return GetSupportedContactProperties(pKeys);
}

HRESULT FakeContactContent::GetValue(
    _In_   REFPROPERTYKEY         Key, 
    _In_   IPortableDeviceValues* pStore)
{
    HRESULT hr = S_OK;

    PropVariantWrapper pvValue;

    if(pStore == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
    {
        // Add WPD_OBJECT_ID
        pvValue = ObjectID;
        hr = pStore->SetValue(WPD_OBJECT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
    {
        // Add WPD_OBJECT_PERSISTENT_UNIQUE_ID
        pvValue = this->PersistentUniqueID;
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
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        // Add WPD_OBJECT_NAME
        pvValue = Name;
        hr = pStore->SetValue(WPD_OBJECT_NAME, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_NAME"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
    {    
        // Add WPD_OBJECT_CONTENT_TYPE
        hr = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTENT_TYPE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
    {
        // Add WPD_OBJECT_FORMAT
        hr = pStore->SetGuidValue(WPD_OBJECT_FORMAT, Format);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_FORMAT"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
    {
        // Add WPD_OBJECT_CAN_DELETE
        hr = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CAN_DELETE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID))
    {
        // Add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID
        hr = pStore->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, ContainerFunctionalObjectID);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_ContactObj_GivenName))
    {
        // Add PKEY_ContactObj_GivenName
        pvValue = GivenName;
        hr = pStore->SetValue(PKEY_ContactObj_GivenName, &pvValue);
        CHECK_HR(hr, ("Failed to set PKEY_ContactObj_GivenName"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_ContactObj_FamilyName))
    {
        // Add PKEY_ContactObj_FamilyName
        pvValue = FamilyName;
        hr = pStore->SetValue(PKEY_ContactObj_FamilyName, &pvValue);
        CHECK_HR(hr, ("Failed to set PKEY_ContactObj_FamilyName"));
    }
    else if (IsEqualPropertyKey(Key, MyContactVersionIdentifier))
    {
        // Add MyContactVersionIdentifier
        pvValue = VersionIdentifier;
        hr = pStore->SetValue(MyContactVersionIdentifier, &pvValue);
        CHECK_HR(hr, ("Failed to set MyContactVersionIdentifier"));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property {%ws}.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }

    return hr;
}


HRESULT FakeContactContent::WriteValue(
    _In_    REFPROPERTYKEY    Key,
    _In_    REFPROPVARIANT    Value)
{
    HRESULT             hr      = S_OK;
    PropVariantWrapper  pvValue;

    if(IsEqualPropertyKey(Key, PKEY_ContactObj_FamilyName))
    {
        if(Value.vt == VT_LPWSTR)
        {
            FamilyName = Value.pwszVal;
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set PKEY_ContactObj_FamilyName because type was not VT_LPWSTR");
        }
    }
    else if(IsEqualPropertyKey(Key, PKEY_ContactObj_GivenName))
    {
        if(Value.vt == VT_LPWSTR)
        {
            GivenName = Value.pwszVal;
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set PKEY_ContactObj_GivenName because type was not VT_LPWSTR");
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(Key.fmtid), Key.pid, ObjectID);
    }

    return hr;
}

HRESULT FakeContactContent::GetPropertyAttributes(
    _In_    REFPROPERTYKEY          Key,
    _In_    IPortableDeviceValues*  pAttributes)
{
    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL attributes parameter");
        return hr;
    }

    hr = GetContactPropertyAttributes(Key, pAttributes);
    CHECK_HR(hr, "Failed to add property attributes for %ws.%d", CComBSTR(Key.fmtid), Key.pid);

    // Some of our properties have extra attributes on top of the ones that are common to all
    if(IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        CAtlStringW strDefaultName;

        strDefaultName.Format(L"%ws%ws", L"Name", ObjectID.GetString());

        hr = pAttributes->SetStringValue(WPD_PROPERTY_ATTRIBUTE_DEFAULT_VALUE, strDefaultName.GetString());;
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_DEFAULT_VALUE");
    }

    return hr;
}

HRESULT FakeContactContent::WriteValues(
    _In_    IPortableDeviceValues* pValues,
    _In_    IPortableDeviceValues* pResults,
    _Out_   bool*                  pbObjectChanged)
{
    HRESULT hr = FakeContent::WriteValues(pValues, pResults, pbObjectChanged);

    if (SUCCEEDED(hr) && (*pbObjectChanged == true))
    {
        UpdateVersion();
    }

    return hr;
}

void FakeContactContent::UpdateVersion()
{
    if (VersionIdentifier < ULONG_MAX)
    {
        VersionIdentifier++;
    }
    else
    {
        VersionIdentifier = 0;
    }
}
