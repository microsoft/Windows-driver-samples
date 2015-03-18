#include "stdafx.h"

#include "FakeDeviceContent.tmh"

const PropertyAttributeInfo g_SupportedDeviceProperties[] =
{
    {&WPD_OBJECT_ID,                                VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_PERSISTENT_UNIQUE_ID,              VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_PARENT_ID,                         VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_NAME,                              VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_FORMAT,                            VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_CONTENT_TYPE,                      VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_CAN_DELETE,                        VT_BOOL,   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,    VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL}, 
    {&WPD_FUNCTIONAL_OBJECT_CATEGORY,               VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_FIRMWARE_VERSION,                  VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_POWER_LEVEL,                       VT_UI4,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_POWER_SOURCE,                      VT_UI4,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_PROTOCOL,                          VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_MODEL,                             VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_SERIAL_NUMBER,                     VT_LPWSTR, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_SUPPORTS_NON_CONSUMABLE,           VT_BOOL,   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_MANUFACTURER,                      VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_FRIENDLY_NAME,                     VT_CLSID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_DEVICE_TYPE,                              VT_UI4,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
};

HRESULT FakeDeviceContent::InitializeContent(
    _Inout_ DWORD *pdwLastObjectID)
{
    HRESULT hr = S_OK;
    
    if (pdwLastObjectID == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Add top level object: Contacts Service
    CAutoPtr<FakeContactsServiceContent> pContactsService(new FakeContactsServiceContent());
    if (pContactsService)
    {
        hr = pContactsService->InitializeContent(pdwLastObjectID);
        if (hr == S_OK)
        {
            _ATLTRY
            {
                m_Children.Add(pContactsService);
                pContactsService.Detach();
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeContactsServiceContent");
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_HR(hr, "Failed to allocate contacts service content");
    }

    // Add top level object: Storage
    CAutoPtr<FakeStorage> pFakeStorage(new FakeStorage());
    if (pFakeStorage)
    {
        _ATLTRY
        {
            m_Children.Add(pFakeStorage);
            pFakeStorage.Detach();
        }
        _ATLCATCH(e)
        {
            hr = e;
            CHECK_HR(hr, "ATL Exception when adding FakeStorage");
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_HR(hr, "Failed to allocate storage content");
    }

    return hr;
}

HRESULT FakeDeviceContent::InitializeEnumerationContext(
            ACCESS_SCOPE                Scope,
    _In_    WpdObjectEnumeratorContext* pEnumeratorContext)
{
    HRESULT hr = S_OK;

    if (pEnumeratorContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }    
    
    // Initialize the enumeration context
    if (Scope == CONTACTS_SERVICE_ACCESS)
    {
        // scoped by contacts service, so only the contacts service is visible
        pEnumeratorContext->m_TotalChildren = 1;
    }
    else
    {
        // default device wide enumeration, all children are visible
        pEnumeratorContext->m_TotalChildren = static_cast<DWORD>(m_Children.GetCount());
    }

    return hr;
}

HRESULT FakeDeviceContent::GetSupportedProperties(
    _In_    IPortableDeviceKeyCollection* pKeys)
{
    HRESULT hr = S_OK;
    if (pKeys == NULL)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    // Add the PROPERTYKEYs for the 'DEVICE' object
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedDeviceProperties); dwIndex++)
    {
        hr = pKeys->Add(*g_SupportedDeviceProperties[dwIndex].pKey);
        CHECK_HR(hr, "Failed to add device property");
    }

    return hr;
}

HRESULT FakeDeviceContent::GetValue(
    _In_    REFPROPERTYKEY          Key,
    _In_    IPortableDeviceValues*  pStore)
{
    HRESULT hr = S_OK;

    if (pStore == NULL)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    // Set DEVICE object properties
    if (IsEqualPropertyKey(Key, WPD_DEVICE_FIRMWARE_VERSION))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_FIRMWARE_VERSION, FirmwareVersion);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_FIRMWARE_VERSION");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_LEVEL))
    {
        hr = pStore->SetUnsignedIntegerValue(WPD_DEVICE_POWER_LEVEL, PowerLevel);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_LEVEL");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_POWER_SOURCE))
    {
        hr = pStore->SetUnsignedIntegerValue(WPD_DEVICE_POWER_SOURCE, PowerSource);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_POWER_SOURCE");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_PROTOCOL))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_PROTOCOL, Protocol);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_PROTOCOL");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_MODEL))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_MODEL, Model);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_MODEL");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_SERIAL_NUMBER))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_SERIAL_NUMBER, SerialNumber);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_SERIAL_NUMBER");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_MANUFACTURER))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_MANUFACTURER, Manufacturer);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_MANUFACTURER");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_FRIENDLY_NAME))
    {
        hr = pStore->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, FriendlyName);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_FRIENDLY_NAME");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_TYPE))
    {
        hr = pStore->SetUnsignedIntegerValue(WPD_DEVICE_TYPE, DeviceType);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_TYPE");
    }
    
    // Set general properties for DEVICE
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
        CHECK_HR(hr, "Failed to set WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID");
    }
    else if (IsEqualPropertyKey(Key, WPD_DEVICE_SUPPORTS_NON_CONSUMABLE))
    {
        hr = pStore->SetBoolValue(WPD_DEVICE_SUPPORTS_NON_CONSUMABLE, SupportsNonConsumable);
        CHECK_HR(hr, "Failed to set WPD_DEVICE_SUPPORTS_NON_CONSUMABLE");
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property %ws.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }

    return hr;
}


