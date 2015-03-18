#pragma once

class WpdNetworkConfig
{
public:
    WpdNetworkConfig();
    ~WpdNetworkConfig();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);


    HRESULT OnProcessWFCObject(_In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);
private:
    FakeDevice* m_pFakeDevice;
};

