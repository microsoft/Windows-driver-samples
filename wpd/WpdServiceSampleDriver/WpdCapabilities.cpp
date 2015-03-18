#include "stdafx.h"

#include "WpdCapabilities.tmh"

WpdCapabilities::WpdCapabilities() : m_pDevice(NULL)
{

}

WpdCapabilities::~WpdCapabilities()
{

}

HRESULT WpdCapabilities::Initialize(_In_ FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pDevice = pDevice;
    return S_OK;
}

HRESULT WpdCapabilities::DispatchWpdMessage(_In_    REFPROPERTYKEY      Command,
                                            _In_    IPortableDeviceValues*  pParams,
                                            _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_CAPABILITIES)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_COMMANDS))
        {
            hr = OnGetSupportedCommands(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported commands");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_COMMAND_OPTIONS))
        {
            hr = OnGetCommandOptions(pParams, pResults);
            CHECK_HR(hr, "Failed to get command options");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FUNCTIONAL_CATEGORIES))
        {
            hr = OnGetFunctionalCategories(pParams, pResults);
            CHECK_HR(hr, "Failed to get functional categories");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_FUNCTIONAL_OBJECTS))
        {
            hr = OnGetFunctionalObjects(pParams, pResults);
            CHECK_HR(hr, "Failed to get functional objects");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES))
        {
            hr = OnGetSupportedContentTypes(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported content types");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMATS))
        {
            hr = OnGetSupportedFormats(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported formats");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES))
        {
            hr = OnGetSupportedFormatProperties(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported format properties");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_FIXED_PROPERTY_ATTRIBUTES))
        {
            hr = OnGetFixedPropertyAttributes(pParams, pResults);
            CHECK_HR(hr, "Failed to get fixed property attributes");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_EVENTS))
        {
            hr = OnGetSupportedEvents(pParams, pResults);
            CHECK_HR(hr, "Failed to get supported events");
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_CAPABILITIES_GET_EVENT_OPTIONS))
        {
            hr = OnGetEventOptions(pParams, pResults);
            CHECK_HR(hr, "Failed to get event options");
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "This object does not support this command id %d", Command.pid);
        }
    }
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_COMMANDS
 *  command.
 *
 *  The parameters sent to us are:
 *  - none.
 *
 *  The driver should:
 *  - Return all commands supported by this driver as an
 *    IPortableDeviceKeyCollection in WPD_PROPERTY_CAPABILITIES_SUPPORTED_COMMANDS.
 *    This includes custom commands, if any.
 *
 *    Note that certain commands require a "command target" to function correctly.
 *    (e.g. delete object command) It is understood that not all objects are necessarily
 *    valid targets (e.g. you cannot delete the device object).
 */
HRESULT WpdCapabilities::OnGetSupportedCommands(
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
        hr = m_pDevice->GetSupportedCommands(pCommands);
        CHECK_HR(hr, "Failed to get the supported commands");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_SUPPORTED_COMMANDS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_SUPPORTED_COMMANDS, pCommands);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_SUPPORTED_COMMANDS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_COMMAND_OPTIONS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_COMMAND: a collection of property keys containing a single value,
 *      which identifies the specific command options are requested to return.
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_CAPABILITIES_COMMAND_OPTIONS, containing
 *      the relevant options.  If no options are available for this command, the driver should
 *      return an IPortableDeviceValues with no elements in it.
 */
HRESULT WpdCapabilities::OnGetCommandOptions(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr      = S_OK;
    PROPERTYKEY Command = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues> pOptions;

    // Get the command whose options have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_CAPABILITIES_COMMAND, &Command);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_COMMAND");
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
        hr = m_pDevice->GetCommandOptions(Command, pOptions);
        CHECK_HR(hr, "Failed to get the command options");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_COMMAND_OPTIONS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_COMMAND_OPTIONS, pOptions);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_COMMAND_OPTIONS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FUNCTIONAL_CATEGORIES
 *  command.
 *
 *  The parameters sent to us are:
 *  - none.
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection (of type VT_CLSID) in
 *      WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORIES, containing
 *      the supported functional categories for this device.
 */
HRESULT WpdCapabilities::OnGetFunctionalCategories(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDevicePropVariantCollection> pFunctionalCategories;

    UNREFERENCED_PARAMETER(pParams);

    // CoCreate a collection to store the supported functional categories.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFunctionalCategories);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Add the supported functional categories to the collection.
    if (hr == S_OK)
    {
        hr = m_pDevice->GetSupportedFunctionalCategories(pFunctionalCategories);
        CHECK_HR(hr, "Failed to get the supported functional categories");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORIES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORIES, pFunctionalCategories);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORIES");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_FUNCTIONAL_OBJECTS
 *  command.  It is sent when the caller is interesting in finding the object IDs for all
 *  functional objects belonging to the specified functional category.
 *  Note: the number of functional objects is expected to be very small (less than 8 for the
 *  whole device).
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY - a GUID value containing the category
 *    the caller is looking for.  If the value is WPD_FUNCTIONAL_CATEGORY_ALL, then the driver
 *    must return all functional objects, no matter which category they belong to.
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection (of type VT_LPWSTR) in
 *      WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_OBJECTS, containing
 *      the ids of the functional objects who belong to the specified functional category.
 *      If there are no objects in the specified category, the driver should return an
 *      empty collection.
 */
HRESULT WpdCapabilities::OnGetFunctionalObjects(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                     = S_OK;
    GUID    guidFunctionalCategory = GUID_NULL;
    CComPtr<IPortableDevicePropVariantCollection> pFunctionalObjects;

    // Get the functional category whose functional object identifiers have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY, &guidFunctionalCategory);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY");
    }

    // CoCreate a collection to store the supported functional object identifiers.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFunctionalObjects);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Add the supported functional object identifiers for the specified functional
    // category to the collection.
    if (hr == S_OK)
    {
        hr = m_pDevice->GetFunctionalObjects(guidFunctionalCategory, pFunctionalObjects);
        CHECK_HR(hr, "Failed to get the functional objects");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_OBJECTS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_OBJECTS, pFunctionalObjects);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_OBJECTS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES
 *  command.  This message is sent when the client needs to know the possible content types supported
 *  by the specified functional category.
 *  If the driver has multiple functional objects that may support different content types,
 *  the driver should simply merge them together and report all possible types in one list here.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY - a GUID value containing the functional category
 *    whose content types the caller is interested in.  If the value is WPD_FUNCTIONAL_CATEGORY_ALL, then the driver
 *    must return a list of all content types supported by the device.
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection (of type VT_CLSID) in
 *      WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES, containing
 *      the content types supported by the specified functional category.
 *      If there are no objects in the specified category, the driver should return an
 *      empty collection.
 */
HRESULT WpdCapabilities::OnGetSupportedContentTypes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr                     = S_OK;
    GUID    guidFunctionalCategory = GUID_NULL;
    CComPtr<IPortableDevicePropVariantCollection> pContentTypes;

    // Get the functional category whose supported content types have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY, &guidFunctionalCategory);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY");
    }

    // CoCreate a collection to store the supported content types.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pContentTypes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Add the supported content types for the specified functional
    // category to the collection.
    // Note that the contacts service does not support any content types
    if (hr == S_OK)
    {
        hr = m_pDevice->GetSupportedContentTypes(guidFunctionalCategory, pContentTypes);
        CHECK_HR(hr, "Failed to get the content types");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES, pContentTypes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_CONTENT_TYPES");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMATS
 *  command.  This message is sent when the client needs to know the possible formats supported
 *  by the specified content type (e.g. for image objects, the driver may choose to support JPEG and BMP files).
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_CONTENT_TYPE - a GUID value containing the content type
 *    whose formats the caller is interested in.  If the value is WPD_CONTENT_TYPE_ALL, then the driver
 *    must return a list of all formats supported by the device.
 *
 *  The driver should:
 *  - Return an IPortableDevicePropVariantCollection (of type VT_CLSID) in
 *      WPD_PROPERTY_CAPABILITIES_FORMATS, indicating the formats supported by the
 *      specified content type.
 *      If there are no formats supported by the specified content type, the driver should return an
 *      empty collection.
 */
HRESULT WpdCapabilities::OnGetSupportedFormats(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr              = S_OK;
    GUID    guidContentType = GUID_NULL;
    CComPtr<IPortableDevicePropVariantCollection> pFormats;

    // Get the content type whose supported formats have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_CONTENT_TYPE, &guidContentType);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_CONTENT_TYPE");
    }

    // CoCreate a collection to store the supported formats.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFormats);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Add the supported formats for the specified content type to the collection.
    if (hr == S_OK)
    {
        hr = m_pDevice->GetSupportedFormats(guidContentType, pFormats);
        CHECK_HR(hr, "Failed to get the content types");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_FORMATS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_FORMATS, pFormats);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_FORMATS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES
 *  command.  This message is sent when the client needs to know the typical properties for objects of
 *  a given format.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_FORMAT - a GUID value specifying the format the caller is interested in.
 *
 *  The driver should:
 *  - Return an IPortableDeviceKeyCollection in WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS,
 *      containing the property keys.
 */
HRESULT WpdCapabilities::OnGetSupportedFormatProperties(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr               = S_OK;
    GUID    guidObjectFormat = GUID_NULL;
    CComPtr<IPortableDeviceKeyCollection> pKeys;

    // Get the object format whose supported properties have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FORMAT, &guidObjectFormat);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FORMAT");
    }

    // CoCreate a collection to store the supported properties.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    // Add the supported properties for the specified object format to the collection.
    if (hr == S_OK)
    {
        hr = m_pDevice->GetSupportedFormatProperties(guidObjectFormat, pKeys);
        CHECK_HR(hr, "Failed to get supported properties for a format");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS, pKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_FIXED_PROPERTY_ATTRIBUTES
 *  command.  This message is sent when the client needs to know the property attributes that
 *  are the same for all objects of the given format.
 *
 *  Typically, a driver treats objects of a given format the same. Many properties therefore will
 *  have attributes that are identical across all objects of that format.
 *  These can be returned here. There are some attributes which may be differ per object instance,
 *  which are not returned here.
 *  See WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_FORMAT - a GUID value specifying the format the caller is interested in.
 *  - WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS - a collection of property keys containing a single value,
 *    which is the key identifying the specific property attributes we are requested to return.
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_CAPABILITIES_PROPERTY_ATTRIBUTES
 *    containing the fixed property attributes.
 */
HRESULT WpdCapabilities::OnGetFixedPropertyAttributes(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr               = S_OK;
    GUID        guidObjectFormat = GUID_NULL;
    PROPERTYKEY key              = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues> pAttributes;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object format whose fixed property attributes have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FORMAT, &guidObjectFormat);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FORMAT");
    }

    // Get the property whose fixed property attributes have been requested
    if(hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS, &key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS");
    }

    // CoCreate a collection to store the fixed property attributes.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Add the fixed property attributes for the specified object format and property
    if (hr == S_OK)
    {
        hr = m_pDevice->GetFixedPropertyAttributes(guidObjectFormat, key, pAttributes);
        CHECK_HR(hr, "Failed to get fixed property attributes");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_PROPERTY_ATTRIBUTES value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_CAPABILITIES_PROPERTY_ATTRIBUTES, pAttributes);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_PROPERTY_ATTRIBUTES");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_EVENTS
 *  command.
 *
 *  The parameters sent to us are:
 *  - none.
 *
 *  The driver should:
 *  - Return all events supported by this driver should be returned as an
 *    IPortableDeviceKeyCollection in WPD_PROPERTY_CAPABILITIES_SUPPORTED_EVENTS.
 *    That includes custom commands, if any.
 */
HRESULT WpdCapabilities::OnGetSupportedEvents(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDevicePropVariantCollection> pEvents;
    UNREFERENCED_PARAMETER(pParams);

    // CoCreate a collection to store the supported events.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pEvents);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Add the supported events to the collection.
    if (hr == S_OK)
    {
        hr = m_pDevice->GetSupportedEvents(pEvents);
        CHECK_HR(hr, "Failed to get supported events");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_SUPPORTED_EVENTS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_SUPPORTED_EVENTS, pEvents);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_SUPPORTED_EVENTS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_EVENT_OPTIONS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_CAPABILITIES_EVENT: a GUID value indicating the Event whose options should be returned.
 *
 *  The driver should:
 *  - Return an IPortableDeviceValues in WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS, containing
 *      the relevant options.
 */
HRESULT WpdCapabilities::OnGetEventOptions(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr    = S_OK;
    GUID    Event = GUID_NULL;
    CComPtr<IPortableDeviceValues> pOptions;

    // Get the event whose options have been requested
    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_EVENT, &Event);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_EVENT");
    }

    // CoCreate a collection to store the event options.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pOptions);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Add event options to the collection
    if (hr == S_OK)
    {
        hr = m_pDevice->GetEventOptions(pOptions);
        CHECK_HR(hr, "Failed to get event options");
    }

    // Set the WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS, pOptions);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS");
    }

    return hr;
}