HRESULT FakeDeviceContent::GetSupportedResources(
    _In_    IPortableDeviceKeyCollection* pResources)
{
    HRESULT hr = S_OK;

    if (pResources       == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (hr == S_OK)
    {
        hr = pResources->Add(WPD_RESOURCE_ICON);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ICON for the Device object");
    }

    return hr;
}

HRESULT FakeDeviceContent::GetResourceAttributes(
    _In_    REFPROPERTYKEY         Resource,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (IsEqualPropertyKey(Resource, WPD_RESOURCE_ICON))
    {
        hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_DELETE, FALSE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_DELETE");

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedLargeIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON));
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_READ, TRUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_READ");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_WRITE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_WRITE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_DELETE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_DELETE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_ICON);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE, FILE_OPTIMAL_READ_BUFFER_SIZE_VALUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE, FILE_OPTIMAL_WRITE_BUFFER_SIZE_VALUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE");
        }
    }

    return hr;
}

HRESULT FakeDeviceContent::OpenResource(
    _In_    REFPROPERTYKEY            Resource,
            const DWORD               dwMode,
    _In_    WpdObjectResourceContext* pResourceContext)
{
    HRESULT     hr = S_OK;
    
    if (pResourceContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Validate whether the params given to us are correct.  In this case, we need to check that the object
    // supports the resource requested, and can be opened in the requested access mode.

    // In this sample, we only support one resource (WPD_RESOURCE_ICON) for reading only.
    // So if any resource or dwMode is specified, it must be invalid.
    if (!IsEqualPropertyKey(Resource, WPD_RESOURCE_ICON))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Only WPD_RESOURCE_DEFAULT is supported in this sample driver");
    }

    if ((hr == S_OK) && ((dwMode & STGM_WRITE) != 0))
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "This resource is not available for write access");
    }

    if (hr == S_OK)
    {
        // Initialize the resource context with ...
        pResourceContext->m_strObjectID      = ObjectID;
        pResourceContext->m_Resource         = Resource;
        pResourceContext->m_BytesTransferred = 0;
        pResourceContext->m_BytesTotal       = GetResourceSize(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON);
    }

    return hr;
}

HRESULT FakeDeviceContent::ReadResourceData(
    _In_                            WpdObjectResourceContext*   pResourceContext,
    _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)   BYTE* pBuffer,
                                    const DWORD                 dwNumBytesToRead,
    _Out_                           DWORD*                      pdwNumBytesRead)
{
    HRESULT hr                = S_OK;
    PBYTE   pResource         = NULL;
    DWORD   dwBytesToTransfer = 0;

    if ((pResourceContext == NULL) ||
        (pBuffer          == NULL) ||
        (pdwNumBytesRead  == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    *pdwNumBytesRead = 0;

    pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON);
    if (pResource == NULL)
    {
        hr = E_UNEXPECTED;
        CHECK_HR(hr, "Failed to get the resource representing device icon data");
    }

    //  Calculate how many bytes to transfer
    if (hr == S_OK)
    {
        if (pResourceContext->m_BytesTotal >= pResourceContext->m_BytesTransferred)
        {
            dwBytesToTransfer = (DWORD)min((ULONGLONG)dwNumBytesToRead, (pResourceContext->m_BytesTotal - pResourceContext->m_BytesTransferred));

            // Copy the embedded icon file data.
            memcpy(pBuffer, pResource + pResourceContext->m_BytesTransferred, dwBytesToTransfer);

            // set the number of bytes actually read into to pBuffer
            *pdwNumBytesRead = dwBytesToTransfer;
        }
    }

    return hr;
}
