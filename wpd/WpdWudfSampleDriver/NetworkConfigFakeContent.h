#include "NetworkConfigFakeContent.h.tmh"

class NetworkConfigFakeContent : public FakeContent
{
public:
    NetworkConfigFakeContent()
    {
    }

    virtual ~NetworkConfigFakeContent()
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
            hr = AddSupportedProperties(WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for NetworkConfigFakeContent");
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

        // Add WPD_FUNCTIONAL_OBJECT_CATEGORY
        hrSetValue = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
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
            hr = AddFixedPropertyAttributes(WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, Key, pAttributes);
            CHECK_HR(hr, "Failed to add fixed property attributes for %ws.%d on NetworkConfigFakeContent", CComBSTR(Key.fmtid), Key.pid);
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
            hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceKeyCollection,
                                  (VOID**) &pKeys);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
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
        *ppAttributes = NULL;
        return E_NOTIMPL;
    }

    virtual HRESULT ReadData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead) BYTE*   pBuffer,
                DWORD           dwNumBytesToRead,
        _Out_   DWORD*          pdwNumBytesRead)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        UNREFERENCED_PARAMETER(dwStartByte);
        UNREFERENCED_PARAMETER(pBuffer);
        UNREFERENCED_PARAMETER(dwNumBytesToRead);
        *pdwNumBytesRead = 0;
        return E_NOTIMPL;
    }

    virtual HRESULT WriteData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _In_reads_(dwNumBytesToWrite) BYTE* pBuffer,
                DWORD           dwNumBytesToWrite,
        _Out_   DWORD*          pdwNumBytesWritten)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        UNREFERENCED_PARAMETER(dwStartByte);
        UNREFERENCED_PARAMETER(pBuffer);
        UNREFERENCED_PARAMETER(dwNumBytesToWrite);
        *pdwNumBytesWritten = 0;
        return E_NOTIMPL;
    }

    virtual GUID GetObjectFormat()
    {
        return WPD_OBJECT_FORMAT_PROPERTIES_ONLY;
    }
};

class FakeNetworkAssociationContent : public FakeContent
{
public:
    FakeNetworkAssociationContent()
    {
        PropVariantInit(&HostEUI64Array);
    }

    virtual ~FakeNetworkAssociationContent()
    {
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

        hr = FakeContent::GetAllValues(pStore);
        if (FAILED(hr))
        {
            CHECK_HR(hr, "Failed to get basic property set");
            return hr;
        }

        // Add WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS
        if (HostEUI64Array.vt == VT_EMPTY)
        {
            // This value is supposed to be available, but it is just a soft error if it is missing
            pvValue.SetErrorValue(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
            hrSetValue = pStore->SetValue(WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS, &pvValue);

            hr = S_FALSE;
        }
        else
        {
            hrSetValue = pStore->SetValue(WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS, &HostEUI64Array);
        }
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS"));
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

        if(IsEqualPropertyKey(key, WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS))
        {
            if (Value.vt == (VARTYPE)(VT_VECTOR | VT_UI1))
            {
                if ((Value.caub.cElems % 8) == 0)
                {
                    if (FAILED(PropVariantClear(&HostEUI64Array)))
                    {
                        PropVariantInit(&HostEUI64Array);
                    }

                    hr = PropVariantCopy(&HostEUI64Array, &Value);
                    CHECK_HR(hr, "Error setting WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS");
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Failed to set WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS because value was the wrong length");
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS because type was not VT_VECTOR|VT_UI1");
            }
        }
        else
        {
            hr = FakeContent::WriteValue(key, Value);
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
        *ppAttributes = NULL;
        return E_NOTIMPL;
    }

    virtual HRESULT ReadData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead) BYTE*   pBuffer,
                DWORD           dwNumBytesToRead,
        _Out_   DWORD*          pdwNumBytesRead)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        UNREFERENCED_PARAMETER(dwStartByte);
        UNREFERENCED_PARAMETER(pBuffer);
        UNREFERENCED_PARAMETER(dwNumBytesToRead);
        *pdwNumBytesRead = 0;
        return E_NOTIMPL;
    }

    virtual HRESULT WriteData(
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _In_reads_(dwNumBytesToWrite) BYTE* pBuffer,
                DWORD           dwNumBytesToWrite,
        _Out_   DWORD*          pdwNumBytesWritten)
    {
        UNREFERENCED_PARAMETER(ResourceKey);
        UNREFERENCED_PARAMETER(dwStartByte);
        UNREFERENCED_PARAMETER(pBuffer);
        UNREFERENCED_PARAMETER(dwNumBytesToWrite);
        *pdwNumBytesWritten = 0;
        return E_NOTIMPL;
    }

    virtual GUID GetObjectFormat()
    {
        return WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION;
    }

    PROPVARIANT HostEUI64Array;
};


class FakeWirelessProfileContent : public FakeContent
{
public:
    FakeWirelessProfileContent()
    {
    }

    virtual ~FakeWirelessProfileContent()
    {
    }

    virtual GUID GetObjectFormat()
    {
        return WPD_OBJECT_FORMAT_MICROSOFT_WFC;
    }
};


