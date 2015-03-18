#include "stdafx.h"

#include "WpdServiceCapabilities.tmh"

WpdServiceCapabilities::WpdServiceCapabilities() : m_pContactsService(NULL)
{

}

WpdServiceCapabilities::~WpdServiceCapabilities()
{

}

HRESULT WpdServiceCapabilities::Initialize(_In_ FakeContactsService* pContactsService)
{
    if (pContactsService == NULL)
    {
        return E_POINTER;
    }
    m_pContactsService = pContactsService;
    return S_OK;
}

HRESULT WpdServiceCapabilities::DispatchWpdMessage(
    _In_    REFPROPERTYKEY         Command,
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;
    if (Command.fmtid != WPD_CATEGORY_SERVICE_CAPABILITIES)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        return hr;
    }

    if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_COMMANDS))
    {
        hr = OnGetSupportedCommands(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported commands");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_COMMAND_OPTIONS))
    {
        hr = OnGetCommandOptions(pParams, pResults);
        CHECK_HR(hr, "Failed to get command options");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS))
    {
        hr = OnGetSupportedMethods(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported methods");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS_BY_FORMAT))
    {
        hr = OnGetSupportedMethodsByFormat(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported methods by format");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_ATTRIBUTES))
    {
        hr = OnGetMethodAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get method attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_PARAMETER_ATTRIBUTES))
    {
        hr = OnGetMethodParameterAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get method parameter attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMATS))
    {
        hr = OnGetSupportedFormats(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported formats");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_ATTRIBUTES))
    {
        hr = OnGetFormatAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get format attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES))
    {
        hr = OnGetSupportedFormatProperties(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported format properties");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_PROPERTY_ATTRIBUTES))
    {
        hr = OnGetFormatPropertyAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get format property attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_EVENTS))
    {
        hr = OnGetSupportedEvents(pParams, pResults);
        CHECK_HR(hr, "Failed to get supported events");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_ATTRIBUTES))
    {
        hr = OnGetEventAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get event attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_PARAMETER_ATTRIBUTES))
    {
        hr = OnGetEventParameterAttributes(pParams, pResults);
        CHECK_HR(hr, "Failed to get event parameter attributes");
    }
    else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_CAPABILITIES_GET_INHERITED_SERVICES))
    {
        hr = OnGetInheritedServices(pParams, pResults);
        CHECK_HR(hr, "Failed to get inherited services");
    }
    else
    {
        hr = E_NOTIMPL;
        CHECK_HR(hr, "This object does not support this command id %d", Command.pid);
    }

    return hr;
}


