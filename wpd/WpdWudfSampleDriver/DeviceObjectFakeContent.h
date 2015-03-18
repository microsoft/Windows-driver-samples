#include "DeviceObjectFakeContent.h.tmh"

#define DEVICE_PROTOCOL_VALUE                 L"WPD Sample Driver Protocol ver 1.00"
#define DEVICE_FIRMWARE_VERSION_VALUE         L"1.0.0.0"
#define DEVICE_MODEL_VALUE                    L"Sample Device 1000"
#define DEVICE_MANUFACTURER_VALUE             L"Windows Portable Devices Group"
#define DEVICE_SERIAL_NUMBER_VALUE            L"12309342465230-12390123432111"
#define DEVICE_POWER_LEVEL_VALUE              100
#define DEVICE_FRIENDLY_NAME                  L"My Sample Device 1000"

__declspec(selectany) BYTE g_NetworkIdentifier[] = { 0x01, 0x02, 0x03, 0xff, 0xff, 0x04, 0x05, 0x06 };

class DeviceObjectFakeContent : public FakeContent
{
public:
    DeviceObjectFakeContent(
        _In_ IPortableDeviceClassExtension* pPortableDeviceClassExtension) : FakeContent()
    {
        FriendlyName                    = DEVICE_FRIENDLY_NAME;
        SyncPartner                     = L"";
        m_pPortableDeviceClassExtension = pPortableDeviceClassExtension;
    }

    DeviceObjectFakeContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~DeviceObjectFakeContent()
    {
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

        if (SUCCEEDED(hr))
        {
            hr = AddSupportedProperties(WPD_FUNCTIONAL_CATEGORY_DEVICE, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for DeviceObjectFakeContent");
        }
        return hr;
    }

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pValues)
    {
        HRESULT             hr          = S_OK;
        HRESULT             hrSetValue  = S_OK;

        if(pValues == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        // Call the base class to fill in the standard properties
        hr = FakeContent::GetAllValues(pValues);
        if (FAILED(hr))
        {
            CHECK_HR(hr, "Failed to get basic property set");
            return hr;
        }

        // Add WPD_DEVICE_SUPPORTS_NON_CONSUMABLE
        hrSetValue = pValues->SetBoolValue(WPD_DEVICE_SUPPORTS_NON_CONSUMABLE, TRUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_SUPPORTS_NON_CONSUMABLE");
            return hrSetValue;
        }

        // Add WPD_OBJECT_CONTENT_TYPE
        hrSetValue = pValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_OBJECT_CONTENT_TYPE");
            return hrSetValue;
        }

        // Add WPD_FUNCTIONAL_OBJECT_CATEGORY
        hrSetValue = pValues->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_DEVICE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
            return hrSetValue;
        }

