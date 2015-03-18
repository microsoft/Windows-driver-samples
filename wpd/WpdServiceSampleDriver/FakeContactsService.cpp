#include "stdafx.h"

#include "FakeContactsService.tmh"

const FormatAttributeInfo g_SupportedContactFormats[] =
{
    {&FORMAT_AbstractContact,   L"AbstractContact"},
    {&FORMAT_VCard2Contact,     L"VCard2"}
};

const GUID* g_SupportedMethods[] =
{
    &METHOD_FullEnumSyncSvc_BeginSync,
    &METHOD_FullEnumSyncSvc_EndSync,
    &MyCustomMethod
};

// Method parameters
const MethodParameterAttributeInfo g_MethodParameters[] =
{
    {&MyCustomMethodResult,      VT_BOOL,    WPD_PARAMETER_USAGE_RETURN, WPD_PARAMETER_ATTRIBUTE_FORM_UNSPECIFIED,       0, L"Result"},
    {&MyCustomMethodParam,       VT_UI4,     WPD_PARAMETER_USAGE_IN,     WPD_PARAMETER_ATTRIBUTE_FORM_UNSPECIFIED,       1, L"Integer_Param"},
    {&MyCustomMethodParamInOut,  VT_LPWSTR,  WPD_PARAMETER_USAGE_INOUT,  WPD_PARAMETER_ATTRIBUTE_FORM_OBJECT_IDENTIFIER, 2, L"ObjectId_Param"},
};

const GUID* g_SupportedServiceEvents[] =
{
    &WPD_EVENT_OBJECT_ADDED,
    &WPD_EVENT_OBJECT_REMOVED,
    &WPD_EVENT_OBJECT_UPDATED,
    &MyCustomEvent,
};

// Event parameters
const EventParameterAttributeInfo g_ServiceEventParameters[] =
{
    {&WPD_EVENT_OBJECT_ADDED,   &WPD_EVENT_PARAMETER_EVENT_ID,                              VT_CLSID},
    {&WPD_EVENT_OBJECT_ADDED,   &WPD_OBJECT_PERSISTENT_UNIQUE_ID,                           VT_LPWSTR},
    {&WPD_EVENT_OBJECT_ADDED,   &WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID,    VT_LPWSTR},
    {&WPD_EVENT_OBJECT_ADDED,   &WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,                 VT_LPWSTR},

    {&WPD_EVENT_OBJECT_REMOVED, &WPD_EVENT_PARAMETER_EVENT_ID,                              VT_CLSID},
    {&WPD_EVENT_OBJECT_REMOVED, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,                           VT_LPWSTR},
    {&WPD_EVENT_OBJECT_REMOVED, &WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID,    VT_LPWSTR},
    {&WPD_EVENT_OBJECT_REMOVED, &WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,                 VT_LPWSTR},

    {&WPD_EVENT_OBJECT_UPDATED, &WPD_EVENT_PARAMETER_EVENT_ID,                              VT_CLSID},
    {&WPD_EVENT_OBJECT_UPDATED, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,                           VT_LPWSTR},
    {&WPD_EVENT_OBJECT_UPDATED, &WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID,    VT_LPWSTR},
    {&WPD_EVENT_OBJECT_UPDATED, &WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,                 VT_LPWSTR},

    {&MyCustomEvent,            &WPD_EVENT_PARAMETER_EVENT_ID,                              VT_CLSID},
    {&MyCustomEvent,            &WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,                 VT_LPWSTR},
    {&MyCustomEvent,            &MyCustomEventParam0,                                       VT_BOOL},
    {&MyCustomEvent,            &MyCustomEventParam1,                                       VT_UI4},
};

