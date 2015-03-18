#include "stdafx.h"

#include "WpdObjectEnum.tmh"

WpdObjectEnumerator::WpdObjectEnumerator() : m_pDevice(NULL)
{

}

WpdObjectEnumerator::~WpdObjectEnumerator()
{

}

HRESULT WpdObjectEnumerator::Initialize(_In_ FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pDevice = pDevice;
    return S_OK;
}

HRESULT WpdObjectEnumerator::DispatchWpdMessage(
    _In_     REFPROPERTYKEY         Command,
    _In_     IPortableDeviceValues* pParams,
    _In_     IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_OBJECT_ENUMERATION)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
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
HRESULT WpdObjectEnumerator::OnStartFind(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT      hr              = S_OK;
    LPWSTR       wszParentID     = NULL;
    ContextMap*  pContextMap     = NULL;
    CAtlStringW  strEnumContext;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the object identifier of the parent where the enumeration is starting from.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID, &wszParentID);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID");
    }

    // Get the client context map so we can store an enumeration context for this enumeration
    // operation.
    if (SUCCEEDED(hr))
    {
        hr = GetClientContextMap(pParams, &pContextMap);
        CHECK_HR(hr, "Failed to get client context map");
    }

    // Create and initialize a new enumeration context.
    // Add the new enumertion context to the client context map.  This context is used to
    // keep track of this particular enumeration operation.
    if (SUCCEEDED(hr))
    {
        WpdObjectEnumeratorContext* pEnumeratorContext = new WpdObjectEnumeratorContext();

        if (pEnumeratorContext != NULL)
        {
            ACCESS_SCOPE Scope = m_pDevice->GetAccessScope(pParams);
            m_pDevice->InitializeEnumerationContext(Scope, wszParentID, pEnumeratorContext);

            // Add the enumeration context to the client context map.  This calls AddRef() on pEnumeratorContext
            hr = pContextMap->Add(pEnumeratorContext, strEnumContext);
            CHECK_HR(hr, "Failed to add the enumeration context");
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate enumeration context");
        }

        SAFE_RELEASE(pEnumeratorContext);
    }

    // Set the WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT value in the results.
    // This context identifier will be passed back during OnFindNext and OnEndFind to allow the driver to access it.
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, strEnumContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszParentID);

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT WpdObjectEnumerator::OnFindNext(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT                     hr                     = S_OK;
    LPWSTR                      wszEnumContext         = NULL;
    DWORD                       dwNumObjectsRequested  = 0;
    DWORD                       dwNumObjectsEnumerated = 0;
    WpdObjectEnumeratorContext* pEnumeratorContext     = NULL;

    CComPtr<IPortableDevicePropVariantCollection> pObjectIDCollection;

    // First get ALL parameters for this command.  If we cannot get ALL parameters
    // then E_INVALIDARG should be returned and no further processing should occur.

    // Get the enumeration context identifier for this enumeration operation.
    // The enumeration context identifier is needed to lookup the specific
    // enumeration context in the client context map for this enumeration operation.
    // NOTE that more than one enumeration may be in progress.
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, &wszEnumContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Get the number of objects requested for this enumeration call.
    // The driver should always attempt to meet this requested value.
    // If there are fewer children than requested, the driver should return the remaining
    // children and a return code of S_FALSE.
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED, &dwNumObjectsRequested);
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED");
        }
    }

    // Get the enumeration context for this enumeration operation.
    if (SUCCEEDED(hr))
    {
        hr = GetClientContext(pParams, wszEnumContext, (IUnknown**)&pEnumeratorContext);
        CHECK_HR(hr, "Failed to get the enumeration context");
    }

    // CoCreate a collection to store the object identifiers being returned for this enumeration call.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pObjectIDCollection);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pDevice->FindNext(dwNumObjectsRequested, pEnumeratorContext, pObjectIDCollection, &dwNumObjectsEnumerated);
        CHECK_HR(hr, "Failed to get the next object");
    }

    // Set the collection of object identifiers enumerated in the results
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS, pObjectIDCollection);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS");
    }

    // If the enumeration context reports that their are no more objects to return then return S_FALSE indicating to the
    // caller that we are finished.
    if (SUCCEEDED(hr))
    {
        // Update the number of children we have enumerated and returned to the caller
        pEnumeratorContext->m_ChildrenEnumerated += dwNumObjectsEnumerated;

        // Check the number requested against the number enumerated and set the HRESULT
        // accordingly.
        if (dwNumObjectsEnumerated < dwNumObjectsRequested)
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

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszEnumContext);

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
HRESULT WpdObjectEnumerator::OnEndFind(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
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
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Get the client context map so we can retrieve the enumeration context for this enumeration
    // operation using the WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT property value obtained above.
    if (SUCCEEDED(hr))
    {
        hr = GetClientContextMap(pParams, &pContextMap);
        CHECK_HR(hr, "Failed to get the client context map");
    }

    // Destroy any data allocated/associated with the enumeration context and then remove it from the context map.
    // We no longer need to keep this context around because the enumeration has been ended.
    if (SUCCEEDED(hr))
    {
        pContextMap->Remove(wszEnumContext);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(wszEnumContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

