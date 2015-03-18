#pragma once

// This class is used to store the context for a specific enumeration.
// Currently, this is done by storing the object index.
class BulkPropertiesContext : public IUnknown
{
public:
    BulkPropertiesContext() :
        Scope(FULL_DEVICE_ACCESS),
        NextObject(0),
        m_cRef(1)
    {

    }

    ~BulkPropertiesContext()
    {

    }

    ACCESS_SCOPE                                  Scope;
    CComPtr<IPortableDevicePropVariantCollection> ObjectIDs;
    DWORD                                         NextObject;
    CComPtr<IPortableDeviceKeyCollection>         Properties;
    CComPtr<IPortableDeviceValuesCollection>      ValuesCollection;

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

class WpdObjectPropertiesBulk
{
public:
    WpdObjectPropertiesBulk();
    ~WpdObjectPropertiesBulk();

    HRESULT Initialize(_In_ FakeDevice *pDevice);

    HRESULT DispatchWpdMessage(
        _In_     REFPROPERTYKEY         Command,
        _In_     IPortableDeviceValues* pParams,
        _In_     IPortableDeviceValues* pResults);

    HRESULT OnGetValuesByObjectListStart(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetValuesByObjectListNext(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetValuesByObjectListEnd(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetValuesByObjectFormatStart(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetValuesByObjectFormatNext(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnGetValuesByObjectFormatEnd(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnSetValuesByObjectListStart(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnSetValuesByObjectListNext(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

    HRESULT OnSetValuesByObjectListEnd(
        _In_    IPortableDeviceValues*  pParams,
        _In_    IPortableDeviceValues*  pResults);

private:
    HRESULT CreateBulkPropertiesContext(
        _In_        ACCESS_SCOPE                          Scope,
        _In_        ContextMap*                           pContextMap,
        _In_        IPortableDevicePropVariantCollection* pObjectIDs,
        _In_        IPortableDeviceKeyCollection*         pProperties,
        _Outptr_result_nullonfailure_    LPWSTR*          ppszBulkPropertiesContext);

    HRESULT CreateBulkPropertiesContext(
        _In_        ACCESS_SCOPE                          Scope,
        _In_        ContextMap*                           pContextMap,
        _In_        REFGUID                               guidObjectFormat,
        _In_        LPCWSTR                               pszParentObjectID,
        _In_        DWORD                                 dwDepth,
        _In_        IPortableDeviceKeyCollection*         pProperties,
        _Outptr_result_nullonfailure_    LPWSTR*          ppszBulkPropertiesContext);

    HRESULT CreateBulkPropertiesContext(
        _In_        ACCESS_SCOPE                          Scope,
        _In_        ContextMap*                           pContextMap,
        _In_        IPortableDeviceValuesCollection*      pValuesCollection,
        _Outptr_result_nullonfailure_    LPWSTR*          ppszBulkPropertiesContext);

    HRESULT DestroyBulkPropertiesContext(
        _In_        ContextMap*                           pContextMap, 
        _In_        LPCWSTR                               pszBulkPropertiesContext);

    FakeDevice* m_pDevice;
};
