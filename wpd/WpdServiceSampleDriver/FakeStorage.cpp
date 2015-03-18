#include "stdafx.h"

#include "FakeStorage.tmh"

const PropertyAttributeInfo g_SupportedStorageProperties[] =
{
    {&WPD_OBJECT_ID,                                VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_PERSISTENT_UNIQUE_ID,              VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_PARENT_ID,                         VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_NAME,                              VT_LPWSTR,  UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,     NULL}, 
    {&WPD_OBJECT_FORMAT,                            VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_CONTENT_TYPE,                      VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_CAN_DELETE,                        VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,    VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL}, 
    {&WPD_STORAGE_TYPE,                             VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
    {&WPD_STORAGE_FILE_SYSTEM_TYPE,                 VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
    {&WPD_STORAGE_CAPACITY,                         VT_UI8,     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
    {&WPD_STORAGE_FREE_SPACE_IN_BYTES,              VT_UI8,     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
    {&WPD_STORAGE_SERIAL_NUMBER,                    VT_LPWSTR,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
    {&WPD_STORAGE_DESCRIPTION,                      VT_LPWSTR,  UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,     NULL},
    {&WPD_FUNCTIONAL_OBJECT_CATEGORY,               VT_CLSID,   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,  NULL},
};

HRESULT FakeStorage::GetSupportedProperties(
    _In_    IPortableDeviceKeyCollection *pKeys)
{
    HRESULT hr = S_OK;

    if (pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedStorageProperties); dwIndex++)
    {
        hr = pKeys->Add(*g_SupportedStorageProperties[dwIndex].pKey);
        CHECK_HR(hr, "Failed to add storage property");
    }

    return hr;
}

HRESULT FakeStorage::GetValue(
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr = S_OK;

    if(pStore == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (IsEqualPropertyKey(Key, WPD_STORAGE_SERIAL_NUMBER))
    {
        hr = pStore->SetStringValue(WPD_STORAGE_SERIAL_NUMBER, SerialNumber);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_SERIAL_NUMBER");
    }
    else if (IsEqualPropertyKey(Key, WPD_STORAGE_FREE_SPACE_IN_BYTES))
    {
        hr = pStore->SetUnsignedLargeIntegerValue(WPD_STORAGE_FREE_SPACE_IN_BYTES, FreeSpace);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_FREE_SPACE_IN_BYTES");
    }
    else if (IsEqualPropertyKey(Key, WPD_STORAGE_CAPACITY))
    {
        hr = pStore->SetUnsignedLargeIntegerValue(WPD_STORAGE_CAPACITY, Capacity);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_CAPACITY");
    }
    else if (IsEqualPropertyKey(Key, WPD_STORAGE_TYPE))
    {
        hr = pStore->SetUnsignedIntegerValue(WPD_STORAGE_TYPE, StorageType);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_TYPE");
    }
    else if (IsEqualPropertyKey(Key, WPD_STORAGE_FILE_SYSTEM_TYPE))
    {
        hr = pStore->SetStringValue(WPD_STORAGE_FILE_SYSTEM_TYPE, FileSystemType);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_FILE_SYSTEM_TYPE");
    }
    else if (IsEqualPropertyKey(Key, WPD_STORAGE_DESCRIPTION))
    {
        hr = pStore->SetStringValue(WPD_STORAGE_DESCRIPTION, Description);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_DESCRIPTION");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
    {
        hr = pStore->SetStringValue(WPD_OBJECT_ID, ObjectID);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_ID");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        hr = pStore->SetStringValue(WPD_OBJECT_NAME, Name);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
    {
        hr = pStore->SetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, PersistentUniqueID);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
    {
        hr = pStore->SetStringValue(WPD_OBJECT_PARENT_ID, ParentID);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_PARENT_ID");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
    {
        hr = pStore->SetGuidValue(WPD_OBJECT_FORMAT, Format);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
    {
        hr = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTENT_TYPE");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
    {
        hr = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_CAN_DELETE");
    }
    else if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
    {
        hr = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, FunctionalCategory);
        CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID))
    {
        hr = pStore->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, ContainerFunctionalObjectID);
        CHECK_HR(hr, "Failed to set WPD_STORAGE_DESCRIPTION");
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property {%ws}.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }

    return hr;
}

HRESULT FakeStorage::WriteValue(
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
            CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME because type was not VT_LPWSTR");
        }
    }
    else if(IsEqualPropertyKey(Key, WPD_STORAGE_DESCRIPTION))
    {
        if(Value.vt == VT_LPWSTR)
        {
            if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
            {
                Description = Value.pwszVal;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_STORAGE_DESCRIPTION because value was an empty string");
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set WPD_STORAGE_DESCRIPTION because type was not VT_LPWSTR");
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(Key.fmtid), Key.pid, ObjectID);
    }

    return hr;
}


HRESULT FakeStorage::GetPropertyAttributes(
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

    hr = SetPropertyAttributes(Key, &g_SupportedStorageProperties[0], ARRAYSIZE(g_SupportedStorageProperties), pAttributes);
    CHECK_HR(hr, "Failed to set storage property attributes");
    
    return hr;
}