        // Add WPD_DEVICE_FIRMWARE_VERSION
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_FIRMWARE_VERSION, DEVICE_FIRMWARE_VERSION_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_FIRMWARE_VERSION");
            return hrSetValue;
        }

        // Add WPD_DEVICE_POWER_LEVEL
        hrSetValue = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_LEVEL, DEVICE_POWER_LEVEL_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_POWER_LEVEL");
            return hrSetValue;
        }

        // Add WPD_DEVICE_POWER_SOURCE
        hrSetValue = pValues->SetUnsignedIntegerValue(WPD_DEVICE_POWER_SOURCE, WPD_POWER_SOURCE_EXTERNAL);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_POWER_SOURCE");
            return hrSetValue;
        }

        // Add WPD_DEVICE_PROTOCOL
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_PROTOCOL, DEVICE_PROTOCOL_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_PROTOCOL");
            return hrSetValue;
        }

        // Add WPD_DEVICE_MODEL
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_MODEL, DEVICE_MODEL_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_MODEL");
            return hrSetValue;
        }

        // Add WPD_DEVICE_FRIENDLY_NAME
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, FriendlyName);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_FRIENDLY_NAME");
            return hrSetValue;
        }

        // Add WPD_DEVICE_SERIAL_NUMBER
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_SERIAL_NUMBER, DEVICE_SERIAL_NUMBER_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_SERIAL_NUMBER");
            return hrSetValue;
        }

        // Add WPD_DEVICE_MANUFACTURER
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_MANUFACTURER, DEVICE_MANUFACTURER_VALUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_MANUFACTURER");
            return hrSetValue;
        }

        // Add WPD_DEVICE_TYPE
        hrSetValue = pValues->SetUnsignedIntegerValue(WPD_DEVICE_TYPE, WPD_DEVICE_TYPE_PHONE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_TYPE");
            return hrSetValue;
        }

        // Add WPD_DEVICE_SYNC_PARTNER
        hrSetValue = pValues->SetStringValue(WPD_DEVICE_SYNC_PARTNER, SyncPartner);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_SYNC_PARTNER");
            return hrSetValue;
        }

        // Add WPD_DEVICE_NETWORK_IDENTIFIER
        if (sizeof(ULONGLONG) == sizeof(g_NetworkIdentifier))
        {
            ULONGLONG ullTemp;

            CopyMemory(&ullTemp, g_NetworkIdentifier, sizeof(ULONGLONG));
            hrSetValue = pValues->SetUnsignedLargeIntegerValue(WPD_DEVICE_NETWORK_IDENTIFIER, ullTemp);
            if (hrSetValue != S_OK)
            {
                CHECK_HR(hrSetValue, "Failed to set WPD_DEVICE_NETWORK_IDENTIFIER");
                return hrSetValue;
            }
        }
        else
        {
            hr = E_UNEXPECTED;    // This is a coding error
            CHECK_HR(hr, "Failed to set WPD_DEVICE_NETWORK_IDENTIFIER");
        }

        return hr;
    }

    virtual HRESULT WriteValue(
        _In_ REFPROPERTYKEY key,
        _In_ REFPROPVARIANT Value)
    {
        HRESULT             hr      = S_OK;
        PropVariantWrapper  pvValue;

        if(IsEqualPropertyKey(key, WPD_DEVICE_FRIENDLY_NAME))
        {
            if(Value.vt == VT_LPWSTR)
            {
                if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
                {
                    FriendlyName = Value.pwszVal;
                    // Let the Class Extension know about the change in Friendly Name so it can update the registry value of the same name
                    HRESULT hrTemp = UpdateDeviceFriendlyName(m_pPortableDeviceClassExtension, Value.pwszVal);
                    CHECK_HR(hrTemp, "Failed to update device friendly name");
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Failed to set WPD_DEVICE_FRIENDLY_NAME because value was an empty string");
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_DEVICE_FRIENDLY_NAME because type was not VT_LPWSTR");
            }
        }
        else if(IsEqualPropertyKey(key, WPD_DEVICE_SYNC_PARTNER))
        {
            if (Value.vt == VT_LPWSTR)
            {
                SyncPartner = Value.pwszVal;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_DEVICE_SYNC_PARTNER because type was not VT_LPWSTR");
            }
        }
        else
        {
            // Let the base class take care of any other property writes
            hr = FakeContent::WriteValue(key, Value);
            // No need to log the error since the base class already does it.
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
            // Fill in the common resource attributes
            hr = GetCommonResourceAttributes(&pAttributes);
            CHECK_HR(hr, "Failed to get common resource attributes set");
        }

        if (SUCCEEDED(hr))
        {
            if (IsEqualPropertyKey(Key, WPD_RESOURCE_ICON))
            {
                // Override the size attribute for this resource.
                hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON));
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

        *pdwNumBytesRead = 0;

        if (IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_DEFAULT))
        {
            return FakeContent::ReadData(ResourceKey, dwStartByte, pBuffer, dwNumBytesToRead, pdwNumBytesRead);
        }

        pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON);
        dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_DEVICE_ICON);

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
        return FakeDeviceContent_Format;
    }

private:
    CAtlStringW FriendlyName;
    CAtlStringW SyncPartner;
    CComPtr<IPortableDeviceClassExtension> m_pPortableDeviceClassExtension;
};

