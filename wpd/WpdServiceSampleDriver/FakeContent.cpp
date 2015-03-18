#include "stdafx.h"
#include "FakeContent.tmh"

HRESULT FakeContent::InitializeContent(_Inout_ DWORD *pdwLastObjectID)
{
    UNREFERENCED_PARAMETER(pdwLastObjectID);
    return S_OK;
}

HRESULT FakeContent::InitializeEnumerationContext(
            ACCESS_SCOPE                Scope,
    _In_    WpdObjectEnumeratorContext* pEnumeratorContext)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(Scope);

    if (pEnumeratorContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    pEnumeratorContext->m_TotalChildren = static_cast<DWORD>(m_Children.GetCount());
    return hr;
}

HRESULT FakeContent::GetSupportedProperties(_In_   IPortableDeviceKeyCollection *pKeys)
{
    HRESULT hr = S_OK;

    if(pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL collection parameter");
        return hr;
    }

    hr = pKeys->Add(WPD_OBJECT_ID);
    CHECK_HR(hr, "Failed to add WPD_OBJECT_ID");

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_PERSISTENT_UNIQUE_ID);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_PERSISTENT_UNIQUE_ID");
    }            

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_PARENT_ID);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_PARENT_ID");
    }

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_NAME);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_NAME");
    }

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_CONTENT_TYPE);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_CONTENT_TYPE");
    }

    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_FORMAT);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_FORMAT");
    }
    
    if (hr == S_OK)
    {
        hr = pKeys->Add(WPD_OBJECT_CAN_DELETE);
        CHECK_HR(hr, "Failed to add WPD_OBJECT_CAN_DELETE");
    }

    return hr;
}

HRESULT FakeContent::GetPropertyAttributes(
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL attributes parameter");
        return hr;
    }
    
    hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_DELETE, FALSE);
    CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_DELETE");

    if (hr == S_OK)
    {
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_READ, TRUE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_READ");
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, TRUE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
        }
        else
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, FALSE);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
        }            
    }

    if (hr == S_OK)
    {
        hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY, TRUE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY");
    }

    if (hr == S_OK)
    {
        hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_UNSPECIFIED);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM");
    }

    return hr;
}

