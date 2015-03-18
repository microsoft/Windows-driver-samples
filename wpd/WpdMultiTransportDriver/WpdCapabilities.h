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

    HRESULT OnGetSupportedContentTypes(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedFormats(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedFormatProperties(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetFixedPropertyAttributes(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedEvents(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetEventOptions(
        _In_ IPortableDeviceValues*  pParams,
        _In_ IPortableDeviceValues*  pResults);

private:
    HRESULT AddSupportedPropertyKeys(_In_ REFGUID                        guidObjectFormat,
                                     _In_ IPortableDeviceKeyCollection*  pKeys);

    HRESULT GetFixedPropertyAttributesForFormat(_In_ REFGUID                guidObjectFormat,
                                                _In_ REFPROPERTYKEY         Key,
                                                _In_ IPortableDeviceValues* pAttributes);
};