/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_COMMANDS
 *  command.
 *
 *  The parameters sent to us are:
 *  - none.
 *
 *  The driver should:
 *  - Return all commands supported by this service as an
 *    IPortableDeviceKeyCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_COMMANDS.
 *    This includes custom commands, if any.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedCommands(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    UNREFERENCED_PARAMETER(pParams);
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceKeyCollection> pCommands;

    // CoCreate a collection to store the supported commands.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pCommands);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    // Add the supported commands to the collection.
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedCommands(pCommands);
        CHECK_HR(hr, "Failed to get the supported commands");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_COMMANDS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_COMMANDS, pCommands);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_COMMANDS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_COMMAND_OPTIONS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND: a collection of property keys containing a single value,
 *      which identifies the specific command options are requested to return.
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND_OPTIONS, containing
 *      the relevant options.  If no options are available for this command, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetCommandOptions(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr      = S_OK;
    PROPERTYKEY Command = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues> pOptions;

    // Get the command whose options have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND, &Command);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND");
    }

    // CoCreate a collection to store the command options.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pOptions);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Add command options to the collection
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetCommandOptions(Command, pOptions);
        CHECK_HR(hr, "Failed to get the command options");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND_OPTIONS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND_OPTIONS, pOptions);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_COMMAND_OPTIONS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS command.
 *
 *  The parameters sent to us are:
 *  - none.
 *
 *  The driver should:
 *  - Return all methods supported by this service as an
 *    IPortableDevicePropVariantCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS.
 *    If no methods are available for this service, the driver should return an IPortableDevicePropVariantCollection 
 *    with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedMethods(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    UNREFERENCED_PARAMETER(pParams);

    CComPtr<IPortableDevicePropVariantCollection> pMethods;

    // CoCreate a collection to store the supported methods.
    HRESULT hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IPortableDevicePropVariantCollection,
                                      (VOID**) &pMethods);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedMethods(pMethods);
        CHECK_HR(hr, "Failed to get the supported methods");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS, pMethods);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS_BY_FORMAT command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT: Identifies the format whose methods are being requested
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS, 
 *    containing the supported methods that apply to this format.  If no methods are available for this format, 
 *    the driver should return an IPortableDevicePropVariantCollection with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedMethodsByFormat(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr     = S_OK;
    GUID    Format = GUID_NULL;

    CComPtr<IPortableDevicePropVariantCollection> pMethods;

    // Get the format parameter
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT, &Format);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT");

    if (hr == S_OK)
    {
        // CoCreate a collection to store the supported methods.
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pMethods);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedMethodsByFormat(Format, pMethods);
        CHECK_HR(hr, "Failed to get the supported methods by format");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS, pMethods);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_ATTRIBUTES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD: Identifies the method whose attributes are being requested
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD_ATTRIBUTES, containing
 *      the method attributes.  If no attributes are available for this method, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetMethodAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr     = S_OK;
    GUID    Method = GUID_NULL;
    
    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the method
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD, &Method);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD");
    
    if (hr == S_OK)
    {
        // CoCreate a collection to store the method attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetMethodAttributes(Method, pAttributes);
        CHECK_HR(hr, "Failed to add method attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_METHODS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_PARAMETER_ATTRIBUTES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER: Identifies the parameter whose attributes are being requested
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_METHOD_PARAMETER_ATTRIBUTES, containing
 *      the parameter attributes.  If no attributes are available for this parameter, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetMethodParameterAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr        = S_OK;
    PROPERTYKEY Parameter = WPD_PROPERTY_NULL;
    
    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the method
    hr = pParams->GetKeyValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER, &Parameter);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER");
    
    if (hr == S_OK)
    {
        // CoCreate a collection to store the parameter attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetMethodParameterAttributes(Parameter, pAttributes);
        CHECK_HR(hr, "Failed to get the method parameter attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMATS command.
 *
 *  The parameters sent to us are:
 *  - None
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_FORMATS, containing
 *      the supported formats for the service.  If no formats are supported by this service, the driver should
 *      return an IPortableDevicePropVariantCollection with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedFormats(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    UNREFERENCED_PARAMETER(pParams);

    CComPtr<IPortableDevicePropVariantCollection> pFormats;
   
    // CoCreate a collection to store the formats.
    HRESULT hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDevicePropVariantCollection,
                          (VOID**) &pFormats);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedFormats(pFormats);
        CHECK_HR(hr, "Failed to get the supported formats");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_FORMATS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMATS, pFormats);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_FORMATS");
    }
   
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_ATTRIBUTES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT: Identifies the format whose attributes are being requested
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT_ATTRIBUTES, containing
 *      the attributes for the format.  If no attributes are supported by the format, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetFormatAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr     = S_OK;
    GUID    Format = GUID_NULL;
    
    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the format
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT, &Format);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT");
    
    if (hr == S_OK)
    {
        // CoCreate a collection to store the attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetFormatAttributes(Format, pAttributes);
        CHECK_HR(hr, "Failed to add format attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT_ATTRIBUTES");
    }
   
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES command.
 *  This list is the super-set of all properties that will be supported by an object of the given format. 
 *  Individual objects can be queried for their properties using WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED. 
 *  Note that this method is generally much quicker than calling WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED, 
 *  since the driver does not have to perform a dynamic lookup based on a specific object.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT: Identifies the format whose attributes are being requested
 *
 *  The driver should:
 *  - Return an IPortableDeviceKeyCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS, containing
 *      the supported properties for the format.  If no properties are supported by the format, the driver should
 *      return an IPortableDeviceKeyCollection with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedFormatProperties(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr     = S_OK;
    GUID    Format = GUID_NULL;
  
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // Get the format
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT, &Format);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT");

    if (hr == S_OK)
    {
        // CoCreate a collection to store the attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedFormatProperties(Format, pKeys);
        CHECK_HR(hr, "Failed to add the supported format properties");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS");
    }
   
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_PROPERTY_ATTRIBUTES command.
 *  Often, a driver treats objects of a given format the same. Many properties therefore will have attributes 
 *  that are identical across all objects of that format. These can be returned here. There are some attributes 
 *  which may be differ per object instance, which are not returned here. See WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT: Identifies the format whose property attributes are being requested
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS: An IPortableDeviceKeyCollection containing a single value, 
 *    which is the key identifying the specific property attributes the driver is requested to return.
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_ATTRIBUTES, containing
 *      the attributes for the property.  If no attributes are supported by the property, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetFormatPropertyAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr                   = S_OK;
    GUID        Format               = GUID_NULL;
    PROPERTYKEY Property             = WPD_PROPERTY_NULL;

    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the format
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT, &Format);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT");

    if (hr == S_OK)
    {
        // Get the property
        hr = pParams->GetKeyValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_KEYS, &Property);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_FORMAT");
    }

    if (hr == S_OK)
    {
        // CoCreate a collection to store the attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetPropertyAttributes(Format, Property, pAttributes);
        CHECK_HR(hr, "Failed to get the supported property attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_PROPERTY_ATTRIBUTES");
    }

    return hr;

}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_EVENTS command.
 *
 *  The parameters sent to us are:
 *  - None
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_EVENTS, containing
 *    the events for the service.  If no events are supported by the service, the driver should
 *    return an IPortableDevicePropVariantCollection with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetSupportedEvents(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    UNREFERENCED_PARAMETER(pParams);

    CComPtr<IPortableDevicePropVariantCollection> pEvents;

    // CoCreate a collection to store the supported events.
    HRESULT hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDevicePropVariantCollection,
                          (VOID**) &pEvents);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");

    // Add the supported events to the collection.
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetSupportedEvents(pEvents);
        CHECK_HR(hr, "Failed to get the supported events");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_EVENTS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_EVENTS, pEvents);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_EVENTS");
    }

    return hr;

}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_ATTRIBUTES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT: Indicates the event the caller is interested in
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT_ATTRIBUTES, containing
 *    the event attributes.  If there are no attributes for that event, the driver should
 *    return an IPortableDeviceValues with no elements in it.  
 */
