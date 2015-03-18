#include "stdafx.h"

#include "WpdServiceMethods.tmh"

CMethodTask::CMethodTask(_In_ ServiceMethodContext* pContext) :
    m_hThread(NULL),
    m_pContext(pContext)
{
    m_pContext->AddRef();
}

CMethodTask::~CMethodTask()
{
    if (m_hThread != NULL)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    SAFE_RELEASE(m_pContext);
}

HRESULT CMethodTask::Run()
{
    HRESULT hr = S_OK;

    // Create the thread
    m_hThread = CreateThread(NULL, 0, ThreadProc, m_pContext, 0, NULL);
    if (m_hThread == NULL)
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}

ServiceMethodContext::ServiceMethodContext() :
    m_cRef(1),
    m_pServiceMethods(NULL)
{
    m_pTask = NULL;
}

ServiceMethodContext::~ServiceMethodContext()
{
    if (m_pTask)
    {
        delete m_pTask;
        m_pTask = NULL;
    }
}

HRESULT ServiceMethodContext::Initialize(
    _In_ WpdServiceMethods*     pServiceMethods,
    _In_ IPortableDeviceValues* pStartParams,
    _In_ LPCWSTR                pwszContext)
{
    HRESULT hr = S_OK;

    m_pTask = new CMethodTask(this);
    if (m_pTask != NULL)
    {
        m_pServiceMethods  = pServiceMethods;
        m_pStartParameters = pStartParams;
        m_strContext       = pwszContext;

        hr = m_pTask->Run();
        CHECK_HR(hr, "Failed to run method task");
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_HR(hr, "Failed to allocate method task");
    }
    return hr;
}

VOID ServiceMethodContext::InvokeMethod()
{
    if (m_pServiceMethods        != NULL &&
        m_pStartParameters       != NULL &&
        m_strContext.GetLength()  > 0)
    {
        m_hrStatus = m_pServiceMethods->DispatchMethod(m_strContext, m_pStartParameters, &m_pResults);
    }
    CHECK_HR(m_hrStatus, "Failed to Dispatch method");
}


WpdServiceMethods::WpdServiceMethods()
    : m_pContactsService(NULL)
{

}

WpdServiceMethods::~WpdServiceMethods()
{

}

HRESULT WpdServiceMethods::Initialize(
    _In_  FakeContactsService* pContactsService)
{
    if (pContactsService == NULL)
    {
        return E_POINTER;
    }
    m_pContactsService = pContactsService;
    return S_OK;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_METHODS_START_INVOKE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_METHOD: Indicates the method to invoke.
 *     This must be from the list returned by WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_METHODS
 *     or WPD_COMMAND_SERVICE_CAPABILITIES_GET_SUPPORTED_ METHODS_BY_FORMAT.
 *
 *  - WPD_PROPERTY_SERVICE_METHOD_PARAMETER_VALUES: IPortableDeviceValues containing the method parameters.
 *     Each parameter must be set in the ordering specified by WPD_PARAMETER_ATTRIBUTE_ORDER, with all parameters present.
 *     This must be an empty set if the method does not have any parameters.
 *
 *  The driver should:
 *  - Return immediately with the method invocation context in WPD_PROPERTY_SERVICE_METHOD_CONTEXT.
 *  - When this method invocation completes, the driver must send a WPD_EVENT_SERVICE_METHOD_COMPLETE event
 *    with the WPD_EVENT_PARAMETER_SERVICE_METHOD_CONTEXT parameter set as this method context.
 *  - Lastly, the driver should wait for the WPD_COMMAND_SERVICE_METHODS_END_INVOKE command
 *    before cleaning up associated resources with this context
 */
HRESULT WpdServiceMethods::OnStartInvoke(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr          = S_OK;
    LPWSTR  pwszContext = NULL;

    // Create a new method context
    hr = StartMethod(pParams, &pwszContext);
    CHECK_HR(hr, "Failed to create a new method context");

    // Return the method context in the results
    if (SUCCEEDED(hr))
    {
        hr = pResults->SetStringValue(WPD_PROPERTY_SERVICE_METHOD_CONTEXT, pwszContext);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_METHOD_CONTEXT");
    }

    CoTaskMemFree(pwszContext);
    return hr;
}


/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_METHODS_END_INVOKE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_METHOD_CONTEXT: Context of the method invocation being ended.
 *     This must be returned from WPD_COMMAND_SERVICE_METHODS_START_INVOKE
 *
 *  The driver should:
 *  - Return the method results in WPD_PROPERTY_SERVICE_METHOD_RESULT_VALUES
 *  - Return the overall method status code in WPD_PROPERTY_SERVICE_METHOD_HRESULT
 *  - Destroy any resources associated with this context.
 */
HRESULT WpdServiceMethods::OnEndInvoke(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr          = S_OK;
    HRESULT     hrStatus    = S_OK;
    LPWSTR      pwszContext = NULL;
    ContextMap* pContextMap = NULL;

    CComPtr<IPortableDeviceValues> pMethodResults;

    hr = pParams->GetStringValue(WPD_PROPERTY_SERVICE_METHOD_CONTEXT, &pwszContext);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_METHOD_CONTEXT from IPortableDeviceValues");

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        hr = EndMethod(pContextMap, pwszContext, &pMethodResults, &hrStatus);
        CHECK_HR(hr, "Failed to destroy method context %ws", pwszContext);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetErrorValue(WPD_PROPERTY_SERVICE_METHOD_HRESULT, hrStatus);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_METHOD_HRESULT for method context %ws", pwszContext);
    }

    if (SUCCEEDED(hr))
    {
        hr = pResults->SetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_METHOD_RESULT_VALUES, pMethodResults);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_SERVICE_METHOD_RESULT_VALUES for method context %ws", pwszContext);
    }

    CoTaskMemFree(pwszContext);
    SAFE_RELEASE(pContextMap);
    return hr;
}


