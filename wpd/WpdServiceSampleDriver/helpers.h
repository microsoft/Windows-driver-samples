#pragma once

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif

// {CDD18979-A7B0-4D5E-9EB2-0A826805CBBD}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, 0xCDD18979, 0xA7B0, 0x4D5E, 0x9E, 0xB2, 0x0A, 0x82, 0x68, 0x05, 0xCB, 0xBD, 2);
// {9BD949E5-59CF-41AE-90A9-BE1D044F578F}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, 0x9BD949E5, 0x59CF, 0x41AE, 0x90, 0xA9, 0xBE, 0x1D, 0x04, 0x4F, 0x57, 0x8F, 2);
// {4DF6C8C7-2CE5-457C-9F53-EFCECAA95C04}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, 0x4DF6C8C7, 0x2CE5, 0x457C, 0x9F, 0x53, 0xEF, 0xCE, 0xCA, 0xA9, 0x5C, 0x04, 2);
// {67BA8D9E-1DC4-431C-B89C-9D03F7D8C223}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_REQUEST_FILENAME, 0x67BA8D9E, 0x1DC4, 0x431C, 0xB8, 0x9C, 0x9D, 0x03, 0xF7, 0xD8, 0xC2, 0x23, 2);

// Service event and parameters
// {D93102D5-8FED-4A39-AF84-228FE15888D0}
DEFINE_GUID(MyCustomEvent, 0xD93102D5, 0x8FED, 0x4A39, 0xAF, 0x84, 0x22, 0x8F, 0xE1, 0x58, 0x88, 0xD0);
// {D93102D5-8FED-4A39-AF84-228FE15888D0}.2
DEFINE_PROPERTYKEY(MyCustomEventParam0, 0xD93102D5, 0x8FED, 0x4A39, 0xAF, 0x84, 0x22, 0x8F, 0xE1, 0x58, 0x88, 0xD0, 2);
// {D93102D5-8FED-4A39-AF84-228FE15888D0}.3
DEFINE_PROPERTYKEY(MyCustomEventParam1, 0xD93102D5, 0x8FED, 0x4A39, 0xAF, 0x84, 0x22, 0x8F, 0xE1, 0x58, 0x88, 0xD0, 3);

// Service method and parameters
// {ECFC865F-7B43-4D76-9C6B-D67309A3F6F7}
DEFINE_GUID(MyCustomMethod, 0xECFC865F, 0x7B43, 0x4D76, 0x9C, 0x6B, 0xD6, 0x73, 0x09, 0xA3, 0xF6, 0xF7);
// {ECFC865F-7B43-4D76-9C6B-D67309A3F6F7}.2
DEFINE_PROPERTYKEY(MyCustomMethodResult, 0xECFC865F, 0x7B43, 0x4D76, 0x9C, 0x6B, 0xD6, 0x73, 0x09, 0xA3, 0xF6, 0xF7, 2);
// {ECFC865F-7B43-4D76-9C6B-D67309A3F6F7}.3
DEFINE_PROPERTYKEY(MyCustomMethodParam, 0xECFC865F, 0x7B43, 0x4D76, 0x9C, 0x6B, 0xD6, 0x73, 0x09, 0xA3, 0xF6, 0xF7, 3);
// {ECFC865F-7B43-4D76-9C6B-D67309A3F6F7}.4
DEFINE_PROPERTYKEY(MyCustomMethodParamInOut, 0xECFC865F, 0x7B43, 0x4D76, 0x9C, 0x6B, 0xD6, 0x73, 0x09, 0xA3, 0xF6, 0xF7, 4);

// Contact versioning property
// {2B0D5AA4-7EB3-4674-BD36-23FE4C39A2C2}.2
DEFINE_PROPERTYKEY(MyContactVersionIdentifier, 0x2B0D5AA4, 0x7EB3, 0x4674, 0xBD, 0x36, 0x23, 0xFE, 0x4C, 0x39, 0xA2, 0xC2, 2);

