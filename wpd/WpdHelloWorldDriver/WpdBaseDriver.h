#pragma once

class WpdBaseDriver :
    public IUnknown
{
public:
    WpdBaseDriver();
    virtual ~WpdBaseDriver();

    HRESULT Initialize();
    VOID    Uninitialize();

    HRESULT DispatchWpdMessage(_In_ IPortableDeviceValues* pParams,
                               _In_ IPortableDeviceValues* pResults);

private:
    HRESULT OnGetObjectIDsFromPersistentUniqueIDs(_In_ IPortableDeviceValues* pParams,
                                                  _In_ IPortableDeviceValues* pResults);

public: // IUnknown
    ULONG __stdcall AddRef();

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);

public:
    WpdObjectEnumerator m_ObjectEnum;
    WpdObjectProperties m_ObjectProperties;
    WpdObjectResources  m_ObjectResources;
    WpdCapabilities     m_Capabilities;

private:
    ULONG m_cRef;
};

