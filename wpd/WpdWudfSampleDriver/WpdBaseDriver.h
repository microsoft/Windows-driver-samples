#pragma once

// This class is used to store the connected client information.
// This is for demonstration purposes only - this driver does not
// make use of the information.
class ClientContext : public IUnknown
{
public:
    ClientContext() :
        MajorVersion(0),
        MinorVersion(0),
        Revision(0),
        m_cRef(1)
    {
    }

    ~ClientContext()
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

private:
    DWORD m_cRef;

public:
    CAtlStringW ClientName;
    CAtlStringW EventCookie;
    DWORD       MajorVersion;
    DWORD       MinorVersion;
    DWORD       Revision;
};

class WpdBaseDriver :
    public IUnknown
{
public:
    WpdBaseDriver();
    virtual ~WpdBaseDriver();

    HRESULT Initialize(_In_ LPCWSTR pszPortName, _In_ IPortableDeviceClassExtension* pPortableDeviceClassExtension);
    VOID    Uninitialize();

    HRESULT DispatchWpdMessage(_In_ IPortableDeviceValues* pParams,
                               _In_ IPortableDeviceValues* pResults);

private:
    HRESULT OnSaveClientInfo(_In_ IPortableDeviceValues* pParams,
                             _In_ IPortableDeviceValues* pResults);

    HRESULT OnGetObjectIDsFromPersistentUniqueIDs(_In_ IPortableDeviceValues* pParams,
                                                  _In_ IPortableDeviceValues* pResults);

public: // IUnknown
    ULONG __stdcall AddRef();

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);

private:
    WpdObjectEnumerator                 m_ObjectEnum;
    WpdObjectProperties                 m_ObjectProperties;
    WpdObjectPropertiesBulk             m_ObjectPropertiesBulk;
    WpdObjectResources                  m_ObjectResources;
    WpdObjectManagement                 m_ObjectManagement;
    WpdCapabilities                     m_Capabilities;
    WpdStorage                          m_Storage;
    WpdNetworkConfig                    m_NetworkConfig;
    FakeDevice                          m_FakeDevice;
    CComPtr<IPortableDeviceClassExtension> m_pPortableDeviceClassExtension;
    ULONG m_cRef;
};

