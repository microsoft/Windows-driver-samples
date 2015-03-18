#pragma once

// This context is used for managing reads/writes for data transfer.
// It keeps track of the number of bytes read/written for the current transfer.
class ResourceContext : public IUnknown
{
public:
    ResourceContext() :
        NumBytesTransfered(0),
        m_cRef(1),
        CreateRequest(FALSE)
    {


        Key = WPD_PROPERTY_NULL;
    }

    ~ResourceContext()
    {

    }

    CAtlStringW ObjectID;
    PROPERTYKEY Key;
    DWORD       NumBytesTransfered;
    BOOL        CreateRequest;

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

private:
    DWORD m_cRef;
};

class WpdObjectResources
{
public:
    WpdObjectResources();
    ~WpdObjectResources();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetSupportedResources(_In_ IPortableDeviceValues*  pParams,
                                    _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetAttributes(_In_ IPortableDeviceValues*  pParams,
                            _In_ IPortableDeviceValues*  pResults);

    HRESULT OnOpen(_In_ IPortableDeviceValues*  pParams,
                   _In_ IPortableDeviceValues*  pResults);

    HRESULT OnRead(_In_ IPortableDeviceValues*  pParams,
                   _In_ IPortableDeviceValues*  pResults);

    HRESULT OnWrite(_In_ IPortableDeviceValues*  pParams,
                    _In_ IPortableDeviceValues*  pResults);

    HRESULT OnClose(_In_ IPortableDeviceValues*  pParams,
                    _In_ IPortableDeviceValues*  pResults);

    HRESULT OnDelete(_In_ IPortableDeviceValues* pParams,
                     _In_ IPortableDeviceValues* pResults);

    HRESULT OnCreate(_In_ IPortableDeviceValues* pParams,
                     _In_ IPortableDeviceValues* pResults);

    HRESULT OnRevert(_In_ IPortableDeviceValues* pParams,
                     _In_ IPortableDeviceValues* pResults);

    HRESULT OnSeek(_In_ IPortableDeviceValues* pParams,
                   _In_ IPortableDeviceValues* pResults);

private:
    HRESULT CreateResourceContext(
        _In_     ContextMap*     pContextMap,
        _In_     LPCWSTR         pszObjectID,
        _In_     REFPROPERTYKEY  ResourceKey,
        _In_     BOOL            bCreateRequest,
        _Outptr_ LPWSTR* ppszResourceContext);

    HRESULT DestroyResourceContext(
        _In_     ContextMap*     pContextMap,
        _In_     LPCWSTR         pszResourceContext);

private:

    FakeDevice* m_pFakeDevice;
};
