#pragma once

class WpdStorage
{
public:
    WpdStorage();
    ~WpdStorage();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    HRESULT OnFormat(_In_ IPortableDeviceValues*  pParams,
                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnMoveObject(_In_ IPortableDeviceValues*  pParams,
                         _In_ IPortableDeviceValues*  pResults);

private:

    FakeDevice*                     m_pFakeDevice;
};