// Full Enumeration Sync Replica ID
// {81176f1e-2c42-4b4e-8f79-bc1a7f3da046}
DEFINE_GUID(MyFullEnumSyncReplicaId, 0x81176f1e, 0x2c42, 0x4b4e, 0x8f, 0x79, 0xbc, 0x1a, 0x7f, 0x3d, 0xa0, 0x46);

// Access Scope is a bit mask, where each bit enables access to a particular scope
// for example, contacts service is bit 1.   
// The next scope, if any, will be in bit 2
// Full device access is a combination of all, requires all bits to be set
typedef enum tagACCESS_SCOPE
{
    CONTACTS_SERVICE_ACCESS = 1,
    FULL_DEVICE_ACCESS = 0xFFFFFFFF
}ACCESS_SCOPE;

typedef enum tagFakeDevicePropertyAttributesType
{
    UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,
    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,
} FakeDevicePropertyAttributesType;

typedef struct tagPropertyAttributeInfo
{
    const PROPERTYKEY*                pKey;
    VARTYPE                           Vartype;
    FakeDevicePropertyAttributesType  AttributesType;
    PCWSTR                            wszName;
} PropertyAttributeInfo;

typedef struct tagMethodParameterAttributeInfo
{
    const PROPERTYKEY*                pKey;
    VARTYPE                           Vartype;
    WPD_PARAMETER_USAGE_TYPES         UsageType;
    WpdParameterAttributeForm         Form;
    DWORD                             Order;
    PCWSTR                            wszName;
} MethodParameterAttributeInfo;

typedef struct tagEventParameterAttributeInfo
{
    const GUID*                       pEventGuid;
    const PROPERTYKEY*                pParameter;
    VARTYPE                           Vartype;
} EventParameterAttributeInfo;

typedef struct tagFormatAttributeInfo
{
    const GUID*                       pFormatGuid;
    PCWSTR                            wszName;
} FormatAttributeInfo;


class ContextMap : public IUnknown
{
public:
    ContextMap() :
        m_cRef(1)
    {

    }

    ~ContextMap()
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);

        IUnknown*   pUnk            = NULL;
        POSITION    elementPosition = NULL;

        elementPosition = m_Map.GetStartPosition();
        while(elementPosition != NULL)
        {
            pUnk = m_Map.GetNextValue(elementPosition);
            if(pUnk != NULL)
            {
                pUnk->Release();
            }
        }
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


public: // Context accessor methods

    // If successful, this method AddRef's the context and returns
    // a context key
    HRESULT Add(
        _In_    IUnknown*     pContext,
        _Out_   CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        HRESULT  hr          = S_OK;
        GUID     guidContext = GUID_NULL;
        CComBSTR bstrContext;
        key = L"";

        // Create a unique context key
        hr = CoCreateGuid(&guidContext);
        if (hr == S_OK)
        {
            bstrContext = guidContext;
            if(bstrContext.Length() > 0)
            {
                key = bstrContext;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == S_OK)
        {
            // Insert this into the map
            POSITION  elementPosition = m_Map.SetAt(key, pContext);
            if(elementPosition != NULL)
            {
                // AddRef since we are holding onto it
                pContext->AddRef();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        return hr;
    }

    void Remove(
        const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // Remove the entry for it
            m_Map.RemoveKey(key);

            // Release it
            pContext->Release();
        }
    }

    // Returns the context pointer.  If not found, return value is NULL.
    // If non-NULL, caller is responsible for Releasing when it is done,
    // since this method will AddRef the context.
    IUnknown* GetContext(
        const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // AddRef
            pContext->AddRef();
        }
        return pContext;
    }

private:
    CComAutoCriticalSection         m_CriticalSection;
    CAtlMap<CAtlStringW, IUnknown*> m_Map;
    DWORD                           m_cRef;
};


// This class is used to store the connected client information.
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


class PropVariantWrapper : public tagPROPVARIANT
{
public:
    PropVariantWrapper()
    {
        PropVariantInit(this);
    }

    PropVariantWrapper(LPCWSTR pszSrc)
    {
        PropVariantInit(this);

        *this = pszSrc;
    }

