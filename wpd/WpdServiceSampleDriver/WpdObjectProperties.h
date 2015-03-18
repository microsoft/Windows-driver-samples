#pragma once

class WpdObjectProperties
{
public:
    WpdObjectProperties();
    virtual ~WpdObjectProperties();

    HRESULT Initialize(_In_ FakeDevice* pDevice);

    HRESULT DispatchWpdMessage(
        _In_     REFPROPERTYKEY         Command,
        _In_     IPortableDeviceValues* pParams,
        _In_     IPortableDeviceValues* pResults);

    HRESULT OnGetSupportedProperties(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetPropertyValues(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetAllPropertyValues(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnSetPropertyValues(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetPropertyAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnDeleteProperties(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

private:
    FakeDevice* m_pDevice;
};
