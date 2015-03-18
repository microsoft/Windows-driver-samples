#include "stdafx.h"

#include "WpdBaseDriver.tmh"

WpdBaseDriver::WpdBaseDriver() :
    m_cRef(1)
{
}

WpdBaseDriver::~WpdBaseDriver()
{

}

ULONG __stdcall WpdBaseDriver::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

_At_(this, __drv_freesMem(Mem)) 
ULONG __stdcall WpdBaseDriver::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

HRESULT __stdcall WpdBaseDriver::QueryInterface(
    REFIID riid,
    void** ppv)
{
    HRESULT hr = S_OK;

    if(riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

/**
 * This method is called to initialize the driver object.
 * This is where the driver would set up it's I/O libraries
 * and so on.
 */
HRESULT WpdBaseDriver::Initialize()
{
    HRESULT hr = m_Device.InitializeContent();
    CHECK_HR(hr, ("Failed to initialize content"));

    if (hr == S_OK)
    {
        hr = m_Service.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdService"));
    }

    if (hr == S_OK)
    {
        hr = m_ObjectEnum.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdObjectEnum"));
    }

    if (hr == S_OK)
    {
        m_Capabilities.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdCapabilities"));
    }

    if (hr == S_OK)
    {
        m_ObjectManagement.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdObjectManagement"));
    }

    if (hr == S_OK)
    {
        m_ObjectProperties.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdObjectProperties"));
    }

    if (hr == S_OK)
    {
        m_ObjectResources.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdObjectResources"));
    }

    if (hr == S_OK)
    {
        m_ObjectPropertiesBulk.Initialize(&m_Device);
        CHECK_HR(hr, ("Failed to initialize WpdObjectPropertiesBulk"));
    }

    return hr;
}

/**
 * This method is called to uninitialize the driver object.
 * In a real driver, this is where the driver would clean up
 * any resources held by this driver.
 */
VOID WpdBaseDriver::Uninitialize()
{
}

HRESULT WpdBaseDriver::DispatchWpdMessage(_In_  IPortableDeviceValues* pParams,
                                          _In_  IPortableDeviceValues* pResults)
{

    HRESULT     hr                  = S_OK;
    GUID        guidCommandCategory = {0};
    DWORD       dwCommandID         = 0;
    PROPERTYKEY CommandKey          = WPD_PROPERTY_NULL;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, &guidCommandCategory);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_CATEGORY from input parameters");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, &dwCommandID);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_ID from input parameters");
    }

    // If WPD_PROPERTY_COMMON_COMMAND_CATEGORY or WPD_PROPERTY_COMMON_COMMAND_ID could not be extracted
    // properly then we should return E_INVALIDARG to the client.
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_CATEGORY or WPD_PROPERTY_COMMON_COMMAND_ID from input parameters");
    }

    if (hr == S_OK)
    {
        CommandKey.fmtid = guidCommandCategory;
        CommandKey.pid   = dwCommandID;

        if (CommandKey.fmtid == WPD_CATEGORY_OBJECT_ENUMERATION)
        {
            hr = m_ObjectEnum.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_OBJECT_PROPERTIES)
        {
            hr = m_ObjectProperties.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_OBJECT_RESOURCES)
        {
            hr = m_ObjectResources.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_CAPABILITIES)
        {
            hr = m_Capabilities.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_MANAGEMENT)
        {
            hr = m_ObjectManagement.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_PROPERTIES_BULK)
        {
            hr = m_ObjectPropertiesBulk.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (IsEqualPropertyKey(CommandKey, WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS))
        {
            hr = OnGetObjectIDsFromPersistentUniqueIDs(pParams, pResults);
        }
        else if(IsEqualPropertyKey(CommandKey, WPD_COMMAND_COMMON_SAVE_CLIENT_INFORMATION))
        {
            hr = OnSaveClientInfo(pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_SERVICE_COMMON        || 
                 CommandKey.fmtid == WPD_CATEGORY_SERVICE_METHODS       ||
                 CommandKey.fmtid == WPD_CATEGORY_SERVICE_CAPABILITIES)
        {
            hr = m_Service.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "Unknown command %ws.%d received",CComBSTR(CommandKey.fmtid), CommandKey.pid);
        }
    }

    HRESULT hrTemp = pResults->SetErrorValue(WPD_PROPERTY_COMMON_HRESULT, hr);
    CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_COMMON_HRESULT"));

    // Set to a success code, to indicate that the message was received.
    // the return code for the actual command's results is stored in the
    // WPD_PROPERTY_COMMON_HRESULT property.
    hr = S_OK;

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS: Contains an IPortableDevicePropVariantCollection of VT_LPWSTR,
 *    indicating the PersistentUniqueIDs.
 *
 *  The driver should:
 *  - Iterate through the PersistentUniqueIDs, and convert to a currently valid object id.
 *    This object ID list should be returned as an IPortableDevicePropVariantCollection of VT_LPWSTR
 *    in WPD_PROPERTY_COMMON_OBJECT_IDS.
 *    Order is implicit, i.e. the first element in the Persistent Unique ID list corresponds to the
 *    to the first element of the ObjectID list and so on.
 *
 *    For those elements where an existing ObjectID could not be found (e.g. the
 *    object is no longer present on the device), the element will contain the
 *    empty string (L"").
 */
HRESULT WpdBaseDriver::OnGetObjectIDsFromPersistentUniqueIDs(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{

    HRESULT hr      = S_OK;
    CComPtr<IPortableDevicePropVariantCollection> pPersistentIDs;
    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;

    if((pParams    == NULL) ||
       (pResults   == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Get the list of Persistent IDs
    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS, &pPersistentIDs);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS");
    }

    // Create the collection to hold the ObjectIDs
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pObjectIDs);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }


    if (hr == S_OK)
    {
        ACCESS_SCOPE Scope = m_Device.GetAccessScope(pParams);
        hr = m_Device.GetObjectIDsFromPersistentUniqueIDs(Scope, pPersistentIDs, pObjectIDs);
        CHECK_HR(hr, "Failed to get object IDs from persistent IDs");
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_COMMON_OBJECT_IDS, pObjectIDs);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_OBJECT_IDS");
    }

    return hr;
}