// Supported commands for this service
const PROPERTYKEY* g_ServiceSupportedCommands[] =
{
    // WPD_CATEGORY_OBJECT_ENUMERATION
    &WPD_COMMAND_OBJECT_ENUMERATION_START_FIND,
    &WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT,
    &WPD_COMMAND_OBJECT_ENUMERATION_END_FIND,

    // WPD_CATEGORY_OBJECT_PROPERTIES
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL,
    &WPD_COMMAND_OBJECT_PROPERTIES_SET,
    &WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES,
    &WPD_COMMAND_OBJECT_PROPERTIES_DELETE,

    // WPD_CATEGORY_OBJECT_RESOURCES
    &WPD_COMMAND_OBJECT_RESOURCES_GET_SUPPORTED,
    &WPD_COMMAND_OBJECT_RESOURCES_OPEN,
    &WPD_COMMAND_OBJECT_RESOURCES_READ,
    &WPD_COMMAND_OBJECT_RESOURCES_CLOSE,
    &WPD_COMMAND_OBJECT_RESOURCES_GET_ATTRIBUTES,

    // WPD_CATEGORY_OBJECT_MANAGEMENT
    &WPD_COMMAND_OBJECT_MANAGEMENT_CREATE_OBJECT_WITH_PROPERTIES_ONLY,
    &WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS,

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

    // WPD_CATEGORY_COMMON
    &WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS,

    // WPD_CATEGORY_SERVICE_COMMON
    &WPD_COMMAND_SERVICE_COMMON_GET_SERVICE_OBJECT_ID,

    // WPD_CATEGORY_SERVICE_CAPABILITIES
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_COMMANDS,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_COMMAND_OPTIONS,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS_BY_FORMAT,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_METHOD_PARAMETER_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMATS,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_FORMAT_PROPERTIES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_FORMAT_PROPERTY_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_EVENTS,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_EVENT_PARAMETER_ATTRIBUTES,
    &WPD_COMMAND_SERVICE_CAPABILITIES_GET_INHERITED_SERVICES
};


HRESULT FakeContactsService::GetSupportedCommands(
    _In_    IPortableDeviceKeyCollection* pCommands)
{
    HRESULT hr = S_OK;

    if(pCommands == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_ServiceSupportedCommands); dwIndex++)
    {
        PROPERTYKEY key = *(g_ServiceSupportedCommands[dwIndex]);
        hr = pCommands->Add(key);
        CHECK_HR(hr, "Failed to add supported command at index %d", dwIndex);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

HRESULT FakeContactsService::GetCommandOptions(
    _In_    REFPROPERTYKEY         Command,
    _In_    IPortableDeviceValues* pOptions)
{
    HRESULT hr = S_OK;

    if(pOptions == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    //  Check for command options
    if (IsEqualPropertyKey(WPD_COMMAND_OBJECT_MANAGEMENT_DELETE_OBJECTS, Command))
    {
        // This driver does not support recursive deletion
        hr = pOptions->SetBoolValue(WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED, TRUE);
        CHECK_HR(hr, "Failed to set WPD_OPTION_OBJECT_MANAGEMENT_RECURSIVE_DELETE_SUPPORTED");
    }

    return hr;
}


HRESULT FakeContactsService::GetSupportedMethods(
    _In_    IPortableDevicePropVariantCollection* pMethods)
{
    HRESULT hr = S_OK;

    if (pMethods == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Add the supported methods to the collection.
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedMethods); dwIndex++)
    {
        PROPVARIANT pv;
        pv.vt = VT_CLSID;
        pv.puuid = (GUID*)g_SupportedMethods[dwIndex]; // Assignment only, don't PropVariantClear this

        hr = pMethods->Add(&pv);
        CHECK_HR(hr, "Failed to add supported method at index %d", dwIndex);
        if (FAILED(hr))
        {
            break;
        }
    }

    return hr;
}

BOOL FakeContactsService::IsMethodSupported(
    _In_    REFGUID Method)
{
    // Add the supported methods to the collection.
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_SupportedMethods); dwIndex++)
    {
        if (Method == *g_SupportedMethods[dwIndex])
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT FakeContactsService::GetSupportedMethodsByFormat(
    _In_    REFGUID                               Format,
    _In_    IPortableDevicePropVariantCollection* pMethods)
{
    HRESULT hr = S_OK;

    if (pMethods == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    for (DWORD i=0; i<ARRAYSIZE(g_SupportedContactFormats); i++)
    {
        if (Format == *g_SupportedContactFormats[i].pFormatGuid)
        {
            // Add the supported methods for the format to the collection, right now there are none, so we
            // return an emtpy collection
            hr = S_OK;
            break;
        }
    }
    CHECK_HR(hr, "Format %ws is not supported", CComBSTR(Format));

    return hr;
}


HRESULT FakeContactsService::GetMethodAttributes(
    _In_    REFGUID                Method,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceKeyCollection> pParameters;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // CoCreate a collection for specifying the method parameters.
    hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDeviceKeyCollection,
                          (VOID**) &pParameters);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");

    // Add the method attributes to this collection
    if (Method == METHOD_FullEnumSyncSvc_BeginSync)
    {
        if (hr == S_OK)
        {
            hr = pAttributes->SetStringValue(WPD_METHOD_ATTRIBUTE_NAME, L"BeginSync");
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_NAME");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_METHOD_ATTRIBUTE_ACCESS, WPD_COMMAND_ACCESS_READWRITE);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ACCESS");
        }

        if (hr == S_OK)
        {
            // no parameters, set empty collection
            hr = pAttributes->SetIPortableDeviceKeyCollectionValue(WPD_METHOD_ATTRIBUTE_PARAMETERS, pParameters);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_PARAMETERS");
        }

        if (hr == S_OK)
        {
            // no associated format, set GUID_NULL
            hr = pAttributes->SetGuidValue(WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT, GUID_NULL);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT");
        }

    }
    else if (Method == METHOD_FullEnumSyncSvc_EndSync)
    {
        if (hr == S_OK)
        {
            hr = pAttributes->SetStringValue(WPD_METHOD_ATTRIBUTE_NAME, L"EndSync");
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_NAME");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_METHOD_ATTRIBUTE_ACCESS, WPD_COMMAND_ACCESS_READWRITE);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ACCESS");
        }

        if (hr == S_OK)
        {
            // no parameters, set empty collection
            hr = pAttributes->SetIPortableDeviceKeyCollectionValue(WPD_METHOD_ATTRIBUTE_PARAMETERS, pParameters);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_PARAMETERS");
        }

        if (hr == S_OK)
        {
            // no associated format, set GUID_NULL
            hr = pAttributes->SetGuidValue(WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT, GUID_NULL);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT");
        }
    }
    else if (Method == MyCustomMethod)
    {
        if (hr == S_OK)
        {
            hr = pAttributes->SetStringValue(WPD_METHOD_ATTRIBUTE_NAME, L"CustomMethod");
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_NAME");
        }

        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_METHOD_ATTRIBUTE_ACCESS, WPD_COMMAND_ACCESS_READWRITE);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ACCESS");
        }

        if (hr == S_OK)
        {
            // Set the supported parameters
            for (size_t i=0; i<ARRAYSIZE(g_MethodParameters); i++)
            {
                pParameters->Add(*(g_MethodParameters[i].pKey));
            }

            hr = pAttributes->SetIPortableDeviceKeyCollectionValue(WPD_METHOD_ATTRIBUTE_PARAMETERS, pParameters);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_PARAMETERS");
        }

        if (hr == S_OK)
        {
            // no associated format, set GUID_NULL
            hr = pAttributes->SetGuidValue(WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT, GUID_NULL);
            CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_ASSOCIATED_FORMAT");
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Unknown method %ws received",CComBSTR(Method));
    }

    return hr;
}