/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_METHODS_CANCEL_INVOKE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_SERVICE_METHOD_CONTEXT: Context of the method invocation being cancelled.
 *     This must be returned from WPD_COMMAND_SERVICE_METHODS_START_INVOKE
 *
 *  The driver should:
 *  - Destroy any resources associated with this context.
 *
 */
HRESULT WpdServiceMethods::OnCancelInvoke(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT     hr          = S_OK;
    LPWSTR      pwszContext = NULL;
    ContextMap* pContextMap = NULL;
    UNREFERENCED_PARAMETER(pResults);

    hr = pParams->GetStringValue(WPD_PROPERTY_SERVICE_METHOD_CONTEXT, &pwszContext);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_METHOD_CONTEXT from IPortableDeviceValues");

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        hr = CancelMethod(pContextMap, pwszContext);
        CHECK_HR(hr, "Failed to cancel the method");
    }

    CoTaskMemFree(pwszContext);
    SAFE_RELEASE(pContextMap);
    return hr;
}

HRESULT WpdServiceMethods::StartMethod(
    _In_                           IPortableDeviceValues*  pParams,
    _Outptr_result_nullonfailure_  LPWSTR*                 ppwszMethodContext)
{
    HRESULT                 hr          = S_OK;
    ContextMap*             pContextMap = NULL;
    ServiceMethodContext*   pContext    = NULL;
    GUID                    Method      = GUID_NULL;

    CAtlStringW  strKey;
    CComPtr<IPortableDeviceValues> pMethodParams;

    if((pParams         == NULL)  ||
       (ppwszMethodContext    == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppwszMethodContext = NULL;

    // Check if the method is supported
    hr = pParams->GetGuidValue(WPD_PROPERTY_SERVICE_METHOD, &Method);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_METHOD");

    if (SUCCEEDED(hr) && !m_pContactsService->IsMethodSupported(Method))
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Unknown method %ws received",CComBSTR(Method));
    }

    if (SUCCEEDED(hr))
    {
        // Get the context map which the driver stored in pParams for convenience
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**)&pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr))
    {
        pContext = new ServiceMethodContext();
        if(pContext == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate new method context");
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pContextMap->Add(pContext, strKey);
        CHECK_HR(hr, "Failed to insert method context into our context Map");
    }

    if (SUCCEEDED(hr))
    {
        hr = pContext->Initialize(this, pParams, strKey);
        CHECK_HR(hr, "Failed to initialize the method context");
    }

    if (SUCCEEDED(hr))
    {
        *ppwszMethodContext = AtlAllocTaskWideString(strKey);
        if (*ppwszMethodContext == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate method context string");
        }
    }

    SAFE_RELEASE(pContextMap);
    SAFE_RELEASE(pContext);

    return hr;
}

HRESULT WpdServiceMethods::EndMethod(
    _In_                          ContextMap*             pContextMap,
    _In_                          LPCWSTR                 pwszMethodContext,
    _COM_Outptr_result_maybenull_ IPortableDeviceValues** ppResults,
    _Out_                         HRESULT*                phrStatus)
{
    HRESULT                 hr       = S_OK;
    ServiceMethodContext*   pContext = NULL;
    CAtlStringW             strKey   = pwszMethodContext;

    *ppResults = NULL;
    *phrStatus = S_OK;
    pContext = (ServiceMethodContext*) pContextMap->GetContext(strKey);

    if (pContext != NULL)
    {
        if (pContext->m_pResults)
        {
            hr = pContext->m_pResults->QueryInterface(IID_IPortableDeviceValues, (void**)ppResults);
            CHECK_HR(hr, "Failed to QueryInterface IPortableDeviceValues for results");
        }

        if (SUCCEEDED(hr))
        {
            *phrStatus = pContext->m_hrStatus;
        }
        pContextMap->Remove(strKey);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        CHECK_HR(hr, "Failed to get the context for %ws", pwszMethodContext);
    }

    if (FAILED(hr))
    {
        *phrStatus = hr;
    }

    SAFE_RELEASE(pContext);
    return hr;
}

