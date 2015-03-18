#include "stdafx.h"

#include "WpdObjectEnum.tmh"

WpdObjectEnumerator::WpdObjectEnumerator()
{

}

WpdObjectEnumerator::~WpdObjectEnumerator()
{

}

HRESULT WpdObjectEnumerator::Initialize(_In_ WpdBaseDriver* pBaseDriver)
{
    m_pBaseDriver = pBaseDriver;
    return S_OK;
}

HRESULT WpdObjectEnumerator::DispatchWpdMessage(_In_ REFPROPERTYKEY         Command,
                                                _In_ IPortableDeviceValues* pParams,
                                                _In_ IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if (Command.fmtid != WPD_CATEGORY_OBJECT_ENUMERATION)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
    }

    if (hr == S_OK)
    {
        if (Command.pid == WPD_COMMAND_OBJECT_ENUMERATION_START_FIND.pid)
        {
            hr = OnStartFind(pParams, pResults);
            CHECK_HR(hr, "Failed to begin enumeration");
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT.pid)
        {
            hr = OnFindNext(pParams, pResults);
            if(FAILED(hr))
            {
                CHECK_HR(hr, "Failed to find next object");
            }
        }
        else if (Command.pid == WPD_COMMAND_OBJECT_ENUMERATION_END_FIND.pid)
        {
            hr = OnEndFind(pParams, pResults);
            CHECK_HR(hr, "Failed to end enumeration");
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
 *  This method is called when we receive a WPD_COMMAND_OBJECT_ENUMERATION_START_FIND
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID: the parent where we should start
 *      the enumeration.
 *  - WPD_PROPERTY_OBJECT_ENUMERATION_FILTER: the filter to use when doing
 *      enumeration.  Since this parameter is optional, it may not exist.
 *      This driver currently ignores the filter parameter.
 *
 *  The driver should:
 *  - Create a new context for this enumeration.
 *  - Set the string identifier in WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT for the newly created enumeration context.
 *    This value will be passed back during OnFindNext and OnEndFind.
 */
HRESULT WpdObjectEnumerator::OnStartFind(_In_ IPortableDeviceValues*  pParams,
                                         _In_ IPortableDeviceValues*  pResults)
{
    HRESULT      hr             = S_OK;
    LPWSTR       wszParentID    = NULL;
    ContextMap*  pContextMap    = NULL;
    CAtlStringW  strEnumContext;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier of the parent where the enumeration is starting from.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID, &wszParentID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID");
    }

    // Get the client context map so we can store an enumeration context for this enumeration
    // operation.
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Create and initialize a new enumeration context.
    // Add the new enumertion context to the client context map.  This context is used to
    // keep track of this particular enumeration operation.
    if (hr == S_OK)
    {
        WpdObjectEnumeratorContext* pEnumeratorContext = new WpdObjectEnumeratorContext();
        if (pEnumeratorContext != NULL)
        {
            // Initialize the enumeration context
            InitializeEnumerationContext(pEnumeratorContext, wszParentID);

            // Add the enumeration context to the client context map.
            pContextMap->Add(pEnumeratorContext, strEnumContext);

            // Release the context because Add AddRef's it
            SAFE_RELEASE(pEnumeratorContext);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate enumeration context");
        }
    }

    // Set the WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT value in the results.
    // This context identifier will be passed back during OnFindNext and OnEndFind to allow the driver to access it.
    if (hr == S_OK)
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, strEnumContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Free the memory. 
    CoTaskMemFree(wszParentID);
    wszParentID = NULL;

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT WpdObjectEnumerator::OnFindNext(_In_ IPortableDeviceValues*  pParams,
                                        _In_ IPortableDeviceValues*  pResults)
{
    HRESULT                     hr                    = S_OK;
    LPWSTR                      wszEnumContext        = NULL;
    DWORD                       dwNumObjectsRequested = 0;
    ContextMap*                 pContextMap           = NULL;
    WpdObjectEnumeratorContext* pEnumeratorContext    = NULL;
    DWORD                       NumObjectsEnumerated  = 0;

    CComPtr<IPortableDevicePropVariantCollection> pObjectIDCollection;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the enumeration context identifier for this enumeration operation.
    // The enumeration context identifier is needed to lookup the specific
    // enumeration context in the client context map for this enumeration operation.
    // NOTE that more than one enumeration may be in progress.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, &wszEnumContext);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Get the number of objects requested for this enumeration call.
    // The driver should always attempt to meet this requested value.
    // If there are fewer children than requested, the driver should return the remaining
    // children and a return code of S_FALSE.
    hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED, &dwNumObjectsRequested);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED");
    }

    // Get the client context map so we can retrieve the enumeration context for this enumeration
    // operation.
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (hr == S_OK)
    {
        pEnumeratorContext = (WpdObjectEnumeratorContext*)pContextMap->GetContext(wszEnumContext);
        if (pEnumeratorContext == NULL)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing enumeration context");
        }
    }

    // CoCreate a collection to store the object identifiers being returned for this enumeration call.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pObjectIDCollection);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // If the enumeration context reports that their are more objects to return, then continue, if not,
    // return an empty results set.
    if ((hr == S_OK) && (pEnumeratorContext != NULL) && (pEnumeratorContext->HasMoreChildrenToEnumerate() == TRUE))
    {
        if (pEnumeratorContext->m_strParentObjectID.CompareNoCase(L"") == 0)
        {
            // We are being asked for the WPD_DEVICE_OBJECT_ID
            hr = AddStringValueToPropVariantCollection(pObjectIDCollection, WPD_DEVICE_OBJECT_ID);
            CHECK_HR(hr, "Failed to add 'DEVICE' object ID to enumeration collection");

            // Update the the number of children we are returning for this enumeration call
            NumObjectsEnumerated++;
        }
        else if (pEnumeratorContext->m_strParentObjectID.CompareNoCase(WPD_DEVICE_OBJECT_ID) == 0)
        {
    
            // We are being asked for direct children of the WPD_DEVICE_OBJECT_ID
            switch (m_pBaseDriver->m_SensorType)
            {
                case WpdBaseDriver::UNKNOWN:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::COMPASS:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, COMPASS_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::SENSIRON:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, TEMP_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::FLEX:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, FLEX_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::PING:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, PING_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::PIR:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, PIR_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::MEMSIC:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, MEMSIC_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::QTI:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, QTI_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::PIEZO:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, PIEZO_SENSOR_OBJECT_ID);
                    break;
                case WpdBaseDriver::HITACHI:
                    hr = AddStringValueToPropVariantCollection(pObjectIDCollection, HITACHI_SENSOR_OBJECT_ID);
                    break;
                default:
                    break;
            }
            CHECK_HR(hr, "Failed to add storage object ID to enumeration collection");

            // Update the the number of children we are returning for this enumeration call
            NumObjectsEnumerated++;

        }
    }

    // Set the collection of object identifiers enumerated in the results
    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS, pObjectIDCollection);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS");
    }

    // If the enumeration context reports that their are no more objects to return then return S_FALSE indicating to the
    // caller that we are finished.
    if (hr == S_OK)
    {
        if (pEnumeratorContext != NULL)
        {
            // Update the number of children we have enumerated and returned to the caller
            pEnumeratorContext->m_ChildrenEnumerated += NumObjectsEnumerated;

            // Check the number requested against the number enumerated and set the HRESULT
            // accordingly.
            if (NumObjectsEnumerated < dwNumObjectsRequested)
            {
                // We returned less than the number of objects requested to the caller
                hr = S_FALSE;
            }
            else
            {
                // We returned exactly the number of objects requested to the caller
                hr = S_OK;
            }
        }
    }

    // Free the memory.
    CoTaskMemFree(wszEnumContext);
    wszEnumContext = NULL;

    SAFE_RELEASE(pContextMap);
    SAFE_RELEASE(pEnumeratorContext);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_OBJECT_ENUMERATION_END_FIND
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT: the context the driver returned to
 *      the client in OnStartFind.
 *
 *  The driver should:
 *  - Destroy any data associated with this context.
 */
