#include "FakeVideoContent.h.tmh"

class FakeVideoContent : public FakeContent
{
public:
    FakeVideoContent()
    {
    }

    FakeVideoContent(const FakeVideoContent& src)
    {
        *this = src;
    }

    virtual ~FakeVideoContent()
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
            hr = AddSupportedProperties(WPD_OBJECT_FORMAT_WMV, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for FakeVideoContent");
        }

        return hr;
    }

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pStore)
    {
        HRESULT             hr          = S_OK;
        HRESULT             hrSetValue  = S_OK;
        CAtlStringW         strVal;
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

        // Add WPD_MEDIA_TITLE
        strVal.Format(L"Video_%ws", ObjectID.GetString());
        hrSetValue = pStore->SetStringValue(WPD_MEDIA_TITLE, strVal.GetString());
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MEDIA_TITLE"));
            return hrSetValue;
        }

        // Add WPD_MEDIA_DURATION
        ULONGLONG ulDuration = 6000;
        hrSetValue = pStore->SetUnsignedLargeIntegerValue(WPD_MEDIA_DURATION, ulDuration);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MUSIC_DURATION"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_SIZE
        hrSetValue = pStore->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_VIDEO));
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_SIZE"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_DATE_AUTHORED and WPD_OBJECT_DATE_MODIFIED
        SYSTEMTIME systemtime = {0};
        PROPVARIANT pv = {0};
        PropVariantInit(&pv);
        pv.vt = VT_DATE;
        systemtime.wDay          = 27;
        systemtime.wDayOfWeek    = 1;
        systemtime.wHour         = 5;
        systemtime.wMinute       = 30;
        systemtime.wMilliseconds = 100;
        systemtime.wMonth        = 6;
        systemtime.wSecond       = 15;
        systemtime.wYear         = 2004;

        SystemTimeToVariantTime(&systemtime, &pv.date);

        hrSetValue = pStore->SetValue(WPD_OBJECT_DATE_AUTHORED , &pv);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_DATE_AUTHORED "));
            return hrSetValue;
        }
        hrSetValue = pStore->SetValue(WPD_OBJECT_DATE_MODIFIED , &pv);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_DATE_MODIFIED "));
            return hrSetValue;
        }

        PropVariantClear(&pv);

        // Add WPD_MEDIA_WIDTH
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_MEDIA_WIDTH, 160);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MEDIA_WIDTH"));
            return hrSetValue;
        }

        // Add WPD_MEDIA_HEIGHT
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_MEDIA_HEIGHT, 120);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_MEDIA_HEIGHT"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_ORIGINAL_FILE_NAME
        pvValue = FileName;
        hrSetValue = pStore->SetValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pvValue);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME"));
            return hrSetValue;
        }

        // Add WPD_VIDEO_SCAN_TYPE
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_VIDEO_SCAN_TYPE, WPD_VIDEO_SCAN_TYPE_UNUSED);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_VIDEO_SCAN_TYPE"));
            return hrSetValue;
        }

        // Add WPD_VIDEO_BITRATE
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_VIDEO_BITRATE, 40960);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_VIDEO_BITRATE"));
            return hrSetValue;
        }

        // Add WPD_VIDEO_FOURCC_CODE
        hrSetValue = pStore->SetUnsignedIntegerValue(WPD_VIDEO_FOURCC_CODE, MAKEFOURCC('W', 'M', 'V', '3'));
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_VIDEO_FOURCC_CODE"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_GENERATE_THUMBNAIL_FROM_RESOURCE
        hrSetValue = pStore->SetBoolValue(WPD_OBJECT_GENERATE_THUMBNAIL_FROM_RESOURCE, TRUE);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_GENERATE_THUMBNAIL_FROM_RESOURCE "));
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
            hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_VIDEO));
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
        }

        // Override the format attribute for this resource.
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_WMV);
            CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
        }

        // Return the resource attributes
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }
        return hr;
    }

    // This sample driver uses a embedded music file resource as its data.
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

        UNREFERENCED_PARAMETER(ResourceKey);

        if((pBuffer         == NULL) ||
           (pdwNumBytesRead == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        *pdwNumBytesRead = 0;

        pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_VIDEO);
        dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_VIDEO);

        if (pResource == NULL)
        {
            hr = E_UNEXPECTED;
            CHECK_HR(hr, "Failed to get resource representing image data");
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

        // Copy the embedded music file data.
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
        return WPD_OBJECT_FORMAT_WMV;
    }
};

