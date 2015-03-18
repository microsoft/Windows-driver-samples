#pragma once

// This class is used to store the context for a specific enumeration.
// Currrently, this is done by storing the object index.
class EnumContext : public IUnknown
{
public:
    EnumContext() :
        m_cRef(1),
        SearchIndex(0)
    {

    }

    ~EnumContext()
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
    DWORD       SearchIndex;
    CAtlStringW ParentID;
private:
    DWORD       m_cRef;
};

class WpdObjectEnumerator
{
public:
    WpdObjectEnumerator();
    ~WpdObjectEnumerator();

    HRESULT Initialize(_In_ FakeDevice *pFakeDevice);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                               _In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    HRESULT OnStartFind(_In_ IPortableDeviceValues*  pParams,
                        _In_ IPortableDeviceValues*  pResults);

    HRESULT OnFindNext(_In_ IPortableDeviceValues*  pParams,
                       _In_ IPortableDeviceValues*  pResults);

    HRESULT OnEndFind(_In_ IPortableDeviceValues*  pParams,
                      _In_ IPortableDeviceValues*  pResults);

private:
    HRESULT CreateEnumContext(
        _In_         ContextMap*         pContextMap,
        _In_         LPCWSTR             pszParentID,
        _Outptr_result_nullonfailure_   LPWSTR* ppszEnumContext);

    HRESULT DestroyEnumContext(
        _In_         ContextMap*         pContextMap,
        _In_         LPCWSTR             pszEnumContext);

    HRESULT GetObjectIDs(
        _In_         ContextMap*         pContextMap,
        _In_         const DWORD         dwNumObjects,
        _In_         LPCWSTR             pszEnumContext,
        _COM_Outptr_ IPortableDevicePropVariantCollection**  ppCollection);

    FakeDevice* m_pFakeDevice;
};