HRESULT FakeContent::GetValue(
    _In_    REFPROPERTYKEY         Key,
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr = S_OK;
    PropVariantWrapper pvValue;

    if(pStore == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
    {
        // Add WPD_OBJECT_ID
        pvValue = ObjectID;
        hr = pStore->SetValue(WPD_OBJECT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
    {
        // Add WPD_OBJECT_PERSISTENT_UNIQUE_ID
        pvValue = this->PersistentUniqueID;
        hr = pStore->SetValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
    {
        // Add WPD_OBJECT_PARENT_ID
        pvValue = ParentID;
        hr = pStore->SetValue(WPD_OBJECT_PARENT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PARENT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        // Add WPD_OBJECT_NAME
        pvValue = Name;
        hr = pStore->SetValue(WPD_OBJECT_NAME, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_NAME"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
    {    
        // Add WPD_OBJECT_CONTENT_TYPE
        hr = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTENT_TYPE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
    {
        // Add WPD_OBJECT_FORMAT
        hr = pStore->SetGuidValue(WPD_OBJECT_FORMAT, Format);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_FORMAT"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
    {
        // Add WPD_OBJECT_CAN_DELETE
        hr = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CAN_DELETE"));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property {%ws}.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }
    return hr;
}

HRESULT FakeContent::WriteValue(
    _In_    REFPROPERTYKEY  Key,
    _In_    REFPROPVARIANT  Value)
{
    HRESULT             hr      = S_OK;
    PropVariantWrapper  pvValue;

    if(IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        if(Value.vt == VT_LPWSTR)
        {
            if (Value.pwszVal != NULL && Value.pwszVal[0] != L'\0')
            {
                Name = Value.pwszVal;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME because value was an empty string");
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Failed to set WPD_OBJECT_NAME because type was not VT_LPWSTR");
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "Property %ws.%d on [%ws] does not support set value operation", CComBSTR(Key.fmtid), Key.pid, ObjectID);
    }

    return hr;
}

HRESULT FakeContent::CreatePropertiesOnlyObject(
    _In_                           IPortableDeviceValues* pObjectProperties, 
    _Out_                          DWORD*                 pdwLastObjectID,
    _Outptr_result_nullonfailure_  FakeContent**          ppNewObject)
{
    UNREFERENCED_PARAMETER(pObjectProperties);
    UNREFERENCED_PARAMETER(pdwLastObjectID);
    *ppNewObject = NULL;
    return E_ACCESSDENIED;
}

HRESULT FakeContent::GetSupportedResources(
    _In_    IPortableDeviceKeyCollection* pResources)
{
    UNREFERENCED_PARAMETER(pResources);
    return S_OK;
}

HRESULT FakeContent::GetResourceAttributes(
    _In_    REFPROPERTYKEY         Resource,
    _In_    IPortableDeviceValues* pAttributes)
{
    UNREFERENCED_PARAMETER(Resource);
    UNREFERENCED_PARAMETER(pAttributes);
    return S_OK;
}

HRESULT FakeContent::OpenResource(
    _In_    REFPROPERTYKEY            Resource,
            const DWORD               dwMode,
    _In_    WpdObjectResourceContext* pResourceContext)
{
    UNREFERENCED_PARAMETER(Resource);
    UNREFERENCED_PARAMETER(dwMode);
    UNREFERENCED_PARAMETER(pResourceContext);
    return S_OK;
}

HRESULT FakeContent::ReadResourceData(
    _In_                            WpdObjectResourceContext*   pResourceContext,
    _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)   BYTE* pBuffer,
                                    const DWORD                 dwNumBytesToRead,
    _Out_                           DWORD*                      pdwNumBytesRead)
{
    UNREFERENCED_PARAMETER(pResourceContext);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(dwNumBytesToRead);
    *pdwNumBytesRead = 0;
    return S_OK;
}

bool FakeContent::CanAccess(
    ACCESS_SCOPE Scope)
{
    return ((Scope & RequiredScope) == RequiredScope);
}

HRESULT FakeContent::GetAllValues(
    _In_    IPortableDeviceValues* pStore)
{
    HRESULT hr    = S_OK;
    DWORD   cKeys = 0;
    CComPtr<IPortableDeviceKeyCollection> pKeys;
    
    if(pStore == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // CoCreate a collection to store the property keys we are going to use
    // to request the property values of.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pKeys);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }    
    
    if (hr == S_OK)
    {
        hr = GetSupportedProperties(pKeys);
        CHECK_HR(hr, "Failed to get supported properties");
    }
    
    if (hr == S_OK)
    {
        hr = pKeys->GetCount(&cKeys);
        CHECK_HR(hr, "Failed to get supported properties");
    }
    
    if (hr == S_OK)
    {
        for (DWORD i=0; i<cKeys; i++)
        {
            PROPERTYKEY Key = {0};
            hr = pKeys->GetAt(i, &Key);
            CHECK_HR(hr, "Failed to get supported property at index %d", i);
            if (hr == S_OK)
            {
                hr = GetValue(Key, pStore);
                CHECK_HR(hr, "Failed to get property value at index %d", i);
            }
        }    
    }

    return hr;
}

HRESULT FakeContent::WriteValues(
    _In_    IPortableDeviceValues* pValues,
    _In_    IPortableDeviceValues* pResults,
    _Out_   bool*                  pbObjectChanged)
{
    HRESULT hr             = S_OK;
    DWORD   cValues        = 0;
    bool    hasFailedWrite = false;

    if (pValues == NULL || pResults == NULL || pbObjectChanged == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;        
    }

    hr = pValues->GetCount(&cValues);
    CHECK_HR(hr, "Failed to get total number of values");

    (*pbObjectChanged) = false;

    for (DWORD dwIndex = 0; dwIndex < cValues; dwIndex++)
    {
        PROPERTYKEY Key = WPD_PROPERTY_NULL;
        PROPVARIANT Value;
        PropVariantInit(&Value);

        hr = pValues->GetAt(dwIndex, &Key, &Value);
        CHECK_HR(hr, "Failed to get PROPERTYKEY at index %d", dwIndex);

        if (hr == S_OK)
        {
            HRESULT hrWrite = WriteValue(Key, Value);
            if (FAILED(hrWrite))
            {
                CHECK_HR(hrWrite, "Failed to write value at index %d", dwIndex);
                hasFailedWrite = true;
            }
            else
            {
                (*pbObjectChanged) = true;
            }

            hr = pResults->SetErrorValue(Key, hrWrite);
            CHECK_HR(hr, "Failed to set error result value at index %d", dwIndex);
        }

        PropVariantClear(&Value);
    }

    // Since we have set failures for the property set operations we must let the application
    // know by returning S_FALSE. This will instruct the application to look at the
    // property set operation results for failure values.
    if ((hr == S_OK) && hasFailedWrite)
    {
        hr = S_FALSE;
    }

    return hr;
}

_Success_(return)
bool FakeContent::FindNext(
                                  ACCESS_SCOPE  Scope,
                                  const DWORD   dwIndex,
    _Outptr_result_nullonfailure_ FakeContent** ppChild)
{
    HRESULT hr     = S_OK;
    bool    bFound = false;

    if (ppChild == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return false;
    }

    *ppChild = NULL;

    if (dwIndex < m_Children.GetCount())
    {
        if (m_Children[dwIndex] && (m_Children[dwIndex]->CanAccess(Scope)))
        {
            *ppChild = m_Children[dwIndex];
            bFound = true;
        }
    }

    return bFound;
}

HRESULT FakeContent::GetContent(
                                    ACCESS_SCOPE   Scope,
    _In_                            LPCWSTR        wszObjectID,
    _Outptr_result_nullonfailure_   FakeContent**  ppContent)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    *ppContent = NULL;

    if (CanAccess(Scope))
    {
        if (ObjectID.CompareNoCase(wszObjectID) == 0)
        {
                hr = S_OK;
                *ppContent = this;
        }
        else
        {
            DWORD dwIndex = 0;
            FakeContent* pChild = NULL;
            while (FindNext(Scope, dwIndex, &pChild))
            {
                hr = pChild->GetContent(Scope, wszObjectID, ppContent);
                if (hr == S_OK || hr == E_ACCESSDENIED)
                {
                    break;
                }
                dwIndex++;
            }
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "GetContent: '%ws' was found but falls outside scope", wszObjectID);
    }

    return hr;
}

HRESULT FakeContent::GetObjectIDsByFormat(
            ACCESS_SCOPE                          Scope,
    _In_    REFGUID                               guidFormat,
            const DWORD                           dwDepth,
    _In_    IPortableDevicePropVariantCollection* pObjectIDs)
{
    HRESULT hr = S_OK;

    if (CanAccess(Scope))
    {
        DWORD        dwIndex = 0;
        FakeContent* pChild  = NULL;

        if (Format == guidFormat || guidFormat == WPD_OBJECT_FORMAT_ALL)
        {
            PROPVARIANT pv = {0};
            PropVariantInit(&pv);
            pv.vt = VT_LPWSTR;
            pv.pwszVal = ObjectID.GetBuffer();
            hr = pObjectIDs->Add(&pv);
            CHECK_HR(hr, "Failed to add '%ws' to the list of object IDs by format", ObjectID);
        }

        if (dwDepth > 0)
        {
            while ((hr == S_OK) && (FindNext(Scope, dwIndex, &pChild)))
            {
                hr = pChild->GetObjectIDsByFormat(Scope, guidFormat, dwDepth-1, pObjectIDs);
                CHECK_HR(hr, "Failed to get object IDs by format for child at index %d", dwIndex);
                dwIndex++;
            }
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    
    return hr;
}

HRESULT FakeContent::GetObjectIDByPersistentID(
            ACCESS_SCOPE                          Scope,
    _In_    LPCWSTR                               wszPersistentID,
    _In_    IPortableDevicePropVariantCollection* pObjectIDs)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (CanAccess(Scope))
    { 
        if (PersistentUniqueID.CompareNoCase(wszPersistentID) == 0)
        {
            PROPVARIANT pv = {0};
            PropVariantInit(&pv);
            pv.vt = VT_LPWSTR;
            pv.pwszVal = ObjectID.GetBuffer();

            hr = pObjectIDs->Add(&pv);
            CHECK_HR(hr, "Failed to add '%ws' to the list of object IDs", ObjectID);
        }
        else
        {
            DWORD        dwIndex = 0;
            FakeContent* pChild  = NULL;
            while (FindNext(Scope, dwIndex, &pChild))
            {            
                hr = pChild->GetObjectIDByPersistentID(Scope, wszPersistentID, pObjectIDs);
                if (hr == S_OK || hr == E_ACCESSDENIED)
                {
                    // Found the object or was denied access
                    break;
                }
                else if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
                {
                    CHECK_HR(hr, "Failed to get object ID for child at index %d", dwIndex);
                }
                dwIndex++;
            }
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

HRESULT FakeContent::MarkForDelete(
    const DWORD dwOptions)
{
    HRESULT hr = S_OK;

    if (CanDelete == false)
    {
        hr = E_ACCESSDENIED;
        CHECK_HR(hr, "Object '%ws' is not deletable", ObjectID);
        return hr;
    }
    
    if (dwOptions == PORTABLE_DEVICE_DELETE_NO_RECURSION)
    {
        if (m_Children.GetCount() > 0)
        {
            hr = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
        }
    }
    else if (dwOptions == PORTABLE_DEVICE_DELETE_WITH_RECURSION)
    {
        // Mark children for delete
        for (size_t Index = 0; Index < m_Children.GetCount(); Index++)
        {
            if (m_Children[Index])
            {
                hr = m_Children[Index]->MarkForDelete(dwOptions);
                CHECK_HR(hr, "Failed to mark child '%ws' for deletion", m_Children[Index]->ObjectID);
            }
        }
    }

    if (hr == S_OK)
    {
        // All successful. Mark self for delete
        MarkedForDeletion = true;
    }

    return hr;
}

HRESULT FakeContent::RemoveObjectsMarkedForDeletion(
    ACCESS_SCOPE Scope)
{
    HRESULT      hr      = S_OK;
    DWORD        dwIndex = 0;
    FakeContent* pChild  = NULL;

    while (FindNext(Scope, dwIndex, &pChild))
    {
        if (pChild != NULL)
        {
            hr = pChild->RemoveObjectsMarkedForDeletion(Scope);
            CHECK_HR(hr, "Failed to remove children marked for deletion for object '%ws'", pChild->ObjectID);

            if ((hr == S_OK) && (pChild->MarkedForDeletion == true))
            {
                m_Children.RemoveAt(dwIndex);
                delete pChild;
                pChild = NULL;
            }
        }
        dwIndex++;
    }

    m_Children.FreeExtra();

    return hr;
}