/**
 *  This method is called when we receive a WPD_COMMAND_COMMON_SAVE_CLIENT_INFORMATION
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_COMMON_CLIENT_INFORMATION: Contains information about the client, including version
 *    and optionally the client event cookie.
 *
 *  The driver should:
 *  - Save the client information and return an LPWSTR context for this client.  
 *    The client can be identified using this context for subsequent commands to the driver.
 */
HRESULT WpdBaseDriver::OnSaveClientInfo(
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{

    HRESULT         hr              = S_OK;
    GUID            guidContext     = GUID_NULL;
    CComBSTR        bstrContext;
    ClientContext*  pContext        = NULL;
    ContextMap*     pContextMap     = NULL;

    CComPtr<IPortableDeviceValues> pClientInfo;

    if((pParams    == NULL) ||
       (pResults   == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = CoCreateGuid(&guidContext);
    if (hr == S_OK)
    {
        bstrContext = guidContext;
        if(bstrContext.Length() == 0)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to create BSTR from GUID");
        }
    }

    // Get the client info
    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_COMMON_CLIENT_INFORMATION, &pClientInfo);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_CLIENT_INFORMATION");
    }

    // Get the context map which the driver stored in pParams for convenience
    if (hr == S_OK)
    {
        hr = GetClientContextMap(pParams, &pContextMap);
        CHECK_HR(hr, "Failed to get client context map");
    }

    // Create the new client info context we will save in the context map
    if (hr == S_OK)
    {
        pContext = new ClientContext();
        if(pContext == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, ("Could not allocate memory for client info context"));
        }
    }

    // Save the client info.  Since these are optional, none of this is fatal if
    // they don't exist.
    if (hr == S_OK)
    {
        LPWSTR pszClientName    = NULL;
        LPWSTR pszEventCookie   = NULL;

        pClientInfo->GetStringValue(WPD_CLIENT_NAME, &pszClientName);
        if(pszClientName != NULL)
        {
            pContext->ClientName = pszClientName;
        }
        pClientInfo->GetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, &(pContext->MajorVersion));
        pClientInfo->GetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, &(pContext->MinorVersion));
        pClientInfo->GetUnsignedIntegerValue(WPD_CLIENT_REVISION,      &(pContext->Revision));
        
        pClientInfo->GetStringValue(WPD_CLIENT_EVENT_COOKIE, &pszEventCookie);
        if (pszEventCookie != NULL)
        {
            pContext->EventCookie = pszEventCookie;
        }

        CoTaskMemFree(pszClientName);
        CoTaskMemFree(pszEventCookie);
    }

    if ((hr == S_OK) &&
        (pContext->ClientName.GetLength() > 0) &&
        (pContextMap != NULL))
    {
        CAtlStringW strContext;
        hr = pContextMap->Add(pContext, strContext);
        CHECK_HR(hr, "Failed to add client info context to context map");

        if (hr == S_OK)
        {
            hr = pResults->SetStringValue(WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT, strContext);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT");
        }
    }

    SAFE_RELEASE(pContext);
    SAFE_RELEASE(pContextMap);

    return hr;
}