HRESULT WpdServiceMethods::CancelMethod(
    _In_     ContextMap*             pContextMap,
    _In_     LPCWSTR                 pwszMethodContext)
{
    HRESULT                 hr       = S_OK;
    ServiceMethodContext*   pContext = NULL;
    CAtlStringW             strKey   = pwszMethodContext;

    pContext = (ServiceMethodContext*) pContextMap->GetContext(strKey);

    if (pContext != NULL)
    {
        //
        // This is where we will cancel the method invocation associated
        // with this context
        //

        // ....

        // When done ... clean up associated resources
        pContextMap->Remove(strKey);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        CHECK_HR(hr, "Failed to get the context for %ws", pwszMethodContext);
    }

    SAFE_RELEASE(pContext);
    return hr;
}

HRESULT WpdServiceMethods::DispatchMethod(
    _In_         LPCWSTR                 pwszContext,
    _In_         IPortableDeviceValues*  pStartParams,
    _COM_Outptr_ IPortableDeviceValues** ppResults)
{
    HRESULT hr       = S_OK;
    HRESULT hrStatus = S_OK;
    GUID    Method   = GUID_NULL;
    CComPtr<IPortableDeviceValues> pMethodParams;
    CComPtr<IPortableDeviceValues> pMethodResults;

    *ppResults = NULL;

    // Get the method GUID
    hr = pStartParams->GetGuidValue(WPD_PROPERTY_SERVICE_METHOD, &Method);
    CHECK_HR(hr, "Failed to get WPD_PROPERTY_SERVICE_METHOD");

    // Get the method parameters.  These can be optional if the methods don't require parameters
    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = pStartParams->GetIPortableDeviceValuesValue(WPD_PROPERTY_SERVICE_METHOD_PARAMETER_VALUES, &pMethodParams);
        CHECK_HR(hrTemp, "Failed to get WPD_PROPERTY_SERVICE_METHOD_PARAMETER_VALUES (ok if method does not require parameters)");
    }

    // Prepare the results collection
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pMethodResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (SUCCEEDED(hr))
    {
        // Invoke the method
        if (IsEqualGUID(METHOD_FullEnumSyncSvc_BeginSync, Method))
        {
            hrStatus = m_pContactsService->OnBeginSync(pMethodParams, *ppResults);
            CHECK_HR(hrStatus, "BeginSync method failed");
        }
        else if (IsEqualGUID(METHOD_FullEnumSyncSvc_EndSync, Method))
        {
            hrStatus = m_pContactsService->OnEndSync(pMethodParams, *ppResults);
            CHECK_HR(hrStatus, "EndSync method failed");
        }
        else if (IsEqualGUID(MyCustomMethod, Method))
        {
            CComPtr<IPortableDeviceValues> pCustomEventParams;

            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**)&pCustomEventParams);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

            if (SUCCEEDED(hr))
            {
                hrStatus = m_pContactsService->OnMyCustomMethod(pMethodParams, pMethodResults, pCustomEventParams);
                CHECK_HR(hrStatus, "MyCustomMethod method failed");
            }

            if (SUCCEEDED(hr))
            {
                // In addition to a method complete event, we can also send a custom event,
                // for example, to indicate progress of the method
                hr = PostWpdEvent(pStartParams, pCustomEventParams);
                CHECK_HR(hr, "Failed to post custom event");
            }
        }
        else
        {
            hrStatus = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            CHECK_HR(hr, "Unknown method %ws received",CComBSTR(Method));
        }
    }

    // We always want to post a method completion event
    // Even if the method has failed
    {
        CComPtr<IPortableDeviceValues> pEventParams;
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&pEventParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

        if (SUCCEEDED(hr))
        {
            hr = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, WPD_EVENT_SERVICE_METHOD_COMPLETE);
            CHECK_HR(hr, "Failed to set the event id to WPD_EVENT_SERVICE_METHOD_COMPLETE");
        }

        if (SUCCEEDED(hr))
        {
            hr = pEventParams->SetStringValue(WPD_EVENT_PARAMETER_SERVICE_METHOD_CONTEXT, pwszContext);
            CHECK_HR(hr, "Failed to set the method context for WPD_EVENT_SERVICE_METHOD_COMPLETE");
        }

        if (SUCCEEDED(hr))
        {
            hr = PostWpdEvent(pStartParams, pEventParams);
            CHECK_HR(hr, "Failed to post WPD_EVENT_SERVICE_METHOD_COMPLETE");
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = hrStatus;
    }

    if (SUCCEEDED(hr))
    {
        *ppResults = pMethodResults.Detach();
    }

    return hr;
}
