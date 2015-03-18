#include "FakeContent.h.tmh"

class FakeContent
{
public:
    FakeContent() :
        MarkedForDeletion(FALSE),
        CanDelete(TRUE),
        IsHidden(FALSE),
        IsSystem(FALSE),
        NonConsumable(FALSE)
    {
        ContentType = WPD_CONTENT_TYPE_UNSPECIFIED;
    }

    FakeContent(const FakeContent& src) :
        MarkedForDeletion(FALSE),
        CanDelete(TRUE),
        IsHidden(FALSE),
        IsSystem(FALSE),
        NonConsumable(FALSE)
    {
        *this = src;
    }

    virtual ~FakeContent()
    {
    }

    virtual FakeContent& operator= (const FakeContent& src)
    {
        ObjectID            = src.ObjectID;
        PersistentUniqueID  = src.PersistentUniqueID;
        ParentID            = src.ParentID;
        Name                = src.Name;
        FileName            = src.FileName;
        ContentType         = src.ContentType;
        MarkedForDeletion   = src.MarkedForDeletion;
        CanDelete           = src.CanDelete;
        IsHidden            = src.IsHidden;
        IsSystem            = src.IsSystem;
        NonConsumable       = src.NonConsumable;

        return *this;
    }

    virtual HRESULT GetSupportedProperties(_COM_Outptr_ IPortableDeviceKeyCollection** ppKeys)
    {
        HRESULT hr = S_OK;

        if(ppKeys == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL collection parameter");
            return hr;
        }

        hr = AddSupportedProperties(GetObjectFormat(), ppKeys);
        CHECK_HR(hr, ("Failed to add supported proeprties for FakeContent"));
        return hr;
    }

    virtual HRESULT GetAllValues(_In_ IPortableDeviceValues* pStore)
    {
        HRESULT             hr          = S_OK;
        HRESULT             hrSetValue  = S_OK;
        PropVariantWrapper  pvValue;

        if(pStore == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        // Add WPD_OBJECT_ID
        pvValue = ObjectID;
        hrSetValue = pStore->SetValue(WPD_OBJECT_ID, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_ID"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_PERSISTENT_UNIQUE_ID
        pvValue = this->PersistentUniqueID;
        hrSetValue = pStore->SetValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_PARENT_ID
        pvValue = ParentID;
        hrSetValue = pStore->SetValue(WPD_OBJECT_PARENT_ID, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_PARENT_ID"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_NAME
        pvValue = Name;
        hrSetValue = pStore->SetValue(WPD_OBJECT_NAME, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_NAME"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_CONTENT_TYPE
        hrSetValue = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_CONTENT_TYPE"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_FORMAT
        hrSetValue = pStore->SetGuidValue(WPD_OBJECT_FORMAT, GetObjectFormat());
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_FORMAT"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_CAN_DELETE
        hrSetValue = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_CAN_DELETE"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_ISHIDDEN
        hrSetValue = pStore->SetBoolValue(WPD_OBJECT_ISHIDDEN, IsHidden);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_ISHIDDEN"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_ISSYSTEM
        hrSetValue = pStore->SetBoolValue(WPD_OBJECT_ISSYSTEM, IsSystem);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_ISSYSTEM"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_NON_CONSUMABLE
        hrSetValue = pStore->SetBoolValue(WPD_OBJECT_NON_CONSUMABLE, NonConsumable);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_NON_CONSUMABLE"));
            return hrSetValue;
        }

        // Add WPD_FOLDER_CONTENT_TYPES_ALLOWED if necessary
        if(!RestrictToContentTypes.IsEmpty())
        {
            CComPtr<IPortableDevicePropVariantCollection> pContentTypes;
            hrSetValue = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IPortableDevicePropVariantCollection,
                                          (VOID**) &pContentTypes);
            CHECK_HR(hrSetValue, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");

            if (hrSetValue == S_OK)
            {
                PROPVARIANT pvContentType = {0};
                PropVariantInit(&pvContentType);
                pvContentType.vt = VT_CLSID;  // Don't PropVariantClear this since we won't allocate memory for it

                size_t numElems = RestrictToContentTypes.GetCount();
                for(size_t typeIndex = 0; hrSetValue == S_OK && typeIndex < numElems; typeIndex++)
                {
                    pvContentType.puuid = &RestrictToContentTypes[typeIndex];
                    hrSetValue = pContentTypes->Add(&pvContentType);
                    CHECK_HR(hrSetValue, "Failed to add content type");
                }

                if (hrSetValue == S_OK)
                {
                    hrSetValue = pStore->SetIPortableDevicePropVariantCollectionValue(WPD_FOLDER_CONTENT_TYPES_ALLOWED, pContentTypes);
                    CHECK_HR(hrSetValue, "Failed to set WPD_FOLDER_CONTENT_TYPES_ALLOWED");
                }

                if (FAILED(hrSetValue))
                {
                    hr = hrSetValue;
                }
            }
        }

        return hr;
    }

    virtual HRESULT WriteValue(
        _In_ REFPROPERTYKEY key,
        _In_ REFPROPVARIANT Value)
    {
        HRESULT             hr      = S_OK;
        PropVariantWrapper  pvValue;

        if(IsEqualPropertyKey(key, WPD_OBJECT_NAME))
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
        else if (IsEqualPropertyKey(key, WPD_OBJECT_NON_CONSUMABLE))
        {
            if(Value.vt == VT_BOOL)
            {
                NonConsumable = Value.boolVal;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_OBJECT_NON_CONSUMABLE because type was not VT_BOOL");
            }
        }
        else
        {
            hr = E_ACCESSDENIED;
            CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(key.fmtid), key.pid, ObjectID);
        }

        return hr;
    }

    virtual HRESULT GetAttributes(
        _In_         REFPROPERTYKEY          Key,
        _COM_Outptr_ IPortableDeviceValues** ppAttributes)
    {
        HRESULT                         hr      = S_OK;
        CComPtr<IPortableDeviceValues>  pAttributes;

        if(ppAttributes == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL attributes parameter");
            return hr;
        }

        *ppAttributes = NULL;

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**) &pAttributes);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
        }

