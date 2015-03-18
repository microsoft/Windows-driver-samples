#include "mmsystem.h"
#include "FakeFolderContent.h.tmh"

class FakeFolderContent : public FakeContent
{
public:
    FakeFolderContent()
    {
    }

    FakeFolderContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeFolderContent()
    {
    }


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

};


class FakeMemoFolderContent : public FakeFolderContent
{
public:
    FakeMemoFolderContent()
    {
    }

    FakeMemoFolderContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeMemoFolderContent()
    {
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
            // Add WPD_RESOURCE_ICON
            hr = pKeys->Add(WPD_RESOURCE_ICON);
            CHECK_HR(hr, "Failed to add WPD_RESOURCE_ICON to collection");
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

        if (IsEqualPropertyKey(Key, WPD_RESOURCE_ICON))
        {
            // Fill in the common resource attributes
            hr = GetCommonResourceAttributes(&pAttributes);
            CHECK_HR(hr, "Failed to get common resource attributes set");

            // Override the size attribute for this resource.
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, GetResourceSize(IDR_WPD_SAMPLEDRIVER_MEMO_FOLDER_ICON));
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
            }

            // Override the format attribute for this resource.
            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetGuidValue(WPD_RESOURCE_ATTRIBUTE_FORMAT, WPD_OBJECT_FORMAT_ICON);
                CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_FORMAT");
            }

        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Object does not support this resource");
        }

        // Return the resource attributes
        if (SUCCEEDED(hr))
        {
            hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }

        return hr;
    }

    // This sample driver uses a embedded icon resource as its data.
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

        if (IsEqualPropertyKey(ResourceKey, WPD_RESOURCE_ICON))
        {
            pResource = GetResourceData(IDR_WPD_SAMPLEDRIVER_MEMO_FOLDER_ICON);
            dwObjectDataSize = GetResourceSize(IDR_WPD_SAMPLEDRIVER_MEMO_FOLDER_ICON);
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Object does not support this resource");
        }

        if (hr == S_OK)
        {
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

            // Copy the embedded data.
            if ((hr == S_OK) && (dwBytesToTransfer > 0))
            {
                memcpy(pBuffer, pResource + dwStartByte, dwBytesToTransfer);
            }

            if (hr == S_OK)
            {
                *pdwNumBytesRead = dwBytesToTransfer;
            }
        }

        return hr;
    }

};

