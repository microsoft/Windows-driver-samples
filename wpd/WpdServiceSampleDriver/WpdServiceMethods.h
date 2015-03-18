#pragma once

class CMethodTask;

// This class is used to store the context for a specific method invocation
class ServiceMethodContext : public IUnknown
{
public:
    ServiceMethodContext();
    ~ServiceMethodContext();

    HRESULT Initialize(
        _In_ WpdServiceMethods*     pServiceMethods,
        _In_ IPortableDeviceValues* pStartParams,
        _In_ LPCWSTR                pwszContext);

    VOID InvokeMethod();

public: // IUnknown
    ULONG __stdcall AddRef()
    {
        InterlockedIncrement((long*) &m_cRef);
        return m_cRef;
    }

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release()
    {
        ULONG ulRefCount = m_cRef - 1;

        if (InterlockedDecrement((long*) &m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return ulRefCount;
    }

    HRESULT __stdcall QueryInterface(
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

public:
    HRESULT                        m_hrStatus;
    CComPtr<IPortableDeviceValues> m_pResults;

private:
    DWORD                          m_cRef;
    CAtlStringW                    m_strContext;
    CMethodTask*                   m_pTask;
    CComPtr<IPortableDeviceValues> m_pStartParameters;
    WpdServiceMethods*             m_pServiceMethods;
};


class CMethodTask
{
public:
    CMethodTask(_In_ ServiceMethodContext* pContext);

    ~CMethodTask();

    HRESULT Run();

    static DWORD ThreadProc(LPVOID pData)
    {
        // Initialize COM
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        {
            ServiceMethodContext* pContext = (ServiceMethodContext*) pData;
            if (pContext != NULL)
            {
                pContext->AddRef();
                pContext->InvokeMethod();
                pContext->Release();
            }

            // Uninitialize COM
            CoUninitialize();
        }
        return 0;
    }
private:
    HANDLE                m_hThread;
    ServiceMethodContext* m_pContext;
};

class WpdServiceMethods
{
public:
    WpdServiceMethods();
    virtual ~WpdServiceMethods();

    HRESULT Initialize(
        _In_    FakeContactsService*    pContactsService);

    // Handler for WPD_COMMAND_SERVICE_METHODS_START_INVOKE
    HRESULT OnStartInvoke(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    // Handler for WPD_COMMAND_SERVICE_METHODS_END_INVOKE
    HRESULT OnEndInvoke(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    // Handler for WPD_COMMAND_SERVICE_METHODS_CANCEL_INVOKE
    HRESULT OnCancelInvoke(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT DispatchMethod(
        _In_         LPCWSTR                 pwszContext,
        _In_         IPortableDeviceValues*  pStartParams,
        _COM_Outptr_ IPortableDeviceValues** ppResults);

private:
    HRESULT StartMethod(
        _In_                            IPortableDeviceValues*  pCommandParams,
        _Outptr_result_nullonfailure_   LPWSTR*                 ppwszMethodContext);

    HRESULT EndMethod(
        _In_                            ContextMap*             pContextMap, 
        _In_                            LPCWSTR                 pwszMethodContext,
        _COM_Outptr_result_maybenull_   IPortableDeviceValues** ppResults,
        _Out_                           HRESULT*                phrStatus);

    HRESULT CancelMethod(
        _In_    ContextMap*             pContextMap, 
        _In_    LPCWSTR                 pwszMethodContext);

private:
    FakeContactsService* m_pContactsService;
};

