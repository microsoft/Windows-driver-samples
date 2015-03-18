#include "stdafx.h"
#include "WpdCapabilities.tmh"

const PROPERTYKEY* g_SupportedCommands[] =
{
    // WPD_CATEGORY_OBJECT_ENUMERATION
    &WPD_COMMAND_OBJECT_ENUMERATION_START_FIND,
    &WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT,
    &WPD_COMMAND_OBJECT_ENUMERATION_END_FIND,

    // WPD_CATEGORY_OBJECT_MANAGEMENT
    &WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS,
    &WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY,
    &WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_AND_DATA,
    &WPD_COMMAND_OBJECT_MANAGEMENT_WRITE_OBJECT_DATA,
    &WPD_COMMAND_OBJECT_MANAGEMENT_COMMIT_OBJECT,
    &WPD_COMMAND_OBJECT_MANAGEMENT_REVERT_OBJECT,
    &WPD_COMMAND_OBJECT_MANAGEMENT_MOVE_OBJECTS,
    &WPD_COMMAND_OBJECT_MANAGEMENT_COPY_OBJECTS,
    &WPD_COMMAND_OBJECT_MANAGEMENT_UPDATE_OBJECT_WITH_PROPERTIES_AND_DATA,

    // WPD_CATEGORY_OBJECT_PROPERTIES
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL,
    &WPD_COMMAND_OBJECT_PROPERTIES_SET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES,
    &WPD_COMMAND_OBJECT_PROPERTIES_DELETE,

    // WPD_CATEGORY_OBJECT_PROPERTIES_BULK
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_END,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_FORMAT_END,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_START,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_NEXT,
    &WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_END,

    // WPD_CATEGORY_OBJECT_RESOURCES
    &WPD_COMMAND_OBJECT_RESOURCES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES,
    &WPD_COMMAND_OBJECT_RESOURCES_OPEN,
    &WPD_COMMAND_OBJECT_RESOURCES_READ,
    &WPD_COMMAND_OBJECT_RESOURCES_WRITE,
    &WPD_COMMAND_OBJECT_RESOURCES_CLOSE,
    &WPD_COMMAND_OBJECT_RESOURCES_DELETE,
    &WPD_COMMAND_OBJECT_RESOURCES_SEEK,

    // WPD_CATEGORY_CAPABILITIES
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_COMMANDS,
    &WPD_COMMAND_CAPABILITIES_GET_COMMAND_OPTIONS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FUNCTIONAL_CATEGORIES,
    &WPD_COMMAND_CAPABILITIES_GET_FUNCTIONAL_OBJECTS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_CONTENT_TYPES,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMATS,
    &WPD_COMMAND_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES,
    &WPD_COMMAND_CAPABILITIES_GET_FIXED_PROPERTY_ATTRIBUTES,

    // WPD_CATEGORY_STORAGE
    &WPD_COMMAND_STORAGE_FORMAT,

    // WPD_CATEGORY_NETWORK_CONFIGURATION
    &WPD_COMMAND_PROCESS_WIRELESS_PROFILE,
};

const GUID* g_SupportedFunctionalCategories[] =
{
    &WPD_FUNCTIONAL_CATEGORY_DEVICE,
    &WPD_FUNCTIONAL_CATEGORY_STORAGE,
    &WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION,
    &WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION,
};

const GUID* g_SupportedEvents[] =
{
    &WPD_EVENT_OBJECT_ADDED,
    &WPD_EVENT_OBJECT_REMOVED,
    &WPD_EVENT_OBJECT_UPDATED,
};

typedef struct _FunctionalCategoryContentTypePair
{
    const GUID* FunctionalCategory;
    const GUID* ContentType;
} FunctionalCategoryContentTypePair;

const FunctionalCategoryContentTypePair g_CategoryContentTypePairs[] =
{
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_UNSPECIFIED},
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_FOLDER},
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_AUDIO},
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_VIDEO},
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_IMAGE},
    {&WPD_FUNCTIONAL_CATEGORY_STORAGE, &WPD_CONTENT_TYPE_CONTACT},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_CONTENT_TYPE_NETWORK_ASSOCIATION},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_CONTENT_TYPE_WIRELESS_PROFILE},
};