HRESULT WpdServiceCapabilities::OnGetEventAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr    = S_OK;
    GUID    Event = GUID_NULL;
  
    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the format
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT, &Event);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT");

    if (hr == S_OK)
    {
        // CoCreate a collection to store the attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }
    
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetEventAttributes(Event, pAttributes);
        CHECK_HR(hr, "Failed to add event attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_SUPPORTED_EVENTS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT_ATTRIBUTES");
    }
    
    return hr;
}


/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_PARAMETER_ATTRIBUTES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER: Identifies the parameter whose attributes are being requested
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT_PARAMETER_ATTRIBUTES, containing
 *      the parameter attributes.  If no attributes are available for this parameter, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdServiceCapabilities::OnGetEventParameterAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr        = S_OK;
    PROPERTYKEY Parameter = WPD_PROPERTY_NULL;
    
    CComPtr<IPortableDeviceValues> pAttributes;

    // Get the method
    hr = pParams->GetKeyValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER, &Parameter);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER");
    
    if (hr == S_OK)
    {
        // CoCreate a collection to store the parameter attributes.
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = m_pContactsService->GetEventParameterAttributes(Parameter, pAttributes);
        CHECK_HR(hr, "Failed to get the event parameter attributes");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_PARAMETER_ATTRIBUTES");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_CAPABILITIES_GET_INHERITED_SERVICES command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITANCE_TYPE: Indicates the inheritance type the caller is interested in
 *    Possible values are from the WPD_SERVICE_INHERITANCE_TYPES enumeration 
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection in WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITED_SERVICES, containing
 *    the inherited services.  For WPD_SERVICE_INHERITANCE_IMPLEMENTATION, this will be an 
 *    IPortableDevicePropVariantCollection (of type VT_CLSID) containing the inherited service type GUIDs.
 *    If there are no inherited services, the driver should return an IPortableDevicePropVariantCollection with no elements in it.  
 */
HRESULT WpdServiceCapabilities::OnGetInheritedServices(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                = S_OK;
    DWORD   dwInheritanceType = 0;

    CComPtr<IPortableDevicePropVariantCollection> pServices;

    hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITANCE_TYPE, &dwInheritanceType);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITANCE_TYPE");

    if (hr == S_OK)
    {
        // CoCreate a collection to store the attributes.
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pServices);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }
    
    if (hr == S_OK)
    {
        hr = m_pContactsService->GetInheritedServices(dwInheritanceType, pServices);
        CHECK_HR(hr, "Failed to add inherited services");
    }

    // Set the WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITED_SERVICES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_SERVICE_CAPABILITIES_INHERITED_SERVICES, pServices);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_CAPABILITIES_EVENT_ATTRIBUTES");
    }
    
    return hr;
}

