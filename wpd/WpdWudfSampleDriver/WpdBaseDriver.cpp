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

HRESULT WpdBaseDriver::DispatchWpdMessage(_In_  IPortableDeviceValues* pParams,
                                          _In_  IPortableDeviceValues* pResults)
{
    HRESULT         hr                   = S_OK;
    GUID            guidCommandCategory  = {0};
    DWORD           dwCommandID          = 0;
    PROPERTYKEY     CommandKey           = WPD_PROPERTY_NULL;

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
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_PROPERTIES)
        {
            hr = m_ObjectProperties.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_PROPERTIES_BULK)
        {
            hr = m_ObjectPropertiesBulk.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_RESOURCES)
        {
            hr = m_ObjectResources.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(CommandKey.fmtid == WPD_CATEGORY_OBJECT_MANAGEMENT)
        {
            hr = m_ObjectManagement.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_CAPABILITIES)
        {
            hr = m_Capabilities.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_STORAGE)
        {
            hr = m_Storage.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_NETWORK_CONFIGURATION)
        {
            hr = m_NetworkConfig.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if(IsEqualPropertyKey(CommandKey, WPD_COMMAND_COMMON_SAVE_CLIENT_INFORMATION))
        {
            hr = OnSaveClientInfo(pParams, pResults);
        }
        else if(IsEqualPropertyKey(CommandKey, WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS))
        {
            hr = OnGetObjectIDsFromPersistentUniqueIDs(pParams, pResults);
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "Unknown command %ws.%d received",CComBSTR(CommandKey.fmtid), CommandKey.pid);
        }
    }

    HRESULT hrTemp = pResults->SetErrorValue(WPD_PROPERTY_COMMON_HRESULT, hr);
    CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_COMMON_HRESULT"));

    // Set to a success code, to indicate that the message was recieved.
    // the return code for the actual command's results is stored in the
    // WPD_PROPERTY_COMMON_HRESULT property.
    hr = S_OK;

    return hr;
}

/**
 * This method is called to initialize the driver object.
 * In a real driver, this is where the driver would set up it's I/O libraries
 * and so on.
 *
 * For this sample driver, since we don't have a real device, we
 * simply ignore the port name and initialize our internal FakeDevice.
 *
 */
HRESULT WpdBaseDriver::Initialize(
    _In_    LPCWSTR                         pszPortName,
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension)
{
    UNREFERENCED_PARAMETER(pszPortName);

    HRESULT     hr  = S_OK;

    if(pPortableDeviceClassExtension == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    m_pPortableDeviceClassExtension = pPortableDeviceClassExtension;

    if (hr == S_OK)
    {
        hr = m_FakeDevice.InitializeContent(m_pPortableDeviceClassExtension);
        CHECK_HR(hr, "Failed to initialize our fake device");
    }

    if (hr == S_OK)
    {
        hr = m_ObjectEnum.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our object enumerator");
    }

    if (hr == S_OK)
    {
        hr = m_ObjectProperties.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our properties object");
    }

    if (hr == S_OK)
    {
        hr = m_ObjectPropertiesBulk.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our properties bulk object");
    }

    if (hr == S_OK)
    {
        hr = m_ObjectResources.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our resources object");
    }

    if (hr == S_OK)
    {
        hr = m_ObjectManagement.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our object managment object");
    }

    if (hr == S_OK)
    {
        hr = m_Capabilities.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our capabilities object");
    }

    if (hr == S_OK)
    {
        hr = m_Storage.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our storage object");
    }

    if (hr == S_OK)
    {
        hr = m_NetworkConfig.Initialize(&m_FakeDevice);
        CHECK_HR(hr, "Failed to initialize our network config object");
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

/**
 * Save the client information
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
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
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
        CAtlStringW strKey = bstrContext;
        hr = pContextMap->Add(strKey, pContext);
        CHECK_HR(hr, "Failed to add client info context to context map");

        if (hr == S_OK)
        {
            hr = pResults->SetStringValue(WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT, bstrContext);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT");
        }
    }

    SAFE_RELEASE(pContext); // Always release the context, pContextMap::Add would have AddRef'ed it
    SAFE_RELEASE(pContextMap);

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
    DWORD   dwCount = 0;
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

    // Iterate through the persistent ID list and add the equivalent object ID for each element.
    if (hr == S_OK)
    {
        hr = pPersistentIDs->GetCount(&dwCount);
        CHECK_HR(hr, "Failed to get count from persistent ID collection");

        if (hr == S_OK)
        {
            DWORD       dwIndex = 0;
            PROPVARIANT pvPersistentID = {0};
            PROPVARIANT pvObjectID = {0};
            PropVariantInit(&pvPersistentID);
            PropVariantInit(&pvObjectID);

            for(dwIndex = 0; dwIndex < dwCount; dwIndex++)
            {
                pvObjectID.vt = VT_LPWSTR;
                hr = pPersistentIDs->GetAt(dwIndex, &pvPersistentID);
                CHECK_HR(hr, "Failed to get persistent ID at index %d", dwIndex);

                if (hr == S_OK)
                {
                    hr = m_FakeDevice.GetObjectIDFromPersistentID(pvPersistentID.pwszVal, &pvObjectID.pwszVal);
                    // Don't log this error, since it is expected if the object is not found
                }

                if(FAILED(hr))
                {
                    hr = S_OK;
                    pvObjectID.pwszVal = AtlAllocTaskWideString(L"");
                    if(pvObjectID.pwszVal == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        CHECK_HR(hr, "Failed to allocate memory for empty ObjectID");
                    }
                }

                if (hr == S_OK)
                {
                    hr = pObjectIDs->Add(&pvObjectID);
                    CHECK_HR(hr, "Failed to add next Object ID");
                }

                PropVariantClear(&pvPersistentID);
                PropVariantClear(&pvObjectID);

                if(FAILED(hr))
                {
                    break;
                }
            }
        }
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_COMMON_OBJECT_IDS, pObjectIDs);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_OBJECT_IDS");
    }

    return hr;
}