typedef struct _ContentTypeFormatPair
{
    const GUID* ContentType;
    const GUID* Format;
} ContentTypeFormatPair;

const ContentTypeFormatPair g_ContentTypeFormatPairs[] =
{
    {&WPD_CONTENT_TYPE_UNSPECIFIED, &WPD_OBJECT_FORMAT_UNSPECIFIED},
    {&WPD_CONTENT_TYPE_UNSPECIFIED, &FakeContent_Format},
    {&WPD_CONTENT_TYPE_FOLDER,      &FakeContent_Format},
    {&WPD_CONTENT_TYPE_AUDIO,       &WPD_OBJECT_FORMAT_WMA},
    {&WPD_CONTENT_TYPE_VIDEO,       &WPD_OBJECT_FORMAT_WMV},
    {&WPD_CONTENT_TYPE_IMAGE,       &WPD_OBJECT_FORMAT_EXIF},
    {&WPD_CONTENT_TYPE_CONTACT,     &WPD_OBJECT_FORMAT_VCARD2},
    {&WPD_CONTENT_TYPE_NETWORK_ASSOCIATION, &WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION},
    {&WPD_CONTENT_TYPE_WIRELESS_PROFILE,    &WPD_OBJECT_FORMAT_MICROSOFT_WFC},
};

WpdCapabilities::WpdCapabilities()
{

}

WpdCapabilities::~WpdCapabilities()
{

}

