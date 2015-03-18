#include "stdafx.h"
#include "WpdObjectEnum.tmh"

WpdObjectEnumerator::WpdObjectEnumerator()
{

}

WpdObjectEnumerator::~WpdObjectEnumerator()
{

}

HRESULT WpdObjectEnumerator::Initialize(_In_ FakeDevice *pFakeDevice)
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

HRESULT WpdObjectEnumerator::DispatchWpdMessage(_In_    REFPROPERTYKEY          Command,
                                                _In_    IPortableDeviceValues*  pParams,
                                                _In_    IPortableDeviceValues*  pResults)
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
 *  - Return an identifier for the context in WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT.
 *
 */
HRESULT WpdObjectEnumerator::OnStartFind(_In_    IPortableDeviceValues*  pParams,
                                         _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr              = S_OK;
    LPWSTR      pszParentID     = NULL;
    LPWSTR      pszEnumContext  = NULL;
    IUnknown*   pContextMap     = NULL;

    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID, &pszParentID);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_PARENT_ID");
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, &pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateEnumContext((ContextMap*)pContextMap, pszParentID, &pszEnumContext);
        CHECK_HR(hr, "Failed to create enumeration context");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, pszEnumContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszParentID);
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszEnumContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT WpdObjectEnumerator::OnFindNext(_In_    IPortableDeviceValues*  pParams,
                                        _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr                  = S_OK;
    LPWSTR      pszEnumContext      = NULL;
    DWORD       dwNumObjects        = 0;
    ContextMap* pContextMap         = NULL;
    CComPtr<IPortableDevicePropVariantCollection>   pObjectIDCollection;

    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, &pszEnumContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED, &dwNumObjects);
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_NUM_OBJECTS_REQUESTED");
        }
    }

    // Get the context map which the driver stored in pParams for convenience
    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        //  Find the next objects in the enumeration
        hr = GetObjectIDs(pContextMap, dwNumObjects, pszEnumContext, &pObjectIDCollection);
        CHECK_HR(hr, "Failed to get the objectIDs");
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS, pObjectIDCollection);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_OBJECT_ENUMERATION_OBJECT_IDS");
    }

    if (SUCCEEDED(hr))
    {
        if(pObjectIDCollection != NULL)
        {
            ULONG    ulCount    = 0;

            pObjectIDCollection->GetCount(&ulCount);
            if(ulCount < dwNumObjects)
            {
                hr = S_FALSE;
            }
        }
    }
    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszEnumContext);

    SAFE_RELEASE(pContextMap);

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
 *  - Destroy any resources associated with this context.
 */
HRESULT WpdObjectEnumerator::OnEndFind(_In_    IPortableDeviceValues*  pParams,
                                       _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr              = S_OK;
    LPWSTR      pszEnumContext  = NULL;
    IUnknown*   pContextMap     = NULL;

    UNREFERENCED_PARAMETER(pResults);

    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT, &pszEnumContext);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing value for WPD_PROPERTY_OBJECT_ENUMERATION_CONTEXT");
    }

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, &pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        hr = DestroyEnumContext((ContextMap*) pContextMap, pszEnumContext);
        CHECK_HR(hr, "Failed to destroy enumeration context");
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszEnumContext);

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT WpdObjectEnumerator::CreateEnumContext(
    _In_                            ContextMap* pContextMap,
    _In_                            LPCWSTR     pszParentID,
    _Outptr_result_nullonfailure_   LPWSTR*     ppszEnumContext)
{
    HRESULT         hr              = S_OK;
    GUID            guidContext     = GUID_NULL;
    CComBSTR        bstrContext;
    EnumContext*    pContext        = NULL;

    if((pContextMap     == NULL) ||
       (pszParentID     == NULL) ||
       (ppszEnumContext == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppszEnumContext = NULL;

    hr = CoCreateGuid(&guidContext);
    if (SUCCEEDED(hr))
    {
        bstrContext = guidContext;
        if(bstrContext.Length() == 0)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to create BSTR from GUID");
        }
    }

    if (SUCCEEDED(hr))
    {
        pContext = new EnumContext();
        if(pContext != NULL)
        {
            CAtlStringW strKey = bstrContext;
            pContext->ParentID = pszParentID;

            hr = pContextMap->Add(strKey, pContext);
            CHECK_HR(hr, "Failed to add enumeration context to client context map");

            pContext->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate enumeration context");
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppszEnumContext = AtlAllocTaskWideString(bstrContext);
    }

    return hr;
}

HRESULT WpdObjectEnumerator::DestroyEnumContext(
    _In_    ContextMap* pContextMap,
    _In_    LPCWSTR     pszEnumContext)
{
    HRESULT     hr  = S_OK;

    CAtlStringW strKey = pszEnumContext;
    pContextMap->Remove(strKey);

    return hr;
}

#pragma warning(suppress: 6388) // PREFast bug means here there is a false positive for the call to CComPtr<>::QueryInterface().
HRESULT WpdObjectEnumerator::GetObjectIDs(
    _In_         ContextMap*                               pContextMap,
    _In_         const DWORD                               dwNumObjects,
    _In_         LPCWSTR                                   pszEnumContext,
    _COM_Outptr_ IPortableDevicePropVariantCollection**    ppCollection)
{
    HRESULT                                         hr              = S_OK;
    EnumContext*                                    pContext        = NULL;
    CComPtr<IPortableDevicePropVariantCollection>   pObjectIDs;

    if(ppCollection == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL collection parameter");
        return hr;
    }

    *ppCollection = NULL;

    hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_PPV_ARGS(&pObjectIDs));
    CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");

    if (SUCCEEDED(hr))
    {
        CAtlStringW strKey = pszEnumContext;
        pContext = (EnumContext*) pContextMap->GetContext(strKey);
        if(pContext != NULL)
        {
            for(DWORD dwCount = 0; dwCount < dwNumObjects; dwCount++)
            {
                CAtlStringW strObjectID;

                DWORD dwNextStartIndex = 0;
                if(m_pFakeDevice->FindNext(pContext->SearchIndex,
                                           pContext->ParentID,
                                           strObjectID,
                                           &dwNextStartIndex))
                {
                    PropVariantWrapper pvObjectID(strObjectID);

                    hr = pObjectIDs->Add(&pvObjectID);
                    CHECK_HR(hr, "Failed to add object [%ws]", pvObjectID.pwszVal);
                    pContext->SearchIndex = dwNextStartIndex;
                }
            }

            pContext->Release();
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Cannot find enum context [%ws]", pszEnumContext);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pObjectIDs->QueryInterface(IID_PPV_ARGS(ppCollection));
        CHECK_HR(hr, "Failed to QI for IID_IPortableDevicePropVariantCollection on IPortableDevicePropVariantCollection");
    }
    return hr;
}

