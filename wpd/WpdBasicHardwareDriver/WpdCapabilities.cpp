#include "stdafx.h"

#include "WpdCapabilities.tmh"

const PROPERTYKEY g_SupportedCommands[] =
{
    // WPD_CATEGORY_OBJECT_ENUMERATION
    WPD_COMMAND_OBJECT_ENUMERATION_START_FIND,
    WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT,
    WPD_COMMAND_OBJECT_ENUMERATION_END_FIND,

    // WPD_CATEGORY_OBJECT_PROPERTIES
    WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED,
    WPD_COMMAND_OBJECT_PROPERTIES_GET,
    WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL,
    WPD_COMMAND_OBJECT_PROPERTIES_SET,
    WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES,
    WPD_COMMAND_OBJECT_PROPERTIES_DELETE,

    // WPD_CATEGORY_CAPABILITIES
    WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_COMMANDS,
    WPD_COMMAND_CAPABILITIES_GET_COMMAND_OPTIONS,
    WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FUNCTIONAL_CATEGORIES,
    WPD_COMMAND_CAPABILITIES_GET_FUNCTIONAL_OBJECTS,
    WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_EVENTS,  
    WPD_COMMAND_CAPABILITIES_GET_EVENT_OPTIONS,
};

const GUID g_SupportedFunctionalCategories[] =
{
    WPD_FUNCTIONAL_CATEGORY_DEVICE,
    FUNCTIONAL_CATEGORY_SENSOR_SAMPLE, // Our device's functional category  
};

const GUID g_SupportedEvents[] = 
{
    EVENT_SENSOR_READING_UPDATED,
};

WpdCapabilities::WpdCapabilities()
{

}

WpdCapabilities::~WpdCapabilities()
{

}

HRESULT WpdCapabilities::DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                                            _In_ IPortableDeviceValues*  pParams,
                                            _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;

    if (Command.fmtid != WPD_CATEGORY_CAPABILITIES)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceKeyCollection> pCommands;
    UNREFERENCED_PARAMETER(pParams);

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
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommands); dwIndex++)
        {
            hr = pCommands->Add(g_SupportedCommands[dwIndex]);
            CHECK_HR(hr, "Failed to add supported command at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT     hr      = S_OK;
    PROPERTYKEY Command = WPD_PROPERTY_NULL;
    CComPtr<IPortableDeviceValues> pOptions;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

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
        // If your driver supports command options, then they should be added here
        // to the command options collection 'pOptions'.
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedFunctionalCategories); dwIndex++)
        {
            PROPVARIANT pv = {0};
            PropVariantInit(&pv);
            // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs

            pv.vt    = VT_CLSID;
            pv.puuid = (GUID*) &g_SupportedFunctionalCategories[dwIndex];

            hr = pFunctionalCategories->Add(&pv);
            CHECK_HR(hr, "Failed to add supported functional category at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr                     = S_OK;
    GUID    guidFunctionalCategory = GUID_NULL;
    CComPtr<IPortableDevicePropVariantCollection> pFunctionalObjects;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

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
        PROPVARIANT pv = {0};
        PropVariantInit(&pv);
        // Don't call PropVariantClear, since we did not allocate the memory for these object identifiers

        // Add WPD_DEVICE_OBJECT_ID to the functional object identifiers collection
        if (hr == S_OK)
        {
           
            if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_DEVICE) ||
                (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_ALL))
            {
                pv.vt       = VT_LPWSTR;
                pv.pwszVal  = WPD_DEVICE_OBJECT_ID;
                hr = pFunctionalObjects->Add(&pv);
                CHECK_HR(hr, "Failed to add device object ID");
            }
        }

        // Add FUNCTIONAL_CATEGORY_SENSOR_SAMPLE to the functional object 
        // identifiers collection
        if (hr == S_OK)
        {
            if ((guidFunctionalCategory  == FUNCTIONAL_CATEGORY_SENSOR_SAMPLE) ||
                (guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_ALL))
            {
                pv.vt       = VT_LPWSTR;
                pv.pwszVal  = SENSOR_OBJECT_ID;
                hr = pFunctionalObjects->Add(&pv);
                CHECK_HR(hr, "Failed to add sensor object ID");
            }
        }
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
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
        // populate the supported events collection
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedEvents); dwIndex++)
        {
            PROPVARIANT pv = {0};
            PropVariantInit(&pv);
            // Don't call PropVariantClear, since we did not allocate the memory for these GUIDs

            pv.vt    = VT_CLSID;
            pv.puuid = (GUID*) &g_SupportedEvents[dwIndex];

            hr = pEvents->Add(&pv);
            CHECK_HR(hr, "Failed to add supported event at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
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
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr    = S_OK;
    GUID    Event = GUID_NULL;
    CComPtr<IPortableDeviceValues> pOptions;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

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
        //  Check for the events we support
        if (Event == EVENT_SENSOR_READING_UPDATED)
        {
            // These events are broadcast events
            hr = pOptions->SetBoolValue(WPD_EVENT_OPTION_IS_BROADCAST_EVENT, TRUE);
            CHECK_HR(hr, "Failed to set WPD_EVENT_OPTION_IS_BROADCAST_EVENT");
        }
    }

    // Set the WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS value in the results.
    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS, pOptions);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS");
    }

    return hr;
}