HRESULT WpdObjectEnumerator::OnEndFind(_In_ IPortableDeviceValues*  pParams,
                                       _In_ IPortableDeviceValues*  pResults)
{
    HRESULT     hr              = S_OK;
    LPWSTR      wszEnumContext  = NULL;
    ContextMap* pContextMap     = NULL;

    UNREFERENCED_PARAMETER(pResults);

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the enumeration context identifier for this enumeration operation.  We will
    // need this to lookup the specific enumeration context in the client context map.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, &wszEnumContext);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Get the client context map so we can retrieve the enumeration context for this enumeration
    // operation using the WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT property value obtained above.
    if (hr == S_OK)
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    // Destroy any data allocated/associated with the enumeration context and then remove it from the context map.
    // We no longer need to keep this context around because the enumeration has been ended.
    if (hr == S_OK)
    {
        pContextMap->Remove(wszEnumContext);
    }

    // Free the memory.
    CoTaskMemFree(wszEnumContext);
    wszEnumContext = NULL;

    SAFE_RELEASE(pContextMap);

    return hr;
}

// Initialize the enumeration context
VOID WpdObjectEnumerator::InitializeEnumerationContext(
    _In_ WpdObjectEnumeratorContext* pEnumeratorContext,
    _In_ LPCWSTR                     wszParentObjectID)
{
    if (pEnumeratorContext == NULL)
    {
        return;
    }

    // Initialize the enumeration context with the parent object identifier
    pEnumeratorContext->m_strParentObjectID = wszParentObjectID;

    // Our sample driver has a very simple object structure where we know
    // how many children are under each parent.
    // The eumeration context is initialized below with this information.
    if (pEnumeratorContext->m_strParentObjectID.CompareNoCase(L"") == 0)
    {
        // Clients passing an 'empty' string for the parent are asking for the
        // 'DEVICE' object.  We should return 1 child in this case.
        pEnumeratorContext->m_TotalChildren = 1;
    }
    else if (pEnumeratorContext->m_strParentObjectID.CompareNoCase(WPD_DEVICE_OBJECT_ID) == 0)
    {
        // The device object contains 1 child (the storage object).
        pEnumeratorContext->m_TotalChildren = 1;
    }
    // If the sensor objects have children, add them here...
    else 
    {
        // The sensor object contains 0 children.
        pEnumeratorContext->m_TotalChildren = 0;
    }
}

HRESULT WpdObjectEnumerator::AddStringValueToPropVariantCollection(
    _In_ IPortableDevicePropVariantCollection* pCollection,
    _In_ LPCWSTR                               wszValue)
{
    HRESULT hr = S_OK;

    if ((pCollection == NULL) ||
        (wszValue    == NULL))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    PROPVARIANT pv = {0};
    PropVariantInit(&pv);

    pv.vt      = VT_LPWSTR;
    pv.pwszVal = (LPWSTR)wszValue;

    // The wszValue will be copied into the collection, keeping the ownership
    // of the string belonging to the caller.
    // Don't call PropVariantClear, since we did not allocate the memory for these string values

    hr = pCollection->Add(&pv);

    return hr;
}