HRESULT FakeContactsService::GetMethodParameterAttributes(
    _In_    REFPROPERTYKEY         Parameter,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = SetMethodParameterAttributes(Parameter, &g_MethodParameters[0], ARRAYSIZE(g_MethodParameters), pAttributes);
    CHECK_HR(hr, "Failed to set method parameter attributes");

    return hr;
}

HRESULT FakeContactsService::GetSupportedFormats(
    _In_    IPortableDevicePropVariantCollection* pFormats)
{
    HRESULT hr = S_OK;

    if (pFormats == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (hr == S_OK)
    {
        // Add the supported formats to this collection
        for (DWORD i=0; i<ARRAYSIZE(g_SupportedContactFormats); i++)
        {
            PROPVARIANT pv = {0};
            pv.vt = VT_CLSID;
            pv.puuid = (CLSID*)g_SupportedContactFormats[i].pFormatGuid; // assignment, do not call PropVariantClear

            hr = pFormats->Add(&pv);
            CHECK_HR(hr, "Failed to add format to IPortableDevicePropVariantCollection");
        }
    }

    return hr;
}

HRESULT FakeContactsService::GetFormatAttributes(
    _In_    REFGUID                Format,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    // Add the supported formats to this collection
    for (DWORD i=0; i<ARRAYSIZE(g_SupportedContactFormats); i++)
    {
        if (Format == *g_SupportedContactFormats[i].pFormatGuid)
        {
            hr = pAttributes->SetStringValue(WPD_FORMAT_ATTRIBUTE_NAME, g_SupportedContactFormats[i].wszName);
            CHECK_HR(hr, "Failed to set WPD_FORMAT_ATTRIBUTE_NAME");
            break;
        }
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
    {
        CHECK_HR(hr, "Unknown format %ws received",CComBSTR(Format));
    }

    return hr;
}

HRESULT FakeContactsService::GetSupportedFormatProperties(
    _In_    REFGUID                       Format,
    _In_    IPortableDeviceKeyCollection* pKeys)
{
    HRESULT hr = S_OK;

    if (pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Add the supported format properties to this collection
    // The formats of this service happen to support the same set of properties
    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    for (DWORD i=0; i<ARRAYSIZE(g_SupportedContactFormats); i++)
    {
        if (Format == (*g_SupportedContactFormats[i].pFormatGuid))
        {
            hr = GetSupportedContactProperties(pKeys);
            CHECK_HR(hr, "Failed to add supported contact format properties");
            break;
        }
    }
    CHECK_HR(hr, "Format %ws is not supported", CComBSTR(Format));

    return hr;
}

HRESULT FakeContactsService::GetPropertyAttributes(
    _In_    REFGUID                Format,
    _In_    REFPROPERTYKEY         Property,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    for (DWORD i=0; i<ARRAYSIZE(g_SupportedContactFormats); i++)
    {
        if (Format == (*g_SupportedContactFormats[i].pFormatGuid))
        {
            hr = GetContactPropertyAttributes(Property, pAttributes);
            CHECK_HR(hr, "Failed to get property attributes");
            break;
        }
    }

    CHECK_HR(hr, "Failed to find supported format to retrieve property attributes");

    return hr;
}

HRESULT FakeContactsService::GetSupportedEvents(
    _In_    IPortableDevicePropVariantCollection* pEvents)
{
    HRESULT hr = S_OK;

    if (pEvents == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    PROPVARIANT pv;
    pv.vt = VT_CLSID;

    for (DWORD i=0; i<ARRAYSIZE(g_SupportedServiceEvents); i++)
    {
        pv.puuid = (CLSID*)g_SupportedServiceEvents[i];  // Assignment, don't PropVariantClear this

        hr = pEvents->Add(&pv);
        CHECK_HR(hr, "Failed to add event to the collection");
    }

    return hr;
}

HRESULT FakeContactsService::GetEventAttributes(
    _In_    REFGUID                Event,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pEventOptions;
    CComPtr<IPortableDeviceKeyCollection> pEventParameters;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // CoCreate a collection to store the event options.
    hr = CoCreateInstance(CLSID_PortableDeviceValues,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDeviceValues,
                          (VOID**) &pEventOptions);
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    if (hr == S_OK)
    {
        hr = pEventOptions->SetBoolValue(WPD_EVENT_OPTION_IS_BROADCAST_EVENT, TRUE);
        CHECK_HR(hr, "Failed to set WPD_EVENT_OPTION_IS_BROADCAST_EVENT");
    }

    // Loop through the supported events for this service to find a match
    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    for (DWORD i=0; i<ARRAYSIZE(g_SupportedServiceEvents); i++)
    {
        if (Event == *g_SupportedServiceEvents[i])
        {
            // Set the event options.
            hr = pAttributes->SetIPortableDeviceValuesValue(WPD_EVENT_ATTRIBUTE_OPTIONS, pEventOptions);
            CHECK_HR(hr, "Failed to set WPD_EVENT_ATTRIBUTE_OPTIONS");

            // Set the event parameters.
            if (hr == S_OK)
            {
                hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IPortableDeviceKeyCollection,
                                      (VOID**) &pEventParameters);
                CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
            }

            if (hr == S_OK)
            {
                hr = SetEventParameters(Event, &g_ServiceEventParameters[0], ARRAYSIZE(g_ServiceEventParameters), pEventParameters);
                CHECK_HR(hr, "Failed to set event parameters");

                if (hr == S_OK)
                {
                    hr = pAttributes->SetIPortableDeviceKeyCollectionValue(WPD_EVENT_ATTRIBUTE_PARAMETERS, pEventParameters);
                    CHECK_HR(hr, "Failed to set WPD_METHOD_ATTRIBUTE_PARAMETERS");
                }
            }

            // Set a name for the custom event
            if (hr == S_OK)
            {
                if (Event == MyCustomEvent)
                {
                    hr = pAttributes->SetStringValue(WPD_EVENT_ATTRIBUTE_NAME , L"MyCustomEvent");
                    CHECK_HR(hr, "Failed to set WPD_EVENT_ATTRIBUTE_NAME");
                }
            }
            break;
        }
    }

    return hr;
}

HRESULT FakeContactsService::GetEventParameterAttributes(
    _In_    REFPROPERTYKEY         Parameter,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if (pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = SetEventParameterAttributes(Parameter, &g_ServiceEventParameters[0], ARRAYSIZE(g_ServiceEventParameters), pAttributes);
    CHECK_HR(hr, "Failed to set event parameter attributes");

    return hr;
}

HRESULT FakeContactsService::GetInheritedServices(
            const DWORD                           dwInheritanceType,
    _In_    IPortableDevicePropVariantCollection* pServices)
{
    HRESULT hr = S_OK;

    if (pServices == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (dwInheritanceType == WPD_SERVICE_INHERITANCE_IMPLEMENTATION)
    {
        PROPVARIANT pv;
        pv.vt = VT_CLSID;
        pv.puuid = (CLSID*)&SERVICE_FullEnumSync;  // Assignment, don't PropVariantClear this

        hr = pServices->Add(&pv);
        CHECK_HR(hr, "Failed to add service GUID to the collection");
    }

    return hr;
}

HRESULT FakeContactsService::OnBeginSync(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This is where the sync service receives a notification from the application that
    // sync is about to begin so that it can lock the session
    // This method does not do anything right now

    return S_OK;
}

HRESULT FakeContactsService::OnEndSync(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    UNREFERENCED_PARAMETER(pParams);
    UNREFERENCED_PARAMETER(pResults);

    // This is where the sync service receives a notification from the application that
    // sync is about to end so that it can unlock the session
    // This method does not do anything right now

    return S_OK;
}

// This demonstrates how a custom service method can be implemented
HRESULT FakeContactsService::OnMyCustomMethod(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults,
    _In_    IPortableDeviceValues*  pEventParams)
{
    HRESULT hr            = S_OK;
    DWORD   dwParamValue  = 0;
    BOOL    bResultValue  = FALSE;
    LPWSTR  pszParamValue = NULL;

    if (pParams == NULL || pResults == NULL || pEventParams == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = pParams->GetUnsignedIntegerValue(MyCustomMethodParam, &dwParamValue);
    CHECK_HR(hr, "Failed to get MyCustomMethodParam");

    if (hr == S_OK)
    {
        hr = pParams->GetStringValue(MyCustomMethodParamInOut, &pszParamValue);
        CHECK_HR(hr, "Failed to get MyCustomMethodParamInOut");
    }

    if (hr == S_OK)
    {
        // For demonstration purposes only, we simply return the inout parameter as is
        hr = pResults->SetStringValue(MyCustomMethodParamInOut, pszParamValue);
        CHECK_HR(hr, "Failed to set MyCustomMethodParamInOut");
    }

    if (hr == S_OK)
    {
        // This is where the device will process the method invocation
        // For demonstration purposes only, we return TRUE if the input is an even number
        bResultValue = (dwParamValue % 1 == 0)?TRUE:FALSE;
    }

    if (hr == S_OK)
    {
        hr = pResults->SetBoolValue(MyCustomMethodResult, bResultValue);
        CHECK_HR(hr, "Failed to set MyCustomMethodResult");
    }

    if (hr == S_OK)
    {
        hr = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, MyCustomEvent);
        CHECK_HR(hr, "Failed to add WPD_EVENT_PARAMETER_EVENT_ID");

        if (hr == S_OK)
        {
            // Adding this event parameter will allow WPD to scope this event to the container functional object
            hr = pEventParams->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, RequestFilename);
            CHECK_HR(hr, "Failed to add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID");
        }

        if (hr == S_OK)
        {
            // set the first custom event parameter
            hr = pEventParams->SetBoolValue(MyCustomEventParam0, bResultValue);
            CHECK_HR(hr, "Failed to add MyCustomEvent parameter 0");
        }

        if (hr == S_OK)
        {
            // set the second custom event parameter
            hr = pEventParams->SetUnsignedIntegerValue(MyCustomEventParam1, dwParamValue);
            CHECK_HR(hr, "Failed to add MyCustomEvent parameter 1");
        }
    }

    CoTaskMemFree(pszParamValue);
    return hr;
}