HRESULT WpdCapabilities::Initialize(_In_ FakeDevice *pFakeDevice)
{

    HRESULT hr = S_OK;

    if(pFakeDevice == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    m_pFakeDevice = pFakeDevice;
    return hr;
}


HRESULT WpdCapabilities::DispatchWpdMessage(_In_    REFPROPERTYKEY          Command,
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
 *  - Return all commands supported by this driver should be returned as an
 *    IPortableDeviceKeyCollection in WPD_PROPERTY_CAPABILITIES_SUPPORTED_COMMANDS.
 *    That includes custom commands, if any.
 *    Note that certain commands require a "command target"
 *    to function correctly (e.g. delete object), and it is understood that not all objects
 *    are necessarily valid targets (e.g. you cannot delete the device object).
 */
HRESULT WpdCapabilities::OnGetSupportedCommands(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;

    CComPtr<IPortableDeviceKeyCollection> pCommands;
    UNREFERENCED_PARAMETER(pParams);

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pCommands);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    if (hr == S_OK)
    {
        // Add the supported commands
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedCommands); dwIndex++)
        {
            hr = pCommands->Add(*g_SupportedCommands[dwIndex]);
            CHECK_HR(hr, "Failed to add supported command at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
    }

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
    HRESULT     hr              = S_OK;
    PROPERTYKEY Command         = WPD_PROPERTY_NULL;

    CComPtr<IPortableDeviceValues> pOptions;

    if (hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_CAPABILITIES_COMMAND, &Command);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_COMMAND");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pOptions);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        //  Check for command options
        if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS, Command))
        {
            // This driver does not support recursive deletion
            hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED, FALSE);
            CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED");
        }
        else if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_RESOURCES_SEEK, Command))
        {
            // This driver supports Seek on resources opened for Read access
            hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_RESOURCES_SEEK_ON_READ_SUPPORTED, TRUE);
            CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_RESOURCES_SEEK_ON_READ_SUPPORTED");

            if (hr == S_OK)
            {
                // This driver does not support Seek on resources opened for WRITE access
                hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_RESOURCES_SEEK_ON_WRITE_SUPPORTED, FALSE);
                CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_RESOURCES_SEEK_ON_WRITE_SUPPORTED");
            }
        }
        else if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_RESOURCES_READ, Command))
        {
            // For better read performance, tell the API not to provide the input buffer parameter
            // when issuing a WPD_COMMAND_OBJECT_RESOURCES_READ command.
            hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_RESOURCES_NO_INPUT_BUFFER_ON_READ, TRUE);
            CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_RESOURCES_NO_INPUT_BUFFER_ON_READ");
        }
    }

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

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFunctionalCategories);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        // Add the supported functional categories
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedFunctionalCategories); dwIndex++)
        {
            PROPVARIANT pv = {0};

            PropVariantInit(&pv);

            // Don't call PropVariantClear, since we did not allocate the memory for this GUID
            pv.vt    = VT_CLSID;
            pv.puuid = (GUID*) g_SupportedFunctionalCategories[dwIndex];

            hr = pFunctionalCategories->Add(&pv);
            CHECK_HR(hr, "Failed to add supported functional category at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
    }

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
    HRESULT     hr                      = S_OK;
    GUID        guidFunctionalCategory  = GUID_NULL;

    CComPtr<IPortableDevicePropVariantCollection> pFunctionalObjects;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY, &guidFunctionalCategory);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY");
    }


    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFunctionalObjects);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        PROPVARIANT pv = {0};

        PropVariantInit(&pv);


        if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_STORAGE) ||
            (guidFunctionalCategory == WPD_FUNCTIONAL_CATEGORY_ALL))
        {
            pv.vt       = VT_LPWSTR;
            pv.pwszVal  = STORAGE1_OBJECT_ID;
            hr = pFunctionalObjects->Add(&pv);
            CHECK_HR(hr, "Failed to add %ws object ID", STORAGE1_OBJECT_ID);

            if (hr == S_OK)
            {
                pv.pwszVal  = STORAGE2_OBJECT_ID;
                hr = pFunctionalObjects->Add(&pv);
                CHECK_HR(hr, "Failed to add %ws object ID", STORAGE2_OBJECT_ID);
            }
        }
        if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION) ||
            (guidFunctionalCategory == WPD_FUNCTIONAL_CATEGORY_ALL))
        {
            pv.vt       = VT_LPWSTR;
            pv.pwszVal  = RENDERING_INFORMATION_OBJECT_ID;
            hr = pFunctionalObjects->Add(&pv);
            CHECK_HR(hr, "Failed to add %ws object ID", RENDERING_INFORMATION_OBJECT_ID);
        }
        if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_DEVICE) ||
            (guidFunctionalCategory == WPD_FUNCTIONAL_CATEGORY_ALL))
        {
            pv.vt       = VT_LPWSTR;
            pv.pwszVal  = WPD_DEVICE_OBJECT_ID;
            hr = pFunctionalObjects->Add(&pv);
            CHECK_HR(hr, "Failed to add %ws object ID", WPD_DEVICE_OBJECT_ID);
        }
        if ((guidFunctionalCategory  == WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION) ||
            (guidFunctionalCategory == WPD_FUNCTIONAL_CATEGORY_ALL))
        {
            pv.vt       = VT_LPWSTR;
            pv.pwszVal  = NETWORK_CONFIG_OBJECT_ID;
            hr = pFunctionalObjects->Add(&pv);
            CHECK_HR(hr, "Failed to add %ws object ID", NETWORK_CONFIG_OBJECT_ID);
        }
    }

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
    HRESULT     hr                      = S_OK;
    GUID        guidFunctionalCategory  = GUID_NULL;

    CComPtr<IPortableDevicePropVariantCollection> pContentTypes;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY, &guidFunctionalCategory);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FUNCTIONAL_CATEGORY");
    }


    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pContentTypes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        PROPVARIANT pv = {0};

        PropVariantInit(&pv);

        for(DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_CategoryContentTypePairs); dwIndex++)
        {
            pv.vt       = VT_CLSID;
            if ((*g_CategoryContentTypePairs[dwIndex].FunctionalCategory == guidFunctionalCategory) ||
                (guidFunctionalCategory == WPD_FUNCTIONAL_CATEGORY_ALL))
            {
                pv.puuid = (CLSID*)g_CategoryContentTypePairs[dwIndex].ContentType;
                hr       = pContentTypes->Add(&pv);
                CHECK_HR(hr, "Failed to add content type");
            }
            // Don't clear the PropVariant since we don't own the memory for the GUIDs
        }
    }

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
    HRESULT     hr               = S_OK;
    GUID        guidContentType  = GUID_NULL;

    CComPtr<IPortableDevicePropVariantCollection> pFormats;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_CONTENT_TYPE, &guidContentType);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_CONTENT_TYPE");
    }


    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pFormats);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        PROPVARIANT pv = {0};

        PropVariantInit(&pv);

        for(DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_ContentTypeFormatPairs); dwIndex++)
        {
            pv.vt       = VT_CLSID;
            if ((*g_ContentTypeFormatPairs[dwIndex].ContentType == guidContentType) ||
                (guidContentType == WPD_CONTENT_TYPE_ALL))
            {
                // Don't add duplicates.  Some formats appear under more than one content type
                if(!ExistsInCollection(*g_ContentTypeFormatPairs[dwIndex].Format, pFormats))
                {
                    pv.puuid = (CLSID*)g_ContentTypeFormatPairs[dwIndex].Format;
                    hr       = pFormats->Add(&pv);
                    CHECK_HR(hr, "Failed to add Format");
                }
            }
            // Don't clear the PropVariant since we don't own the memory for the GUIDs
        }
    }

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
    HRESULT     hr                = S_OK;
    GUID        guidObjectFormat  = GUID_NULL;

    CComPtr<IPortableDeviceKeyCollection> pPropertyKeys;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FORMAT, &guidObjectFormat);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FORMAT");
    }

    if (hr == S_OK)
    {
        hr = AddSupportedProperties(guidObjectFormat, &pPropertyKeys);
        CHECK_HR(hr, "Failed to add supported properties for %ws", (LPWSTR)CComBSTR(guidObjectFormat));
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDeviceKeyCollectionValue(WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS, pPropertyKeys);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS");
    }

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_CAPABILITIES_GET_FIXED_PROPERTY_ATTRIBUTES
 *  command.  This message is sent when the client needs to know the the property attributes that
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
    HRESULT     hr                = S_OK;
    GUID        guidObjectFormat  = GUID_NULL;
    PROPERTYKEY key               = WPD_PROPERTY_NULL;

    CComPtr<IPortableDeviceValues> pAttributes;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_FORMAT, &guidObjectFormat);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_FORMAT");
    }

    if(hr == S_OK)
    {
        hr = pParams->GetKeyValue(WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS, &key);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_PROPERTY_KEYS");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = AddFixedPropertyAttributes(guidObjectFormat, key, pAttributes);
        CHECK_HR(hr, "Failed to add fixed property attributes for format %ws and key %ws.%d", CComBSTR(guidObjectFormat), CComBSTR(key.fmtid), key.pid);
    }

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
    HRESULT hr  = S_OK;

    CComPtr<IPortableDevicePropVariantCollection> pEvents;
    UNREFERENCED_PARAMETER(pParams);

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pEvents);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (hr == S_OK)
    {
        PROPVARIANT pv = {0};
        PropVariantInit(&pv);

        pv.vt = VT_CLSID;
        // Add the supported events
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedEvents); dwIndex++)
        {
            pv.puuid = (CLSID*) g_SupportedEvents[dwIndex];
            hr = pEvents->Add(&pv);
            CHECK_HR(hr, "Failed to add supported events at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
    }

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
    HRESULT hr      = S_OK;
    GUID    Event   = GUID_NULL;

    CComPtr<IPortableDeviceValues> pOptions;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_CAPABILITIES_EVENT, &Event);
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_CAPABILITIES_EVENT");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pOptions);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        //  Check for the events we support
        if ((Event == WPD_EVENT_OBJECT_ADDED) ||
            (Event == WPD_EVENT_OBJECT_REMOVED) ||
            (Event == WPD_EVENT_OBJECT_UPDATED))
        {
            // These events are boradcast events
            hr = pOptions->SetBoolValue(WPD_EVENT_OPTION_IS_BROADCAST_EVENT, TRUE);
            CHECK_HR(hr, "Failed to set WPD_EVENT_OPTION_IS_BROADCAST_EVENT");
        }

    }

    if (hr == S_OK)
    {
        hr = pResults->SetIUnknownValue(WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS, pOptions);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_CAPABILITIES_EVENT_OPTIONS");
    }

    return hr;
}