        if (SUCCEEDED(hr))
        {
            hr = AddFixedPropertyAttributes(GetObjectFormat(), Key, pAttributes);
            CHECK_HR(hr, "Failed to add fixed property attributes for %ws.%d on FakeContent", CComBSTR(Key.fmtid), Key.pid);
        }

        // Some of our properties have extra attributes on top of the ones that are common to all
        if(IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
        {
            CAtlStringW strDefaultName;

            strDefaultName.Format(L"%ws%ws", L"Name", ObjectID.GetString());

            hr = pAttributes->SetStringValue(WPD_PROPERTY_ATTRIBUTE_DEFAULT_VALUE, strDefaultName.GetString());;
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_DEFAULT_VALUE");
        }

        // Return the property attributes
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }
        return hr;
    }

    virtual HRESULT GetSupportedResources(_COM_Outptr_ IPortableDeviceKeyCollection** ppKeys)
    {
        HRESULT                                 hr      = S_OK;
        CComPtr<IPortableDeviceKeyCollection>   pKeys;

        if(ppKeys == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL collection parameter");
            return hr;
        }

        *ppKeys = NULL;

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceKeyCollection,
                                  (VOID**) &pKeys);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
        }

        // This object has no resources so return the empty collection
        if (SUCCEEDED(hr))
        {
            hr = pKeys->QueryInterface(IID_IPortableDeviceKeyCollection, (VOID**) ppKeys);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceKeyCollection on IPortableDeviceKeyCollection");
        }

        return hr;
    }

    virtual HRESULT GetResourceAttributes(
        _In_            REFPROPERTYKEY          Key,
        _COM_Outptr_    IPortableDeviceValues** ppAttributes)
    {
        UNREFERENCED_PARAMETER(Key);
        *ppAttributes = NULL;

        // This object has no resources therefore has no attributes to return.

        return E_NOTIMPL;
    }

    virtual HRESULT ReadData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead) BYTE*   pBuffer,
                DWORD           dwNumBytesToRead,
        _Out_   DWORD*          pdwNumBytesRead)
    {
        HRESULT         hr                  = S_OK;
        DWORD           dwBytesToTransfer   = 0;

        if((pBuffer         == NULL) ||
           (pdwNumBytesRead == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }
        
        *pdwNumBytesRead = 0;
        UNREFERENCED_PARAMETER(ResourceKey);

        //  Calculate how many bytes to transfer
        if (hr == S_OK)
        {
            if (dwStartByte < FAKE_DATA_SIZE)
            {
                dwBytesToTransfer = (FAKE_DATA_SIZE - dwStartByte);
                if (dwBytesToTransfer > dwNumBytesToRead)
                {
                    dwBytesToTransfer = dwNumBytesToRead;
                }
            }
        }

        // This sample driver does not have any real data content,
        // so we generate fake data content.
        // The content we generate is made up of the Name string repeated
        // in the data buffer.
        if ((hr == S_OK) && (dwBytesToTransfer > 0))
        {
            DWORD dwOffset                  = 0;
            DWORD dwSourceStringSizeInBytes = (Name.GetLength() * sizeof(WCHAR)) + sizeof(L'\0');
            for (DWORD counter = 0; counter < (dwBytesToTransfer / dwSourceStringSizeInBytes); counter++)
            {
                _Analysis_assume_((dwOffset + dwSourceStringSizeInBytes) <= dwNumBytesToRead);
                memcpy(pBuffer + dwOffset, Name.GetString(), dwSourceStringSizeInBytes);
                dwOffset += dwSourceStringSizeInBytes;
            }
            memset(pBuffer + dwOffset, 0, dwBytesToTransfer % dwSourceStringSizeInBytes);
        }

        if (hr == S_OK)
        {
            *pdwNumBytesRead = dwBytesToTransfer;
        }

        return hr;
    }

    virtual HRESULT WriteData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _In_reads_(dwNumBytesToWrite) BYTE* pBuffer,
                DWORD           dwNumBytesToWrite,
        _Out_   DWORD*          pdwNumBytesWritten)
    {
        HRESULT hr = S_OK;

        if((pBuffer             == NULL) ||
           (pdwNumBytesWritten  == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        *pdwNumBytesWritten = 0;
        UNREFERENCED_PARAMETER(ResourceKey);
        UNREFERENCED_PARAMETER(dwStartByte);
        // This fake driver does nothing with the data.  The write method is simply
        // a dummy one.
        // Normally, a driver would copy the contents from the buffer to the device.

        if (hr == S_OK)
        {
            *pdwNumBytesWritten = dwNumBytesToWrite;
        }

        return hr;
    }

    virtual GUID GetObjectFormat()
    {
        return FakeContent_Format;
    }

    virtual HRESULT EnableResource(
        _In_ REFPROPERTYKEY ResourceKey)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        return E_NOTIMPL;
    }

    CAtlStringW     ObjectID;
    CAtlStringW     PersistentUniqueID;
    CAtlStringW     ParentID;
    CAtlStringW     Name;
    CAtlStringW     FileName;
    GUID            ContentType;
    BOOL            MarkedForDeletion;
    BOOL            CanDelete;
    BOOL            IsHidden;
    BOOL            IsSystem;
    BOOL            NonConsumable;
    CAtlArray<GUID> RestrictToContentTypes;
};

