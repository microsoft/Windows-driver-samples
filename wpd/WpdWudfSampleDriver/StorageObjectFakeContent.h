#include "StorageObjectFakeContent.h.tmh"

class StorageObjectFakeContent : public FakeContent
{
public:
    StorageObjectFakeContent()
    {
        Capacity            = 0;
        FreeSpaceInBytes    = 0;
        IsExternalStorage   = FALSE;
    }

    StorageObjectFakeContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~StorageObjectFakeContent()
    {
    }

    virtual StorageObjectFakeContent& operator= (const StorageObjectFakeContent& src)
    {
        ObjectID            = src.ObjectID;
        PersistentUniqueID  = src.PersistentUniqueID;
        ParentID            = src.ParentID;
        Name                = src.Name;
        ContentType         = src.ContentType;
        MarkedForDeletion   = src.MarkedForDeletion;
        CanDelete           = src.CanDelete;
        IsHidden            = src.IsHidden;
        IsSystem            = src.IsSystem;
        NonConsumable       = src.NonConsumable;
        Capacity            = src.Capacity;
        FreeSpaceInBytes    = src.FreeSpaceInBytes;
        IsExternalStorage   = src.IsExternalStorage;

        return *this;
    }

    virtual HRESULT GetSupportedProperties(_COM_Outptr_ IPortableDeviceKeyCollection** ppKeys)
    {
        HRESULT hr  = S_OK;

        if(ppKeys == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL collection parameter");
            return hr;
        }

        *ppKeys = NULL;

        if (SUCCEEDED(hr))
        {
            hr = AddSupportedProperties(WPD_FUNCTIONAL_CATEGORY_STORAGE, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for StorageObjectFakeContent");
        }
        return hr;
    }

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pStore)
    {
        HRESULT hr          = S_OK;
        HRESULT hrSetValue  = S_OK;

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

        // Add WPD_STORAGE_CAPACITY
        hrSetValue = pStore->SetUnsignedLargeIntegerValue(WPD_STORAGE_CAPACITY, Capacity);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_STORAGE_CAPACITY");
            return hrSetValue;
        }

        // Add WPD_STORAGE_FREE_SPACE_IN_BYTES
        hrSetValue = pStore->SetUnsignedLargeIntegerValue(WPD_STORAGE_FREE_SPACE_IN_BYTES, FreeSpaceInBytes);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_STORAGE_FREE_SPACE_IN_BYTES");
            return hrSetValue;
        }

        // Add WPD_FUNCTIONAL_OBJECT_CATEGORY
        hrSetValue = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_STORAGE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
            return hrSetValue;
        }

        // Add WPD_STORAGE_TYPE
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_STORAGE_TYPE, WPD_STORAGE_TYPE_FIXED_RAM);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_STORAGE_TYPE");
            return hrSetValue;
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
            hr = AddFixedPropertyAttributes(FakeStorageContent_Format, Key, pAttributes);
            CHECK_HR(hr, "Failed to add fixed property attributes for %ws.%d on StorageObjectFakeContent", CComBSTR(Key.fmtid), Key.pid);
        }

        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on IPortableDeviceValues");
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
            // Call the base class to fill in the standard resources if any exist
            hr = FakeContent::GetSupportedResources(&pKeys);
            CHECK_HR(hr, "Failed to get basic supported resources");
        }

        if (SUCCEEDED(hr))
        {
            // Add the icon resource
            hr = pKeys->Add(WPD_RESOURCE_ICON);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_ICON to supported resource list");
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
        UNREFERENCED_PARAMETER(Key);

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
            // Fill in the standard resource attributes
            hr = GetCommonResourceAttributes(&pAttributes);
            CHECK_HR(hr, "Failed to get common resource attributes set");
        }

        if (SUCCEEDED(hr))
        {
            if (IsEqualPropertyKey(Key, WPD_RESOURCE_ICON))
            {
                // Override the size attribute for this resource.
                hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetResourceSize(GetResourceID()));
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");

                // Override the format attribute for this resource.
                hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_ICON);
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
            }
        }

        // Return the resource attributes
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }
        return hr;
    }

    // This sample driver uses a embedded image file resource as its data.
    virtual HRESULT ReadData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead) BYTE*   pBuffer,
                DWORD           dwNumBytesToRead,
        _Out_   DWORD*          pdwNumBytesRead)
    {
        HRESULT hr                = S_OK;
        DWORD   dwBytesToTransfer = 0;
        DWORD   dwObjectDataSize  = 0;
        PBYTE   pResource         = NULL;

        if((pBuffer         == NULL) ||
           (pdwNumBytesRead == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        if (IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_DEFAULT))
        {
            return FakeContent::ReadData(ResourceKey, dwStartByte, pBuffer, dwNumBytesToRead, pdwNumBytesRead);
        }

        *pdwNumBytesRead = 0;

        pResource = GetResourceData(GetResourceID());
        dwObjectDataSize = GetResourceSize(GetResourceID());

        if (pResource == NULL)
        {
            hr = E_UNEXPECTED;
            CHECK_HR(hr, "Failed to get resource representing the device icon data");
        }

        //  Calculate how many bytes to transfer
        if (hr == S_OK)
        {
            if (dwStartByte < dwObjectDataSize)
            {
                dwBytesToTransfer = (dwObjectDataSize - dwStartByte);
                if (dwBytesToTransfer > dwNumBytesToRead)
                {
                    dwBytesToTransfer = dwNumBytesToRead;
                }
            }
        }

        // Copy the embedded image file data.
        if ((hr == S_OK) && (dwBytesToTransfer > 0))
        {
            memcpy(pBuffer, pResource + dwStartByte, dwBytesToTransfer);
        }

        if (hr == S_OK)
        {
            *pdwNumBytesRead = dwBytesToTransfer;
        }

        return hr;
    }

    virtual GUID GetObjectFormat()
    {
        return FakeStorageContent_Format;
    }

    UINT GetResourceID()
    {
        UINT uiResourceID = IDR_WPD_SAMPLEDRIVER_INTERNAL_STORAGE_ICON;
        if(IsExternalStorage == TRUE)
        {
            uiResourceID = IDR_WPD_SAMPLEDRIVER_EXTERNAL_STORAGE_ICON;
        }

        return uiResourceID;
    }

    BOOL        IsExternalStorage;
    ULONGLONG   Capacity;
    ULONGLONG   FreeSpaceInBytes;
};

