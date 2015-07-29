#pragma once

class WpdCapabilities
{
public:
    WpdCapabilities();
    virtual ~WpdCapabilities();

    HRESULT Initialize();

    HRESULT DispatchWpdMessage(
        _In_ REFPROPERTYKEY          Command,
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedCommands(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetCommandOptions(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetFunctionalCategories(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetFunctionalObjects(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedEvents(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetEventOptions(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

};