class FakeGenericFileContent : public FakeContent
{
public:

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pStore)
    {
        HRESULT             hr          = S_OK;
        PropVariantWrapper  pvValue;

        if(pStore == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        // Call the base class to fill in the standard properties
        hr = FakeContent::GetAllValues(pStore);
        if (FAILED(hr))
        {
            CHECK_HR(hr, "Failed to get basic property set");
            return hr;
        }

        // Add WPD_OBJECT_ORIGINAL_FILE_NAME
        pvValue = FileName;
        hr = pStore->SetValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pvValue);
        if (hr != S_OK)
        {
            CHECK_HR(hr, ("Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME"));
            return hr;
        }

        return hr;
    }

    virtual HRESULT GetSupportedResources(_COM_Outptr_ IPortableDeviceKeyCollection** ppKeys)
    {
        HRESULT                                 hr      = S_OK;
        CComPtr<IPortableDeviceKeyCollection>   pKeys;

        if(ppKeys == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL collection parameter");
            return hr;
        }

        *ppKeys = NULL;

        if (SUCCEEDED(hr))
        {
            // Call the base class to fill in the standard resources if any
            hr = FakeContent::GetSupportedResources(&pKeys);
            CHECK_HR(hr, "Failed to get basic supported resources");
        }

        if (SUCCEEDED(hr))
        {
            // Add WPD_RESOURCE_DEFAULT
            hr = pKeys->Add(WPD_RESOURCE_DEFAULT);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_DEFAULT to collection");
        }

        if (SUCCEEDED(hr))
        {
            hr = pKeys->QueryInterface(IID_IPortableDeviceKeyCollection, (VOID**) ppKeys);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceKeyCollection on IPortableDeviceKeyCollection");
        }

        return hr;
    }

    virtual HRESULT GetResourceAttributes(
        _In_         REFPROPERTYKEY          Key,
        _COM_Outptr_ IPortableDeviceValues** ppAttributes)
    {
        HRESULT                         hr      = S_OK;
        CComPtr<IPortableDeviceValues>  pAttributes;

        if(ppAttributes == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL attributes parameter");
            return hr;
        }

        *ppAttributes = NULL;

        if (SUCCEEDED(hr))
        {
            // Fill in the common resource attributes
            hr = GetCommonResourceAttributes(&pAttributes);
            CHECK_HR(hr, "Failed to get common resource attributes set");
        }

        // Override the necessary attributes for this resource.
        if ((hr == S_OK) && (IsEqualPropertyKey(Key, WPD_RESOURCE_DEFAULT)))
        {
            hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_WRITE, TRUE);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_WRITE");
        }

        // Return the resource attributes
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }
        return hr;
    }
};
