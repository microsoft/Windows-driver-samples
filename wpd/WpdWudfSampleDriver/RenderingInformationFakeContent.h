#include "RenderingInformationFakeContent.h.tmh"

class RenderingInformationFakeContent : public FakeContent
{
public:
    RenderingInformationFakeContent()
    {
    }

    RenderingInformationFakeContent(const RenderingInformationFakeContent& src)
    {
        *this = src;
    }

    virtual ~RenderingInformationFakeContent()
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
            hr = AddSupportedProperties(WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, ppKeys);
            CHECK_HR(hr, "Failed to add additional properties for RenderingInformationFakeContent");
        }

        return hr;
    }

    virtual HRESULT GetAllValues(
        _In_ IPortableDeviceValues*  pStore)
    {
        HRESULT             hr          = S_OK;
        HRESULT             hrSetValue  = S_OK;

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
        hrSetValue = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hr, "Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY");
            return hrSetValue;
        }

        // Add WPD_RENDERING_INFORMATION_PROFILES
        hrSetValue = SetRenderingProfiles(pStore);
        if (hrSetValue != S_OK)
        {
            CHECK_HR(hrSetValue, ("Failed to set WPD_RENDERING_INFORMATION_PROFILES"));
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
            hr = AddFixedPropertyAttributes(WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, Key, pAttributes);
            CHECK_HR(hr, "Failed to add fixed property attributes for %ws.%d on RenderingInformationFakeContent", CComBSTR(Key.fmtid), Key.pid);
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

    virtual GUID GetObjectFormat()
    {
        return WPD_OBJECT_FORMAT_UNSPECIFIED;
    }
};

