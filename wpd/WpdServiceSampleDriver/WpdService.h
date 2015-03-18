#pragma once

class WpdService
{
public:
    WpdService();
    virtual ~WpdService();

    HRESULT Initialize(_In_ FakeDevice* pDevice);

    HRESULT DispatchWpdMessage(
        _In_    REFPROPERTYKEY         Command,
        _In_    IPortableDeviceValues* pParams,
        _In_    IPortableDeviceValues* pResults);

private:
    HRESULT OnGetServiceObjectID(
        _In_    LPCWSTR                pszRequestFilename,
        _In_    IPortableDeviceValues* pParams,
        _In_    IPortableDeviceValues* pResults);

    HRESULT CheckRequestFilename(
        _In_    LPCWSTR                 pszRequestFilename);

private:
    WpdServiceMethods       m_ServiceMethods;
    WpdServiceCapabilities  m_ServiceCapabilities;
    FakeContactsService*    m_pContactsService;
};

