#pragma once

/**
 * This class represents an abstraction of a contacts service that implements 
 * the full enumeration sync model.
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */

class FakeContactsService
{
public:
    FakeContactsService() : RequestFilename(CONTACTS_SERVICE_OBJECT_ID)
    {
    }

    ~FakeContactsService()
    {
    }

    // Capabilities
    HRESULT GetSupportedCommands(
        _In_    IPortableDeviceKeyCollection*         pCommands);
    
    HRESULT GetCommandOptions(
        _In_    REFPROPERTYKEY                        Command,
        _In_    IPortableDeviceValues*                pOptions);

    HRESULT GetSupportedMethods(
        _In_    IPortableDevicePropVariantCollection* pMethods);

    BOOL IsMethodSupported(
        _In_    REFGUID Method);

    HRESULT GetSupportedMethodsByFormat(
        _In_    REFGUID                               Format, 
        _In_    IPortableDevicePropVariantCollection* pMethods);

    HRESULT GetMethodAttributes(
        _In_    REFGUID                               Method, 
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetMethodParameterAttributes(
        _In_    REFPROPERTYKEY                        Parameter, 
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetSupportedFormats(
        _In_    IPortableDevicePropVariantCollection* pFormats);

    HRESULT GetFormatAttributes(
        _In_    REFGUID                               Format, 
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetSupportedFormatProperties(
        _In_    REFGUID                               Format, 
        _In_    IPortableDeviceKeyCollection*         pKeys);

    HRESULT GetPropertyAttributes(
        _In_    REFGUID                               Format,
        _In_    REFPROPERTYKEY                        Property,
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetSupportedEvents(
        _In_    IPortableDevicePropVariantCollection* pEvents);

    HRESULT GetEventAttributes(
        _In_    REFGUID                               Event, 
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetEventParameterAttributes(
        _In_    REFPROPERTYKEY                        Parameter, 
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT GetInheritedServices(
                const DWORD                           dwInheritanceType, 
        _In_    IPortableDevicePropVariantCollection* pServices);

    // Methods
    HRESULT OnBeginSync(
        _In_    IPortableDeviceValues*                pParams, 
        _In_    IPortableDeviceValues*                pResults);

    HRESULT OnEndSync(
        _In_    IPortableDeviceValues*                pParams, 
        _In_    IPortableDeviceValues*                pResults);

    HRESULT OnMyCustomMethod(
        _In_    IPortableDeviceValues*                pParams, 
        _In_    IPortableDeviceValues*                pResults,
        _In_    IPortableDeviceValues*                pEventParameters);

    LPCWSTR GetRequestFilename()
    {
        return RequestFilename.GetString();
    }

private:
    CAtlStringW RequestFilename;
};


