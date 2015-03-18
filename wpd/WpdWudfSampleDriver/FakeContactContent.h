#include "FakeContactContent.h.tmh"

class FakeContactContent : public FakeContent
{
public:
    FakeContactContent() :
        bHasContactPhoto(FALSE)
    {
    }

    FakeContactContent(BOOL bHasPhoto)
    {
        bHasContactPhoto = bHasPhoto;
    }

    FakeContactContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeContactContent()
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
            hr = AddSupportedProperties(WPD_OBJECT_FORMAT_VCARD2, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for FakeContactContent");
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
            CHECK_HR(hr, "Failed to get basic property values");
            return hr;
        }

        // Add WPD_OBJECT_ORIGINAL_FILE_NAME.  We construct the original filename from the display name plus the ".vcf" extension,
        // since this object's default resource is a VCARD.
        CAtlStringW strFileName;
        strFileName.Format(L"%ws.vcf", DisplayName.GetString());
        hrSetValue = pStore->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, strFileName);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_CONTACT_DISPLAY_NAME"));
            return hrSetValue;
        }

        // Add WPD_CONTACT_DISPLAY_NAME
        hrSetValue = pStore->SetStringValue(WPD_CONTACT_DISPLAY_NAME, DisplayName);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_CONTACT_DISPLAY_NAME"));
            return hrSetValue;
        }

        // Add WPD_CONTACT_PRIMARY_PHONE
        hrSetValue = pStore->SetStringValue(WPD_CONTACT_PRIMARY_PHONE, PrimaryPhone);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_CONTACT_PRIMARY_PHONE"));
            return hrSetValue;
        }

        // Add WPD_CONTACT_BUSINESS_PHONE
        hrSetValue = pStore->SetStringValue(WPD_CONTACT_BUSINESS_PHONE, WorkPhone);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_CONTACT_BUSINESS_PHONE"));
            return hrSetValue;
        }

        // Add WPD_CONTACT_MOBILE_PHONE
        hrSetValue = pStore->SetStringValue(WPD_CONTACT_MOBILE_PHONE, CellPhone);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_CONTACT_MOBILE_PHONE"));
            return hrSetValue;
        }

        // Add WPD_OBJECT_SIZE.  Object Size is defined to be the same size as the default resource.
        // In this case the Contact Photo resource (if the object has one) is not counted in the object size.  This is
        // because the default resource for this object contains text-only VCARD2 data, and doesn't include the Contact Photo.
        // When an application transfers the default resource for this object, it will only get the VCARD2 text data, and the
        // Contact Photo resource has to be transferred separately.
        // This is done mainly for illustrative purposes (and convenience in the sample).  In most cases, it is better to
        // embed the non-default resource data in the default resource when the file format allows it
        // (e.g. a VCARD2 with an embedded contact photo rather than a separate one).  It does involve more work
        // (because the driver has to extract the embedded data) but is usually a better experience.
        CAtlStringA strVCard;
        DWORD       dwSize = 0;
        hr = CreateVCard(pStore, strVCard);
        if (hr == S_OK)
        {
            dwSize = strVCard.GetLength() + 1;
            // Add WPD_OBJECT_SIZE
            hrSetValue = pStore->SetUnsignedIntegerValue(WPD_OBJECT_SIZE, dwSize);
            if (hrSetValue != S_OK)
            {
                CHECK_HR(hrSetValue, ("Failed to set WPD_OBJECT_SIZE"));
                return hrSetValue;
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

        if(IsEqualPropertyKey(key, WPD_CONTACT_DISPLAY_NAME) ||
           IsEqualPropertyKey(key, WPD_CONTACT_PERSONAL_PHONE) ||
           IsEqualPropertyKey(key, WPD_CONTACT_BUSINESS_PHONE) ||
           IsEqualPropertyKey(key, WPD_CONTACT_MOBILE_PHONE))
        {
            if(Value.vt == VT_LPWSTR)
            {
                if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
                {
                    // Basic validation passed, so save the value appropriately
                    if(IsEqualPropertyKey(key, WPD_CONTACT_DISPLAY_NAME))
                    {
                        DisplayName = Value.pwszVal;
                    }
                    else if(IsEqualPropertyKey(key, WPD_CONTACT_PRIMARY_PHONE))
                    {
                        PrimaryPhone = Value.pwszVal;
                    }
                    else if(IsEqualPropertyKey(key, WPD_CONTACT_BUSINESS_PHONE))
                    {
                        WorkPhone = Value.pwszVal;
                    }
                    else if(IsEqualPropertyKey(key, WPD_CONTACT_MOBILE_PHONE))
                    {
                        CellPhone = Value.pwszVal;
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Failed to set property because value was an empty string");
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set property because type was not VT_LPWSTR");
            }
        }
        else
        {
            // Call the base class to write the standard properties
            hr = FakeContent::WriteValue(key, Value);
            CHECK_HR(hr, "Failed to write basic value");
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

        if (SUCCEEDED(hr) && (bHasContactPhoto == TRUE))
        {
            // Add the contact photo resource
            hr = pKeys->Add(WPD_RESOURCE_CONTACT_PHOTO);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_CONTACT_PHOTO to supported resource list");
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

        // Override the size attribute for this resource.
        if (SUCCEEDED(hr))
        {
            DWORD dwSize = 0;
            if (IsEqualPropertyKey(Key, WPD_RESOURCE_DEFAULT))
            {
                CAtlStringA strVCard;
                hr = GetVCardString(strVCard);
                if (hr == S_OK)
                {
                    dwSize = strVCard.GetLength() + 1;
                }
            }
            else if((IsEqualPropertyKey(Key, WPD_RESOURCE_CONTACT_PHOTO)) && (bHasContactPhoto == TRUE))
            {
                dwSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_CONTACT_PHOTO);
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

        if (SUCCEEDED(hr) && (IsEqualPropertyKey(Key, WPD_RESOURCE_CONTACT_PHOTO)))
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_MEDIA_WIDTH, 100);
            CHECK_HR(hr, "Failed to set WPD_MEDIA_WIDTH");

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_MEDIA_HEIGHT, 134);
                CHECK_HR(hr, "Failed to set WPD_MEDIA_HEIGHT");
            }

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_IMAGE_BITDEPTH, 32);
                CHECK_HR(hr, "Failed to set WPD_IMAGE_BITDEPTH");
            }

            if (SUCCEEDED(hr))
            {
                // Override the format attribute for this resource.
                hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_PNG);
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
        CAtlStringA strVCard;

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
            hr = GetVCardString(strVCard);
            if (hr == S_OK)
            {
                pResource = (PBYTE) strVCard.GetString();
                dwObjectDataSize = strVCard.GetLength() + 1;
            }
        }
        else if((IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_CONTACT_PHOTO)) && (bHasContactPhoto == TRUE))
        {
            pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_CONTACT_PHOTO);
            dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_CONTACT_PHOTO);
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

                // Copy the embedded image file data.
                if (dwBytesToTransfer > 0)
                {
                    memcpy(pBuffer, pResource + dwStartByte, dwBytesToTransfer);
                    *pdwNumBytesRead = dwBytesToTransfer;
                }
            }
        }
        return hr;
    }

    virtual GUID GetObjectFormat()
    {
        return WPD_OBJECT_FORMAT_VCARD2;
    }

    virtual HRESULT EnableResource(
        _In_ REFPROPERTYKEY ResourceKey)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        bHasContactPhoto = TRUE;
        return S_OK;
    }

    HRESULT GetVCardString(
        _Out_ CAtlStringA&    strVCard)
    {
        HRESULT hr = S_OK;
        CComPtr<IPortableDeviceValues> pContactProperties;

        strVCard = L"";

        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pContactProperties);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

        if (hr == S_OK)
        {
            hr = GetAllValues(pContactProperties);
            CHECK_HR(hr, "Failed to get all values for contact object [%ws]", ObjectID);
        }

        if (hr == S_OK)
        {
            hr = CreateVCard(pContactProperties, strVCard);
            CHECK_HR(hr, "Failed to create VCard string");
        }

        return hr;
    }

    CAtlStringW DisplayName;
    CAtlStringW PrimaryPhone;
    CAtlStringW WorkPhone;
    CAtlStringW CellPhone;
    BOOL        bHasContactPhoto;
};

