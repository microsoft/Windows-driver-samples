#include "mmsystem.h"
#include "FakeImageContent.h.tmh"

class FakeImageContent : public FakeContent
{
public:
    FakeImageContent()
    {
    }

    FakeImageContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeImageContent()
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

        *ppKeys = NULL;

        if (SUCCEEDED(hr))
        {
            hr = AddSupportedProperties(WPD_OBJECT_FORMAT_EXIF, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for WPD_OBJECT_FORMAT_EXIF");
        }

        return hr;
    }

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pStore)
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

        // Call the base class to fill in the standard properties
        hr = FakeContent::GetAllValues(pStore);
        if (FAILED(hr))
        {
            CHECK_HR(hr, "Failed to get basic property set");
            return hr;
        }

        // Add WPD_MEDIA_WIDTH
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_MEDIA_WIDTH, 800);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MEDIA_WIDTH"));
            return hrSetValue;
        }

        // Add WPD_MEDIA_HEIGHT
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_MEDIA_HEIGHT, 600);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MEDIA_HEIGHT"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_DATE_CREATED
        SYSTEMTIME systemtime = {0};
        PROPVARIANT pv = {0};
        PropVariantInit(&pv);
        pv.vt = VT_DATE;
        systemtime.wDay          = 26;
        systemtime.wDayOfWeek    = 0;
        systemtime.wHour         = 5;
        systemtime.wMinute       = 30;
        systemtime.wMilliseconds = 100;
        systemtime.wMonth        = 6;
        systemtime.wSecond       = 15;
        systemtime.wYear         = 2004;

        SystemTimeToVariantTime(&systemtime, &pv.date);

        hrSetValue = pStore->SetValue(WPD_OBJECT_DATE_CREATED , &pv);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_DATE_CREATED"));
            return hrSetValue;
        }

        PropVariantClear(&pv);

        // Add WPD_OBJECT_ORIGINAL_FILE_NAME
        pvValue = FileName;
        hrSetValue = pStore->SetValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_SIZE
        hrSetValue = pStore->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_IMAGE));
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_SIZE"));
            return hrSetValue;
        }

        return hr;
    }

    virtual HRESULT WriteValue(
        _In_ REFPROPERTYKEY key,
        _In_ REFPROPVARIANT Value)
    {
        HRESULT             hr      = S_OK;
        PropVariantWrapper  pvValue;

        if(IsEqualPropertyKey(key, WPD_OBJECT_ORIGINAL_FILE_NAME))
        {
            if(Value.vt == VT_LPWSTR)
            {
                if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
                {
                    FileName = Value.pwszVal;
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME because value was an empty string");
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME because type was not VT_LPWSTR");
            }
        }
        else
        {
            hr = FakeContent::WriteValue(key, Value);
            CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(key.fmtid), key.pid, ObjectID);
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
            // Add the thumbnail resource
            hr = pKeys->Add(WPD_RESOURCE_THUMBNAIL);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_THUMBNAIL to supported resource list");
        }

        if (SUCCEEDED(hr))
        {
            // Add the audio annotation resource
            hr = pKeys->Add(WPD_RESOURCE_AUDIO_CLIP);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_AUDIO_CLIP to supported resource list");
        }

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

        // Override the size attribute for this resource.
        if (SUCCEEDED(hr))
        {
            DWORD dwSize = 0;
            if (IsEqualPropertyKey(Key, WPD_RESOURCE_DEFAULT))
            {
                dwSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_IMAGE);
            }
            else if(IsEqualPropertyKey(Key, WPD_RESOURCE_THUMBNAIL))
            {
                dwSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_IMAGE_THUMBNAIL);
            }
            else if(IsEqualPropertyKey(Key, WPD_RESOURCE_AUDIO_CLIP))
            {
                dwSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_AUDIO_ANNOTATION);
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Could not return resource attributes for unknown resource %ws.%d", (LPWSTR)CComBSTR(Key.fmtid), Key.pid);
            }

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, dwSize);
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
            }
        }

        if (SUCCEEDED(hr) && (IsEqualPropertyKey(Key, WPD_RESOURCE_THUMBNAIL)))
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_MEDIA_WIDTH, 96);
            CHECK_HR(hr, "Failed to set WPD_MEDIA_WIDTH");

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_MEDIA_HEIGHT, 96);
                CHECK_HR(hr, "Failed to set WPD_MEDIA_HEIGHT");
            }

            // Override the format attribute for this resource.
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_EXIF);
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
            }
        }

        if (SUCCEEDED(hr) && (IsEqualPropertyKey(Key, WPD_RESOURCE_AUDIO_CLIP)))
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_AUDIO_BITRATE, 180224 /* 176kbps */);
            CHECK_HR(hr, "Failed to set WPD_AUDIO_BITRATE");
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetFloatValue(WPD_AUDIO_CHANNEL_COUNT, 1.0f /* Mono */);
                CHECK_HR(hr, "Failed to set WPD_AUDIO_CHANNEL_COUNT");
            }
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_AUDIO_FORMAT_CODE, WAVE_FORMAT_PCM);
                CHECK_HR(hr, "Failed to set WPD_AUDIO_FORMAT_CODE");
            }
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_MEDIA_SAMPLE_RATE, 22000 /* 22kHz */);
                CHECK_HR(hr, "Failed to set WPD_MEDIA_SAMPLE_RATE");
            }
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_AUDIO_BIT_DEPTH, 8);
                CHECK_HR(hr, "Failed to set WPD_AUDIO_BIT_DEPTH");
            }

            // Override the format attribute for this resource.
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_WAVE);
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
            pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_IMAGE);
            dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_IMAGE);
        }
        else if(IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_THUMBNAIL))
        {
            pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_IMAGE_THUMBNAIL);
            dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_IMAGE_THUMBNAIL);
        }
        else if(IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_AUDIO_CLIP))
        {
            pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_AUDIO_ANNOTATION);
            dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_AUDIO_ANNOTATION);
        }

        if (pResource == NULL)
        {
            hr = E_UNEXPECTED;
            CHECK_HR(hr, "Failed to get DLL resource representing WPD resource data");
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
        return WPD_OBJECT_FORMAT_EXIF;
    }
};

