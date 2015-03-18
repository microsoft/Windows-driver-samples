#pragma once

class WpdServiceCapabilities
{
public:
    WpdServiceCapabilities();
    virtual ~WpdServiceCapabilities();

    HRESULT Initialize(_In_ FakeContactsService* pContactsService);

    HRESULT DispatchWpdMessage(
        _In_    REFPROPERTYKEY         Command,
        _In_    IPortableDeviceValues* pParams,
        _In_    IPortableDeviceValues* pResults);

private:
    HRESULT OnGetSupportedCommands(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetCommandOptions(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedMethods(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedMethodsByFormat(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetMethodAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetMethodParameterAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedFormats(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetFormatAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedFormatProperties(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetFormatPropertyAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedEvents(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetEventAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetEventParameterAttributes(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetInheritedServices(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

private:
    FakeContactsService* m_pContactsService;
};

