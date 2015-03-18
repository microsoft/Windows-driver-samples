#pragma once

// This class is used to store the context for a specific enumeration.
// Currently, this is done by storing the object index.
class ObjectManagementContext  : public IUnknown
{
public:
    ObjectManagementContext() :
        m_cRef(1),
        UpdateRequest(FALSE)
    {

    }

    ~ObjectManagementContext()
    {

    }

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
    CAtlStringW                     Name;
    CComPtr<IPortableDeviceValues>  ObjectProperties;
    BOOL                            UpdateRequest;
private:
    DWORD                           m_cRef;
};

class WpdObjectManagement
{
public:
    WpdObjectManagement();
    ~WpdObjectManagement();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    HRESULT OnDelete(_In_ IPortableDeviceValues*  pParams,
                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnCreateObjectWithPropertiesOnly(_In_ IPortableDeviceValues*  pParams,
                                             _In_ IPortableDeviceValues*  pResults);

    HRESULT OnCreateObjectWithPropertiesAndData(_In_ IPortableDeviceValues*  pParams,
                                                _In_ IPortableDeviceValues*  pResults);

    HRESULT OnWriteObjectData(_In_ IPortableDeviceValues*  pParams,
                              _In_ IPortableDeviceValues*  pResults);

    HRESULT OnRevert(_In_ IPortableDeviceValues*  pParams,
                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnCommit(_In_ IPortableDeviceValues*  pParams,
                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnMove(_In_ IPortableDeviceValues*  pParams,
                   _In_ IPortableDeviceValues*  pResults);

    HRESULT OnCopy(_In_ IPortableDeviceValues*  pParams,
                   _In_ IPortableDeviceValues*  pResults);

    HRESULT OnUpdateObjectWithPropertiesAndData(_In_ IPortableDeviceValues*  pParams,
                                                _In_ IPortableDeviceValues*  pResults);

private:
    HRESULT CreateObjectManagementContext(
        _In_    ContextMap*                     pContextMap,
        _In_    LPCWSTR                         pszObjectName,
        _In_    IPortableDeviceValues*          pObjectProperties,
        _In_    BOOL                            bUpdateRequest,
        _Outptr_result_nullonfailure_   LPWSTR* ppszObjectManagementContext);

    HRESULT DestroyObjectManagementContext(
        _In_    ContextMap*                 pContextMap,
        _In_    LPCWSTR                     pszObjectManagementContext);

    HRESULT CommitNewObject(
        _In_    ObjectManagementContext*    pContext,
        _In_    LPCWSTR                     pszContext,
        _In_    IPortableDeviceValues*      pParams,
        _In_    IPortableDeviceValues*      pResults);

    HRESULT UpdateObject(
        _In_    ObjectManagementContext*    pContext,
        _In_    IPortableDeviceValues*      pParams,
        _In_    IPortableDeviceValues*      pResults);

    FakeDevice* m_pFakeDevice;
};
