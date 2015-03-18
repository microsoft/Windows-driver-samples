#pragma once

class WpdObjectManagement
{
public:
    WpdObjectManagement();
    ~WpdObjectManagement();

    HRESULT Initialize(_In_ FakeDevice* m_pDevice);

    HRESULT DispatchWpdMessage(
        _In_     REFPROPERTYKEY         Command,
        _In_     IPortableDeviceValues* pParams,
        _In_     IPortableDeviceValues* pResults);


    HRESULT OnCreateObjectWithPropertiesOnly(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnDelete(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

private:
    FakeDevice* m_pDevice;
};
