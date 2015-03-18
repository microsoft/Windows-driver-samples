#pragma once

class WpdObjectProperties
{
public:
    WpdObjectProperties();
    ~WpdObjectProperties();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedProperties(_In_ IPortableDeviceValues*  pParams,
                                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetValues(_In_ IPortableDeviceValues*  pParams,
                        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetAllValues(_In_ IPortableDeviceValues*  pParams,
                           _In_ IPortableDeviceValues*  pResults);

    HRESULT OnWriteProperties(_In_ IPortableDeviceValues*  pParams,
                              _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetAttributes(_In_ IPortableDeviceValues*  pParams,
                            _In_ IPortableDeviceValues*  pResults);

    HRESULT OnDelete(_In_ IPortableDeviceValues*  pParams,
                     _In_ IPortableDeviceValues*  pResults);

private:

    FakeDevice* m_pFakeDevice;
};