    virtual ~PropVariantWrapper()
    {
        Clear();
    }

    void Clear()
    {
        PropVariantClear(this);
    }

    PropVariantWrapper& operator= (const ULONG ulValue)
    {
        Clear();
        vt      = VT_UI4;
        ulVal   = ulValue;

        return *this;
    }

    PropVariantWrapper& operator= (_In_ LPCWSTR pszSrc)
    {
        Clear();

        pwszVal = AtlAllocTaskWideString(pszSrc);
        if(pwszVal != NULL)
        {
            vt = VT_LPWSTR;
        }
        return *this;
    }

    PropVariantWrapper& operator= (_In_ IUnknown* punkSrc)
    {
        Clear();

        // Need to AddRef as PropVariantClear will Release
        if (punkSrc != NULL)
        {
            vt      = VT_UNKNOWN;
            punkVal = punkSrc;
            punkVal->AddRef();
        }
        return *this;
    }

    void SetErrorValue(const HRESULT hr)
    {
        Clear();
        vt      = VT_ERROR;
        scode   = hr;
    }

    void SetBoolValue(const bool bValue)
    {
        Clear();
        vt      = VT_BOOL;
        if(bValue)
        {
            boolVal = VARIANT_TRUE;
        }
        else
        {
            boolVal = VARIANT_FALSE;
        }
    }
};

HRESULT UpdateDeviceFriendlyName(
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_    LPCWSTR                         wszDeviceFriendlyName);

HRESULT RegisterServices(
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
            const bool                      bUnregister);

HRESULT CheckRequestFilename(
    _In_    LPCWSTR  pszRequestFilename);

DWORD GetResourceSize(
            const UINT uiResource);

PBYTE GetResourceData(
            const UINT uiResource);

HRESULT AddStringValueToPropVariantCollection(
    _In_    IPortableDevicePropVariantCollection* pCollection,
    _In_    LPCWSTR                               wszValue);

HRESULT PostWpdEvent(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams);

HRESULT GetClientContextMap(
    _In_     IPortableDeviceValues*  pParams,
    _Outptr_ ContextMap**            ppContextMap);

HRESULT GetClientContext(
    _In_         IPortableDeviceValues*  pParams,
    _In_         LPCWSTR                 pszContextKey,
    _COM_Outptr_ IUnknown**              ppContext);

HRESULT GetClientEventCookie(
    _In_                      IPortableDeviceValues*  pParams,
    _Outptr_result_maybenull_ LPWSTR*                 ppszEventCookie);

HRESULT AddPropertyAttributesByType(
             const FakeDevicePropertyAttributesType type,
    _In_     IPortableDeviceValues*           pAttributes);

HRESULT SetPropertyAttributes(
    _In_                        REFPROPERTYKEY                  Key,
    _In_reads_(cAttributeInfo)  const PropertyAttributeInfo*    AttributeInfo,
    _In_                        DWORD                           cAttributeInfo,
    _In_                        IPortableDeviceValues*          pAttributes);

HRESULT SetMethodParameterAttributes(
    _In_                        REFPROPERTYKEY                          Parameter,
    _In_reads_(cAttributeInfo)  const  MethodParameterAttributeInfo*    AttributeInfo,
    _In_                        DWORD                                   cAttributeInfo,
    _In_                        IPortableDeviceValues*                  pAttributes);

HRESULT SetEventParameterAttributes(
    _In_                        REFPROPERTYKEY                      Parameter,
    _In_reads_(cAttributeInfo)  const EventParameterAttributeInfo*  AttributeInfo,
    _In_                        DWORD                               cAttributeInfo,
    _In_                        IPortableDeviceValues*              pAttributes);

HRESULT SetEventParameters(
    _In_                        REFGUID                             Event,
    _In_reads_(cAttributeInfo)  const EventParameterAttributeInfo*  AttributeInfo,
    _In_                        DWORD                               cAttributeInfo,
    _In_                        IPortableDeviceKeyCollection*       pParameters);
